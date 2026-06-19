#include "common.h"
#include "functions.h"
#include "Game/entity_events.h"
#include "Game/callback_slot.h"
#include "globals.h"

extern void *g_pBlbHeapBase;
extern void CheckAndDisableSpawnDataOffscreen(Entity *entity);
extern void CollisionCheckWrapper(Entity *e, s32 a, s32 b, s32 c);
extern void CheckAndDisableChildEntityOffscreen(Entity *e);
extern void CheckAndDisableSpawnRefOffscreen(Entity *e);
extern void EntityOffScreenChildCleanup(Entity *e);
extern void UpdateEntitySoundPanning(Entity *e, u32 sound);
extern void CollectibleTickCallback(Entity *e);
extern void CollectibleTickFinnMode(Entity *e);
extern void EntityStateSetWalk(Entity *e);
extern void SetAnimationSpriteId(Entity *e, s32 id);
extern void SetAnimationFrameCallback(Entity *e, u32 packed);
extern Entity *InitEntityWithSprite(Entity *entity, void *spriteDef, s32 z, s16 x, s16 y);
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);
extern Entity *InitCollectibleEntity(Entity *e, void *spawn);
extern void *PROJECTILE_PATH_ENTITY_VTABLE asm("D_80010C64");
extern void *PATH_HAZARD_ENTITY_VTABLE asm("D_80010CE4");
extern void *PATH_ENEMY_SOUND_VTABLE asm("D_80010DA4");
extern void *COLLECTIBLE_ENTITY_VTABLE asm("D_80010DE4");
extern void *SIMPLE_FREE_ONLY_ENTITY_VTABLE asm("D_80010E04");
extern void *CAMERA_TRACKING_ENTITY_VTABLE asm("D_80011048");
extern void *INDEXED_SPRITE_ENTITY_VTABLE asm("D_80011068");
extern void *CAMERA_HELPER_ENTITY_VTABLE asm("D_80011088");
extern void *TRIGGER_ZONE_ENTITY_VTABLE asm("D_800110A8");
extern void *SWITCH_BLOCK_ENTITY_VTABLE asm("D_800110E8");
extern void *SCALED_MOVING_ENTITY_VTABLE asm("D_80011108");
extern void *BOUNCABLE_CLAY_ENTITY_VTABLE asm("D_80011128");
extern void *DIRECTIONAL_SCALED_ENTITY_VTABLE asm("D_80011148");
extern void *SCALED_PLATFORM_ENTITY_VTABLE asm("D_80011168");
extern void *ALT_COLLECTIBLE_ENTITY_VTABLE asm("D_80011188");
extern void *CHECKPOINT_ENTITY_VTABLE asm("D_800111A8");
extern void *ENTITY_FREE_ONLY_VTABLE asm("D_800111C8");
extern void *ALT_PATH_FOLLOWING_ENTITY_VTABLE asm("D_800111E8");
extern void *ENEMY_ENTITY_VTABLE asm("D_80011228");
extern void *ENEMY_FREE_ONLY_ENTITY_VTABLE asm("D_80011248");
extern void RemoveEntityFromAllLists(GameState *gs, s32 idx);
extern void EntitySetState(Entity *e, u32 marker, void *fn);
void FreeEntityNoTeardown_80041468(Entity *e, u32 size);
void EntityUpdateWithCollisionWrapper(Entity *e);
void FreeEntityNoTeardown_80045eb4(Entity *e, u32 size);
void FreeEntityNoTeardown_80046d28(Entity *e, u32 size);
void StartAnimationSequence(u8 *entity, s32 animData, s16 startFrame);

/* gp_rel tentative defs (resolved via the .sdata blob's strong defs). */
u32   SOUND_EMITTER_STUN_EXPIRED_STATE_MARKER asm("D_800A5A68");
void *SOUND_EMITTER_STUN_EXPIRED_STATE_CALLBACK asm("D_800A5A6C");
u32   PLATFORM_ACTIVATE_BY_FRAME_STATE_MARKER asm("D_800A5AE8");
void *PLATFORM_ACTIVATE_BY_FRAME_STATE_CALLBACK asm("D_800A5AEC");
u32   HAZARD_TIMER_EXPIRED_STATE_MARKER asm("D_800A5B08");
void *HAZARD_TIMER_EXPIRED_STATE_CALLBACK asm("D_800A5B0C");
u32   HAZARD_READY_STATE_MARKER asm("D_800A5B10");
void *HAZARD_READY_STATE_CALLBACK asm("D_800A5B14");

INCLUDE_ASM("asm/nonmatchings/enemies", LineSegmentIntersectsRect);

void *InitCollectibleEntityFromSpawn(void *e, void *spawn, u32 spriteId) {
    InitEntitySprite(e, spriteId, 0x3CA,
                     *(s16 *)((u8 *)spawn + 0x8),
                     (s16)(*(u16 *)((u8 *)spawn + 0xA) - 1), 0);
    ((Entity *)e)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, spawn);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleFromSpawn);

void *InitCollectibleEntityDirect(void *e, u32 spriteId, s16 x, s16 y) {
    InitEntitySprite(e, spriteId, 0x3CA, x, y, 0);
    ((Entity *)e)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, NULL);
    return e;
}

void *CreateCollectibleAtPosition(void *e, void *spriteDef, s16 x, s16 y) {
    InitEntityWithSprite(e, spriteDef, 0x3CA, x, y);
    ((Entity *)e)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    InitCollectibleEntity(e, NULL);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CalcEntityYFromTileHeight);

INCLUDE_ASM("asm/nonmatchings/enemies", UpdateCollectibleTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", CheckCollectibleOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickCallback);

void TimedCollectibleTickCallback(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x104);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    CollectibleTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleTickFinnMode);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerIdle);

INCLUDE_ASM("asm/nonmatchings/enemies", SpawnCollectibleParticles);

INCLUDE_ASM("asm/nonmatchings/enemies", CreateCollectibleWithFlags);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleSparkleTickCallback);

void TimedSparkleCollectibleTick(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x104);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    CollectibleSparkleTickCallback(e);
}

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

extern u8 ENEMY_ANIM_SEQUENCE_4A_DATA[] asm("D_8009B55C");
extern u8 ENEMY_ANIM_SEQUENCE_4B_DATA[] asm("D_8009B57C");
extern u8 ENEMY_ANIM_SEQUENCE_4C_DATA[] asm("D_8009B59C");

void StartAnimSequence4A(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ENEMY_ANIM_SEQUENCE_4A_DATA, 4);
}

void StartAnimSequence4B(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ENEMY_ANIM_SEQUENCE_4B_DATA, 4);
}

void StartAnimSequence4C(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ENEMY_ANIM_SEQUENCE_4C_DATA, 4);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEnemy);

void DestroySoundEmitterEntity(void *e, u32 flags) {
    ((Entity *)e)->collisionVtable = &PATH_ENEMY_SOUND_VTABLE;
    StopSPUVoice(*(s32 *)((u8 *)e + 0x128));
    ((Entity *)e)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterTickCallback);

void SoundEmitterStunnedTickCallback(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x124);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntitySetState(e, SOUND_EMITTER_STUN_EXPIRED_STATE_MARKER,
                           SOUND_EMITTER_STUN_EXPIRED_STATE_CALLBACK);
        }
    }
    EntityUpdateCallback(e);
    CheckCollectibleOffscreen(e);
}

s32 EntitySimpleEventPassthrough(void *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
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

void TripleLaserMonkeyConditionalTick(Entity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == *(u8 *)((u8 *)e + 0x110)) {
        EntityProcessCallbackQueue(e);
    }
    TripleLaserMonkeyDeathTick(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWithCountdownToWalk);

INCLUDE_ASM("asm/nonmatchings/enemies", LaserMonkeyWalkState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTripleLaserMonkeyAttackState);

INCLUDE_ASM("asm/nonmatchings/enemies", LaserMonkeyIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitWalkingCollectibleEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyDeathWithParticles);

void EntityTimerDeathWithParticles(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x104);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    EnemyDeathWithParticles(e);
}

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

extern u8 ENEMY_ANIM_SEQUENCE_3A_DATA[] asm("D_8009B5BC");
extern u8 ENEMY_ANIM_SEQUENCE_3B_DATA[] asm("D_8009B5D4");
extern u8 ENEMY_ANIM_SEQUENCE_9_FRAME_DATA[] asm("D_8009B5EC");

void StartAnimSequence3A(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ENEMY_ANIM_SEQUENCE_3A_DATA, 3);
}

void StartAnimSequence3B(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ENEMY_ANIM_SEQUENCE_3B_DATA, 3);
}

void StartAnimSequence9Frames(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ENEMY_ANIM_SEQUENCE_9_FRAME_DATA, 9);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitSoundEmittingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerSpawnWalkingEnemy);

INCLUDE_ASM("asm/nonmatchings/enemies", AnimatedEntityToggleSpriteA);

INCLUDE_ASM("asm/nonmatchings/enemies", AnimatedEntityToggleSpriteB);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetLoopingAnimation);

INCLUDE_ASM("asm/nonmatchings/enemies", InitTimerBasedMenuEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySpawnerTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingHazard);

void DestroySoundEntityWithVoice(void *e, u32 flags) {
    ((Entity *)e)->collisionVtable = &PATH_HAZARD_ENTITY_VTABLE;
    StopSPUVoice(*(s32 *)((u8 *)e + 0x110));
    DestroyEntityAndFreeMemory(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterIdleTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityPathFollowerWithTriggerZone);

INCLUDE_ASM("asm/nonmatchings/enemies", InitLevelStateCollectible);

void ConditionalCollectibleTick(Entity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == *(u8 *)((u8 *)e + 0x110)) {
        EntityProcessCallbackQueue(e);
    }
    CollectibleTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerCountdownToWalkWithSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleWalkState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitConditionalCollectibleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitAnimatedTimedCollectible);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleFrameConditionalTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleVariant);

INCLUDE_ASM("asm/nonmatchings/enemies", InitEntityWithDeathSpawn);

void EntityTimerCountdownDeathSpawn(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x112);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    InitEntityWithDeathSpawn(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerAnimationSwitch);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerTimerCountdown);

void InitEntityRandomIdleOrAnimated(Entity *e) {
    if (rand() & 1) {
        InitEntityState_Animated(e);
    } else {
        InitEntityState_Idle(e);
    }
}

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
    ((Entity *)entity)->collisionVtable = &PROJECTILE_PATH_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800410bc(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_80041120(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010E04(void *entity, u32 flag) {
    ((Entity *)entity)->collisionVtable = &SIMPLE_FREE_ONLY_ENTITY_VTABLE;
    if (flag & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", func_800411CC);

INCLUDE_ASM("asm/nonmatchings/enemies", FreeEntityFromHeapContext);

void EntityDestroyCallback_Vt80010DE4_80041230(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_80041294(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800412f8(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_8004135c(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010DE4_800413c0(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &COLLECTIBLE_ENTITY_VTABLE;
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
    ((Entity *)entity)->collisionVtable = &SIMPLE_FREE_ONLY_ENTITY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80041468(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80041468(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitCheckpointEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", DestroyEntityWithChildRemoval);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityFloatingWithCollisionTick);

INCLUDE_ASM("asm/nonmatchings/enemies", InitCollectibleEntity_Alt);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityTimedStateSwitchTick);

void EntityUpdateWithCollisionOffscreen(void *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
    CheckAndDisableChildEntityOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableChildEntityOffscreen);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledPlatformEntity);

void EntityConditionalActivateTick(Entity *e) {
    if ((g_pGameState->frame_counter & 0x7F) == *(s32 *)(*(u8 **)((u8 *)e + 0x108) + 0xC)) {
        EntitySetState(e, PLATFORM_ACTIVATE_BY_FRAME_STATE_MARKER,
                       PLATFORM_ACTIVATE_BY_FRAME_STATE_CALLBACK);
    }
    EntityUpdateCallback(e);
    CheckAndDisableSpawnDataOffscreen(e);
}

void EntityUpdateWithSpawnDataCheck(void *entity) {
    EntityUpdateCallback(entity);
    CheckAndDisableSpawnDataOffscreen(entity);
}

void EntityUpdateWithCollisionSpawnCheck(void *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
    CheckAndDisableSpawnDataOffscreen(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", CheckAndDisableSpawnDataOffscreen);

s32 EntitySimpleEventPassthrough_V2(void *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
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
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
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
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 2);
    EntityOffScreenChildCleanup(e);
}

void HazardTimerTickCallback(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x110);
    EntityUpdateCallback(e);
    EntityOffScreenChildCleanup(e);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntitySetState(e, HAZARD_TIMER_EXPIRED_STATE_MARKER,
                           HAZARD_TIMER_EXPIRED_STATE_CALLBACK);
        }
    }
}

s32 HazardEventHandler_0x1001(void *e, u32 ev, u32 a2, u32 a3) {
    if ((ev & 0xFFFF) == EVT_SET_READY) {
        EntitySetState(e, HAZARD_READY_STATE_MARKER,
                       HAZARD_READY_STATE_CALLBACK);
    }
    return 0;
}

s32 HazardEventHandler_0x1001_V2(void *e, u32 ev, u32 a2, u32 a3) {
    u32 maskedEv = ev & 0xFFFF;
    if (maskedEv == EVT_SET_READY) {
        EntitySetState(e, HAZARD_READY_STATE_MARKER,
                       HAZARD_READY_STATE_CALLBACK);
    }
    if (maskedEv == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return 0;
}

s32 EntityEventPassthrough_Event2(void *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

extern void InterpolateTimedPathPosition(void *time, s16 *out, void *pathData, s16 duration, s32 unused);

void EntityPathMovementUpdate(TimedPathEntity *e) {
    s16 out[2];
    e->pathTime += 1;
    InterpolateTimedPathPosition(
        &e->pathTime,
        out,
        e->pathData,
        e->pathDuration,
        8
    );
    e->sprite.base.worldX = e->pathOriginX + (u16)out[0];
    e->sprite.base.worldY = e->pathOriginY + (u16)out[1];
}

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_IdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_DestroyingState);

INCLUDE_ASM("asm/nonmatchings/enemies", HazardActiveState);

INCLUDE_ASM("asm/nonmatchings/enemies", BounceClay_HiddenState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitScaledMovingEntity);

void TimedEntityTickCallback(Entity *e) {
    u8 *ctr = (u8 *)e + 0x104;
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    EntityUpdateCallback(e);
}

void TimedEntityTickCallbackWithCollision(Entity *e) {
    u8 *ctr = (u8 *)e + 0x104;
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    EntityUpdateWithCollisionWrapper(e);
}

void EntityUpdateWithCollisionWrapper(Entity *e) {
    EntityUpdateCallback(e);
    CollisionCheckWrapper(e, 2, EVT_DAMAGE, 1);
}

s32 SwitchEventHandler_SetGameFlag(u8 *e, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        u8 *p = *(u8 **)(e + 0x100);
        u16 val = *(u16 *)(p + 0x12);
        if ((u16)(val - 0x22) < 2 || val == 0x24) {
            *(u8 *)&g_pGameState->screen_shake_index = 0x14;
        }
    }
    return 0;
}

void EntityIncrementWorldX(Entity *e) {
    e->worldX += 1;
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
    if ((event & 0xFFFF) == EVT_TICK) {
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

void SoundEmitterDestroyCallback(void *e, u32 flags) {
    ((Entity *)e)->collisionVtable = &CAMERA_TRACKING_ENTITY_VTABLE;
    StopSPUVoice(*(s32 *)((u8 *)e + 0x20));
    ((Entity *)e)->collisionVtable = &ENTITY_FREE_ONLY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/enemies", SoundEmitterWithPanningTick);

void EntityDestroyCallback_Vt80011068(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &INDEXED_SPRITE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011088(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &CAMERA_HELPER_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800110A8(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &TRIGGER_ZONE_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800111C8(void *e, u32 flags) {
    ((Entity *)e)->collisionVtable = &ENTITY_FREE_ONLY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

void EntityDestroyCallback_Vt800110E8(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &SWITCH_BLOCK_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011108(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &SCALED_MOVING_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011128(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &BOUNCABLE_CLAY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011148(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &DIRECTIONAL_SCALED_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011168(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &SCALED_PLATFORM_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011188(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &ALT_COLLECTIBLE_ENTITY_VTABLE;
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
    ((Entity *)entity)->collisionVtable = &ENTITY_FREE_ONLY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80045eb4(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80045eb4(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitFloatingPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", CollectibleScaledTickCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityEventHandlerWalkWithTimer);

INCLUDE_ASM("asm/nonmatchings/enemies", EntityGroundSnapMovementCallback);

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemSparkleIdleState);

INCLUDE_ASM("asm/nonmatchings/enemies", InitItemRevealState);

extern u8 ITEM_REVEAL_ANIM_SEQUENCE_7_FRAME_DATA[] asm("D_8009B7A4");
extern u8 ITEM_REVEAL_ANIM_SEQUENCE_13_FRAME_DATA[] asm("D_8009B7DC");

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
    StartAnimationSequence((u8 *)e, (s32)ITEM_REVEAL_ANIM_SEQUENCE_7_FRAME_DATA, 7);
}

void StartAnimSequence13Frames(void *e) {
    StartAnimationSequence((u8 *)e, (s32)ITEM_REVEAL_ANIM_SEQUENCE_13_FRAME_DATA, 13);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitPathFollowingEntity_Alt);

void SoundEntityDestroyCallback(void *e, u32 flags) {
    ((Entity *)e)->collisionVtable = &ALT_PATH_FOLLOWING_ENTITY_VTABLE;
    StopSPUVoice(*(s32 *)((u8 *)e + 0x118));
    ((Entity *)e)->collisionVtable = &ENEMY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

void FinnModeCollectibleTickCallback(void *e) {
    UpdateEntitySoundPanning(e, *(u32 *)((u8 *)e + 0x118));
    CollectibleTickFinnMode(e);
}

INCLUDE_ASM("asm/nonmatchings/enemies", EnemyPathFollowWithFacingFlip);

extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flags);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetWalkSprite);

INCLUDE_ASM("asm/nonmatchings/enemies", EnemySetIdleSprite);

void EnemyDestroyCallback_0x80011228(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &ENEMY_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80011228(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &ENEMY_ENTITY_VTABLE;
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
    ((Entity *)entity)->collisionVtable = &ENEMY_FREE_ONLY_ENTITY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80046d28(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80046d28(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundParticleEmitter);

INCLUDE_ASM("asm/nonmatchings/enemies", ParticleEmitterTickCallback);

s32 EntitySetReadyFlag(void *e, u16 mode) {
    if (mode == EVT_TICK) {
        *(u8 *)((u8 *)e + 0x100) = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/enemies", InitBackgroundSparkleEntity);

INCLUDE_ASM("asm/nonmatchings/enemies", BackgroundSparkleFadeTickCallback);

s32 BackgroundSparkleContactEventHandler(Entity *e, u32 ev) {
    if ((ev & 0xFFFF) == EVT_TICK) {
        if (*(u8 *)((u8 *)e + 0x100) != 0) {
            *(u8 *)((u8 *)(*(void **)((u8 *)e + 0x34)) + 0xA) = 0;
            SetAnimationSpriteId(e, *(s16 *)((u8 *)e + 0xDA));
            EntitySetRenderFlags(e, 0);
        }
    }
    return 0;
}

void InitBackgroundSparkleRevealState(void *e) {
    *(u8 *)((u8 *)e + 0x100) = 1;
    SetAnimationSpriteId(e, -1);
    SetAnimationFrameCallback(e, 0x2421405);
}

void SetEntityAnimationState(Entity *e) {
    *(u8 *)((u8 *)(*(void **)((u8 *)e + 0x34)) + 0xA) = 1;
    *(u8 *)((u8 *)e + 0x100) = 0;
    EntitySetRenderFlags(e, 1);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationSpriteCallback(e, 0x2421405);
    SetAnimationFrameIndex(e, 0);
}

void func_8004727C(void *e) {
    *(u8 *)((u8 *)e + 0x101) = 0x14;
}

