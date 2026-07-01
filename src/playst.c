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

/* Bulk-installs four caller-supplied (marker, callback) FSM slots onto an
 * entity -- tick (+0x00), event (+0x08), +0x104, and render (+0x1C) -- each
 * marshalled through a shared local scratch, then swaps the sprite. Used to
 * wire up a player-helper entity's callbacks in one shot.
 *
 * SHELVED (single-instruction scheduling diff): frame (0x28), the sp+4 scratch
 * hole (padded slot struct -> scratch at sp+0x14), and all four callback-pair
 * struct copies match byte-for-byte. The only diff: TARGET loads the last
 * (stack) arg `spriteId` into $a1 at instruction 2 -- reusing $a1 right after
 * spilling its original value -- and holds it across the whole body, while cc1
 * loads it just before the SetEntitySpriteId call. A `u32 sid = spriteId;`
 * hoist local did not move it (prologue reg-alloc choice, not source-reachable),
 * and the permuter (-j4, 150s) floored at 70. Equivalent C (parked in
 * nonmatchings/PlayerSetupCallbacksAndSprite):
 *   typedef struct { s32 marker; void *fn; } SetupSlot;      // 4 by-value pairs
 *   typedef struct { s32 pad; SetupSlot s; } PaddedSetupSlot; // sp+4 hole
 *   PaddedSetupSlot u;
 *   u.s = tick;  *(SetupSlot*)&e->tickMarker  = u.s;   // then event@0x8,
 *   ...          *(SetupSlot*)((u8*)e+0x104)  = u.s;   // slot104, render@0x1C
 *   SetEntitySpriteId(e, spriteId, 1); */
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

/* PlayerState_IdleRandom: unit spans 0x8005BAD0..0x8005BB80 — absorbs former split symbols PlayerState_SimpleTick (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerState_IdleRandom);


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
    u16 *ctr = (u16 *)&e->respawnTimer;
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

/* PlayerEvent_ZoneTriggerHandlerAlt: unit spans 0x8005BF5C..0x8005C0A0 — absorbs former split symbols PlayerState_SetAndProcessQueue (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerAlt);


/* PlayerEvent_ZoneTriggerHandlerV2: unit spans 0x8005C0A0..0x8005C1C4 — absorbs former split symbols PlayerCallback_SetStateAndProcessQueue (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerEvent_ZoneTriggerHandlerV2);



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

/* PlayerCallback_ThrowEventHandler: unit spans 0x8005DAA4..0x8005DDE0 — absorbs former split symbols PlayerCallback_ThrowSetStateAndSpawn (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ThrowEventHandler);



/* PlayerCallback_CheckpointEventHandler: unit spans 0x8005DDE0..0x8005E170 — absorbs former split symbols PlayerCallback_CheckpointSaveAndSpawnHUD (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CheckpointEventHandler);



/* PlayerCallback_ProjectileEventHandler: unit spans 0x8005E170..0x8005E450 — absorbs former split symbols PlayerCallback_ProjectileSetStateAndSpawn (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProjectileEventHandler);



/* PlayerCallback_BaseEventHandler: unit spans 0x8005E450..0x8005E574 — absorbs former split symbols PlayerCallback_SetStateWithQueueProcess (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_BaseEventHandler);



/* PlayerCallback_DeathAnimEventHandler: unit spans 0x8005E574..0x8005F29C — absorbs former split symbols PlayerCallback_DeathAnimSetStateHandler, PlayerCallback_DeathSpawnMenuEntities, func_8005EAAC, PlayerCallback_DeathSpawnMenuEntity, func_8005ECEC, PlayerCallback_AllocAndInitScrollLayer, func_8005EF68, PlayerCallback_InitScrollLayerAndAdd, func_8005F0DC, PlayerCallback_SpawnTrailParticle (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DeathAnimEventHandler);












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

/* PlayerCallback_JumpInputAndCounters: unit spans 0x800602E0..0x800605D8 — absorbs former split symbols PlayerCallback_JumpDirectionAndCounters (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpInputAndCounters);


/* PlayerCallback_JumpTickHandler: unit spans 0x800605D8..0x80060890 — absorbs former split symbols PlayerCallback_SetStateAndUpdateCounters, PlayerCallback_JumpCounterUpdate (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_JumpTickHandler);



INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_FallInputHandler);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CrouchClimbInputHandler);




/* PlayerCallback_CrouchClimbTickHandler: unit spans 0x80060C9C..0x80060E24 — absorbs former split symbols PlayerCallback_SetStatePassthrough (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_CrouchClimbTickHandler);




/* PlayerCallback_DebugCameraInput: unit spans 0x80060E24..0x80061180 — absorbs former split symbols PlayerCallback_DebugCameraVertical, PlayerCallback_DebugCameraUpdate (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_DebugCameraInput);



INCLUDE_ASM("asm/nonmatchings/playst", ApplyEntityPositionUpdate);

/* PlayerCallback_HorizontalWallCollision: unit spans 0x8006120C..0x8006187C — absorbs former split symbols PlayerCallback_ProcessBounceWrapper3, func_80061774, PlayerCallback_SetStatePassthrough2 (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_HorizontalWallCollision);




INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_VerticalCollisionCheck);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_RidingPlatformPhysics);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ProcessBounceWrapper1);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerRidingPlatformUpdate);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ScaleBoundsWrapper);

INCLUDE_ASM("asm/nonmatchings/playst", PlayerCallback_ScaleBoundsAndCollision);





