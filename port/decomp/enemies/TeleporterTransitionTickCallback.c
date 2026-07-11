/* =============================================================================
 * TeleporterTransitionTickCallback.c  --  functional-C body for enemies.c
 * TeleporterTransitionTickCallback (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/TeleporterTransitionTickCallback.s
 * (0x80045314, 0xFC). Tick installed by CreateCameraEntity: runs the teleporter
 * arrival/departure scale animation off the byte countdown at +0x100 (seeded
 * 0x5A by the constructor).
 *
 * While the countdown is >= 0x3C it eases the X scale (+0x50/+0x54) from 0 up
 * to the base scale snapshot (+0x104); once it drops below 0x20 it pulses the
 * scale through the 8-entry 8.8 factor table D_8009B6E4 (countdown & 7). At 0
 * it transitions the entity state to {D_800A5B28, D_800A5B2C} = the
 * TeleporterExitState pair (both strong C data in src/enemies.c). The live
 * scale is mirrored into the linked entity at +0x108 if present, then the
 * normal sprite update runs.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void EntitySetState(void *e, s32 marker, s32 fn);
extern void EntityUpdateCallback(void *e);
extern s32  D_800A5B28 asm("D_800A5B28");
extern s32  D_800A5B2C asm("D_800A5B2C");

/* Full 0xC0-byte data blob at 0x8009B6E4 (asm/data/8BE34.data.s), verbatim.
 * Only the first 8 words (the 8.8 scale-pulse factors) are indexed here
 * (countdown & 7); the tail words are kept so the blob stays byte-faithful. */
static const unsigned int s_telepScalePulse[48] asm("D_8009B6E4") = {
    0x00000100, 0x000000FC, 0x000000F0, 0x000000E4,
    0x000000E0, 0x000000E4, 0x000000F0, 0x000000FC,
    0x40C21C10, 0x40DA1C10, 0x40EA1C10, 0x408A1C10,
    0x404A1C10, 0x41CA1C10, 0x42CA1C10, 0x44CA1C10,
    0x48CA1C10, 0x40C22C10, 0x40DA2C10, 0x40EA2C10,
    0x408A2C10, 0x404A2C10, 0x41CA2C10, 0x42CA2C10,
    0x44CA2C10, 0x48CA2C10, 0x50CA2C10, 0x80CA0430,
    0x80CA1C30, 0x80CA2C30, 0x80CA4C30, 0x80CA8C30,
    0x80CB0C30, 0x80C80C30, 0x80CE0C30, 0x80C20C30,
    0x80DA0C30, 0x80CA1C50, 0x80CA2C50, 0x80CA4C50,
    0x80CA8C50, 0x80CB0C50, 0x80C80C50, 0x80CE0C50,
    0x80C20C50, 0x80DA0C50, 0x80EA0C50, 0x80CA2C90,
};

void TeleporterTransitionTickCallback(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 count = e[0x100];

    if (count != 0) {
        count--;
        e[0x100] = count;
        if (count < 0x20) {
            s32 scaled = ((s32)s_telepScalePulse[count & 7] *
                          *(s32 *)(e + 0x104)) >> 8;
            *(s32 *)(e + 0x50) = scaled;
            *(s32 *)(e + 0x54) = scaled;
        } else if (count >= 0x3C) {
            s32 base = *(s32 *)(e + 0x104);
            s32 eased = base - (s32)(count - 0x3C) * (base / 30);
            *(s32 *)(e + 0x50) = eased;
            *(s32 *)(e + 0x54) = eased;
        }
    } else {
        EntitySetState(e, D_800A5B28, D_800A5B2C);
    }

    {
        u8 *linked = *(u8 **)(e + 0x108);
        if (linked != NULL) {
            s32 s = *(s32 *)(e + 0x50);
            *(s32 *)(linked + 0x50) = s;
            *(s32 *)(linked + 0x54) = s;
        }
    }
    EntityUpdateCallback(e);
}
