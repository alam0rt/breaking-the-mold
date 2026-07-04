#include "common.h"
#include "functions.h"
#include "Game/entity.h"
#include "Game/entity_events.h"

extern s32 PlatformEntityOnCollision(Entity *entity, u32 event, u32 arg2, u32 arg3);

/*
 * CalcEntityYFromTileCollision @ 0x80071048
 *
 * Adjusts entity Y onto the tile slope beneath one of its bbox
 * anchors. Dispatches through the two entity callback tables at
 * +0x26/0x28 and +0x2E/0x30 (count <= 0 -> single fn pointer, count > 0
 * -> table of (fn,arg) pairs indexed by count) and ultimately calls
 * GetSlopeHeightAtSubpixel on the active GameState.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", CalcEntityYFromTileCollision);

/*
 * CheckEntityNearTileY @ 0x80071228
 *
 * Predicate counterpart to CalcEntityYFromTileCollision: returns
 * whether the entity is within a threshold of the slope-tile Y under
 * its bbox anchor points. Same callback-table dispatch shape, also
 * routed through GetSlopeHeightAtSubpixel.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", CheckEntityNearTileY);

/*
 * PlatformTick_MainWithSoundAndTimers @ 0x800713F4
 *
 * Per-frame tick for SOAR/vehicle "platform" players (SoarPlayerEntity
 * field offsets are visible: 0x11E stateReturnTimer, 0x120
 * levelLoadTimer, 0x11B jumpHoldTimer, 0x11C pendingLevelLoadId,
 * 0x11D inputEnabled). Pans the looping voice via
 * UpdateEntitySoundPanning, decrements the three timers (latching
 * pendingLevelLoadId into GameState+0x148 when levelLoadTimer hits 0),
 * runs the bbox callback chain, then dispatches
 * PlatformEntityProcessInput (if inputEnabled),
 * PlatformEntityCheckTriggerZones, and EntityUpdateCallback.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", PlatformTick_MainWithSoundAndTimers);

/*
 * PlatformEntityOnCollision @ 0x80071560
 *
 * Vehicle-player collision event handler. On masked event 0x100F runs
 * the bbox callback chain and stores adjusted X/Y back into worldX
 * (0x68) and worldY (0x6A); on 0x1000 transitions to the death/hit
 * state pair at gp-rel D_800A5FBC/D_800A5FC0 via EntitySetState. Other
 * events fall through and return 0.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", PlatformEntityOnCollision);

/*
 * PlatformEvent_CollisionAndQueue @ 0x800716F0
 *
 * Thin wrapper around PlatformEntityOnCollision. After delegating it
 * also drains the entity's deferred callback queue when the masked
 * event is 2 (post-tick / state-ready notification). Returns the
 * underlying handler's result in both paths.
 */
s32 PlatformEvent_CollisionAndQueue(Entity *entity, u32 event, u32 arg2, u32 arg3) {
    s32 result;
    u32 maskedEvent = event & 0xFFFF;
    result = PlatformEntityOnCollision(entity, maskedEvent, arg2, arg3);
    if (maskedEvent == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
        return result;
    }
    return result;
}

/*
 * PlatformEvent_QueueOnAnimReady @ 0x8007174C
 *
 * Minimal vehicle-state event handler: only drains the entity's pending
 * callback queue when the masked event is 2 (state/animation-ready),
 * and always returns 0. Used by RUNN/SOAR substates whose sole behavior
 * is reacting to that notification. "AnimReady" is a guess - event 2 is
 * the generic post-tick wakeup, not strictly animation.
 */
s32 PlatformEvent_QueueOnAnimReady(Entity *entity, u32 event) {
    if ((event & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

/*
 * FinnAdvanceXByVelocity @ 0x80071778
 *
 * Advances the entity's worldX via its 16.16 fixed-point sub-pixel
 * accumulator: packs worldX:fracX into a 32-bit position, adds the
 * velocity word stored at entity+0x104 (FinnPlayerEntity.velocityX
 * shape), and splits the result back into worldX (upper 16) and fracX
 * (lower 16).
 */
void FinnAdvanceXByVelocity(FinnPlayerEntity *entity) {
    s32 pos = ((s32)entity->sprite.base.worldX << 16) +
              (u16)entity->sprite.base.velocityX +
              (s32)entity->wakeEntity_or_velocityX;
    entity->sprite.base.worldX = pos >> 16;
    entity->sprite.base.velocityX = (s16)pos;
}

/*
 * PlatformEntityTickMoving @ 0x800717A0
 *
 * Vehicle "moving" physics tick: probes ground/wall geometry by calling
 * EntityApplyMovementCallbacks at several Y offsets below the entity
 * (-0xF, -0x10, -0x20, -0x30, -0x40, -0x7, ...). When any probe returns
 * the sentinel material 0xB5 (out-of-bounds / death), transitions via
 * EntitySetState into the death/respawn state pair at gp-rel
 * D_800A5FBC/D_800A5FC0.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", PlatformEntityTickMoving);

/*
 * PlatformSpeedRampTick @ 0x80071A30
 *
 * Speed-ramp wrapper around PlatformEntityTickMoving. Each tick adjusts
 * the velocity accumulator at entity+0x104: while marker at +0x110 is
 * < 0x100, it ramps up by 0x4000/tick capped at 0x80000; otherwise it
 * bleeds down by 0x8000/tick with a 0x50000 floor. Then chains into
 * PlatformEntityTickMoving for the actual movement pass.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", PlatformSpeedRampTick);

/*
 * RunnPhysicsAndTileCollisionTick @ 0x80071AD4
 *
 * RUNN auto-scroller per-frame physics + tile probe pass. Integrates
 * worldX:fracX (0x68:0x6C) with velocityX (+0x104), worldY:fracY
 * (0x6A:0x6E) with velocityY (+0x108, ramped by 0x5000/tick and capped
 * at 0x80000), then probes ground/wall via EntityApplyMovementCallbacks
 * at multiple Y offsets. Branches to hazard, landing, or fall handlers
 * based on the returned material codes. This is the physics half of the
 * RUNN tick; rendering happens elsewhere.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnPhysicsAndTileCollisionTick);

/*
 * RunnState_DieAndAdvanceLevel @ 0x80071D10
 *
 * Tiny tail-call helper: hands the entity off to EntitySetState with
 * the death-and-advance state pair at gp-rel D_800A5FCC/D_800A5FD0,
 * then jumps to RunnState_SelectNextByInput. Reached by fall-through
 * from RunnPhysicsAndTileCollisionTick when a probe returns material
 * 0x5B.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnState_DieAndAdvanceLevel);

/*
 * RunnPhysicsLandingDispatch @ 0x80071D28 (RUNN landing-dispatch tail)
 *
 * Continuation of the RUNN physics path: if the latest probe returned
 * material 0xB5 it routes into the hazard-damage transition; otherwise
 * it walks downward through CheckEntityNearTileY + more
 * EntityApplyMovementCallbacks probes to decide between
 * RunnLand_GroundContactAndWallCheck and the fall path. Tightly coupled
 * to its caller (RunnPhysicsAndTileCollisionTick) via saved registers
 * ($s0..$s5).
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnPhysicsLandingDispatch);

/*
 * RunnLand_GroundContactAndWallCheck @ 0x80071E2C
 *
 * RUNN landing/ground handler: snaps worldY onto the slope via
 * CalcEntityYFromTileCollision, then probes 0xC pixels ahead with
 * EntityApplyMovementCallbacks for a wall. Material 0xB5 -> hazard
 * transition through the gp-rel D_800A5FBC/D_800A5FC0 state pair;
 * otherwise jumps to SoarState_SelectNextByInput (shared selector with
 * SOAR, despite "Runn" in the name).
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnLand_GroundContactAndWallCheck);

/*
 * RunnLand_HazardDamageTransition @ 0x80071E84 (4 bytes - likely a
 * spurious splat split)
 *
 * Splat exposed this as a 4-instruction "function" but it is really the
 * fall-through tail of RunnLand_GroundContactAndWallCheck that
 * decrements worldY before continuing into the RunnLand_HazardDamageAlt
 * label (which calls EntitySetState). Likely should be merged back into
 * its caller once that function is decompiled.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnLand_HazardDamageTransition);

/* vehicle .sdata (0x800A5FAC..0x800A603C): {marker=0xFFFF0000, callback}
 * descriptor pairs for the Soar / Runn / Platform / Finn vehicle state machines,
 * with two "END2" (0x32444E45) sentinel+pad terminators embedded mid-table.
 * Migrated from the pooled asm sdata blob (sdata-under-split Phase 4). Address
 * order == declaration order (cc1 2.7.2 emits initialized .sdata in decl order).
 * Consumed only by unmatched asm; all entries use their D_ names. Two callbacks
 * (D_800A6010, D_800A6038) reference unnamed functions and are emitted as raw
 * absolute code addresses (byte-identical to the gold .word). */
extern void EntityEnterAnimatedIdleState();
extern void SoarStateInit_BeginFlightMode();
extern void RunnEntityDamageEffect();
extern void PlatformStateInit_IdleNoGravity();
extern void RunnState_DeathAdvanceLevel();
extern void PlatformStateInit_BounceWithGravity();
extern void PlatformStateInit_LandFromFlight();
extern void PlatformState_StopSound();
extern void PlatformState_EnablePlayerInput();
extern void RunnSetIdleState();
extern void RunnSetActiveState();
extern void FinnState_Spawn();
extern void FinnSetActiveState();
extern void FinnSetTurnState();
u32 D_800A5FAC asm("D_800A5FAC") = 0xFFFF0000;
EntityCallback D_800A5FB0 asm("D_800A5FB0") = (EntityCallback)EntityEnterAnimatedIdleState;
u32 D_800A5FB4 asm("D_800A5FB4") = 0xFFFF0000;
EntityCallback D_800A5FB8 asm("D_800A5FB8") = (EntityCallback)SoarStateInit_BeginFlightMode;
u32 D_800A5FBC asm("D_800A5FBC") = 0xFFFF0000;
EntityCallback D_800A5FC0 asm("D_800A5FC0") = (EntityCallback)RunnEntityDamageEffect;
u32 D_800A5FC4 asm("D_800A5FC4") = 0xFFFF0000;
EntityCallback D_800A5FC8 asm("D_800A5FC8") = (EntityCallback)PlatformStateInit_IdleNoGravity;
u32 D_800A5FCC asm("D_800A5FCC") = 0xFFFF0000;
EntityCallback D_800A5FD0 asm("D_800A5FD0") = (EntityCallback)RunnState_DeathAdvanceLevel;
u32 D_800A5FD4 asm("D_800A5FD4") = 0xFFFF0000;
EntityCallback D_800A5FD8 asm("D_800A5FD8") = (EntityCallback)PlatformStateInit_BounceWithGravity;
u32 D_800A5FDC asm("D_800A5FDC") = 0xFFFF0000;
EntityCallback D_800A5FE0 asm("D_800A5FE0") = (EntityCallback)PlatformStateInit_LandFromFlight;
u32 D_800A5FE4 asm("D_800A5FE4") = 0xFFFF0000;
EntityCallback D_800A5FE8 asm("D_800A5FE8") = (EntityCallback)PlatformState_StopSound;
u32 D_800A5FEC asm("D_800A5FEC") = 0xFFFF0000;
EntityCallback D_800A5FF0 asm("D_800A5FF0") = (EntityCallback)PlatformState_EnablePlayerInput;
u32 D_800A5FF4[2] asm("D_800A5FF4") = {0x32444E45, 0x00000000};
u32 D_800A5FFC asm("D_800A5FFC") = 0xFFFF0000;
EntityCallback D_800A6000 asm("D_800A6000") = (EntityCallback)RunnSetIdleState;
u32 D_800A6004 asm("D_800A6004") = 0xFFFF0000;
EntityCallback D_800A6008 asm("D_800A6008") = (EntityCallback)RunnSetActiveState;
u32 D_800A600C asm("D_800A600C") = 0xFFFF0000;
EntityCallback D_800A6010 asm("D_800A6010") = (EntityCallback)0x80073DE0;
u32 D_800A6014[2] asm("D_800A6014") = {0x32444E45, 0x00000000};
u32 D_800A601C asm("D_800A601C") = 0xFFFF0000;
EntityCallback D_800A6020 asm("D_800A6020") = (EntityCallback)FinnState_Spawn;
u32 D_800A6024 asm("D_800A6024") = 0xFFFF0000;
EntityCallback D_800A6028 asm("D_800A6028") = (EntityCallback)FinnSetActiveState;
u32 D_800A602C asm("D_800A602C") = 0xFFFF0000;
EntityCallback D_800A6030 asm("D_800A6030") = (EntityCallback)FinnSetTurnState;
u32 D_800A6034 asm("D_800A6034") = 0xFFFF0000;
EntityCallback D_800A6038 asm("D_800A6038") = (EntityCallback)0x80074D18;
