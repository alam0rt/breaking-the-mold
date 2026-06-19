#include "common.h"
#include "Game/level_data_context.h"

typedef struct LevelShapeHeader {
    /* 0x00 */ u16 field0;
    /* 0x02 */ u16 field2;
    /* 0x04 */ u16 width;
    /* 0x06 */ u16 height;
    /* 0x08 */ u16 levelWidth;
    /* 0x0A */ u16 levelHeight;
    /* 0x0C */ u16 spawnX;
    /* 0x0E */ u16 spawnY;
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

void ClearSpriteContext(SpriteContext *ctx);
void InitSpriteContext(SpriteContext *ctx);
extern u8 DecodeRLESpriteCore(u8 *unused, u8 *src, u16 size, u8 *dst);

/* Tentative defs unlock gp_rel via maspsx --use-comm-section. */
u8 *SPRITE_TOC_TABLE asm("D_800A6060");
u8 *SPRITE_DATA_TABLE asm("D_800A6064");

u8 GetLevelCount(LevelDataContext *ctx) {
    return *(u8 *)(ctx->blb_header + 0xF31);
}

u8 GetLevelAssetIndex(LevelDataContext *ctx, u8 index) {
    u8 *p = (u8 *)(ctx->blb_header + index * 112 + 0xC);
    return *p;
}

u8 *func_8007A9E8(LevelDataContext *ctx, u8 index) {
    return (u8 *)(ctx->blb_header + index * 112 + 0x56);
}

char *getLevelName(LevelDataContext *ctx, u8 index) {
    return (char *)(ctx->blb_header + index * 112 + 0x5B);
}

u8 GetLevelFlagByIndex(LevelDataContext *ctx, u8 index) {
    u8 *p = (u8 *)(ctx->blb_header + index * 112 + 0xD);
    return *p;
}

u8 GetCurrentLevelAssetIndex(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) == 3) {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 world = blb + slot * 0x70;
        return *(u8 *)(world + 0xC);
    }
    return 0;
}

char *GetCurrentLevelDisplayName(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) != 3) {
        return NULL;
    }
    {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 world = blb + slot * 0x70;
        return (char *)(world + 0x56);
    }
}

char *func_8007AADC(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    u8 kind = *(u8 *)(entry + 0xF36);
    u8 slot;
    if (kind < 3) {
        return NULL;
    }
    if (kind == 3) {
        slot = *(u8 *)(entry + 0xF92);
    } else if (kind == 6) {
        slot = *(u8 *)(entry + 0xF91);
    } else {
        return NULL;
    }
    return (char *)(blb + slot * 112 + 0x5B);
}

u8 GetLoadingScreenMinDisplayTime(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    u8 mode = *(u8 *)(entry + 0xF36);
    if ((u32)(mode - 4) < 2) {
        u8 slot = *(u8 *)(entry + 0xF92);
        return *(u8 *)(blb + (slot << 4) + 0xCD1);
    }
    return 0;
}

u8 GetLoadingScreenMaxDisplayTime(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    u8 mode = *(u8 *)(entry + 0xF36);
    if ((u32)(mode - 4) < 2) {
        u8 slot = *(u8 *)(entry + 0xF92);
        return *(u8 *)(blb + (slot << 4) + 0xCD2);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentSectorOffset);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentSectorCount);

u8 GetAssetCount(LevelDataContext *ctx) {
    return *(u8 *)(ctx->blb_header + 0xF32);
}

u8 *GetMovieEntryByIndex(LevelDataContext *ctx, u8 index) {
    return (u8 *)(index * 28 + ctx->blb_header + 0xB64);
}

u8 *func_8007AD10(LevelDataContext *ctx, u8 index) {
    return (u8 *)(index * 28 + ctx->blb_header + 0xB69);
}

char *GetCurrentMovieReserved(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) != 1) {
        return NULL;
    }
    {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 movie = slot * 28 + blb;
        return (char *)(movie + 0xB64);
    }
}

u8 *GetMovieDataForLevel(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) != 1) {
        return NULL;
    }
    {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 movie = slot * 28 + blb;
        return (u8 *)(movie + 0xB69);
    }
}

char *GetCurrentMovieFilename(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) != 1) {
        return NULL;
    }
    {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 movie = slot * 28 + blb;
        return (char *)(movie + 0xB6C);
    }
}

u16 GetMovieFrameField00(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) == 1) {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 movie = blb + slot * 28;
        return *(u16 *)(movie + 0xB60);
    }
    return 0;
}

u16 GetMovieSectorCount(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    if (*(u8 *)(entry + 0xF36) != 1) {
        return 0;
    }
    {
        u32 off = *(u8 *)(entry + 0xF92) * 28;
        return *(u16 *)(blb + off + 0xB62);
    }
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentModeReservedData);

u8 func_8007AFA4(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u32 entry = blb + ctx->current_sequence_index;
    u8 flag = *(u8 *)(entry + 0xF36);
    if (flag == 6) return 1;
    if (flag != 3) return 0;
    {
        u8 slot = *(u8 *)(entry + 0xF92);
        LevelListEntry *p = (LevelListEntry *)(blb + slot * 112);
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
    return *(u16 *)(ctx->tile_header + 0x18);
}

u16 GetTileHeaderWorldIndex(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x20);
}

u16 GetTileHeaderField1A(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x1A);
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
    u8 *entry;
    if (base == NULL) return NULL;
    entry = base + idx * 12;
    return base + *(s32 *)(entry + 0xC);
}

u8 *GetPaletteAnimData(LevelDataContext *ctx) {
    return (u8 *)ctx->palette_anim;
}

u32 GetTotalTileCount(LevelDataContext *ctx) {
    u32 th = ctx->tile_header;
    return *(u16 *)(th + 0x10) + *(u16 *)(th + 0x12) + *(u16 *)(th + 0x14);
}

u32 func_8007B55C(LevelDataContext *ctx) {
    u32 th = ctx->tile_header;
    return *(u16 *)(th + 0x10) + *(u16 *)(th + 0x12);
}

u16 func_8007B574(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x14);
}

INCLUDE_ASM("asm/nonmatchings/blbacc", CopyTilePixelData);

u8 *GetAnimatedTileData(LevelDataContext *ctx, u32 idx) {
    u8 *header = (u8 *)ctx->tile_header;
    u16 a = *(u16 *)(header + 0x10);
    u16 b = *(u16 *)(header + 0x12);
    u16 sum = (u16)(a + b);
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
    u32 base = ctx->tilemap_container;
    u32 off = index * 12;
    return (u8 *)(base + *(u32 *)(base + off + 0xC));
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
    return *(u16 *)(ctx->tile_header + 0x1E);
}

u8 *GetEntityDataPtr(LevelDataContext *ctx) {
    return (u8 *)ctx->entities;
}

u16 GetAsset100Field1C(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x1C);
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
        u8 *entry = layers + idx * 12;
        u32 offset = *(u32 *)(entry + 0xC);
        return *(u16 *)(layers + offset + 4);
    }
    return 0;
}

u8 *GetTilemapLayerDataPtr(LevelDataContext *ctx, u16 layer_idx) {
    u8 *layers = (u8 *)ctx->anim_offsets;
    if (layers != NULL) {
        u16 idx = (layer_idx - 1) & 0xFFFF;
        u8 *entry = layers + idx * 12;
        u32 offset = *(u32 *)(entry + 0xC);
        return layers + offset + 6;
    }
    return NULL;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", CopyTilemapLayerIndex);

u16 GetTileHeaderField16(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x16);
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
