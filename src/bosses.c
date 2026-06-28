#include "common.h"
#include "Game/asset_ids.h"
#include "functions.h"
#include "Game/callback_slot.h"
#include "globals.h"

extern void *g_pBlbHeapBase;
extern void *BOSS_ENTITY_VTABLE asm("D_800112E8");
extern void *MONKEY_MAGE_BOSS_VTABLE asm("D_80011308");
extern void *BOSS_AUX_ENTITY_VTABLE asm("D_80011328");
extern void *MONKEY_MAGE_AUX_VTABLE asm("D_80011348");
extern void *BOSS_PARTICLE_SPAWN_VTABLE asm("D_80011368");
extern u8 BOSS_SPU_CLEANUP_VTABLE[] asm("D_80011388");
extern u8 BOSS_SPU_STOP_PRE_VTABLE[] asm("D_800112C8");
extern void *KLOGG_BOSS_VTABLE asm("D_80011268");
extern void *JOE_HEAD_JOE_BOSS_VTABLE asm("D_80011288");
extern void *MONKEY_MAGE_SIMPLE_ALLOC_VTABLE asm("D_800113A8");

extern s32 EntityMessageHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 BossEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 JoeHeadJoeAttackEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void JoeHeadJoeUpdateWithCollisionCheck(Entity *e);
extern void JoeHeadJoeSetIdleState(Entity *e);
extern void EnemyUpdateWithCollisionAndDeath(Entity *e);
extern void EnemyTickWithCollision(Entity *e);
extern s32 ShrineyGuardAttackEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 GlennYntisDamageEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 GlennYntisEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void SetAnimationLoopFrame(Entity *e, u32 frame);
extern void SetAnimationSpriteCallback(Entity *e, u32 spriteId);
extern void SetAnimationFrameIndex(Entity *e, s32 frame);
extern u32  PlayEntityPositionSound(Entity *e, u32 soundId);
extern void EntitySetCallback(Entity *e, u32 marker, EntityCallback fn);
extern void SetEntityFacingDirection(Entity *e, s32 dir);
extern void JoeHeadJoeMoveAndCheckAttack(Entity *e);
extern void ShrineyGuardMoveCallback(Entity *e);
extern void EntityTickWithTimer(Entity *e);
extern void GliderStartFallState(Entity *e);
extern void EntityTickWithReadyCheck(Entity *e);
extern s32 GliderEventHandlerWithCounter();
extern void GliderSetActiveState(Entity *e);
extern void GliderWaitState(Entity *e);
extern void KloggMoveToTargetPosition(Entity *e);
extern void KloggSpawnProjectilesCallback(Entity *e);
extern void KloggDeathEventHandler();
extern void MonkeyMageDeathCallback(u8 *e);
void KloggDeathCallback(u8 *e);
void GlennYntisSetPhaseFromHP();
extern s32 EnemyHitMessageHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void GlennYntisSelectRandomAnimState();
extern void HazardActivateWithSound(Entity *e);
extern void GlennYntisAttackEventHandler(Entity *e);
extern void EntitySetRenderFlags(Entity *e, u32 flags);
extern void EntityDestroyWithEffects(Entity *e);
extern void CollectibleTickCallback(Entity *e);
extern void UpdateEntitySoundPanning(Entity *e, u32 sound);
extern void SetAnimationSpriteId(Entity *e, s32 id);
extern Entity *CreateFadeOverlayEntity(Entity *e);
extern void AddToZOrderList(GameState *gs, Entity *entity);
extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");

typedef struct BossTimedSpriteEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x104 - 0x100];
    /* 0x104 */ u16 shortTimer;
} BossTimedSpriteEntity;

typedef struct BossVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x118 - 0x100];
    /* 0x118 */ s32 voiceHandle;
} BossVoiceEntity;

typedef struct BossVoice144Entity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x144 - 0x100];
    /* 0x144 */ s32 voiceHandle;
} BossVoice144Entity;

typedef struct HazardTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x114 - 0x100];
    /* 0x114 */ u16 behaviorTimer;
} HazardTimerEntity;

typedef struct ShrineyGuardEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10C - 0x100];
    /* 0x10C */ u8 readyFlag;
    /* 0x10D */ u8 pad10D[0x111 - 0x10D];
    /* 0x111 */ u8 activeTimer;
    /* 0x112 */ u8 idleTimeout;
    /* 0x113 */ u8 stunTimer;
    /* 0x114 */ u8 attackCounter;
    /* 0x115 */ u8 stunActive;
} ShrineyGuardEntity;

typedef struct KloggTriggerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x110 - 0x100];
    /* 0x110 */ u8 triggerTimer;
    /* 0x111 */ u8 triggerActive;
    /* 0x112 */ u8 pad112[0x134 - 0x112];
    /* 0x134 */ u8 *triggerTarget;
} KloggTriggerEntity;

typedef struct GliderEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100;
    /* 0x101 */ u8 readyFlag;
    /* 0x102 */ u8 pad102[0x10D - 0x102];
    /* 0x10D */ u8 directionFlag;
} GliderEntity;

typedef struct JoeHeadJoeEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x112 - 0x100];
    /* 0x112 */ u16 attackTimer;
    /* 0x114 */ u8 pad114[0x118 - 0x114];
    /* 0x118 */ s32 voiceHandle;
} JoeHeadJoeEntity;

typedef struct KloggBossEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x104 - 0x100];
    /* 0x104 */ u16 shortTimer;
    /* 0x106 */ u8 pad106[0x110 - 0x106];
    /* 0x110 */ s32 voiceHandle;
    /* 0x114 */ u8 pad114[2];
    /* 0x116 */ u16 voicePanTimer;
} KloggBossEntity;

/* Minimal view of the sprite-render context attached to an Entity via
 * Entity.spriteContext (+0x34). Only the active/visible byte is needed
 * by the death callbacks here. */
typedef struct SpriteRenderContextRef {
    /* 0x00 */ u8 pad00[0xA];
    /* 0x0A */ u8 activeFlag;
} SpriteRenderContextRef;
/* gp_rel tentative defs (sdata blob owns the strong defs). */
u32   GLIDER_WAKE_STATE_MARKER asm("D_800A5B68");
EntityCallback GLIDER_WAKE_STATE_CALLBACK asm("D_800A5B6C");
u32   HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B98");
EntityCallback HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B9C");
u32   HAZARD_STOP_SOUND_STATE_MARKER asm("D_800A5BC0");
EntityCallback HAZARD_STOP_SOUND_STATE_CALLBACK asm("D_800A5BC4");
u32   HAZARD_IDLE_WITH_SOUND_STATE_MARKER asm("D_800A5BC8");
EntityCallback HAZARD_IDLE_WITH_SOUND_STATE_CALLBACK asm("D_800A5BCC");
u32   GLENN_YNTIS_ANIM_B_STATE_MARKER asm("D_800A5BD0");
EntityCallback GLENN_YNTIS_ANIM_B_STATE_CALLBACK asm("D_800A5BD4");
u32   GLENN_YNTIS_ANIM_IDLE_STATE_MARKER asm("D_800A5BD8");
EntityCallback GLENN_YNTIS_ANIM_IDLE_STATE_CALLBACK asm("D_800A5BDC");
u32   GLENN_YNTIS_ANIM_C_STATE_MARKER asm("D_800A5BE0");
EntityCallback GLENN_YNTIS_ANIM_C_STATE_CALLBACK asm("D_800A5BE4");
u32   SHRINEY_GUARD_IDLE_TIMEOUT_STATE_MARKER asm("D_800A5BF0");
EntityCallback SHRINEY_GUARD_IDLE_TIMEOUT_STATE_CALLBACK asm("D_800A5BF4");
u32   SHRINEY_GUARD_REPEAT_ATTACK_STATE_MARKER asm("D_800A5C10");
EntityCallback SHRINEY_GUARD_REPEAT_ATTACK_STATE_CALLBACK asm("D_800A5C14");
u32   SHRINEY_GUARD_FINISH_ATTACK_STATE_MARKER asm("D_800A5C18");
EntityCallback SHRINEY_GUARD_FINISH_ATTACK_STATE_CALLBACK asm("D_800A5C1C");
u32   JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_MARKER asm("D_800A5C20");
EntityCallback JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_CALLBACK asm("D_800A5C24");

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggBossEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickWithReadyCheck);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickWithTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderFallTickCallback);

s32 GliderEventHandler(GliderEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 maskedEvent = event & 0xFFFF;
    if (maskedEvent == 0x1001) goto event_1001;
    if (maskedEvent >= 0x1002) goto high_event;
    if (maskedEvent == 1) goto event_1;
    goto out;
high_event:
    if (maskedEvent == 0x1016) goto event_1016;
    goto out;
event_1001:
    EntitySetState((Entity *)e, GLIDER_WAKE_STATE_MARKER, GLIDER_WAKE_STATE_CALLBACK);
    goto out;
event_1016:
    e->readyFlag = 1;
    goto out;
event_1:
    if (arg2 == 0x10D86282) goto direction_one;
    if (arg2 != 0xB0C10420) goto out;
    e->directionFlag = 0;
    goto out;
direction_one:
    e->directionFlag = 1;
out:
    return 0;
}

s32 GliderEventHandlerWithComplete(GliderEntity *e, u32 event, u32 arg2, u32 arg3) {
    GliderEntity *entity = e;
    u32 savedEvent = event;
    if ((savedEvent & 0xFFFF) == 0x1001) goto event_1001;
    if ((s32)(savedEvent & 0xFFFF) >= 0x1002) goto high_event;
    if ((savedEvent & 0xFFFF) == 1) goto event_1;
    goto check_complete;
high_event:
    if ((savedEvent & 0xFFFF) == 0x1016) goto event_1016;
    goto check_complete;
event_1001:
    EntitySetState((Entity *)entity, GLIDER_WAKE_STATE_MARKER, GLIDER_WAKE_STATE_CALLBACK);
    goto check_complete;
event_1016:
    entity->readyFlag = 1;
    goto reset_masked;
event_1:
    if (arg2 == 0x10D86282) goto direction_one;
    if (arg2 != 0xB0C10420) {
        goto check_complete;
    }
    entity->directionFlag = 0;
    goto check_complete;
direction_one:
    entity->directionFlag = 1;
reset_masked:
check_complete:
    if ((savedEvent & 0xFFFF) == 2) {
        EntityProcessCallbackQueue((Entity *)entity);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/bosses", GliderEventHandlerWithCounter);

void GliderStateInit_WakeUp(Entity *e) {
    TripadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = EntityTickWithReadyCheck;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = (void (*)())GliderEventHandlerWithCounter;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x883D14B4, 1);
    fn = GliderSetActiveState;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
}

void GliderSetActiveState(Entity *e) {
    TripadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = EntityTickWithReadyCheck;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = (void (*)())GliderEventHandlerWithComplete;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x10057441, 1);
    fn = GliderWaitState;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/bosses", GliderWaitState);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderStartFallState);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderLandedState);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderStateInit_ResetAtSpawn);

INCLUDE_ASM("asm/nonmatchings/bosses", InitMonkeyMageBoss);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyTickWithCollision);

void EntityTickWithShortTimer(Entity *e) {
    BossTimedSpriteEntity *entity = (BossTimedSpriteEntity *)e;
    if (entity->shortTimer != 0) {
        entity->shortTimer -= 1;
        if (entity->shortTimer == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    EntityUpdateCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", EntityMessageHandler);

s32 EntityEventHandlerWithComplete(Entity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = EntityMessageHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue(e);
    }
    return result;
}

s32 KloggEventHandlerWithTrigger(KloggTriggerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u8 timer;
    u32 maskedEvent = event & 0xFFFF;
    result = EntityMessageHandler((Entity *)e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        e->triggerActive = 1;
        timer = e->triggerTimer - 1;
        e->triggerTimer = timer;
        if (timer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        } else if (timer == 1) {
            e->triggerTarget[0x101] = 0x14;
        }
    }
    return result;
}

/* Layout for an entity that walks through a fixed sequence of (x,y)
 * waypoints. Each entry in the path table is 4 bytes: an s16 x and an
 * s16 y stored consecutively. nextWaypointIndex is bumped on every
 * tick until it reaches waypointCount; from that point on the entity
 * falls through to its queued state instead. */
typedef struct FollowPathEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x139 - 0x100];
    /* 0x139 */ u8 nextWaypointIndex;
    /* 0x13A */ u8 waypointCount;
    /* 0x13B */ u8 pad13B;
    /* 0x13C */ s16 *path;
} FollowPathEntity;

/* Single-step waypoint follower. While there are still entries left,
 * copies the current path[idx].x/.y into the entity's worldX/worldY
 * (+0x68/+0x6A) and bumps the counter. Once the counter hits the total
 * waypoint count, dispatches to the queued state via
 * EntityProcessCallbackQueue (which is what wakes up the entity's
 * pending FSM event, typically "arrived at destination").
 *
 * Match notes: cc1 re-reads e->nextWaypointIndex *three* times — once
 * for the bounds check / first index, once between the +0x68 and +0x6A
 * stores, and once for the increment-and-store. Writing it that way
 * in C is the only thing that lines up the lbu/sll/addu sequence. */
void EntityFollowPathTick(FollowPathEntity *e) {
    if (e->nextWaypointIndex >= e->waypointCount) {
        EntityProcessCallbackQueue((Entity *)e);
        return;
    }
    e->sprite.base.worldX = e->path[e->nextWaypointIndex * 2 + 0];
    e->sprite.base.worldY = e->path[e->nextWaypointIndex * 2 + 1];
    e->nextWaypointIndex = e->nextWaypointIndex + 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyWithEffects);

void EntityStopSound(BossVoice144Entity *e) {
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
}

/* CollectibleDestroyState — destruction state for a collectible boss.
 * The original entity (e1) carries a "linked sprite" at +0x134 (e2);
 * this re-enables e2's sprite render context, clears its +0x100 status
 * byte, restores its render flags, resets the collect-anim sequence
 * (loopFrame/spriteCallback/frameIndex), then destroys e1 with its
 * VFX, sets e1 to anim frame 8, and stamps +0x111 = 2 to mark the
 * post-destroy state for the next queue tick. */
void CollectibleDestroyState(Entity *e1) {
    SpriteEntity *e2 = *(SpriteEntity **)((u8 *)e1 + 0x134);

    ((SpriteRenderContextRef *)e2->base.spriteContext)->activeFlag = 1;
    ((u8 *)e2)[0x100] = 0;
    EntitySetRenderFlags((Entity *)e2, 1);
    SetAnimationLoopFrame((Entity *)e2, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback((Entity *)e2, ANIM_FINISHED_CB);
    SetAnimationFrameIndex((Entity *)e2, 0);
    EntityDestroyWithEffects(e1);
    ((u8 *)e1)[0x111] = 2;
    SetAnimationFrameIndex(e1, 8);
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggChaseState);

INCLUDE_ASM("asm/nonmatchings/bosses", BossState_SpawnDeathParticle);

INCLUDE_ASM("asm/nonmatchings/bosses", BossState_DefeatedWithParticles);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectiblePickupState);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleRiseState);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleFloatState);

void CollectibleIdleWaitState(u8 *e_raw) {
    Entity *entity = (Entity *)e_raw;
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    g_pGameState->direct_level_load = e_raw[0x140];
    ((SpriteRenderContextRef *)entity->spriteContext)->activeFlag = 0;
    SetAnimationActive(entity, 0);

    fn = EnemyTickWithCollision;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&entity->tickMarker = slot.s[0];

    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&entity->renderMarker = slot.s[0];

    e_raw[0x106] = 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", EntitySwapChildPointer);

INCLUDE_ASM("asm/nonmatchings/bosses", InitBonusItemEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", BossHPBarTickCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", InitGlennYntisBoss);

/* Entity layout for the boss-class destructor below — has an SPU voice
 * handle at +0x118 (vs +0x10C used elsewhere). */
typedef struct BossWithSpuVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x118 - 0x100];
    /* 0x118 */ s32 voiceHandle;
} BossWithSpuVoiceEntity;

/* Destructor referenced from boss vtables. Three-phase teardown:
 *   1. Swap collisionVtable to BOSS_SPU_STOP_PRE_VTABLE (D_800112C8) so the
 *      SPU callback (if any) sees the "stopping" vtable, then call
 *      StopSPUVoice with the handle at +0x118.
 *   2. Clear voiceHandle to -1, swap collisionVtable to
 *      BOSS_SPU_CLEANUP_VTABLE (D_80011388), then DestroyEntityAndFreeMemory.
 *   3. If flags bit 0 set, return the BLB-heap allocation.
 *
 * Both vtable writes target the same +0x18 field (collisionVtable); the
 * intermediate D_800112C8 swap is the boss-specific quirk that distinguishes
 * this from playdtor's single-vtable destructor.
 *
 * Match recipe: the lw of voiceHandle MUST be on the line BEFORE the
 * lui/addiu for D_800112C8 because cc1 schedules it into the StopSPUVoice
 * call's delay slot. Likewise the -1 store at +0x118 is folded into the
 * second vtable lui's delay slot. Writing the function as a flat sequence
 * (no temps) produces this. */
void EntityDestructor_WithSPUStop(BossWithSpuVoiceEntity *e, u32 flags) {
    e->sprite.base.collisionVtable = BOSS_SPU_STOP_PRE_VTABLE;
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
    e->sprite.base.collisionVtable = BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(&e->sprite, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

void Hazard_TickWithBehaviorTransition(Entity *e) {
    HazardTimerEntity *entity = (HazardTimerEntity *)e;
    CollectibleTickCallback(e);
    if (entity->behaviorTimer != 0) {
        entity->behaviorTimer -= 1;
        if (entity->behaviorTimer == 0) {
            EntitySetState(e, HAZARD_TIMER_EXPIRED_STATE_MARKER,
                           HAZARD_TIMER_EXPIRED_STATE_CALLBACK);
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDamageEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDeathEventHandler);

/* HazardActivateWithSound — entry state for a sound-emitting hazard.
 * Spawns a positional voice (id 0x7A318245), stashes its handle at +0x118
 * so HazardStopSound can release it later, primes the behaviour timer at
 * +0x114 to 0x12C (300 frames), installs the Glenn-Yntis event handler and
 * the Hazard tick (which decrements the timer and dispatches to
 * HAZARD_TIMER_EXPIRED_STATE), sets the active sprite (0x8068815C), and
 * queues the HAZARD_STOP_SOUND_STATE on the regular queued-state slot. */
void HazardActivateWithSound(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    *(s32 *)((u8 *)e + 0x118) = PlayEntityPositionSound(e, 0x7A318245);
    *(u16 *)((u8 *)e + 0x114) = 0x12C;
    do {} while (0);
    fn = (void (*)())GlennYntisEventHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = Hazard_TickWithBehaviorTransition;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, 0x8068815C, 1);
    EntitySetCallback(e, HAZARD_STOP_SOUND_STATE_MARKER,
                      HAZARD_STOP_SOUND_STATE_CALLBACK);
}

void HazardStopSound(BossVoiceEntity *e) {
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
}

void CollectibleAnimState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())GlennYntisDamageEventHandler;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, 0xC8F90114, 1);
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
    ((u8 *)e)[0x111] = 3;
    do {} while (0);
    fn = GlennYntisSetPhaseFromHP;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

void HazardIdleWithSound(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    *(s32 *)((u8 *)e + 0x118) = PlayEntityPositionSound(e, 0x62318245);
    do {} while (0);
    fn = (void (*)())GlennYntisAttackEventHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, 0x407801D0, 1);
    ((u8 *)e)[0x111] = 0;
    ((u8 *)e)[0x112] = 0;
    do {} while (0);
    fn = HazardActivateWithSound;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
    EntitySetCallback(e, HAZARD_IDLE_WITH_SOUND_STATE_MARKER,
                      HAZARD_IDLE_WITH_SOUND_STATE_CALLBACK);
}

void HazardStopSoundAlt(BossVoiceEntity *e) {
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
}

/* Switches GlennYntis into the active boss state: installs the attack
 * event handler on +0x08, CollectibleTickCallback on the tick slot, sets
 * the active sprite (0x407801DC), clears the attack-counter and phase
 * timer (+0x111/+0x112), then queues GlennYntisSetPhaseFromHP on the
 * queued-state slot. */
void CollectibleActiveState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())GlennYntisAttackEventHandler;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, 0x407801DC, 1);
    ((u8 *)e)[0x111] = 0;
    ((u8 *)e)[0x112] = 0;
    do {} while (0);
    fn = GlennYntisSetPhaseFromHP;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

/* Re-arms a boss-state countdown (+0x112 = idleTimeout) to (5 - boss_hp)
 * frames, picks a fresh idle animation via GlennYntisSelectRandomAnimState,
 * then installs HazardActivateWithSound on the queued-state slot
 * (sprite.queuedStateMarker @ +0x98). Used when a phase transition starts. */
void GlennYntisSetPhaseFromHP(ShrineyGuardEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    e->idleTimeout = 5 - PLAYER_STATE_DATA->boss_hp;
    GlennYntisSelectRandomAnimState((Entity *)e);
    fn = HazardActivateWithSound;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

void HazardSelectRandomBehavior(ShrineyGuardEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->idleTimeout = 0;
    GlennYntisSelectRandomAnimState((Entity *)e);
    fn = HazardActivateWithSound;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

/* GlennYntisSelectRandomAnimState — rolls rand() & 7 and dispatches to
 * one of three anim sub-states via EntitySetState (so the next-frame
 * processor consumes it). Distribution: r in [0,3) → AnimStateB,
 * r in [3,6) → IdleAnimState, r in [6,8) → AnimStateC. Then queues
 * HazardActivateWithSound on the queued-state slot to re-emit the
 * sound-effect on the following tick. */
void GlennYntisSelectRandomAnimState(SpriteEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    u32 r;
    u32 marker;
    EntityCallback cb;

    r = rand() & 7;
    if (r < 3) {
        marker = GLENN_YNTIS_ANIM_B_STATE_MARKER;
        cb = GLENN_YNTIS_ANIM_B_STATE_CALLBACK;
    } else if (r < 6) {
        marker = GLENN_YNTIS_ANIM_IDLE_STATE_MARKER;
        cb = GLENN_YNTIS_ANIM_IDLE_STATE_CALLBACK;
    } else {
        marker = GLENN_YNTIS_ANIM_C_STATE_MARKER;
        cb = GLENN_YNTIS_ANIM_C_STATE_CALLBACK;
    }
    EntitySetState((Entity *)e, marker, cb);
    fn = HazardActivateWithSound;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->queuedStateMarker = slot.s;
}

/* Glenn Yntis boss "idle anim" state: installs the attack event handler
 * on the event slot (+0x08) and CollectibleTickCallback on the tick slot
 * (+0x00), switches sprite to 0x2D688254 (boss idle frame), and clears
 * the +0x111 active-timer byte. Called by GlennYntisSelectRandomAnimState
 * when the idle animation is picked. */
void GlennYntisIdleAnimState(ShrineyGuardEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = GlennYntisAttackEventHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    SetEntitySpriteId((Entity *)e, SPR_GLENN_YNTIS_ANIM_A, 1);
    e->activeTimer = 0;
}

/* Sibling of GlennYntisIdleAnimState — same handler/tick install, just
 * uses sprite-id 0x2B79835D (alt boss frame B). */
void GlennYntisAnimStateB(ShrineyGuardEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = GlennYntisAttackEventHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    SetEntitySpriteId((Entity *)e, SPR_GLENN_YNTIS_ANIM_B, 1);
    e->activeTimer = 0;
}

/* Sibling of GlennYntisIdleAnimState — same handler/tick install, just
 * uses sprite-id 0x69588258 (alt boss frame C). */
void GlennYntisAnimStateC(ShrineyGuardEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = GlennYntisAttackEventHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    fn = CollectibleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    SetEntitySpriteId((Entity *)e, SPR_GLENN_YNTIS_ANIM_C, 1);
    e->activeTimer = 0;
}

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDefeatedState);

void GlennYntisVictoryCallback(u8 *e_raw) {
    Entity *entity = (Entity *)e_raw;
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    g_pGameState->direct_level_load = e_raw[0x110];
    ((SpriteRenderContextRef *)entity->spriteContext)->activeFlag = 0;
    SetAnimationActive(entity, 0);

    fn = CollectibleTickCallback;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&entity->tickMarker = slot.s[0];

    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&entity->renderMarker = slot.s[0];

    e_raw[0x106] = 1;
}

/* =========================================================================
 *  SHRINEY GUARD (a.k.a. "Shriney Guard" / boss of MEGA level 5)
 * =========================================================================
 *
 *  Entity type  : ENTITY_TYPE_101_BOSS_SHRINEY_GUARD (0x65)
 *  Vtable       : SHRINEY_GUARD_VTABLE  @ D_800112A8
 *  Aux alloc    : 0x2710 (10000) bytes from BLB heap (auxiliary sprite stack)
 *  Init sprite  : 0xA8482860  (DOWN_MOUNTLET — boss base sprite, 0xA2 x 0xF0)
 *  Sprite roster:
 *      0xA8482860  BASE       — initial sprite (set in InitEntitySprite)
 *      0x09382152  IDLE       — passive idle pose
 *      0x4C106054  WINDUP     — attack windup (single-attack path)
 *      0x40106054  LOOP_LINK  — first frame of the looping-attack chain
 *      0x2C182010  LOOP_START — looping attack stun-window stance
 *      0x08192250  READY      — facing the player just before a slam
 *      0x085860D4  SLAM       — the actual slam animation (movement-driven)
 *      0x0A1820D4  DEATH      — death pose (set in ShrineyGuardDeathState)
 *  Anim helpers:
 *      SetAnimationLoopFrame(ANIM_LOOP_DEFAULT) — slam-anim loop frame id
 *      SetAnimationLoopFrame(ANIM_SHRINEY_GUARD_LOOP_KEYFRAME) — looping-attack second-loop id
 *      SetAnimationSpriteCallback(ANIM_FINISHED_CB) — anim-finished callback hash
 *
 *  Health: PlayerState.boss_hp (PLAYER_STATE_DATA[+0x1D]) is set to 3 by
 *  InitShrineyGuardBoss. BossEventHandler decrements it on damage event 0x1002
 *  and dispatches into ShrineyGuardStartLoopAttackState while hp>0, or
 *  ShrineyGuardDeathState when hp reaches 0. Confirmed by runtime trace
 *  game_watcher/logs/struct_watch_20260621_110109.jsonl (boss_hp 3→2→1→0,
 *  three damage hits, then respawn on player death):
 *      f0405: spawn (boss_hp 0->3)
 *      f0592: hit 1 (shake=19, hp 3->2)
 *      f0805: hit 2 (shake=19, hp 2->1)
 *      f0929: player death  (shake=28)
 *      f1053: respawn (boss_hp 0->3)
 *      f1180,f1360,f1541: three damage hits → death (shake=19 throughout)
 *
 *  FSM markers (gp_rel pair: marker | callback):
 *      D_800A5BE8/EC  init-state         → BossRandomAttackChoice
 *      D_800A5BF0/F4  idle-timeout fires → ShrineyGuardAttackCounterState
 *      D_800A5BF8/FC  damage to death    → ShrineyGuardDeathState
 *      D_800A5C00/04  damage but alive   → ShrineyGuardStartLoopAttackState
 *      D_800A5C08/0C  slam decel == 0    → ShrineyGuardReadyAttackState
 *      D_800A5C10/14  repeat (<3 attacks)→ ShrineyGuardAttackAnimState
 *      D_800A5C18/1C  finish (3 attacks) → ShrineyGuardIdleState
 *  (markers are tag 0xFFFF0000 so EntitySetState matches against them.)
 *
 *  Per-entity scratch layout (overlay on SpriteEntity, see ShrineyGuardEntity
 *  typedef above):
 *      +0x10C  readyFlag   — 0=idle, 1=slam-armed (set by AttackEventHandler),
 *                            2=stun/death (set by StartLoop & DeathState)
 *      +0x110  destLevel   — level number written to GS.direct_level_load on
 *                            victory (read from EntitySpawnData[+0xC] in init)
 *      +0x111  activeTimer — ShrineyGuardActiveEventHandler decrement target
 *      +0x112  idleTimeout — frames until boss issues next attack (init 0xB4
 *                            = 180 frames; BossRandomAttackChoice resets 0x5A)
 *      +0x113  stunTimer   — StartLoopAttackState arms with 5; StunTick decays
 *      +0x114  attackCounter — number of consecutive slams (resets at 3)
 *      +0x115  stunActive  — 1 while ReadyAttack windup is playing
 *      +0x118  slamVelocity  (s32, 16.16 fixed)  — see MoveCallback below
 *      +0x11C  slamFrameCounter (u8)             — see MoveCallback below
 *
 *  Slam physics (ShrineyGuardMoveCallback):
 *      max velocity = 0x50000   (~5.0 world-units per frame)
 *      accel        = +0x8000   (~0.5/frame) for the first 25 (0x19) frames
 *      decel        = -0x1800   (~0.094/frame) after frame 25
 *      direction    = sign from entity facing byte (+0x74: 0=right, 1=left)
 *      total slam   ≈ 78 frames (~1.56 s at 50fps) before vel reaches 0
 *      on vel == 0  → EntitySetState(D_800A5C08, ShrineyGuardReadyAttackState)
 *
 *  Magic event ARG constants (consumed by BossEventHandler, event==0x0001):
 *      0x400B43A0  spawn directional projectile  (offset ±0x38 X from facing)
 *      0x18443181  set GS+0x11A = 0x14           (intro/cinematic trigger)
 *      0x46384180  clear GS.level_active +
 *                  CreateFadeOverlayEntity()     (level-end fade)
 *  Magic event ARG (consumed by ShrineyGuardAttackEventHandler, event==1):
 *      ANIM_LOOP_DEFAULT  player landed on head: install ShrineyGuardMoveCallback on
 *                  +0x1C/0x20 movement slot, set readyFlag=1, stunActive=0
 *
 *  Companion entities:
 *    - The shriney-styled sound-emitting CLAYBALL (entity type 0x5D) shares
 *      a name prefix but is implemented in clayball.c
 *      (InitBonusClayballEntity / ShrineyGuardSoundUpdateTick /
 *       ShrineyGuardEventWithSound / ShrineyGuardDestroyWithSoundCleanup).
 *      It is the audio-driven rolling variant the boss summons; it manages
 *      its own SPU voices and is NOT the boss entity itself.
 *
 *  Cross-references:
 *      docs/systems/boss-ai/boss-shriney-guard.md  (per-boss design notes)
 *      docs/systems/boss-entity-pattern.md         (shared boss layout)
 *      docs/systems/player/trace-findings.md       (runtime trace evidence)
 * ========================================================================= */

/* Boss-entity constructor invoked from EntityType101_Boss_ShrineyGuard_Init
 * in entinit.c. Allocates the 0x2710-byte auxiliary sprite stack from the
 * BLB heap, copies the player-state boss_hp byte (now 3) into aux[+0x100],
 * pointer-chases EntitySpawnData[+0xC] into main[+0x110] to remember the
 * destination level, installs SHRINEY_GUARD_VTABLE at +0x18, writes
 * PLAYER_STATE_DATA[+0x1D] = 3 (boss HP), then EntitySetState into the
 * D_800A5BE8 marker which dispatches BossRandomAttackChoice. Finally sets
 * the idle timeout (+0x112) to 0xB4 (180 frames) and arms +0x115. */
INCLUDE_ASM("asm/nonmatchings/bosses", InitShrineyGuardBoss);

/* Shared enemy per-tick driver used by the Shriney Guard, Glenn Yntis and
 * other "hit-on-bounce" enemies. Runs the entity's own update callback,
 * then — unless readyFlag (+0x10C) == 2 (stunned/death lockout) — invokes
 * the standard collision wrapper (CollisionCheckWrapper(2, 0x1000)) which
 * dispatches events 0x1001/0x1002 to whoever hits the boss. Finally
 * inspects the +0x106 death flag and runs the queued callback slot at
 * +0x98/0x9C if the entity has been marked dead. */
INCLUDE_ASM("asm/nonmatchings/bosses", EnemyUpdateWithCollisionAndDeath);

/* Per-tick callback installed by ShrineyGuardIdleState. Decrements the
 * idle-timeout byte and, when it hits 0, transitions through
 * D_800A5BF0 → ShrineyGuardAttackCounterState (i.e. boss has been idle
 * long enough; pick the next attack). Always also runs the shared
 * EnemyUpdateWithCollisionAndDeath tick so collision still fires. */
void ShrineyGuardIdleTickCallback(Entity *e) {
    ShrineyGuardEntity *entity = (ShrineyGuardEntity *)e;
    if (entity->idleTimeout != 0) {
        entity->idleTimeout -= 1;
        if (entity->idleTimeout == 0) {
            EntitySetState(e, SHRINEY_GUARD_IDLE_TIMEOUT_STATE_MARKER,
                           SHRINEY_GUARD_IDLE_TIMEOUT_STATE_CALLBACK);
        }
    }
    EnemyUpdateWithCollisionAndDeath(e);
}

/* Per-tick callback installed by ShrineyGuardStartLoopAttackState. Counts
 * the 5-frame stun window down; once it elapses the boss is re-armed by
 * clearing stunActive (+0x115) and re-asserting readyFlag (+0x10C) = 1, so
 * the next player-bounce hit will trigger AttackEventHandler again. */
void ShrineyGuardStunTickCallback(Entity *e) {
    ShrineyGuardEntity *entity = (ShrineyGuardEntity *)e;
    if (entity->stunTimer != 0) {
        entity->stunTimer -= 1;
        if (entity->stunTimer == 0) {
            entity->stunActive = 0;
            entity->readyFlag = 1;
        }
    }
    EnemyUpdateWithCollisionAndDeath(e);
}

/* Master event dispatcher for boss-class entities (shared by Shriney
 * Guard, Glenn Yntis, Joe-Head-Joe, Klogg). Handles:
 *   event 0x0001 (script triggers) — fires several arg2-keyed magic
 *     constants:
 *       0x400B43A0 → spawn a directional projectile via
 *                    InitDirectionalPositionEntity (X offset ±0x38 based
 *                    on facing +0x74).
 *       0x18443181 → write GS+0x11A = 0x14 (intro/cinematic flag).
 *       0x46384180 → clear GS.level_active + spawn a fade-overlay entity
 *                    (CreateFadeOverlayEntity), triggering the end-of-
 *                    level transition.
 *   event 0x1001 (player bounce land)  — sets readyFlag (+0x10C)=1 unless
 *     stunActive (+0x115) already set; dispatches the queued state slot.
 *   event 0x1002 (player attack hit)   — decrements PLAYER_STATE_DATA
 *     [+0x1D] (boss_hp). If hp>0 → EntitySetState(D_800A5C00,
 *     ShrineyGuardStartLoopAttackState). If hp==0 → EntitySetState
 *     (D_800A5BF8, ShrineyGuardDeathState).
 *   event 0x1008 (force-death/cleanup) — synchronous teardown path. */
INCLUDE_ASM("asm/nonmatchings/bosses", BossEventHandler);

/* Wrapper-event-handler installed by ShrineyGuardReadyAttackState /
 * StartLoopAttackState / DeathState. Forwards events to BossEventHandler
 * (so damage/spawn/level-end still works) but also services event 0x0002
 * which is the per-tick "anim heartbeat" — used to count down activeTimer
 * (+0x111) and call SetAnimationSpriteId(-1) to drop the current animation
 * once it reaches zero (effectively a finite-time sprite). When the timer
 * is already 0, advances the queued-state slot. */
s32 ShrineyGuardActiveEventHandler(ShrineyGuardEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = BossEventHandler((Entity *)e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        if (e->activeTimer != 0) {
            e->activeTimer -= 1;
            if (e->activeTimer == 0) {
                SetAnimationSpriteId((Entity *)e, -1);
            }
        } else {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    return result;
}

/* Event handler installed by ShrineyGuardAttackAnimState (the slam-anim).
 * On event 0x0001 with arg2 == ANIM_LOOP_DEFAULT (an anim-keyframe trigger fired
 * from inside the slam animation), it overwrites the entity's movement
 * slot (+0x1C/0x20) with ShrineyGuardMoveCallback, sets readyFlag
 * (+0x10C) = 1 and clears stunActive (+0x115). This is what kicks off
 * the horizontal slam: until this frame the boss was rooted; from this
 * frame on MoveCallback drives the velocity ramp. */
s32 ShrineyGuardAttackEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3) {
    ShrineyGuardEntity *se = (ShrineyGuardEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();
    s32 ret;
    u32 evt;

    evt = event & 0xFFFF;
    ret = BossEventHandler(e, evt, arg2, arg3);
    if (evt == 1 && arg2 == ANIM_LOOP_DEFAULT) {
        fn = ShrineyGuardMoveCallback;
        do {} while (0);
        m1 = -1;
        slot.s.markerLo = 0;
        slot.s.markerHi = m1;
        slot.s.fn = fn;
        *(CallbackSlot *)&se->sprite.base.renderMarker = slot.s;
        se->readyFlag = 1;
        se->stunActive = 0;
    }
    return ret;
}

/* Movement callback installed mid-animation by AttackEventHandler. Each
 * tick:
 *   - increment +0x11C (slamFrameCounter, u8) — gated against 0xFF
 *   - if slamFrameCounter < 0x19 (25): add 0x8000 to +0x118
 *     (slamVelocity, s32 16.16), clamp at 0x50000 (~5.0/frame)
 *   - else: subtract 0x1800 from slamVelocity; if it reaches 0 dispatch
 *     EntitySetState(D_800A5C08, ShrineyGuardReadyAttackState).
 *   - apply ±slamVelocity to (worldX hi:lo at +0x68:+0x6C) according to
 *     facing byte (+0x74) — 0 = move right, 1 = move left.
 * Net effect: a 25-frame acceleration ramp followed by a 53-frame decel,
 * ~78 frames total (≈1.56 s at 50 fps PAL). */
INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardMoveCallback);

/* State entered from D_800A5BE8 (init) and from the idle-timeout marker.
 * Resets idleTimeout to 0x5A (90 frames ≈ 1.8 s — the time the boss
 * "shows" its next attack), then 50/50 RNG picks between the single
 * attack-windup path (ShrineyGuardSetAttackState) and the looping
 * attack path (ShrineyGuardSetLoopingAttackState). */
void BossRandomAttackChoice(Entity *e) {
    ShrineyGuardEntity *entity = (ShrineyGuardEntity *)e;
    entity->idleTimeout = 0x5A;
    if ((rand() & 1) == 0) {
        ShrineyGuardSetAttackState(e);
    } else {
        ShrineyGuardSetLoopingAttackState(e);
    }
}

/* Reached when the idle-timeout marker fires (boss has been idle long
 * enough). Counts how many slams have already been queued in this idle
 * cycle (+0x114, attackCounter): if < 3 → loop back through
 * SHRINEY_GUARD_REPEAT_ATTACK_STATE (ShrineyGuardAttackAnimState);
 * otherwise reset counter and dispatch SHRINEY_GUARD_FINISH_ATTACK_STATE
 * (ShrineyGuardIdleState) so the boss returns to the idle pose. */
void ShrineyGuardAttackCounterState(Entity *e) {
    ShrineyGuardEntity *entity = (ShrineyGuardEntity *)e;
    u8 *counter = &entity->attackCounter;
    (*counter)++;
    if (*counter < 3) {
        EntitySetState(e, SHRINEY_GUARD_REPEAT_ATTACK_STATE_MARKER,
                       SHRINEY_GUARD_REPEAT_ATTACK_STATE_CALLBACK);
    } else {
        EntitySetState(e, SHRINEY_GUARD_FINISH_ATTACK_STATE_MARKER,
                       SHRINEY_GUARD_FINISH_ATTACK_STATE_CALLBACK);
        *counter = 0;
    }
}

/* Switches the boss into its single attack-windup state: installs
 * BossEventHandler on the +0x08 event slot and ShrineyGuardIdleTickCallback
 * on the +0x00 tick slot, then switches the sprite to 0x4C106054
 * (WINDUP — animation frame the player can bounce on to trigger the
 * slam via AttackEventHandler/ANIM_LOOP_DEFAULT). */
void ShrineyGuardSetAttackState(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())BossEventHandler;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = ShrineyGuardIdleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, SPR_SHRINEY_GUARD_WINDUP, 1);
}

/* Variant of ShrineyGuardSetAttackState used for the chained/looping
 * attack: same handler+tick install but the sprite-id is 0x40106054 (the
 * LOOP_LINK frame), and additionally calls SetAnimationLoopFrame
 * (ANIM_SHRINEY_GUARD_LOOP_KEYFRAME) with ((u8*)e)[0x111]=2 packed in the delay slot, plus
 * SetAnimationSpriteCallback(ANIM_FINISHED_CB) + SetAnimationFrameIndex(0)
 * to drive the looping-attack second-loop, and queues
 * ShrineyGuardSetAttackState on the +0x98 queued-state slot so each loop
 * iteration falls through to a single-attack windup. */
void ShrineyGuardSetLoopingAttackState(Entity *e) {
    TripadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())ShrineyGuardActiveEventHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = ShrineyGuardIdleTickCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, SPR_SHRINEY_GUARD_LOOP_LINK, 1);
    ((u8 *)e)[0x111] = 2;
    SetAnimationLoopFrame(e, ANIM_SHRINEY_GUARD_LOOP_KEYFRAME);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
    fn = ShrineyGuardSetAttackState;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
}

/* Passive idle state, dispatched from the FINISH_ATTACK marker (after the
 * boss has slammed 3 times). Installs ShrineyGuardActiveEventHandler on
 * +0x08, clears activeTimer (+0x111) = 0, installs
 * EnemyUpdateWithCollisionAndDeath on +0x00, sets sprite-id 0x09382152
 * (IDLE pose), and queues BossRandomAttackChoice on the +0x98 queued-state
 * slot so the next idle-timeout will pick a fresh attack pattern. */
void ShrineyGuardIdleState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())ShrineyGuardActiveEventHandler;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    ((u8 *)e)[0x111] = 0;
    do {} while (0);
    fn = EnemyUpdateWithCollisionAndDeath;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, SPR_SHRINEY_GUARD_IDLE, 1);
    fn = BossRandomAttackChoice;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

/* Slam-animation state, dispatched via the REPEAT_ATTACK marker (and
 * also re-entered each loop iteration). Installs ShrineyGuardAttack
 * EventHandler on +0x08 and EnemyUpdateWithCollisionAndDeath on +0x00,
 * sprite-id 0x085860D4 (SLAM frame), clears slamVelocity (+0x118) and
 * slamFrameCounter (+0x11C) so the upcoming AttackEventHandler/ANIM_LOOP_DEFAULT
 * trigger can start the ramp from zero. Sets the anim loop frame to
 * ANIM_LOOP_DEFAULT (so the keyframe fires) and the sprite callback to
 * ANIM_FINISHED_CB (the shared anim-finished hash). */
void ShrineyGuardAttackAnimState(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = (void (*)())ShrineyGuardAttackEventHandler;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    fn = EnemyUpdateWithCollisionAndDeath;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, SPR_SHRINEY_GUARD_SLAM, 1);
    *(u32 *)((u8 *)e + 0x118) = 0;
    ((u8 *)e)[0x11C] = 0;
    SetAnimationLoopFrame(e, ANIM_LOOP_DEFAULT);
    SetAnimationSpriteCallback(e, ANIM_FINISHED_CB);
    SetAnimationFrameIndex(e, 0);
}
/* Entered from the D_800A5C08 marker (set by MoveCallback when the slam
 * velocity decays to 0). Sets stunActive (+0x115) = 1, calls
 * SetEntityFacingDirection(e, 2) (face the player), clears readyFlag
 * (+0x10C) = 0, installs ShrineyGuardActiveEventHandler on +0x08, clears
 * activeTimer (+0x111) = 0, installs EnemyUpdateWithCollisionAndDeath on
 * +0x00, sprite-id 0x08192250 (READY pose), and queues
 * BossRandomAttackChoice on +0x98 so the windup completion advances the
 * FSM. This is the "stand up and re-aim" moment between slams. */
/* ShrineyGuardReadyAttackState — sets stunActive (+0x115) = 1, readyFlag
 * (+0x10C) = 0, calls SetEntityFacingDirection(e, 2), installs
 * ShrineyGuardActiveEventHandler on +0x08, clears activeTimer (+0x111),
 * installs EnemyUpdateWithCollisionAndDeath on +0x00, sprite 0x08192250,
 * clears render slot, queues BossRandomAttackChoice on +0x98. */
void ShrineyGuardReadyAttackState(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e)[0x115] = 1;
    ((u8 *)e)[0x10C] = 0;
    SetEntityFacingDirection(e, 2);
    fn = (void (*)())ShrineyGuardActiveEventHandler;
    m1 = -1;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    ((u8 *)e)[0x111] = 0;
    do {} while (0);
    fn = EnemyUpdateWithCollisionAndDeath;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    SetEntitySpriteId(e, SPR_SHRINEY_GUARD_READY, 1);
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->renderMarker = slot.s[0];
    fn = BossRandomAttackChoice;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s[0];
}

/* Entered from BossEventHandler/event 0x1002 (damage but hp > 0). Sets
 * stunTimer (+0x113) = 5, clears stunActive (+0x115), sets readyFlag
 * (+0x10C) = 2 (stun lockout — prevents EnemyUpdateWithCollisionAndDeath
 * from re-firing collision until the stun expires), installs
 * ShrineyGuardActiveEventHandler on +0x08, clears activeTimer (+0x111) = 0,
 * installs ShrineyGuardStunTickCallback on +0x00, sprite-id 0x2C182010
 * (LOOP_START / stun pose), and queues ShrineyGuardAttackCounterState
 * on +0x98 so after the 5-frame stun the boss decides whether to repeat
 * or finish its attack chain.
 *
 * SKIP: cc1 register allocator picks v1 for fn-ptr load (v0 busy with byte
 * write constants 5/2), produces same 57 instrs but different reg choice. */
INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardStartLoopAttackState);

/* Entered from BossEventHandler/event 0x1002 when boss_hp reaches 0.
 * Sprite-id 0x0A1820D4 (DEATH pose), writes g_pGameState[+0x19C] = 1 and
 * g_pGameState[+0x19D] = entity[+0x110] (the destination-level number
 * cached at init), sets readyFlag (+0x10C) = 2 (death lockout), installs
 * ShrineyGuardActiveEventHandler + EnemyUpdateWithCollisionAndDeath,
 * EntitySetRenderFlags(e, 0) to drop visibility, and queues
 * ShrineyGuardDeathCallback on +0x98 so the next queued-state dispatch
 * actually triggers the level transition. */
INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardDeathState);

/* Final death tick: copies the cached destination-level byte (+0x110,
 * set at init from EntitySpawnData[+0xC]) into GS.direct_level_load
 * (== 99 in vanilla MEGA → main menu / credits) and sets the entity-dead
 * flag (+0x106) = 1 so the entity-manager garbage-collects the boss next
 * tick. The DispatcherTickCallback running game-state then sees
 * direct_level_load != 0 and performs the cross-level transition. */
void ShrineyGuardDeathCallback(u8 *e) {
    g_pGameState->direct_level_load = e[0x110];
    e[0x106] = 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitJoeHeadJoeBoss);

void JoeHeadJoeBossDestructor(JoeHeadJoeEntity *entity, s32 flags) {
    entity->sprite.base.collisionVtable = &JOE_HEAD_JOE_BOSS_VTABLE;
    entity->voiceHandle = -1;
    entity->sprite.base.collisionVtable = &BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(&entity->sprite, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeUpdateWithCollisionCheck);

void JoeHeadJoe_CheckAttackAndUpdate(Entity *e) {
    JoeHeadJoeEntity *entity = (JoeHeadJoeEntity *)e;
    JoeHeadJoeCheckPlayerInAttackRange(e);
    if (entity->attackTimer != 0) {
        entity->attackTimer -= 1;
        if (entity->attackTimer == 0) {
            EntitySetState(e, JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_MARKER,
                           JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_CALLBACK);
        }
    }
    JoeHeadJoeUpdateWithCollisionCheck(e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeAttackEventHandler);

s32 JoeHeadJoeBounceEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = JoeHeadJoeAttackEventHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue(e);
    }
    return result;
}

s32 JoeHeadJoeDeathEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3) {
    u32 maskedEvent = event & 0xFFFF;
    if (maskedEvent == 1) goto event_one;
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue(e);
    }
    goto out;
event_one:
    if (arg2 == 0x46384180) {
        Entity *entity;
        g_pGameState->level_active = 0;
        entity = CreateFadeOverlayEntity((Entity *)AllocateFromHeap(g_pBlbHeapBase, 0x24, 1, 0));
        AddToZOrderList(g_pGameState, entity);
    }
out:
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSelectAttackPattern);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeCheckPlayerInAttackRange);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSetIdleState);

void JoeHeadJoeClearVoice(JoeHeadJoeEntity *e) {
    e->voiceHandle = -1;
}

/* JoeHeadJoeSetFacingAndAttack — faces the player (direction = 2), clears
 * the misc counter at +0x117, re-arms the standard tick + bounce-event
 * handlers, sets the attack sprite (0x8A3809F2), and queues
 * JoeHeadJoeSetIdleState on the queued-state slot so the boss returns to
 * idle once the attack callback finishes. The byte-clear at +0x117 lives
 * in the SetEntityFacingDirection delay slot. */
void JoeHeadJoeSetFacingAndAttack(JoeHeadJoeEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e)[0x117] = 0;
    SetEntityFacingDirection((Entity *)e, 2);
    fn = JoeHeadJoeUpdateWithCollisionCheck;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = (void (*)())JoeHeadJoeBounceEventHandler;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x8A3809F2, 1);
    fn = JoeHeadJoeSetIdleState;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

/* JoeHeadJoeEnterActiveState — install active-mode callbacks on a Joe-Head-
 * Joe boss. Resets the per-state misc counter at +0x117 and arms the
 * "active" flag at +0x116, installs the standard
 * JoeHeadJoeUpdateWithCollisionCheck tick + JoeHeadJoeBounceEventHandler
 * event handlers, sets the active sprite (0x1A3109B2), and queues the
 * JoeHeadJoeMoveAndCheckAttack callback on the queued-state slot. */
void JoeHeadJoeEnterActiveState(JoeHeadJoeEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e)[0x117] = 0;
    ((u8 *)e)[0x116] = 1;
    do {} while (0);
    fn = JoeHeadJoeUpdateWithCollisionCheck;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = (void (*)())JoeHeadJoeBounceEventHandler;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0x1A3109B2, 1);
    fn = JoeHeadJoeMoveAndCheckAttack;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeMoveAndCheckAttack);

void JoeHeadJoeClearVoiceAlt(JoeHeadJoeEntity *e) {
    e->voiceHandle = -1;
}

void JoeHeadJoeReturnToIdleState(JoeHeadJoeEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e)[0x117] = 0;
    do {} while (0);
    fn = JoeHeadJoeUpdateWithCollisionCheck;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = (void (*)())JoeHeadJoeBounceEventHandler;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0xB081DBB, 1);
    fn = JoeHeadJoeSetIdleState;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

void JoeHeadJoeReturnToIdleStateAlt(JoeHeadJoeEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    ((u8 *)e)[0x117] = 0;
    do {} while (0);
    fn = JoeHeadJoeUpdateWithCollisionCheck;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = (void (*)())JoeHeadJoeBounceEventHandler;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    SetEntitySpriteId((Entity *)e, 0xB0811BB, 1);
    fn = JoeHeadJoeSetIdleState;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeDeathAnimState);

void KloggDeathCallback(u8 *e) {
    Entity *entity = (Entity *)e;
    SpriteRenderContextRef *p = entity->spriteContext;
    p->activeFlag = 0;
    g_pGameState->direct_level_load = e[0x110];
    e[0x106] = 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggBoss);

void KloggDestroyCallback(KloggBossEntity *e, u32 flags) {
    e->sprite.base.collisionVtable = &KLOGG_BOSS_VTABLE;
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
    DestroyEntityAndFreeMemory(&e->sprite, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", SpawnParticleAndMenuEntity);

void KloggUpdateWithSound(Entity *e) {
    KloggBossEntity *entity = (KloggBossEntity *)e;
    UpdateEntitySoundPanning(e, entity->voiceHandle);
    if (entity->voicePanTimer != 0) {
        entity->voicePanTimer -= 1;
    }
    EntityUpdateCallback(e);
}

void KloggUpdateWithTimer(KloggBossEntity *e) {
    if (e->shortTimer != 0) {
        e->shortTimer -= 1;
        if (e->shortTimer == 0) {
            EntityProcessCallbackQueue((Entity *)e);
        }
    }
    UpdateEntitySoundPanning((Entity *)e, e->voiceHandle);
    if (e->voicePanTimer != 0) {
        e->voicePanTimer -= 1;
    }
    EntityUpdateCallback((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyHitMessageHandler);

s32 EntityCollision_EnemyHitWithCallback(Entity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = EnemyHitMessageHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue(e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/bosses", KloggDeathEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggMoveToTargetPosition);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggSpawnProjectilesCallback);

void KloggSetMoveState(Entity *e) {
    TripadSlot slot;
    s16 m1;
    void (*fn)();

    do {} while (0);
    fn = KloggMoveToTargetPosition;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->renderMarker = slot.s;
    fn = KloggUpdateWithSound;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    fn = (void (*)())EnemyHitMessageHandler;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0x193CA112, 1);
    fn = KloggSpawnProjectilesCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s;
}

void EnemyIdleTimerState(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    ((KloggBossEntity *)e)->shortTimer = 0xB4;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->renderMarker = slot.s[0];
    fn = (void (*)())KloggUpdateWithTimer;
    m1 = -1;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = (void (*)())EnemyHitMessageHandler;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    SetEntitySpriteId(e, 0x193CA112, 1);
    fn = KloggSetMoveState;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s[0];
}

void EnemySpriteState(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();

    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->renderMarker = slot.s[0];
    fn = KloggUpdateWithSound;
    m1 = -1;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    fn = (void (*)())EntityCollision_EnemyHitWithCallback;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    SetEntitySpriteId(e, 0x08BC8013, 1);
    fn = KloggSetMoveState;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&((SpriteEntity *)e)->queuedStateMarker = slot.s[0];
}

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyDefeatState);

void MonkeyMageDeathCallback(u8 *e) {
    Entity *entity = (Entity *)e;
    SpriteRenderContextRef *p = entity->spriteContext;
    p->activeFlag = 0;
    g_pGameState->direct_level_load = e[0x106];
}

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageDestroyCallback);

void MonkeyMagePartDestroyCallback(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void MonkeyMagePlatformDestroyCallback(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &BOSS_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void MonkeyMageForceFieldDestroyCallback(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &BOSS_AUX_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void MonkeyMageAuxDestroyCallback(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &MONKEY_MAGE_AUX_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void MonkeyMageBossPartDestroyCallback(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &BOSS_PARTICLE_SPAWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void MonkeyMageHUDDestroyCallback(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = &BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void func_8004F024(void) {
}

void func_8004F02C(void) {
}

void FreeEntityAllocOnly(Entity *e, u32 size);

void MonkeyMageSimpleDestroyCallback(Entity *entity, u32 flag) {
    entity->collisionVtable = &MONKEY_MAGE_SIMPLE_ALLOC_VTABLE;
    if (flag & 1) {
        FreeEntityAllocOnly(entity, 0x1C);
    }
}

void FreeEntityAllocOnly(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
}

extern u8 D_8009C09C[];
extern u8 D_8009C11C[];

s16 func_8004F098(s32 arg0) {
    char a;
    s32 sign;
    u32 t;
    u32 new_var;
    s32 r;
    a = arg0;
    sign = 0;
    if ((u32)(a & 0xFF) >= 0x80) {
        a = arg0 - 0x80;
        sign = 1;
    }
    t = a & 0xFF;
    if (t == 0x40) {
        r = 0x100;
    } else if (t < 0x40) {
        r = D_8009C09C[t];
    } else {
        r = *(D_8009C11C - (new_var = t));
    }
    if ((sign & 0xFF) != 0) {
        r = -r;
    }
    return (s16)r;
}

s16 func_8004F114(s32 arg0) {
    char a;
    s32 sign;
    u32 t;
    u32 new_var;
    s32 r;
    a = arg0 + 0x40;
    sign = 0;
    if ((u32)(a & 0xFF) >= 0x80) {
        a = arg0 - 0x40;
        sign = 1;
    }
    t = a & 0xFF;
    if (t == 0x40) {
        r = 0x100;
    } else if (t < 0x40) {
        r = D_8009C09C[t];
    } else {
        r = *(D_8009C11C - (new_var = t));
    }
    if ((sign & 0xFF) != 0) {
        r = -r;
    }
    return (s16)r;
}

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F190);

INCLUDE_ASM("asm/nonmatchings/bosses", MaskAngleCosineEntry);

