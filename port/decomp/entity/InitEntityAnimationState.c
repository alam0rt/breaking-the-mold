/* =============================================================================
 * InitEntityAnimationState.c  --  PC-port entity animation-state init (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entity/InitEntityAnimationState.s
 * (0x8001C980, 0xE0 bytes; reference = export/SLES_010.90.c 12933). Clears the
 * entity's animation/render-state region (roughly +0x90..+0xFF plus a few
 * lower velocity/marker fields), sets the default anim flags (doubleFrameDelay,
 * visibility, animLoopFlag, animActive = 1), and allocates a 0x18-byte sprite-
 * asset descriptor from the BLB heap into +0x90.
 *
 * Offsets/widths transcribed directly from the disassembly. Defaults set to 1:
 *   +0xFE, +0xF6, +0xF1, +0xF2. Everything else in the touched set is zeroed.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8 *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);

void InitEntityAnimationState(void *entity) {
    u8 *e = (u8 *)entity;

    e[0xFE] = 1;
    e[0xFD] = 0;
    e[0xF6] = 1;
    e[0xF7] = 0;
    e[0xF8] = 0;
    *(s16 *)(e + 0x6C) = 0;
    *(s16 *)(e + 0x6E) = 0;
    *(s32 *)(e + 0xCC) = 0;
    *(s16 *)(e + 0xDA) = 0;
    *(s16 *)(e + 0xDC) = 0;
    *(s16 *)(e + 0xDE) = 0;
    e[0xF0] = 0;
    e[0xF1] = 1;
    e[0xF2] = 1;
    *(s16 *)(e + 0xEC) = 0;
    *(s32 *)(e + 0xB4) = 0;
    *(s32 *)(e + 0xB8) = 0;
    *(s16 *)(e + 0xE6) = 0;
    *(s16 *)(e + 0xE8) = 0;
    *(s16 *)(e + 0xE0) = 0;
    *(s32 *)(e + 0x94) = 0;
    *(s16 *)(e + 0x98) = 0;
    *(s16 *)(e + 0x9A) = 0;
    *(s32 *)(e + 0x9C) = 0;
    *(s16 *)(e + 0xA0) = 0;
    *(s16 *)(e + 0xA2) = 0;
    *(s32 *)(e + 0xA4) = 0;
    *(s16 *)(e + 0xA8) = 0;
    *(s16 *)(e + 0xAA) = 0;
    *(s32 *)(e + 0xAC) = 0;
    *(s32 *)(e + 0xB0) = 0;
    *(s32 *)(e + 0xD4) = 0;
    *(u8 **)(e + 0x90) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x18, 1, 0);
    *(s32 *)(e + 0xD0) = 0;
    *(s16 *)(e + 0xEA) = 0;
    e[0xF9] = 0;
    *(s16 *)(e + 0xEE) = 0;
    e[0xFA] = 0;
    e[0xFB] = 0;
    e[0xFC] = 0;
}
