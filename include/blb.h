/* =============================================================================
 * BLB File Format Structures
 * =============================================================================
 * Data structures for GAME.BLB archive parsing.
 * Based on verified offsets from Ghidra analysis and ImHex template.
 * =============================================================================
 */

#ifndef BLB_H
#define BLB_H

#include "common.h"

/* -----------------------------------------------------------------------------
 * BLB Header (0x1000 bytes at file start)
 * -------------------------------------------------------------------------- */

/* Movie table entry (28 bytes, 13 entries at 0xB60) */
typedef struct {
    /* 0x00 */ u16     reserved;          // Always 0
    /* 0x02 */ u16     sectorCount;       // Movie size in sectors
    /* 0x04 */ char    movieId[5];        // 4-char ID + null (e.g., "DREA")
    /* 0x09 */ char    shortName[3];      // 2-char + null
    /* 0x0C */ char    isoPath[16];       // ISO path (e.g., "\\MVLOGO.STR;1")
} MovieTableEntry;  // Size: 0x1C (28 bytes)

/* Sector table entry (16 bytes, 32 entries at 0xCD0 for PAL) */
typedef struct {
    /* 0x00 */ u8      levelIndex;        // 0-25 when entryFlags=0x00
    /* 0x01 */ u8      entryFlags;        // 0x00=level, 0x03=game over, 0x05=special
    /* 0x02 */ u8      displayTimeout;    // Max display time in seconds (×60 for frames)
    /* 0x03 */ char    code[5];           // 4-char code + null (e.g., "PIRA")
    /* 0x08 */ char    shortName[4];      // Truncated description
    /* 0x0C */ u16     sectorOffset;      // Sector offset in BLB
    /* 0x0E */ u16     sectorCount;       // Sector count
} SectorTableEntry;  // Size: 0x10 (16 bytes)

/* Level metadata entry (112 bytes, 26 entries at 0x000) */
typedef struct {
    /* 0x00 */ char    levelId[5];        // 4-char ID + null (e.g., "SCIE")
    /* 0x05 */ u8      stageCount;        // Number of stages (1-6)
    /* 0x06 */ u16     padding06;
    /* 0x08 */ u16     primarySector;     // Primary segment sector
    /* 0x0A */ u16     primarySectorCount;
    /* 0x0C */ u16     secondarySector;   // Secondary (tiles) segment sector
    /* 0x0E */ u16     secondarySectorCount;
    /* 0x10 */ u16     stageSectors[6];   // Tertiary sectors for each stage
    /* 0x1C */ u16     stageSectorCounts[6];
    /* 0x28 */ u16     subBlockSectors[6]; // Secondary sub-blocks
    /* 0x34 */ u16     subBlockSectorCounts[6];
    /* 0x40 */ u32     unknown40[12];     // Unknown data
} LevelMetadata;  // Size: 0x70 (112 bytes)

/* BLB File Header (loaded to RAM @ 0x800AE3E0 for PAL) */
typedef struct {
    /* 0x000 */ LevelMetadata   levels[26];        // 26 × 0x70 = 0xB60
    /* 0xB60 */ MovieTableEntry movies[13];        // 13 × 0x1C = 0x16C
    /* 0xCCC */ u32             padding_ccc;       // 4 zeros
    /* 0xCD0 */ SectorTableEntry sectors[32];      // 32 × 0x10 = 0x200 (PAL/NTSC-US)
    /* 0xED0 */ u32             mode6Sectors[16];  // Password screen sectors (overlaps sectors[31])
    /* 0xF10 */ u8              creditsSeq[33];    // Credits sequence data
    /* 0xF31 */ u8              levelCount;        // 26
    /* 0xF32 */ u8              movieCount;        // 13
    /* 0xF33 */ u8              sectorCount;       // Sector table entry count
    /* 0xF34 */ u8              playbackData[204]; // Mode/index arrays
} BLBHeader;  // Size: 0x1000 (4096 bytes)

/* Note: JP version (SLPS-01501) has different layout:
 * - Sector table at 0xCB0 (32 bytes earlier)
 * - 12 movies instead of 13
 * - Different field order in sector entries
 */

/* -----------------------------------------------------------------------------
 * Asset Container Structures (TOC format)
 * -------------------------------------------------------------------------- */

/* Asset TOC entry (12 bytes) */
typedef struct {
    /* 0x00 */ u16     assetId;           // Asset type ID (e.g., 100, 200, 300)
    /* 0x02 */ u16     reserved;          // Always 0
    /* 0x04 */ u32     size;              // Asset data size in bytes
    /* 0x08 */ u32     offset;            // Offset from TOC start
} AssetTOCEntry;  // Size: 0x0C (12 bytes)

/* Asset Container Header (TOC) */
typedef struct {
    /* 0x00 */ u16     entryCount;        // Number of assets
    /* 0x02 */ u16     reserved;          // Always 0
    /* 0x04 */ u32     containerSize;     // Total size including header
    /* 0x08 */ u32     reserved08;        // Always 0
    /* 0x0C */ AssetTOCEntry entries[1];  // Variable length (use [1] for C89 compatibility)
} AssetContainer;  // Size: 0x0C + (entryCount × 0x0C)

/* -----------------------------------------------------------------------------
 * Tile Header (Asset 100, 36 bytes)
 * -------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u16     backgroundColor;   // 15-bit BGR color (0BBBBBGGGGGRRRRR)
    /* 0x02 */ u16     playerSpawnX;      // Player spawn position (tile coords × 16)
    /* 0x04 */ u16     playerSpawnY;
    /* 0x06 */ u16     tileCount16x16;    // Number of 16×16 tiles
    /* 0x08 */ u16     tileCount8x8;      // Number of 8×8 tiles
    /* 0x0A */ u16     tileCountExtra;    // Additional tiles (purpose unclear)
    /* 0x0C */ u16     paletteCount;      // Number of 256-color palettes
    /* 0x0E */ u16     unknown0E;
    /* 0x10 */ u16     unknown10;
    /* 0x12 */ u16     unknown12;
    /* 0x14 */ u16     unknown14;
    /* 0x16 */ u16     unknown16;
    /* 0x18 */ u16     unknown18;
    /* 0x1A */ u16     unknown1A;
    /* 0x1C */ u16     entityCount;       // Number of entities in Asset 501
    /* 0x1E */ u16     unknown1E;
    /* 0x20 */ u16     unknown20;
    /* 0x22 */ u16     unknown22;
} TileHeader;  // Size: 0x24 (36 bytes)

/* -----------------------------------------------------------------------------
 * VRAM Slot Config (Asset 101, 12 bytes)
 * -------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u16     bankACount;        // Slots in Bank A
    /* 0x02 */ u16     bankBCount;        // Slots in Bank B
    /* 0x04 */ u32     reserved04;        // Always 0
    /* 0x08 */ u32     reserved08;        // Always 0
} VRAMSlotConfig;  // Size: 0x0C (12 bytes)

/* -----------------------------------------------------------------------------
 * Layer Entry (Asset 201, 92 bytes per layer)
 * -------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u16     width;             // Layer width in tiles
    /* 0x02 */ u16     height;            // Layer height in tiles
    /* 0x04 */ s16     xOffset;           // Screen position X
    /* 0x06 */ s16     yOffset;           // Screen position Y
    /* 0x08 */ u32     scrollX;           // Parallax scroll X (0x10000 = 1.0)
    /* 0x0C */ u32     scrollY;           // Parallax scroll Y
    /* 0x10 */ u16     tilemapOffset;     // Offset in tilemap container
    /* 0x12 */ u16     unknown12;
    /* 0x14 */ u32     unknown14;
    /* 0x18 */ u32     unknown18;
    /* 0x1C */ u8      unknownData[64];   // Additional layer data
} LayerEntry;  // Size: 0x5C (92 bytes)

/* -----------------------------------------------------------------------------
 * Entity Definition (Asset 501, 24 bytes)
 * -------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u16     x1;                // Bounding box left (pixels)
    /* 0x02 */ u16     y1;                // Bounding box top
    /* 0x04 */ u16     x2;                // Bounding box right
    /* 0x06 */ u16     y2;                // Bounding box bottom
    /* 0x08 */ u16     xCenter;           // Center position X
    /* 0x0A */ u16     yCenter;           // Center position Y
    /* 0x0C */ u16     variant;           // Entity variant/parameter
    /* 0x0E */ u32     padding0E;         // Always 0
    /* 0x12 */ u16     entityType;        // Entity type ID (0-120+)
    /* 0x14 */ u16     layer;             // Render layer / Z-order
    /* 0x16 */ u16     padding16;         // Reserved
} EntityDefinition;  // Size: 0x18 (24 bytes)

#endif /* BLB_H */
