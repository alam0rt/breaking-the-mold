#include "common.h"
#include "functions.h"
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

extern s32 EntityMessageHandler(void *e, u32 event);
extern s32 JoeHeadJoeAttackEventHandler(void *e, u32 event);
extern void EntitySetState(Entity *e, u32 marker, void *fn);
extern void GlennYntisSelectRandomAnimState(Entity *e);
extern void HazardActivateWithSound(Entity *e);
extern void UpdateEntitySoundPanning(void *e, u32 sound);
/* gp_rel tentative defs (sdata blob owns the strong defs). */
u32   HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B98");
void *HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B9C");
u32   SHRINEY_GUARD_IDLE_TIMEOUT_STATE_MARKER asm("D_800A5BF0");
void *SHRINEY_GUARD_IDLE_TIMEOUT_STATE_CALLBACK asm("D_800A5BF4");
u32   SHRINEY_GUARD_REPEAT_ATTACK_STATE_MARKER asm("D_800A5C10");
void *SHRINEY_GUARD_REPEAT_ATTACK_STATE_CALLBACK asm("D_800A5C14");
u32   SHRINEY_GUARD_FINISH_ATTACK_STATE_MARKER asm("D_800A5C18");
void *SHRINEY_GUARD_FINISH_ATTACK_STATE_CALLBACK asm("D_800A5C1C");
u32   JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_MARKER asm("D_800A5C20");
void *JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_CALLBACK asm("D_800A5C24");

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggBossEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickWithReadyCheck);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickWithTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderFallTickCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GliderEventHandlerWithComplete);

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
    u16 *ctr = (u16 *)((u8 *)e + 0x104);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    EntityUpdateCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", EntityMessageHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityEventHandlerWithComplete);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggEventHandlerWithTrigger);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityFollowPathTick);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyWithEffects);

void EntityStopSound(void *e) {
    StopSPUVoice(*(s32 *)((u8 *)e + 0x144));
    *(s32 *)((u8 *)e + 0x144) = -1;
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
    u16 *ctr = (u16 *)((u8 *)e + 0x114);
    CollectibleTickCallback(e);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
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

void HazardStopSound(void *e) {
    StopSPUVoice(*(s32 *)((u8 *)e + 0x118));
    *(s32 *)((u8 *)e + 0x118) = -1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardIdleWithSound);

void HazardStopSoundAlt(void *e) {
    StopSPUVoice(*(s32 *)((u8 *)e + 0x118));
    *(s32 *)((u8 *)e + 0x118) = -1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleActiveState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisSetPhaseFromHP);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardSelectRandomBehavior);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisSelectRandomAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisIdleAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisAnimStateB);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisAnimStateC);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDefeatedState);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisVictoryCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", InitShrineyGuardBoss);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyUpdateWithCollisionAndDeath);

void ShrineyGuardIdleTickCallback(Entity *e) {
    u8 *ctr = (u8 *)e + 0x112;
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntitySetState(e, SHRINEY_GUARD_IDLE_TIMEOUT_STATE_MARKER,
                           SHRINEY_GUARD_IDLE_TIMEOUT_STATE_CALLBACK);
        }
    }
    EnemyUpdateWithCollisionAndDeath(e);
}

void ShrineyGuardStunTickCallback(Entity *e) {
    u8 *ctr = (u8 *)e + 0x113;
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            *(u8 *)((u8 *)e + 0x115) = 0;
            *(u8 *)((u8 *)e + 0x10C) = 1;
        }
    }
    EnemyUpdateWithCollisionAndDeath(e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", BossEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardActiveEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardMoveCallback);

void BossRandomAttackChoice(Entity *e) {
    *(u8 *)((u8 *)e + 0x112) = 0x5A;
    if ((rand() & 1) == 0) {
        ShrineyGuardSetAttackState(e);
    } else {
        ShrineyGuardSetLoopingAttackState(e);
    }
}

void ShrineyGuardAttackCounterState(Entity *e) {
    u8 *counter = (u8 *)e + 0x114;
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

void JoeHeadJoeBossDestructor(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &JOE_HEAD_JOE_BOSS_VTABLE;
    *(s32 *)((u8 *)entity + 0x118) = -1;
    ((Entity *)entity)->collisionVtable = &BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeUpdateWithCollisionCheck);

void JoeHeadJoe_CheckAttackAndUpdate(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x112);
    JoeHeadJoeCheckPlayerInAttackRange(e);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntitySetState(e, JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_MARKER,
                           JOE_HEAD_JOE_ATTACK_TIMEOUT_STATE_CALLBACK);
        }
    }
    JoeHeadJoeUpdateWithCollisionCheck(e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBounceEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeDeathEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSelectAttackPattern);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeCheckPlayerInAttackRange);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSetIdleState);

void JoeHeadJoeClearVoice(void *e) {
    *(s32 *)((u8 *)e + 0x118) = -1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSetFacingAndAttack);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeEnterActiveState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeMoveAndCheckAttack);

void JoeHeadJoeClearVoiceAlt(void *e) {
    *(s32 *)((u8 *)e + 0x118) = -1;
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

void KloggDestroyCallback(void *e, u32 flags) {
    ((Entity *)e)->collisionVtable = &KLOGG_BOSS_VTABLE;
    StopSPUVoice(*(s32 *)((u8 *)e + 0x110));
    *(s32 *)((u8 *)e + 0x110) = -1;
    DestroyEntityAndFreeMemory(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", SpawnParticleAndMenuEntity);

void KloggUpdateWithSound(Entity *e) {
    UpdateEntitySoundPanning(e, *(u32 *)((u8 *)e + 0x110));
    if (*(u16 *)((u8 *)e + 0x116) != 0) {
        *(u16 *)((u8 *)e + 0x116) -= 1;
    }
    EntityUpdateCallback(e);
}

void KloggUpdateWithTimer(Entity *e) {
    u16 *ctr1 = (u16 *)((u8 *)e + 0x104);
    if (*ctr1 != 0) {
        *ctr1 -= 1;
        if (*ctr1 == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    UpdateEntitySoundPanning(e, *(s32 *)((u8 *)e + 0x110));
    if (*(u16 *)((u8 *)e + 0x116) != 0) {
        *(u16 *)((u8 *)e + 0x116) -= 1;
    }
    EntityUpdateCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyHitMessageHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityCollision_EnemyHitWithCallback);

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

void MonkeyMagePartDestroyCallback(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMagePlatformDestroyCallback(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &BOSS_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageForceFieldDestroyCallback(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &BOSS_AUX_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageAuxDestroyCallback(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &MONKEY_MAGE_AUX_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageBossPartDestroyCallback(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &BOSS_PARTICLE_SPAWN_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageHUDDestroyCallback(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &BOSS_SPU_CLEANUP_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_8004F024(void) {
}

void func_8004F02C(void) {
}

void MonkeyMageSimpleDestroyCallback(void *entity, u32 flag) {
    ((Entity *)entity)->collisionVtable = &MONKEY_MAGE_SIMPLE_ALLOC_VTABLE;
    if (flag & 1) {
        FreeEntityAllocOnly(entity, 0x1C);
    }
}

void FreeEntityAllocOnly(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F098);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F114);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F190);

INCLUDE_ASM("asm/nonmatchings/bosses", MaskAngleCosineEntry);

