#include "common.h"

/* Hand-rolled libc-style memmove (PSY-Q runtime, byte-by-byte, overlap-safe):
 * backward copy from the tail when dst >= src, forward copy otherwise.
 * Byte-matched; the C-level `return dst` in the forward path is an m2c
 * reconstruction artifact — the original asm returns the un-mutated
 * destination pointer. Ghidra carved the shared return-dest epilogue into
 * a phantom `MemmoveReturnDest` @ 0x80082FF8, which is not a real separate
 * function (treat as part of memmove). */
void *memmove(u8 *dst, u8 *src, s32 len) {
    u8 *dstEnd;
    u8 *srcEnd;

    if (dst >= src) {
        if (len-- > 0) {
            dstEnd = (u8 *)(len + (s32)dst);
            srcEnd = (u8 *)(len + (s32)src);

            do {
                *dstEnd-- = *srcEnd--;
            } while (len-- > 0);
        }
    } else {
        if (len-- > 0) {
            do {
                *dst++ = *src++;
            } while (len-- > 0);
        }
    }

    return dst;
}
