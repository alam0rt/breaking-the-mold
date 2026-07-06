/* =============================================================================
 * EntityTick_PlatformRideIdle.c  --  PC-port platform-ride idle tick
 * =============================================================================
 * Functional-C conversion of
 * asm/nonmatchings/blb/EntityTick_PlatformRideIdle.s
 * (0x80026F34, 0xD8 bytes; reference = export/SLES_010.90.c 23248).
 *
 * Idle state for a rideable platform: runs the base entity callback, then the
 * shared up/down ride-state transition lattice (identical to the tail of
 * EntityTick_DigitDisplayUpdate) driven by the flags at +0x10C/+0x111/+0x10E
 * and the fsm slots D_800A5990/5994 (ride-up) and D_800A5998/599C (ride-down).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void EntityUpdateCallback(void *entityPtr, u32 param2, s16 param3);
extern void EntitySetState(void *entity, u32 newMarker, u32 newCallback);

extern u32 D_800A5990; /* ride-up marker   */
extern u32 D_800A5994; /* ride-up fn       */
extern u32 D_800A5998; /* ride-down marker */
extern u32 D_800A599C; /* ride-down fn     */

void EntityTick_PlatformRideIdle(void *entity, u32 param_2, s16 param_3) {
    u8 *e = (u8 *)entity;
    int gotState = 0;
    u32 marker = 0;
    u32 fn = 0;

    EntityUpdateCallback(e, param_2, param_3);

    if (*(u16 *)(e + 0x10C) != 0 || *(u8 *)(e + 0x111) != 0) {
        if (*(u8 *)(e + 0x10E) == 0) {
            marker = D_800A5990;
            fn = D_800A5994;
            gotState = 1;
        }
    } else {
        if (*(u8 *)(e + 0x111) == 0 && *(u8 *)(e + 0x10E) != 0) {
            marker = D_800A5998;
            fn = D_800A599C;
            gotState = 1;
        }
    }

    if (gotState) {
        EntitySetState(e, marker, fn);
    }

    if (*(u16 *)(e + 0x10C) != 0 && *(u8 *)(e + 0x111) == 0) {
        *(u16 *)(e + 0x10C) = (u16)(*(u16 *)(e + 0x10C) - 1);
    }
}
