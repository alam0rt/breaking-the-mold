#include "common.h"
#include "Game/asset_ids.h"
#include "functions.h"
#include "Game/entity_events.h"
#include "Game/callback_slot.h"
#include "Game/fsm_dispatch.h"
#include "Game/enemy_entities.h"
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

extern u8 IsEntityOffScreen(Entity *e);
extern u8 IsEntityOffScreen_EntityLoop(GameState *gs, Entity *e);

typedef void (*GsCullNotifyCB)(void *dst, s16 eventId, s32 arg, void *src);
typedef struct { s32 arg; GsCullNotifyCB fn; } GsCullNotifySlot;

extern void *g_pBlbHeapBase;
extern void CheckAndDisableSpawnDataOffscreen(OffscreenCullEntity *entity);
extern void CollisionCheckWrapper(Entity *e, s32 a, s32 b, s32 c);
extern void CheckAndDisableChildEntityOffscreen(OffscreenCullEntity *e);
extern void CheckAndDisableSpawnRefOffscreen(OffscreenCullEntity *e);
extern void EntityOffScreenChildCleanup(OffscreenCullEntity *e);
extern void UpdateEntitySoundPanning(Entity *e, u32 sound);
extern void CollectibleTickCallback(Entity *e);
extern void CollectibleTickFinnMode(Entity *e);
extern s32 EntityEventHandlerIdle(Entity *e, u16 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerWalk(Entity *e, u16 event, u32 arg2, u32 arg3);
extern void EntityStateSetWalk(Entity *e);
extern void SoundEmitterTickCallback(Entity *e);
extern void AnimatedEntityToggleSpriteA(Entity *e);
extern void AnimatedEntityToggleSpriteB(Entity *e);
extern void SetAnimationTargetFrameIndex(Entity *e, s32 id);
extern void SetAnimationLoopFrameIndex(Entity *e, s32 frame);
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
/* gp_rel sdata: strong initialized defs live in the end-of-file block (Phase 4). */
extern u32 ENEMY_ANIMATED_CALLBACK_MARKER asm("D_800A5AD0");
extern EntityCallback ENEMY_ANIMATED_CALLBACK_FN asm("D_800A5AD4");
extern void RemoveEntityFromAllLists(GameState *gs, s32 idx);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void EntitySetCallback(Entity *e, u32 marker, EntityCallback fn);
extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);
extern void AIEntityRandomBehaviorTick(Entity *e);
extern s32 EntityEventHandlerWithRandomWalk(Entity *e, u16 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerWithDelayedWalk(Entity *e, u16 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerSpawnProjectile(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerWithCountdownToWalk(Entity *e, u16 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerSpawnParticle(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerCountdownToWalkWithSprite(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerTimerCountdown(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandlerAnimationSwitch(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntityFallingGravityWithCollision(Entity *e);
extern void ApplyAnimationPositionOffsets(Entity *e);
extern void CollectibleSparkleTickCallback(Entity *e);
extern Entity *InitParticleEntity(u8 *mem, u32 spriteId, u32 packedXY, u8 facing, s32 scaleRender, s32 arg5, s32 arg6);
extern void TripleLaserMonkeyDeathTick(Entity *e);
extern s32 EntityEventHandler0x1001_1002_1008(Entity *e, u16 event, u32 arg2, u32 arg3);
extern s32 EntityEventHandler0x1001_1002_1008_V2(Entity *e, u16 event, u32 arg2, u32 arg3);
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

/* gp_rel sdata tentative defs -> now forward declarations; strong initialized
 * defs live in the address-ordered block at the end of this file (Phase 4). */
extern u32 SOUND_EMITTER_STUN_EXPIRED_STATE_MARKER asm("D_800A5A68");
extern EntityCallback SOUND_EMITTER_STUN_EXPIRED_STATE_CALLBACK asm("D_800A5A6C");
extern u32 PLATFORM_ACTIVATE_BY_FRAME_STATE_MARKER asm("D_800A5AE8");
extern EntityCallback PLATFORM_ACTIVATE_BY_FRAME_STATE_CALLBACK asm("D_800A5AEC");
extern u32 HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B08");
extern EntityCallback HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B0C");
extern u32 HAZARD_READY_STATE_MARKER asm("D_800A5B10");
extern EntityCallback HAZARD_READY_STATE_CALLBACK asm("D_800A5B14");

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
extern u16 CalcEntityYFromTileHeight(Entity *e, u32 tile, s16 x, s16 y);

INCLUDE_ASM("asm/nonmatchings/enemies", UpdateCollectibleTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckCollectibleOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickCallback);

/* Per-frame tick for a collectible with a countdown: decrements the +0x100
 * timer and, when it hits 0, fires the queued state-transition callback, then
 * runs the standard collectible tick (offscreen cull + collision pickup). */
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

/* Attack-token arbitration handler installed while the enemy is in its WALK
 * state. Despite "Walk", the body does no movement — it manages the
 * single-attacker token at +0x108 (holder id) and the +0x106 "ready" flag:
 *   EVT_TOKEN_QUERY  - mark ready; if `attacker` is the current holder, release
 *                      the token; return self (the candidate).
 *   EVT_SET_READY    - mark ready.
 *   EVT_TOKEN_CLAIM  - claim the token if free, returning 1 on success.
 * (NAME reflects the installing state, not behavior — see EntityEventHandlerIdle
 * for the identical logic + an EVT_TICK callback-queue pump.) */
s32 EntityEventHandlerWalk(Entity *e, u16 event, u32 unused, u32 attacker) {
    u8 *e8 = (u8 *)e;
    s32 result = 0;
    switch (event) {
    case EVT_TOKEN_QUERY:
        e8[0x106] = 1;
        if (attacker == *(s32 *)(e8 + 0x108)) {
            *(s32 *)(e8 + 0x108) = 0;
        }
        result = (s32)e;
        break;
    case EVT_SET_READY:
        e8[0x106] = 1;
        break;
    case EVT_TOKEN_CLAIM:
        if (*(s32 *)(e8 + 0x108) == 0) {
            *(s32 *)(e8 + 0x108) = attacker;
            result = 1;
        }
        break;
    }
    return result;
}

/* Attack-token arbitration installed while IDLE — identical token/ready logic
 * to EntityEventHandlerWalk, plus it pumps the callback queue on EVT_TICK (so
 * an idle enemy still advances its queued state transitions). */
s32 EntityEventHandlerIdle(Entity *e, u16 event, u32 unused, u32 attacker) {
    u8 *e8 = (u8 *)e;
    s32 result = 0;
    switch (event) {
    case EVT_TOKEN_QUERY:
        e8[0x106] = 1;
        if (attacker == *(s32 *)(e8 + 0x108)) {
            *(s32 *)(e8 + 0x108) = 0;
        }
        result = (s32)e;
        break;
    case EVT_SET_READY:
        e8[0x106] = 1;
        break;
    case EVT_TOKEN_CLAIM:
        if (*(s32 *)(e8 + 0x108) == 0) {
            *(s32 *)(e8 + 0x108) = attacker;
            result = 1;
        }
        break;
    }
    if (event == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnCollectibleParticles);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleWithFlags);

/* Per-tick "twinkle" driver for sparkle collectibles: runs the base collectible
 * tick, then (while enabled) pulses the sprite brightness through a 4-phase
 * cycle (phase 0 brighten +0xC, 1 hold 0xBF + spawn particle, 2 dim -0xC,
 * 3 rest) and, during the bright hold-phase, periodically spawns a short-lived
 * sparkle particle jittered by rand()&0x1F around the entity's position. The
 * two rand() results are packed as (X | Y<<16) into one InitParticleEntity arg;
 * the spawned particle's TPage is computed via GetTPage(colorMode, 1,
 * vramX & ~0x3F, vramY & ~0xFF) using the same s32-locals idiom as
 * InitCollectibleEntity_Alt.
 *
 * SHELVED — instruction-exact (141==141) and behaviourally complete, but three
 * cc1 scheduling/coloring residuals remain that neither do-while fences, the
 * abr $a1 register-pin, nor the permuter (-j6, best score ~660) have cracked:
 *   1. the coords reload+pack (coords[0] | coords[1]<<16) is scheduled early
 *      into $t0 rather than late in the InitParticleEntity delay slot;
 *   2. the `li a1,1` (GetTPage abr arg) lands at a different point;
 *   3. a phaseTimer (+0x11C) store ordering in the transition chain,
 * plus the entity pointer colors to a different saved reg (frame 0x38 vs 0x40).
 * Structurally-correct draft parked in the (gitignored) permuter dir
 * nonmatchings/CollectibleSparkleTickCallback — a strong base for a longer /
 * multi-core permuter run. The SparkleCollectibleEntity + SpriteRenderContext
 * color-field views above are verified against the disassembly. */
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

void EntityStateSetIdle(Entity *e);
void EntityStateSetRandomBehavior(Entity *e);
void EntityStateSetAttack(EnemyTimerStateEntity *e);

/* Per-tick AI driver for the "random behavior" ground enemy (the standard
 * Skullmonkey). Counts down walkDelay (+0x112); when it expires, re-rolls the
 * next state: if the attack-enabled flag (+0x118) is set, 50% chance to attack;
 * otherwise a coin flip between idle and a fresh random behavior. Independently
 * counts down stateTimer (+0x104) to flush the queued-state slot, then runs the
 * shared sparkle/render tick. */
void AIEntityRandomBehaviorTick(Entity *arg) {
    EnemyTimerStateEntity *e = (EnemyTimerStateEntity *)arg;
    if (e->walkDelay != 0) {
        e->walkDelay -= 1;
        if (e->walkDelay == 0) {
            if (((u8 *)e)[0x118] != 0 && (rand() & 1)) {
                EntityStateSetAttack(e);
            } else if (rand() & 1) {
                EntityStateSetIdle((Entity *)e);
            } else {
                EntityStateSetRandomBehavior((Entity *)e);
            }
        }
    }
    if (e->stateTimer != 0) {
        e->stateTimer -= 1;
        if (e->stateTimer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    CollectibleSparkleTickCallback((Entity *)e);
}

/* Same 0x1001/0x1002/0x1008 switch skeleton as EntityEventHandlerIdle, plus an
 * EVT_TICK countdown on +0x110 (queue at 0, SetAnimationTargetFrameIndex(-1) at 1).
 * SHELVED: register-coloring split only — TARGET births `result` in the dead
 * $a2 arg reg during the switch and copies to $s0 only across the post-tick
 * calls; cc1 here keeps `result` in $s0 throughout. Same instruction count,
 * permuter territory. */
INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandler0x1001_1002_1008);

/* Behaviorally identical to EntityEventHandlerIdle (attack-token arbitration +
 * EVT_TICK callback-queue pump) — a second copy at its own address. NAME is a
 * placeholder (the hash is just the three handled event ids 0x1001/2/8); it's
 * really another idle-state token handler. */
s32 EntityEventHandler0x1001_1002_1008_V2(Entity *e, u16 event, u32 unused, u32 attacker) {
    u8 *e8 = (u8 *)e;
    s32 result = 0;
    switch (event) {
    case EVT_TOKEN_QUERY:
        e8[0x106] = 1;
        if (attacker == *(s32 *)(e8 + 0x108)) {
            *(s32 *)(e8 + 0x108) = 0;
        }
        result = (s32)e;
        break;
    case EVT_SET_READY:
        e8[0x106] = 1;
        break;
    case EVT_TOKEN_CLAIM:
        if (*(s32 *)(e8 + 0x108) == 0) {
            *(s32 *)(e8 + 0x108) = attacker;
            result = 1;
        }
        break;
    }
    if (event == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return result;
}

/* Attack-token arbitration (same as Idle) but on EVT_TICK it arms a random
 * walk-hold timer at +0x112 (rand()&0xF + 8 frames) and transitions the enemy
 * into its walk state — i.e. idle enemies that periodically wander. */
s32 EntityEventHandlerWithRandomWalk(Entity *e, u16 event, u32 unused, u32 attacker) {
    s32 result = 0;
    switch (event) {
    case EVT_TOKEN_QUERY:
        ((u8 *)e)[0x106] = 1;
        if (attacker == *(s32 *)((u8 *)e + 0x108)) {
            *(s32 *)((u8 *)e + 0x108) = 0;
        }
        result = (s32)e;
        break;
    case EVT_SET_READY:
        ((u8 *)e)[0x106] = 1;
        break;
    case EVT_TOKEN_CLAIM:
        if (*(s32 *)((u8 *)e + 0x108) == 0) {
            *(s32 *)((u8 *)e + 0x108) = attacker;
            result = 1;
        }
        break;
    }
    if (event == EVT_TICK) {
        ((u8 *)e)[0x112] = (rand() & 0xF) + 8;
        EntityStateSetWalk(e);
    }
    return result;
}

/* Idle-skeleton switch + EVT_TICK delay countdown (rand/SetWalk at 0,
 * SetAnimationTargetFrameIndex(-1) at 1). SHELVED: same result register-coloring split
 * as EntityEventHandler0x1001_1002_1008 — TARGET births `result` in the dead
 * $a0 arg reg during the switch and copies to $s1 across the post-tick calls;
 * cc1 keeps it in $s1 throughout. Permuter territory. */
INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithDelayedWalk);

/* As EntityGroundSnapMovementCallback, but gated by the animation state: only
 * (re)applies the animation's position offsets when the current frame is 0 and
 * the one-shot +0x111 flag is set (clearing it), or unconditionally while the
 * frame is nonzero. Then runs the same two-probe (worldY-7 / worldY+2) ground
 * snap onto the first solid tile found. */
void EntityGroundSnapWithAnimation(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    u8 tile;

    if (se->currentFrame == 0) {
        if (((u8 *)e)[0x111] != 0) {
            ApplyAnimationPositionOffsets(e);
            ((u8 *)e)[0x111] = 0;
        }
    } else {
        ApplyAnimationPositionOffsets(e);
    }
    tile = EntityApplyMovementCallbacks(e, e->worldX, (s16)(e->worldY - 7));
    if (tile != 0 && tile < 0x3C) {
        e->worldY = CalcEntityYFromTileHeight(e, tile, e->worldX,
                                              (s16)(e->worldY - 7));
        return;
    }
    tile = EntityApplyMovementCallbacks(e, e->worldX, (s16)(e->worldY + 2));
    if (tile != 0 && tile < 0x3C) {
        e->worldY = CalcEntityYFromTileHeight(e, tile, e->worldX,
                                              (s16)(e->worldY + 2));
    }
}

/* Enter walk state with a random walk-hold (rand()&0xF + 4 frames) and a
 * 0x2D-frame state timer before the next transition. */
void EntityStartWalkWithTimer0x2d(EnemyTimerStateEntity *e) {
    e->walkDelay = (rand() & 0xF) + 4;
    e->stateTimer = 0x2D;
    EntityStateSetWalk((Entity *)e);
}

/* As EntityStartWalkWithTimer0x2d but with a shorter 0xA-frame state timer. */
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
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    fn = AIEntityRandomBehaviorTick;
    m1 = -1;
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[1], 1);
}

void EntityStateSetIdle(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;
    do {} while (0);
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    fn = TimedSparkleCollectibleTick;
    m1 = -1;
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerWithRandomWalk;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[3], 1);
}

/* Same EntityStateSetWalk/Idle 3-slot install idiom (event/render-clear/tick),
 * but the event handler is chosen by rand()&1 (EntityEventHandlerWithRandomWalk
 * vs EntityEventHandlerWalk); tick is TimedSparkleCollectibleTick; sprite from
 * spriteIds[4].
 *
 * SHELVED — cc1 hoist divergence (2-instr shortfall). Target emits the naive
 * un-hoisted diamond: beqz .else (li v0,-1 = m1 in the delay slot), if-branch
 * loads RandomWalk then `j` over the else, else-branch loads Walk. With plain
 * if/else, ternary OR goto, this cc1 instead hoists the else-value (fn=Walk)
 * into the scheduling gap right after `jal rand`, then conditionally overwrites
 * with RandomWalk — no `j`, 2 instrs shorter, and the freed register cascades
 * v0/v1 -> v1/a0 coloring across every following struct-copy. The matched
 * siblings (EntityStateSetWalk/Idle) only avoid this because their fn is
 * unconditional. Permuter (-j4, 240s) plateaus at score 140: its best move is
 * shoving a spurious markerLo store into the else arm to force the diamond, but
 * that is neither correct source nor a byte-match, and the permuter cannot add
 * the missing `j`+`nop`. Draft + permuter outputs kept in
 * nonmatchings/EntityStateSetRandomBehavior. */
INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetRandomBehavior);

/* Enter the ATTACK state: arms the 3-frame state delay, installs the
 * sparkle tick + delayed-walk event handler, swaps to attack sprite
 * (spriteIds[5]), and sets the attack animation (loop/callback frames). */
void EntityStateSetAttack(EnemyTimerStateEntity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;
    e->stateDelay = 3;
    do {} while (0);
    SLOT_CLEAR(slot.s[0], e->sprite.base.renderMarker);
    fn = TimedSparkleCollectibleTick;
    m1 = -1;
    SLOT_STORE(slot.s[0], e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerWithDelayedWalk;
    SLOT_STORE(slot.s[0], e->sprite.base.eventMarker, m1, fn);
    spriteIds = e->spriteIds;
    SetEntitySpriteId((Entity *)e, spriteIds[5], 1);
    SetAnimationLoopFrame((Entity *)e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback((Entity *)e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex((Entity *)e, 0);
}

/* Sparkle-collectible "revealed" state: faces dir 2, installs position-offset
 * render + sparkle tick + token-arbitration event handler, swaps to the
 * sparkle sprite (spriteIds[2]) at anim frame 2. */
void EntityStateSetSparkleCollectible(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;

    SetEntityFacingDirection(e, 2);
    fn = ApplyAnimationPositionOffsets;
    do {} while (0);
    m1 = -1;
    SLOT_STORE(slot.s, e->renderMarker, m1, fn);
    fn = CollectibleSparkleTickCallback;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandler0x1001_1002_1008_V2;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[2], 1);
    SetAnimationFrameIndex(e, 2);
}

void EntityStateSetSparkle(Entity *e);

/* EntitySetSparkleDelay{3,2,1}: enter the sparkle state with the state-delay
 * counter preset to 3/2/1 frames respectively (staggers multi-sparkle reveals). */
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

/* Sparkle "spawning" state: sets the +0x111 active flag, installs ground-snap
 * render + sparkle tick + token-arbitration event handler, swaps to the first
 * sprite (spriteIds[0]) and starts its sparkle animation. */
void EntityStateSetSparkle(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    u32 *spriteIds;

    ((u8 *)e)[0x111] = 1;
    do {} while (0);
    fn = EntityGroundSnapWithAnimation;
    m1 = -1;
    SLOT_STORE(slot.s, e->renderMarker, m1, fn);
    fn = CollectibleSparkleTickCallback;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandler0x1001_1002_1008;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    spriteIds = ((EnemyTimerStateEntity *)e)->spriteIds;
    SetEntitySpriteId(e, spriteIds[0], 1);
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
}

extern u8 ENEMY_ANIM_SEQUENCE_4A_DATA[] asm("D_8009B55C");
extern u8 ENEMY_ANIM_SEQUENCE_4B_DATA[] asm("D_8009B57C");
extern u8 ENEMY_ANIM_SEQUENCE_4C_DATA[] asm("D_8009B59C");

/* StartAnimSequence4{A,B,C}: kick off a 4-frame canned animation sequence from
 * the corresponding ENEMY_ANIM_SEQUENCE_4{A,B,C}_DATA table. */
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

/* Tick while a sound-emitter enemy is stunned: counts the stun timer down and,
 * at 0, transitions to the stun-expired state; then runs the normal update +
 * offscreen cull each frame. */
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

/* Minimal event handler: pumps the callback queue on EVT_TICK, ignores
 * everything else, always returns 0. Installed by entities whose only
 * per-frame need is advancing queued state transitions. */
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

/* Triple-laser-monkey tick: fires its queued transition once every 0x80 frames
 * (when the low 7 bits of the global frame counter match this entity's phase
 * offset), then runs the per-frame death/laser tick. */
void TripleLaserMonkeyConditionalTick(ConditionalPhaseEntity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == e->framePhase) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    TripleLaserMonkeyDeathTick((Entity *)e);
}

/* Idle-skeleton switch + EVT_TICK predecrement of +0x111; at 0 runs
 * SetAnimationFrameCallback and installs EntityEventHandlerWalk into the
 * eventMarker slot. SHELVED: same result register-coloring split as the other
 * countdown handlers (TARGET births `result` in dead $a0, copies to $s1 across
 * the EVT_TICK call; cc1 keeps it in $s1 throughout). Permuter territory. */
INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithCountdownToWalk);

void LaserMonkeyWalkState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = TripleLaserMonkeyConditionalTick;
    m1 = -1;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0x60B98CBD, 1);
    fn = LaserMonkeyIdleState;
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, fn);
}

void InitTripleLaserMonkeyAttackState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = TripleLaserMonkeyConditionalTick;
    m1 = -1;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    ((u8 *)e)[0x111] = 3;
    do {} while (0);
    fn = EntityEventHandlerWithCountdownToWalk;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0x64BB1CBE, 1);
    fn = LaserMonkeyIdleState;
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, fn);
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
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerIdle;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
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
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, nextFn);
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
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
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
    SLOT_STORE(slot.s[0], se->queuedStateMarker, m2, fn);
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
    SetAnimationLoopFrameIndex(e, 0x17);
    SetAnimationTargetFrameIndex(e, 0x18);
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
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerIdle;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
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
    SLOT_STORE(slot.s[0], se->queuedStateMarker, m2, fn);
}

typedef s16 (*XformCB)();
typedef struct { s32 arg; XformCB fn; } XformSlot;
extern void CalculateEntityRenderBounds(Entity *e, s16 *bounds);

/* True if the entity (after parallax) sits more than 0x140 px left of the
 * camera. Resolves its X via the moveCallbackX FSM slot (GetEntityXPosition
 * dispatch shape, fn $a2 / then-fn $s3), passing the render-bounds value from
 * CalculateEntityRenderBounds. Instruction-perfect; residual is a 2-instr
 * coloring (final (s16)val sra lands $a1 vs target $v0). Permuter. */
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
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
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
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
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
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
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
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
    SetEntitySpriteId((Entity *)e, 0x21A81D54, 1);
}

void InitShooterEnemyStateA(EnemyTimerStateEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->stateInitDelay = 0x2F;
    do {} while (0);
    fn = CollectibleTickCallback;
    m1 = -1;
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerSpawnProjectile;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
    SetEntitySpriteId((Entity *)e, 0xA0299A4D, 1);
}

void InitShooterEnemyStateB(EnemyTimerStateEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->stateInitDelay = 0x14;
    do {} while (0);
    fn = CollectibleTickCallback;
    m1 = -1;
    SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    fn = EntityEventHandlerSpawnProjectile;
    SLOT_STORE(slot.s, e->sprite.base.eventMarker, m1, fn);
    SetEntitySpriteId((Entity *)e, 0x21229C5C, 1);
}

void InitCollectibleIdleState(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    m1 = -1;
    fn = CollectibleTickCallback;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerIdle;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0x53209C1C, 1);
}

void InitCollectibleIdleStateB(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    m1 = -1;
    fn = CollectibleTickCallback;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerIdle;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
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
 * Pulling the entityType field into a u16 temp before the compare colors the
 * (8|3) result into $v0 to match TARGET (cc1 picks $a0 with an inline field
 * read); the do{}while(0) after the store keeps it ahead of the call setup.
 * (Same recipe cracked the sister AnimatedEntityToggleSpriteB via permuter.) */
void AnimatedEntityToggleSpriteA(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    void *def;
    u16 entityType;
    def = *(void **)((u8 *)e + 0x100);
    entityType = *(u16 *)((u8 *)def + 0x12);
    ((u8 *)e)[0x110] = (entityType == 0x2F) ? 8 : 3;
    do {} while (0);
    SetEntitySpriteId(e, 0x35289FAE, 1);
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
    fn = AnimatedEntityToggleSpriteB;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
}

/* Animated-decor toggle state B: the sister of AnimatedEntityToggleSpriteA.
 * Same (8 if entityType==0x2F else 3) speed byte at +0x110, loads the
 * variant-B sprite (0x212A9C2D) + standard animation triad, and queues A as
 * the next state so the following pulse swaps back. The entityType field is
 * pulled into a u16 temp before the compare so cc1 colors the (8|3) result
 * into $v0 (matching TARGET) instead of $a0; the do{}while(0) after the store
 * keeps the +0x110 write ahead of the SetEntitySpriteId argument setup. */
void AnimatedEntityToggleSpriteB(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    void *def;
    u16 entityType;
    def = *(void **)((u8 *)e + 0x100);
    entityType = *(u16 *)((u8 *)def + 0x12);
    ((u8 *)e)[0x110] = (entityType == 0x2F) ? 8 : 3;
    do {} while (0);
    SetEntitySpriteId(e, 0x212A9C2D, 1);
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
    fn = AnimatedEntityToggleSpriteA;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
}

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
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
    fn = AnimatedEntityToggleSpriteA;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
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

void EntityPathFollowerWithTriggerZone(Entity *e);

/* Idle tick for a path-following sound emitter: pans its voice, and once the
 * global frame counter has advanced past 0xAA (and the render slot is empty),
 * installs the path-follower render callback. Then runs the standard
 * update + damage collision check. */
void SoundEmitterIdleTickCallback(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    UpdateEntitySoundPanning(e, *(u32 *)((u8 *)e + 0x110));
    if (*(s32 *)((u8 *)e + 0x104) != 0) {
        if (*(s16 *)((u8 *)e + 0x1E) == 0) {
            if (g_pGameState->frame_counter >= 0xAA) {
                do {} while (0);
                fn = EntityPathFollowerWithTriggerZone;
                do {} while (0);
                m1 = -1;
                SLOT_STORE(slot.s, e->renderMarker, m1, fn);
            }
        }
    }
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, 0x1000, 1);
}

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
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0xED209C94, 1);
    fn = CollectibleIdleState;
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, fn);
}

void InitConditionalCollectibleEntity(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = ConditionalCollectibleTick;
    m1 = -1;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    ((u8 *)e)[0x111] = 3;
    do {} while (0);
    fn = EntityEventHandlerCountdownToWalkWithSprite;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0x6D209C95, 1);
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
    fn = CollectibleIdleState;
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, fn);
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
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerIdle;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
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
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, nextFn);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitAnimatedTimedCollectible);

/* Collectible tick gated by a per-entity frame phase: once every 152 global
 * frames (when frame_counter % 152 matches this entity's phase), retargets the
 * animation to frame (frame_counter / 76), rewinds it, and reinstalls
 * CollectibleTickCallback into the tick slot. Runs the collectible tick every
 * frame regardless. */
void CollectibleFrameConditionalTick(ConditionalPhaseEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    if (g_pGameState->frame_counter % 152 == e->framePhase) {
        SetAnimationTargetFrameIndex((Entity *)e, -1);
        SetAnimationLoopFrameIndex((Entity *)e, 0);
        SetAnimationFrameIndex((Entity *)e, 0);
        do {} while (0);
        fn = CollectibleTickCallback;
        m1 = -1;
        SLOT_STORE(slot.s, e->sprite.base.tickMarker, m1, fn);
    }
    CollectibleTickCallback((Entity *)e);
}

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
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerWalk;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
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
    SLOT_STORE(slot.s[0], se->queuedStateMarker, m1, nextFn);
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
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    /* Animation-callback hash; value collides with FX_KLAY_FOOTSTEP_QUIET but
     * is unrelated (this is a generic enemy, not Klaymen). */
    SetAnimationSpriteCallback(e, 0x44D4C8D8); /* NOLINT_ASSET_ID: hash collision */
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
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    fn = EntityEventHandlerTimerCountdown;
    SLOT_STORE(slot.s[0], e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0x611C5804, 1);
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->renderMarker);
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
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
    SLOT_STORE(slot.s[0], se->queuedStateMarker, m1, nextFn);
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
    SLOT_STORE(slot.s, e->renderMarker, m1, fn);
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

void EntityDestroyCallback_Vt80010DE4_800411cc(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

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
    SetAnimationTargetFrameIndex(e, 0);
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

/* Offscreen cull: if this entity AND its linked child (+0x100) are both
 * offscreen, clear the child's active bit (+0x16 bit 0), detach it, and
 * notify the GameState event FSM with EVT_GAME_NOTIFY (3, arg 0, srcEntity=e).
 * First of a five-member family differing only in which linked slot is
 * culled (+0x100/+0x104/+0x108) and an optional trailing tick. */
void CheckAndDisableChildEntityOffscreen(OffscreenCullEntity *e) {
    GameState *gs;
    s16 m;
    FSM_REG(GsCullNotifyCB, fn, "$8");   /* $t0 home (jalr target) */
    FSM_REG(GsCullNotifyCB, ft, "$19");  /* $s3 then-fn (relays into $t0) */
    FSM_REG(s32, slotArg, "$18");        /* $s2 slot arg */
    s32 adj;
    s32 lo;
    int slotArgWide;
    s16 t;
    s16 s;
    FSM_REG(OffscreenCullEntity *, self, "$17");
    FSM_REG(s32, off, "$16");
    Entity *c;

    self = e;
    off = 0;
    if (self->cullChild != NULL) {
        if (IsEntityOffScreen((Entity *)self) != 0) {
            off = IsEntityOffScreen_EntityLoop(g_pGameState, self->cullChild) != 0;
        }
    }
    if (off != 0) {
        c = self->cullChild;
        *((u8 *)c + 0x16) &= 0xFE;
        gs = g_pGameState;
        self->cullChild = NULL;
        m = ((s16 *)&gs->event_marker)[1];
        if (m != 0) {
            t = m;
            FSM_RELAY(s, t); /* stages the survivor through $v0 */
            if (m > 0) {
                GsCullNotifySlot *base =
                    *(GsCullNotifySlot **)((u8 *)gs + *(s16 *)&gs->event_callback);
                slotArg = base[m - 1].arg;
                ft = base[m - 1].fn;
                FSM_RELAY(fn, ft);
            } else {
                fn = (GsCullNotifyCB)gs->event_callback;
            }
            slotArgWide = slotArg;
            lo = ((s16 *)&gs->event_marker)[0];
            if (s > 0) {
                adj = (s16)slotArgWide + lo;
            } else {
                adj = lo;
            }
            FSM_KEEP_LIVE(s);
            fn((void *)((u8 *)gs + adj), 3, 0, self);
        }
    }
}

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

/* Sibling of CheckAndDisableChildEntityOffscreen for the +0x108 slot. */
void CheckAndDisableSpawnDataOffscreen(OffscreenCullEntity *e) {
    GameState *gs;
    s16 m;
    FSM_REG(GsCullNotifyCB, fn, "$8");   /* $t0 home (jalr target) */
    FSM_REG(GsCullNotifyCB, ft, "$19");  /* $s3 then-fn (relays into $t0) */
    FSM_REG(s32, slotArg, "$18");        /* $s2 slot arg */
    s32 adj;
    s32 lo;
    int slotArgWide;
    s16 t;
    s16 s;
    FSM_REG(OffscreenCullEntity *, self, "$17");
    FSM_REG(s32, off, "$16");
    Entity *c;

    self = e;
    off = 0;
    if (self->cullSpawnData != NULL) {
        if (IsEntityOffScreen((Entity *)self) != 0) {
            off = IsEntityOffScreen_EntityLoop(g_pGameState, self->cullSpawnData) != 0;
        }
    }
    if (off != 0) {
        c = self->cullSpawnData;
        *((u8 *)c + 0x16) &= 0xFE;
        gs = g_pGameState;
        self->cullSpawnData = NULL;
        m = ((s16 *)&gs->event_marker)[1];
        if (m != 0) {
            t = m;
            FSM_RELAY(s, t); /* stages the survivor through $v0 */
            if (m > 0) {
                GsCullNotifySlot *base =
                    *(GsCullNotifySlot **)((u8 *)gs + *(s16 *)&gs->event_callback);
                slotArg = base[m - 1].arg;
                ft = base[m - 1].fn;
                FSM_RELAY(fn, ft);
            } else {
                fn = (GsCullNotifyCB)gs->event_callback;
            }
            slotArgWide = slotArg;
            lo = ((s16 *)&gs->event_marker)[0];
            if (s > 0) {
                adj = (s16)slotArgWide + lo;
            } else {
                adj = lo;
            }
            FSM_KEEP_LIVE(s);
            fn((void *)((u8 *)gs + adj), 3, 0, self);
        }
    }
}

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
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
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
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntitySimpleEventPassthrough_V2;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, platRef->spawn->spriteId, 1);
    SetAnimationLoopFrame(e, 0x20140828);
    fn = EntityHideAndDisable;
    SLOT_STORE(slot.s, se->queuedStateMarker, m1, fn);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitDirectionalScaledEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformTimerTickCallback);

void PlatformCollisionTickCallback(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
    CheckAndDisableSpawnRefOffscreen(e);
}

/* Sibling of CheckAndDisableChildEntityOffscreen for the +0x104 slot. */
void CheckAndDisableSpawnRefOffscreen(OffscreenCullEntity *e) {
    GameState *gs;
    s16 m;
    FSM_REG(GsCullNotifyCB, fn, "$8");   /* $t0 home (jalr target) */
    FSM_REG(GsCullNotifyCB, ft, "$19");  /* $s3 then-fn (relays into $t0) */
    FSM_REG(s32, slotArg, "$18");        /* $s2 slot arg */
    s32 adj;
    s32 lo;
    int slotArgWide;
    s16 t;
    s16 s;
    FSM_REG(OffscreenCullEntity *, self, "$17");
    FSM_REG(s32, off, "$16");
    Entity *c;

    self = e;
    off = 0;
    if (self->cullSpawnRef != NULL) {
        if (IsEntityOffScreen((Entity *)self) != 0) {
            off = IsEntityOffScreen_EntityLoop(g_pGameState, self->cullSpawnRef) != 0;
        }
    }
    if (off != 0) {
        c = self->cullSpawnRef;
        *((u8 *)c + 0x16) &= 0xFE;
        gs = g_pGameState;
        self->cullSpawnRef = NULL;
        m = ((s16 *)&gs->event_marker)[1];
        if (m != 0) {
            t = m;
            FSM_RELAY(s, t); /* stages the survivor through $v0 */
            if (m > 0) {
                GsCullNotifySlot *base =
                    *(GsCullNotifySlot **)((u8 *)gs + *(s16 *)&gs->event_callback);
                slotArg = base[m - 1].arg;
                ft = base[m - 1].fn;
                FSM_RELAY(fn, ft);
            } else {
                fn = (GsCullNotifyCB)gs->event_callback;
            }
            slotArgWide = slotArg;
            lo = ((s16 *)&gs->event_marker)[0];
            if (s > 0) {
                adj = (s16)slotArgWide + lo;
            } else {
                adj = lo;
            }
            FSM_KEEP_LIVE(s);
            fn((void *)((u8 *)gs + adj), 3, 0, self);
        }
    }
}

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
    SLOT_STORE(slot.s[0], e->tickMarker, m1, fn);
    /* @hack: SLOT_CLEAR routes the scratch-build + struct-value-store through the macro so the marker/fn write order matches the original codegen; see memories/repo/decomp-patterns.md. */
    SLOT_CLEAR(slot.s[0], e->eventMarker);
}

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitBouncableClayEntity);

/* Byte-identical duplicate of CheckAndDisableChildEntityOffscreen (the
 * original binary compiled the same code twice). */
void EntityOffScreenChildCleanup(OffscreenCullEntity *e) {
    GameState *gs;
    s16 m;
    FSM_REG(GsCullNotifyCB, fn, "$8");   /* $t0 home (jalr target) */
    FSM_REG(GsCullNotifyCB, ft, "$19");  /* $s3 then-fn (relays into $t0) */
    FSM_REG(s32, slotArg, "$18");        /* $s2 slot arg */
    s32 adj;
    s32 lo;
    int slotArgWide;
    s16 t;
    s16 s;
    FSM_REG(OffscreenCullEntity *, self, "$17");
    FSM_REG(s32, off, "$16");
    Entity *c;

    self = e;
    off = 0;
    if (self->cullChild != NULL) {
        if (IsEntityOffScreen((Entity *)self) != 0) {
            off = IsEntityOffScreen_EntityLoop(g_pGameState, self->cullChild) != 0;
        }
    }
    if (off != 0) {
        c = self->cullChild;
        *((u8 *)c + 0x16) &= 0xFE;
        gs = g_pGameState;
        self->cullChild = NULL;
        m = ((s16 *)&gs->event_marker)[1];
        if (m != 0) {
            t = m;
            FSM_RELAY(s, t); /* stages the survivor through $v0 */
            if (m > 0) {
                GsCullNotifySlot *base =
                    *(GsCullNotifySlot **)((u8 *)gs + *(s16 *)&gs->event_callback);
                slotArg = base[m - 1].arg;
                ft = base[m - 1].fn;
                FSM_RELAY(fn, ft);
            } else {
                fn = (GsCullNotifyCB)gs->event_callback;
            }
            slotArgWide = slotArg;
            lo = ((s16 *)&gs->event_marker)[0];
            if (s > 0) {
                adj = (s16)slotArgWide + lo;
            } else {
                adj = lo;
            }
            FSM_KEEP_LIVE(s);
            fn((void *)((u8 *)gs + adj), 3, 0, self);
        }
    }
}

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
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = (void (*)())HazardEventHandler_0x1001;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0xF175320E, 1);
}

/* Destroying state for a bounce-clay hazard: arms the +0x110 timer, installs
 * the hazard timer tick + event passthrough, swaps to the destroying sprite,
 * and queues BounceClay_HiddenState as the next state. */
void BounceClay_HiddenState(Entity *e);

void BounceClay_DestroyingState(HazardTimerEntity *e) {
    TripadSlot slot;
    void (*fn)();
    s16 m1;
    Entity *entity = (Entity *)e;
    SpriteEntity *spriteEntity = (SpriteEntity *)e;

    e->timer = 0x12C;
    do {} while (0);
    fn = (void (*)())HazardTimerTickCallback;
    m1 = -1;
    SLOT_STORE(slot.s, entity->tickMarker, m1, fn);
    fn = (void (*)())EntityEventPassthrough_Event2;
    SLOT_STORE(slot.s, entity->eventMarker, m1, fn);
    SetEntitySpriteId(entity, 0x657C322C, 1);
    fn = BounceClay_HiddenState;
    SLOT_STORE(slot.s, spriteEntity->queuedStateMarker, m1, fn);
}

INCLUDE_ASM("asm/nonmatchings/enemies", HazardActiveState);

/* Hidden state for a bounce-clay hazard: clears the event slot, installs the
 * hazard timer tick, sets the hidden sprite + blank anim, drops render flags,
 * and clears the child sprite-context active byte. */
void BounceClay_HiddenState(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    SLOT_CLEAR(slot.s[0], e->eventMarker);
    fn = HazardTimerTickCallback;
    m1 = -1;
    do { slot.s[0].markerLo = 0; slot.s[0].markerHi = m1; slot.s[0].fn = fn; } while (0);
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    SetEntitySpriteId(e, 0xE7443A6F, 1);
    SetAnimationTargetFrameIndex(e, 0);
    EntitySetRenderFlags(e, 0);
    *((u8 *)e->spriteContext + 0xA) = 0;
}

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

extern void InitSwitchActivatedState(Entity *e);

/* Hides an entity for 0x1E ticks: arms the byte timer at +0x104, clears the
 * child sub-entity's active byte (+0x34 -> +0xA), drops render flags, then
 * installs the timed-tick callback, clears the event slot, and queues
 * InitSwitchActivatedState to fire when the timer elapses. */
void EntityHideWithTimer(TimedByteEntity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    e->timer = 0x1E;
    *((u8 *)e->sprite.base.spriteContext + 0xA) = 0;
    EntitySetRenderFlags((Entity *)e, 0);
    fn = TimedEntityTickCallback;
    m1 = -1;
    SLOT_STORE(slot.s[0], e->sprite.base.tickMarker, m1, fn);
    SLOT_CLEAR(slot.s[0], e->sprite.base.eventMarker);
    fn = InitSwitchActivatedState;
    SLOT_STORE(slot.s[0], e->sprite.queuedStateMarker, m1, fn);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitSwitchActivatedState);

void InitSwitchMovingState(Entity *e) {
    PadSlot slot;
    s16 m1 = -1;
    void (*fn)();

    fn = EntityUpdateWithCollisionWrapper;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityIncrementWorldX;
    SLOT_STORE(slot.s, e->renderMarker, m1, fn);
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

/* Teleporter "exit/settle" state: if a linked entity (+0x108) exists, hides its
 * sprite context (active byte +0xA = 0); restores the entity's render scale
 * (0x50/0x54) from the saved value at +0x104; installs the plain update tick +
 * the portal event handler on the tick/event slots; swaps to the exit sprite
 * (0x1A2AB594) and clears the render flags.
 *
 * SHELVED (single-instruction scheduling diff): the body is fully correct and
 * every instruction matches EXCEPT the `beqz $v0` (linkedEntity null-check) delay
 * slot — cc1 fills it with the `move a0,s0` call-arg setup, while TARGET leaves it
 * `nop` and emits `move a0,s0` later (after the two fn-pointer loads). That single
 * placement shifts everything after by 4 bytes. Two named fn-ptr locals assigned
 * up front reproduce TARGET's hoist of both callback addresses (a2 + t1); a
 * do{}while(0) after the if-block did NOT stop the delay-slot fill, and the
 * permuter (-j4, 150s) floored at 230 — it can't empty a cc1-filled branch delay
 * slot. Equivalent C (parked in nonmatchings/TeleporterExitState):
 *   typedef struct { SpriteEntity sprite; u8 pad100[4]; s32 savedScale;
 *       Entity *linkedEntity; } TeleporterExitEntity;   // +0x104 / +0x108
 *   sub = e->linkedEntity;
 *   if (sub != NULL) ((u8 *)sub->spriteContext)[0xA] = 0;
 *   fnTick = EntityUpdateCallback; fnEvent = TeleporterPortalEventHandler;
 *   e->sprite.base.scaleRender = e->sprite.base.scaleRender2 = e->savedScale;
 *   <install {0,-1,fnTick} @tickMarker, {0,-1,fnEvent} @eventMarker>
 *   SetEntitySpriteId(e, 0x1A2AB594, 1); EntitySetRenderFlags(e, 0); */
INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterExitState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitIndexedSpriteEntity);

/* CheckAndDisableChildEntityOffscreen + unconditional EntityUpdateCallback
 * tick afterwards (per-frame parent cleanup wrapper). */
void EntityOffscreenParentCleanupTick(OffscreenCullEntity *e) {
    GameState *gs;
    s16 m;
    FSM_REG(GsCullNotifyCB, fn, "$8");   /* $t0 home (jalr target) */
    FSM_REG(GsCullNotifyCB, ft, "$19");  /* $s3 then-fn (relays into $t0) */
    FSM_REG(s32, slotArg, "$18");        /* $s2 slot arg */
    s32 adj;
    s32 lo;
    int slotArgWide;
    s16 t;
    s16 s;
    FSM_REG(OffscreenCullEntity *, self, "$16");
    FSM_REG(s32, off, "$17");
    Entity *c;

    self = e;
    off = 0;
    if (self->cullChild != NULL) {
        if (IsEntityOffScreen((Entity *)self) != 0) {
            off = IsEntityOffScreen_EntityLoop(g_pGameState, self->cullChild) != 0;
        }
    }
    if (off != 0) {
        c = self->cullChild;
        *((u8 *)c + 0x16) &= 0xFE;
        gs = g_pGameState;
        self->cullChild = NULL;
        m = ((s16 *)&gs->event_marker)[1];
        if (m != 0) {
            t = m;
            FSM_RELAY(s, t); /* stages the survivor through $v0 */
            if (m > 0) {
                GsCullNotifySlot *base =
                    *(GsCullNotifySlot **)((u8 *)gs + *(s16 *)&gs->event_callback);
                slotArg = base[m - 1].arg;
                ft = base[m - 1].fn;
                FSM_RELAY(fn, ft);
            } else {
                fn = (GsCullNotifyCB)gs->event_callback;
            }
            slotArgWide = slotArg;
            lo = ((s16 *)&gs->event_marker)[0];
            if (s > 0) {
                adj = (s16)slotArgWide + lo;
            } else {
                adj = lo;
            }
            FSM_KEEP_LIVE(s);
            fn((void *)((u8 *)gs + adj), 3, 0, self);
        }
    }
    EntityUpdateCallback((Entity *)self);
}

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

/* Two-probe ground snap: after applying the animation's position offsets,
 * sample the tile just above (worldY-7) and just below (worldY+2) the entity.
 * The first probe that reports a solid ground tile (nonzero, < 0x3C) settles
 * the entity's worldY onto that tile's slope height. */
void EntityGroundSnapMovementCallback(Entity *e) {
    u8 tile;

    ApplyAnimationPositionOffsets(e);
    tile = EntityApplyMovementCallbacks(e, e->worldX, (s16)(e->worldY - 7));
    if (tile != 0 && tile < 0x3C) {
        e->worldY = CalcEntityYFromTileHeight(e, tile, e->worldX,
                                              (s16)(e->worldY - 7));
        return;
    }
    tile = EntityApplyMovementCallbacks(e, e->worldX, (s16)(e->worldY + 2));
    if (tile != 0 && tile < 0x3C) {
        e->worldY = CalcEntityYFromTileHeight(e, tile, e->worldX,
                                              (s16)(e->worldY + 2));
    }
}

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

extern void EntityGroundSnapMovementCallback();

/* Switches an entity into its "special collectible" state: installs the
 * ground-snap render-move callback, the scaled collectible tick, and the
 * walk-with-timer event handler (all direct-call), then sets the sprite. */
void EntityStateSetSpecial(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = EntityGroundSnapMovementCallback;
    m1 = -1;
    SLOT_STORE(slot.s, e->renderMarker, m1, fn);
    fn = CollectibleScaledTickCallback;
    SLOT_STORE(slot.s, e->tickMarker, m1, fn);
    fn = EntityEventHandlerWalkWithTimer;
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
    SetEntitySpriteId(e, 0xE6830101, 1);
}

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
    SLOT_STORE(slot.s, e->eventMarker, m1, fn);
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
            SetAnimationTargetFrameIndex((Entity *)sparkle, sparkle->sprite.currentFrame);
            EntitySetRenderFlags((Entity *)sparkle, 0);
        }
    }
    return 0;
}

void InitBackgroundSparkleRevealState(BackgroundSparkleEntity *e) {
    e->contactFlag = 1;
    SetAnimationTargetFrameIndex((Entity *)e, -1);
    SetAnimationFrameCallback((Entity *)e, ANIM_FINISHED_CB);
}

void SetEntityAnimationState(BackgroundSparkleEntity *sparkle) {
    BackgroundSparkleChildContext *child = sparkle->sprite.base.spriteContext;
    child->activeFlag = 1;
    sparkle->contactFlag = 0;
    EntitySetRenderFlags((Entity *)sparkle, 1);
    SetAnimationLoopFrame((Entity *)sparkle, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback((Entity *)sparkle, ANIM_FINISHED_CB);
    SetAnimationFrameIndex((Entity *)sparkle, 0);
}

void StartBackgroundSparkleFade(BackgroundSparkleEntity *e) {
    e->revealTimer = 0x14;
}

/* enemies .sdata (0x800A5A50..0x800A5B60): {marker=0xFFFF0000, callback}
 * descriptor pairs for the enemy / collectible / platform / hazard / teleporter
 * state machines, with three "END2" (0x32444E45) sentinel+pad terminators
 * embedded mid-table. Migrated from the pooled asm sdata blob (sdata-under-split
 * Phase 4). Address order == declaration order (cc1 2.7.2 emits initialized
 * .sdata in decl order). Five pairs carry friendly names (forward-declared
 * above) used by matched code earlier in the file; the rest use D_ names.
 * Callbacks defined elsewhere are extern-declared; all are cast to EntityCallback. */
extern void EnemyStartMovingWithSound();
extern void EntityShowAndActivate();
extern void PlatformShowAndActivate();
extern void HazardActiveState();
extern void TeleporterActivate();
extern void TeleporterExitState();
u32 D_800A5A50 asm("D_800A5A50") = 0xFFFF0000;
EntityCallback D_800A5A54 asm("D_800A5A54") = (EntityCallback)StartAnimSequence4A;
u32 D_800A5A58 asm("D_800A5A58") = 0xFFFF0000;
EntityCallback D_800A5A5C asm("D_800A5A5C") = (EntityCallback)StartAnimSequence4B;
u32 D_800A5A60 asm("D_800A5A60") = 0xFFFF0000;
EntityCallback D_800A5A64 asm("D_800A5A64") = (EntityCallback)StartAnimSequence4C;
u32 SOUND_EMITTER_STUN_EXPIRED_STATE_MARKER asm("D_800A5A68") = 0xFFFF0000;
EntityCallback SOUND_EMITTER_STUN_EXPIRED_STATE_CALLBACK asm("D_800A5A6C") = (EntityCallback)EnemyStartMovingWithSound;
u32 D_800A5A70 asm("D_800A5A70") = 0xFFFF0000;
EntityCallback D_800A5A74 asm("D_800A5A74") = (EntityCallback)EnemyEnterSoundEmitterState;
u32 D_800A5A78 asm("D_800A5A78") = 0xFFFF0000;
EntityCallback D_800A5A7C asm("D_800A5A7C") = (EntityCallback)LaserMonkeyIdleState;
u32 D_800A5A80 asm("D_800A5A80") = 0xFFFF0000;
EntityCallback D_800A5A84 asm("D_800A5A84") = (EntityCallback)LaserMonkeyWalkState;
u32 D_800A5A88 asm("D_800A5A88") = 0xFFFF0000;
EntityCallback D_800A5A8C asm("D_800A5A8C") = (EntityCallback)EnemyPatrolState;
u32 D_800A5A90 asm("D_800A5A90") = 0xFFFF0000;
EntityCallback D_800A5A94 asm("D_800A5A94") = (EntityCallback)EnemyDeathState;
u32 D_800A5A98 asm("D_800A5A98") = 0xFFFF0000;
EntityCallback D_800A5A9C asm("D_800A5A9C") = (EntityCallback)StartAnimSequence3A;
u32 D_800A5AA0 asm("D_800A5AA0") = 0xFFFF0000;
EntityCallback D_800A5AA4 asm("D_800A5AA4") = (EntityCallback)StartAnimSequence3B;
u32 D_800A5AA8 asm("D_800A5AA8") = 0xFFFF0000;
EntityCallback D_800A5AAC asm("D_800A5AAC") = (EntityCallback)StartAnimSequence9Frames;
u32 D_800A5AB0 asm("D_800A5AB0") = 0xFFFF0000;
EntityCallback D_800A5AB4 asm("D_800A5AB4") = (EntityCallback)EnemySetLoopingAnimation;
u32 D_800A5AB8 asm("D_800A5AB8") = 0xFFFF0000;
EntityCallback D_800A5ABC asm("D_800A5ABC") = (EntityCallback)CollectibleIdleState;
u32 D_800A5AC0 asm("D_800A5AC0") = 0xFFFF0000;
EntityCallback D_800A5AC4 asm("D_800A5AC4") = (EntityCallback)CollectibleWalkState;
u32 D_800A5AC8 asm("D_800A5AC8") = 0xFFFF0000;
EntityCallback D_800A5ACC asm("D_800A5ACC") = (EntityCallback)InitEntityState_Idle;
u32 ENEMY_ANIMATED_CALLBACK_MARKER asm("D_800A5AD0") = 0xFFFF0000;
EntityCallback ENEMY_ANIMATED_CALLBACK_FN asm("D_800A5AD4") = (EntityCallback)EntitySetFacingRight;
u32 D_800A5AD8[2] asm("D_800A5AD8") = {0x32444E45, 0x00000000};
u32 D_800A5AE0 asm("D_800A5AE0") = 0xFFFF0000;
EntityCallback D_800A5AE4 asm("D_800A5AE4") = (EntityCallback)EntityHideAndDisable;
u32 PLATFORM_ACTIVATE_BY_FRAME_STATE_MARKER asm("D_800A5AE8") = 0xFFFF0000;
EntityCallback PLATFORM_ACTIVATE_BY_FRAME_STATE_CALLBACK asm("D_800A5AEC") = (EntityCallback)EntityShowAndActivate;
u32 D_800A5AF0 asm("D_800A5AF0") = 0xFFFF0000;
EntityCallback D_800A5AF4 asm("D_800A5AF4") = (EntityCallback)PlatformShowAndActivate;
u32 D_800A5AF8 asm("D_800A5AF8") = 0xFFFF0000;
EntityCallback D_800A5AFC asm("D_800A5AFC") = (EntityCallback)PlatformHideAndDisable;
u32 D_800A5B00 asm("D_800A5B00") = 0xFFFF0000;
EntityCallback D_800A5B04 asm("D_800A5B04") = (EntityCallback)BounceClay_IdleState;
u32 HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B08") = 0xFFFF0000;
EntityCallback HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B0C") = (EntityCallback)HazardActiveState;
u32 HAZARD_READY_STATE_MARKER asm("D_800A5B10") = 0xFFFF0000;
EntityCallback HAZARD_READY_STATE_CALLBACK asm("D_800A5B14") = (EntityCallback)BounceClay_DestroyingState;
u32 D_800A5B18 asm("D_800A5B18") = 0xFFFF0000;
EntityCallback D_800A5B1C asm("D_800A5B1C") = (EntityCallback)EntityHideWithTimer;
u32 D_800A5B20 asm("D_800A5B20") = 0xFFFF0000;
EntityCallback D_800A5B24 asm("D_800A5B24") = (EntityCallback)TeleporterActivate;
u32 D_800A5B28 asm("D_800A5B28") = 0xFFFF0000;
EntityCallback D_800A5B2C asm("D_800A5B2C") = (EntityCallback)TeleporterExitState;
u32 D_800A5B30[2] asm("D_800A5B30") = {0x32444E45, 0x00000000};
u32 D_800A5B38 asm("D_800A5B38") = 0xFFFF0000;
EntityCallback D_800A5B3C asm("D_800A5B3C") = (EntityCallback)StartAnimSequence13Frames;
u32 D_800A5B40 asm("D_800A5B40") = 0xFFFF0000;
EntityCallback D_800A5B44 asm("D_800A5B44") = (EntityCallback)StartAnimSequence7Frames;
u32 D_800A5B48 asm("D_800A5B48") = 0xFFFF0000;
EntityCallback D_800A5B4C asm("D_800A5B4C") = (EntityCallback)EnemySetWalkSprite;
u32 D_800A5B50 asm("D_800A5B50") = 0xFFFF0000;
EntityCallback D_800A5B54 asm("D_800A5B54") = (EntityCallback)EnemySetIdleSprite;
u32 D_800A5B58[2] asm("D_800A5B58") = {0x32444E45, 0x00000000};


/* .data island 0x8009B55C..0x8009B634 (216B, enemy behavior dispatch tables) migrated from asm. */
extern void EntityStartWalkWithTimer0x2d();
extern void EntitySetSparkleDelay1();
extern void EntityStateSetSparkleCollectible();
extern void StartAnimSequence4A();
extern void EntitySetSparkleDelay2();
extern void StartAnimSequence4B();
extern void EntitySetSparkleDelay3();
extern void StartAnimSequence4C();
extern void InitCollectibleTimer0x3c_SpriteB();
extern void InitShooterEnemyStateB();
extern void StartAnimSequence3A();
extern void InitCollectibleTimer0x3c_SpriteA();
extern void InitShooterEnemyStateA();
extern void StartAnimSequence3B();
extern void InitCollectibleTimer0x1e_SpriteC();
extern void InitCollectibleIdleStateB();
extern void InitCollectibleTimer0x1e();
extern void InitCollectibleIdleState();
extern void StartAnimSequence9Frames();

void *D_8009B55C[8] asm("D_8009B55C") = {
    (void *)0xFFFF0000,
    (void *)EntityStartWalkWithTimer0x2d,
    (void *)0xFFFF0000,
    (void *)EntitySetSparkleDelay1,
    (void *)0xFFFF0000,
    (void *)EntityStateSetSparkleCollectible,
    (void *)0xFFFF0000,
    (void *)StartAnimSequence4A,
};

void *D_8009B57C[8] asm("D_8009B57C") = {
    (void *)0xFFFF0000,
    (void *)EntityStartWalkWithTimer0x2d,
    (void *)0xFFFF0000,
    (void *)EntitySetSparkleDelay2,
    (void *)0xFFFF0000,
    (void *)EntityStateSetSparkleCollectible,
    (void *)0xFFFF0000,
    (void *)StartAnimSequence4B,
};

void *D_8009B59C[8] asm("D_8009B59C") = {
    (void *)0xFFFF0000,
    (void *)EntityStartWalkWithTimer0x2d,
    (void *)0xFFFF0000,
    (void *)EntitySetSparkleDelay3,
    (void *)0xFFFF0000,
    (void *)EntityStateSetSparkleCollectible,
    (void *)0xFFFF0000,
    (void *)StartAnimSequence4C,
};

void *D_8009B5BC[6] asm("D_8009B5BC") = {
    (void *)0xFFFF0000,
    (void *)InitCollectibleTimer0x3c_SpriteB,
    (void *)0xFFFF0000,
    (void *)InitShooterEnemyStateB,
    (void *)0xFFFF0000,
    (void *)StartAnimSequence3A,
};

void *D_8009B5D4[6] asm("D_8009B5D4") = {
    (void *)0xFFFF0000,
    (void *)InitCollectibleTimer0x3c_SpriteA,
    (void *)0xFFFF0000,
    (void *)InitShooterEnemyStateA,
    (void *)0xFFFF0000,
    (void *)StartAnimSequence3B,
};

void *D_8009B5EC[18] asm("D_8009B5EC") = {
    (void *)0xFFFF0000,
    (void *)InitCollectibleTimer0x1e_SpriteC,
    (void *)0xFFFF0000,
    (void *)InitShooterEnemyStateA,
    (void *)0xFFFF0000,
    (void *)InitCollectibleTimer0x1e_SpriteC,
    (void *)0xFFFF0000,
    (void *)InitCollectibleIdleStateB,
    (void *)0xFFFF0000,
    (void *)InitCollectibleTimer0x1e,
    (void *)0xFFFF0000,
    (void *)InitShooterEnemyStateB,
    (void *)0xFFFF0000,
    (void *)InitCollectibleTimer0x1e,
    (void *)0xFFFF0000,
    (void *)InitCollectibleIdleState,
    (void *)0xFFFF0000,
    (void *)StartAnimSequence9Frames,
};
