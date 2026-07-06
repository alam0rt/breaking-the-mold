/* =============================================================================
 * PlayerApplyPositionWithCollision.c  --  PC-port slope-relative floor snap
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/player/
 * PlayerApplyPositionWithCollision.s (0x8005A218, 0x1E0). The post-physics
 * "snap to ground" step: runs the entity's X and Y coordinate-transform slots
 * (the same tagged-union FSM used by EntityApplyMovementCallbacks /
 * IsEntityNearSoundTrigger), queries the tile slope height at the resolved X
 * (GetSlopeHeightAtSubpixel), and returns the slope-relative world Y:
 *
 *   Yout = (((resolvedY | 0xF) - slope) << 16) / entity->scalePowerupY(+0x5C)
 *
 * When the entity is at half Y-scale (scalePowerupX(+0x58) == 0xC000) and the
 * quotient's low two bits are 1 or 2, it rounds up by one (sub-pixel bias so
 * the half-size sprite settles flush on the slope). Returns the s16 world Y.
 *
 * This is what lets Klaymen land on the level's slope tiles (attribute < 0x3C):
 * FallingPhysicsMain detects the slope, IsEntityNearSoundTrigger confirms
 * contact, and this resolves the exact landing Y. Its X/Y-slot decode is the
 * shared coordinate-transform form (see IsEntityNearSoundTrigger.c).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8 GetSlopeHeightAtSubpixel(void *gs, u32 tileIdx, s32 worldX);

typedef s32 (*CoordFn)(void *base, s32 coord);

/* Tagged-slot coordinate transform: slot = { s16 argBase, s16 marker, s32 fn }
 * at (e + slotOff). marker 0 -> pass coord through; marker < 0 -> direct fn at
 * e+argBase; marker > 0 -> table entry {argOff, fn} at *(e + (s16)fn) +
 * (marker-1)*8, base e + argBase + argOff. */
static s32 coord_dispatch(u8 *e, s32 slotOff, s32 coord) {
    s16 marker = *(s16 *)(e + slotOff + 2);
    s16 argBase = *(s16 *)(e + slotOff);
    CoordFn fn;

    if (marker == 0) {
        return coord;
    }
    if (marker > 0) {
        s32 tableBase = *(s32 *)(e + *(s16 *)(e + slotOff + 4));
        u8 *entry = (u8 *)(uintptr_t)(tableBase + (s32)marker * 8);
        argBase = (s16)(argBase + (s16)*(s32 *)(entry - 8));
        fn = *(CoordFn *)(entry - 4);
    } else {
        fn = *(CoordFn *)(e + slotOff + 4);
    }
    return fn(e + argBase, (s16)coord);
}

s16 PlayerApplyPositionWithCollision(void *entity, u8 tileAttr, s16 worldX, s16 y) {
    u8 *e = (u8 *)entity;
    s32 resolvedY = coord_dispatch(e, 0x2C, y);
    s32 resolvedX = coord_dispatch(e, 0x24, worldX);
    u8  slope = GetSlopeHeightAtSubpixel(g_pGameState, tileAttr & 0xFF, resolvedX);
    s32 q = ((s32)((resolvedY | 0xF) - slope) << 16) / *(s32 *)(e + 0x5C);

    if (*(s32 *)(e + 0x58) == 0xC000 && (u32)((q & 3) - 1) < 2) {
        q += 1;
    }
    return (s16)q;
}
