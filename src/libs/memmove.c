#include "common.h"

void *func_80082F94(u8 *dst, u8 *src, s32 len) {
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
