/* =============================================================================
 * initPlayerState.c  --  functional-C body for blb.c initPlayerState (TARGET_PC)
 * =============================================================================
 * PC-port replacement for the asm at asm/nonmatchings/blb/initPlayerState.s
 * (0x800260D0). Zeroes the small player-state struct and stamps its default
 * flag bytes. Behaviour transcribed field-by-field from the disassembly; the
 * struct is treated as a raw byte array so no layout header is needed.
 * ========================================================================== */
#include "common.h"

/* Offsets written by the original (all single bytes unless noted):
 *   [0x00]=1 [0x01]=1 [0x10]=1 [0x11]=5   -- default mode/flags
 *   [0x02]=0 (halfword) [0x04]=0 [0x05]=0
 *   [0x12..0x1D] individually zeroed
 *   [0x06..0x0F] zeroed by the trailing i=0..9 loop
 */
void initPlayerState(void *state) {
    u8 *p = (u8 *)state;
    s32 i;
    if (!p) {
        return;
    }
    p[0x00] = 1;
    p[0x01] = 1;
    p[0x11] = 5;
    p[0x12] = 0;
    p[0x14] = 0;
    p[0x15] = 0;
    p[0x16] = 0;
    p[0x1C] = 0;
    p[0x13] = 0;
    p[0x19] = 0;
    p[0x1A] = 0;
    p[0x1B] = 0;
    p[0x17] = 0;
    p[0x18] = 0;
    p[0x1D] = 0;
    p[0x10] = 1;
    *(u16 *)(p + 0x02) = 0;
    p[0x04] = 0;
    p[0x05] = 0;
    for (i = 0; i < 10; i++) {
        p[0x06 + i] = 0;
    }
}

/* InitializePlayerState @ blb/lvlload: a byte-identical sibling of
 * initPlayerState (same field writes verified via m2c). Shares the body. */
void InitializePlayerState(void *state) {
    initPlayerState(state);
}
