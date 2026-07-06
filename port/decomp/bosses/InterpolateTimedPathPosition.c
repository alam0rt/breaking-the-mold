/* =============================================================================
 * InterpolateTimedPathPosition.c  --  PC-port waypoint path interpolation
 * =============================================================================
 * Functional-C body for InterpolateTimedPathPosition (INCLUDE_ASM in
 * src/bosses.c; used by decor path followers, platforms and path enemies).
 * pathData is an array of s16 waypoint pairs (x, y) -- possibly unaligned,
 * hence the byte reads. `duration` is the waypoint count; each segment runs
 * `steps` ticks. Writes the interpolated (x, y) to out[0..1]:
 *   seg  = *time / steps  (wrapping to 0 past the end, normalizing *time)
 *   step = *time % steps
 * then walks the major axis of segment seg->seg+1 (wrapping to waypoint 0)
 * proportionally, with the minor axis scaled by dy/dx (Bresenham-style).
 * The original's lwl/lwr soup in the export is just these unaligned reads.
 * ========================================================================== */
#include "common.h"
#include <string.h>

static s16 wp(u8 *pathData, s32 idx) {
    s16 v;
    memcpy(&v, pathData + idx * 2, sizeof(v));
    return v;
}

void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData,
                                  s16 duration, s32 steps) {
    s32 seg = (s16)*time / steps;
    s32 step;
    s16 curX, curY, nxtX, nxtY;
    s32 dx, dy, t;

    if (seg >= duration) {
        seg = 0;
        *time = (u16)((s16)*time % steps);
    }
    step = (s16)*time % steps;

    curX = wp(pathData, seg * 2);
    curY = wp(pathData, seg * 2 + 1);
    out[0] = curX;
    out[1] = curY;
    if (seg + 1 < duration) {
        nxtX = wp(pathData, (seg + 1) * 2);
        nxtY = wp(pathData, (seg + 1) * 2 + 1);
    } else {
        nxtX = wp(pathData, 0);
        nxtY = wp(pathData, 1);
    }

    dx = (nxtX < curX) ? (curX - nxtX) : (nxtX - curX);
    dy = (nxtY < curY) ? (curY - nxtY) : (nxtY - curY);

    if (dx < dy) {
        t = (s16)(dy * step / steps);
        if (nxtX < curX) {
            out[0] = curX - (s16)(t * dx / dy);
        } else {
            out[0] = curX + (s16)(t * dx / dy);
        }
        if (nxtY < curY) {
            out[1] = curY - (s16)t;
        } else {
            out[1] = curY + (s16)t;
        }
    } else {
        if (dx == 0) {
            return;                     /* degenerate segment: stay put */
        }
        t = (s16)(dx * step / steps);
        if (nxtX < curX) {
            out[0] = curX - (s16)t;
        } else {
            out[0] = curX + (s16)t;
        }
        if (nxtY < curY) {
            out[1] = curY - (s16)(t * dy / dx);
        } else {
            out[1] = curY + (s16)(t * dy / dx);
        }
    }
}
