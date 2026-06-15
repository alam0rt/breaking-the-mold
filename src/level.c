#include "common.h"
#include "Game/level_data_context.h"

extern s32 strcmp(const char *s1, const char *s2);
extern s32 LoadAssetContainer(LevelDataContext *ctx, s32 arg1, s32 arg2);
extern char D_800A6058[];

void ResetGameStateInputAndContext(LevelDataContext *ctx);
void ClearContextOffsets68to7C(LevelDataContext *ctx);
void ClearLevelDataContext(LevelDataContext *ctx);

void InitLevelDataContext(LevelDataContext *ctx, u32 blb_header, u32 callback) {
    ctx->blb_header = blb_header;
    ctx->sector_read_callback = callback;
    ctx->current_sequence_index = 0xFF;
    ResetGameStateInputAndContext(ctx);
}

void ResetGameStateInputAndContext(LevelDataContext *ctx) {
    ClearContextOffsets68to7C(ctx);
    ClearLevelDataContext(ctx);
}

void ClearContextOffsets68to7C(LevelDataContext *ctx) {
    ctx->primary_data = 0;
    ctx->secondary_data = 0;
    ctx->container_600 = 0;
    ctx->container_601 = 0;
    ctx->container_601_size = 0;
    ctx->container_602 = 0;
}

void ClearLevelDataContext(LevelDataContext *ctx) {
    ctx->current_sub_block = 0;
    ctx->tile_header = 0;
    ctx->vram_slot_config = 0;
    ctx->palette_container = 0;
    ctx->palette_anim = 0;
    ctx->tilemap_container = 0;
    ctx->layer_entries = 0;
    ctx->tile_attributes = 0;
    ctx->tile_pixels = 0;
    ctx->palette_indices = 0;
    ctx->tile_flags = 0;
    ctx->animated_tiles = 0;
    ctx->entities = 0;
    ctx->vram_rects = 0;
    ctx->vehicle_data = 0;
    ctx->anim_offsets = 0;
    ctx->sprites = 0;
    ctx->sprites_size = 0;
    ctx->audio = 0;
    ctx->audio_size = 0;
    ctx->palette = 0;
    ctx->spu_samples = 0;
    ctx->spu_samples_size = 0;
}

u8 AdvancePlaybackSequence(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    u32 entry = blb + idx;
    u8 result;

    if (*(u8 *)(entry + 0xF36) == 2) {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 name = slot * 12 + blb;
        if (strcmp((char *)(name + 0xF1C), D_800A6058) == 0) {
            return 0;
        }
    }
    {
        u8 next;
        u32 hdr;
        hdr = ctx->blb_header;
        next = ctx->current_sequence_index + 1;
        ctx->current_sequence_index = next;
        if ((u8)(next & 0xFF) < *(u8 *)(hdr + 0xF30)) {
            result = *(u8 *)(hdr + (next & 0xFF) + 0xF36);
        } else {
            result = 0;
        }
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/level", SetSequenceIndexByMode);

INCLUDE_ASM("asm/nonmatchings/level", SeekToLevelInSequence);

INCLUDE_ASM("asm/nonmatchings/level", FindSaveSlotByName);

void AdvanceLevelSequence(LevelDataContext *ctx) {
    ctx->current_sequence_index = *(u8 *)(ctx->blb_header + 0xF30) - 2;
}

u8 PeekNextPlaybackMode(LevelDataContext *ctx) {
    u32 blb = ctx->blb_header;
    u8 idx = ctx->current_sequence_index;

    if ((s32)idx >= *(u8 *)(blb + 0xF30) - 1) {
        return 0;
    }
    return *(u8 *)(blb + idx + 0xF37);
}

s32 GetPrimaryBufferSize(LevelDataContext *ctx) {
    u32 blb;
    u8 mode;
    u32 entry;

    blb = ctx->blb_header;
    entry = blb + ctx->current_sequence_index;
    mode = *(u8 *)(entry + 0xF36);
    if (mode < 3) {
        return 0;
    }
    if (mode != 3) {
        return 0x7D000;
    }
    {
        u8 slot = *(u8 *)(entry + 0xF92);
        u32 world = blb + slot * 0x70;
        return *(s32 *)(world + 4);
    }
}

INCLUDE_ASM("asm/nonmatchings/level", LevelDataParser);

INCLUDE_ASM("asm/nonmatchings/level", LoadLevelByWorldIndex);

INCLUDE_ASM("asm/nonmatchings/level", LoadLevelByWorldId);
