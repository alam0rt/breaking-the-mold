/* =============================================================================
 * EntityEventHandler0x1001_1002_1008.c  --  PC-port conversion (0x8003BB84)
 * =============================================================================
 * Ride-platform event slot shared by platform-ish entities:
 *   0x1001: set the "player touched" byte (+0x106); returns 0.
 *   0x1002: set +0x106, and if the departing rider (arg) is the latched one
 *           (+0x108) clear the latch; returns the entity.
 *   0x1008: latch the rider (+0x108) if free; returns 1 when latched.
 *   0x0002 (animation tick): countdown +0x110 -- at 1 rewind the animation
 *           (SetAnimationTargetFrameIndex -1), at 0 drain the callback queue.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void EntityProcessCallbackQueue(void *e);
extern void SetAnimationTargetFrameIndex(void *e, s32 frame);

s32 EntityEventHandler0x1001_1002_1008(void *arg0, s32 ev, s32 unused, s32 arg) {
    u8 *e = (u8 *)arg0;
    u16 event = (u16)ev;
    s32 ret = 0;

    if (event == 0x1002) {
        e[0x106] = 1;
        if (arg == *(s32 *)(e + 0x108)) {
            *(s32 *)(e + 0x108) = 0;
        }
        ret = (s32)(uintptr_t)arg0;
    } else if (event == 0x1001) {
        e[0x106] = 1;
    } else if (event == 0x1008) {
        if (*(s32 *)(e + 0x108) == 0) {
            *(s32 *)(e + 0x108) = arg;
            ret = 1;
        }
    }

    if (event == 2) {
        u8 c = e[0x110];
        if (c != 0) {
            c = (u8)(c - 1);
            e[0x110] = c;
            if (c == 0) {
                EntityProcessCallbackQueue(arg0);
            } else if (c == 1) {
                SetAnimationTargetFrameIndex(arg0, -1);
            }
        }
    }
    return ret;
}
