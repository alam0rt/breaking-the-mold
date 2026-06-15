#include "common.h"

extern s32 rand(void);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessBounceCollision);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerClearSwirlPortalEntity);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerNotifyEntityRelease);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessTileCollision);

INCLUDE_ASM("asm/nonmatchings/playst", SpawnSwirlPortalEntity);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerSpawnDustParticle);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessEnemyHit);

INCLUDE_ASM("asm/nonmatchings/playst", CheckPlayerHitByEnemy);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerSetupCallbacksAndSprite);

INCLUDE_ASM("asm/nonmatchings/playst", SpawnPlayerColoredParticle);

INCLUDE_ASM("asm/nonmatchings/playst", SpawnPlayerDeathEffect);

void ApplyRandomRGBEffect(void *e) {
    *(u8 *)((u8 *)e + 0x15D) = rand();
    *(u8 *)((u8 *)e + 0x15E) = rand();
    *(u8 *)((u8 *)e + 0x15F) = rand();
    *(u8 *)((u8 *)e + 0x1AE) = 1;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerTickCallback);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_IdleRandom);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SimpleTick);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_FrameCountTick);

void PlayerState_CooldownTick(void *e) {
    u8 *p = (u8 *)e;
    if (p[0x13C] != 0) {
        p[0x13C]--;
    }
    PlayerTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_FallWithRotation);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_TransitionToPickup);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_QueuedCallbackTimer);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_UpdateAttachedEntity);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerAlt);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SetAndProcessQueue);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005C080);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerV2);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateAndProcessQueue);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005C1A4);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityCollisionHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityEventHandlerAlt);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_EventHandlerWithQueue);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_AnimFrameParticleHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CollisionHandlerWithQueue);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerBounceCallback);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_TeleporterAnimHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BounceEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathDebrisSpawner);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ThrowEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ThrowSetStateAndSpawn);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005DDC0);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CheckpointEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CheckpointSaveAndSpawnHUD);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005E150);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProjectileEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProjectileSetStateAndSpawn);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005E430);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BaseEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateWithQueueProcess);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005E554);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathAnimEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathAnimSetStateHandler);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005EA60);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathSpawnMenuEntities);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005EAAC);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathSpawnMenuEntity);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005ECEC);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_AllocAndInitScrollLayer);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005EF68);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_InitScrollLayerAndAdd);

INCLUDE_ASM("asm/nonmatchings/playst", func_8005F0DC);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SpawnTrailParticle);

INCLUDE_ASM("asm/nonmatchings/playst", SpawnPlayerTrailParticle);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_EventHandlerAltWithQueue);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CollisionDamageSetup);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkToRunTransition);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CollisionTrailEntityUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", TryActivatePowerup);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerPlaySoundOnDirectionInput);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_IdleInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkSpeedCheck);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateIfNotWalkingLeft);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_EntitySetStateProxy);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputWithJump);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputWithFall);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpWallCollisionInput);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpInputAndCounters);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpDirectionAndCounters);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpTickHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateAndUpdateCounters);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpCounterUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_FallInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CrouchClimbInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", func_80060C18);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateSimple);

INCLUDE_ASM("asm/nonmatchings/playst", func_80060C4C);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CrouchClimbTickHandler);

INCLUDE_ASM("asm/nonmatchings/playst", func_80060DB4);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStatePassthrough);

INCLUDE_ASM("asm/nonmatchings/playst", func_80060DE8);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DebugCameraInput);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DebugCameraVertical);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DebugCameraUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", ApplyEntityPositionUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_HorizontalWallCollision);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProcessBounceWrapper3);

INCLUDE_ASM("asm/nonmatchings/playst", func_80061774);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStatePassthrough2);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_VerticalCollisionCheck);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_RidingPlatformPhysics);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProcessBounceWrapper1);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerRidingPlatformUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ScaleBoundsWrapper);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ScaleBoundsAndCollision);

