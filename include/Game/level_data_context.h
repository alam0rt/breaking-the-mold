#ifndef LEVEL_DATA_CONTEXT_H
#define LEVEL_DATA_CONTEXT_H

#include "common.h"

/* =============================================================================
 * LevelDataContext
 * 
 * Asset loading context structure embedded at GameState+0x84.
 * Stores pointers to loaded BLB assets for the current level/stage.
 * 
 * Size: 128 bytes (0x80)
 * Initialized by: InitLevelDataContext @ 0x8007a1bc
 * Cleared by: ClearLevelDataContext @ 0x8007a234
 * Populated by: LoadAssetContainer @ 0x8007b074
 * 
 * Asset types map to context indices as follows (from LoadAssetContainer):
 *   100 (0x64)  → ctx[1]  = TileHeader
 *   101 (0x65)  → ctx[2]  = VRAMSlotConfig  
 *   200 (0xC8)  → ctx[3]  = TilemapContainer
 *   201 (0xC9)  → ctx[4]  = LayerEntries
 *   300 (0x12C) → ctx[5]  = TilePixels
 *   301 (0x12D) → ctx[6]  = PaletteIndices
 *   302 (0x12E) → ctx[7]  = TileFlags
 *   303 (0x12F) → ctx[10] = AnimatedTiles
 *   400 (0x190) → ctx[8]  = PaletteContainer
 *   401 (0x191) → ctx[9]  = PaletteAnim
 *   500 (0x1F4) → ctx[11] = TileAttributes
 *   501 (0x1F5) → ctx[14] = Entities
 *   502 (0x1F6) → ctx[15] = VRAMRects
 *   503 (0x1F7) → ctx[12] = AnimOffsets
 *   504 (0x1F8) → ctx[13] = VehicleData
 *   600 (0x258) → ctx[16,17] = Sprites + size
 *   601 (0x259) → ctx[18,19] = Audio + size
 *   602 (0x25A) → ctx[20] = Palette
 *   700 (0x2BC) → ctx[21,22] = SPUSamples + size
 * ============================================================================= */

typedef struct {
    /* Asset Pointers (indices 0-22, cleared by ClearLevelDataContext) */
    /* 0x00 */ u32  current_sub_block;      /* Current sub-block index (0=none, 1=primary, 2=secondary) */
    /* 0x04 */ u32  tile_header;            /* Asset 100: TileHeader (36 bytes) */
    /* 0x08 */ u32  vram_slot_config;       /* Asset 101: VRAMSlotConfig (12 bytes) */
    /* 0x0C */ u32  tilemap_container;      /* Asset 200: TilemapContainer sub-TOC */
    /* 0x10 */ u32  layer_entries;          /* Asset 201: LayerEntries (92 bytes each) */
    /* 0x14 */ u32  tile_pixels;            /* Asset 300: TilePixels (8bpp indexed) */
    /* 0x18 */ u32  palette_indices;        /* Asset 301: PaletteIndices (1 byte/tile) */
    /* 0x1C */ u32  tile_flags;             /* Asset 302: TileFlags (semi-trans, size, skip) */
    /* 0x20 */ u32  palette_container;      /* Asset 400: PaletteContainer sub-TOC */
    /* 0x24 */ u32  palette_anim;           /* Asset 401: PaletteAnim data */
    /* 0x28 */ u32  animated_tiles;         /* Asset 303: AnimatedTiles lookup */
    /* 0x2C */ u32  tile_attributes;        /* Asset 500: TileAttributes (collision, 1 byte/tile) */
    /* 0x30 */ u32  anim_offsets;           /* Asset 503: AnimOffsets (ToolX sequences) */
    /* 0x34 */ u32  vehicle_data;           /* Asset 504: VehicleData (FINN/RUNN levels) */
    /* 0x38 */ u32  entities;               /* Asset 501: Entities (24-byte structs) */
    /* 0x3C */ u32  vram_rects;             /* Asset 502: VRAMRects (texture pages) */
    /* 0x40 */ u32  sprites;                /* Asset 600: Sprites/Geometry data */
    /* 0x44 */ u32  sprites_size;           /* Asset 600: Size in bytes */
    /* 0x48 */ u32  audio;                  /* Asset 601: Audio (SPU ADPCM) */
    /* 0x4C */ u32  audio_size;             /* Asset 601: Size in bytes */
    /* 0x50 */ u32  palette;                /* Asset 602: Palette (15-bit PSX colors) */
    /* 0x54 */ u32  spu_samples;            /* Asset 700: SPUSamples (additional audio) */
    /* 0x58 */ u32  spu_samples_size;       /* Asset 700: Size in bytes */

    /* BLB Navigation (indices 23-25, set by InitLevelDataContext) */
    /* 0x5C */ u32  blb_header;             /* Pointer to loaded BLB header */
    /* 0x60 */ u8   current_sequence_index; /* Current playback sequence index (0xFF = none) */
    /* 0x61 */ u8   pad61;
    /* 0x62 */ u8   pad62;
    /* 0x63 */ u8   pad63;
    /* 0x64 */ u32  sector_read_callback;   /* BLB sector read callback: (sectorOffset, sectorCount, dst) */

    /* Loaded Data Pointers (indices 26-31, set by LevelDataParser) */
    /* 0x68 */ u32  primary_data;           /* Primary asset container base */
    /* 0x6C */ u32  secondary_data;         /* Secondary/tertiary container base */
    /* 0x70 */ u32  container_600;          /* Asset 600 container (from mode 6) */
    /* 0x74 */ u32  container_601;          /* Asset 601 container */
    /* 0x78 */ u32  container_601_size;     /* Asset 601 size */
    /* 0x7C */ u32  container_602;          /* Asset 602 container */
} LevelDataContext;  /* Size: 0x80 (128 bytes) */

#endif /* LEVEL_DATA_CONTEXT_H */
