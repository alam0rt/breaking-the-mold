/* =============================================================================
 * tile_vram.c  --  PC-port tile-graphics decode + VRAM upload (TARGET_PC)
 * =============================================================================
 * Faithful ports (from m2c drafts, cross-checked vs the .s) of the layer.c/
 * gfx.c functions that decode a level's tile graphics and upload them into VRAM:
 *   InitMenuSpriteRenderContext, UploadTextureOrClut, SetTexturePageParams,
 *   CopyTilePixelData, CopyTextureData, LoadTileDataToVRAM.
 * This is the "make pixels appear" subsystem (CP-2.4). It allocates the per-tile
 * render-state array (GameState+0x108) and per-palette sprite-render-contexts
 * (GameState+0x110), uploads each palette's CLUT and each tile's pixels to the
 * VRAM texture (via the HAL LoadImage/LoadClut in gpu_gl.c), and records each
 * tile's tpage/CLUT/coords for the renderer.
 *
 * The sprite render context (0x3C bytes) fields used here (from m2c):
 *   +0x00 s16 tick marker   +0x04 u32 tick callback   +0x14 u16 init flag
 *   +0x18 u32 vtable        +0x1C u32 CLUT backup ptr (palette anim)
 *   +0x28 u16 VRAM CLUT X   +0x2A u16 VRAM CLUT Y      +0x2C u16 CLUT id
 *   +0x34 u8  type (0=texture,1=clut)  +0x35..+0x39 palette-anim params
 *
 * VRAM placement uses vram_coords.c (CalculateVRAMCoordinates + simplified
 * AllocateCLUTSlot). See docs/plans/pc-port.md CP-2.2/CP-2.4.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "psyq_pc.h"
#include <string.h>

/* HAL (gpu_gl.c) + heap (heap_alloc.c) + VRAM coords (vram_coords.c). */
extern long GetTPage(int tp, int abr, int x, int y);
extern u_short GetClut(int x, int y);
extern int  LoadImage(RECT *rect, u_int *data);
extern int  LoadClut(u_int *clut, int x, int y);
extern void DrawSync(int mode);
extern u8  *AllocateFromHeap(u8 *base, s32 align, s32 size, s32 flags);
extern void FreeFromHeap(u8 *base, u8 *ptr, s32 size, s32 flags);
extern s16  CalculateVRAMCoordinates(void *base, void *out, s32 size);
extern s32  AllocateCLUTSlot(void *base, void *out, s32 mode);

/* LevelDataContext accessors (matched C in blbacc.c / level.c). */
extern s16  GetTotalTileCount(void *ldc);
extern u8   GetPaletteGroupCount(void *ldc);
extern void *GetPaletteAnimData(void *ldc);
extern void *GetPaletteDataPtr(void *ldc, s32 idx);
extern void *GetPaletteIndices(void *ldc, s32 arg);
extern void *GetTileSizeFlags(void *ldc);

/* Sprite-render-context vtable + callback (asm aliases / weak stub). */
extern u8 g_EntityVtable_Destroyed[] asm("g_EntityVtable_Destroyed");
extern u8 D_8001048C[] asm("D_8001048C");
extern void CLUTPaletteCycleTickCallback(void);

/* --- InitMenuSpriteRenderContext @ 0x8001979C --------------------------------
 * Initialise a 0x3C-byte sprite render context and allocate its CLUT slot. */
void *InitMenuSpriteRenderContext(void *arg0, s8 type) {
    u8 *c = (u8 *)arg0;
    *(void **)(c + 0x18) = g_EntityVtable_Destroyed;
    *(s16 *)(c + 0x00) = 0;  *(s16 *)(c + 0x02) = 0;  *(s32 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x14) = 1;
    c[0x34] = (u8)type;
    *(s16 *)(c + 0x08) = 0;  *(s16 *)(c + 0x0A) = 0;
    *(s32 *)(c + 0x0C) = 0;  *(s16 *)(c + 0x10) = 0;  *(s16 *)(c + 0x12) = 0;
    *(void **)(c + 0x18) = D_8001048C;
    AllocateCLUTSlot(g_pBlbHeapBase, c + 0x28, type & 0xFF);
    *(u16 *)(c + 0x2C) = GetClut(*(u16 *)(c + 0x28), *(u16 *)(c + 0x2A));
    *(s32 *)(c + 0x1C) = 0;
    c[0x35] = 0; c[0x36] = 0xFF; c[0x37] = 0; c[0x38] = 1; c[0x39] = 0;
    *(s32 *)(c + 0x20) = 0;  *(s32 *)(c + 0x24) = 0;
    *(u16 *)(c + 0x2E) = 0;  *(u16 *)(c + 0x30) = 0;  *(u16 *)(c + 0x32) = 0;
    *(u16 *)(c + 0x14) = 0;
    return arg0;
}

/* --- UploadTextureOrClut @ layer -------------------------------------------
 * Upload the context's palette data to VRAM: a CLUT (LoadClut) or a texture
 * (LoadImage), selected by the context type at +0x34. */
void UploadTextureOrClut(void *arg0, void *data) {
    u8 *c = (u8 *)arg0;
    u8 type = c[0x34];
    if (type == 0) {
        RECT r;
        r.x = *(s16 *)(c + 0x28);
        r.y = *(s16 *)(c + 0x2A);
        r.w = 0x10;
        r.h = 1;
        LoadImage(&r, (u_int *)data);
    } else if (type == 1) {
        LoadClut((u_int *)data, *(s16 *)(c + 0x28), *(s16 *)(c + 0x2A));
    }
}

/* --- SetTexturePageParams @ layer ------------------------------------------
 * Install the palette-animation cycle callback + params on a context (only if
 * it has a CLUT backup buffer at +0x1C). */
void SetTexturePageParams(void *arg0, s8 a1, s8 a2, s8 a3, u8 a4) {
    u8 *c = (u8 *)arg0;
    if (*(s32 *)(c + 0x1C) != 0) {
        c[0x35] = (u8)a2; c[0x36] = (u8)a3; c[0x37] = a4;
        c[0x38] = (u8)a1; c[0x39] = (u8)a1;
        *(s16 *)(c + 0x00) = 0;
        *(void **)(c + 0x04) = (void *)CLUTPaletteCycleTickCallback;
    }
}

/* --- CopyTilePixelData @ layer ---------------------------------------------
 * Copy one tile's 16 rows of indexed pixels from the tile-pixel asset into a
 * staging buffer (with a destination row stride). Full-width tiles are 256
 * bytes; the overflow tiles past tile_header->count are 128 bytes (8-wide). */
s32 CopyTilePixelData(void *ldc, s32 tileIdx, void *dst, s32 dstStride) {
    u8 *c = (u8 *)ldc;
    u8 *tileHeader;
    u8 *pixBase;
    u16 fullCount;
    u32 idx0;
    u8 *src;
    s32 srcRow = 0x10;
    s32 r;

    if (tileIdx == 0) {
        return 0;
    }
    idx0 = (u32)(tileIdx - 1);
    tileHeader = *(u8 **)(c + 0x04);       /* LevelDataContext.tile_header */
    fullCount = *(u16 *)(tileHeader + 0x10);
    pixBase = *(u8 **)(c + 0x14);          /* LevelDataContext.tile_pixels */
    if (idx0 < fullCount) {
        src = pixBase + (idx0 << 8);
    } else {
        src = pixBase + ((u32)fullCount << 8) + ((idx0 - fullCount) << 7);
        srcRow = 8;
    }
    for (r = 0; r < 0x10; r++) {
        memcpy((u8 *)dst, src, (size_t)srcRow);
        dst = (u8 *)dst + (dstStride & 0xFFFF);
        src += srcRow;
    }
    return 1;
}

/* --- CopyTextureData @ layer -----------------------------------------------
 * Back up a context's CLUT into a heap buffer (for palette animation): 0x10
 * bytes for a 4-bit CLUT (type 0), 0x100 for an 8-bit CLUT (type 1). */
void CopyTextureData(void *arg0, void *data) {
    u8 *c = (u8 *)arg0;
    u8 type = c[0x34];
    if (type == 0) {
        if (*(u8 **)(c + 0x1C) == NULL) {
            *(u8 **)(c + 0x1C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 2, 0x10, 0);
        }
        memcpy(*(u8 **)(c + 0x1C), data, 0x20);   /* 0x10 shorts = 0x20 bytes */
    } else if (type == 1) {
        if (*(u8 **)(c + 0x1C) == NULL) {
            *(u8 **)(c + 0x1C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 2, 0x100, 0);
        }
        memcpy(*(u8 **)(c + 0x1C), data, 0x200);  /* 0x100 shorts = 0x200 bytes */
    }
}

/* --- LoadTileDataToVRAM @ layer (0x242 lines) -------------------------------
 * Decode the level's tiles + palettes and upload them into VRAM. Builds:
 *   gs+0x104 u16   tile count (+1)
 *   gs+0x108 ptr   tile-render-state array (stride 8): {tpage@0, clut@2,
 *                  subX@4, y@5, sizeFlag@6}
 *   gs+0x114 u8    palette group count
 *   gs+0x110 ptr   palette sprite-render-context array (stride 4)
 */
void LoadTileDataToVRAM(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *ldc = gs + 0x84;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u16 tileCount = (u16)(GetTotalTileCount(ldc) + 1);
    u8  palCount;
    u8 *animData;
    u8 *palIndices;
    u8 *sizeFlags;
    u8 *staging;
    s32 i;

    *(u16 *)(gs + 0x104) = tileCount;
    *(u8 **)(gs + 0x108) = AllocateFromHeap(heap, 8, tileCount & 0xFFFF, 0);
    palCount = GetPaletteGroupCount(ldc);
    *(u8 *)(gs + 0x114) = palCount;
    *(u8 **)(gs + 0x110) = AllocateFromHeap(heap, 4, palCount & 0xFF, 0);
    animData = (u8 *)GetPaletteAnimData(ldc);

    /* ---- palette groups: one sprite render context (+ CLUT upload) each ---- */
    for (i = 0; i < *(u8 *)(gs + 0x114); i++) {
        u8 **palArray = *(u8 ***)(gs + 0x110);
        palArray[i] = InitMenuSpriteRenderContext(AllocateFromHeap(heap, 0x3C, 1, 0), 1);
        UploadTextureOrClut(palArray[i], GetPaletteDataPtr(ldc, i & 0xFF));
        if (animData != NULL) {
            u8 *ad = animData + i * 4;
            if (ad[0x0] & 1) {
                CopyTextureData(palArray[i], GetPaletteDataPtr(ldc, i & 0xFF));
                SetTexturePageParams(palArray[i], (s8)ad[0x3], (s8)ad[0x1],
                                     (s8)ad[0x2], (u8)(ad[0x0] & 2));
            }
        }
    }

    /* ---- tiles: decode pixels + upload to VRAM, record render state -------- */
    palIndices = (u8 *)GetPaletteIndices(ldc, 0x3C);
    sizeFlags = (u8 *)GetTileSizeFlags(ldc);
    staging = AllocateFromHeap(heap, 1, 0x100, 0);

    for (i = 1; i < *(u16 *)(gs + 0x104); i++) {
        u8 flag = sizeFlags[i - 1];
        if (!(flag & 4)) {
            s32 is4bit = ((flag & 2) == 0);   /* temp_v0_3 / temp_s0_4 */
            u8 *palCtx = (*(u8 ***)(gs + 0x110))[palIndices[i - 1]];
            u16 clutId = *(u16 *)(palCtx + 0x2C);
            s32 rowBytes = is4bit ? 8 : 0x10;
            RECT rect;
            u8 *rs;

            CopyTilePixelData(ldc, i, staging, rowBytes);
            CalculateVRAMCoordinates(heap, &rect, is4bit);   /* fills rect.x/.y */
            rect.w = (short)(is4bit ? 8 : 4);
            rect.h = 0x10;
            LoadImage(&rect, (u_int *)staging);
            DrawSync(0);

            /* tile render-state entry i (stride 8). */
            rs = *(u8 **)(gs + 0x108) + i * 8;
            *(s32 *)(rs + 0) = GetTPage(is4bit, 0, rect.x & ~0x3F, rect.y & ~0xFF);
            *(u16 *)(rs + 2) = clutId;
            if (is4bit == 0) {
                rs[4] = (u8)((rect.x & 0x3F) * 4);
            } else if (is4bit == 1) {
                rs[4] = (u8)((rect.x & 0x3F) * 2);
            }
            rs[5] = (s8)rect.y;
            rs[6] = (s8)(sizeFlags[i - 1] & 1);
        }
    }

    FreeFromHeap(heap, staging, 0, 0);
}
