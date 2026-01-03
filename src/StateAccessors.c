#include "common.h"

/**
 * StateAccessors.c - BLB Header Level Accessor Functions
 * 
 * These functions provide access to level metadata stored in the BLB header.
 * The level table starts at offset 0x000 in the header, with each entry
 * being 0x70 (112) bytes.
 * 
 * BLB Header Layout (relevant sections):
 *   0x000 - 0xB5F  Level metadata table (26 entries × 0x70 bytes)
 *   0xF31          Level count (u8)
 *   0xF32          Asset count (u8)
 * 
 * Level Entry Structure (0x70 bytes each):
 *   +0x00  Sector offset (u16)
 *   +0x02  Sector count (u16)
 *   +0x04  Unknown data
 *   +0x08  Static data pointer info
 *   +0x0C  Level asset index (u8) - used for asset loading
 *   +0x56  Display flags/data (5 bytes)
 *   +0x5B  Level name string (null-terminated, max 21 chars)
 * 
 * GameState Structure Offsets:
 *   +0x5C  headerBuffer - Pointer to BLB header in RAM (0x800AE3E0 PAL)
 */

/**
 * GetLevelCount - Get the total number of levels from BLB header
 * 
 * Reads the level count byte at header offset 0xF31.
 * For PAL version, this returns 26 (including MENU as level 0).
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Number of levels defined in BLB header
 */
INCLUDE_ASM("asm/pal/nonmatchings/StateAccessors", GetLevelCount);

/**
 * GetLevelAssetIndex - Get the asset index for a specific level
 * 
 * Each level entry has an asset index at offset +0x0C that determines
 * which asset set to load for that level. This is used by the asset
 * loading system to find the correct graphics, sounds, etc.
 * 
 * @param gameState   Pointer to GameState structure
 * @param levelIndex  Level entry number (0-25)
 * @return            Asset index for the specified level
 */
INCLUDE_ASM("asm/pal/nonmatchings/StateAccessors", GetLevelAssetIndex);

/**
 * GetLevelDisplayName - Get pointer to level's display name field
 * 
 * Returns a pointer to the display name/flags area at offset +0x56
 * within a level entry. The actual level name string starts at +0x5B.
 * 
 * @param gameState   Pointer to GameState structure
 * @param levelIndex  Level entry number (0-25)
 * @return            Pointer to display data at level[n]+0x56
 */
INCLUDE_ASM("asm/pal/nonmatchings/StateAccessors", GetLevelDisplayName);

char *getLevelName(void *arg0, u8 levelIndex) {
    return (char *)(*(s32 *)((u8 *)arg0 + 0x5C) + (levelIndex * 0x70) + 0x5B);
}
