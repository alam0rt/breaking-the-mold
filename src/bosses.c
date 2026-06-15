#include "common.h"
#include "functions.h"

extern u8 *g_pGameState;
extern void *g_pBlbHeapBase;
extern void *D_800112E8;
extern void *D_80011308;
extern void *D_80011328;
extern void *D_80011348;
extern void *D_80011368;
extern void *D_80011388;
extern void *D_80011268;
extern s32 EntityMessageHandler(void *e, u32 event);
extern s32 JoeHeadJoeAttackEventHandler(void *e, u32 event);
extern void EntitySetState(Entity *e, u32 marker, void *fn);
extern void GlennYntisSelectRandomAnimState(Entity *e);
extern void HazardActivateWithSound(Entity *e);
/* gp_rel tentative defs (sdata blob owns the strong defs). */
u32   D_800A5B98;
void *D_800A5B9C;
u32   D_800A5BF0;
void *D_800A5BF4;
u32   D_800A5C10;
void *D_800A5C14;
u32   D_800A5C18;
void *D_800A5C1C;
u32   D_800A5C20;
void *D_800A5C24;

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
            EntitySetState(e, D_800A5B98, D_800A5B9C);
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
            EntitySetState(e, D_800A5BF0, D_800A5BF4);
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
        EntitySetState(e, D_800A5C10, D_800A5C14);
    } else {
        EntitySetState(e, D_800A5C18, D_800A5C1C);
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
    g_pGameState[0x148] = e[0x110];
    e[0x106] = 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitJoeHeadJoeBoss);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBossDestructor);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeUpdateWithCollisionCheck);

void JoeHeadJoe_CheckAttackAndUpdate(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x112);
    JoeHeadJoeCheckPlayerInAttackRange(e);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntitySetState(e, D_800A5C20, D_800A5C24);
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
    g_pGameState[0x148] = e[0x110];
    e[0x106] = 1;
}

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggBoss);

void KloggDestroyCallback(void *e, u32 flags) {
    *(void **)((u8 *)e + 0x18) = &D_80011268;
    StopSPUVoice(*(s32 *)((u8 *)e + 0x110));
    *(s32 *)((u8 *)e + 0x110) = -1;
    DestroyEntityAndFreeMemory(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/bosses", SpawnParticleAndMenuEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggUpdateWithSound);

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
    g_pGameState[0x148] = e[0x106];
}

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageDestroyCallback);

void MonkeyMagePartDestroyCallback(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011388;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMagePlatformDestroyCallback(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_800112E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageForceFieldDestroyCallback(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011328;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageAuxDestroyCallback(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011348;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageBossPartDestroyCallback(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011368;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void MonkeyMageHUDDestroyCallback(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011388;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_8004F024(void) {
}

void func_8004F02C(void) {
}

extern void *D_800113A8;

void MonkeyMageSimpleDestroyCallback(void *entity, u32 flag) {
    *(void **)((u8 *)entity + 0x18) = &D_800113A8;
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

