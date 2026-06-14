#include "common.h"
#include "Game/entity.h"

extern void EntityProcessCallbackQueue(Entity *entity);
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
    if (maskedEvent == 2) {
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
    if ((event & 0xFFFF) == 2) {
        EntityProcessCallbackQueue(entity);
    }
    return 0;
}

/*
 * func_80071778 @ 0x80071778 (unidentified - FINN/vehicle X integrator)
 *
 * Advances the entity's worldX via its 16.16 fixed-point sub-pixel
 * accumulator: packs worldX:fracX into a 32-bit position, adds the
 * velocity word stored at entity+0x104 (FinnPlayerEntity.velocityX
 * shape), and splits the result back into worldX (upper 16) and fracX
 * (lower 16). Likely should be renamed FinnAdvanceXByVelocity or
 * similar.
 */
void func_80071778(Entity *entity) {
    s32 pos = ((s32)entity->worldX << 16) + (u16)entity->velocityX + *(s32 *)((u8 *)entity + 0x104);
    entity->worldX = pos >> 16;
    entity->velocityX = (s16)pos;
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
 * RunnRender_PhysicsAndTileCollision @ 0x80071AD4
 *
 * RUNN auto-scroller per-frame physics + tile probe pass. Integrates
 * worldX:fracX (0x68:0x6C) with velocityX (+0x104), worldY:fracY
 * (0x6A:0x6E) with velocityY (+0x108, ramped by 0x5000/tick and capped
 * at 0x80000), then probes ground/wall via EntityApplyMovementCallbacks
 * at multiple Y offsets. Branches to hazard, landing, or fall handlers
 * based on the returned material codes. Despite "Render" in the name,
 * this is the physics half - rendering is elsewhere; likely should be
 * renamed RunnPhysicsAndTileCollisionTick.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnRender_PhysicsAndTileCollision);

/*
 * RunnState_DieAndAdvanceLevel @ 0x80071D10
 *
 * Tiny tail-call helper: hands the entity off to EntitySetState with
 * the death-and-advance state pair at gp-rel D_800A5FCC/D_800A5FD0,
 * then jumps to RunnState_SelectNextByInput. Reached by fall-through
 * from RunnRender_PhysicsAndTileCollision when a probe returns material
 * 0x5B.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", RunnState_DieAndAdvanceLevel);

/*
 * func_80071D28 @ 0x80071D28 (unidentified - RUNN landing-dispatch tail)
 *
 * Continuation of the RUNN physics path: if the latest probe returned
 * material 0xB5 it routes into the hazard-damage transition; otherwise
 * it walks downward through CheckEntityNearTileY + more
 * EntityApplyMovementCallbacks probes to decide between
 * RunnLand_GroundContactAndWallCheck and the fall path. Tightly coupled
 * to its caller via saved registers ($s0..$s5). Likely should be named
 * something like RunnPhysicsLandingDispatch.
 */
INCLUDE_ASM("asm/nonmatchings/vehicle", func_80071D28);

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
