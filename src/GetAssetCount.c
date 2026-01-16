#include "common.h"

/**
 * GetAssetCount - Get the number of assets/movies from BLB header
 * Address: 0x8007ACDC | Size: 0x14
 * 
 * Returns the value at header offset 0xF32 (movie/asset count = 13 for GAME.BLB)
 */
INCLUDE_ASM("asm/pal/nonmatchings/GetAssetCount", GetAssetCount);
