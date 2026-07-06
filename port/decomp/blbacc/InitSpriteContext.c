/* =============================================================================
 * InitSpriteContext.c  --  PC-port sprite-header parser (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blbacc/InitSpriteContext.s
 * (0x8007BC3C, 0x1AC bytes; reference = export/SLES_010.90.c 107731). Resolves a
 * sprite by id (LookupSpriteById -> the sprite asset header), finds its entry in
 * the header's id table, and fills the 20-byte SpriteContext:
 *   +0x00 frame_metadata  = header + header[+2] (header_size) + entry[+6]
 *   +0x04 secondary_ptr   = header + header[+8]        (only if non-zero)
 *   +0x08 rle_data        = header + header[+4]
 *   +0x0C max_width       = max frame width  (scan)
 *   +0x0E max_height      = max frame height (scan)
 *   +0x10 total_frame_cnt = entry[+4]
 *   +0x12 spriteLookupByte= (u8)entry[+8]
 *   +0x13 is_valid        = 1 on success (0 return on miss)
 *
 * Id table: entries at header+0xC, stride 0xC, entry[+0] = u32 sprite id.
 * Frame table: entries at frame_metadata, stride 0x24; each frame's width is the
 * s16 at +0xA and height the s16 at +0xC (the asm reads these via unaligned
 * words; the redundant unaligned load of frame+6 is a dead store and omitted).
 * Returns 1 when a valid sprite was resolved, else 0.
 * ========================================================================== */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>

extern void *LookupSpriteById(u32 spriteId);

s32 InitSpriteContext(void *param_1, u32 spriteId) {
    u8 *ctx = (u8 *)param_1;
    u8 *header;
    u16 count;
    u32 idx;
    u8 *entry;
    s32 t0;
    s32 off;

    *(s32 *)(ctx + 0x00) = 0;
    *(s32 *)(ctx + 0x04) = 0;
    *(s32 *)(ctx + 0x08) = 0;
    *(s16 *)(ctx + 0x0C) = 0;
    *(s16 *)(ctx + 0x0E) = 0;
    *(s16 *)(ctx + 0x10) = 0;
    ctx[0x12] = 0;
    ctx[0x13] = 0;

    header = (u8 *)LookupSpriteById(spriteId);
    if (header == NULL) {
        if (getenv("PORT_SPRITE_DEBUG") != NULL) {
            fprintf(stderr, "[port] sprite: InitSpriteContext MISS id=%08X\n",
                    spriteId);
        }
        return 0;
    }
    count = *(u16 *)(header + 0x0);
    if (count == 0) {
        return 0;
    }

    idx = 0;
    entry = header + 0xC;
    while (*(u32 *)entry != spriteId) {
        idx++;
        if (idx >= count) {
            break;
        }
        entry += 0xC;
    }
    if (idx >= *(u16 *)(header + 0x0)) {
        return 0;
    }

    entry = header + 0xC + idx * 0xC;
    *(u16 *)(ctx + 0x10) = *(u16 *)(entry + 0x4);
    *(void **)(ctx + 0x00) = header + *(u16 *)(header + 0x2) + *(u16 *)(entry + 0x6);
    *(void **)(ctx + 0x08) = header + *(u32 *)(header + 0x4);
    ctx[0x12] = (u8)*(u16 *)(entry + 0x8);
    if (*(u32 *)(header + 0x8) != 0) {
        *(void **)(ctx + 0x04) = header + *(u32 *)(header + 0x8);
    }

    off = 0;
    for (t0 = 0; t0 < *(u16 *)(ctx + 0x10); t0++) {
        u8 *frame = *(u8 **)(ctx + 0x00) + off;
        s16 fw = *(s16 *)(frame + 0xA);
        s16 fh = *(s16 *)(frame + 0xC);
        if (*(s16 *)(ctx + 0xC) < fw) {
            *(s16 *)(ctx + 0xC) = fw;
        }
        if (*(s16 *)(ctx + 0xE) < fh) {
            *(s16 *)(ctx + 0xE) = fh;
        }
        off += 0x24;
    }
    ctx[0x13] = 1;
    return 1;
}
