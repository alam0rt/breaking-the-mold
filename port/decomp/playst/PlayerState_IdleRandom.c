/* =============================================================================
 * PlayerState_IdleRandom.c  --  PC-port conversion (playst.c, 0x800679xx)
 * =============================================================================
 * The grounded idle "fidget" tick: two countdown timers decide when to fire a
 * random idle animation. Timer A (u16 @ +0x13A) triggers the big idle anim
 * (FSM slot D_800A5DA0/D_800A5DA4, reroll (rand() & 0x100) + 0x200); timer B
 * (u8 @ +0x139) the small look-around (D_800A5DA8/D_800A5DAC, reroll
 * (rand() & 0x7F) + 0x30). Falls through to PlayerTickCallback every frame.
 * rand() is the BIOS LCG (port/spec/bios.c) -- demo determinism depends on
 * consuming draws in exactly this pattern.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void EntitySetState(void *e, s32 marker, s32 fn);
extern void PlayerTickCallback(void *e);
extern s32  rand(void);

extern s32 IdleAnimAMarker asm("D_800A5DA0");
extern s32 IdleAnimAFn     asm("D_800A5DA4");
extern s32 IdleAnimBMarker asm("D_800A5DA8");
extern s32 IdleAnimBFn     asm("D_800A5DAC");

void PlayerState_IdleRandom(void *arg0) {
    u8 *p = (u8 *)arg0;
    u16 tA;
    u8  tB;

    tA = (u16)(*(u16 *)(p + 0x13A) - 1);
    *(u16 *)(p + 0x13A) = tA;
    if (tA == 0) {
        *(u16 *)(p + 0x13A) = (u16)((rand() & 0x100) + 0x200);
        EntitySetState(arg0, IdleAnimAMarker, IdleAnimAFn);
    } else {
        tB = (u8)(p[0x139] - 1);
        p[0x139] = tB;
        if (tB == 0) {
            p[0x139] = (u8)((rand() & 0x7F) + 0x30);
            EntitySetState(arg0, IdleAnimBMarker, IdleAnimBFn);
        }
    }
    PlayerTickCallback(arg0);
}
