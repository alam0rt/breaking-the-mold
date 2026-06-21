#include "common.h"
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
extern s32 EnemyHitMessageHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void GlennYntisSelectRandomAnimState(Entity *e);
extern void HazardActivateWithSound(Entity *e);
extern void GlennYntisAttackEventHandler(Entity *e);
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

INCLUDE_ASM("asm/nonmatchings/bosses", GliderStateInit_WakeUp);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderSetActiveState);

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

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleDestroyState);

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggChaseState);

INCLUDE_ASM("asm/nonmatchings/bosses", BossState_SpawnDeathParticle);

INCLUDE_ASM("asm/nonmatchings/bosses", BossState_DefeatedWithParticles);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectiblePickupState);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleRiseState);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleFloatState);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleIdleWaitState);

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

INCLUDE_ASM("asm/nonmatchings/bosses", HazardActivateWithSound);

void HazardStopSound(BossVoiceEntity *e) {
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardIdleWithSound);

void HazardStopSoundAlt(BossVoiceEntity *e) {
    StopSPUVoice(e->voiceHandle);
    e->voiceHandle = -1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleActiveState);

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

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisSelectRandomAnimState);

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
    SetEntitySpriteId((Entity *)e, 0x2D688254, 1);
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
    SetEntitySpriteId((Entity *)e, 0x2B79835D, 1);
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
    SetEntitySpriteId((Entity *)e, 0x69588258, 1);
    e->activeTimer = 0;
}

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDefeatedState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisVictoryCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", InitShrineyGuardBoss);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyUpdateWithCollisionAndDeath);

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

INCLUDE_ASM("asm/nonmatchings/bosses", BossEventHandler);

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

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardMoveCallback);

void BossRandomAttackChoice(Entity *e) {
    ShrineyGuardEntity *entity = (ShrineyGuardEntity *)e;
    entity->idleTimeout = 0x5A;
    if ((rand() & 1) == 0) {
        ShrineyGuardSetAttackState(e);
    } else {
        ShrineyGuardSetLoopingAttackState(e);
    }
}

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

/* Switches the boss into its attack-windup state: installs BossEventHandler
 * on the +0x08 event slot and ShrineyGuardIdleTickCallback on the +0x00 tick
 * slot, then switches the sprite to 0x4C106054 (attack-windup animation). */
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
    SetEntitySpriteId(e, 0x4C106054, 1);
}

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardSetLoopingAttackState);

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
    SetEntitySpriteId(e, 0x9382152, 1);
    fn = BossRandomAttackChoice;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardAttackAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardReadyAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardStartLoopAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardDeathState);

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

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSetFacingAndAttack);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeEnterActiveState);

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

INCLUDE_ASM("asm/nonmatchings/bosses", KloggSetMoveState);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyIdleTimerState);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemySpriteState);

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

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F098);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F114);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F190);

INCLUDE_ASM("asm/nonmatchings/bosses", MaskAngleCosineEntry);

