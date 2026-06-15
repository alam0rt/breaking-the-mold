#include "common.h"
#include "functions.h"
#include "globals.h"

extern s32 PlayerEntityEventHandler(void *e, u32 event);
extern s32 PlayerEntityEventHandlerAlt(void *e, u32 event);
extern void RemoveEntityFromAllLists(void *gs, void *entity);
extern void PlayEntityPositionSound(Entity *e, u32 soundId);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessBounceCollision);

void PlayerClearSwirlPortalEntity(void *e) {
    *(u8 *)((u8 *)g_pGameState + 0x62) = 0;
    if (*(void **)((u8 *)e + 0x140) != NULL) {
        RemoveEntityFromAllLists(g_pGameState, *(void **)((u8 *)e + 0x140));
        *(void **)((u8 *)e + 0x140) = NULL;
    }
}

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

void PlayerState_FrameCountTick(void *e) {
    *(u16 *)((u8 *)e + 0x156) += 1;
    PlayerTickCallback(e);
}

void PlayerState_CooldownTick(void *e) {
    u8 *p = (u8 *)e;
    if (p[0x13C] != 0) {
        p[0x13C]--;
    }
    PlayerTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_FallWithRotation);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_TransitionToPickup);

void PlayerState_QueuedCallbackTimer(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x166);
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(e);
        }
    }
    PlayerTickCallback(e);
}

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

/* Plays the directional-walk SFX (asset 0x64221E61) if any of the four
 * cardinal D-pad bits are pressed in the player's current input snapshot
 * (e->input->u2 — bit 1 = left, 2 = down, 4 = right, 8 = up). The
 * compiler short-circuits the four bit tests in the rather odd
 * 4 → 1 → 8 → 2 order seen in the original — preserved here for byte-match. */
void PlayerPlaySoundOnDirectionInput(Entity *e) {
    void *p = *(void **)((u8 *)e + 0x100);
    u16 flags = *(u16 *)((u8 *)p + 2);
    s32 a1 = 0;
    if (flags & 4) goto fire;
    if (flags & 1) goto fire;
    if (flags & 8) goto fire;
    if (flags & 2) goto fire;
    goto check;
fire:
    a1 = 1;
check:
    if ((u8)a1) {
        PlayEntityPositionSound(e, 0x64221E61);
    }
}

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

