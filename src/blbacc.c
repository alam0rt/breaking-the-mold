#include "common.h"
#include "Game/level_data_context.h"

u8 GetLevelCount(LevelDataContext *ctx) {
    return *(u8 *)(ctx->blb_header + 0xF31);
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetLevelAssetIndex);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007A9E8);

INCLUDE_ASM("asm/nonmatchings/blbacc", getLevelName);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetLevelFlagByIndex);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentLevelAssetIndex);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentLevelDisplayName);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007AADC);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetLoadingScreenMinDisplayTime);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetLoadingScreenMaxDisplayTime);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentSectorOffset);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentSectorCount);

u8 GetAssetCount(LevelDataContext *ctx) {
    return *(u8 *)(ctx->blb_header + 0xF32);
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetMovieEntryByIndex);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007AD10);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentMovieReserved);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetMovieDataForLevel);

INCLUDE_ASM("asm/nonmatchings/blbacc", ReturnZero);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentMovieFilename);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetMovieFrameField00);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetMovieSectorCount);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentModeReservedData);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007AFA4);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetCurrentTertiaryDataSize);

INCLUDE_ASM("asm/nonmatchings/blbacc", LoadAssetContainer);

u8 GetCurrentStageIndex(LevelDataContext *ctx) {
    return *(u8 *)ctx;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAsset101Entry);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetLevelDimensions);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetSpawnPosition);

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

INCLUDE_ASM("asm/nonmatchings/blbacc", GetPaletteGroupCount);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetPaletteDataPtr);

void *GetPaletteAnimData(LevelDataContext *ctx) {
    return (void *)ctx->palette_anim;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetTotalTileCount);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007B55C);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007B574);

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

INCLUDE_ASM("asm/nonmatchings/blbacc", GetTilemapDataPtr);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetLayerEntry);

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007B724);

s32 HasTileAttributes(LevelDataContext *ctx) {
    return ctx->tile_attributes != 0;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", GetTileAttributeUnknown);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetTileAttributeDimensions);

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

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007B7E8);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetTilemapLayerWidth);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetTilemapLayerDataPtr);

INCLUDE_ASM("asm/nonmatchings/blbacc", CopyTilemapLayerIndex);

u16 GetTileHeaderField16(LevelDataContext *ctx) {
    return *(u16 *)(ctx->tile_header + 0x16);
}

void *GetVehicleDataPtr(LevelDataContext *ctx) {
    return (void *)ctx->vehicle_data;
}

INCLUDE_ASM("asm/nonmatchings/blbacc", func_8007B930);

INCLUDE_ASM("asm/nonmatchings/blbacc", FindSpriteInTOC);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAsset601Size);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAsset601Ptr);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAsset602Ptr);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetDemoDataPtr);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetAssetHeaderPtr);

INCLUDE_ASM("asm/nonmatchings/blbacc", ReturnZero2);

INCLUDE_ASM("asm/nonmatchings/blbacc", SetSpriteTables);

INCLUDE_ASM("asm/nonmatchings/blbacc", LookupSpriteById);

INCLUDE_ASM("asm/nonmatchings/blbacc", ClearSpriteContextWrapper);

INCLUDE_ASM("asm/nonmatchings/blbacc", InitSpriteContextWrapper);

INCLUDE_ASM("asm/nonmatchings/blbacc", ClearSpriteContext);

INCLUDE_ASM("asm/nonmatchings/blbacc", InitSpriteContext);

INCLUDE_ASM("asm/nonmatchings/blbacc", RenderSprite);

INCLUDE_ASM("asm/nonmatchings/blbacc", GetFrameMetadata);

INCLUDE_ASM("asm/nonmatchings/blbacc", DecodeRLESpriteChecked);
