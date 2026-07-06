/* =============================================================================
 * PlatformRideStartDown.c  --  PC-port platform ride-return state init
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/PlatformRideStartDown.s
 * (0x8002713C, 0x90). Twin of PlatformRideStartUp fired when the rider leaves:
 * eases the platform back from its current Y (+0x6A) toward the stored rest Y
 * (+0x108) over 0x14 frames, installs PlatformInterpolatePosition in the render
 * slot and PlatformRideComplete in the queued-next slot. Differs from StartUp
 * only in the target source (+0x108 vs +0x106) and clearing the ride-active
 * latch (+0x10E = 0) instead of setting it.
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern char PlatformInterpolatePosition[];
extern char PlatformRideComplete[];

void PlatformRideStartDown(void *entity) {
    u8 *e = (u8 *)entity;
    u16 curY = *(u16 *)(e + 0x6A);
    u16 targetY = *(u16 *)(e + 0x108);

    e[0x10A] = 0x14;                       /* ease duration (frames) */
    e[0x10F] = 0;
    e[0x110] = 0;
    e[0x10B] = 0;                          /* ease counter */
    *(s16 *)(e + 0x100) = (s16)targetY;
    *(s16 *)(e + 0x102) = (s16)curY;
    *(s16 *)(e + 0x104) = (s16)(targetY - curY);

    *(u32 *)(e + 0x1C) = 0xFFFF0000u;      /* render slot */
    *(void **)(e + 0x20) = (void *)PlatformInterpolatePosition;
    e[0x10E] = 0;                          /* ride-active latch cleared */
    *(u32 *)(e + 0x98) = 0xFFFF0000u;      /* queued-next slot */
    *(void **)(e + 0x9C) = (void *)PlatformRideComplete;
}
