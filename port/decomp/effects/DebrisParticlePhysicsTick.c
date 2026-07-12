/* =============================================================================
 * DebrisParticlePhysicsTick.c  --  functional-C body for effects.c
 * DebrisParticlePhysicsTick (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/effects/DebrisParticlePhysicsTick.s
 * (0x800351BC, 0x160). Installed in the debris entity's update slot
 * (+0x1C/+0x20, fired from EntityUpdateCallback each frame): 16.16 gravity on
 * velY (+0x5000/frame, terminal 0xA0000), integrates world position with
 * sub-pixel remainders at +0x6C/+0x6E, and "spins" by adding the spin rate
 * (+0x108) to both render-scale fields (+0x50/+0x54). Scale bounces off the
 * 0x40000 ceiling (negate spin); shrinking under 0x2000 sends event 3 (entity
 * removal) to the GameState -- the debris' end of life.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

void DebrisParticlePhysicsTick(void *arg0) {
    u8 *e = (u8 *)arg0;
    s32 velY;
    s32 x16, y16;

    velY = *(s32 *)(e + 0x104) + 0x5000;
    if (velY > 0xA0000) {
        velY = 0xA0000;
    }
    *(s32 *)(e + 0x104) = velY;

    x16 = ((s32)*(s16 *)(e + 0x68) << 16) + *(u16 *)(e + 0x6C);
    y16 = ((s32)*(s16 *)(e + 0x6A) << 16) + *(u16 *)(e + 0x6E) + velY;
    *(u16 *)(e + 0x6E) = (u16)y16;
    x16 += *(s32 *)(e + 0x100);
    *(s32 *)(e + 0x54) += *(s32 *)(e + 0x108);
    *(s32 *)(e + 0x50) += *(s32 *)(e + 0x108);
    *(s16 *)(e + 0x68) = (s16)(x16 >> 16);
    *(s16 *)(e + 0x6A) = (s16)(y16 >> 16);
    *(u16 *)(e + 0x6C) = (u16)x16;

    if (*(s32 *)(e + 0x50) > 0x40000) {
        *(s32 *)(e + 0x50) = 0x40000;
        *(s32 *)(e + 0x54) = 0x40000;
        *(s32 *)(e + 0x108) = -*(s32 *)(e + 0x108);
    }
    if (*(s32 *)(e + 0x50) < 0x2001) {
        fsm_send_event((u8 *)g_pGameState, 3, 0, e);
    }
}
