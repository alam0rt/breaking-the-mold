/* =============================================================================
 * parallax_ticks.c  --  PC-port Standard/Small parallax layer tick callbacks
 * =============================================================================
 * Faithful transcriptions of UpdateParallaxScrollWithWrap_Standard (0x8001F19C)
 * and UpdateParallaxScrollWithWrap_Small (0x8001F5A4) from export/SLES_010.90.c
 * -- siblings of the already-converted _Medium (anim/UpdateParallaxScrollWith-
 * Wrap_Medium.c; same layer-render-context layout, see that file's header).
 * Per frame: scroll = camera * parallax scale (16.16), optional reverse,
 * optional auto-scroll accumulator wrapping on the layer span; the negated
 * offset is written into the layer's scrollX/scrollY shorts via +0x1C.
 * _Standard normalises the accumulator against the largest multiple of the
 * span (the 0x7FFFFFFF/span dance); _Small wraps on the span directly.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

static s16 parallax_axis(u8 *ctx, s32 camera, s32 scaleOff, s32 accOff,
                         s32 stepOff, s32 spanOff, s32 enableOff, s32 revOff,
                         s32 screenSpanTiles, s32 bigWrap) {
    s16 off;

    if (ctx[revOff] == 0) {
        off = (s16)((u32)(camera * *(s32 *)(ctx + scaleOff)) >> 16);
    } else {
        off = (s16)((screenSpanTiles - *(s16 *)(ctx + spanOff)) * -0x10 -
              (s16)((u32)(camera * *(s32 *)(ctx + scaleOff)) >> 16));
    }
    if (ctx[enableOff] != 0 && *(s16 *)(ctx + stepOff) != 0) {
        s32 span = *(s16 *)(ctx + spanOff) * 0x80;
        s32 acc;
        if (bigWrap) {
            span = span * (0x7FFFFFFF / span);   /* largest multiple of span */
        }
        if (!bigWrap) {
            off = (s16)(off + (s16)(*(s32 *)(ctx + accOff) >> 3));
        }
        acc = *(s32 *)(ctx + accOff) - *(s16 *)(ctx + stepOff);
        *(s32 *)(ctx + accOff) = acc;
        if (span < acc) {
            *(s32 *)(ctx + accOff) = acc - (bigWrap ? span
                                        : (s32)(*(s16 *)(ctx + spanOff) * 0x80));
        }
        if (*(s32 *)(ctx + accOff) < 0) {
            *(s32 *)(ctx + accOff) += (bigWrap ? span
                                        : (s32)(*(s16 *)(ctx + spanOff) * 0x80));
        }
        if (bigWrap) {
            off = (s16)(off + (s16)(*(s32 *)(ctx + accOff) >> 3));
        }
    }
    return off;
}

void UpdateParallaxScrollWithWrap_Standard(void *arg0) {
    u8 *ctx = (u8 *)arg0;
    s16 off;

    off = parallax_axis(ctx, (s32)g_pGameState->camera_x, 0x20, 0x28, 0x30,
                        0x34, 0x38, 0x3A, 0x14, 1);
    *(s16 *)(*(u8 **)(ctx + 0x1C) + 0) = (s16)-off;
    off = parallax_axis(ctx, (s32)g_pGameState->camera_y, 0x24, 0x2C, 0x32,
                        0x36, 0x39, 0x3B, 0x0F, 1);
    *(s16 *)(*(u8 **)(ctx + 0x1C) + 2) = (s16)-off;
}

void UpdateParallaxScrollWithWrap_Small(void *arg0) {
    u8 *ctx = (u8 *)arg0;
    s16 off;

    off = parallax_axis(ctx, (s32)g_pGameState->camera_x, 0x20, 0x28, 0x30,
                        0x34, 0x38, 0x3A, 0x14, 0);
    *(s16 *)(*(u8 **)(ctx + 0x1C) + 0) = (s16)-off;
    off = parallax_axis(ctx, (s32)g_pGameState->camera_y, 0x24, 0x2C, 0x32,
                        0x36, 0x39, 0x3B, 0x0F, 0);
    *(s16 *)(*(u8 **)(ctx + 0x1C) + 2) = (s16)-off;
}
