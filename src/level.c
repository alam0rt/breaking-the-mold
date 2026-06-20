#include "common.h"
#include "Game/level_data_context.h"
#include "Game/blb.h"

extern s32 strcmp(const char *s1, const char *s2);
extern s32 LoadAssetContainer(LevelDataContext *ctx, s32 arg1, s32 arg2);
extern char TERMINAL_LEVEL_NAME[] asm("D_800A6058");

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
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;
    u8 result;

    if (header->sequence_modes[idx] == 2) {
        u8 slot = header->sequence_targets[idx];
        /* Credits-table walk: header + slot*12 + 0xF1C indexes credits[slot+1].code
         * (credits entries are 12 bytes; the table starts at 0xF10, so 0xF1C is
         * credits[1].code). Kept as raw arithmetic because the +1 stride offset
         * has no clean typed equivalent. */
        u32 name = slot * 12 + (u32)header;
        if (strcmp((char *)(name + 0xF1C), TERMINAL_LEVEL_NAME) == 0) {
            return 0;
        }
    }
    {
        BlbHeader *hdr;
        u8 next;
        hdr = (BlbHeader *)ctx->blb_header;
        next = ctx->current_sequence_index + 1;
        ctx->current_sequence_index = next;
        if ((u8)(next & 0xFF) < hdr->sequence_count) {
            result = hdr->sequence_modes[next & 0xFF];
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
    ctx->current_sequence_index = ((BlbHeader *)ctx->blb_header)->sequence_count - 2;
}

u8 PeekNextPlaybackMode(LevelDataContext *ctx) {
    BlbHeader *header = (BlbHeader *)ctx->blb_header;
    u8 idx = ctx->current_sequence_index;

    if ((s32)idx >= header->sequence_count - 1) {
        return 0;
    }
    return header->sequence_modes[idx + 1];
}

s32 GetPrimaryBufferSize(LevelDataContext *ctx) {
    BlbHeader *header;
    u8 mode;
    u8 idx;

    header = (BlbHeader *)ctx->blb_header;
    idx = ctx->current_sequence_index;
    mode = header->sequence_modes[idx];
    if (mode < 3) {
        return 0;
    }
    if (mode != 3) {
        return 0x7D000;
    }
    {
        u8 slot = header->sequence_targets[idx];
        return header->levels[slot].primary_buffer_size;
    }
}

INCLUDE_ASM("asm/nonmatchings/level", LevelDataParser);

u8 LoadLevelByWorldIndex(LevelDataContext *ctx, u32 arg1, u8 world_index) {
    BlbHeader *header;
    u8 i = 0;

    header = (BlbHeader *)ctx->blb_header;
    if (header->sequence_count != 0) {
        world_index &= 0xFF;
        do {
            /* `entry` is the header pointer biased by the byte-offset i so both
             * sequence_modes[0] and sequence_targets[0] share a single base
             * register (the original codegen pattern). char-arithmetic avoids
             * the no-byte-pointer-arithmetic lint that fires on (u8 *)+offset. */
            BlbHeader *entry = (BlbHeader *)((char *)header + (u8)i);
            if (entry->sequence_modes[0] >= 3) {
                if (entry->sequence_targets[0] == world_index) {
                    ctx->current_sequence_index = i;
                    break;
                }
            }
            header = (BlbHeader *)ctx->blb_header;
            i++;
        } while ((u8)i < header->sequence_count);
    }
    return LevelDataParser(ctx, arg1) & 0xFF;
}

u8 LoadLevelByWorldId(LevelDataContext *ctx, u32 arg1, u8 world_id) {
    BlbHeader *header;
    u8 i = 0;

    header = (BlbHeader *)ctx->blb_header;
    if (header->sequence_count != 0) {
        world_id &= 0xFF;
        do {
            BlbHeader *entry = (BlbHeader *)((char *)header + (u8)i);
            if (entry->sequence_modes[0] >= 3) {
                u8 slot = entry->sequence_targets[0];
                if (header->levels[slot].level_index == world_id) {
                    ctx->current_sequence_index = i;
                    break;
                }
            }
            header = (BlbHeader *)ctx->blb_header;
            i++;
        } while ((u8)i < header->sequence_count);
    }
    return LevelDataParser(ctx, arg1) & 0xFF;
}
