/**
 * @file tile_header.h
 * @brief TileHeader structure - Asset 100 (0x64) - 36 bytes
 *
 * Contains level metadata: dimensions, spawn position, tile counts, colors,
 * and various behavior flags.
 *
 * Loaded into LevelDataContext[1] by LoadAssetContainer (0x8007b074).
 * Accessed via GetTileHeaderPtr (0x8007b4b8) which returns ctx[1].
 *
 * Source of truth: scripts/blb.hexpat (verified via Ghidra)
 */
#ifndef GAME_TILE_HEADER_H
#define GAME_TILE_HEADER_H

#include "common.h"

/**
 * TileHeader - Level metadata from Asset 100
 *
 * Key accessor functions:
 *   GetTileHeaderPtr      (0x8007b4b8) - Returns ctx[1]
 *   GetSecondaryColorPtr  (0x8007b4c4) - Returns TileHeader + 4
 *   GetTotalTileCount     (0x8007b53c) - Sum of tile counts at 0x10/0x12/0x14
 *   GetLevelFlags         (0x8007b600) - Returns field at +0x18
 *   GetTileHeaderField1A  (0x8007b614) - Returns field at +0x1A (special_level_id)
 *   GetTileHeaderField16  (0x8007b628) - Returns field at +0x16 (vehicle_waypoint_count)
 *   GetEntityCount        (0x8007b7a8) - Returns field at +0x1E
 *   GetVRAMRectCount      (0x8007b7c8) - Returns field at +0x1C
 *   GetTileHeaderWorldIdx (0x8007b490) - Returns field at +0x20
 *
 * Color loading functions:
 *   LoadBGColorFromTileHeader        (0x80024678) - Reads 0x00-0x02 → GameState+0x131/132/133
 *   LoadSecondaryColorFromTileHeader (0x800246d0) - Reads 0x04-0x06 → GameState+0x127/128/129
 *
 * Spawn position:
 *   FUN_8007b458 reads spawn_x/spawn_y, converts to pixels:
 *   spawn_x_pixels = spawn_x * 16 + 8
 *   spawn_y_pixels = spawn_y * 16 + 15
 */
typedef struct TileHeader {
    /* 0x00 */ u8 bg_r;                    /* Background red (0-255) */
    /* 0x01 */ u8 bg_g;                    /* Background green */
    /* 0x02 */ u8 bg_b;                    /* Background blue */
    /* 0x03 */ u8 pad_03;

    /* 0x04 */ u8 secondary_r;             /* Fog/secondary red */
    /* 0x05 */ u8 secondary_g;             /* Fog/secondary green */
    /* 0x06 */ u8 secondary_b;             /* Fog/secondary blue */
    /* 0x07 */ u8 pad_07;

    /* 0x08 */ u16 level_width;            /* Level width in tiles */
    /* 0x0A */ u16 level_height;           /* Level height in tiles */

    /* 0x0C */ u16 spawn_x;                /* Player spawn X (tiles) */
    /* 0x0E */ u16 spawn_y;                /* Player spawn Y (tiles) */

    /* 0x10 */ u16 count_16x16;            /* Number of 16x16 pixel tiles */
    /* 0x12 */ u16 count_8x8_a;            /* Primary 8x8 tile count */
    /* 0x14 */ u16 count_8x8_b;            /* Additional tile count (often 0) */

    /* 0x16 */ u16 vehicle_waypoint_count; /* Asset 504 waypoint count (FINN/RUNN only) */

    /* 0x18 */ u16 level_flags;            /* Level behavior bitfield (see below) */

    /* 0x1A */ u16 special_level_id;       /* 99 = special mode (FINN/SEVN), 0 = normal */

    /* 0x1C */ u16 vram_rect_count;        /* Number of VRAM rectangles (Asset 502 entries) */

    /* 0x1E */ u16 entity_count;           /* Number of entities in Asset 501 */

    /* 0x20 */ u16 world_index;            /* World/area index (0-6), accumulated across levels */

    /* 0x22 */ u8 pad_22[2];
} TileHeader; /* 36 bytes (0x24) */

/**
 * Level Flags (field 0x18) - Bitfield meanings:
 *
 *   Bit 1  (0x0002): SEVN, FINN - special gameplay mode (special_level_id=99)
 *   Bit 2  (0x0004): FINN only - FlynnBoy-specific flag
 *   Bit 3  (0x0008): EGGS, GLID, WEED, HEAD, MEGA, TMPL, FOOD - possibly collision behavior
 *   Bit 4  (0x0010): RUNN only - runner vehicle mode
 *   Bit 6  (0x0040): WIZZ, HEAD, RUNN, GLEN, MEGA, TMPL, FOOD - boss/special rendering
 *   Bit 7  (0x0080): EVIL only
 *   Bit 8  (0x0100): GLEN only
 *   Bit 9  (0x0200): MENU only
 *   Bit 12 (0x1000): PHRO, MEGA - possibly world 1 / first encounter
 *   Bit 14 (0x4000): CLOU only
 *   Bit 15 (0x8000): FOOD only
 *
 * Common values: 0x0000 (9 levels), 0x0008 (3), 0x1048 (2), unique flags for others
 */
#define TILE_FLAG_SPECIAL_MODE     0x0002
#define TILE_FLAG_FLYNNBOY         0x0004
#define TILE_FLAG_COLLISION_ALT    0x0008
#define TILE_FLAG_RUNNER_MODE      0x0010
#define TILE_FLAG_BOSS_RENDER      0x0040
#define TILE_FLAG_EVIL             0x0080
#define TILE_FLAG_GLEN             0x0100
#define TILE_FLAG_MENU             0x0200
#define TILE_FLAG_WORLD1           0x1000
#define TILE_FLAG_CLOU             0x4000
#define TILE_FLAG_FOOD             0x8000

#endif /* GAME_TILE_HEADER_H */
