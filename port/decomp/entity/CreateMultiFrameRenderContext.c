/* =============================================================================
 * CreateMultiFrameRenderContext.c  --  PC-port multi-frame sprite ctx builder
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entity/CreateMultiFrameRender
 * Context.s (0x8001CEA4, 0x180 bytes; reference = export/SLES_010.90.c 13363).
 * The multi-frame sibling of AllocSpriteRenderContext: instead of one sprite it
 * scans a 0-terminated frame-id list for the maximum frame width/height, then
 * allocates the 0x3C sprite render context sized to that max, reserves its VRAM
 * slot (PrepareSpriteVRAMSlotForContext), and allocates the decoded-pixel buffer.
 *
 * Per-frame it fills a scratch SpriteContext (InitSpriteContextWrapper ->
 * InitSpriteContext) and reads max_width @ +0xC / max_height @ +0xE. Width is
 * rounded up to a multiple of 4 for the VRAM slot. Result fields (entity+0x34
 * ctx, +0xD4 pixel-area, +0xB0 pixel buffer) match AllocSpriteRenderContext.
 * The export's unaligned word idioms are Ghidra noise around the aligned reads.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern s32  InitSpriteContext(void *ctx, u32 spriteId);
extern u8  *PrepareSpriteVRAMSlotForContext(void *ctx, s16 zOrder, s16 w, s16 h, s32 flag);

void CreateMultiFrameRenderContext(void *entity, u16 tpage, u32 *frameList) {
    u8 *e = (u8 *)entity;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    s16 maxW = 0, maxH = 0;
    u8  scratch[0x20];      /* SpriteContext (20 bytes) */
    u8 *ctx;
    s32 area;

    InitSpriteContext(e + 0x78, frameList[0]);
    for (; *frameList != 0; frameList++) {
        s16 w, h;
        InitSpriteContext(scratch, *frameList);
        w = *(s16 *)(scratch + 0xC);
        h = *(s16 *)(scratch + 0xE);
        if (maxW < w) {
            maxW = w;
        }
        if (maxH < h) {
            maxH = h;
        }
    }

    ctx = AllocateFromHeap(heap, 0x3C, 1, 0);
    ctx = PrepareSpriteVRAMSlotForContext(ctx, (s16)tpage, (s16)((maxW + 3) & 0xFFFC), maxH, 1);
    *(void **)(e + 0x34) = ctx;

    area = (s32)(*(s16 *)(ctx + 0x4)) * (s32)(*(s16 *)(ctx + 0x6));
    *(s32 *)(e + 0xD4) = area;
    *(void **)(e + 0xB0) = AllocateFromHeap(heap, 1, area, 0);
}
