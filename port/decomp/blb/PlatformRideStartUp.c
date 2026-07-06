/* =============================================================================
 * PlatformRideStartUp.c  --  PC-port platform ride-start state init
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/PlatformRideStartUp.s
 * (0x800270A8, 0x94). Fired when the player lands on a ride platform: latches
 * the ride-dip easing params (target Y +0x100 = +0x106, start Y +0x102 =
 * current +0x6A, delta +0x104; 0x14-frame timer @+0x10A, counter @+0x10B),
 * installs PlatformInterpolatePosition in the render slot (+0x1C) and
 * PlatformRideComplete in the queued-next slot (+0x98), and sets the
 * ride-active latch (+0x10E).
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern char PlatformInterpolatePosition[];
extern char PlatformRideComplete[];

void PlatformRideStartUp(void *entity) {
    u8 *e = (u8 *)entity;
    u16 curY = *(u16 *)(e + 0x6A);
    u16 targetY = *(u16 *)(e + 0x106);

    e[0x10F] = 0;
    e[0x110] = 0;
    e[0x10A] = 0x14;                       /* ease duration (frames) */
    e[0x10B] = 0;                          /* ease counter */
    *(s16 *)(e + 0x100) = (s16)targetY;
    *(s16 *)(e + 0x102) = (s16)curY;
    *(s16 *)(e + 0x104) = (s16)(targetY - curY);

    *(u32 *)(e + 0x1C) = 0xFFFF0000u;      /* render slot */
    *(void **)(e + 0x20) = (void *)PlatformInterpolatePosition;
    e[0x10E] = 1;                          /* ride-active latch */
    *(u32 *)(e + 0x98) = 0xFFFF0000u;      /* queued-next slot */
    *(void **)(e + 0x9C) = (void *)PlatformRideComplete;
}
