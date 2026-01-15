#include "common.h"

/**
 * GetLevelCount - Get the total number of levels from BLB header
 * 
 * Reads the level count byte at header offset 0xF31.
 * For PAL version, this returns 26 (including MENU as level 0).
 * 
 * GameState Structure:
 *   +0x5C  headerBuffer - Pointer to BLB header in RAM (0x800AE3E0 PAL)
 * 
 * BLB Header:
 *   +0xF31  Level count (u8) - value is 26
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Number of levels defined in BLB header
 */
u8 GetLevelCount(void *gameState) {
    return ((u8 *)(*(s32 *)((u8 *)gameState + 0x5C)))[0xF31];
}
