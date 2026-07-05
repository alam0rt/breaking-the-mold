/* =============================================================================
 * AllocSpriteRenderContext.c  --  PC-port animated-sprite render-context alloc
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/entity/AllocSpriteRenderContext.s
 * (0x8001CDAC, 0xF8 bytes; reference = export/SLES_010.90.c 13278). The
 * animated-sprite path of InitEntitySprite (staticSpriteFlag == 0): initialises
 * the sprite context at entity+0x78 (InitSpriteContext, which also stamps the
 * sprite's pixel width/height into entity+0x84/+0x86), allocates the 0x3C-byte
 * render context, reserves its VRAM slot, and allocates the decoded-pixel buffer.
 *
 * Transcribed from the disassembly (the export's unaligned bit-fiddling is m2c
 * noise around the two `lwl/lwr` reads of the width/height word at entity+0x84):
 *   w = *(u16*)(entity+0x84);  h = *(u16*)(entity+0x86);
 *   roundedW = (w + 3) & 0xFFFC;                    // round width up to mult-of-4
 *   ctx = AllocateFromHeap(heap, 0x3C, 1, 0);
 *   ctx = PrepareSpriteVRAMSlotForContext(ctx, zOrder, roundedW, h, 1);
 *   entity+0x34 = ctx;
 *   area = (s16)ctx[+0x4] * (s16)ctx[+0x6];          // allocated slot w*h
 *   entity+0xD4 = area;
 *   entity+0xB0 = AllocateFromHeap(heap, area, 1, 0); // pixel buffer
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void InitSpriteContext(void *ctx, u32 spriteId);
extern u8  *PrepareSpriteVRAMSlotForContext(void *ctx, s16 zOrder, s16 w, s16 h, s32 flag);

void AllocSpriteRenderContext(s32 param_1, s16 zOrder, u32 spriteId) {
    u8 *e = (u8 *)param_1;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u16 w, h;
    s16 roundedW;
    u8 *ctx;
    s32 area;

    InitSpriteContext(e + 0x78, spriteId);
    w = *(u16 *)(e + 0x84);
    h = *(u16 *)(e + 0x86);
    roundedW = (s16)((w + 3) & 0xFFFC);

    ctx = AllocateFromHeap(heap, 0x3C, 1, 0);
    ctx = PrepareSpriteVRAMSlotForContext(ctx, zOrder, roundedW, (s16)h, 1);
    *(void **)(e + 0x34) = ctx;

    area = (s32)(*(s16 *)(ctx + 0x4)) * (s32)(*(s16 *)(ctx + 0x6));
    *(s32 *)(e + 0xD4) = area;
    *(void **)(e + 0xB0) = AllocateFromHeap(heap, area, 1, 0);
}
