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
extern void *BOSS_SPU_CLEANUP_VTABLE asm("D_80011388");
extern void *KLOGG_BOSS_VTABLE asm("D_80011268");
extern void *JOE_HEAD_JOE_BOSS_VTABLE asm("D_80011288");
extern void *MONKEY_MAGE_SIMPLE_ALLOC_VTABLE asm("D_800113A8");

extern s32 EntityMessageHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 BossEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 JoeHeadJoeAttackEventHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern s32 EnemyHitMessageHandler(Entity *e, u32 event, u32 arg2, u32 arg3);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void GlennYntisSelectRandomAnimState(Entity *e);
extern void HazardActivateWithSound(Entity *e);
extern void UpdateEntitySoundPanning(Entity *e, u32 sound);
extern void SetAnimationSpriteId(Entity *e, s32 id);
extern Entity *CreateFadeOverlayEntity(Entity *e);
extern void AddToZOrderList(GameState *gs, Entity *entity);

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

INCLUDE_ASM("asm/nonmatchings/bosses", EntityFollowPathTick);

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

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestructor_WithSPUStop);

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

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisSetPhaseFromHP);

void HazardSelectRandomBehavior(ShrineyGuardEntity *e) {
    PadSlot slot;
    void (*fn)();
    s16 m1;
    e->idleTimeout = 0;
    GlennYntisSelectRandomAnimState((Entity *)e);
    fn = HazardActivateWithSound;
    __asm__ volatile("" : "=r"(fn) : "0"(fn));
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)((u8 *)e + 0x98) = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisSelectRandomAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisIdleAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisAnimStateB);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisAnimStateC);

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

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardSetAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardSetLoopingAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardIdleState);

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
        *(u8 *)((u8 *)g_pGameState + 0x170) = 0;
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

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeReturnToIdleState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeReturnToIdleStateAlt);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeDeathAnimState);

void KloggDeathCallback(u8 *e) {
    u8 *p = *(u8 **)(e + 0x34);
    p[0xA] = 0;
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
    u8 *p = *(u8 **)(e + 0x34);
    p[0xA] = 0;
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

