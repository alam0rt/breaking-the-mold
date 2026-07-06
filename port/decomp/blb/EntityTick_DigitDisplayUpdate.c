/* =============================================================================
 * EntityTick_DigitDisplayUpdate.c  --  PC-port HUD digit tick
 * =============================================================================
 * Functional-C conversion of
 * asm/nonmatchings/blb/EntityTick_DigitDisplayUpdate.s
 * (0x80027418, 0x1CC bytes; reference = export/SLES_010.90.c 23606).
 *
 * HUD "rolling digit" entity tick. Maintains an animated counter (value @ +0x114
 * chasing target @ +0x116) with a per-frame countdown timer (+0x11A), refreshes
 * the digit sprite via UpdateDigitDisplay, nudges the entity's worldX for the
 * shrink-when-single-digit layout, runs the base entity callback, and drives an
 * up/down "ride" state transition through EntitySetState.
 *
 * Offsets are taken directly from the disassembly (entity base = s0):
 *   +0x068 worldX (s16)
 *   +0x10C flagA        (u16)   +0x10E flagC (u8)   +0x111 flagB (u8)
 *   +0x114 value        (u16)   +0x116 target (u16) +0x118 divisor (u16)
 *   +0x11A timer        (u8)    +0x11B xShrink (s8) +0x11C renderX (u16)
 *   D_800A5990/5994 = ride-up   fsm slot {marker, fn}
 *   D_800A5998/599C = ride-down fsm slot {marker, fn}
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void UpdateDigitDisplay(int entity);
extern void EntityUpdateCallback(void *entityPtr, u32 param2, s16 param3);
extern void EntitySetState(void *entity, u32 newMarker, u32 newCallback);

extern u32 D_800A5990; /* ride-up marker   */
extern u32 D_800A5994; /* ride-up fn       */
extern u32 D_800A5998; /* ride-down marker */
extern u32 D_800A599C; /* ride-down fn     */

void EntityTick_DigitDisplayUpdate(void *entity, u32 param_2, s16 param_3) {
    u8 *e = (u8 *)entity;
    s8 timer;

    /* If the counter just became active (flagA || flagB) and it is not paused
     * (flagC == 0), (re)arm the timer and refresh the sprite. */
    if ((*(u16 *)(e + 0x10C) != 0 || *(u8 *)(e + 0x111) != 0) &&
        *(u8 *)(e + 0x10E) == 0) {
        *(u8 *)(e + 0x11A) = 0x14;
        UpdateDigitDisplay((int)(intptr_t)e);
    }

    /* Tick the countdown timer. */
    timer = (s8)(*(u8 *)(e + 0x11A) - 1);
    *(u8 *)(e + 0x11A) = (u8)timer;
    if ((u8)timer == 0) {
        u16 target = *(u16 *)(e + 0x116);
        u16 value = *(u16 *)(e + 0x114);
        u32 t = target;
        u32 v = value;
        *(u8 *)(e + 0x11A) = 0x0A; /* re-arm timer */

        if (v < t) {
            *(u16 *)(e + 0x114) = (u16)(value + 1);
            UpdateDigitDisplay((int)(intptr_t)e);
        } else if (v > t) {
            if ((s32)(t + 0xA) < (s32)v) {
                *(u16 *)(e + 0x114) = target;      /* snap toward target */
            } else {
                *(u16 *)(e + 0x114) = (u16)(value - 1);
            }
            UpdateDigitDisplay((int)(intptr_t)e);
        }
        /* v == t: nothing to do (no refresh). */
    }

    /* When divisor (place value) is 0, adjust worldX: single-digit values get
     * the shrink offset applied, multi-digit use the base render X. */
    if (*(u16 *)(e + 0x118) == 0) {
        if (*(u16 *)(e + 0x114) < 0xA) {
            s8 xShrink = (s8)*(u8 *)(e + 0x11B);
            *(s16 *)(e + 0x68) = (s16)(*(u16 *)(e + 0x11C) - xShrink);
        } else {
            *(s16 *)(e + 0x68) = (s16)*(u16 *)(e + 0x11C);
        }
    }

    EntityUpdateCallback(e, param_2, param_3);

    /* Ride-state transition dispatch (mirrors the branch lattice in the asm). */
    {
        int gotState = 0;
        u32 marker = 0;
        u32 fn = 0;

        if (*(u16 *)(e + 0x10C) != 0 || *(u8 *)(e + 0x111) != 0) {
            if (*(u8 *)(e + 0x10E) == 0) {
                marker = D_800A5990;
                fn = D_800A5994;
                gotState = 1;
            }
        } else {
            /* flagA == 0 && flagB == 0 */
            if (*(u8 *)(e + 0x111) == 0 && *(u8 *)(e + 0x10E) != 0) {
                marker = D_800A5998;
                fn = D_800A599C;
                gotState = 1;
            }
        }

        if (gotState) {
            EntitySetState(e, marker, fn);
        }

        /* Decrement flagA countdown when active and flagB clear. */
        if (*(u16 *)(e + 0x10C) != 0 && *(u8 *)(e + 0x111) == 0) {
            *(u16 *)(e + 0x10C) = (u16)(*(u16 *)(e + 0x10C) - 1);
        }
    }
}
