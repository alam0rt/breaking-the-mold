#include "common.h"
#include "functions.h"
#include "globals.h"

extern s32 PlayerEntityEventHandler(PlayerEntity *e, u32 event, u32 arg2, u32 arg3);
extern s32 PlayerEntityEventHandlerAlt(PlayerEntity *e, u32 event, u32 arg2, u32 arg3);
extern s32 PlayerEntityCollisionHandler(PlayerEntity *e, u32 event, u32 arg2, u32 arg3);
extern void RemoveEntityFromAllLists(GameState *gs, Entity *entity);
extern void PlayEntityPositionSound(Entity *e, u32 soundId);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerProcessBounceCollision);

void PlayerClearSwirlPortalEntity(PlayerEntity *e) {
    g_pGameState->camera_follow_direction = 0;
    if (e->swirlPortalEntity != NULL) {
        RemoveEntityFromAllLists(g_pGameState, e->swirlPortalEntity);
        e->swirlPortalEntity = NULL;
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

void ApplyRandomRGBEffect(PlayerEntity *e) {
    e->baseRGB[0] = rand();
    e->baseRGB[1] = rand();
    e->baseRGB[2] = rand();
    e->damageFlag = 1;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerTickCallback);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_IdleRandom);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SimpleTick);

void PlayerState_FrameCountTick(PlayerEntity *e) {
    e->jumpParam += 1;
    PlayerTickCallback(e);
}

/* Bounce-cooldown tick installed by the bounce-on-enemy collision path
 * (PlayerCallback_CollisionHandlerWithQueue @ 0x8005CEC8). Per-frame
 * decrement of the cooldown timer at +0x13C, then tail-call into
 * PlayerTickCallback for animation + state-transition. NOTE: this
 * function does NOT itself check timer13C == 0 — the state-exit
 * transition happens inside PlayerTickCallback (which the trace
 * confirms is running every frame regardless of the wrapper). The
 * struct_watch trace from 2026-06-21 captured this tick running for
 * 5319+ consecutive frames during the Shriney Guard fight while
 * pFrameTable pointed at 0x801388E0 (bounce-cooldown anim). */
void PlayerState_CooldownTick(PlayerEntity *e) {
    if (e->timer13C != 0) {
        e->timer13C--;
    }
    PlayerTickCallback(e);
}

/* Fall-with-rotation tick: gradually decays the player's vertical velocity
 * (entity+0x50/+0x54) by 0xC00 per frame while still nonzero, and rotates
 * the visual angle (entity+0x72) by +0x20 per frame, wrapping at 0x1000.
 * Tail-calls PlayerTickCallback for the standard sprite/animation tick. */
void PlayerState_FallWithRotation(Entity *e) {
    s32 vy = e->scaleRender;
    if (vy != 0) {
        vy -= 0xC00;
        e->scaleRender = vy;
        e->scaleRender2 = vy;
    }
    {
        u16 angle = e->targetY;
        u16 new_angle = angle + 0x20;
        e->targetY = new_angle;
        if (new_angle >= 0x1000) {
            e->targetY = angle - 0xFE0;
        }
    }
    PlayerTickCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_TransitionToPickup);

void PlayerState_QueuedCallbackTimer(PlayerEntity *e) {
    u16 *ctr = (u16 *)&e->_pad164[2];
    if (*ctr != 0) {
        *ctr -= 1;
        if (*ctr == 0) {
            EntityProcessCallbackQueue(&e->sprite.base);
        }
    }
    PlayerTickCallback(&e->sprite.base);
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_UpdateAttachedEntity);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerAlt);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_SetAndProcessQueue);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerV2);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateAndProcessQueue);


INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityCollisionHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerEntityEventHandlerAlt);

s32 PlayerCallback_EventHandlerWithQueue(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlayerEntityEventHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_AnimFrameParticleHandler);

s32 PlayerCallback_CollisionHandlerWithQueue(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlayerEntityCollisionHandler(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerBounceCallback);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_TeleporterAnimHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BounceEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathDebrisSpawner);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ThrowEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ThrowSetStateAndSpawn);


INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CheckpointEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CheckpointSaveAndSpawnHUD);


INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProjectileEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProjectileSetStateAndSpawn);


INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BaseEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_SetStateWithQueueProcess);


INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathAnimEventHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathAnimSetStateHandler);


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

s32 PlayerCallback_EventHandlerAltWithQueue(PlayerEntity *e, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlayerEntityEventHandlerAlt(e, maskedEvent, arg2, arg3);
    if (maskedEvent == 2) {
        EntityProcessCallbackQueue((Entity *)e);
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CollisionDamageSetup);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkToRunTransition);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CollisionTrailEntityUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", TryActivatePowerup);

/* Plays the directional-walk SFX (asset 0x64221E61) if any of the four
 * cardinal D-pad bits are pressed in the player's current input snapshot
 * (e->input->u2 — bit 1 = left, 2 = down, 4 = right, 8 = up). The
 * compiler short-circuits the four bit tests in the rather odd
 * 4 → 1 → 8 → 2 order seen in the original — preserved here for byte-match. */
void PlayerPlaySoundOnDirectionInput(PlayerEntity *e) {
    InputState *input = e->pInput;
    u16 flags = input->buttons_pressed;
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
        PlayEntityPositionSound(&e->sprite.base, 0x64221E61);
    }
}

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_IdleInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_WalkInputHandler);

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

