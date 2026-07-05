/* =============================================================================
 * UpdateParallaxScrollWithWrap_Medium.c  --  PC-port medium-layer parallax tick
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c UpdateParallaxScrollWithWrap_Medium
 * (0x8001F3B4, INCLUDE_ASM in src/anim.c). Per-frame tick callback of a "medium"
 * background layer's render context: derives the layer's on-screen scroll offset
 * from the camera position times the layer's parallax scale (with an optional
 * reverse and an optional auto-scroll accumulator that wraps every N tiles), and
 * writes the negated offset into the layer's tilemap primitive (screen X at
 * *(+0x1c), screen Y at *(+0x1c)+2).
 *
 * Layer render context (0x3C, from InitLayerRenderContext_Medium):
 *   +0x1c  ptr to the layer's tilemap primitive (short screenX @0, screenY @+2)
 *   +0x20  s32 parallax scale X (16.16)      +0x24 parallax scale Y
 *   +0x28  s32 auto-scroll accumulator X     +0x2c accumulator Y
 *   +0x30  s16 auto-scroll step X            +0x32 step Y
 *   +0x34  s16 wrap width (tiles)            +0x36 wrap height (tiles)
 *   +0x38  u8  auto-scroll enable X          +0x39 enable Y
 *   +0x3a  u8  reverse X                     +0x3b reverse Y
 * The X wrap span is 0x14 tiles, Y is 0xf tiles (the medium-layer constants).
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

void UpdateParallaxScrollWithWrap_Medium(void *pLayerCtx) {
    u8 *ctx = (u8 *)pLayerCtx;
    s16 off;

    /* ---- X axis ---- */
    if (*(char *)(ctx + 0x3a) == 0) {
        off = (s16)((u32)((s32)g_pGameState->camera_x * *(s32 *)(ctx + 0x20)) >> 0x10);
    } else {
        off = (0x14 - *(s16 *)(ctx + 0x34)) * -0x10 -
              (s16)((u32)((s32)g_pGameState->camera_x * *(s32 *)(ctx + 0x20)) >> 0x10);
    }
    if ((*(char *)(ctx + 0x38) != 0) && (*(s16 *)(ctx + 0x30) != 0)) {
        s32 acc = *(s32 *)(ctx + 0x28) - (s32)*(s16 *)(ctx + 0x30);
        s32 span = *(s16 *)(ctx + 0x34) * 0x80;
        off = off + (s16)(*(s32 *)(ctx + 0x28) >> 3);
        *(s32 *)(ctx + 0x28) = acc;
        if (span < acc) {
            *(s32 *)(ctx + 0x28) = acc + *(s16 *)(ctx + 0x34) * -0x80;
        }
        if (*(s32 *)(ctx + 0x28) < 0) {
            *(s32 *)(ctx + 0x28) = *(s32 *)(ctx + 0x28) + span;
        }
    }
    **(s16 **)(ctx + 0x1c) = -off;

    /* ---- Y axis ---- */
    if (*(char *)(ctx + 0x3b) == 0) {
        off = (s16)((u32)((s32)g_pGameState->camera_y * *(s32 *)(ctx + 0x24)) >> 0x10);
    } else {
        off = (0xf - *(s16 *)(ctx + 0x36)) * -0x10 -
              (s16)((u32)((s32)g_pGameState->camera_y * *(s32 *)(ctx + 0x24)) >> 0x10);
    }
    if ((*(char *)(ctx + 0x39) != 0) && (*(s16 *)(ctx + 0x32) != 0)) {
        s32 acc = *(s32 *)(ctx + 0x2c) - (s32)*(s16 *)(ctx + 0x32);
        s32 span = *(s16 *)(ctx + 0x36) * 0x80;
        off = off + (s16)(*(s32 *)(ctx + 0x2c) >> 3);
        *(s32 *)(ctx + 0x2c) = acc;
        if (span < acc) {
            *(s32 *)(ctx + 0x2c) = acc + *(s16 *)(ctx + 0x36) * -0x80;
        }
        if (*(s32 *)(ctx + 0x2c) < 0) {
            *(s32 *)(ctx + 0x2c) = *(s32 *)(ctx + 0x2c) + span;
        }
    }
    *(s16 *)(*(s32 *)(ctx + 0x1c) + 2) = -off;
}
