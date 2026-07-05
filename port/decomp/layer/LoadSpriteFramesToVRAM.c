/* =============================================================================
 * LoadSpriteFramesToVRAM.c  --  PC-port sprite-sheet -> VRAM uploader
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/layer/LoadSpriteFramesToVRAM.s
 * (0x80018DDC, 0x400 bytes).
 *
 * Given a sprite-sheet hash, this looks the sheet up in a 20-entry (0x14) cache
 * of 0x18-byte descriptors. On a miss it claims the first empty slot, builds a
 * SpriteContext for the hash, reserves a CLUT slot + one VRAM slot per frame,
 * RLE-decodes each frame into a scratch buffer and uploads it with LoadImage.
 *
 * Cache entry (0x18 bytes):
 *   +0x00 u32  hash
 *   +0x04 ptr  per-frame array   (0x14-byte entries)
 *   +0x08 u16  max width
 *   +0x0A u16  max height
 *   +0x0C s16  CLUT x
 *   +0x0E s16  CLUT y
 *   +0x10 u16  total frames
 *   +0x12 u16  CLUT id
 *   +0x14 u8   lookup byte
 *
 * Per-frame entry (0x14 bytes):
 *   +0x00 ptr  frame metadata (0x24-byte record: w@+0xA, h@+0xC)
 *   +0x04 s16  VRAM x            +0x06 s16 VRAM y
 *   +0x08 s16  VRAM half-width   +0x0A s16 VRAM height
 *   +0x0C u16  texture page      +0x0E u8 U   +0x0F u8 V   +0x10 u8 vram-ok
 *
 * Faithfulness note: the original loads the sprite context through
 * InitSpriteContextWrapper, which relies on the sheet hash still living in $a1
 * when it tail-calls InitSpriteContext(ctx). That register passthrough does not
 * survive x86 cdecl, so we call InitSpriteContext(ctx, hash) directly here --
 * behaviourally identical. Likewise $s0 (the bit-depth flag) is written
 * unconditionally to 1 in a branch-delay slot, so sprites are always 8-bit CLUT
 * (mode 1); the mode==0 / mode>=2 arms are kept for fidelity but never taken.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

typedef struct { s16 x, y, w, h; } PortRect;

extern void *g_pBlbHeapBase;

extern u8  *AllocateFromHeap(u8 *heap, s32 elemSize, s32 count, s32 flags);
extern void FreeFromHeap(u8 *heap, void *ptr, s32 size, s32 flags);
extern s32  InitSpriteContext(void *ctx, u32 spriteId);
extern s32  AllocateCLUTSlot(void *heap, u32 outXY, u8 count);
extern u16  GetClut(int x, int y);
extern int  LoadClut(void *clut, int x, int y);
extern int  LoadImage(void *rect, void *data);
extern u8   AllocateVRAMSlot(void *heap, u16 *outRect, u8 mode, s16 width, s16 height);
extern long GetTPage(int tp, int abr, int x, int y);
extern void RenderSprite(void *ctx, u32 frameIdx, void *dst, s32 rowStride, s32 flagH, s32 flagV);
extern void DrawSync(int mode);
extern void memset_entrypoint(void *dst, int count, int fill);

void LoadSpriteFramesToVRAM(u8 *cacheBase, u32 hash) {
    u8  *heap;
    u8  *entry;
    u8   ctxBuf[0x18];        /* local SpriteContext (20 bytes) */
    void *frameMetaBase;
    void *cacheSecondary;     /* CLUT source data (ctx +0x04) */
    u8  *perFrame;
    s32  bitDepth;            /* $s0 -- always 1 here */
    s32  mode;               /* $s6 = bitDepth & 0xFF */
    s32  i;
    s32  fp;

    /* 1. Return early if this sheet is already cached. */
    for (i = 0; i < 0x14; i++) {
        if (*(u32 *)(cacheBase + i * 0x18) == hash) {
            return;
        }
    }

    /* 2. Find the first empty slot; bail if the cache is full. */
    for (i = 0; i < 0x14; i++) {
        if (*(u32 *)(cacheBase + i * 0x18) == 0) {
            break;
        }
    }
    if (i >= 0x14) {
        return;
    }
    entry = cacheBase + i * 0x18;
    heap = (u8 *)g_pBlbHeapBase;

    /* Build the sprite context and copy its summary fields into the cache. */
    *(u32 *)(entry + 0x00) = hash;
    InitSpriteContext(ctxBuf, hash);
    *(u16 *)(entry + 0x10) = *(u16 *)(ctxBuf + 0x10);   /* total frames */
    *(u16 *)(entry + 0x08) = *(u16 *)(ctxBuf + 0x0C);   /* max width  */
    *(u16 *)(entry + 0x0A) = *(u16 *)(ctxBuf + 0x0E);   /* max height */
    *(u8  *)(entry + 0x14) = *(u8  *)(ctxBuf + 0x12);   /* lookup byte */

    /* Reserve a CLUT slot and record its id. */
    bitDepth = 1;
    if (AllocateCLUTSlot(heap, (u32)(uintptr_t)(entry + 0x0C), 1) & 0xFF) {
        *(u16 *)(entry + 0x12) = GetClut(*(s16 *)(entry + 0x0C), *(s16 *)(entry + 0x0E));
    } else {
        *(u16 *)(entry + 0x12) = 0;
    }

    /* Upload the palette. bitDepth is always 1 -> LoadClut path. */
    cacheSecondary = *(void **)(ctxBuf + 0x04);
    if ((bitDepth & 0xFF) == 0) {
        PortRect clutRect;
        clutRect.x = *(s16 *)(entry + 0x0C);
        clutRect.y = *(s16 *)(entry + 0x0E);
        clutRect.w = 0x10;
        clutRect.h = 1;
        LoadImage(&clutRect, cacheSecondary);
    } else if ((bitDepth & 0xFF) == 1) {
        LoadClut(cacheSecondary, *(s16 *)(entry + 0x0C), *(s16 *)(entry + 0x0E));
    }
    DrawSync(0);

    /* Allocate the per-frame descriptor array (0x14-byte stride). */
    mode = bitDepth & 0xFF;
    perFrame = AllocateFromHeap(heap, 0x14, *(u16 *)(entry + 0x10), 0);
    *(void **)(entry + 0x04) = perFrame;

    frameMetaBase = *(void **)(ctxBuf + 0x00);

    for (fp = 0; (s16)fp < (s32)*(u16 *)(entry + 0x10); fp++) {
        u8  *frame = (u8 *)frameMetaBase + (fp & 0xFFFF) * 0x24;
        s16  h = *(s16 *)(frame + 0x0C);
        u16  roundedW = (u16)((*(u16 *)(frame + 0x0A) + 3) & 0xFFFC);
        u8  *pf = perFrame + fp * 0x14;
        u8   vramOk;

        *(void **)(pf + 0x00) = frame;
        vramOk = AllocateVRAMSlot(heap, (u16 *)(pf + 0x04), (u8)mode, (s16)roundedW, h);
        *(u8 *)(pf + 0x10) = vramOk;
        if (vramOk & 0xFF) {
            s16   vram_x = *(s16 *)(pf + 0x04);
            s16   vram_y = *(s16 *)(pf + 0x06);
            s32   roundedWi = (s16)roundedW;
            u16   tpage = (u16)GetTPage(mode, 0, vram_x & ~0x3F, vram_y & ~0xFF);
            u8   *pixBuf;
            PortRect rect;
            s32   rectW;

            *(u16 *)(pf + 0x0C) = tpage;
            if (mode == 0) {
                *(u8 *)(pf + 0x0E) = (u8)((vram_x & 0x3F) << 2);
            } else if (mode == 1) {
                *(u8 *)(pf + 0x0E) = (u8)((vram_x & 0x3F) << 1);
            } else {
                *(u8 *)(pf + 0x0E) = (u8)(vram_x & 0x3F);
            }
            *(u8 *)(pf + 0x0F) = (u8)vram_y;

            pixBuf = AllocateFromHeap(heap, 1, roundedWi * h, 0);
            memset_entrypoint(pixBuf, (roundedWi * h) & 0xFFFC, 0);
            RenderSprite(ctxBuf, fp & 0xFFFF, pixBuf, roundedWi, 0, 0);

            rect.x = vram_x;
            rect.y = vram_y;
            if (mode == 0) {
                rectW = (roundedWi >> 2) + 1;
            } else if (mode == 1) {
                rectW = (roundedWi >> 1) + 1;
            } else {
                rectW = (s32)roundedW + 1;
            }
            rect.w = (s16)(rectW & ~1);
            rect.h = h;
            LoadImage(&rect, pixBuf);
            FreeFromHeap(heap, pixBuf, 0, 0);
            DrawSync(0);
        }
    }
}
