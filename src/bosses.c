#include "common.h"

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

INCLUDE_ASM("asm/nonmatchings/bosses", EntityTickWithShortTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityMessageHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityEventHandlerWithComplete);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggEventHandlerWithTrigger);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityFollowPathTick);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityDestroyWithEffects);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityStopSound);

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

INCLUDE_ASM("asm/nonmatchings/bosses", Hazard_TickWithBehaviorTransition);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDamageEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", GlennYntisDeathEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardActivateWithSound);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardStopSound);

INCLUDE_ASM("asm/nonmatchings/bosses", CollectibleAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardIdleWithSound);

INCLUDE_ASM("asm/nonmatchings/bosses", HazardStopSoundAlt);

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

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardIdleTickCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardStunTickCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", BossEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardActiveEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardMoveCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", BossRandomAttackChoice);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardAttackCounterState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardSetAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardSetLoopingAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardIdleState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardAttackAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardReadyAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardStartLoopAttackState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardDeathState);

INCLUDE_ASM("asm/nonmatchings/bosses", ShrineyGuardDeathCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", InitJoeHeadJoeBoss);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBossDestructor);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeUpdateWithCollisionCheck);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoe_CheckAttackAndUpdate);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeAttackEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeBounceEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeDeathEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSelectAttackPattern);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeCheckPlayerInAttackRange);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSetIdleState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeClearVoice);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeSetFacingAndAttack);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeEnterActiveState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeMoveAndCheckAttack);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeClearVoiceAlt);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeReturnToIdleState);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeReturnToIdleStateAlt);

INCLUDE_ASM("asm/nonmatchings/bosses", JoeHeadJoeDeathAnimState);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggDeathCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", InitKloggBoss);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", SpawnParticleAndMenuEntity);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggUpdateWithSound);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggUpdateWithTimer);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyHitMessageHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", EntityCollision_EnemyHitWithCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggDeathEventHandler);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggMoveToTargetPosition);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggSpawnProjectilesCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", KloggSetMoveState);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyIdleTimerState);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemySpriteState);

INCLUDE_ASM("asm/nonmatchings/bosses", EnemyDefeatState);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageDeathCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMagePartDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMagePlatformDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageForceFieldDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageAuxDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageBossPartDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageHUDDestroyCallback);

void func_8004F024(void) {
}

void func_8004F02C(void) {
}

INCLUDE_ASM("asm/nonmatchings/bosses", MonkeyMageSimpleDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/bosses", FreeEntityAllocOnly);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F098);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F114);

INCLUDE_ASM("asm/nonmatchings/bosses", func_8004F190);

INCLUDE_ASM("asm/nonmatchings/bosses", MaskAngleCosineEntry);

