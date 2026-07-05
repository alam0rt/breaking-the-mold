/* =============================================================================
 * sprite_decode.c  --  PC-port sprite frame decode helpers (TARGET_PC)
 * =============================================================================
 * Faithful reconstruction of RenderSprite (asm/nonmatchings/blbacc/RenderSprite.s
 * 0x8007BDE8) + GetFrameMetadata (0x8007BEBC), the per-frame RLE decode path used
 * by LoadSpriteFramesToVRAM. RenderSprite allocates a 0x18-byte frame-metadata
 * scratch, fills it (GetFrameMetadata), runs the RLE decoder (DecodeRLESpriteChecked,
 * matched C), and frees the scratch. GetFrameMetadata reads the frame's
 * width/height/RLE-offset from the SpriteContext's frame table (stride 0x24) and
 * lays out the decode descriptor (rle count/ptr, row stride, output base, flip).
 *
 * SpriteContext (spriteCtx, 20 bytes): +0x00 frame_metadata ptr, +0x08 rle_data
 * ptr, +0x13 valid byte. Frame entry (0x24 bytes): +0x0A width, +0x0C height,
 * +0x20 rle offset. Decode descriptor `dst` (0x18 bytes): [0] rle run count,
 * [1] row stride, [2] output base, [3] rle data ptr (+2 past count), [4] rle end,
 * [5] flagH.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void FreeFromHeap(u8 *heap, void *ptr, s32 size, s32 flags);
extern u8   DecodeRLESpriteChecked(void *e, u8 *src, u16 size, u8 *dst);

void GetFrameMetadata(void *spriteCtx, u32 *dst, u32 frameIdx, u32 outputBase,
                      u32 rowStride, u8 flagH, u8 flagV) {
    u8 *ctx = (u8 *)spriteCtx;
    if (ctx[0x13] != 0) {
        u8 *frame = *(u8 **)(ctx + 0x0) + (frameIdx & 0xFFFF) * 0x24;
        u16 w = *(u16 *)(frame + 0xA);
        u16 h = *(u16 *)(frame + 0xC);
        u8 *rle = *(u8 **)(ctx + 0x8) + *(s32 *)(frame + 0x20);
        u32 rleBase = (u32)(uintptr_t)rle;
        u32 runCount = (u32)*(u16 *)rle;

        dst[1] = rowStride & 0xFFFF;
        dst[2] = outputBase;
        *(u16 *)(dst + 5) = (u16)flagH;
        dst[0] = runCount;
        dst[3] = rleBase + 2;
        dst[4] = rleBase + 2 + dst[0] * 2;
        if (flagV != 0) {
            dst[1] = (u32)(-(s32)dst[1]);
            dst[2] = dst[2] + rowStride * (h - 1);
        }
        if (flagH != 0) {
            dst[2] = dst[2] + (w - 1);
        }
    }
}

void RenderSprite(void *spriteCtx, u32 frameIdx, u32 outputBase, u32 rowStride,
                  u8 flagH, u8 flagV) {
    u8 *ctx = (u8 *)spriteCtx;
    if (ctx[0x13] != 0) {
        u32 *meta = (u32 *)AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x18, 1, 0);
        GetFrameMetadata(ctx, meta, frameIdx & 0xFFFF, outputBase, rowStride, flagH, flagV);
        DecodeRLESpriteChecked(ctx, (u8 *)meta, 0xFFFF, NULL);
        FreeFromHeap((u8 *)g_pBlbHeapBase, meta, 0, 0);
    }
}
