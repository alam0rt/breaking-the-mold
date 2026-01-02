#include "common.h"

/**
 * CreditsAccessors.c - BLB Header Credits/Movie/Sector Accessor Functions
 * 
 * These functions provide access to movie, credits, and sector data based
 * on the current game mode. The mode is stored in a sliding window of
 * state bytes within the header.
 * 
 * BLB Header Layout (relevant sections):
 *   0xB60 - 0xCCB  Movie table (10 entries × 0x1C bytes)
 *   0xCC0 - 0xECF  Sector offset table (16 entries × 0x10 bytes)
 *   0xF10 - 0xF30  Credits table (entries × 0x0C bytes)
 *   0xF31          Level count
 *   0xF32          Asset count
 *   0xF34 - 0xFFF  State data (sliding window mode/index pairs)
 * 
 * State Sliding Window:
 *   The game uses paired bytes to track current mode and table index:
 *   - Mode byte at 0xF36 + stateOffset
 *   - Index byte at 0xF92 + stateOffset
 * 
 *   Mode values:
 *     1 = Movie playback mode
 *     2 = Credits display mode  
 *     3 = Level loading mode
 *     4-5 = Sector/asset loading mode
 * 
 * Movie Entry Structure (0x1C = 28 bytes each, at 0xB60):
 *   +0x00  Reserved (u16) - always 0
 *   +0x02  Sector count (u16)
 *   +0x04  Reserved data (4 bytes)
 *   +0x09  Short name (5 chars)
 *   +0x0C  Filename string (null-terminated)
 * 
 * Credits Entry Structure (0x0C = 12 bytes each, at 0xF10):
 *   +0x08  Sector offset (u16)
 *   +0x0A  Sector count (u16)
 * 
 * Sector Table Entry Structure (0x10 = 16 bytes each, at 0xCC0):
 *   +0x0C  Sector offset (u16)
 *   +0x0E  Sector count (u16)
 * 
 * GameState Structure Offsets:
 *   +0x5C  headerBuffer - Pointer to BLB header in RAM
 *   +0x60  stateOffset - Current offset into state arrays (sliding window)
 */

/**
 * GetCurrentSectorOffset - Get sector offset for current game mode
 * 
 * Based on the current mode (credits or level/sector), returns the
 * starting sector offset from the appropriate table.
 * 
 * Mode 2 (Credits): Reads from credits table at 0xF18 + (index × 0x0C)
 * Mode 4-5 (Level): Reads from sector table at 0xCCC + (index × 0x10)
 * Other modes: Returns 0
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Sector offset, or 0 if not in valid mode
 */
#ifdef NON_MATCHING
u16 GetCurrentSectorOffset(void *gameState) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    u8 stateOffset = *((u8 *)gameState + 0x60);
    u8 *statePtr = headerBuffer + stateOffset;
    u8 mode = statePtr[0xF36];
    u8 tableIndex;
    
    if (mode == 2) {
        /* Credits mode - read from credits table */
        tableIndex = statePtr[0xF92];
        return *(u16 *)(headerBuffer + (tableIndex * 0x0C) + 0xF18);
    }
    if (mode >= 4 && mode < 6) {
        /* Level/sector mode - read from sector table */
        tableIndex = statePtr[0xF92];
        return *(u16 *)(headerBuffer + (tableIndex * 0x10) + 0xCCC);
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetCurrentSectorOffset);
#endif

/**
 * GetCurrentSectorCount - Get sector count for current game mode
 * 
 * Similar to GetCurrentSectorOffset but returns the sector count
 * (reads from offset +2 in each table entry).
 * 
 * Mode 2 (Credits): Reads from credits table at 0xF1A + (index × 0x0C)
 * Mode 4-5 (Level): Reads from sector table at 0xCCE + (index × 0x10)
 * Other modes: Returns 0
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Sector count, or 0 if not in valid mode
 */
#ifdef NON_MATCHING
u16 GetCurrentSectorCount(void *gameState) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    u8 stateOffset = *((u8 *)gameState + 0x60);
    u8 *statePtr = headerBuffer + stateOffset;
    u8 mode = statePtr[0xF36];
    u8 tableIndex;
    
    if (mode == 2) {
        /* Credits mode - read sector count from credits table */
        tableIndex = statePtr[0xF92];
        return *(u16 *)(headerBuffer + (tableIndex * 0x0C) + 0xF1A);
    }
    if (mode >= 4 && mode < 6) {
        /* Level/sector mode - read sector count from sector table */
        tableIndex = statePtr[0xF92];
        return *(u16 *)(headerBuffer + (tableIndex * 0x10) + 0xCCE);
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetCurrentSectorCount);
#endif

/**
 * GetAssetCount - Get total asset count from BLB header
 * 
 * Reads the asset count byte at header offset 0xF32.
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Number of assets defined in BLB header
 */
#ifdef NON_MATCHING
u8 GetAssetCount(void *gameState) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    return headerBuffer[0xF32];
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetAssetCount);
#endif

/**
 * GetMovieReservedByIndex - Get pointer to movie's reserved field by index
 * 
 * Returns pointer to the reserved field (+0x04) of a movie entry,
 * given an explicit movie index.
 * 
 * @param gameState   Pointer to GameState structure
 * @param movieIndex  Movie table index (0-9)
 * @return            Pointer to Movie[movieIndex]+0x04
 */
#ifdef NON_MATCHING
s32 GetMovieReservedByIndex(void *gameState, s32 movieIndex) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    /* Movie table at 0xB60, each entry 0x1C bytes, reserved at +0x04 */
    return (s32)(headerBuffer + 0xB64 + ((movieIndex & 0xFF) * 0x1C));
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetMovieReservedByIndex);
#endif

/**
 * GetMovieShortNameByIndex - Get pointer to movie's short name by index
 * 
 * Returns pointer to the short name field (+0x09) of a movie entry,
 * given an explicit movie index. Short names are typically 4-5 chars.
 * 
 * @param gameState   Pointer to GameState structure
 * @param movieIndex  Movie table index (0-9)
 * @return            Pointer to Movie[movieIndex]+0x09
 */
#ifdef NON_MATCHING
s32 GetMovieShortNameByIndex(void *gameState, s32 movieIndex) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    /* Movie table at 0xB60, each entry 0x1C bytes, short name at +0x09 */
    return (s32)(headerBuffer + 0xB69 + ((movieIndex & 0xFF) * 0x1C));
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetMovieShortNameByIndex);
#endif

/**
 * GetCurrentMovieReserved - Get pointer to current movie's reserved field
 * 
 * Uses the state sliding window to get the current movie index,
 * then returns a pointer to that movie's reserved field.
 * Only valid when mode == 1 (movie playback mode).
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Pointer to current movie's reserved field, or 0
 */
#ifdef NON_MATCHING
s32 GetCurrentMovieReserved(void *gameState) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    u8 stateOffset = *((u8 *)gameState + 0x60);
    u8 *statePtr = headerBuffer + stateOffset;
    
    if (statePtr[0xF36] == 1) {
        /* Movie mode - get current movie's reserved field */
        u8 movieIndex = statePtr[0xF92];
        return (s32)(headerBuffer + 0xB64 + (movieIndex * 0x1C));
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetCurrentMovieReserved);
#endif

/**
 * GetCurrentMovieShortName - Get pointer to current movie's short name
 * 
 * Uses the state sliding window to get the current movie index,
 * then returns a pointer to that movie's short name field.
 * Only valid when mode == 1 (movie playback mode).
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Pointer to current movie's short name, or 0
 */
#ifdef NON_MATCHING
s32 GetCurrentMovieShortName(void *gameState) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    u8 stateOffset = *((u8 *)gameState + 0x60);
    u8 *statePtr = headerBuffer + stateOffset;
    
    if (statePtr[0xF36] == 1) {
        /* Movie mode - get current movie's short name */
        u8 movieIndex = statePtr[0xF92];
        return (s32)(headerBuffer + 0xB69 + (movieIndex * 0x1C));
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetCurrentMovieShortName);
#endif

/**
 * GetCurrentMovieFilename - Get pointer to current movie's filename
 * 
 * Uses the state sliding window to get the current movie index,
 * then returns a pointer to that movie's filename string.
 * Only valid when mode == 1 (movie playback mode).
 * 
 * The filename is a null-terminated string used to construct
 * the path for loading movie data from the BLB archive.
 * 
 * @param gameState  Pointer to GameState structure
 * @return           Pointer to current movie's filename, or 0
 */
#ifdef NON_MATCHING
s32 GetCurrentMovieFilename(void *gameState) {
    u8 *headerBuffer = *(u8 **)((u8 *)gameState + 0x5C);
    u8 stateOffset = *((u8 *)gameState + 0x60);
    u8 *statePtr = headerBuffer + stateOffset;
    
    if (statePtr[0xF36] == 1) {
        /* Movie mode - get current movie's filename */
        u8 movieIndex = statePtr[0xF92];
        return (s32)(headerBuffer + 0xB6C + (movieIndex * 0x1C));
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CreditsAccessors", GetCurrentMovieFilename);
#endif
