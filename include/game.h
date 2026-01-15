/* =============================================================================
 * Game Runtime Structures
 * =============================================================================
 * Runtime data structures for game state, entities, and level loading.
 * Based on verified offsets from Ghidra analysis and PCSX-Redux MCP dumps.
 * =============================================================================
 */

#ifndef GAME_H
#define GAME_H

#include "common.h"
#include "blb.h"

/* -----------------------------------------------------------------------------
 * Player State Structure (size unknown, at least 0x1E bytes)
 * Verified via initPlayerState @ 0x8001F3B4
 * -------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u8      lives;             // Number of lives
    /* 0x01 */ u8      continues;         // Number of continues
    /* 0x02 */ s16     unknown02;         // Cleared by init
    /* 0x04 */ u8      unknown04;
    /* 0x05 */ u8      unknown05;
    /* 0x06 */ u8      levelUnlockFlags[10]; // Level unlock bitfield (cleared by init)
    /* 0x10 */ u8      unknown10;         // Set to 1 by init
    /* 0x11 */ u8      health;            // Health counter (init = 5)
    /* 0x12 */ u8      unknown12;
    /* 0x13 */ u8      unknown13;
    /* 0x14 */ u8      unknown14;
    /* 0x15 */ u8      unknown15;
    /* 0x16 */ u8      unknown16;
    /* 0x17 */ u8      unknown17;
    /* 0x18 */ u8      unknown18;
    /* 0x19 */ u8      unknown19;
    /* 0x1A */ u8      unknown1A;
    /* 0x1B */ u8      unknown1B;
    /* 0x1C */ u8      unknown1C;
    /* 0x1D */ u8      unknown1D;
} PlayerState;  // Size: >= 0x1E bytes

/* -----------------------------------------------------------------------------
 * Level Data Context (128 bytes @ GameState+0x84, PAL: 0x8009DCC4)
 * Populated by LoadAssetContainer @ 0x8007B074
 * -------------------------------------------------------------------------- */

typedef struct {
    /* Asset Pointers - Populated by LoadAssetContainer */
    /* 0x00 */ s32     subBlockFlag;      // Set to stageIndex or 1
    /* 0x04 */ TileHeader* tileHeader;    // Asset 100 (36 bytes)
    /* 0x08 */ VRAMSlotConfig* vramSlotConfig; // Asset 101 (12 bytes)
    /* 0x0C */ void*   tilemapContainer;  // Asset 200
    /* 0x10 */ LayerEntry* layerEntries;  // Asset 201 (92 bytes each)
    /* 0x14 */ void*   tilePixels;        // Asset 300 (8bpp indexed)
    /* 0x18 */ u8*     paletteIndices;    // Asset 301 (1 byte per tile)
    /* 0x1C */ u8*     tileSizeFlags;     // Asset 302 (flags: 0x01=semi-trans, 0x02=8x8, 0x04=skip)
    /* 0x20 */ void*   paletteContainer;  // Asset 400
    /* 0x24 */ void*   paletteAnimData;   // Asset 401
    /* 0x28 */ void*   animatedTileData;  // Asset 303
    /* 0x2C */ u8*     tileAttributes;    // Asset 500 (collision map, 1 byte per tile)
    /* 0x30 */ void*   animOffsets;       // Asset 503 (ToolX sequence data)
    /* 0x34 */ void*   vehicleData;       // Asset 504 (FINN/RUNN levels only)
    /* 0x38 */ EntityDefinition* entityData; // Asset 501 (24-byte structs)
    /* 0x3C */ void*   vramRects;         // Asset 502 (texture page definitions)
    /* 0x40 */ void*   levelGeometry;     // Asset 600 (sprites or geometry)
    /* 0x44 */ u32     levelGeometrySize; // Asset 600 size
    /* 0x48 */ void*   audioSamples;      // Asset 601 (SPU ADPCM)
    /* 0x4C */ u32     audioSamplesSize;  // Asset 601 size
    /* 0x50 */ void*   paletteData;       // Asset 602 (15-bit PSX colors)
    /* 0x54 */ void*   spuAudioData;      // Asset 700 (additional audio)
    /* 0x58 */ u32     spuAudioDataSize;  // Asset 700 size
    
    /* Context State */
    /* 0x5C */ BLBHeader* blbHeaderBuffer; // → 0x800AE3E0 (PAL)
    /* 0x60 */ u8      slidingWindowIndex; // Playback sequence index
    /* 0x61 */ u8      padding61[3];
    /* 0x64 */ void*   loaderCallback;    // → 0x80020848
    /* 0x68 */ void*   primaryDataBuffer; // Primary TOC buffer
    /* 0x6C */ void*   secondaryDataBuffer; // Secondary buffer
    
    /* Primary TOC Assets */
    /* 0x70 */ void*   primaryLevel600;   // Primary Asset 600
    /* 0x74 */ void*   primaryAudio601;   // Primary Asset 601
    /* 0x78 */ u32     primaryAudio601Size;
    /* 0x7C */ void*   primaryAudioMeta602; // Primary Asset 602
} LevelDataContext;  // Size: 0x80 (128 bytes)

/* -----------------------------------------------------------------------------
 * Entity Runtime Structure (size unknown)
 * -------------------------------------------------------------------------- */

typedef struct Entity Entity;  // Forward declaration

struct Entity {
    /* 0x00+ */ void*  callback;         // Entity type callback function
    /* Unknown layout - to be discovered */
    /* References to EntityDefinition data (position, type, etc.) */
};

/* -----------------------------------------------------------------------------
 * Game State Structure (size unknown, base @ 0x8009DC40 for PAL)
 * Verified via multiple accessor functions
 * -------------------------------------------------------------------------- */

typedef struct {
    /* 0x00-0x1B */ u8      unknown00[0x1C];
    
    /* Entity Management */
    /* 0x1C */ Entity* entityTickList;    // Z-sorted tick list (EntityTickLoop iterates)
    /* 0x20 */ Entity* entityRenderList;  // Z-sorted render list (RenderEntities iterates)
    /* 0x24 */ Entity* entityCollisionQueue; // Collision/update queue
    /* 0x28 */ EntityDefinition* entityDefPool; // Raw 24-byte defs from Asset 501
    /* 0x2C */ Entity* playerEntityAlt;   // Alternate player reference
    /* 0x30 */ Entity* playerEntity;      // Main player entity pointer
    
    /* 0x34-0x7B */ u8      unknown34[0x48];
    
    /* 0x7C */ void**  entityTypeCallbacks; // → 0x8009D5F8 (121 entries × 8 bytes)
    
    /* 0x80-0x83 */ u8      unknown80[4];
    
    /* 0x84 */ LevelDataContext levelContext; // 128 bytes
    
    /* 0x104+ */ u8      unknown104[8];
    /* 0x10C */ u32     unknown10C;       // Referenced by SaveCheckpointState
    
    /* 0x110+ */ u8      unknown110[0x24];
    /* 0x134 */ Entity* checkpointEntityList; // Saved tick list for respawn
    
    /* Additional fields to be discovered */
} GameState;  // Size: unknown (at least 0x14A bytes)

/* -----------------------------------------------------------------------------
 * CD-ROM File Structure (CdlFILE)
 * Already defined in PSY-Q SDK (LIBCD.H)
 * -------------------------------------------------------------------------- */
/* CdlFILE is provided by psyq/LIBCD.H - don't redefine it */

/* -----------------------------------------------------------------------------
 * Global Variables (to be extern'd in game code)
 * -------------------------------------------------------------------------- */

/* PAL version addresses */
#define GAME_STATE_ADDR        0x8009DC40  /* GameState base */
#define LEVEL_CONTEXT_ADDR     0x8009DCC4  /* GameState + 0x84 */
#define BLB_HEADER_ADDR        0x800AE3E0  /* Loaded BLB header */
#define ENTITY_CALLBACKS_ADDR  0x8009D5F8  /* Entity type callback table */
#define GAME_BLB_FILE_ADDR     0x8009B4B4  /* CdlFILE structure */
#define GAME_BLB_SECTOR_ADDR   0x800A59F0  /* Starting sector (typically 0x146) */

#endif /* GAME_H */
