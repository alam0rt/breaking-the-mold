/* =============================================================================
 * PlayerCallback_JumpInputAndCounters.c  --  PC-port airborne input handler
 * =============================================================================
 * Functional-C reconstruction of PlayerCallback_JumpInputAndCounters
 * (0x800602E0, 0x2F8; export/SLES_010.90.c). Installed in the player's
 * input-state FSM slot (+0x104/+0x108) by the airborne states: weapon-button
 * click SFX, swirly-Q air toss, glide deploy / jump-release fall transitions,
 * air steering (facing + velocityX), and the landing/bounce cushion timers.
 *
 * NAME CHECKS (verified against the .s, .sdata masks and playst.c pairs):
 *  - Ghidra's `_g_DefaultBGColorR/G` are junk names at WRONG addresses; the
 *    real masks are the src/gfx.c sdata island: D_800A596C (0x40, jump) and
 *    D_800A596E (0x20, special/swirly toss).
 *  - The fsmSlot_* names come from the target state-init functions; the
 *    D_800A5E68..5E84 pair addresses were verified in the .s (gp-rel loads).
 *    The state names themselves look drift-prone (e.g. "ShrinkAndFall" is
 *    installed on jump-release-while-grounded) -- kept as-is, the pair
 *    addresses are what matter.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/player_state.h"
#include <stdint.h>

extern void EntitySetState(void *entity, u32 marker, u32 callback);
extern void PlayEntityPositionSound(void *entity, u32 soundId);

/* button masks (src/gfx.c sdata island) */
extern u16 D_800A596C_jump    asm("D_800A596C");   /* 0x40 */
extern u16 D_800A596E_special asm("D_800A596E");   /* 0x20 */

/* FSM pairs (src/playst.c data island; addresses verified in the .s) */
extern u32 D_800A5E68 asm("D_800A5E68");  extern void *D_800A5E6C asm("D_800A5E6C");
extern u32 D_800A5E70 asm("D_800A5E70");  extern void *D_800A5E74 asm("D_800A5E74");
extern u32 D_800A5E78 asm("D_800A5E78");  extern void *D_800A5E7C asm("D_800A5E7C");
extern u32 D_800A5E80 asm("D_800A5E80");  extern void *D_800A5E84 asm("D_800A5E84");

void PlayerCallback_JumpInputAndCounters(void *arg0) {
    u8  *e = (u8 *)arg0;
    u8  *input = *(u8 **)(e + 0x100);
    u16  pressed = *(u16 *)(input + 2);
    u16  held    = *(u16 *)(input + 0);

    /* weapon-button click */
    if ((pressed & 4) || (pressed & 1) || (pressed & 8) || (pressed & 2)) {
        PlayEntityPositionSound(e, 0x64221E61);
    }

    /* swirly-Q air toss / special buffering */
    if (e[0x170] == 0 && g_pPlayerState->swirly_q_count != 0 &&
        (D_800A596E_special & pressed) != 0) {
        if (e[0x135] == 0) {                     /* specialMoveMode */
            EntitySetState(e, D_800A5E68, (u32)(uintptr_t)D_800A5E6C);
        } else {
            e[0x134] = 1;                        /* specialMoveQueued */
        }
    } else if ((D_800A596C_jump & pressed) != 0) {
        e[0x13C] = 10;                           /* timer13C */
    }

    /* jump-release / glide-deploy transitions */
    if ((D_800A596C_jump & held) == 0) {
        if (e[0x170] != 0) {                     /* grounded: land */
            EntitySetState(e, D_800A5E78, (u32)(uintptr_t)D_800A5E7C);
        }
    } else if (*(s32 *)(e + 0x16C) != 0 &&       /* glideEntity */
               *(s32 *)(e + 0x110) >= 1 &&       /* falling */
               e[0x170] == 0) {                  /* airborne */
        EntitySetState(e, D_800A5E70, (u32)(uintptr_t)D_800A5E74);
    }

    /* air steering */
    if (e[0x11F] == 0) {                         /* jumpHoldCounter */
        if ((held & 0x2000) != 0) {              /* logical RIGHT (asm: andi 0x2000) */
            *(s32 *)(e + 0x114) = *(s32 *)(e + 0x124);   /* velX = +max */
            if (e[0x170] != 0 && e[0x74] != 0) {
                e[0x74] = 0;
                EntitySetState(e, D_800A5E80, (u32)(uintptr_t)D_800A5E84);
            } else {
                e[0x74] = 0;
            }
        } else if ((held & 0x8000) != 0) {       /* logical LEFT (asm: andi 0x8000) */
            *(s32 *)(e + 0x114) = -*(s32 *)(e + 0x124);  /* velX = -max */
            if (e[0x170] != 0 && e[0x74] == 0) {
                e[0x74] = 1;
                EntitySetState(e, D_800A5E80, (u32)(uintptr_t)D_800A5E84);
            } else {
                e[0x74] = 1;
            }
        }
    } else {
        e[0x11F]--;
        *(s32 *)(e + 0x114) = *(s32 *)(e + 0x120);       /* velX = altSpeed */
    }

    /* landing/bounce cushion timers */
    if (e[0x11E] != 0) {                         /* bounceLockTimer */
        e[0x11E]--;
        *(s32 *)(e + 0x118) = -0x1200;           /* cushionVelY */
        e[0x11C]--;                              /* landingTimer */
    } else if (e[0x11C] != 0) {
        e[0x11C]--;
        if ((D_800A596C_jump & held) == 0) {
            if (*(s32 *)(e + 0x110) < -0x48000) {        /* velY floor */
                *(s32 *)(e + 0x110) = -0x48000;
            }
        } else {
            *(s32 *)(e + 0x118) = -0x1200;
        }
    }
}
