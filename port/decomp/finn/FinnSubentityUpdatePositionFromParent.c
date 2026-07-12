/* =============================================================================
 * FinnSubentityUpdatePositionFromParent.c  --  functional-C body for finn.c
 * FinnSubentityUpdatePositionFromParent (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/finn/FinnSubentityUpdatePositionFromParent.s
 * (0x8006E008, 0x7C). Per-frame tick for parent-following companion entities
 * (the halo; Finn-mode subentities): copy the parent's render scale (+0x58 ->
 * +0x28) and position (worldX -> +0x20, worldY + sprite Y offset +0x3A ->
 * +0x22, unaligned read as in CreateHaloEntity); when the self-destruct flag
 * at +0x2C is set, disable the companion's icon prim (+0x24, enabled byte
 * +0xA).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

void FinnSubentityUpdatePositionFromParent(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *parent = *(u8 **)(e + 0x1C);
    u16 yoff = (u16)(parent[0x3A] | (parent[0x3B] << 8));

    *(s32 *)(e + 0x28) = *(s32 *)(parent + 0x58);
    *(u16 *)(e + 0x20) = *(u16 *)(parent + 0x68);
    *(s16 *)(e + 0x22) = (s16)(*(s16 *)(parent + 0x6A) + yoff);
    if (e[0x2C] != 0) {
        (*(u8 **)(e + 0x24))[0xA] = 0;
    }
}
