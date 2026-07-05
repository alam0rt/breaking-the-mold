/* =============================================================================
 * tile_attr_query.c  --  PC-port tile-attribute lookup (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/GetTileAttributeAtPosition.s
 * (reference = export/SLES_010.90.c 20244). Returns the tile-attribute byte at a
 * world pixel position by indexing the attribute grid installed by
 * InitTileAttributeState (GameState fields: +0x68 data ptr, +0x6C/+0x6E origin
 * X/Y in tiles, +0x70/+0x72 grid width/height in tiles). Pixel->tile is a >>4
 * (16px tiles) minus the grid origin; out-of-range or absent grid returns 0.
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

u8 GetTileAttributeAtPosition(s32 levelData, s32 worldX, s32 worldY) {
    u8 *ld = (u8 *)(uintptr_t)levelData;

    if (*(s32 *)(ld + 0x68) != 0) {
        s32 tileX = (s16)(((worldX << 16) >> 20) - (u32)*(u16 *)(ld + 0x6C));
        if (tileX >= 0 && tileX < *(s16 *)(ld + 0x70)) {
            s32 tileY = (s16)(((worldY << 16) >> 20) - (u32)*(u16 *)(ld + 0x6E));
            if (tileY < 0) {
                return 0;
            }
            if (tileY < *(s16 *)(ld + 0x72)) {
                return *(u8 *)((uintptr_t)*(s32 *)(ld + 0x68)
                               + tileY * *(s16 *)(ld + 0x70) + tileX);
            }
        }
    }
    return 0;
}
