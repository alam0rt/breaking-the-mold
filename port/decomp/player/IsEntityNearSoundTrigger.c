/* =============================================================================
 * IsEntityNearSoundTrigger.c  --  PC-port proximity test for sound triggers
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/player/IsEntityNearSoundTrigger.s
 * (0x8005A3F8, 0x1CC). Resolves the entity's world Y through its optional
 * Y-position-transform slot (marker@+0x2E, argBase@+0x2C, fn@+0x30), resolves
 * its world X through the X-transform slot (marker@+0x26, argBase@+0x24,
 * fn@+0x28), queries the tile slope height at that X, and returns whether the
 * caller's reference value is within 10 subpixels of the entity's slope-relative
 * height: ((u16)(refValue - (((resolvedY | 0xF) - slope) << 16) / entity->+0x5C))
 * < 0xA.
 *
 * Both transform slots use the shared tagged-dispatch encoding but call a
 * two-argument coordinate transform fn(base, coord) that returns s32; a zero
 * marker passes the coordinate through unchanged.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8 GetSlopeHeightAtSubpixel(void *gs, u32 tileIdx, s32 worldX);

typedef s32 (*CoordFn)(void *base, s32 coord);

/* Tagged-slot coordinate transform: slot = { s16 argBase, s16 marker, s32 fn }
 * at (e + slotOff). marker 0 -> pass `coord` through; marker < 0 -> direct
 * fn call at e+argBase; marker > 0 -> table entry {argOff, fn} at
 * *(e + (s16)fnWord) + (marker-1)*8, base e + argBase + argOff. */
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

s32 IsEntityNearSoundTrigger(void *entity, u8 tileIdx, s16 x, s16 y, s32 refValue) {
    u8 *e = (u8 *)entity;
    s32 resolvedY = coord_dispatch(e, 0x2C, y);
    s32 resolvedX = coord_dispatch(e, 0x24, x);
    u8  slope = GetSlopeHeightAtSubpixel(g_pGameState, tileIdx & 0xFF, resolvedX);
    s32 quotient = ((s32)((resolvedY | 0xF) - slope) << 16) / *(s32 *)(e + 0x5C);

    return ((u16)(refValue - quotient) < 0xA) ? 1 : 0;
}
