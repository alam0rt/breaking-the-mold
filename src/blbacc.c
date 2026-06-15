#include "common.h"
#include "Game/level_data_context.h"

void ClearSpriteContext(void *ctx);
void InitSpriteContext(void *ctx);

/* Tentative defs unlock gp_rel via maspsx --use-comm-section. */
void *D_800A6060;
void *D_800A6064;

u8 GetLevelCount(LevelDataContext *ctx) {
    return *(u8 *)(ctx->blb_header + 0xF31);
}

u8 GetLevelAssetIndex(LevelDataContext *ctx, u8 index) {
    u8 *p = (u8 *)(ctx->blb_header + index * 112 + 0xC);
    return *p;
}

void *func_8007A9E8(LevelDataContext *ctx, u8 index) {
    return (void *)(ctx->blb_header + index * 112 + 0x56);
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

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007AADC);

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

void *GetMovieEntryByIndex(LevelDataContext *ctx, u8 index) {
    return (void *)(index * 28 + ctx->blb_header + 0xB64);
}

void *func_8007AD10(LevelDataContext *ctx, u8 index) {
    return (void *)(index * 28 + ctx->blb_header + 0xB69);
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

INCLUDE_ASM("asm/nonmatchings/blbacc", GetMovieDataForLevel);

INCLUDE_ASM("asm/nonmatchings/blbacc", ReturnZero);

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

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007AFA4);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentTertiaryDataSize);

INCLUDE_ASM("asm/nonmatchings/blbacc", LoadAssetContainer);

u8 GetCurrentStageIndex(LevelDataContext *ctx) {
    return *(u8 *)ctx;
}

void *GetAsset101Entry(LevelDataContext *ctx, u16 idx) {
    void **table = (void **)ctx->vram_slot_config;
    if (table != NULL) {
        if (idx < 2) {
            return table[idx];
        }
    }
    return NULL;
}

void *GetLevelDimensions(void *dst, void *ctx) {
    void *src = *(void **)((u8 *)ctx + 4);
    __builtin_memcpy(dst, (u8 *)src + 8, 4);
    return dst;
}

void *GetSpawnPosition(void *dst, void *ctx) {
    void *src = *(void **)((u8 *)ctx + 4);
    __builtin_memcpy(dst, (u8 *)src + 0xC, 4);
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

void *GetTileHeaderPtr(LevelDataContext *ctx) {
    return (void *)ctx->tile_header;
}

void *GetSecondaryColorPtr(LevelDataContext *ctx) {
    return (void *)(ctx->tile_header + 4);
}

u8 GetPaletteGroupCount(LevelDataContext *ctx) {
    u8 *p = (u8 *)ctx->palette_container;
    if (p == NULL) return 0;
    return *p;
}

void *GetPaletteDataPtr(LevelDataContext *ctx, u8 idx) {
    u8 *base = (u8 *)ctx->palette_container;
    u8 *entry;
    if (base == NULL) return NULL;
    entry = base + idx * 12;
    return base + *(s32 *)(entry + 0xC);
}

void *GetPaletteAnimData(LevelDataContext *ctx) {
    return (void *)ctx->palette_anim;
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

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAnimatedTileData);

void *GetPaletteIndices(LevelDataContext *ctx) {
    return (void *)ctx->palette_indices;
}

u32 GetTileSizeFlags(LevelDataContext *ctx) {
    return ctx->tile_flags;
}

u16 GetLayerCount(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tilemap_container);
}

void *GetTilemapDataPtr(LevelDataContext *ctx, u16 index) {
    u32 base = ctx->tilemap_container;
    u32 off = index * 12;
    return (void *)(base + *(u32 *)(base + off + 0xC));
}

void *GetLayerEntry(LevelDataContext *ctx, u16 index) {
    return (void *)(ctx->layer_entries + index * 92);
}

void *func_8007B724(LevelDataContext *ctx, u16 idx) {
    return (void *)(ctx->layer_entries + idx * 92 + 0x2C);
}

s32 HasTileAttributes(LevelDataContext *ctx) {
    return ctx->tile_attributes != 0;
}

void *GetTileAttributeUnknown(void *dst, void *ctx) {
    void *src = *(void **)((u8 *)ctx + 0x2C);
    __builtin_memcpy(dst, src, 4);
    return dst;
}

void *GetTileAttributeDimensions(void *dst, void *ctx) {
    void *src = *(void **)((u8 *)ctx + 0x2C);
    __builtin_memcpy(dst, (u8 *)src + 4, 4);
    return dst;
}

void *GetTileAttributeData(LevelDataContext *ctx) {
    return (void *)(ctx->tile_attributes + 8);
}

u16 GetEntityCount(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x1E);
}

void *GetEntityDataPtr(LevelDataContext *ctx) {
    return (void *)ctx->entities;
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

void *GetVehicleDataPtr(LevelDataContext *ctx) {
    return (void *)ctx->vehicle_data;
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

void *GetAsset601Ptr(LevelDataContext *ctx) {
    if (ctx->tile_header != 0) {
        return (void *)ctx->audio;
    }
    return (void *)ctx->container_601;
}

void *GetAsset602Ptr(LevelDataContext *ctx) {
    if (ctx->tile_header != 0) {
        return (void *)ctx->palette;
    }
    return (void *)ctx->container_602;
}

void *GetDemoDataPtr(LevelDataContext *ctx) {
    if (ctx->spu_samples != 0) {
        return (void *)(ctx->spu_samples + 0x10);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAssetHeaderPtr);

INCLUDE_ASM("asm/nonmatchings/blbacc", ReturnZero2);

void SetSpriteTables(void *a, void *b) {
    D_800A6060 = a;
    D_800A6064 = b;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", LookupSpriteById);

void *ClearSpriteContextWrapper(void *ctx) {
    ClearSpriteContext(ctx);
    return ctx;
}

void *InitSpriteContextWrapper(void *ctx) {
    InitSpriteContext(ctx);
    return ctx;
}

void ClearSpriteContext(void *ctx) {
    *(u32 *)((u8 *)ctx + 0x0) = 0;
    *(u32 *)((u8 *)ctx + 0x4) = 0;
    *(u32 *)((u8 *)ctx + 0x8) = 0;
    *(u16 *)((u8 *)ctx + 0xC) = 0;
    *(u16 *)((u8 *)ctx + 0xE) = 0;
    *(u16 *)((u8 *)ctx + 0x10) = 0;
    *(u8 *)((u8 *)ctx + 0x12) = 0;
    *(u8 *)((u8 *)ctx + 0x13) = 0;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", InitSpriteContext);

INCLUDE_ASM("asm/nonmatchings/blbacc", RenderSprite);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetFrameMetadata);

INCLUDE_ASM("asm/nonmatchings/blbacc", DecodeRLESpriteChecked);
