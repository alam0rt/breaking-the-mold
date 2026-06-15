#include "common.h"

INCLUDE_ASM("asm/nonmatchings/enemies", LineSegmentIntersectsRect);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntityFromSpawn);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleFromSpawn);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntityDirect);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleAtPosition);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CalcEntityYFromTileHeight);

INCLUDE_ASM("asm/nonmatchings/enemies", UpdateCollectibleTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckCollectibleOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", TimedCollectibleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickFinnMode);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerIdle);

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnCollectibleParticles);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleWithFlags);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleSparkleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", TimedSparkleCollectibleTick);

INCLUDE_ASM("asm/nonmatchings/enemies", AIEntityRandomBehaviorTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandler0x1001_1002_1008);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandler0x1001_1002_1008_V2);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithRandomWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithDelayedWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityGroundSnapWithAnimation);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStartWalkWithTimer0x2d);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStartWalkWithTimer10);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetIdle);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetRandomBehavior);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetAttack);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySetSparkleCollectibleState);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySetSparkleDelay3);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySetSparkleDelay2);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySetSparkleDelay1);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetSparkle);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence4A);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence4B);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence4C);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroySoundEmitterEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterStunnedTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySimpleEventPassthrough);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFollowPathWithWrapping);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyStartMovingWithSound);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityInitSoundEmitterState);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyEnterSoundEmitterState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSpecialPickupEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TripleLaserMonkeyDeathTick);

INCLUDE_ASM("asm/nonmatchings/enemies", TripleLaserMonkeyConditionalTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithCountdownToWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", LaserMonkeyWalkState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTripleLaserMonkeyAttackState);

INCLUDE_ASM("asm/nonmatchings/enemies", LaserMonkeyIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitWalkingCollectibleEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyDeathWithParticles);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityTimerDeathWithParticles);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnParticle);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFallingGravityWithCollision);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyPatrolState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEnemyFallingState);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyDeathState);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckEntityBehindCamera);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSnoBloEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnProjectile);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleTimer0x1e_SpriteC);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleTimer0x1e);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleTimer0x3c_SpriteA);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleTimer0x3c_SpriteB);

INCLUDE_ASM("asm/nonmatchings/enemies", InitShooterEnemyStateA);

INCLUDE_ASM("asm/nonmatchings/enemies", InitShooterEnemyStateB);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleIdleStateB);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence3A);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence3B);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence9Frames);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSoundEmittingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnWalkingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", AnimatedEntityToggleSpriteA);

INCLUDE_ASM("asm/nonmatchings/enemies", AnimatedEntityToggleSpriteB);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetLoopingAnimation);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTimerBasedMenuEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySpawnerTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingHazard);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroySoundEntityWithVoice);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterIdleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityPathFollowerWithTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", InitLevelStateCollectible);

INCLUDE_ASM("asm/nonmatchings/enemies", ConditionalCollectibleTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerCountdownToWalkWithSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleWalkState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitConditionalCollectibleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitAnimatedTimedCollectible);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleFrameConditionalTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleVariant);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithDeathSpawn);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityTimerCountdownDeathSpawn);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerAnimationSwitch);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerTimerCountdown);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityRandomIdleOrAnimated);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityState_Idle);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEnemyAnimatedWithDeathSpawn);

void SetEntityFacingDirection(void *e, s32 dir);

void EntitySetFacingRight(void *e) {
    SetEntityFacingDirection(e, 2);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityState_Animated);

INCLUDE_ASM("asm/nonmatchings/enemies", InitProjectilePathEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", ProjectilePathFollowerTick);

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnProjectileEntityDef);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithChildSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroyEntityWithSoundAndChild);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickWithSoundPanning);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnMultipleProjectiles);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010C64);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_800410bc);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_80041120);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010E04);

INCLUDE_ASM("asm/nonmatchings/enemies", func_800411CC);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityFromHeapContext);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_80041230);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_80041294);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_800412f8);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_8004135c);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010DE4_800413c0);

void NopStub_80041424(void) {
}

void NopStub_8004142c(void) {
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80010E04_80041434);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityNoTeardown_80041468);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCheckpointEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroyEntityWithChildRemoval);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFloatingWithCollisionTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntity_Alt);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityTimedStateSwitchTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityUpdateWithCollisionOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableChildEntityOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityConditionalActivateTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityUpdateWithSpawnDataCheck);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityUpdateWithCollisionSpawnCheck);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnDataOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySimpleEventPassthrough_V2);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventTimerCountdownWithGameState);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityHideAndDisable);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithTypeBasedTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPlatformEntityState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitDirectionalScaledEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformTimerTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformCollisionTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnRefOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformEventHandlerSpawnEffect);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformHideAndDisable);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitBouncableClayEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityOffScreenChildCleanup);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityTick_CollisionWithCleanup);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardTimerTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardEventHandler_0x1001);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardEventHandler_0x1001_V2);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventPassthrough_Event2);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityPathMovementUpdate);

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_IdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_DestroyingState);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardActiveState);

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_HiddenState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledMovingEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TimedEntityTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", TimedEntityTickCallbackWithCollision);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityUpdateWithCollisionWrapper);

INCLUDE_ASM("asm/nonmatchings/enemies", SwitchEventHandler_SetGameFlag);

void EntityIncrementWorldX(void *e) {
    *(s16 *)((u8 *)e + 0x68) += 1;
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityHideWithTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSwitchActivatedState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitSwitchMovingState);

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

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterEventPassthrough_Event2);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTeleporterActivatingState);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCameraEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterTransitionTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterPortalEventHandler);

INCLUDE_ASM("asm/nonmatchings/enemies", TeleporterExitState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitIndexedSpriteEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityOffscreenParentCleanupTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCameraTrackingEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterWithPanningTick);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011068);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011088);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt800110A8);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt800111C8);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt800110E8);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011108);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011128);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011148);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011168);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011188);

void NopStub_80045e70(void) {
}

void NopStub_80045e78(void) {
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt800111C8_80045e80);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityNoTeardown_80045eb4);

INCLUDE_ASM("asm/nonmatchings/enemies", InitFloatingPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleScaledTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWalkWithTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityGroundSnapMovementCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemSparkleIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemRevealState);

INCLUDE_ASM("asm/nonmatchings/enemies", SetEntitySpecialState_1);

INCLUDE_ASM("asm/nonmatchings/enemies", SetEntitySpecialState_2);

INCLUDE_ASM("asm/nonmatchings/enemies", SetEntitySpecialState_3);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetSpecial);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence7Frames);

INCLUDE_ASM("asm/nonmatchings/enemies", StartAnimSequence13Frames);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEntity_Alt);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEntityDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", FinnModeCollectibleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyPathFollowWithFacingFlip);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetWalkSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetIdleSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyDestroyCallback_0x80011228);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011228);

void NopStub_80046ce4(void) {
}

void NopStub_80046cec(void) {
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt80011248);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityNoTeardown_80046d28);

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundParticleEmitter);

INCLUDE_ASM("asm/nonmatchings/enemies", ParticleEmitterTickCallback);

s32 EntitySetReadyFlag(void *e, u16 mode) {
    if (mode == 2) {
        *(u8 *)((u8 *)e + 0x100) = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundSparkleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", BackgroundSparkleFadeTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", BackgroundSparkleContactEventHandler);

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundSparkleRevealState);

INCLUDE_ASM("asm/nonmatchings/enemies", SetEntityAnimationState);

void func_8004727C(void *e) {
    *(u8 *)((u8 *)e + 0x101) = 0x14;
}

