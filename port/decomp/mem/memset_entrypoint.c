#include "common.h"

/*
 * memset_entrypoint - the game's internal memset (first function in .text).
 *
 * Faithful transcription of export/SLES_010.90.c. Fills `count` bytes at `dst`
 * with byte `fill`, 16 bytes at a time then a byte tail. Note the original
 * quirk: the single `*(char*)dst = fill` after the block loop executes
 * unconditionally, so memset_entrypoint(dst, 0, fill) still writes one byte.
 *
 * Signature: void memset_entrypoint(void *dst, int count, int fill)
 */
void memset_entrypoint(void *dst, int count, int fill) {
    unsigned int uVar1;

    uVar1 = (fill & 0xffU) << 8 | (fill & 0xffU);
    uVar1 = uVar1 | uVar1 << 0x10;
    while (-1 < count + -0x10) {
        *(unsigned int *)dst = uVar1;
        *(unsigned int *)((char *)dst + 4) = uVar1;
        *(unsigned int *)((char *)dst + 8) = uVar1;
        *(unsigned int *)((char *)dst + 0xc) = uVar1;
        dst = (void *)((char *)dst + 0x10);
        count = count + -0x10;
    }
    *(char *)dst = (char)fill;
    if (count != 0) {
        while (1) {
            dst = (void *)((char *)dst + 1);
            count = count + -1;
            if (count == 0) {
                break;
            }
            *(char *)dst = (char)fill;
        }
    }
}
