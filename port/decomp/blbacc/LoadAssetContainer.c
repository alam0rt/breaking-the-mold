/* =============================================================================
 * LoadAssetContainer.c  --  PC-port BLB asset-container decoder (TARGET_PC)
 * =============================================================================
 * PC reconstruction of asm/nonmatchings/blbacc/LoadAssetContainer.s (0x8007B074,
 * 262 lines). Reads a level's asset container out of GAME.BLB (via the
 * LevelDataContext.sector_read_callback) and populates the per-asset pointer
 * fields in the context. Recursive: on a failed primary read it retries with
 * asset index 1.
 *
 * All offsets are transcribed directly from the disassembly (not m2c's struct
 * guesses). The container/record format is:
 *   - blb_header directory: a type byte at (blb_header + seqIdx + 0xF36); when 3,
 *     a sub-index at +0xF92 selects a 0x70-byte directory entry
 *     (blb_header + subIdx*0x70 + assetIdx*2) holding sector offset/count u16s at
 *     +0x1C/+0x2A (primary) or +0x38/+0x46 (secondary).
 *   - loaded container: base[0] = record count (u32), then records at stride 0xC:
 *     record i fields = { type @ +4, size @ +8, dataOffset @ +0xC }; the asset's
 *     data pointer = base + dataOffset. Asset-ID -> ctx-field map is verified
 *     against both the .s switch and include/Game/level_data_context.h.
 *
 * See docs/plans/pc-port.md CP-2.2.
 * ========================================================================== */
#include "common.h"
#include "Game/level_data_context.h"
#include <stdint.h>

typedef s32 (*BlbReadFn)(u32 sectorOff, u32 sectorCnt, u8 *dst);

/* Clear the 23 asset-pointer fields (0x00..0x58) the loader repopulates. */
static void clear_asset_fields(LevelDataContext *ctx) {
    ctx->current_sub_block = 0;
    ctx->tile_header = 0; ctx->vram_slot_config = 0;
    ctx->tilemap_container = 0; ctx->layer_entries = 0;
    ctx->tile_pixels = 0; ctx->palette_indices = 0; ctx->tile_flags = 0;
    ctx->palette_container = 0; ctx->palette_anim = 0; ctx->animated_tiles = 0;
    ctx->tile_attributes = 0; ctx->anim_offsets = 0; ctx->vehicle_data = 0;
    ctx->entities = 0; ctx->vram_rects = 0;
    ctx->sprites = 0; ctx->sprites_size = 0;
    ctx->audio = 0; ctx->audio_size = 0; ctx->palette = 0;
    ctx->spu_samples = 0; ctx->spu_samples_size = 0;
}

/* Walk the container's record list, storing each asset's data pointer (and size
 * for 600/601/700) into the matching context field. */
static void parse_records(LevelDataContext *ctx, u8 *base) {
    u32 count = *(u32 *)base;
    u32 i;
    u8 *cursor = base;                 /* record i header at base + i*0xC */
    for (i = 0; i < count; i++, cursor += 0xC) {
        u32 type    = *(u32 *)(cursor + 0x4);
        u32 size    = *(u32 *)(cursor + 0x8);
        u32 offset  = *(u32 *)(cursor + 0xC);
        u32 dataPtr = (u32)(uintptr_t)(base + offset);
        switch (type) {
        case 0x64:  ctx->tile_header       = dataPtr; break;  /* 100 */
        case 0x65:  ctx->vram_slot_config  = dataPtr; break;  /* 101 */
        case 0xC8:  ctx->tilemap_container = dataPtr; break;  /* 200 */
        case 0xC9:  ctx->layer_entries     = dataPtr; break;  /* 201 */
        case 0x12C: ctx->tile_pixels       = dataPtr; break;  /* 300 */
        case 0x12D: ctx->palette_indices   = dataPtr; break;  /* 301 */
        case 0x12E: ctx->tile_flags        = dataPtr; break;  /* 302 */
        case 0x12F: ctx->animated_tiles    = dataPtr; break;  /* 303 */
        case 0x190: ctx->palette_container = dataPtr; break;  /* 400 */
        case 0x191: ctx->palette_anim      = dataPtr; break;  /* 401 */
        case 0x1F4: ctx->tile_attributes   = dataPtr; break;  /* 500 */
        case 0x1F5: ctx->entities          = dataPtr; break;  /* 501 */
        case 0x1F6: ctx->vram_rects        = dataPtr; break;  /* 502 */
        case 0x1F7: ctx->anim_offsets      = dataPtr; break;  /* 503 */
        case 0x1F8: ctx->vehicle_data      = dataPtr; break;  /* 504 */
        case 0x258: ctx->sprites = dataPtr; ctx->sprites_size = size; break;      /* 600 */
        case 0x259: ctx->audio = dataPtr;   ctx->audio_size = size;   break;      /* 601 */
        case 0x25A: ctx->palette = dataPtr; break;                               /* 602 */
        case 0x2BC: ctx->spu_samples = dataPtr; ctx->spu_samples_size = size; break; /* 700 */
        default: break;
        }
    }
}

void LoadAssetContainer(LevelDataContext *ctx, s32 assetIdx, s32 flag) {
    u8 *blb = (u8 *)(uintptr_t)ctx->blb_header;
    u8 *dir = blb + ctx->current_sequence_index;
    u8 type = dir[0xF36];
    u8 *base;

    if (type < 3) {
        return;
    }
    if (type != 3 && !(flag & 0xFF)) {
        return;
    }

    clear_asset_fields(ctx);

    /* Re-fetch (matches the asm: fields were just cleared). */
    dir = (u8 *)(uintptr_t)ctx->blb_header + ctx->current_sequence_index;
    if (dir[0xF36] == 3) {
        u8 subIdx = dir[0xF92];
        u8 *entry = blb + subIdx * 0x70 + assetIdx * 2;
        u16 sectorOff, sectorCnt;
        BlbReadFn read = (BlbReadFn)(uintptr_t)ctx->sector_read_callback;

        if (flag & 0xFF) {
            sectorOff = *(u16 *)(entry + 0x1C);
            sectorCnt = *(u16 *)(entry + 0x2A);
        } else {
            sectorOff = *(u16 *)(entry + 0x38);
            sectorCnt = *(u16 *)(entry + 0x46);
        }
        if (!(read(sectorOff, sectorCnt, (u8 *)(uintptr_t)ctx->secondary_data) & 0xFF)) {
            if (assetIdx != 1) {
                LoadAssetContainer(ctx, 1, flag & 0xFF);
            }
            return;
        }
        base = (u8 *)(uintptr_t)ctx->secondary_data;
        ctx->current_sub_block = assetIdx;
    } else {
        base = (u8 *)(uintptr_t)ctx->primary_data;
        ctx->current_sub_block = type;
    }

    parse_records(ctx, base);
}
