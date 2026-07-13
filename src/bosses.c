#include "common.h"
#include "Game/asset_ids.h"
#include "functions.h"
#include "Game/callback_slot.h"
#include "Game/fsm_dispatch.h"
#include "Game/boss_entities.h"
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
extern s32 KloggDeathEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void MonkeyMageDeathCallback(u8 *e);
void KloggDeathCallback(u8 *e);
void GlennYntisSetPhaseFromHP();
extern s32 EnemyHitMessageHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void GlennYntisSelectRandomAnimState(SpriteEntity *e);
extern void HazardActivateWithSound(Entity *e);
extern s32 GlennYntisAttackEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntitySetRenderFlags(Entity *e, u32 flags);
extern void EntityDestroyWithEffects(Entity *e);
extern void CollectibleTickCallback(Entity *e);
extern void UpdateEntitySoundPanning(Entity *e, u32 sound);
extern void SetAnimationTargetFrameIndex(Entity *e, s32 id);
extern Entity *CreateFadeOverlayEntity(Entity *e);
extern void AddToZOrderList(GameState *gs, Entity *entity);
extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");

/* gp_rel sdata: strong initialized defs live in the address-ordered block at the
 * end of this file (sdata-under-split Phase 4). These are forward declarations. */
extern u32   GLIDER_WAKE_STATE_MARKER asm("D_800A5B68");
extern EntityCallback GLIDER_WAKE_STATE_CALLBACK asm("D_800A5B6C");
extern u32   HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B98");
extern EntityCallback HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B9C");
extern u32   HAZARD_STOP_SOUND_STATE_MARKER asm("D_800A5BC0");
extern EntityCallback HAZARD_STOP_SOUND_STATE_CALLBACK asm("D_800A5BC4");
extern u32   HAZARD_IDLE_WITH_SOUND_STATE_MARKER asm("D_800A5BC8");
extern EntityCallback HAZARD_IDLE_WITH_SOUND_STATE_CALLBACK asm("D_800A5BCC");
extern u32   GLENN_YNTIS_ANIM_B_STATE_MARKER asm("D_800A5BD0");
extern EntityCallback GLENN_YNTIS_ANIM_B_STATE_CALLBACK asm("D_800A5BD4");
extern u32   GLENN_YNTIS_ANIM_IDLE_STATE_MARKER asm("D_800A5BD8");
extern EntityCallback GLENN_YNTIS_ANIM_IDLE_STATE_CALLBACK asm("D_800A5BDC");
extern u32   GLENN_YNTIS_ANIM_C_STATE_MARKER asm("D_800A5BE0");
extern EntityCallback GLENN_YNTIS_ANIM_C_STATE_CALLBACK asm("D_800A5BE4");
extern u32   SHRINEY_GUARD_IDLE_TIMEOUT_STATE_MARKER asm("D_800A5BF0");
extern EntityCallback SHRINEY_GUARD_IDLE_TIMEOUT_STATE_CALLBACK asm("D_800A5BF4");
extern u32   SHRINEY_GUARD_REPEAT_ATTACK_STATE_MARKER asm("D_800A5C10");
extern EntityCallback SHRINEY_GUARD_REPEAT_ATTACK_STATE_CALLBACK asm("D_800A5C14");
extern u32   SHRINEY_GUARD_FINISH_ATTACK_STATE_MARKER asm("D_800A5C18");
extern EntityCallback SHRINEY_GUARD_FINISH_ATTACK_STATE_CALLBACK asm("D_800A5C1C");
extern u32   JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_MARKER asm("D_800A5C20");
extern EntityCallback JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_CALLBACK asm("D_800A5C24");

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggBossEntity);

/* Standard entity tick + ready-flag check: when +0x101 is set, notify the
 * GameState event FSM (EVT_GAME_NOTIFY, arg 0, srcEntity=e). S-reg
 * dispatch family (slot pair $s0/$s1, fn $t0, self $s2). */
void EntityTickWithReadyCheck(Entity *e) {
    GS_NOTIFY_DECLS_S(Entity *);

    self = e;
    EntityUpdateCallback(self);
    if (*((u8 *)self + 0x101) != 0) {
        GS_NOTIFY_DISPATCH(0, self);
    }
}

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
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
}

void GliderSetActiveState(Entity *e) {
    SpriteEntity *se = (SpriteEntity *)e;
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
    *(CallbackSlot *)&se->queuedStateMarker = slot.s;
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
void EntityDestructor_WithSPUVoiceStopAndClear(BossWithSpuVoiceEntity *e, u32 flags) {
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
    GlennYntisHazardEntity *entity = (GlennYntisHazardEntity *)e;
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
    GlennYntisHazardEntity *hz = (GlennYntisHazardEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    hz->voiceHandle = PlayEntityPositionSound(e, FX_BOSS_YNT_IDLE_01);
    hz->behaviorTimer = 0x12C;
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
    SetEntitySpriteId(e, SPR_YNT_IDLE_1, 1);
    EntitySetCallback(e, HAZARD_STOP_SOUND_STATE_MARKER,
                      HAZARD_STOP_SOUND_STATE_CALLBACK);
}

void HazardStopSound(GlennYntisHazardEntity *e) {
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
    GlennYntisHazardEntity *hz = (GlennYntisHazardEntity *)e;
    PadSlot slot;
    s16 m1;
    void (*fn)();

    hz->voiceHandle = PlayEntityPositionSound(e, FX_BOSS_YNT_IDLE_02);
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
    SetEntitySpriteId(e, SPR_YNT_HIT_1, 1);
    ((u8 *)e)[0x111] = 0;
    ((u8 *)e)[0x112] = 0;
    do {} while (0);
    fn = HazardActivateWithSound;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
    EntitySetCallback(e, HAZARD_IDLE_WITH_SOUND_STATE_MARKER,
                      HAZARD_IDLE_WITH_SOUND_STATE_CALLBACK);
}

void HazardStopSoundAlt(GlennYntisHazardEntity *e) {
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
    SetEntitySpriteId(e, SPR_YNT_HIT_2, 1);
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
    GlennYntisSelectRandomAnimState((SpriteEntity *)e);
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
    GlennYntisSelectRandomAnimState((SpriteEntity *)e);
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
    fn = (void (*)())GlennYntisAttackEventHandler;
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
    fn = (void (*)())GlennYntisAttackEventHandler;
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
    fn = (void (*)())GlennYntisAttackEventHandler;
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
 *  and dispatches into ShrineyGuardStartLoopingAttackState while hp>0, or
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
 *      D_800A5C00/04  damage but alive   → ShrineyGuardStartLoopingAttackState
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

/* Per-tick callback installed by ShrineyGuardStartLoopingAttackState. Counts
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
 *     ShrineyGuardStartLoopingAttackState). If hp==0 → EntitySetState
 *     (D_800A5BF8, ShrineyGuardDeathState).
 *   event 0x1008 (force-death/cleanup) — synchronous teardown path. */
INCLUDE_ASM("asm/nonmatchings/bosses", BossEventHandler);

/* Wrapper-event-handler installed by ShrineyGuardReadyAttackState /
 * StartLoopAttackState / DeathState. Forwards events to BossEventHandler
 * (so damage/spawn/level-end still works) but also services event 0x0002
 * which is the per-tick "anim heartbeat" — used to count down activeTimer
 * (+0x111) and call SetAnimationTargetFrameIndex(-1) to drop the current animation
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
                SetAnimationTargetFrameIndex((Entity *)e, -1);
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
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
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
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s[0]; }
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
INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardStartLoopingAttackState);

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
    SetEntitySpriteId(e, SPR_KLOGG_IDLE_1, 1);
    fn = KloggSpawnProjectilesCallback;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s; }
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
    SetEntitySpriteId(e, SPR_KLOGG_IDLE_1, 1);
    fn = KloggSetMoveState;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s[0]; }
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
    SetEntitySpriteId(e, SPR_KLOGG_HIT, 1);
    fn = KloggSetMoveState;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    { SpriteEntity *se = (SpriteEntity *)e; *(CallbackSlot *)&se->queuedStateMarker = slot.s[0]; }
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

INCLUDE_ASM("asm/nonmatchings/bosses", ScaleByCosine);

INCLUDE_ASM("asm/nonmatchings/bosses", MaskAngleMagnitudeEntry);

INCLUDE_ASM("asm/nonmatchings/bosses", CalculateVectorMagnitude);

INCLUDE_ASM("asm/nonmatchings/bosses", InitScaledEntityWithSprite);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileUpdateWithCleanup);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityMoveHorizontalByFacing);

INCLUDE_ASM("asm/nonmatchings/bosses", InitEnemyEntityWithAI);

INCLUDE_ASM("asm/nonmatchings/bosses", GenericEntityDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyAIUpdateWithRandomization);

s32 EntityEvent_ClearTargetOnDestroy(u8 *e, s32 eventId, s32 arg, s32 source) {
    if ((eventId & 0xFFFF) == 0x1009) {
        if (source == *(s32 *)(e + 0x108)) {
            *(s32 *)(e + 0x108) = 0;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/bosses", EntityEventHandler_TokenReleaseAndQueueTick);

INCLUDE_ASM("asm/nonmatchings/bosses", HomingProjectile_TrackTarget);

INCLUDE_ASM("asm/nonmatchings/bosses", FindNearestTargetEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyStateInit_SetSpriteAndHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyState_TurnAroundWithToken);

INCLUDE_ASM("asm/nonmatchings/bosses", InitEntity_ScaledSprite1);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyTickWithCollisionAndOffscreen);

void MoveEntityForward5px(u8 *e) {
    s16 x;
    if (e[0x74] != 0) {
        x = *(u16 *)(e + 0x68) - 5;
    } else {
        x = *(u16 *)(e + 0x68) + 5;
    }
    *(s16 *)(e + 0x68) = x;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitProjectileWithTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTick_CollisionWithTimerTransition);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityCollision_ProcessQueueOnly);

INCLUDE_ASM("asm/nonmatchings/bosses", HomingProjectileTick);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileEnterActiveState);

void ProjectileDeactivate(u8 *e) {
    u8 *sc = *(u8 **)(e + 0x34);
    sc[0xA] = 0;
    e[0xF1] = 0;
    e[0x108] = 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitEntity_ScaledAnimated1);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileEntityTick);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileMoveHorizontal);

INCLUDE_ASM("asm/nonmatchings/bosses", InitWalkingEnemyEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTick_WithParticleSpawn);

INCLUDE_ASM("asm/nonmatchings/bosses", HomingMissileTrackTarget);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileState_HomingActive);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileState_HomingActiveVariant2);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileState_HomingActiveVariant3);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileHomingTickState);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileState_HomingMissileTrack);

INCLUDE_ASM("asm/nonmatchings/bosses", InitHomingProjectileEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickWithAutoDestroy);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileApplyVelocity);

INCLUDE_ASM("asm/nonmatchings/bosses", InitAuraEffectAtPlayer);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileTickWithLifetime);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileZOrderCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileKeyframeEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", HudFollowPlayerPosition);

INCLUDE_ASM("asm/nonmatchings/bosses", InitSimpleParameterEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", ProjectileTickWithCollision);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBallFallCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", InitAnimatedDirectionalEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", PlayerDeathGrowingTick);

INCLUDE_ASM("asm/nonmatchings/bosses", DelayedDeathTimerTick);

INCLUDE_ASM("asm/nonmatchings/bosses", DeathAnimationTick);

INCLUDE_ASM("asm/nonmatchings/bosses", InitDirectionalPositionEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickCollisionNotify);

INCLUDE_ASM("asm/nonmatchings/bosses", BouncingProjectileTick);

INCLUDE_ASM("asm/nonmatchings/bosses", InitJoeHeadJoeBallRegular);

INCLUDE_ASM("asm/nonmatchings/bosses", DestroySoundEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityCollisionStateChange);

INCLUDE_ASM("asm/nonmatchings/bosses", FallingSoundEntityTick);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBallRegularInitState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBallStartRolling);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBallStopSound);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeState_EnterCollisionState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeState_HideAndNotifyGameState);

INCLUDE_ASM("asm/nonmatchings/bosses", InitJoeHeadJoeBallSpiky);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeTickWithSoundPanning);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeUpdatePosition);

INCLUDE_ASM("asm/nonmatchings/bosses", InitJoeHeadJoeBallSpecial);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeTickWithCollision);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeEventHandler2);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeEventWithStateTransition);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeState_CollisionTick);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeState_IdleAfterCollision);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeClearChildFlag);

INCLUDE_ASM("asm/nonmatchings/bosses", InitConditionalStateEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeTickWithOffscreenCheck);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeApplyGravity);

INCLUDE_ASM("asm/nonmatchings/bosses", InitNegativeVelocityEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeTickWithPlayerMessage);

void JoeHeadJoeApplyGravitySlow(u8 *e) {
    s32 pos;
    s32 vel;
    pos = ((s32)*(s16 *)(e + 0x6A) << 16) + *(u16 *)(e + 0x6E);
    vel = *(s32 *)(e + 0x100) + 0x1000;
    pos += vel;
    *(s32 *)(e + 0x100) = vel;
    *(s16 *)(e + 0x6A) = pos >> 16;
    *(s16 *)(e + 0x6E) = pos;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitEntity_ScaledSprite2);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeTickWithTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeApplyPhysicsWithBounce);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800113C8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800113E8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011408);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011428);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011488);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800114A8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800114C8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800114E8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011508);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800115E8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011548);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011568);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011588);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800115C8);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt800115E8_800556b8);

INCLUDE_ASM("asm/nonmatchings/bosses", NopStub_8005571c);

INCLUDE_ASM("asm/nonmatchings/bosses", NopStub_80055724);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyCallback_Vt80011608);

INCLUDE_ASM("asm/nonmatchings/bosses", FreeEntityNoTeardown_80055760);

INCLUDE_ASM("asm/nonmatchings/bosses", CalculatePathDistance);

INCLUDE_ASM("asm/nonmatchings/bosses", UpdateEntityAlongPath);

INCLUDE_ASM("asm/nonmatchings/bosses", InterpolateTimedPathPosition);

INCLUDE_ASM("asm/nonmatchings/bosses", InitGenericSpriteEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", InitClayballProjectile);

INCLUDE_ASM("asm/nonmatchings/bosses", GenericSpriteEntityInitCallback);

extern s32 CheckTriggerZoneCollision(GameState *gs, s16 x, s16 y, s32 *outAttr, s32 *outData);
extern s32 HandleGenericTriggerZone(s32 unused, u8 zoneId, u8 *outR, u8 *outG, u8 *outB);

/* If the entity has a spawn record (+0x100), test its (x,y) @ +0x8/+0xA
 * against the level's trigger zones; on a hit whose attribute resolves to an
 * RGB tint, stamp the r/g/b into the render context (+0x34) at +0x34..+0x36. */
void EntityCheckTriggerZone(u8 *e) {
    u8 *spawnRec = *(u8 **)(e + 0x100);
    s32 attr, data;
    u8 r, g, b;
    u8 *ctx;

    if (spawnRec == NULL) {
        return;
    }
    if ((CheckTriggerZoneCollision(g_pGameState, *(s16 *)(spawnRec + 0x8),
                                   *(s16 *)(spawnRec + 0xA), &attr, &data) & 0xFF) == 0) {
        return;
    }
    if ((HandleGenericTriggerZone((s32)g_pGameState, (u8)attr, &r, &g, &b) & 0xFF) == 0) {
        return;
    }
    ctx = *(u8 **)(e + 0x34);
    {
        /* copy to non-addressable temps so all three loads hoist above the
         * stores (r/g/b are address-taken and would otherwise alias ctx) */
        u8 rr = r, gg = g, bb = b;
        ctx[0x34] = rr;
        ctx[0x35] = gg;
        ctx[0x36] = bb;
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", GenericSpriteEntityTickCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityCollision_SpawnSwitchBlock);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTick_InterpolateTimedPath);

INCLUDE_ASM("asm/nonmatchings/bosses", InitClockPlatformWithTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestructor_WithChildEntityCleanup);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballIndicatorWaitTick);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballIndicatorExpandTick);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballIndicatorTimerTick);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballIndicatorEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballState_HideIndicatorAndWait);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballState_DestroyWithDebris);

INCLUDE_ASM("asm/nonmatchings/bosses", ClayballIndicatorState_Wait);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardDeactivateWithSound);

INCLUDE_ASM("asm/nonmatchings/bosses", UpdateChildEntityOffset);

INCLUDE_ASM("asm/nonmatchings/bosses", InitClayballOnPath);

INCLUDE_ASM("asm/nonmatchings/bosses", PathFollowerEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", PathFollowerUpdatePosition);

INCLUDE_ASM("asm/nonmatchings/bosses", PathFollowerUpdateWithSpeedDown);

INCLUDE_ASM("asm/nonmatchings/bosses", PathFollowerUpdateWithSpeedUp);

INCLUDE_ASM("asm/nonmatchings/bosses", InitClayballAtWaypoint);

INCLUDE_ASM("asm/nonmatchings/bosses", PathFollowerTickWithCollision);

INCLUDE_ASM("asm/nonmatchings/bosses", PathFollowerTickWithOffscreenCheck);

void PathFollowerApplyFallVelocity(u8 *e) {
    *(u16 *)(e + 0x6A) = *(u16 *)(e + 0x6A) + *(u16 *)(e + 0x128);
    *(u16 *)(e + 0x128) = *(u16 *)(e + 0x128) + 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardEventHandler);

/* bosses .sdata (0x800A5B60..0x800A5D10): {marker=0xFFFF0000, callback} descriptor
 * pairs for the glider / hazard / GlennYntis / ShrineyGuard / JoeHeadJoe / projectile
 * / clayball boss state machines, with three "END2" (0x32444E45) sentinel+pad
 * terminators embedded mid-table. Migrated from the pooled asm sdata blob
 * (sdata-under-split Phase 4). Address order == declaration order (cc1 2.7.2 emits
 * initialized .sdata in decl order). Eleven pairs carry friendly names
 * (forward-declared near the top of the file) used by matched code; the rest use
 * D_ names. Callbacks defined elsewhere are extern-declared; all cast to EntityCallback. */
extern void GliderStateInit_ResetAtSpawn();
extern void EntityDestroyWithEffects();
extern void BossState_DefeatedWithParticles();
extern void BossState_SpawnDeathParticle();
extern void GlennYntisDefeatedState();
extern void ShrineyGuardDeathState();
extern void ShrineyGuardStartLoopingAttackState();
extern void JoeHeadJoeSelectAttackPattern();
extern void JoeHeadJoeDeathAnimState();
extern void EnemyDefeatState();
extern void EnemyStateInit_SetSpriteAndHandler();
extern void EnemyState_TurnAroundWithToken();
extern void ProjectileEnterActiveState();
extern void ProjectileState_HomingActive();
extern void ProjectileState_HomingActiveVariant2();
extern void ProjectileState_HomingActiveVariant3();
extern void ProjectileState_HomingMissileTrack();
extern void JoeHeadJoeBallRegularInitState();
extern void JoeHeadJoeState_HideAndNotifyGameState();
extern void JoeHeadJoeState_EnterCollisionState();
extern void JoeHeadJoeBallStopSound();
extern void JoeHeadJoeState_CollisionTick();
extern void JoeHeadJoeState_IdleAfterCollision();
extern void ClayballIndicatorState_Wait();
extern void ClayballState_DestroyWithDebris();
extern void ShrineyGuardDeactivateWithSound();
extern void ClayballState_HideIndicatorAndWait();
u32 D_800A5B60 asm("D_800A5B60") = 0xFFFF0000;
EntityCallback D_800A5B64 asm("D_800A5B64") = (EntityCallback)GliderStateInit_WakeUp;
u32 GLIDER_WAKE_STATE_MARKER asm("D_800A5B68") = 0xFFFF0000;
EntityCallback GLIDER_WAKE_STATE_CALLBACK asm("D_800A5B6C") = (EntityCallback)GliderStateInit_ResetAtSpawn;
u32 D_800A5B70 asm("D_800A5B70") = 0xFFFF0000;
EntityCallback D_800A5B74 asm("D_800A5B74") = (EntityCallback)EntityDestroyWithEffects;
u32 D_800A5B78 asm("D_800A5B78") = 0xFFFF0000;
EntityCallback D_800A5B7C asm("D_800A5B7C") = (EntityCallback)BossState_DefeatedWithParticles;
u32 D_800A5B80 asm("D_800A5B80") = 0xFFFF0000;
EntityCallback D_800A5B84 asm("D_800A5B84") = (EntityCallback)BossState_SpawnDeathParticle;
u32 D_800A5B88 asm("D_800A5B88") = 0xFFFF0000;
EntityCallback D_800A5B8C asm("D_800A5B8C") = (EntityCallback)EntityStopSound;
u32 D_800A5B90 asm("D_800A5B90") = 0xFFFF0000;
EntityCallback D_800A5B94 asm("D_800A5B94") = (EntityCallback)HazardActivateWithSound;
u32 HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B98") = 0xFFFF0000;
EntityCallback HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B9C") = (EntityCallback)HazardSelectRandomBehavior;
u32 D_800A5BA0 asm("D_800A5BA0") = 0xFFFF0000;
EntityCallback D_800A5BA4 asm("D_800A5BA4") = (EntityCallback)CollectibleAnimState;
u32 D_800A5BA8 asm("D_800A5BA8") = 0xFFFF0000;
EntityCallback D_800A5BAC asm("D_800A5BAC") = (EntityCallback)HazardIdleWithSound;
u32 D_800A5BB0 asm("D_800A5BB0") = 0xFFFF0000;
EntityCallback D_800A5BB4 asm("D_800A5BB4") = (EntityCallback)GlennYntisDefeatedState;
u32 D_800A5BB8 asm("D_800A5BB8") = 0xFFFF0000;
EntityCallback D_800A5BBC asm("D_800A5BBC") = (EntityCallback)CollectibleActiveState;
u32 HAZARD_STOP_SOUND_STATE_MARKER asm("D_800A5BC0") = 0xFFFF0000;
EntityCallback HAZARD_STOP_SOUND_STATE_CALLBACK asm("D_800A5BC4") = (EntityCallback)HazardStopSound;
u32 HAZARD_IDLE_WITH_SOUND_STATE_MARKER asm("D_800A5BC8") = 0xFFFF0000;
EntityCallback HAZARD_IDLE_WITH_SOUND_STATE_CALLBACK asm("D_800A5BCC") = (EntityCallback)HazardStopSoundAlt;
u32 GLENN_YNTIS_ANIM_B_STATE_MARKER asm("D_800A5BD0") = 0xFFFF0000;
EntityCallback GLENN_YNTIS_ANIM_B_STATE_CALLBACK asm("D_800A5BD4") = (EntityCallback)GlennYntisAnimStateB;
u32 GLENN_YNTIS_ANIM_IDLE_STATE_MARKER asm("D_800A5BD8") = 0xFFFF0000;
EntityCallback GLENN_YNTIS_ANIM_IDLE_STATE_CALLBACK asm("D_800A5BDC") = (EntityCallback)GlennYntisIdleAnimState;
u32 GLENN_YNTIS_ANIM_C_STATE_MARKER asm("D_800A5BE0") = 0xFFFF0000;
EntityCallback GLENN_YNTIS_ANIM_C_STATE_CALLBACK asm("D_800A5BE4") = (EntityCallback)GlennYntisAnimStateC;
u32 D_800A5BE8 asm("D_800A5BE8") = 0xFFFF0000;
EntityCallback D_800A5BEC asm("D_800A5BEC") = (EntityCallback)BossRandomAttackChoice;
u32 SHRINEY_GUARD_IDLE_TIMEOUT_STATE_MARKER asm("D_800A5BF0") = 0xFFFF0000;
EntityCallback SHRINEY_GUARD_IDLE_TIMEOUT_STATE_CALLBACK asm("D_800A5BF4") = (EntityCallback)ShrineyGuardAttackCounterState;
u32 D_800A5BF8 asm("D_800A5BF8") = 0xFFFF0000;
EntityCallback D_800A5BFC asm("D_800A5BFC") = (EntityCallback)ShrineyGuardDeathState;
u32 D_800A5C00 asm("D_800A5C00") = 0xFFFF0000;
EntityCallback D_800A5C04 asm("D_800A5C04") = (EntityCallback)ShrineyGuardStartLoopingAttackState;
u32 D_800A5C08 asm("D_800A5C08") = 0xFFFF0000;
EntityCallback D_800A5C0C asm("D_800A5C0C") = (EntityCallback)ShrineyGuardReadyAttackState;
u32 SHRINEY_GUARD_REPEAT_ATTACK_STATE_MARKER asm("D_800A5C10") = 0xFFFF0000;
EntityCallback SHRINEY_GUARD_REPEAT_ATTACK_STATE_CALLBACK asm("D_800A5C14") = (EntityCallback)ShrineyGuardAttackAnimState;
u32 SHRINEY_GUARD_FINISH_ATTACK_STATE_MARKER asm("D_800A5C18") = 0xFFFF0000;
EntityCallback SHRINEY_GUARD_FINISH_ATTACK_STATE_CALLBACK asm("D_800A5C1C") = (EntityCallback)ShrineyGuardIdleState;
u32 JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_MARKER asm("D_800A5C20") = 0xFFFF0000;
EntityCallback JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_CALLBACK asm("D_800A5C24") = (EntityCallback)JoeHeadJoeSelectAttackPattern;
u32 D_800A5C28 asm("D_800A5C28") = 0xFFFF0000;
EntityCallback D_800A5C2C asm("D_800A5C2C") = (EntityCallback)JoeHeadJoeDeathAnimState;
u32 D_800A5C30 asm("D_800A5C30") = 0xFFFF0000;
EntityCallback D_800A5C34 asm("D_800A5C34") = (EntityCallback)JoeHeadJoeEnterActiveState;
u32 D_800A5C38 asm("D_800A5C38") = 0xFFFF0000;
EntityCallback D_800A5C3C asm("D_800A5C3C") = (EntityCallback)JoeHeadJoeReturnToIdleState;
u32 D_800A5C40 asm("D_800A5C40") = 0xFFFF0000;
EntityCallback D_800A5C44 asm("D_800A5C44") = (EntityCallback)JoeHeadJoeReturnToIdleStateAlt;
u32 D_800A5C48 asm("D_800A5C48") = 0xFFFF0000;
EntityCallback D_800A5C4C asm("D_800A5C4C") = (EntityCallback)JoeHeadJoeClearVoice;
u32 D_800A5C50 asm("D_800A5C50") = 0xFFFF0000;
EntityCallback D_800A5C54 asm("D_800A5C54") = (EntityCallback)JoeHeadJoeClearVoiceAlt;
u32 D_800A5C58 asm("D_800A5C58") = 0xFFFF0000;
EntityCallback D_800A5C5C asm("D_800A5C5C") = (EntityCallback)EnemyIdleTimerState;
u32 D_800A5C60 asm("D_800A5C60") = 0xFFFF0000;
EntityCallback D_800A5C64 asm("D_800A5C64") = (EntityCallback)EnemyDefeatState;
u32 D_800A5C68 asm("D_800A5C68") = 0xFFFF0000;
EntityCallback D_800A5C6C asm("D_800A5C6C") = (EntityCallback)EnemySpriteState;
u32 D_800A5C70[2] asm("D_800A5C70") = {0x32444E45, 0x00000000};
u32 D_800A5C78 asm("D_800A5C78") = 0xFFFF0000;
EntityCallback D_800A5C7C asm("D_800A5C7C") = (EntityCallback)EnemyStateInit_SetSpriteAndHandler;
u32 D_800A5C80 asm("D_800A5C80") = 0xFFFF0000;
EntityCallback D_800A5C84 asm("D_800A5C84") = (EntityCallback)EnemyState_TurnAroundWithToken;
u32 D_800A5C88 asm("D_800A5C88") = 0xFFFF0000;
EntityCallback D_800A5C8C asm("D_800A5C8C") = (EntityCallback)ProjectileEnterActiveState;
u32 D_800A5C90 asm("D_800A5C90") = 0xFFFF0000;
EntityCallback D_800A5C94 asm("D_800A5C94") = (EntityCallback)ProjectileState_HomingActive;
u32 D_800A5C98 asm("D_800A5C98") = 0xFFFF0000;
EntityCallback D_800A5C9C asm("D_800A5C9C") = (EntityCallback)ProjectileState_HomingActiveVariant2;
u32 D_800A5CA0 asm("D_800A5CA0") = 0xFFFF0000;
EntityCallback D_800A5CA4 asm("D_800A5CA4") = (EntityCallback)ProjectileState_HomingActiveVariant3;
u32 D_800A5CA8 asm("D_800A5CA8") = 0xFFFF0000;
EntityCallback D_800A5CAC asm("D_800A5CAC") = (EntityCallback)ProjectileState_HomingMissileTrack;
u32 D_800A5CB0 asm("D_800A5CB0") = 0xFFFF0000;
EntityCallback D_800A5CB4 asm("D_800A5CB4") = (EntityCallback)JoeHeadJoeBallRegularInitState;
u32 D_800A5CB8 asm("D_800A5CB8") = 0xFFFF0000;
EntityCallback D_800A5CBC asm("D_800A5CBC") = (EntityCallback)JoeHeadJoeState_HideAndNotifyGameState;
u32 D_800A5CC0 asm("D_800A5CC0") = 0xFFFF0000;
EntityCallback D_800A5CC4 asm("D_800A5CC4") = (EntityCallback)JoeHeadJoeState_EnterCollisionState;
u32 D_800A5CC8 asm("D_800A5CC8") = 0xFFFF0000;
EntityCallback D_800A5CCC asm("D_800A5CCC") = (EntityCallback)JoeHeadJoeBallStopSound;
u32 D_800A5CD0 asm("D_800A5CD0") = 0xFFFF0000;
EntityCallback D_800A5CD4 asm("D_800A5CD4") = (EntityCallback)JoeHeadJoeState_CollisionTick;
u32 D_800A5CD8 asm("D_800A5CD8") = 0xFFFF0000;
EntityCallback D_800A5CDC asm("D_800A5CDC") = (EntityCallback)JoeHeadJoeState_IdleAfterCollision;
u32 D_800A5CE0[2] asm("D_800A5CE0") = {0x32444E45, 0x00000000};
u32 D_800A5CE8 asm("D_800A5CE8") = 0xFFFF0000;
EntityCallback D_800A5CEC asm("D_800A5CEC") = (EntityCallback)ClayballIndicatorState_Wait;
u32 D_800A5CF0 asm("D_800A5CF0") = 0xFFFF0000;
EntityCallback D_800A5CF4 asm("D_800A5CF4") = (EntityCallback)ClayballState_DestroyWithDebris;
u32 D_800A5CF8 asm("D_800A5CF8") = 0xFFFF0000;
EntityCallback D_800A5CFC asm("D_800A5CFC") = (EntityCallback)ShrineyGuardDeactivateWithSound;
u32 D_800A5D00 asm("D_800A5D00") = 0xFFFF0000;
EntityCallback D_800A5D04 asm("D_800A5D04") = (EntityCallback)ClayballState_HideIndicatorAndWait;
u32 D_800A5D08[2] asm("D_800A5D08") = {0x32444E45, 0x00000000};

/* Migrated from .data blob: boss cos/sin + hitbox lookup tables
 * (D_8009BC08..D_8009C174, 1388B, 9 symbols grouped at 4-aligned anchor;
 * D_8009BC0A..D_8009C11C aliased via .set). Supersedes the C11C pilot. */
/* group island: 0-byte pad at 0x8009BC08, 9 aliased symbol(s); anchor D_8009BC08 (1388B). */
u8 D_8009BC08[1388] asm("D_8009BC08") = {
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0xFA, 0xFF,
    0x02, 0x00, 0xF5, 0xFF, 0x01, 0x00, 0xEF, 0xFF,
    0x01, 0x00, 0xEA, 0xFF, 0xFF, 0xFF, 0xE8, 0xFF,
    0xFD, 0xFF, 0xE6, 0xFF, 0xFA, 0xFF, 0xE5, 0xFF,
    0xF8, 0xFF, 0xE4, 0xFF, 0xF6, 0xFF, 0xE4, 0xFF,
    0xF4, 0xFF, 0xE5, 0xFF, 0xF2, 0xFF, 0xE6, 0xFF,
    0xEF, 0xFF, 0xE7, 0xFF, 0xEE, 0xFF, 0xE9, 0xFF,
    0xED, 0xFF, 0xEC, 0xFF, 0xEC, 0xFF, 0xEE, 0xFF,
    0xEB, 0xFF, 0xF0, 0xFF, 0xEB, 0xFF, 0xF3, 0xFF,
    0xEB, 0xFF, 0xF6, 0xFF, 0xEC, 0xFF, 0xF9, 0xFF,
    0xEC, 0xFF, 0xFC, 0xFF, 0xED, 0xFF, 0xFE, 0xFF,
    0xEE, 0xFF, 0x01, 0x00, 0xEF, 0xFF, 0x03, 0x00,
    0xF0, 0xFF, 0x06, 0x00, 0xF1, 0xFF, 0x08, 0x00,
    0xF2, 0xFF, 0x0B, 0x00, 0xF4, 0xFF, 0x0D, 0x00,
    0xF5, 0xFF, 0x10, 0x00, 0xF7, 0xFF, 0x13, 0x00,
    0xF9, 0xFF, 0x16, 0x00, 0xFD, 0xFF, 0x17, 0x00,
    0x01, 0x00, 0x19, 0x00, 0x04, 0x00, 0x17, 0x00,
    0x07, 0x00, 0x16, 0x00, 0x0A, 0x00, 0x13, 0x00,
    0x0C, 0x00, 0x10, 0x00, 0x0E, 0x00, 0x0D, 0x00,
    0x10, 0x00, 0x09, 0x00, 0x11, 0x00, 0x05, 0x00,
    0x11, 0x00, 0x01, 0x00, 0x11, 0x00, 0xFD, 0xFF,
    0x11, 0x00, 0xF9, 0xFF, 0x10, 0x00, 0xF5, 0xFF,
    0x10, 0x00, 0xF1, 0xFF, 0x0F, 0x00, 0xED, 0xFF,
    0x0D, 0x00, 0xE8, 0xFF, 0x0C, 0x00, 0xE5, 0xFF,
    0x09, 0x00, 0xE0, 0xFF, 0x06, 0x00, 0xDE, 0xFF,
    0x02, 0x00, 0xDB, 0xFF, 0xFD, 0xFF, 0xD9, 0xFF,
    0xF8, 0xFF, 0xD8, 0xFF, 0xF3, 0xFF, 0xD9, 0xFF,
    0xEE, 0xFF, 0xDA, 0xFF, 0xEA, 0xFF, 0xDB, 0xFF,
    0xE4, 0xFF, 0xDF, 0xFF, 0xE0, 0xFF, 0xE3, 0xFF,
    0xDE, 0xFF, 0xE8, 0xFF, 0xDC, 0xFF, 0xEE, 0xFF,
    0xDB, 0xFF, 0xF4, 0xFF, 0xDB, 0xFF, 0xFB, 0xFF,
    0xDB, 0xFF, 0x00, 0x00, 0xDB, 0xFF, 0x05, 0x00,
    0xDC, 0xFF, 0x0B, 0x00, 0xDF, 0xFF, 0x10, 0x00,
    0xE1, 0xFF, 0x16, 0x00, 0xE5, 0xFF, 0x1A, 0x00,
    0xE9, 0xFF, 0x1F, 0x00, 0xEF, 0xFF, 0x23, 0x00,
    0xF6, 0xFF, 0x26, 0x00, 0xFD, 0xFF, 0x27, 0x00,
    0x05, 0x00, 0x26, 0x00, 0x0D, 0x00, 0x24, 0x00,
    0x14, 0x00, 0x21, 0x00, 0x1B, 0x00, 0x1B, 0x00,
    0x21, 0x00, 0x15, 0x00, 0x25, 0x00, 0x0D, 0x00,
    0x28, 0x00, 0x05, 0x00, 0x29, 0x00, 0xFC, 0xFF,
    0x29, 0x00, 0xF3, 0xFF, 0x28, 0x00, 0xEB, 0xFF,
    0x26, 0x00, 0xE3, 0xFF, 0x22, 0x00, 0xDB, 0xFF,
    0x1D, 0x00, 0xD4, 0xFF, 0x16, 0x00, 0xCF, 0xFF,
    0x0E, 0x00, 0xCA, 0xFF, 0x06, 0x00, 0xC8, 0xFF,
    0xFE, 0xFF, 0xC6, 0xFF, 0xF6, 0xFF, 0xC5, 0xFF,
    0xED, 0xFF, 0xC5, 0xFF, 0xE5, 0xFF, 0xC6, 0xFF,
    0xDE, 0xFF, 0xC8, 0xFF, 0xD7, 0xFF, 0xCC, 0xFF,
    0xD0, 0xFF, 0xD1, 0xFF, 0xCB, 0xFF, 0xD6, 0xFF,
    0xC4, 0xFF, 0xDC, 0xFF, 0xC0, 0xFF, 0xE3, 0xFF,
    0xBB, 0xFF, 0xEC, 0xFF, 0xB9, 0xFF, 0xF5, 0xFF,
    0xB9, 0xFF, 0x00, 0x00, 0xB9, 0xFF, 0x09, 0x00,
    0xBB, 0xFF, 0x11, 0x00, 0xBD, 0xFF, 0x19, 0x00,
    0xC1, 0xFF, 0x22, 0x00, 0xC7, 0xFF, 0x2A, 0x00,
    0xCE, 0xFF, 0x31, 0x00, 0xD5, 0xFF, 0x37, 0x00,
    0xDE, 0xFF, 0x3C, 0x00, 0xE9, 0xFF, 0x40, 0x00,
    0xF3, 0xFF, 0x42, 0x00, 0xFE, 0xFF, 0x43, 0x00,
    0x09, 0x00, 0x42, 0x00, 0x14, 0x00, 0x40, 0x00,
    0x1E, 0x00, 0x3C, 0x00, 0x28, 0x00, 0x38, 0x00,
    0x2F, 0x00, 0x33, 0x00, 0x38, 0x00, 0x2B, 0x00,
    0x3E, 0x00, 0x24, 0x00, 0x46, 0x00, 0x1B, 0x00,
    0x4B, 0x00, 0x12, 0x00, 0x4F, 0x00, 0x06, 0x00,
    0x52, 0x00, 0xFC, 0xFF, 0x52, 0x00, 0xF0, 0xFF,
    0x50, 0x00, 0xE5, 0xFF, 0x4D, 0x00, 0xD9, 0xFF,
    0x48, 0x00, 0xCD, 0xFF, 0x42, 0x00, 0xC2, 0xFF,
    0x38, 0x00, 0xB8, 0xFF, 0x2B, 0x00, 0xAE, 0xFF,
    0x1D, 0x00, 0xA5, 0xFF, 0x0D, 0x00, 0x9E, 0xFF,
    0xFE, 0xFF, 0x9A, 0xFF, 0xEE, 0xFF, 0x98, 0xFF,
    0xDE, 0xFF, 0x98, 0xFF, 0xCF, 0xFF, 0x9C, 0xFF,
    0xBF, 0xFF, 0xA1, 0xFF, 0xB2, 0xFF, 0xA9, 0xFF,
    0xA7, 0xFF, 0xB3, 0xFF, 0x9D, 0xFF, 0xC0, 0xFF,
    0x94, 0xFF, 0xCF, 0xFF, 0x8C, 0xFF, 0xDE, 0xFF,
    0x87, 0xFF, 0xEF, 0xFF, 0x85, 0xFF, 0x02, 0x00,
    0x87, 0xFF, 0x17, 0x00, 0x8B, 0xFF, 0x2A, 0x00,
    0x93, 0xFF, 0x3E, 0x00, 0x9C, 0xFF, 0x4E, 0x00,
    0xAA, 0xFF, 0x60, 0x00, 0xC0, 0xFF, 0x6E, 0x00,
    0xD9, 0xFF, 0x7B, 0x00, 0xF2, 0xFF, 0x7F, 0x00,
    0x0B, 0x00, 0x80, 0x00, 0x21, 0x00, 0x7C, 0x00,
    0x35, 0x00, 0x76, 0x00, 0x48, 0x00, 0x6F, 0x00,
    0x56, 0x00, 0x65, 0x00, 0x69, 0x00, 0x59, 0x00,
    0x77, 0x00, 0x4A, 0x00, 0x85, 0x00, 0x39, 0x00,
    0x8E, 0x00, 0x28, 0x00, 0x97, 0x00, 0x0D, 0x00,
    0x9C, 0x00, 0xF6, 0xFF, 0x9A, 0x00, 0xDE, 0xFF,
    0x95, 0x00, 0xC7, 0xFF, 0x8B, 0x00, 0xB0, 0xFF,
    0x7C, 0x00, 0x98, 0xFF, 0x69, 0x00, 0x85, 0xFF,
    0x54, 0x00, 0x76, 0xFF, 0x41, 0x00, 0x68, 0xFF,
    0x2B, 0x00, 0x5D, 0xFF, 0x10, 0x00, 0x55, 0xFF,
    0xF5, 0xFF, 0x50, 0xFF, 0xDB, 0xFF, 0x52, 0xFF,
    0xC1, 0xFF, 0x58, 0xFF, 0xA3, 0xFF, 0x66, 0xFF,
    0x87, 0xFF, 0x78, 0xFF, 0x70, 0xFF, 0x8D, 0xFF,
    0x5E, 0xFF, 0xA3, 0xFF, 0x4E, 0xFF, 0xBC, 0xFF,
    0x41, 0xFF, 0xDA, 0xFF, 0x3B, 0xFF, 0xF7, 0xFF,
    0x37, 0xFF, 0x19, 0x00, 0x3E, 0xFF, 0x39, 0x00,
    0x49, 0xFF, 0x5B, 0x00, 0x63, 0xFF, 0x77, 0x00,
    0x7F, 0xFF, 0x90, 0x00, 0x9D, 0xFF, 0x9D, 0x00,
    0xBA, 0xFF, 0xA8, 0x00, 0xDC, 0xFF, 0xAD, 0x00,
    0xFF, 0xFF, 0xB0, 0x00, 0x22, 0x00, 0xAF, 0x00,
    0x43, 0x00, 0xAA, 0x00, 0x61, 0x00, 0x9F, 0x00,
    0x7D, 0x00, 0x91, 0x00, 0x94, 0x00, 0x80, 0x00,
    0xAA, 0x00, 0x6C, 0x00, 0xC0, 0x00, 0x49, 0x00,
    0xD0, 0x00, 0x26, 0x00, 0xD8, 0x00, 0x02, 0x00,
    0x01, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xFD, 0xFF,
    0xFB, 0xFF, 0xFA, 0xFF, 0xF8, 0xFF, 0xF7, 0xFF,
    0xF5, 0xFF, 0xF4, 0xFF, 0xF1, 0xFF, 0xF2, 0xFF,
    0xEE, 0xFF, 0xEF, 0xFF, 0xEA, 0xFF, 0xED, 0xFF,
    0xE7, 0xFF, 0xEA, 0xFF, 0xE3, 0xFF, 0xE8, 0xFF,
    0xE0, 0xFF, 0xE6, 0xFF, 0xDC, 0xFF, 0xE4, 0xFF,
    0xD8, 0xFF, 0xE2, 0xFF, 0xD5, 0xFF, 0xE0, 0xFF,
    0xD1, 0xFF, 0xDE, 0xFF, 0xCD, 0xFF, 0xDD, 0xFF,
    0xC9, 0xFF, 0xDB, 0xFF, 0xC5, 0xFF, 0xDA, 0xFF,
    0xC1, 0xFF, 0xD8, 0xFF, 0xBD, 0xFF, 0xD7, 0xFF,
    0xB9, 0xFF, 0xD6, 0xFF, 0xB5, 0xFF, 0xD5, 0xFF,
    0xB1, 0xFF, 0xD4, 0xFF, 0xAD, 0xFF, 0xD3, 0xFF,
    0xA9, 0xFF, 0xD3, 0xFF, 0xA4, 0xFF, 0xD2, 0xFF,
    0xA0, 0xFF, 0xD1, 0xFF, 0x9C, 0xFF, 0xD1, 0xFF,
    0x98, 0xFF, 0xD1, 0xFF, 0x94, 0xFF, 0xD1, 0xFF,
    0x8F, 0xFF, 0xD1, 0xFF, 0x8B, 0xFF, 0xD1, 0xFF,
    0x87, 0xFF, 0xD1, 0xFF, 0x83, 0xFF, 0xD1, 0xFF,
    0x7F, 0xFF, 0xD1, 0xFF, 0x7A, 0xFF, 0xD2, 0xFF,
    0x76, 0xFF, 0xD3, 0xFF, 0x72, 0xFF, 0xD3, 0xFF,
    0x6E, 0xFF, 0xD4, 0xFF, 0x6A, 0xFF, 0xD5, 0xFF,
    0x66, 0xFF, 0xD6, 0xFF, 0x62, 0xFF, 0xD7, 0xFF,
    0x5E, 0xFF, 0xD8, 0xFF, 0x5A, 0xFF, 0xDA, 0xFF,
    0x56, 0xFF, 0xDB, 0xFF, 0x52, 0xFF, 0xDD, 0xFF,
    0x4E, 0xFF, 0xDE, 0xFF, 0x4A, 0xFF, 0xE0, 0xFF,
    0x46, 0xFF, 0xE2, 0xFF, 0x43, 0xFF, 0xE4, 0xFF,
    0x3F, 0xFF, 0xE6, 0xFF, 0x3B, 0xFF, 0xE8, 0xFF,
    0x38, 0xFF, 0xEA, 0xFF, 0x34, 0xFF, 0xED, 0xFF,
    0x31, 0xFF, 0xEF, 0xFF, 0x2E, 0xFF, 0xF2, 0xFF,
    0x2A, 0xFF, 0xF4, 0xFF, 0x27, 0xFF, 0xF7, 0xFF,
    0x24, 0xFF, 0xFA, 0xFF, 0x21, 0xFF, 0xFD, 0xFF,
    0x1E, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03,
    0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06,
    0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08,
    0x08, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D,
    0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F,
    0x0F, 0x0F, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11,
    0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13,
    0x13, 0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15,
    0x15, 0x16, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17,
    0x17, 0x17, 0x18, 0x18, 0x18, 0x18, 0x18, 0x19,
    0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A,
    0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C,
    0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
    0x1D, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x06, 0x0C, 0x12,
    0x19, 0x1F, 0x25, 0x2B, 0x31, 0x38, 0x3E, 0x44,
    0x4A, 0x50, 0x56, 0x5C, 0x61, 0x67, 0x6D, 0x73,
    0x78, 0x7E, 0x83, 0x88, 0x8E, 0x93, 0x98, 0x9D,
    0xA2, 0xA7, 0xAB, 0xB0, 0xB5, 0xB9, 0xBD, 0xC1,
    0xC5, 0xC9, 0xCD, 0xD1, 0xD4, 0xD8, 0xDB, 0xDE,
    0xE1, 0xE4, 0xE7, 0xEA, 0xEC, 0xEE, 0xF1, 0xF3,
    0xF4, 0xF6, 0xF8, 0xF9, 0xFB, 0xFC, 0xFD, 0xFE,
    0xFE, 0xFF, 0xFF, 0xFF, 0xA0, 0x80, 0x94, 0x06,
    0xA0, 0x80, 0x95, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x22, 0x16, 0xA1, 0x2C, 0x32, 0x7C, 0x65,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xC0, 0xFF,
    0x0D, 0x00, 0xC1, 0xFF, 0x19, 0x00, 0xC5, 0xFF,
    0x24, 0x00, 0xCB, 0xFF, 0x2E, 0x00, 0xD3, 0xFF,
    0x36, 0x00, 0xDC, 0xFF, 0x3C, 0x00, 0xE7, 0xFF,
    0x3F, 0x00, 0xF3, 0xFF, 0x40, 0x00, 0x00, 0x00,
    0x3F, 0x00, 0x0C, 0x00, 0x3C, 0x00, 0x18, 0x00,
    0x36, 0x00, 0x23, 0x00, 0x2E, 0x00, 0x2D, 0x00,
    0x24, 0x00, 0x35, 0x00, 0x19, 0x00, 0x3B, 0x00,
    0x0D, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x3F, 0x00,
    0xF4, 0xFF, 0x3E, 0x00, 0xE8, 0xFF, 0x3B, 0x00,
    0xDD, 0xFF, 0x35, 0x00, 0xD4, 0xFF, 0x2D, 0x00,
    0xCC, 0xFF, 0x23, 0x00, 0xC6, 0xFF, 0x18, 0x00,
    0xC2, 0xFF, 0x0C, 0x00, 0xC1, 0xFF, 0x00, 0x00,
    0xC2, 0xFF, 0xF3, 0xFF, 0xC6, 0xFF, 0xE7, 0xFF,
    0xCC, 0xFF, 0xDC, 0xFF, 0xD4, 0xFF, 0xD3, 0xFF,
    0xDD, 0xFF, 0xCB, 0xFF, 0xE8, 0xFF, 0xC5, 0xFF,
    0xF4, 0xFF, 0xC1, 0xFF,
};
asm(".globl D_8009BC0A\n.set D_8009BC0A, D_8009BC08 + 2\n.globl D_8009BF28\n.set D_8009BF28, D_8009BC08 + 800\n.globl D_8009C01C\n.set D_8009C01C, D_8009BC08 + 1044\n.globl D_8009C09C\n.set D_8009C09C, D_8009BC08 + 1172\n.globl D_8009C0DC\n.set D_8009C0DC, D_8009BC08 + 1236\n.globl D_8009C0E8\n.set D_8009C0E8, D_8009BC08 + 1248\n.globl D_8009C0F4\n.set D_8009C0F4, D_8009BC08 + 1260\n.globl D_8009C11C\n.set D_8009C11C, D_8009BC08 + 1300\n");
