/* =============================================================================
 * tile_colors.c  --  PC-port level secondary-colour loader (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/LoadSecondaryColorFromTile
 * Header.s (0x800246D0, 24 lines). Sibling of the matched-C LoadBGColorFromTile
 * Header (src/blb.c): copies the level's SECONDARY RGB triple from the tile
 * header (via GetSecondaryColorPtr, matched C in blbacc.c) into the GameState at
 * +0x127..+0x129. Unlike the background-colour loader it does NOT stamp a
 * "valid" flag byte -- it is a straight 3-byte copy.
 * ========================================================================== */
#include "common.h"
#include "Game/level_data_context.h"

extern u8 *GetSecondaryColorPtr(LevelDataContext *ctx);

void LoadSecondaryColorFromTileHeader(u8 *entity) {
    u8 *ptr = GetSecondaryColorPtr((LevelDataContext *)(entity + 0x84));
    entity[0x127] = ptr[0];
    entity[0x128] = ptr[1];
    entity[0x129] = ptr[2];
}
