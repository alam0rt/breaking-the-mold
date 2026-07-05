/* =============================================================================
 * tilemap_scroll.c  --  PC-port wrap-scrolling tilemap layer (screen grid)
 * =============================================================================
 * Functional-C reconstructions of the wrap-scrolling tilemap layer pair:
 *   InitTilemapLayerRendering (asm/nonmatchings/sprite/InitTilemapLayerRendering.s
 *                              0x8001601C, 0x454; export/SLES_010.90.c 7100)
 *   RenderTilemapSprites16x16 (asm/nonmatchings/sprite/RenderTilemapSprites16x16.s
 *                              0x8001713C, 0x404; export 7777)
 *
 * This layer keeps a fixed screen-sized grid of 0x15 (21) x screenH_tiles
 * SPRT_16+DR_TPAGE primitive pairs per buffer, pre-chained
 * dr[0]->spr[0]->dr[1]->spr[1]->... ; RenderTilemapSprites16x16 re-stamps a
 * rectangular window of that grid from the tilemap, RE-LINKING each cell:
 * empty/out-of-bounds -> CatPrim(dr[i], dr[i+1]) (bypass the sprite),
 * occupied -> CatPrim(dr[i], spr[i]) (restore it). The scroll tick callbacks
 * (vtable D_80010374) call it with edge strips as the layer moves.
 *
 * Layer sub-object (byte offsets -- NOTE the plate comment in the export
 * mislabels +0x68..+0x7B; the offsets below are what the render function
 * actually reads):
 *   +0x00..07 scroll/delta cache (zeroed)   +0x08 u16 tileSizeFlag
 *   +0x0A u8 active=1                       +0x0C vtable D_80010374
 *   +0x10/+0x14 SPRT_16 pool buf0/1         +0x18/+0x1C DR_TPAGE pool buf0/1
 *   +0x50..+0x56 per-buffer scroll deltas (zeroed)
 *   +0x58/+0x5C chain head buf0/1           +0x60/+0x64 chain tail buf0/1
 *   +0x68 tileTable   +0x6C tilemap   +0x70 colorTable
 *   +0x74 s16 originX +0x76 s16 originY  (tiles)
 *   +0x78 s16 width   +0x7A s16 height   (tiles)
 *   +0x7C u16 wrapX   +0x7E u16 wrapY    +0x80 u8 attrFlag
 *
 * PC-port deviation: the original's empty-cell path also zeroes the P_TAG len
 * byte (*(u8*)(dr+3) = 0) so the PSX GPU DMA skips the packet body. Our tag
 * word is a full 32-bit host pointer (see gpu_gl.c), so that write would
 * corrupt the chain; the CatPrim relink alone carries the semantics.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "port_hal.h"
#include <stdint.h>

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *CatPrim(void *p0, void *p1);

extern u8 D_8001039C[] asm("D_8001039C");   /* PrimObject base vtable          */
extern u8 D_80010374[] asm("D_80010374");   /* wrap-scroll tilemap render vtable */

void RenderTilemapSprites16x16(void *layer, u32 buf, s16 screen_x, s16 screen_y,
        u16 start_col, s16 start_row, s16 tile_x, s16 tile_y, s16 cols, s16 rows);

/* Screen height in 16px tile rows (+1 partial row), from the GPU context's
 * screen height u16 at heap+0x2. */
static s32 screen_tile_rows(void) {
    return (((s32)*(u16 *)((u8 *)g_pBlbHeapBase + 2) << 16) >> 20) + 1;
}

void *InitTilemapLayerRendering(void *storage, void *colorTable, s16 tileSizeFlag,
        s32 tileTable, u16 *tilemap, u32 pos0, u32 pos1,
        u16 wrapX, u16 wrapY, u8 attrFlag) {
    u8 *c = (u8 *)storage;
    s32 hTiles, primCount, i;
    u8  buf;

    *(void **)(c + 0x0C) = D_8001039C;
    *(u16 *)(c + 0x08) = (u16)tileSizeFlag;
    *(u16 *)(c + 0x00) = 0;
    *(u16 *)(c + 0x02) = 0;
    *(u16 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x06) = 0;
    c[0x0A] = 1;
    *(void **)(c + 0x0C) = D_80010374;
    c[0x80] = attrFlag;
    *(void **)(c + 0x70) = colorTable;
    *(s32 *)(c + 0x68) = tileTable;
    *(void **)(c + 0x6C) = tilemap;
    /* packed tilemap origin {x,y} and dims {w,h}, in tiles (swl/swr in the .s) */
    *(u16 *)(c + 0x74) = (u16)(pos0 & 0xFFFF);
    *(u16 *)(c + 0x76) = (u16)(pos0 >> 16);
    *(u16 *)(c + 0x78) = (u16)(pos1 & 0xFFFF);
    *(u16 *)(c + 0x7A) = (u16)(pos1 >> 16);
    *(u16 *)(c + 0x7C) = wrapX;
    *(u16 *)(c + 0x7E) = wrapY;

    hTiles = screen_tile_rows() & 0xFF;
    primCount = hTiles * 0x15;

    *(u8 **)(c + 0x10) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10, primCount, 0);
    *(u8 **)(c + 0x14) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10, primCount, 0);
    *(u8 **)(c + 0x18) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, primCount, 0);
    *(u8 **)(c + 0x1C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, primCount, 0);

    /* pre-chain dr[0]->spr[0]->dr[1]->spr[1]->... per buffer */
    for (buf = 0; buf < 2; buf++) {
        u8 *spr = *(u8 **)(c + 0x10 + buf * 4);
        u8 *dr  = *(u8 **)(c + 0x18 + buf * 4);
        CatPrim(dr, spr);
        *(u8 **)(c + 0x58 + buf * 4) = dr;     /* chain head */
        *(u8 **)(c + 0x60 + buf * 4) = spr;    /* chain tail */
        for (i = 1; i < primCount; i++) {
            CatPrim(*(void **)(c + 0x60 + buf * 4), dr + i * 8);
            CatPrim(dr + i * 8, spr + i * 0x10);
            *(u8 **)(c + 0x60 + buf * 4) = spr + i * 0x10;
        }
    }

    /* pre-render the initial screen content (+ wrap seams) into both buffers */
    RenderTilemapSprites16x16(c, 0, 0, 0, 0, 0, 0, 0, 0x15, (s16)hTiles);
    RenderTilemapSprites16x16(c, 1, 0, 0, 0, 0, 0, 0, 0x15, (s16)hTiles);
    if (wrapX != 0 && (s16)wrapX < 0x15) {
        RenderTilemapSprites16x16(c, 0, (s16)(wrapX << 4), 0, wrapX, 0, 0, 0,
                                  (s16)(0x15 - wrapX), (s16)hTiles);
        RenderTilemapSprites16x16(c, 1, (s16)(wrapX << 4), 0, wrapX, 0, 0, 0,
                                  (s16)(0x15 - wrapX), (s16)hTiles);
    }
    if (wrapY != 0 && (s16)wrapY < (s16)hTiles) {
        RenderTilemapSprites16x16(c, 0, 0, (s16)(wrapY << 4), 0, (s16)wrapY, 0, 0,
                                  0x15, (s16)(hTiles - wrapY));
        RenderTilemapSprites16x16(c, 1, 0, (s16)(wrapY << 4), 0, (s16)wrapY, 0, 0,
                                  0x15, (s16)(hTiles - wrapY));
    }

    *(u16 *)(c + 0x50) = 0;
    *(u16 *)(c + 0x52) = 0;
    *(u16 *)(c + 0x54) = 0;
    *(u16 *)(c + 0x56) = 0;
    /* The export types this void, but the call site consumes $v0 as the layer
     * pointer (param_1[7] = InitTilemapLayerRendering(...)); on MIPS the
     * register happened to still hold `storage`. Return it explicitly. */
    return storage;
}

void RenderTilemapSprites16x16(void *layer, u32 buf, s16 screen_x, s16 screen_y,
        u16 start_col, s16 start_row, s16 tile_x, s16 tile_y, s16 cols, s16 rows) {
    u8 *c = (u8 *)layer;
    u8 *cb = c + (buf & 0xFF) * 4;         /* buffer-indexed pool/tail base */
    s32 hTiles = screen_tile_rows() & 0xFF;
    s32 lastSlot = hTiles * 0x15 - 1;
    s16 offX = *(s16 *)(c + 0x74);
    s16 offY = *(s16 *)(c + 0x76);
    s16 mapW = *(s16 *)(c + 0x78);
    s16 mapH = *(s16 *)(c + 0x7A);
    s16 rowBufIdx = start_row;
    s16 ty = tile_y;
    s16 sy = screen_y;
    s16 r;

    for (r = 0; r < rows; r++) {
        u16 col = start_col;
        s16 tx = tile_x;
        s16 sx = screen_x;
        s16 ccol;

        for (ccol = 0; ccol < cols; ccol++) {
            u16 cell = 0;
            s32 inBounds = (tx >= offX && tx < offX + mapW &&
                            ty >= offY && ty < offY + mapH);
            s32 slot = col + rowBufIdx * 0x15;
            u8 *dr = *(u8 **)(cb + 0x18) + slot * 8;

            if (inBounds) {
                cell = *(u16 *)(*(u8 **)(c + 0x6C) +
                                ((tx - offX) + (ty - offY) * mapW) * 2);
            }
            if (cell == 0) {
                /* empty: bypass this cell's sprite in the chain */
                if (slot == lastSlot) {
                    *(u8 **)(cb + 0x60) = dr;
                } else {
                    CatPrim(dr, dr + 8);
                }
                /* (PSX also zeroes the P_TAG len byte here -- N/A on PC) */
            } else {
                SPRT_16 *p = (SPRT_16 *)(*(u8 **)(cb + 0x10) + slot * 0x10);
                u16 *tile = (u16 *)(*(u8 **)(c + 0x68) + (u32)(cell & 0xFFF) * 8);
                u8  *tint = *(u8 **)(c + 0x70) + (u32)(cell >> 12) * 3;

                CatPrim(dr, p);
                if (slot == lastSlot) {
                    *(u8 **)(cb + 0x60) = (u8 *)p;
                }
                SetSprt16(p);
                SetSemiTrans(p, (s32)(u8)(((u8 *)tile)[6]));
                SetShadeTex(p, 0);
                p->r0   = tint[0];
                p->g0   = tint[1];
                p->b0   = tint[2];
                p->x0   = sx;
                p->y0   = sy;
                p->u0   = (u8)tile[2];
                p->v0   = ((u8 *)tile)[5];
                p->clut = tile[1];
                SetDrawTPage((DR_TPAGE *)dr, 0, 0,
                             (s32)((u32)tile[0] | ((u32)c[0x80] << 5)));
            }
            col++;
            if ((s16)col > 0x14) {
                col = 0;
            }
            sx = (s16)(sx + 0x10);
            tx++;
        }
        rowBufIdx++;
        if (rowBufIdx >= (s16)hTiles) {
            rowBufIdx = 0;
        }
        sy = (s16)(sy + 0x10);
        ty++;
    }
}

/* =============================================================================
 * Per-frame scroll-layer functions (SetupTilemapPrimitives + delta renderers)
 * =============================================================================
 * SetupTilemapPrimitives (0x800178FC, export 7705): submit the layer's prim
 * chain to the OT between two DR_OFFSET packets (scroll offset on, then reset).
 * PC deviations: the PSX framebuffer-half Y bias (buf*0x100) and the 11-bit
 * +0x8000 offset encoding do not apply to the GL backend -- raw signed shorts
 * go into the DR_OFFSET packets (gpu_gl.c applies them directly).
 *
 * RenderTilemapHorizontal/VerticalScroll (0x80016838/0x80016AC8): the original
 * re-stamps only the tile strip that scrolled into view. The PC version
 * re-stamps the WHOLE visible window whenever the cached tile position
 * changed -- behaviourally identical output (same cells stamped from the same
 * tilemap), just more per-frame work, which is negligible on a host CPU.
 * Wrap-X/Y seams are handled per-cell by wrapping the tile coordinate.
 * ========================================================================== */
extern void SetDrawOffset(void *p, u16 *ofs);
extern void AddPrim(void *ot, void *prim);
extern void AddPrims(void *ot, void *head, void *tail);

static u32 *port_gpu_ot(void) {
    return *(u32 **)(*(u8 **)((u8 *)g_pBlbHeapBase + 0xA084) + 0x70);
}

/* Re-stamp the full 21 x screenH window for `buf` at the current scroll,
 * wrapping tile coords into [0, wrap) on wrap layers. */
static void port_rts16_window(u8 *c, u32 buf) {
    s32 hTiles = screen_tile_rows() & 0xFF;
    s16 tileX = (s16)(-(s32)*(s16 *)(c + 0x00) >> 4);
    s16 tileY = (s16)(-(s32)*(s16 *)(c + 0x02) >> 4);
    s16 wrapX = *(s16 *)(c + 0x7C);
    s16 wrapY = *(s16 *)(c + 0x7E);
    s16 col0 = (s16)(tileX % 0x15);      if (col0 < 0) col0 += 0x15;
    s16 row0 = (s16)(tileY % hTiles);    if (row0 < 0) row0 += (s16)hTiles;
    s16 r, cc;

    for (r = 0; r < hTiles; r++) {
        s16 ty = (s16)(tileY + r);
        s16 tyl = ty;
        if (wrapY != 0) {
            tyl = (s16)(ty % wrapY);
            if (tyl < 0) tyl += wrapY;
        }
        for (cc = 0; cc < 0x15; cc++) {
            s16 tx = (s16)(tileX + cc);
            s16 txl = tx;
            if (wrapX != 0) {
                txl = (s16)(tx % wrapX);
                if (txl < 0) txl += wrapX;
            }
            RenderTilemapSprites16x16(c, buf,
                (s16)(tx << 4), (s16)(ty << 4),
                (u16)((col0 + cc) % 0x15), (s16)((row0 + r) % hTiles),
                txl, tyl, 1, 1);
        }
    }
}

void SetupTilemapPrimitives(void *layer) {
    u8  *c = (u8 *)layer;
    u32  buf = *(u8 *)((u8 *)g_pBlbHeapBase + 0xA088);
    u32 *ot = port_gpu_ot();
    u16  ofs[2];
    u8  *offReset  = c + buf * 12 + 0x20;   /* embedded DR_OFFSET pair */
    u8  *offScroll = c + buf * 12 + 0x38;

    ofs[0] = 0;
    ofs[1] = 0;                             /* (PSX: buf * 0x100 fb-half bias) */
    SetDrawOffset(offReset, ofs);
    AddPrim(ot, offReset);
    AddPrims(ot, *(u8 **)(c + 0x58 + buf * 4), *(u8 **)(c + 0x60 + buf * 4));
    ofs[0] = *(u16 *)(c + 0x00);            /* scrollX (raw; PSX adds 0x8000 enc) */
    ofs[1] = *(u16 *)(c + 0x02);            /* scrollY (raw; PSX adds fb bias)   */
    SetDrawOffset(offScroll, ofs);
    AddPrim(ot, offScroll);
}

void RenderTilemapHorizontalScroll(void *layer) {
    u8 *c = (u8 *)layer;
    u32 buf;
    s16 cur;

    if (c[0x0A] == 0) {                     /* active flag */
        return;
    }
    buf = *(u8 *)((u8 *)g_pBlbHeapBase + 0xA088);
    cur = (s16)(-(s32)*(s16 *)(c + 0x00) >> 4);
    if (cur != *(s16 *)(c + 0x50 + buf * 4)) {
        port_rts16_window(c, buf);
    }
}

void RenderTilemapVerticalScroll(void *layer) {
    u8 *c = (u8 *)layer;
    u32 buf;
    s16 cur;

    if (c[0x0A] == 0) {
        return;
    }
    buf = *(u8 *)((u8 *)g_pBlbHeapBase + 0xA088);
    cur = (s16)(-(s32)*(s16 *)(c + 0x02) >> 4);
    if (cur != *(s16 *)(c + 0x52 + buf * 4)) {
        port_rts16_window(c, buf);
    }
}
