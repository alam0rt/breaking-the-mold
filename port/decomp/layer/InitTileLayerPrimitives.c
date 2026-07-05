/* =============================================================================
 * InitTileLayerPrimitives.c  --  PC-port bounded tile-layer primitive builder
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/sprite/InitTileLayerPrimitives.s
 * (0x80017D0C, 0x404; reference = export/SLES_010.90.c 8255). Cousin of
 * InitTilemapLayer16x16 (tilemap_prims.c): builds the double-buffered SPRT_16
 * chains for a BOUNDED tile layer (vtable D_80010354, bounded-cull renderer)
 * instead of the wrap-scrolling screen grid. Differences from the 16x16 builder:
 *   - one DR_TPAGE per sprite (paired pools), not per-tpage buckets;
 *   - prims are chained into PER-ROW (or per-COLUMN when wrapX != 0) {head,
 *     tail} lists at +0x68/+0x6C, so the frame renderer can cull whole rows;
 *   - the tpage word gets the layer attribute injected as `tile[0] | attr<<5`.
 *
 * Ghidra's parameter comment mislabels param_6/param_7 as clutId/tpageId; the
 * swl/swr splices store them as the packed layer ORIGIN {x@lo16,y@hi16} at
 * +0x70 and SIZE {w@lo16,h@hi16} at +0x74 (in tiles), exactly like pos0/pos1
 * of InitTilemapLayer16x16 -- verified against the loop bounds in the .s.
 *
 * Layer sub-object (byte offsets):
 *   +0x00..07 zeroed   +0x08 u16 tileId   +0x0A u8 active=1   +0x0C vtable
 *   +0x10/+0x14 SPRT_16 pool buf0/1      +0x18/+0x1C DR_TPAGE pool buf0/1
 *   +0x68/+0x6C row-chain arrays buf0/1 ({s32 head, s32 tail} x rows)
 *   +0x70 u16 x  +0x72 u16 y  +0x74 u16 w  +0x76 u16 h  (tiles, unaligned)
 *   +0x78 colorTable ptr    +0x7C u16 wrapX  +0x7E u16 wrapY
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "port_hal.h"
#include <stdint.h>

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *CatPrim(void *p0, void *p1);

extern u8 D_8001039C[] asm("D_8001039C");   /* PrimObject base vtable        */
extern u8 D_80010354[] asm("D_80010354");   /* bounded tile-layer render vtable */

void *InitTileLayerPrimitives(void *storage, void *colorTable, s16 tileId,
        s32 tileTable, u16 *tilemap, u32 pos0, u32 pos1, s16 primCount,
        u16 wrapX, u16 wrapY, u8 attrFlag) {
    u8 *c = (u8 *)storage;
    s32 rows, i;
    u8  buf;
    s16 tileX, tileY;

    *(void **)(c + 0x78) = colorTable;
    *(void **)(c + 0x0C) = D_8001039C;
    *(u16 *)(c + 0x08) = (u16)tileId;
    *(u16 *)(c + 0x00) = 0;
    *(u16 *)(c + 0x02) = 0;
    *(u16 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x06) = 0;
    c[0x0A] = 1;
    *(void **)(c + 0x0C) = D_80010354;
    *(u16 *)(c + 0x7C) = wrapX;
    *(u16 *)(c + 0x7E) = wrapY;
    /* packed origin/size (the .s stores these with swl/swr -- plain u16 pairs
     * on PC, the layout is identical) */
    *(u16 *)(c + 0x70) = (u16)(pos0 & 0xFFFF);      /* x (tiles) */
    *(u16 *)(c + 0x72) = (u16)(pos0 >> 16);         /* y */
    *(u16 *)(c + 0x74) = (u16)(pos1 & 0xFFFF);      /* w */
    *(u16 *)(c + 0x76) = (u16)(pos1 >> 16);         /* h */

    /* prim pools: SPRT_16 x2 then DR_TPAGE x2, primCount entries each */
    *(u8 **)(c + 0x10) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10, primCount, 0);
    *(u8 **)(c + 0x14) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10, primCount, 0);
    *(u8 **)(c + 0x18) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, primCount, 0);
    *(u8 **)(c + 0x1C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, primCount, 0);

    /* row-chain arrays: one {head, tail} pair per row (or per column when the
     * layer wraps in X) */
    rows = (*(u16 *)(c + 0x7C) == 0) ? *(s16 *)(c + 0x76) : *(s16 *)(c + 0x74);
    *(u8 **)(c + 0x68) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, rows, 0);
    *(u8 **)(c + 0x6C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, rows, 0);
    for (i = 0; i < rows; i++) {
        *(s32 *)(*(u8 **)(c + 0x68) + i * 8) = 0;
        *(s32 *)(*(u8 **)(c + 0x6C) + i * 8) = 0;
    }

    tileX = *(s16 *)(c + 0x70);
    tileY = *(s16 *)(c + 0x72);
    for (buf = 0; buf < 2; buf++) {
        u16     *cell = tilemap;
        SPRT_16 *p    = (SPRT_16 *)*(u8 **)(c + 0x10 + buf * 4);
        DR_TPAGE *pt  = (DR_TPAGE *)*(u8 **)(c + 0x18 + buf * 4);
        u8      *chains = *(u8 **)(c + 0x68 + buf * 4);
        u16      pixY = (u16)(tileY << 4);
        s16      row, col;

        for (row = 0; row < *(s16 *)(c + 0x76); row++) {
            u16 pixX = (u16)(tileX << 4);
            for (col = 0; col < *(s16 *)(c + 0x74); col++) {
                u16 t = *cell;
                if (t != 0) {
                    u16 *tile = (u16 *)((u8 *)(uintptr_t)tileTable + (u32)(t & 0xFFF) * 8);
                    u8  *tint = (u8 *)colorTable + (u32)(t >> 12) * 3;
                    s32 *chain;
                    u16 idx;

                    SetSprt16(p);
                    SetSemiTrans(p, (s32)(u8)(((u8 *)tile)[6]));
                    SetShadeTex(p, 0);
                    p->r0   = tint[0];
                    p->g0   = tint[1];
                    p->b0   = tint[2];
                    p->x0   = (s16)pixX;
                    p->y0   = (s16)pixY;
                    p->u0   = (u8)tile[2];
                    p->v0   = ((u8 *)tile)[5];
                    p->clut = tile[1];
                    SetDrawTPage(pt, 0, 0, (s32)((u32)tile[0] | ((u32)attrFlag << 5)));

                    idx = (u16)((*(u16 *)(c + 0x7C) != 0) ? col : row);
                    chain = (s32 *)(chains + (u32)idx * 8);
                    if (chain[0] == 0) {
                        chain[0] = (s32)(uintptr_t)pt;
                    } else {
                        CatPrim((void *)(uintptr_t)chain[1], pt);
                    }
                    chain[1] = (s32)(uintptr_t)pt;
                    pt++;
                    CatPrim((void *)(uintptr_t)chain[1], p);
                    chain[1] = (s32)(uintptr_t)p;
                    p++;
                }
                cell++;
                pixX = (u16)(pixX + 0x10);
            }
            pixY = (u16)(pixY + 0x10);
        }
    }
    return storage;
}
