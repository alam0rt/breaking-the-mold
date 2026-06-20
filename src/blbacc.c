#include "common.h"
#include "Game/level_data_context.h"
#include "Game/blb.h"

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

void ClearSpriteContext(SpriteContext *ctx);
void InitSpriteContext(SpriteContext *ctx);
extern u8 DecodeRLESpriteCore(u8 *unused, u8 *src, u16 size, u8 *dst);

/* Tentative defs unlock gp_rel via maspsx --use-comm-section. */
u8 *SPRITE_TOC_TABLE asm("D_800A6060");
u8 *SPRITE_DATA_TABLE asm("D_800A6064");

u8 GetLevelCount(LevelDataContext *ctx) {
    return ((BlbHeader *)ctx->blb_header)->level_count;
}

u8 GetLevelAssetIndex(LevelDataContext *ctx, u8 index) {
    return ((BlbHeader *)ctx->blb_header)->levels[index].level_index;
}

u8 *func_8007A9E8(LevelDataContext *ctx, u8 index) {
    return (u8 *)((BlbHeader *)ctx->blb_header)->levels[index].level_id;
}

char *getLevelName(LevelDataContext *ctx, u8 index) {
    return ((BlbHeader *)ctx->blb_header)->levels[index].name;
}

u8 GetLevelFlagByIndex(LevelDataContext *ctx, u8 index) {
    return ((BlbHeader *)ctx->blb_header)->levels[index].password_selectable;
}

u8 GetCurrentLevelAssetIndex(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] == 3) {
        u8 slot = header->sequence_targets[idx];
        return header->levels[slot].level_index;
    }
    return 0;
}

char *GetCurrentLevelDisplayName(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] != 3) {
        return NULL;
    }
    {
        u8 slot = header->sequence_targets[idx];
        return header->levels[slot].level_id;
    }
}

char *func_8007AADC(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    u8 kind = header->sequence_modes[idx];
    u8 slot;
    if (kind < 3) {
        return NULL;
    }
    if (kind == 3) {
        slot = header->sequence_targets[idx];
    } else if (kind == 6) {
        /* Off-by-one alias into the sequence tables: mode-6 entries read
         * one byte earlier than mode-3, landing on the trailing byte of
         * sequence_modes (idx == 0) or on sequence_targets[idx - 1]
         * (idx > 0). Expressed as a u8 array view over the whole header
         * so the read crosses the field boundary cleanly. */
        slot = ((u8 *)header)[0xF91 + idx];
    } else {
        return NULL;
    }
    return header->levels[slot].name;
}

u8 GetLoadingScreenMinDisplayTime(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    u8 mode = header->sequence_modes[idx];
    if ((u32)(mode - 4) < 2) {
        u8 slot = header->sequence_targets[idx];
        return header->sectors[slot].flags;
    }
    return 0;
}

u8 GetLoadingScreenMaxDisplayTime(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    u8 mode = header->sequence_modes[idx];
    if ((u32)(mode - 4) < 2) {
        u8 slot = header->sequence_targets[idx];
        return header->sectors[slot].display_timeout;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentSectorOffset);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentSectorCount);

u8 GetAssetCount(LevelDataContext *ctx) {
    return ((BlbHeader *)ctx->blb_header)->movie_count;
}

u8 *GetMovieEntryByIndex(LevelDataContext *ctx, u8 index) {
    return (u8 *)((BlbHeader *)ctx->blb_header)->movies[index].movie_id;
}

u8 *func_8007AD10(LevelDataContext *ctx, u8 index) {
    return (u8 *)((BlbHeader *)ctx->blb_header)->movies[index].short_name;
}

char *GetCurrentMovieReserved(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] != 1) {
        return NULL;
    }
    {
        u8 slot = header->sequence_targets[idx];
        return header->movies[slot].movie_id;
    }
}

u8 *GetMovieDataForLevel(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] != 1) {
        return NULL;
    }
    {
        u8 slot = header->sequence_targets[idx];
        return (u8 *)header->movies[slot].short_name;
    }
}

char *GetCurrentMovieFilename(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] != 1) {
        return NULL;
    }
    {
        u8 slot = header->sequence_targets[idx];
        return header->movies[slot].iso_path;
    }
}

u16 GetMovieFrameField00(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] == 1) {
        u8 slot = header->sequence_targets[idx];
        return header->movies[slot]._reserved;
    }
    return 0;
}

u16 GetMovieSectorCount(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    if (header->sequence_modes[idx] != 1) {
        return 0;
    }
    {
        u8 slot = header->sequence_targets[idx];
        return header->movies[slot].sector_count;
    }
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentModeReservedData);

u8 func_8007AFA4(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    u8 flag = header->sequence_modes[idx];
    if (flag == 6) return 1;
    if (flag != 3) return 0;
    {
        u8 slot = header->sequence_targets[idx];
        LevelListEntry *p = (LevelListEntry *)&header->levels[slot];
        return p->extraFlag;
    }
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentTertiaryDataSize);

INCLUDE_ASM("asm/nonmatchings/blbacc", LoadAssetContainer);

u8 GetCurrentStageIndex(LevelDataContext *ctx) {
    return *(u8 *)ctx;
}

u8 *GetAsset101Entry(LevelDataContext *ctx, u16 idx) {
    u8 **table = (u8 **)ctx->vram_slot_config;
    if (table != NULL) {
        if (idx < 2) {
            return table[idx];
        }
    }
    return NULL;
}

u8 *GetLevelDimensions(u8 *dst, LevelDataContext *ctx) {
    LevelShapeHeader *src = (LevelShapeHeader *)ctx->tile_header;
    __builtin_memcpy(dst, &src->levelWidth, 4);
    return dst;
}

u8 *GetSpawnPosition(u8 *dst, LevelDataContext *ctx) {
    LevelShapeHeader *src = (LevelShapeHeader *)ctx->tile_header;
    __builtin_memcpy(dst, &src->spawnX, 4);
    return dst;
}

u16 GetLevelFlags(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->flags;
}

u16 GetTileHeaderWorldIndex(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->world_index;
}

u16 GetTileHeaderField1A(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->field1A;
}

u8 *GetTileHeaderPtr(LevelDataContext *ctx) {
    return (u8 *)ctx->tile_header;
}

u8 *GetSecondaryColorPtr(LevelDataContext *ctx) {
    return (u8 *)(ctx->tile_header + 4);
}

u8 GetPaletteGroupCount(LevelDataContext *ctx) {
    u8 *p = (u8 *)ctx->palette_container;
    if (p == NULL) return 0;
    return *p;
}

u8 *GetPaletteDataPtr(LevelDataContext *ctx, u8 idx) {
    u8 *base = (u8 *)ctx->palette_container;
    BlbToc12Entry *entry;
    if (base == NULL) return NULL;
    entry = (BlbToc12Entry *)(base + idx * 12);
    return base + entry->data_offset;
}

u8 *GetPaletteAnimData(LevelDataContext *ctx) {
    return (u8 *)ctx->palette_anim;
}

u32 GetTotalTileCount(LevelDataContext *ctx) {
    LevelShapeHeader *th = (LevelShapeHeader *)ctx->tile_header;
    return th->tile_count_a + th->tile_count_b + th->tile_count_c;
}

u32 func_8007B55C(LevelDataContext *ctx) {
    LevelShapeHeader *th = (LevelShapeHeader *)ctx->tile_header;
    return th->tile_count_a + th->tile_count_b;
}

u16 func_8007B574(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->tile_count_c;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", CopyTilePixelData);

u8 *GetAnimatedTileData(LevelDataContext *ctx, u32 idx) {
    LevelShapeHeader *header = (LevelShapeHeader *)ctx->tile_header;
    u16 sum = (u16)(header->tile_count_a + header->tile_count_b);
    u8 **arr;
    idx--;
    if (idx < sum) return NULL;
    arr = (u8 **)ctx->animated_tiles;
    if (arr == NULL) return NULL;
    return arr[idx - sum];
}

u8 *GetPaletteIndices(LevelDataContext *ctx) {
    return (u8 *)ctx->palette_indices;
}

u32 GetTileSizeFlags(LevelDataContext *ctx) {
    return ctx->tile_flags;
}

u16 GetLayerCount(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tilemap_container);
}

u8 *GetTilemapDataPtr(LevelDataContext *ctx, u16 index) {
    u8 *base = (u8 *)ctx->tilemap_container;
    BlbToc12Entry *entry = (BlbToc12Entry *)(base + index * 12);
    return base + entry->data_offset;
}

u8 *GetLayerEntry(LevelDataContext *ctx, u16 index) {
    return (u8 *)(ctx->layer_entries + index * 92);
}

u8 *func_8007B724(LevelDataContext *ctx, u16 idx) {
    return (u8 *)(ctx->layer_entries + idx * 92 + 0x2C);
}

s32 HasTileAttributes(LevelDataContext *ctx) {
    return ctx->tile_attributes != 0;
}

u8 *GetTileAttributeUnknown(u8 *dst, LevelDataContext *ctx) {
    TileAttributeHeader *src = (TileAttributeHeader *)ctx->tile_attributes;
    __builtin_memcpy(dst, src, 4);
    return dst;
}

u8 *GetTileAttributeDimensions(u8 *dst, LevelDataContext *ctx) {
    TileAttributeHeader *src = (TileAttributeHeader *)ctx->tile_attributes;
    __builtin_memcpy(dst, &src->width, 4);
    return dst;
}

u8 *GetTileAttributeData(LevelDataContext *ctx) {
    return (u8 *)(ctx->tile_attributes + 8);
}

u16 GetEntityCount(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->entity_count;
}

u8 *GetEntityDataPtr(LevelDataContext *ctx) {
    return (u8 *)ctx->entities;
}

u16 GetAsset100Field1C(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->field1C;
}

u32 GetLevelDataContextField3C(LevelDataContext *ctx) {
    return ctx->vram_rects;
}

u16 func_8007B7E8(LevelDataContext *ctx) {
    u16 *p = (u16 *)ctx->anim_offsets;
    if (p != NULL) {
        return *p;
    }
    return 0;
}

u16 GetTilemapLayerWidth(LevelDataContext *ctx, u16 layer_idx) {
    u8 *layers = (u8 *)ctx->anim_offsets;
    if (layers != NULL) {
        u16 idx = (layer_idx - 1) & 0xFFFF;
        BlbToc12Entry *entry = (BlbToc12Entry *)(layers + idx * 12);
        u32 offset = entry->data_offset;
        return ((TilemapLayerPayload *)(layers + offset))->width;
    }
    return 0;
}

u8 *GetTilemapLayerDataPtr(LevelDataContext *ctx, u16 layer_idx) {
    u8 *layers = (u8 *)ctx->anim_offsets;
    if (layers != NULL) {
        u16 idx = (layer_idx - 1) & 0xFFFF;
        BlbToc12Entry *entry = (BlbToc12Entry *)(layers + idx * 12);
        u32 offset = entry->data_offset;
        return ((TilemapLayerPayload *)(layers + offset))->data;
    }
    return NULL;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", CopyTilemapLayerIndex);

u16 GetTileHeaderField16(LevelDataContext *ctx) {
    return ((LevelShapeHeader *)ctx->tile_header)->field16;
}

u8 *GetVehicleDataPtr(LevelDataContext *ctx) {
    return (u8 *)ctx->vehicle_data;
}

u16 func_8007B930(LevelDataContext *ctx) {
    s32 v1 = 0;
    if (ctx->container_600) v1 = *(u16 *)ctx->container_600;
    if (ctx->sprites) v1 += *(s32 *)ctx->sprites;
    return (u16)v1;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", FindSpriteInTOC);

u32 GetAsset601Size(LevelDataContext *ctx) {
    if (ctx->tile_header != 0) {
        return ctx->audio_size;
    }
    return ctx->container_601_size;
}

u8 *GetAsset601Ptr(LevelDataContext *ctx) {
    if (ctx->tile_header != 0) {
        return (u8 *)ctx->audio;
    }
    return (u8 *)ctx->container_601;
}

u8 *GetAsset602Ptr(LevelDataContext *ctx) {
    if (ctx->tile_header != 0) {
        return (u8 *)ctx->palette;
    }
    return (u8 *)ctx->container_602;
}

u8 *GetDemoDataPtr(LevelDataContext *ctx) {
    if (ctx->spu_samples != 0) {
        return (u8 *)(ctx->spu_samples + 0x10);
    }
    return 0;
}

u8 *GetAssetHeaderPtr(AssetPayloadRef *ctx) {
    SpriteAssetPayload *p = ctx->payload;
    if (p) return (u8 *)p - 0x10;
    return NULL;
}

void SetSpriteTables(u8 *a, u8 *b) {
    SPRITE_TOC_TABLE = a;
    SPRITE_DATA_TABLE = b;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", LookupSpriteById);

SpriteContext *ClearSpriteContextWrapper(SpriteContext *ctx) {
    ClearSpriteContext(ctx);
    return ctx;
}

SpriteContext *InitSpriteContextWrapper(SpriteContext *ctx) {
    InitSpriteContext(ctx);
    return ctx;
}

void ClearSpriteContext(SpriteContext *ctx) {
    ctx->frameData = 0;
    ctx->pixelBuffer = 0;
    ctx->frameMetadata = 0;
    ctx->width = 0;
    ctx->height = 0;
    ctx->frameCount = 0;
    ctx->flags = 0;
    ctx->decodeEnabled = 0;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", InitSpriteContext);

INCLUDE_ASM("asm/nonmatchings/blbacc", RenderSprite);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetFrameMetadata);

u8 DecodeRLESpriteChecked(SpriteContext *e, u8 *src, u16 size, u8 *dst) {
    if (e->decodeEnabled == 0) return 1;
    return DecodeRLESpriteCore(NULL, src, size, dst);
}
