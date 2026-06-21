#include "common.h"
#include "functions.h"
#include "Game/entity_events.h"
#include "Game/callback_slot.h"
#include "globals.h"

/* -----------------------------------------------------------------------------
 * Local helpers for the FSM callback-slot install/clear pattern.
 *
 * The engine stores each callback as an 8-byte {markerLo, markerHi, fn} slot
 * scattered through the Entity struct (see include/Game/callback_slot.h).
 * cc1 2.7.2 -O2 emits the install/copy sequence cleanly when the slot is
 * built on the stack and assigned through a scratch local — these macros
 * wrap that boilerplate.
 *
 * Preferred matching idiom (see docs/compiler-quirks.md Quirk 6k):
 *   PaddedSlotPair slot;        (* or PadSlot for the 0x28-frame variant *)
 *   s16  m1;
 *   void (*fn)();
 *   ... preamble (callees, vtable, sprite-table stores) ...
 *   do {} while (0);   // @hack BB boundary - pins scheduler
 *   fn = MyTickFn;     // fn-first vs m1-first is per-function; check fdiff
 *   m1 = -1;
 *   SLOT_STORE(slot.s[0], e->tickMarker,  m1, fn);
 *   fn = MyEventFn;
 *   SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
 *
 * `do {} while (0);` is a zero-instruction basic-block boundary that cc1's
 * sched1 obeys — it replaces the older `__asm__ volatile("" ::: "memory");`
 * + per-fn `__asm__ volatile("" : "=r"(fn) : "0"(fn));` armor for nearly
 * all multi-slot installers. The named `m1`/`fn` locals still drive register
 * coloring (Quirk 6b: fn -> $v1, -1 -> $v0).
 *
 * Single-slot tail installers (e.g. EnemySetWalkSprite) need TWO boundaries:
 * one before `fn = ...;` (pins `sw ra`) and one between `fn` and the slot
 * stores (pins fn coloring).
 *
 * Cases still using the explicit `__asm__ volatile` fn-barrier:
 *   - rand-conditional fn pointers (LaserMonkeyIdleState, etc.)
 *   - register-asm sprite-id patterns (InitEnemyAnimatedWithDeathSpawn)
 *   - pointer-coalesce barriers (Quirk 6i, e.g. InitScrollingLayerEntity)
 * -------------------------------------------------------------------------- */
#define SLOT_CLEAR(scratch, dest_field) ( \
    (scratch).markerLo = 0, \
    (scratch).markerHi = 0, \
    (scratch).fn = NULL, \
    *(CallbackSlot *)&(dest_field) = (scratch) \
)

#define SLOT_STORE(scratch, dest_field, m1_val, fn_val) ( \
    (scratch).markerLo = 0, \
    (scratch).markerHi = (m1_val), \
    (scratch).fn = (fn_val), \
    *(CallbackSlot *)&(dest_field) = (scratch) \
)

extern void *g_pBlbHeapBase;
extern void CheckAndDisableSpawnDataOffscreen(Entity *entity);
extern void CollisionCheckWrapper(Entity *e, s32 a, s32 b, s32 c);
extern void CheckAndDisableChildEntityOffscreen(Entity *e);
extern void CheckAndDisableSpawnRefOffscreen(Entity *e);
extern void EntityOffScreenChildCleanup(Entity *e);
extern void UpdateEntitySoundPanning(Entity *e, u32 sound);
extern void CollectibleTickCallback(Entity *e);
extern void CollectibleTickFinnMode(Entity *e);
extern void EntityEventHandlerIdle(Entity *e);
extern s32 EntityEventHandlerWalk(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntityStateSetWalk(Entity *e);
extern void SoundEmitterTickCallback(Entity *e);
extern void AnimatedEntityToggleSpriteA(Entity *e);
extern void AnimatedEntityToggleSpriteB(Entity *e);
extern void SetAnimationSpriteId(Entity *e, s32 id);
extern void SetEntityTargetFrame(Entity *e, s32 frame);
extern void SetAnimationFrameCallback(Entity *e, u32 packed);
extern void EntitySetRenderFlags(Entity *e, u32 flags);
extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern Entity *InitEntityWithSprite(Entity *entity, u8 *spriteDef, s32 z, s16 x, s16 y);
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern Entity *InitCollectibleEntity(Entity *e, u8 *spawn);
extern void *CHILD_SPRITE_PARENT_VTABLE asm("D_80010C44");
extern void *PROJECTILE_PATH_ENTITY_VTABLE asm("D_80010C64");
extern void *PATH_HAZARD_ENTITY_VTABLE asm("D_80010CE4");
extern void *PATH_ENEMY_SOUND_VTABLE asm("D_80010DA4");
extern void *COLLECTIBLE_ENTITY_VTABLE asm("D_80010DE4");
extern void *SIMPLE_FREE_ONLY_ENTITY_VTABLE asm("D_80010E04");
extern void *CAMERA_TRACKING_ENTITY_VTABLE asm("D_80011048");
extern void *INDEXED_SPRITE_ENTITY_VTABLE asm("D_80011068");
extern void *CAMERA_HELPER_ENTITY_VTABLE asm("D_80011088");
extern void *TRIGGER_ZONE_ENTITY_VTABLE asm("D_800110A8");
extern void *SWITCH_BLOCK_ENTITY_VTABLE asm("D_800110E8");
extern void *SCALED_MOVING_ENTITY_VTABLE asm("D_80011108");
extern void *BOUNCABLE_CLAY_ENTITY_VTABLE asm("D_80011128");
extern void *DIRECTIONAL_SCALED_ENTITY_VTABLE asm("D_80011148");
extern void *SCALED_PLATFORM_ENTITY_VTABLE asm("D_80011168");
extern void *ALT_COLLECTIBLE_ENTITY_VTABLE asm("D_80011188");
extern void *CHECKPOINT_ENTITY_VTABLE asm("D_800111A8");
extern void *ENTITY_FREE_ONLY_VTABLE asm("D_800111C8");
extern void *ALT_PATH_FOLLOWING_ENTITY_VTABLE asm("D_800111E8");
extern void *ENEMY_ENTITY_VTABLE asm("D_80011228");
extern void *ENEMY_FREE_ONLY_ENTITY_VTABLE asm("D_80011248");
u32 ENEMY_ANIMATED_CALLBACK_MARKER asm("D_800A5AD0");
EntityCallback ENEMY_ANIMATED_CALLBACK_FN asm("D_800A5AD4");
extern void RemoveEntityFromAllLists(GameState *gs, s32 idx);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void EntitySetCallback(Entity *e, u32 marker, EntityCallback fn);
extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);
extern void AIEntityRandomBehaviorTick(Entity *e);
extern s32 EntityEventHandlerWithRandomWalk(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerWithDelayedWalk(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerSpawnProjectile(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerWithCountdownToWalk(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerSpawnParticle(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerCountdownToWalkWithSprite(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerTimerCountdown(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerAnimationSwitch(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntityFallingGravityWithCollision(Entity *e);
extern void ApplyAnimationPositionOffsets(Entity *e);
extern void CollectibleSparkleTickCallback(Entity *e);
extern void TripleLaserMonkeyDeathTick(Entity *e);
extern s32 EntityEventHandler0x1001_1002_1008(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandler0x1001_1002_1008_V2(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntityGroundSnapWithAnimation(Entity *e);
extern void SetEntityFacingDirection(Entity *e, s32 dir);
extern void SetAnimationLoopFrame(Entity *e, u32 frame);
extern void SetAnimationSpriteCallback(Entity *e, u32 spriteId);
extern void SetAnimationFrameIndex(Entity *e, s32 frame);
void FreeEntityNoTeardown_80041468(Entity *e, u32 size);
void EntityUpdateWithCollisionWrapper(Entity *e);
void FreeEntityNoTeardown_80045eb4(Entity *e, u32 size);
void FreeEntityNoTeardown_80046d28(Entity *e, u32 size);
void StartAnimationSequence(SpriteEntity *entity, s32 animData, s16 startFrame);
void LaserMonkeyIdleState(Entity *e);
void InitEnemyFallingState(Entity *e);
void EnemyDeathWithParticles(Entity *e);
void CollectibleIdleState(Entity *e);
void InitConditionalCollectibleEntity(Entity *e);
void CollectibleWalkState(Entity *e);
void InitEntityWithDeathSpawn(Entity *e);
void InitEnemyAnimatedWithDeathSpawn(Entity *e);
void ProjectilePathFollowerTick(Entity *e);
void EntityTimedStateSwitchTick(Entity *e);
void EntityUpdateWithCollisionOffscreen(Entity *e);
void EntityConditionalActivateTick();
void PlatformTimerTickCallback();
s32 EntityNullEventHandler(void);

/* gp_rel tentative defs (resolved via the .sdata blob's strong defs). */
u32 SOUND_EMITTER_STUN_EXPIRED_STATE_MARKER asm("D_800A5A68");
EntityCallback SOUND_EMITTER_STUN_EXPIRED_STATE_CALLBACK asm("D_800A5A6C");
u32 PLATFORM_ACTIVATE_BY_FRAME_STATE_MARKER asm("D_800A5AE8");
EntityCallback PLATFORM_ACTIVATE_BY_FRAME_STATE_CALLBACK asm("D_800A5AEC");
u32 HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B08");
EntityCallback HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B0C");
u32 HAZARD_READY_STATE_MARKER asm("D_800A5B10");
EntityCallback HAZARD_READY_STATE_CALLBACK asm("D_800A5B14");

typedef struct CollectibleSpawnData {
    /* 0x00 */ u8 pad0[8];
    /* 0x08 */ s16 x;
    /* 0x0A */ u16 y;
} CollectibleSpawnData;

typedef struct TimedCollectibleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u16 timer;
} TimedCollectibleEntity;

typedef struct EnemyTimerStateEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u16 stateTimer;
    /* 0x106 */ u8 pad106[0xA];
    /* 0x110 */ u8 stateDelay;
    /* 0x111 */ u8 pad111;
    /* 0x112 */ u8 walkDelay;
    /* 0x113 */ u8 pad113;
    /* 0x114 */ u32 *spriteIds;     /* Per-state sprite id table (>= 6 entries: sparkle, walk, sparkleCollect, idle, ?, attack) */
} EnemyTimerStateEntity;

/* Falling-enemy state slab. The +0x110 word is cleared atomically (treated
 * as a single u32 by InitEnemyFallingState even though +0x111-0x113 are
 * never used as a single 32-bit value elsewhere) and +0x116 holds a
 * recovery/state s16 zeroed in the same sequence. */
typedef struct EnemyFallingEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ u32 stateBundle;     /* Atomic-zeroed 4-byte state slab */
    /* 0x114 */ u8 pad114[2];
    /* 0x116 */ s16 recoveryState;
    /* 0x118 */ u8 padFall118[1];
    /* 0x119 */ u8 fallVariant;      /* Selects sprite-id / direction in the if (((u8 *)e)[0x119]) chain */
} EnemyFallingEntity;

/* Entity with embedded child-entity ref + SPU voice id, used by
 * DestroyEntityWithSoundAndChild. */
typedef struct EntityWithSoundAndChild {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ s32 childEntityId;
    /* 0x114 */ u8 pad114[4];
    /* 0x118 */ s32 voiceId;
} EntityWithSoundAndChild;

/* Entity with embedded child-entity ref at +0x104, used by
 * DestroyEntityWithChildRemoval (checkpoint variants). */
typedef struct EntityWithChild104 {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ s32 childEntityId;
} EntityWithChild104;

/* Spawn record referenced by the platform-activation-style entities at
 * Entity+0x100. Field +0x04 holds a sprite-id; field +0x0C is a phase
 * offset added to g_pGameState->frame_counter for the periodic toggle. */
typedef struct PlatformActivationSpawnRef {
    /* 0x00 */ u8 pad00[4];
    /* 0x04 */ u32 spriteId;
    /* 0x08 */ u8 pad08[4];
    /* 0x0C */ s32 framePhaseOffset;
} PlatformActivationSpawnRef;

typedef struct PlatformActivationRefEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ PlatformActivationSpawnRef *spawn;
} PlatformActivationRefEntity;

typedef struct SoundEmitterEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x20];
    /* 0x120 */ u32 *spriteIdPtr;
    /* 0x124 */ u16 stunTimer;
    /* 0x126 */ u8 pad126[2];
    /* 0x128 */ s32 voiceId;
} SoundEmitterEntity;

typedef struct ConditionalPhaseEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ u8 framePhase;
} ConditionalPhaseEntity;

typedef struct HazardVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ s32 voiceId;
} HazardVoiceEntity;

typedef struct HazardTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ u16 timer;
} HazardTimerEntity;

typedef struct TimedByteEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u8 timer;
} TimedByteEntity;

typedef struct TimerTileRecord {
    /* 0x00 */ u8 pad0[0x12];
    /* 0x12 */ u16 tileId;
} TimerTileRecord;

typedef struct TimedByteWithTileEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u8 timer;
    /* 0x105 */ u8 pad105[3];
    /* 0x108 */ TimerTileRecord *tileRecord;
} TimedByteWithTileEntity;

typedef struct DeathSpawnTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x12];
    /* 0x112 */ u16 deathTimer;
} DeathSpawnTimerEntity;

typedef struct SoundPanningEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x18];
    /* 0x118 */ u32 soundId;
} SoundPanningEntity;

typedef struct PlatformActivationSpawn {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ s32 activateFrame;
} PlatformActivationSpawn;

typedef struct PlatformActivationEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[8];
    /* 0x108 */ PlatformActivationSpawn *spawn;
} PlatformActivationEntity;

typedef struct SwitchBlockSpawn {
    /* 0x00 */ u8 pad0[0x12];
    /* 0x12 */ u16 tileId;
} SwitchBlockSpawn;

typedef struct SwitchFlagEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ SwitchBlockSpawn *spawn;
} SwitchFlagEntity;

typedef struct BackgroundSparkleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 contactFlag;
    /* 0x101 */ u8 revealTimer;
} BackgroundSparkleEntity;

typedef struct BackgroundSparkleChildContext {
    /* 0x00 */ u8 pad0[0xA];
    /* 0x0A */ u8 activeFlag;
} BackgroundSparkleChildContext;

/* Subset of the per-entity sprite-render context (the `spriteContext` field
 * on Entity); only the fields touched by InitCollectibleEntity_Alt's TPage
 * setup are mapped here. */
typedef struct SpriteRenderContext {
    /* 0x00 */ u8  pad00[0x0A];
    /* 0x0A */ u8  activeFlag;       /* Set to 1 to enable rendering of this sprite. */
    /* 0x0B */ u8  pad0B[5];
    /* 0x10 */ s16 vramX;            /* Source X in VRAM (pixels) */
    /* 0x12 */ s16 vramY;            /* Source Y in VRAM (pixels) */
    /* 0x14 */ u8  pad14[0x10];
    /* 0x24 */ s16 tpageBits;        /* Cached GetTPage result */
    /* 0x26 */ u8  pad26[0xC];
    /* 0x32 */ u8  colorMode;        /* Color depth code passed to GetTPage */
} SpriteRenderContext;

INCLUDE_ASM("asm/nonmatchings/enemies", LineSegmentIntersectsRect);

Entity *InitCollectibleEntityFromSpawn(Entity *e, CollectibleSpawnData *spawn, u32 spriteId) {
    InitEntitySprite(e, spriteId, 0x3CA,
                     spawn->x,
                     (s16)(spawn->y - 1), 0);
    e->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, (u8 *)spawn);
    return e;
}

/* Same shape as InitCollectibleEntityFromSpawn but takes a sprite-table
 * pointer (resolved through InitEntityWithSprite) instead of a packed
 * sprite-ID, and uses z=0x3CA. */
Entity *CreateCollectibleFromSpawn(Entity *e, CollectibleSpawnData *spawn, u8 *spriteDef) {
    InitEntityWithSprite(e, spriteDef, 0x3CA, spawn->x, (s16)(spawn->y - 1));
    e->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, (u8 *)spawn);
    return e;
}

Entity *InitCollectibleEntityDirect(Entity *e, u32 spriteId, s16 x, s16 y) {
    InitEntitySprite(e, spriteId, 0x3CA, x, y, 0);
    e->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, NULL);
    return e;
}

Entity *CreateCollectibleAtPosition(Entity *e, u8 *spriteDef, s16 x, s16 y) {
    InitEntityWithSprite(e, spriteDef, 0x3CA, x, y);
    e->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, NULL);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CalcEntityYFromTileHeight);

INCLUDE_ASM("asm/nonmatchings/enemies", UpdateCollectibleTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckCollectibleOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickCallback);

void TimedCollectibleTickCallback(TimedCollectibleEntity *e) {
    if (e->timer != 0) {
        e->timer -= 1;
        if (e->timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    CollectibleTickCallback((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickFinnMode);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerIdle);

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnCollectibleParticles);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleWithFlags);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleSparkleTickCallback);

void TimedSparkleCollectibleTick(TimedCollectibleEntity *e) {
    if (e->timer != 0) {
        e->timer -= 1;
        if (e->timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    CollectibleSparkleTickCallback((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", AIEntityRandomBehaviorTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandler0x1001_1002_1008);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandler0x1001_1002_1008_V2);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithRandomWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithDelayedWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityGroundSnapWithAnimation);

void EntityStartWalkWithTimer0x2d(EnemyTimerStateEntity *e) {
    e->walkDelay = (rand() & 0xF) + 4;
    e->stateTimer = 0x2D;
    EntityStateSetWalk((Entity *)e);
}

void EntityStartWalkWithTimer10(EnemyTimerStateEntity *e) {
    e->walkDelay = (rand() & 0xF) + 4;
    e->stateTimer = 0xA;
    EntityStateSetWalk((Entity *)e);
}

void EntityStateSetWalk(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;
    do {} while (0);
    slot.s[0].markerLo = 0; slot.s[0].markerHi = 0; slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->renderMarker = slot.s[0];
    fn = AIEntityRandomBehaviorTick;
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = EntityEventHandlerWalk;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[1], 1);
}

void EntityStateSetIdle(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;
    do {} while (0);
    slot.s[0].markerLo = 0; slot.s[0].markerHi = 0; slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->renderMarker = slot.s[0];
    fn = TimedSparkleCollectibleTick;
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = EntityEventHandlerWithRandomWalk;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[3], 1);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetRandomBehavior);

void EntityStateSetAttack(EnemyTimerStateEntity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;
    e->stateDelay = 3;
    do {} while (0);
    slot.s[0].markerLo = 0; slot.s[0].markerHi = 0; slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->sprite.base.renderMarker = slot.s[0];
    fn = TimedSparkleCollectibleTick;
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s[0];
    fn = EntityEventHandlerWithDelayedWalk;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s[0];
    spriteIds = e->spriteIds;
    SetEntitySpriteId((Entity *)e, spriteIds[5], 1);
    SetAnimationLoopFrame((Entity *)e, 0x1084280);
    SetAnimationSpriteCallback((Entity *)e, 0x2421405);
    SetAnimationFrameIndex((Entity *)e, 0);
}

void EntitySetSparkleCollectibleState(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;

    SetEntityFacingDirection(e, 2);
    fn = ApplyAnimationPositionOffsets;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->renderMarker = slot.s;
    fn = CollectibleSparkleTickCallback;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandler0x1001_1002_1008_V2;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[2], 1);
    SetAnimationFrameIndex(e, 2);
}

void EntityStateSetSparkle(Entity *e);

void EntitySetSparkleDelay3(EnemyTimerStateEntity *e) {
    e->stateDelay = 3;
    EntityStateSetSparkle((Entity *)e);
}

void EntitySetSparkleDelay2(EnemyTimerStateEntity *e) {
    e->stateDelay = 2;
    EntityStateSetSparkle((Entity *)e);
}

void EntitySetSparkleDelay1(EnemyTimerStateEntity *e) {
    e->stateDelay = 1;
    EntityStateSetSparkle((Entity *)e);
}

void EntityStateSetSparkle(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;

    ((u8 *)e)[0x111] = 1;
    do {} while (0);
    fn = EntityGroundSnapWithAnimation;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->renderMarker = slot.s;
    fn = CollectibleSparkleTickCallback;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandler0x1001_1002_1008;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[0], 1);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationSpriteCallback(e, 0x2421405);
    SetAnimationFrameIndex(e, 0);
}

extern u8 ENEMY_ANIM_SEQUENCE_4A_DATA[] asm("D_8009B55C");
extern u8 ENEMY_ANIM_SEQUENCE_4B_DATA[] asm("D_8009B57C");
extern u8 ENEMY_ANIM_SEQUENCE_4C_DATA[] asm("D_8009B59C");

void StartAnimSequence4A(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ENEMY_ANIM_SEQUENCE_4A_DATA, 4);
}

void StartAnimSequence4B(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ENEMY_ANIM_SEQUENCE_4B_DATA, 4);
}

void StartAnimSequence4C(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ENEMY_ANIM_SEQUENCE_4C_DATA, 4);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEnemy);

void DestroySoundEmitterEntity(SoundEmitterEntity *e, u32 flags) {
    e->sprite.base.collisionVtable = &PATH_ENEMY_SOUND_VTABLE;
    StopSPUVoice(e->voiceId);
    e->sprite.base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterTickCallback);

void SoundEmitterStunnedTickCallback(SoundEmitterEntity *e) {
    if (e->stunTimer != 0) {
        e->stunTimer -= 1;
        if (e->stunTimer == 0) {
            EntitySetState((Entity *)e, SOUND_EMITTER_STUN_EXPIRED_STATE_MARKER,
                           SOUND_EMITTER_STUN_EXPIRED_STATE_CALLBACK);
        }
    }
    EntityUpdateCallback((Entity *)e);
    CheckCollectibleOffscreen((Entity *)e);
}

s32 EntitySimpleEventPassthrough(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFollowPathWithWrapping);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyStartMovingWithSound);

/* Re-arms a sound-emitter enemy to its walking-and-emitting state:
 * installs EntityEventHandlerWalk on the +0x08 event slot and
 * SoundEmitterTickCallback on the +0x00 tick slot, then switches the
 * sprite to *e->spriteIdPtr (sprite-id table loaded via the +0x120
 * pointer). Called when the enemy transitions back to active state. */
void EntityInitSoundEmitterState(SoundEmitterEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())EntityEventHandlerWalk;
    do {} while (0);
    m1 = -1;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
    fn = SoundEmitterTickCallback;
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    SetEntitySpriteId((Entity *)e, *e->spriteIdPtr, 1);
}

/* Sound-emitter "enter idle/stunned" entry: faces the enemy down (dir=2),
 * installs the idle/sound-emit handlers, switches sprite to the second
 * id in the table (e->spriteIdPtr[1]), and queues EntityInitSoundEmitterState
 * as the next state callback (so the enemy automatically re-arms after
 * the stun timer). */
void EnemyEnterSoundEmitterState(SoundEmitterEntity *e) {
    TripadSlot slot;
    void (*fn)();
    s16 m1;
    Entity *entity = (Entity *)e;
    SpriteEntity *spriteEntity = (SpriteEntity *)e;

    SetEntityFacingDirection(entity, 2);
    fn = (void (*)())EntityEventHandlerIdle;
    m1 = -1;
    SLOT_STORE(slot.s, entity->eventMarker, m1, fn);
    fn = SoundEmitterTickCallback;
    SLOT_STORE(slot.s, entity->tickMarker, m1, fn);
    SetEntitySpriteId(entity, e->spriteIdPtr[1], 1);
    fn = EntityInitSoundEmitterState;
    SLOT_STORE(slot.s, spriteEntity->queuedStateMarker, m1, fn);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitSpecialPickupEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TripleLaserMonkeyDeathTick);

void TripleLaserMonkeyConditionalTick(ConditionalPhaseEntity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == e->framePhase) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    TripleLaserMonkeyDeathTick((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithCountdownToWalk);

void LaserMonkeyWalkState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = TripleLaserMonkeyConditionalTick;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandlerWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x60B98CBD, 1);
    fn = LaserMonkeyIdleState;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

void InitTripleLaserMonkeyAttackState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = TripleLaserMonkeyConditionalTick;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    ((u8 *)e)[0x111] = 3;
    do {} while (0);
    fn = EntityEventHandlerWithCountdownToWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x64BB1CBE, 1);
    fn = LaserMonkeyIdleState;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

void LaserMonkeyIdleState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();
    void (*nextFn)();

    do {} while (0);
    fn = TripleLaserMonkeyDeathTick;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandlerIdle;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x60B8BC9C, 1);
    if (rand() & 1) {
        nextFn = InitTripleLaserMonkeyAttackState;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch instead of being hoisted above the if (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    } else {
        nextFn = LaserMonkeyWalkState;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch instead of being hoisted above the if (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    }
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = nextFn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitWalkingCollectibleEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyDeathWithParticles);

void EntityTimerDeathWithParticles(TimedCollectibleEntity *e) {
    if (e->timer != 0) {
        e->timer -= 1;
        if (e->timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    EnemyDeathWithParticles((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnParticle);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFallingGravityWithCollision);

void EnemyPatrolState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PaddedSlotPair slot;
    s16 m1;
    s16 m2;
    void (*fn)();
    u32 spriteId;
    register Entity *callArg asm("$4");

    ((EnemyTimerStateEntity *)e)->stateTimer = 10;
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    fn = EntityTimerDeathWithParticles;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = EntityEventHandlerWalk;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    /* Pin e into $a0 early so SetEntitySpriteId's `move a0,s0` lands in the
     * branch delay slot of the lbu below.  The `$a1` clobber forces GCC to
     * re-materialize the high half of the sprite-id immediate in each arm
     * instead of hoisting a shared `lui` above the branch. */
    callArg = e;
    if (((u8 *)e)[0x119]) {
        /* @hack: clobber $a1 in each arm so cc1 re-materializes the sprite-id `lui` per branch (Quirk 6e), instead of hoisting a shared `lui` above the if. */
        __asm__ volatile("" ::: "$5");
        spriteId = 0x60B93CBD;
    } else {
        /* @hack: clobber $a1 in each arm so cc1 re-materializes the sprite-id `lui` per branch (Quirk 6e), instead of hoisting a shared `lui` above the if. */
        __asm__ volatile("" ::: "$5");
        spriteId = 0x60B9BCBD;
    }
    SetEntitySpriteId(callArg, spriteId, 1);
    fn = InitEnemyFallingState;
    do {} while (0);
    m2 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m2; slot.s[0].fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s[0];
}

void InitEnemyFallingState(Entity *e) {
    PadSlot slot;
    register void (*renderFn)() asm("$3");
    register void (*tickFn)() asm("$7");
    register void (*eventFn)() asm("$8");
    s16 m1;
    u32 spriteId;
    u32 oldFlag;
    register Entity *callArg asm("$4");

    renderFn = EntityFallingGravityWithCollision;
    tickFn = EnemyDeathWithParticles;
    /* @hack: pin renderFn/tickFn to their declared `register asm("$3"/"$7")` slots before the stores below (Quirk 6e multi-output barrier). */
    __asm__ volatile("" : "=r"(renderFn), "=r"(tickFn) : "0"(renderFn), "1"(tickFn));
    oldFlag = ((u8 *)e)[0x119];
    /* @hack: force oldFlag into a temp before the *e + 0x116/0x110 zeroing so the `lbu` precedes the stores (matches target load ordering). */
    __asm__ volatile("" : "=r"(oldFlag) : "0"(oldFlag));
    eventFn = EntityEventHandlerSpawnParticle;
    /* @hack: pin eventFn into $8 (register asm above) ahead of the SLOT_STORE chain. */
    __asm__ volatile("" : "=r"(eventFn) : "0"(eventFn));
    ((EnemyFallingEntity *)e)->recoveryState = 0;
    ((EnemyFallingEntity *)e)->stateBundle = 0;
    /* @hack: memory fence orders the +0x116/+0x110 zeroing BEFORE the `[0x119] = (oldFlag < 1)` write (cc1 otherwise reorders the byte store ahead). */
    __asm__ volatile("" ::: "memory");
    ((u8 *)e)[0x119] = (oldFlag < 1);
    m1 = -1;
    /* @hack: SLOT_STORE block reproduces the original render/tick/event install ordering needed for byte-match (the register-asm tickFn/eventFn pins above feed directly into these stores). */
    SLOT_STORE(slot.s, e->renderMarker, m1, renderFn);
    SLOT_STORE(slot.s, e->tickMarker,   m1, tickFn);
    SLOT_STORE(slot.s, e->eventMarker,  m1, eventFn);
    /* See EnemyPatrolState for why $a0 is pinned early and $a1 is clobbered. */
    callArg = e;
    if (((u8 *)e)[0x119]) {
        /* @hack: clobber $a1 in each arm so cc1 re-materializes the sprite-id `lui` per branch (Quirk 6e). */
        __asm__ volatile("" ::: "$5");
        spriteId = 0x60A91C9C;
    } else {
        /* @hack: clobber $a1 in each arm so cc1 re-materializes the sprite-id `lui` per branch (Quirk 6e). */
        __asm__ volatile("" ::: "$5");
        spriteId = 0x61A91C9C;
    }
    SetEntitySpriteId(callArg, spriteId, 1);
    SetEntityTargetFrame(e, 0x17);
    SetAnimationSpriteId(e, 0x18);
    SetAnimationFrameIndex(e, 0);
}

void EnemyDeathState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PaddedSlotPair slot;
    s16 m1;
    s16 m2;
    void (*fn)();
    u32 spriteId;
    register Entity *callArg asm("$4");

    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    fn = EnemyDeathWithParticles;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = EntityEventHandlerIdle;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    /* See EnemyPatrolState for why $a0 is pinned early and $a1 is clobbered. */
    callArg = e;
    if (((u8 *)e)[0x119]) {
        /* @hack: clobber $a1 in each arm so cc1 re-materializes the sprite-id `lui` per branch (Quirk 6e). */
        __asm__ volatile("" ::: "$5");
        spriteId = 0x60AA1C9C;
    } else {
        /* @hack: clobber $a1 in each arm so cc1 re-materializes the sprite-id `lui` per branch (Quirk 6e). */
        __asm__ volatile("" ::: "$5");
        spriteId = 0x62AA1C9C;
    }
    SetEntitySpriteId(callArg, spriteId, 1);
    fn = EnemyPatrolState;
    do {} while (0);
    m2 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m2; slot.s[0].fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s[0];
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckEntityBehindCamera);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSnoBloEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnProjectile);

void InitCollectibleTimer0x1e_SpriteC(TimedCollectibleEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->timer = 0x1E;
    do {} while (0);
    fn = TimedCollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = EntityEventHandlerWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x6B9A6F, 1);
}

void InitCollectibleTimer0x1e(TimedCollectibleEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->timer = 0x1E;
    do {} while (0);
    fn = TimedCollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = EntityEventHandlerWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x21A81D54, 1);
}

void InitCollectibleTimer0x3c_SpriteA(TimedCollectibleEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->timer = 0x3C;
    do {} while (0);
    fn = TimedCollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = EntityEventHandlerWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x6B9A6F, 1);
}

void InitCollectibleTimer0x3c_SpriteB(TimedCollectibleEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->timer = 0x3C;
    do {} while (0);
    fn = TimedCollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = EntityEventHandlerWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x21A81D54, 1);
}

void InitShooterEnemyStateA(EnemyTimerStateEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->pad111 = 0x2F;
    do {} while (0);
    fn = CollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = EntityEventHandlerSpawnProjectile;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0xA0299A4D, 1);
}

void InitShooterEnemyStateB(EnemyTimerStateEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->pad111 = 0x14;
    do {} while (0);
    fn = CollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = EntityEventHandlerSpawnProjectile;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x21229C5C, 1);
}

void InitCollectibleIdleState(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    m1 = -1;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandlerIdle;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x53209C1C, 1);
}

void InitCollectibleIdleStateB(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    m1 = -1;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandlerIdle;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x20211E5F, 1);
}

extern u8 ENEMY_ANIM_SEQUENCE_3A_DATA[] asm("D_8009B5BC");
extern u8 ENEMY_ANIM_SEQUENCE_3B_DATA[] asm("D_8009B5D4");
extern u8 ENEMY_ANIM_SEQUENCE_9_FRAME_DATA[] asm("D_8009B5EC");

void StartAnimSequence3A(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ENEMY_ANIM_SEQUENCE_3A_DATA, 3);
}

void StartAnimSequence3B(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ENEMY_ANIM_SEQUENCE_3B_DATA, 3);
}

void StartAnimSequence9Frames(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ENEMY_ANIM_SEQUENCE_9_FRAME_DATA, 9);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitSoundEmittingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnWalkingEnemy);

/* Animated-decor toggle state A: half of a bidirectional A<->B sprite
 * toggle (entity types 112-114 "AnimatedDecor", e.g. SoundPlatform).
 * Reads the extended EntityDefinition pointer at +0x100 to pick a
 * "speed/step" byte: 8 if entityType == 0x2F (SoundPlatform family),
 * otherwise 3 - stored at +0x110. Then loads the variant-A sprite
 * (0x35289FAE) and standard animation triad, and queues B as the next
 * state so the next pulse swaps to the sister function.
 *
 * SHELVED (15-byte diff): structurally matches with body:
 *     void *def = *(void **)((u8 *)e + 0x100);
 *     ((u8 *)e)[0x110] = (*(u16 *)((u8 *)def + 0x12) == 0x2F) ? 8 : 3;
 *     do {} while (0);
 *     SetEntitySpriteId(e, 0x35289FAE, 1);
 *     SetAnimationLoopFrame(e, 0x1084280);
 *     SetAnimationSpriteCallback(e, 0x2421405);
 *     SetAnimationFrameIndex(e, 0);
 *     fn = AnimatedEntityToggleSpriteB;
 *     do {} while (0);
 *     m1 = -1;
 *     slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
 *     *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
 * but cc1 picks $a0 for the (3|8) conditional result while TARGET picks
 * $v0; the values are stored to +0x110 in the next instruction so the
 * register choice is binary-different but semantically equivalent. No
 * known C idiom forces cc1 to prefer $v0 over $a0 for a short-lived
 * compare result immediately stored to memory. */
INCLUDE_ASM("asm/nonmatchings/enemies", AnimatedEntityToggleSpriteA);

INCLUDE_ASM("asm/nonmatchings/enemies", AnimatedEntityToggleSpriteB);

/* Enemy state init for the "looping animation" mode (e.g. a fan, spinning
 * thing, or animated decor): sets the +0x110 byte flag, switches sprite
 * to 0x212A9C2D, configures the animation loop range + sprite callback,
 * resets the frame index, and installs AnimatedEntityToggleSpriteA on the
 * queued-state slot (+0x98) so the next state pulse toggles the second
 * sprite. */
void EnemySetLoopingAnimation(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e)[0x110] = 1;
    SetEntitySpriteId(e, 0x212A9C2D, 1);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationSpriteCallback(e, 0x2421405);
    SetAnimationFrameIndex(e, 0);
    fn = AnimatedEntityToggleSpriteA;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitTimerBasedMenuEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySpawnerTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingHazard);

void DestroySoundEntityWithVoice(HazardVoiceEntity *e, u32 flags) {
    e->sprite.base.collisionVtable = &PATH_HAZARD_ENTITY_VTABLE;
    StopSPUVoice(e->voiceId);
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterIdleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityPathFollowerWithTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", InitLevelStateCollectible);

void ConditionalCollectibleTick(ConditionalPhaseEntity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == e->framePhase) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    CollectibleTickCallback((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerCountdownToWalkWithSprite);

void CollectibleWalkState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = ConditionalCollectibleTick;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandlerWalk;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0xED209C94, 1);
    fn = CollectibleIdleState;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

void InitConditionalCollectibleEntity(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = ConditionalCollectibleTick;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    ((u8 *)e)[0x111] = 3;
    do {} while (0);
    fn = EntityEventHandlerCountdownToWalkWithSprite;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x6D209C95, 1);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationSpriteCallback(e, 0x2421405);
    SetAnimationFrameIndex(e, 0);
    fn = CollectibleIdleState;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

void CollectibleIdleState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();
    void (*nextFn)();

    do {} while (0);
    fn = CollectibleTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntityEventHandlerIdle;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x6C289C1C, 1);
    if (rand() & 1) {
        nextFn = InitConditionalCollectibleEntity;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    } else {
        nextFn = CollectibleWalkState;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    }
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = nextFn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitAnimatedTimedCollectible);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleFrameConditionalTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleVariant);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithDeathSpawn);

void EntityTimerCountdownDeathSpawn(DeathSpawnTimerEntity *e) {
    if (e->deathTimer != 0) {
        e->deathTimer -= 1;
        if (e->deathTimer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    InitEntityWithDeathSpawn((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerAnimationSwitch);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerTimerCountdown);

void InitEntityRandomIdleOrAnimated(Entity *e) {
    if (rand() & 1) {
        InitEntityState_Animated(e);
    } else {
        InitEntityState_Idle(e);
    }
}

void InitEntityState_Idle(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    void (*nextFn)();

    ((DeathSpawnTimerEntity *)e)->deathTimer = 0x3C;
    do {} while (0);
    fn = EntityTimerCountdownDeathSpawn;
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = EntityEventHandlerWalk;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    SetEntitySpriteId(e, 0x60181A0C, 1);
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    if (((u8 *)e)[0x110]) {
        nextFn = InitEnemyAnimatedWithDeathSpawn;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    } else {
        nextFn = InitEntityRandomIdleOrAnimated;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    }
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = nextFn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s[0];
}

void InitEnemyAnimatedWithDeathSpawn(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PaddedSlotPair slot;
    register void (*tickFn)() asm("$3");
    register void (*eventFn)() asm("$9");
    register u32 spriteId asm("$5");
    void (*fn)();
    s16 m1;

    tickFn = InitEntityWithDeathSpawn;
    eventFn = EntityEventHandlerAnimationSwitch;
    spriteId = 0x400C9A1D;
    /* @hack: triple multi-output barrier pins tickFn/eventFn/spriteId into their declared `register asm("$3"/"$9"/"$5")` slots ahead of the SLOT_STORE/SetEntitySpriteId chain (Quirk 6e + register-asm sprite-id pattern). */
    __asm__ volatile("" : "=r"(tickFn), "=r"(eventFn), "=r"(spriteId) : "0"(tickFn), "1"(eventFn), "2"(spriteId));
    ((u8 *)e)[0x111] = ((u8 *)e)[0x110];
    m1 = -1;
    SLOT_STORE(slot.s[0], e->tickMarker,  m1, tickFn);
    SLOT_STORE(slot.s[0], e->eventMarker, m1, eventFn);
    SetEntitySpriteId(e, spriteId, 1);
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationSpriteCallback(e, 0x44D4C8D8);
    SetAnimationFrameIndex(e, 0);
    fn = InitEntityRandomIdleOrAnimated;
    do {} while (0);
    SLOT_STORE(slot.s[0], se->queuedStateMarker, m1, fn);
    EntitySetCallback(e, ENEMY_ANIMATED_CALLBACK_MARKER, ENEMY_ANIMATED_CALLBACK_FN);
}

void SetEntityFacingDirection(Entity *e, s32 dir);

void EntitySetFacingRight(Entity *e) {
    SetEntityFacingDirection(e, 2);
}

void InitEntityState_Animated(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    void (*nextFn)();

    ((u8 *)e)[0x111] = (rand() & 3) + 2;
    do {} while (0);
    fn = InitEntityWithDeathSpawn;
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = EntityEventHandlerTimerCountdown;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    SetEntitySpriteId(e, 0x611C5804, 1);
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationSpriteCallback(e, 0x2421405);
    SetAnimationFrameIndex(e, 0);
    if (((u8 *)e)[0x110]) {
        nextFn = InitEnemyAnimatedWithDeathSpawn;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    } else {
        nextFn = InitEntityRandomIdleOrAnimated;
        /* @hack: per-arm fn-barrier keeps `la nextFn` inside each branch (Quirk 5/6k rand-conditional). */
        __asm__ volatile("" : "=r"(nextFn) : "0"(nextFn));
    }
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = nextFn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s[0];
}

Entity *InitProjectilePathEntity(Entity *e, s16 x, s16 y) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    InitEntitySprite(e, 0xB8700CA1, 0x3DE, x, y, 1);
    e->collisionVtable = &PROJECTILE_PATH_ENTITY_VTABLE;
    do {} while (0);
    fn = ProjectilePathFollowerTick;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->renderMarker = slot.s;
    ((u8 *)e)[0x100] = 0;
    SetupEntityScaleCallbacks(e);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/enemies", ProjectilePathFollowerTick);

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnProjectileEntityDef);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithChildSprite);

void DestroyEntityWithSoundAndChild(Entity *e, u32 flags) {
    EntityWithSoundAndChild *child = (EntityWithSoundAndChild *)e;
    e->collisionVtable = &CHILD_SPRITE_PARENT_VTABLE;
    StopSPUVoice(child->voiceId);
    RemoveEntityFromAllLists(g_pGameState, child->childEntityId);
    child->childEntityId = 0;
    e->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

void CollectibleTickWithSoundPanning(SoundPanningEntity *e) {
    UpdateEntitySoundPanning((Entity *)e, e->soundId);
    CollectibleTickCallback((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnMultipleProjectiles);

void EntityDestroyCallback_Vt80010C64(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &PROJECTILE_PATH_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800410bc(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_80041120(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010E04(Entity *entity, u32 flag) {
    entity->collisionVtable = &SIMPLE_FREE_ONLY_ENTITY_VTABLE;
    if (flag & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", func_800411CC);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityFromHeapContext);

void EntityDestroyCallback_Vt80010DE4_80041230(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_80041294(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800412f8(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_8004135c(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800413c0(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80041424(void) {
}

void NopStub_8004142c(void) {
}

void EntityDestroyCallback_Vt80010E04_80041434(Entity *entity, u32 flag) {
    entity->collisionVtable = &SIMPLE_FREE_ONLY_ENTITY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80041468(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80041468(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitCheckpointEntity);

void DestroyEntityWithChildRemoval(Entity *e, u32 flags) {
    EntityWithChild104 *child = (EntityWithChild104 *)e;
    e->collisionVtable = &CHECKPOINT_ENTITY_VTABLE;
    RemoveEntityFromAllLists(g_pGameState, child->childEntityId);
    child->childEntityId = 0;
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFloatingWithCollisionTick);

Entity *InitCollectibleEntity_Alt(Entity *e, u8 *spawn) {
    CollectibleSpawnData *spawnData = (CollectibleSpawnData *)spawn;
    PadSlot slot;
    s16 m1;
    void (*fn)();
    u8 *sprite;

    InitEntitySprite(e, 0x88210498, 0x3CA, spawnData->x, spawnData->y - 1, 0);
    e->collisionVtable = &ALT_COLLECTIBLE_ENTITY_VTABLE;
    e->allocSize = 0x384;
    ((PlatformActivationRefEntity *)e)->spawn = (PlatformActivationSpawnRef *)spawn;
    do {} while (0);
    fn = EntityTimedStateSwitchTick;
    do {} while (0);
    m1 = -1;
    /* @hack: SLOT_STORE routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    SetEntitySpriteId(e, 0x88210498, 1);
    SetAnimationSpriteId(e, 0);
    EntitySetRenderFlags(e, 0);
    {
        register s32 abr asm("$5");
        SpriteRenderContext *sprite0;
        SpriteRenderContext *sprite;
        s32 maskX;
        s32 maskY;
        s32 x;
        s32 y;

        sprite0 = (SpriteRenderContext *)e->spriteContext;
        /* @hack: pointer barrier prevents cc1 from coalescing `sprite0` with the later `sprite` reload (Quirk 6i). */
        __asm__ volatile("" : "=r"(sprite0) : "0"(sprite0));
        abr = 1;
        /* @hack: pin abr into $a1 (register asm above) ahead of GetTPage call. */
    __asm__ volatile("" : "=r"(abr) : "0"(abr));
        sprite0->activeFlag = 0;
        sprite = (SpriteRenderContext *)e->spriteContext;
        maskX = -0x40;
        x = sprite->vramX;
        maskY = -0x100;
        maskX = x & maskX;
        y = sprite->vramY;
        sprite->tpageBits = GetTPage(sprite->colorMode, abr, maskX, y & maskY);
    }
    SetupEntityScaleCallbacks(e);
    e->collisionMask = 0;
    e->targetX = 0;
    return e;
}

void EntityTimedStateSwitchTick(Entity *e) {
    PlatformActivationRefEntity *platRef = (PlatformActivationRefEntity *)e;
    PadSlot slot;
    void (*fn)();
    s16 m1;

    EntityUpdateCallback(e);
    if (((g_pGameState->frame_counter + platRef->spawn->framePhaseOffset) % 80) < 2) {
        register Entity *callArg asm("$4");

        callArg = e;
        /* @hack: pin callArg into $a0 (register asm above) ahead of SetEntitySpriteId. */
        __asm__ volatile("" : "=r"(callArg) : "0"(callArg));
        fn = EntityUpdateWithCollisionOffscreen;
        /* @hack: fn-barrier holds `la fn` ahead of the SLOT_STORE so it colors to $v1 (Quirk 5/6e). */
        __asm__ volatile("" : "=r"(fn) : "0"(fn));
        ((u8 *)e->spriteContext)[0xA] = 1;
        m1 = -1;
        SLOT_STORE(slot.s, e->tickMarker, m1, fn);
        SetEntitySpriteId(callArg, 0x88210498, 1);
    }
    CheckAndDisableChildEntityOffscreen(e);
}

void EntityUpdateWithCollisionOffscreen(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
    CheckAndDisableChildEntityOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableChildEntityOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledPlatformEntity);

void EntityConditionalActivateTick(PlatformActivationEntity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == e->spawn->activateFrame) {
        EntitySetState((Entity *)e, PLATFORM_ACTIVATE_BY_FRAME_STATE_MARKER,
                       PLATFORM_ACTIVATE_BY_FRAME_STATE_CALLBACK);
    }
    EntityUpdateCallback((Entity *)e);
    CheckAndDisableSpawnDataOffscreen((Entity *)e);
}

void EntityUpdateWithSpawnDataCheck(Entity *entity) {
    EntityUpdateCallback(entity);
    CheckAndDisableSpawnDataOffscreen(entity);
}

void EntityUpdateWithCollisionSpawnCheck(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
    CheckAndDisableSpawnDataOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnDataOffscreen);

s32 EntitySimpleEventPassthrough_V2(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

s32 EntityEventTimerCountdownWithGameState(TimedByteWithTileEntity *e, u32 event) {
    u8 timer;
    u16 tileId;
    if ((event & 0xFFFF) == EVT_TICK) {
        timer = e->timer - 1;
        e->timer = timer;
        if (timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        } else {
            tileId = e->tileRecord->tileId;
            if ((u16)(tileId - 0x22) < 2 || tileId == 0x24) {
                ((u8 *)g_pGameState)[0x11A] = 0x14;
            }
        }
    }
    return 0;
}

void EntityHideAndDisable(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e->spriteContext)[0xA] = 0;
    EntitySetRenderFlags(e, 0);
    fn = EntityConditionalActivateTick;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->eventMarker);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithTypeBasedTimer);

void InitPlatformEntityState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PlatformActivationRefEntity *platRef = (PlatformActivationRefEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = EntityUpdateWithCollisionSpawnCheck;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = EntitySimpleEventPassthrough_V2;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, platRef->spawn->spriteId, 1);
    SetAnimationLoopFrame(e, 0x20140828);
    fn = EntityHideAndDisable;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitDirectionalScaledEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformTimerTickCallback);

void PlatformCollisionTickCallback(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
    CheckAndDisableSpawnRefOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnRefOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformEventHandlerSpawnEffect);

void PlatformHideAndDisable(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e->spriteContext)[0xA] = 0;
    EntitySetRenderFlags(e, 0);
    fn = PlatformTimerTickCallback;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->eventMarker);
}

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitBouncableClayEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityOffScreenChildCleanup);

void EntityTick_CollisionWithCleanup(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 2);
    EntityOffScreenChildCleanup(e);
}

void HazardTimerTickCallback(HazardTimerEntity *e) {
    EntityUpdateCallback((Entity *)e);
    EntityOffScreenChildCleanup((Entity *)e);
    if (e->timer != 0) {
        e->timer -= 1;
        if (e->timer == 0) {
            EntitySetState((Entity *)e, HAZARD_TIMER_EXPIRED_STATE_MARKER,
                           HAZARD_TIMER_EXPIRED_STATE_CALLBACK);
        }
    }
}

s32 HazardEventHandler_0x1001(Entity *e, u32 ev, u32 a2, u32 a3) {
    if ((ev & 0xFFFF) == EVT_SET_READY) {
        EntitySetState(e, HAZARD_READY_STATE_MARKER,
                       HAZARD_READY_STATE_CALLBACK);
    }
    return 0;
}

s32 HazardEventHandler_0x1001_V2(Entity *e, u32 ev, u32 a2, u32 a3) {
    u32 maskedEv = ev & 0xFFFF;
    if (maskedEv == EVT_SET_READY) {
        EntitySetState(e, HAZARD_READY_STATE_MARKER,
                       HAZARD_READY_STATE_CALLBACK);
    }
    if (maskedEv == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return 0;
}

s32 EntityEventPassthrough_Event2(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

extern void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData, s16 duration, s32 unused);

void EntityPathMovementUpdate(TimedPathEntity *e) {
    s16 out[2];
    e->pathTime += 1;
    InterpolateTimedPathPosition(
        &e->pathTime,
        out,
        e->pathData,
        e->pathDuration,
        8
    );
    e->sprite.base.worldX = e->pathOriginX + (u16)out[0];
    e->sprite.base.worldY = e->pathOriginY + (u16)out[1];
}

/* Returns the BounceClay enemy to idle: re-installs
 * EntityTick_CollisionWithCleanup on the +0x00 tick slot and
 * HazardEventHandler_0x1001 on the +0x08 event slot, then switches the
 * sprite to 0xF175320E (idle animation). Called after a bounce settles. */
void BounceClay_IdleState(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = EntityTick_CollisionWithCleanup;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = (void (*)())HazardEventHandler_0x1001;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0xF175320E, 1);
}

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_DestroyingState);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardActiveState);

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_HiddenState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledMovingEntity);

void TimedEntityTickCallback(TimedByteEntity *e) {
    if (e->timer != 0) {
        e->timer -= 1;
        if (e->timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    EntityUpdateCallback((Entity *)e);
}

void TimedEntityTickCallbackWithCollision(TimedByteEntity *e) {
    if (e->timer != 0) {
        e->timer -= 1;
        if (e->timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    EntityUpdateWithCollisionWrapper((Entity *)e);
}

void EntityUpdateWithCollisionWrapper(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
}

s32 SwitchEventHandler_SetGameFlag(SwitchFlagEntity *e, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        u16 val = e->spawn->tileId;
        if ((u16)(val - 0x22) < 2 || val == 0x24) {
            *(u8 *)&g_pGameState->screen_shake_index = 0x14;
        }
    }
    return 0;
}

void EntityIncrementWorldX(Entity *e) {
    e->worldX += 1;
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityHideWithTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSwitchActivatedState);

void InitSwitchMovingState(Entity *e) {
    PadSlot slot;
    s16 m1 = -1;
    void (*fn)();

    fn = EntityUpdateWithCollisionWrapper;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker   = slot.s;
    fn = EntityIncrementWorldX;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->renderMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitSwitchBlockEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TrackerEntityTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandler_SetFlag0x110);

s32 EntityNullEventHandler(void) {
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitEnemySpawnerEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnerEntityTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTriggerZoneEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterActivateTickCallback);

s32 TeleporterEventPassthrough_Event2(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTeleporterActivatingState);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCameraEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterTransitionTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterPortalEventHandler);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterExitState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitIndexedSpriteEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityOffscreenParentCleanupTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCameraTrackingEntity);

void SoundEmitterDestroyCallback(Entity *e, u32 flags) {
    e->collisionVtable = &CAMERA_TRACKING_ENTITY_VTABLE;
    StopSPUVoice((s32)e->renderCallback);
    e->collisionVtable = &ENTITY_FREE_ONLY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterWithPanningTick);

void EntityDestroyCallback_Vt80011068(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &INDEXED_SPRITE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011088(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &CAMERA_HELPER_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800110A8(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &TRIGGER_ZONE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800111C8(Entity *e, u32 flags) {
    e->collisionVtable = &ENTITY_FREE_ONLY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

void EntityDestroyCallback_Vt800110E8(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &SWITCH_BLOCK_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011108(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &SCALED_MOVING_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011128(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &BOUNCABLE_CLAY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011148(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &DIRECTIONAL_SCALED_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011168(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &SCALED_PLATFORM_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011188(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &ALT_COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80045e70(void) {
}

void NopStub_80045e78(void) {
}

void EntityDestroyCallback_Vt800111C8_80045e80(Entity *entity, u32 flag) {
    entity->collisionVtable = &ENTITY_FREE_ONLY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80045eb4(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80045eb4(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitFloatingPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleScaledTickCallback);

s32 EntityEventHandlerWalkWithTimer(EnemyTimerStateEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = EntityEventHandlerWalk((Entity *)e, maskedEvent, arg2, arg3);
    if (maskedEvent == EVT_TICK) {
        if (e->stateDelay != 0) {
            e->stateDelay -= 1;
            if (e->stateDelay == 0) {
                EntityProcessCallbackQueue((Entity *)e);
            }
        }
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityGroundSnapMovementCallback);

extern void CollectibleScaledTickCallback(Entity *e);

void InitItemSparkleIdleState(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    SLOT_CLEAR(slot.s[0], e->renderMarker);
    fn = (void (*)())CollectibleScaledTickCallback;
    m1 = -1;
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerIdle;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0xB4011101, 1);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemRevealState);

extern u8 ITEM_REVEAL_ANIM_SEQUENCE_7_FRAME_DATA[] asm("D_8009B7A4");
extern u8 ITEM_REVEAL_ANIM_SEQUENCE_13_FRAME_DATA[] asm("D_8009B7DC");

void SetEntitySpecialState_1(EnemyTimerStateEntity *e) {
    e->stateDelay = 1;
    EntityStateSetSpecial((Entity *)e);
}

void SetEntitySpecialState_2(EnemyTimerStateEntity *e) {
    e->stateDelay = 2;
    EntityStateSetSpecial((Entity *)e);
}

void SetEntitySpecialState_3(EnemyTimerStateEntity *e) {
    e->stateDelay = 3;
    EntityStateSetSpecial((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetSpecial);

void StartAnimSequence7Frames(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ITEM_REVEAL_ANIM_SEQUENCE_7_FRAME_DATA, 7);
}

void StartAnimSequence13Frames(SpriteEntity *e) {
    StartAnimationSequence(e, (s32)ITEM_REVEAL_ANIM_SEQUENCE_13_FRAME_DATA, 13);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEntity_Alt);

void SoundEntityDestroyCallback(SoundPanningEntity *e, u32 flags) {
    e->sprite.base.collisionVtable = &ALT_PATH_FOLLOWING_ENTITY_VTABLE;
    StopSPUVoice(e->soundId);
    e->sprite.base.collisionVtable = &ENEMY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

void FinnModeCollectibleTickCallback(SoundPanningEntity *e) {
    UpdateEntitySoundPanning((Entity *)e, e->soundId);
    CollectibleTickFinnMode((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyPathFollowWithFacingFlip);

extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);

void EnemySetWalkSprite(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    fn = EntityEventHandlerWalk;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0xE4CB8330, 1);
}

void EnemySetIdleSprite(Entity *e) {
    TripadSlot slot;
    void (*fn)();
    s16 m1;
    Entity *entity = e;
    SpriteEntity *spriteEntity = (SpriteEntity *)entity;
    SetEntityFacingDirection(entity, 2);
    fn = EntityEventHandlerIdle;
    m1 = -1;
    /* @hack: SLOT_STORE routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_STORE(slot.s, entity->eventMarker, m1, fn);
    SetEntitySpriteId(entity, 0x458B0320, 1);
    fn = EnemySetWalkSprite;
    SLOT_STORE(slot.s, spriteEntity->queuedStateMarker, m1, fn);
}

void EnemyDestroyCallback_0x80011228(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &ENEMY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011228(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &ENEMY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80046ce4(void) {
}

void NopStub_80046cec(void) {
}

void EntityDestroyCallback_Vt80011248(Entity *entity, u32 flag) {
    entity->collisionVtable = &ENEMY_FREE_ONLY_ENTITY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80046d28(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80046d28(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundParticleEmitter);

INCLUDE_ASM("asm/nonmatchings/enemies", ParticleEmitterTickCallback);

s32 EntitySetReadyFlag(BackgroundSparkleEntity *e, u16 mode) {
    if (mode == EVT_TICK) {
        e->contactFlag = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundSparkleEntity);

/* Per-tick fade-in/fade-out animator for background sparkle effects.
 * Counts down revealTimer from its initial value, and uses it to drive a
 * triangle-wave R/G tint on the sprite context: brightness ramps up from
 * 0x40 to 0x150 over the first 11 ticks, then back down. Blue stays
 * pegged at 0x40 to give the sparkle a warm white-yellow tone. When
 * revealTimer hits 0 the fade stops being touched and EntityUpdateCallback
 * is called bare so the sparkle is just rendered statically.
 *
 * SHELVED (1030-byte diff, structurally complete): body matches with
 *     u8 t;
 *     if (e->revealTimer != 0) {
 *         s32 fade = (s32)e->revealTimer - 1;
 *         t = fade;                                      // forces move v0,v1
 *         e->revealTimer = t;
 *         if (t < 0xB) {
 *             brightness = fade << 4;
 *         } else {
 *             brightness = (0x14 - fade) << 4;
 *         }
 *         brightness += 0x40;
 *         u8 *color = e->sprite.base.spriteContext;
 *         color[0x34] = brightness;
 *         color[0x35] = brightness;
 *         color[0x36] = 0x40;
 *     }
 *     EntityUpdateCallback((Entity *)e);
 * but cc1 cross-jumps the if/else arms: it places the simple then-arm's
 * `sll fade<<4` in the bnez delay slot and merges both arms into the
 * fall-through common-code path, eliminating the `j L134` that TARGET
 * keeps. Inline-asm barriers in either arm only partially help (swaps
 * arm order, still removes the j). No clean C-level idiom defeats cc1's
 * tail-merge for this specific shape. */
INCLUDE_ASM("asm/nonmatchings/enemies", BackgroundSparkleFadeTickCallback);

s32 BackgroundSparkleContactEventHandler(BackgroundSparkleEntity *sparkle, u32 ev) {
    if ((ev & 0xFFFF) == EVT_TICK) {
        if (sparkle->contactFlag != 0) {
            BackgroundSparkleChildContext *child = sparkle->sprite.base.spriteContext;
            child->activeFlag = 0;
            SetAnimationSpriteId((Entity *)sparkle, sparkle->sprite.currentFrame);
            EntitySetRenderFlags((Entity *)sparkle, 0);
        }
    }
    return 0;
}

void InitBackgroundSparkleRevealState(BackgroundSparkleEntity *e) {
    e->contactFlag = 1;
    SetAnimationSpriteId((Entity *)e, -1);
    SetAnimationFrameCallback((Entity *)e, 0x2421405);
}

void SetEntityAnimationState(BackgroundSparkleEntity *sparkle) {
    BackgroundSparkleChildContext *child = sparkle->sprite.base.spriteContext;
    child->activeFlag = 1;
    sparkle->contactFlag = 0;
    EntitySetRenderFlags((Entity *)sparkle, 1);
    SetAnimationLoopFrame((Entity *)sparkle, 0x1084280);
    SetAnimationSpriteCallback((Entity *)sparkle, 0x2421405);
    SetAnimationFrameIndex((Entity *)sparkle, 0);
}

void func_8004727C(BackgroundSparkleEntity *e) {
    e->revealTimer = 0x14;
}

