#ifndef BLBACC_RECORDS_H
#define BLBACC_RECORDS_H

#include "common.h"

/* =============================================================================
 * BLB ASSET-RECORD LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These overlay records inside decoded BLB asset blobs (level shape header,
 * tile-attribute header, sprite/asset payloads, TOC entries). Used
 * exclusively by src/blbacc.c. See docs/blb.hexpat for the authoritative
 * on-disk format.
 * ============================================================================= */

typedef struct LevelShapeHeader {
    /* 0x00 */ u16 field0;
    /* 0x02 */ u16 field2;
    /* 0x04 */ u16 width;
    /* 0x06 */ u16 height;
    /* 0x08 */ u16 levelWidth;
    /* 0x0A */ u16 levelHeight;
    /* 0x0C */ u16 spawnX;
    /* 0x0E */ u16 spawnY;
    /* 0x10 */ u16 tile_count_a;            /* Used in GetTotalTileCount + func_8007B55C + func_8007B574 */
    /* 0x12 */ u16 tile_count_b;
    /* 0x14 */ u16 tile_count_c;            /* func_8007B574 returns this */
    /* 0x16 */ u16 field16;                 /* GetTileHeaderField16 */
    /* 0x18 */ u16 flags;                   /* GetLevelFlags */
    /* 0x1A */ u16 field1A;                 /* GetTileHeaderField1A */
    /* 0x1C */ u16 field1C;                 /* GetAsset100Field1C */
    /* 0x1E */ u16 entity_count;            /* GetEntityCount */
    /* 0x20 */ u16 world_index;             /* GetTileHeaderWorldIndex */
} LevelShapeHeader;

typedef struct LevelListEntry {
    /* 0x00 */ u8 pad00[0xE];
    /* 0x0E */ u8 extraFlag;
} LevelListEntry;

typedef struct TileAttributeHeader {
    /* 0x00 */ u16 field0;
    /* 0x02 */ u16 field2;
    /* 0x04 */ u16 width;
    /* 0x06 */ u16 height;
    /* 0x08 */ u8 data[1];
} TileAttributeHeader;

typedef struct SpriteAssetPayload {
    /* 0x00 */ u8 pad00[0x10];
} SpriteAssetPayload;

typedef struct AssetPayloadRef {
    /* 0x00 */ u8 pad00[0x58];
    /* 0x58 */ SpriteAssetPayload *payload;
} AssetPayloadRef;

typedef struct SpriteContext {
    /* 0x00 */ u32 frameData;
    /* 0x04 */ u32 pixelBuffer;
    /* 0x08 */ u32 frameMetadata;
    /* 0x0C */ u16 width;
    /* 0x0E */ u16 height;
    /* 0x10 */ u16 frameCount;
    /* 0x12 */ u8 flags;
    /* 0x13 */ u8 decodeEnabled;
} SpriteContext;

/* 12-byte TOC entry shape shared by the palette container, tilemap
 * container, and anim-offsets container: opaque first 12 bytes followed
 * by a u32 byte-offset (from the table base) to the entry's payload. */
typedef struct {
    /* 0x00 */ u8  _pad00[0xC];
    /* 0x0C */ u32 data_offset;
} BlbToc12Entry;

/* Per-layer payload pointed to by anim_offsets entries. The leading 6
 * bytes are a small header (two u16s + the width word at +4) followed
 * by the packed tile-index stream. */
typedef struct {
    /* 0x00 */ u16 _hdr00;
    /* 0x02 */ u16 _hdr02;
    /* 0x04 */ u16 width;
    /* 0x06 */ u8  data[1];
} TilemapLayerPayload;

#endif /* BLBACC_RECORDS_H */
