/* =============================================================================
 * tile_attrib.c  --  PC-port tile-attribute state init (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/InitTileAttributeState.s
 * (0x80024CF4, 0xD0 bytes). If the level carries a tile-attribute layer
 * (HasTileAttributes), it fetches the attribute's "unknown" pair and its
 * dimensions, resolves the attribute data pointer, and stores all three into the
 * LevelDataContext (GameState+0x84):
 *   +0x68 = GetTileAttributeData(ctx, width, height)  (data pointer, 0 if none)
 *   +0x6C = unknown pair  (2 x u16 from GetTileAttributeUnknown)
 *   +0x70 = dimensions    (2 x u16 from GetTileAttributeDimensions)
 * The original's lwl/lwr/swl/swr sequence is just the compiler copying the two
 * u16 pairs through unaligned words; the net effect is the four halfword stores
 * below.
 * ========================================================================== */
#include "common.h"

extern s8   HasTileAttributes(void *ctx);
extern void GetTileAttributeUnknown(u16 *out, void *ctx);
extern void GetTileAttributeDimensions(u16 *out, void *ctx);
extern s32  GetTileAttributeData(void *ctx, u16 width, u16 height);

void InitTileAttributeState(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *ctx = gs + 0x84;

    if (HasTileAttributes(ctx) & 0xFF) {
        u16 unknown[2];
        u16 dims[2];
        GetTileAttributeUnknown(unknown, ctx);
        GetTileAttributeDimensions(dims, ctx);
        *(s32 *)(gs + 0x68) = GetTileAttributeData(ctx, dims[0], dims[1]);
        *(u16 *)(gs + 0x6C) = unknown[0];
        *(u16 *)(gs + 0x6E) = unknown[1];
        *(u16 *)(gs + 0x70) = dims[0];
        *(u16 *)(gs + 0x72) = dims[1];
    } else {
        *(s32 *)(gs + 0x68) = 0;
    }
}
