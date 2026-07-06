/* =============================================================================
 * PlatformInterpolatePosition.c  --  PC-port platform ride-dip easing tick
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/PlatformInterpolatePosition.s
 * (0x8002700C, 0x9C). Per-frame render-slot callback installed by
 * PlatformRideStartUp: eases the platform's Y (+0x6A) from the ride-start Y
 * (+0x102) toward the target (+0x100 = start + delta@+0x104) over +0x10A
 * frames (counter @+0x10B), decaying by halving shifts (fast at the middle,
 * settling at both ends). When the timer expires it snaps to the target and
 * runs the entity's queued callback (EntityProcessCallbackQueue).
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern void EntityProcessCallbackQueue(void *entity);

void PlatformInterpolatePosition(void *entity) {
    u8 *e = (u8 *)entity;
    u8 t = (u8)(e[0x10B] + 1);
    u8 dur = e[0x10A];
    u8 half = dur >> 1;

    e[0x10B] = t;
    if (dur < t) {                                     /* done: snap + queue */
        *(s16 *)(e + 0x6A) = (s16)*(u16 *)(e + 0x100);
        EntityProcessCallbackQueue(e);
        return;
    }
    if (half >= t) {                                   /* first half */
        s32 sh = (half - t) + 1;
        *(s16 *)(e + 0x6A) =
            (s16)(*(u16 *)(e + 0x102) + (*(s16 *)(e + 0x104) >> sh));
    } else {                                           /* second half */
        s32 sh = (t - half) + 1;
        s16 d = *(s16 *)(e + 0x104);
        *(s16 *)(e + 0x6A) =
            (s16)(*(u16 *)(e + 0x102) + (d - (d >> sh)));
    }
}
