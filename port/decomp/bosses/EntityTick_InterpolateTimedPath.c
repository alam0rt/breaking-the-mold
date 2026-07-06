/* =============================================================================
 * EntityTick_InterpolateTimedPath.c  --  PC-port waypoint-path platform tick
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/bosses/
 * EntityTick_InterpolateTimedPath.s (0x80056888, 0xD0). Per-frame tick for
 * path-following entities (moving platforms): advances the path timer
 * (+0x10E), interpolates the current waypoint position
 * (InterpolateTimedPathPosition over the path data @+0x108, +0x10C waypoints,
 * +0x119 steps-per-segment), then writes the world position as base
 * (+0x104/+0x106) plus the interpolated offset. The Y delta this frame is
 * latched <<16 into +0x114 (riders read it). When the looping flag (+0x118)
 * is set and the interpolated point equals the path's final waypoint, the
 * timer resets to 0.
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData,
                                         s16 duration, s32 steps);

void EntityTick_InterpolateTimedPath(void *entity) {
    u8 *e = (u8 *)entity;
    s16 out[2];
    s16 count = *(s16 *)(e + 0x10C);
    s32 newY;

    *(u16 *)(e + 0x10E) += 1;
    InterpolateTimedPathPosition((u16 *)(e + 0x10E), out,
                                 *(u8 **)(e + 0x108), count, e[0x119]);

    *(s16 *)(e + 0x68) = (s16)(*(u16 *)(e + 0x104) + (u16)out[0]);
    newY = *(s16 *)(e + 0x106) + out[1];
    *(s32 *)(e + 0x114) = (newY - *(s16 *)(e + 0x6A)) << 16;
    *(s16 *)(e + 0x6A) = (s16)(*(u16 *)(e + 0x106) + (u16)out[1]);

    if (e[0x118] != 0) {
        u8 *pend = *(u8 **)(e + 0x108) + count * 4;
        if (out[0] == *(s16 *)(pend - 4) && out[1] == *(s16 *)(pend - 2)) {
            *(u16 *)(e + 0x10E) = 0;
        }
    }
}
