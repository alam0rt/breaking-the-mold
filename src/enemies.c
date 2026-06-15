#include "common.h"

extern void *g_pBlbHeapBase;
extern void FreeFromHeap(void *heap, void *ptr, s32 a2, s32 a3);
extern void EntityProcessCallbackQueue(void *entity);
extern void EntityUpdateCallback(void *entity);
extern void CheckAndDisableSpawnDataOffscreen(void *entity);
extern void DestroyEntityAndFreeMemory(void *entity, s32 mode);
extern void CollisionCheckWrapper(void *e, s32 a, s32 b, s32 c);
extern void CheckAndDisableChildEntityOffscreen(void *e);
extern void CheckAndDisableSpawnRefOffscreen(void *e);
extern void EntityOffScreenChildCleanup(void *e);
extern void UpdateEntitySoundPanning(void *e, u32 sound);
extern void CollectibleTickCallback(void *e);
extern void CollectibleTickFinnMode(void *e);
extern void EntityStateSetWalk(void *e);
extern s32 rand(void);
extern void *D_80010C64;
extern void *D_80010DE4;
extern void *D_80010E04;
extern void *D_80011068;
extern void *D_80011088;
extern void *D_800110A8;
extern void *D_800110E8;
extern void *D_80011108;
extern void *D_80011128;
extern void *D_80011148;
extern void *D_80011168;
extern void *D_80011188;
extern void *D_800111C8;
extern void *D_80011228;
extern void *D_80011248;

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

void EntityStartWalkWithTimer0x2d(void *e) {
    *(u8 *)((u8 *)e + 0x112) = (rand() & 0xF) + 4;
    *(u16 *)((u8 *)e + 0x104) = 0x2D;
    EntityStateSetWalk(e);
}

void EntityStartWalkWithTimer10(void *e) {
    *(u8 *)((u8 *)e + 0x112) = (rand() & 0xF) + 4;
    *(u16 *)((u8 *)e + 0x104) = 0xA;
    EntityStateSetWalk(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetIdle);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetRandomBehavior);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetAttack);

INCLUDE_ASM("asm/nonmatchings/enemies", EntitySetSparkleCollectibleState);

void EntityStateSetSparkle(void *e);

void EntitySetSparkleDelay3(void *e) {
    *(u8 *)((u8 *)e + 0x110) = 3;
    EntityStateSetSparkle(e);
}

void EntitySetSparkleDelay2(void *e) {
    *(u8 *)((u8 *)e + 0x110) = 2;
    EntityStateSetSparkle(e);
}

void EntitySetSparkleDelay1(void *e) {
    *(u8 *)((u8 *)e + 0x110) = 1;
    EntityStateSetSparkle(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetSparkle);

extern u8 D_8009B55C[];
extern u8 D_8009B57C[];
extern u8 D_8009B59C[];

void StartAnimSequence4A(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B55C, 4);
}

void StartAnimSequence4B(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B57C, 4);
}

void StartAnimSequence4C(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B59C, 4);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroySoundEmitterEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterStunnedTickCallback);

s32 EntitySimpleEventPassthrough(void *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

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

extern u8 D_8009B5BC[];
extern u8 D_8009B5D4[];
extern u8 D_8009B5EC[];

void StartAnimSequence3A(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B5BC, 3);
}

void StartAnimSequence3B(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B5D4, 3);
}

void StartAnimSequence9Frames(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B5EC, 9);
}

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

void CollectibleTickWithSoundPanning(void *e) {
    UpdateEntitySoundPanning(e, *(u32 *)((u8 *)e + 0x118));
    CollectibleTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnMultipleProjectiles);

void EntityDestroyCallback_Vt80010C64(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010C64;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800410bc(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_80041120(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010E04(void *entity, u32 flag) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010E04;
    if (flag & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", func_800411CC);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityFromHeapContext);

void EntityDestroyCallback_Vt80010DE4_80041230(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_80041294(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800412f8(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_8004135c(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800413c0(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80010DE4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80041424(void) {
}

void NopStub_8004142c(void) {
}

void EntityDestroyCallback_Vt80010E04_80041434(void *entity, u32 flag) {
    *(void **)((u8 *)entity + 0x18) = &D_80010E04;
    if (flag & 1) {
        FreeEntityNoTeardown_80041468(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80041468(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitCheckpointEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroyEntityWithChildRemoval);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFloatingWithCollisionTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntity_Alt);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityTimedStateSwitchTick);

void EntityUpdateWithCollisionOffscreen(void *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, 0x1000, 1);
    CheckAndDisableChildEntityOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableChildEntityOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityConditionalActivateTick);

void EntityUpdateWithSpawnDataCheck(void *entity) {
    EntityUpdateCallback(entity);
    CheckAndDisableSpawnDataOffscreen(entity);
}

void EntityUpdateWithCollisionSpawnCheck(void *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, 0x1000, 1);
    CheckAndDisableSpawnDataOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnDataOffscreen);

s32 EntitySimpleEventPassthrough_V2(void *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventTimerCountdownWithGameState);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityHideAndDisable);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithTypeBasedTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPlatformEntityState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitDirectionalScaledEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformTimerTickCallback);

void PlatformCollisionTickCallback(void *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, 0x1000, 1);
    CheckAndDisableSpawnRefOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnRefOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformEventHandlerSpawnEffect);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformHideAndDisable);

INCLUDE_ASM("asm/nonmatchings/enemies", PlatformShowAndActivate);

INCLUDE_ASM("asm/nonmatchings/enemies", InitBouncableClayEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityOffScreenChildCleanup);

void EntityTick_CollisionWithCleanup(void *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, 0x1000, 2);
    EntityOffScreenChildCleanup(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", HazardTimerTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardEventHandler_0x1001);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardEventHandler_0x1001_V2);

s32 EntityEventPassthrough_Event2(void *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

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

s32 TeleporterEventPassthrough_Event2(void *entity, u32 event) {
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

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

void EntityDestroyCallback_Vt80011068(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011068;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011088(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011088;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800110A8(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_800110A8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityDestroyCallback_Vt800111C8);

void EntityDestroyCallback_Vt800110E8(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_800110E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011108(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011108;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011128(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011128;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011148(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011148;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011168(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011168;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011188(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011188;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80045e70(void) {
}

void NopStub_80045e78(void) {
}

void EntityDestroyCallback_Vt800111C8_80045e80(void *entity, u32 flag) {
    *(void **)((u8 *)entity + 0x18) = &D_800111C8;
    if (flag & 1) {
        FreeEntityNoTeardown_80045eb4(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80045eb4(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitFloatingPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleScaledTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWalkWithTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityGroundSnapMovementCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemSparkleIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemRevealState);

extern u8 D_8009B7A4[];
extern u8 D_8009B7DC[];

void StartAnimationSequence(u8 *entity, s32 animData, s16 startFrame);

void SetEntitySpecialState_1(void *e) {
    *(u8 *)((u8 *)e + 0x110) = 1;
    EntityStateSetSpecial(e);
}

void SetEntitySpecialState_2(void *e) {
    *(u8 *)((u8 *)e + 0x110) = 2;
    EntityStateSetSpecial(e);
}

void SetEntitySpecialState_3(void *e) {
    *(u8 *)((u8 *)e + 0x110) = 3;
    EntityStateSetSpecial(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityStateSetSpecial);

void StartAnimSequence7Frames(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B7A4, 7);
}

void StartAnimSequence13Frames(void *e) {
    StartAnimationSequence((u8 *)e, (s32)D_8009B7DC, 13);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEntity_Alt);

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEntityDestroyCallback);

void FinnModeCollectibleTickCallback(void *e) {
    UpdateEntitySoundPanning(e, *(u32 *)((u8 *)e + 0x118));
    CollectibleTickFinnMode(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyPathFollowWithFacingFlip);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetWalkSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetIdleSprite);

void EnemyDestroyCallback_0x80011228(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011228;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011228(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011228;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80046ce4(void) {
}

void NopStub_80046cec(void) {
}

void EntityDestroyCallback_Vt80011248(void *entity, u32 flag) {
    *(void **)((u8 *)entity + 0x18) = &D_80011248;
    if (flag & 1) {
        FreeEntityNoTeardown_80046d28(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80046d28(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

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

