/* =============================================================================
 * LevelDataParser.c  --  PC-port level-data sequence parser (TARGET_PC)
 * =============================================================================
 * PC reconstruction of asm/nonmatchings/level/LevelDataParser.s (0x8007A62C,
 * 168 lines). Reads the current playback-sequence's data block out of GAME.BLB
 * into the level buffer (via LevelDataContext.sector_read_callback), records the
 * primary/secondary container bases, scans the block for the 600/601/602
 * sub-containers, then hands off to LoadAssetContainer for non-directory blocks.
 *
 * Offsets transcribed directly from the disassembly + verified against
 * include/Game/level_data_context.h. Header directory format:
 *   dir = blb_header + current_sequence_index;  type = dir[0xF36]
 *   type 3 (directory): entry = blb + subIdx*0x70; sector off/cnt @ +0x0/+0x2
 *   type 6 (movie/raw):  entry = blb + subIdx*4;   sector off/cnt @ +0xECC/+0xECE
 * Record list (in the loaded block): count is a u16 at base[0], then records at
 * stride 0xC: { type @ +4, size @ +8, dataOffset @ +0xC }; dataPtr = base + off.
 *
 * See docs/plans/pc-port.md CP-2.2.
 * ========================================================================== */
#include "common.h"
#include "Game/level_data_context.h"
#include <stdint.h>

typedef s32 (*BlbReadFn)(u32 sectorOff, u32 sectorCnt, u8 *dst);

extern void LoadAssetContainer(LevelDataContext *ctx, s32 assetIdx, s32 flag);

s32 LevelDataParser(LevelDataContext *ctx, void *levelBuf) {
    u8 *blb = (u8 *)(uintptr_t)ctx->blb_header;
    u8 *dir = blb + ctx->current_sequence_index;
    u8 type = dir[0xF36];
    u8 subIdx;
    u16 sectorOff = 0, sectorCnt = 0;
    BlbReadFn read;

    /* Clear loaded-data + asset-pointer fields (0x68..0x7C, then 0x00..0x58). */
    ctx->primary_data = 0; ctx->secondary_data = 0;
    ctx->container_600 = 0; ctx->container_601 = 0;
    ctx->container_601_size = 0; ctx->container_602 = 0;
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

    if (type < 3) {
        return 0;
    }

    subIdx = dir[0xF92];
    if (type == 3) {
        u8 *entry = blb + subIdx * 0x70;
        sectorOff = *(u16 *)(entry + 0x0);
        sectorCnt = *(u16 *)(entry + 0x2);
    } else if (type == 6) {
        u8 *entry = blb + subIdx * 4;
        sectorOff = *(u16 *)(entry + 0xECC);
        sectorCnt = *(u16 *)(entry + 0xECE);
    }

    read = (BlbReadFn)(uintptr_t)ctx->sector_read_callback;
    if (!(read(sectorOff, sectorCnt, (u8 *)levelBuf) & 0xFF)) {
        return 0;
    }

    /* Record the primary block; secondary follows it by entry[+0x8] when the
     * sequence entry is a directory (type 3). */
    ctx->primary_data = (u32)(uintptr_t)levelBuf;
    dir = blb + ctx->current_sequence_index;
    if (dir[0xF36] == 3) {
        u8 *entry = blb + subIdx * 0x70;
        ctx->secondary_data = (u32)(uintptr_t)levelBuf + *(u32 *)(entry + 0x8);
    } else {
        ctx->secondary_data = 0;
    }

    /* Scan the primary block for the 600/601/602 sub-containers. */
    {
        u8 *base = (u8 *)(uintptr_t)ctx->primary_data;
        u16 count = *(u16 *)base;
        u16 i;
        for (i = 0; i < count; i++) {
            u8 *rec = base + i * 0xC;
            u32 rtype   = *(u32 *)(rec + 0x4);
            u32 rsize   = *(u32 *)(rec + 0x8);
            u32 roffset = *(u32 *)(rec + 0xC);
            u32 dataPtr = ctx->primary_data + roffset;
            if (rtype == 0x258) {
                ctx->container_600 = dataPtr;
            } else if (rtype == 0x259) {
                ctx->container_601 = dataPtr;
                ctx->container_601_size = rsize;
            } else if (rtype == 0x25A) {
                ctx->container_602 = dataPtr;
            }
        }
    }

    /* Non-directory blocks still need their asset pointers resolved. */
    dir = blb + ctx->current_sequence_index;
    if (dir[0xF36] != 3) {
        LoadAssetContainer(ctx, 1, 1);
    }
    return 1;
}
