/* =============================================================================
 * CLUTPaletteCycleTickCallback.c  --  PC-port palette-cycle animation tick
 * =============================================================================
 * Faithful transcription of CLUTPaletteCycleTickCallback (0x8001C..., INCLUDE_ASM
 * in src/layer.c). Per-frame tick on a sprite render context: counts down the
 * +0x39 timer, and on wrap rotates the CLUT palette entries in [+0x35..+0x36]
 * by one (direction from +0x37), re-uploads the CLUT, and reloads the timer
 * from +0x38. Installed by SetTexturePageParams.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <string.h>

extern void UploadTextureOrClut(u8 *c, void *data);

void CLUTPaletteCycleTickCallback(void *arg0) {
    u8 *c = arg0;
    u8 count = c[0x39] - 1;

    c[0x39] = count;
    if ((count & 0xFF) != 0) {
        return;
    }

    {
        u16 *clut = *(u16 **)(c + 0x1C);
        if (clut != NULL) {
            u8 start = c[0x35];
            u8 end = c[0x36];
            u16 saved;
            u8 dst;

            if (c[0x37] == 0) {
                saved = clut[start];
                memmove(&clut[start], &clut[start + 1], (end - start) * 2);
                dst = end;
            } else {
                saved = clut[end];
                memmove(&clut[start + 1], &clut[start], (end - start) * 2);
                dst = start;
            }
            clut[dst] = saved;
            UploadTextureOrClut(c, clut);
        }
        c[0x39] = c[0x38];
    }
}
