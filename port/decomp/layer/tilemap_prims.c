/* =============================================================================
 * tilemap_prims.c  --  PC-port medium-layer tilemap primitive builder
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/sprite/InitTilemapLayer16x16.s
 * (0x8001F250, 341 lines; reference = export/SLES_010.90.c 7913). Builds the
 * double-buffered SPRT_16 primitive lists for a 16x16-tile layer: for every
 * non-zero tilemap cell it emits one SPRT_16 textured sprite, buckets it by its
 * texture page, and chains the buckets (each headed by a DR_TPAGE descriptor)
 * into the layer's render list. The runtime renderer (RenderTilemapSprites16x16)
 * walks these prebuilt chains each frame.
 *
 * Sub-object layout (0x70 bytes, byte offsets):
 *   +0x08 u16 tileId    +0x0C vtable      +0x0A u8 active(1)
 *   +0x10/+0x14 DR_TPAGE array ptr (buf0/buf1, 0x2C entries x 8B)
 *   +0x18/+0x1C SPRT_16 array ptr (buf0/buf1, `count` entries x 0x10B)
 *   +0x50/+0x54 final render-list head (buf0/buf1)
 *   +0x58/+0x5C final render-list tail (buf0/buf1)
 *   +0x60 u16 x  +0x62 u16 y  +0x64 u16 w  +0x66 u16 h   (x/y/w/h in tiles)
 *   +0x68 colorTable ptr   +0x6C u8 flagA   +0x6D u8 flagB
 *
 * Tilemap cell (u16): bits 0-11 = 1-based tile index, bits 12-15 = tint index.
 * Tile table entry (8 bytes at colorTable-independent tileTable + idx*8):
 *   +0x0 u16 tpageSel  +0x2 u16 clut  +0x4 u8 u0 /+0x5 u8 v0  +0x6 u8 semiTrans
 * Tint table: colorTable + tint*3 = {r,g,b}.
 * SPRT_16 (16 bytes, PSY-Q layout) filled via named fields.
 *
 * The bucket tracker is a 352-byte stack scratch: heads at [bucket*4], tails at
 * [bucket*4 + 0x2C*4]. Bucket index derived from the tile's tpage selector bits.
 *
 * NOTE (CP-2.4): the final sprite in each chain is left with whatever tag the
 * heap array held -- OT termination is applied by the frame-time render walk,
 * not here (faithful to the original). Pixel-exact CLUT/4-bit sampling is a
 * gpu_gl.c Phase-3 concern; this function only builds the primitive data.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "port_hal.h"
#include <stdint.h>

extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *CatPrim(void *p0, void *p1);

/* Sprite render vtables at fixed rodata (weak-backed on PC). The sub-object's
 * render-callback vtable at +0xC is what RenderEntities dispatches each frame. */
extern u8 D_8001039C[] asm("D_8001039C");   /* PrimObject base vtable    */
extern u8 D_80010364[] asm("D_80010364");   /* 16x16 tilemap render vtable */

#define SUB_U16(c, off)  (*(u16 *)((u8 *)(c) + (off)))
#define SUB_PTR(c, off)  (*(u8 **)((u8 *)(c) + (off)))

void *InitTilemapLayer16x16(void *storage, void *colorTable, s16 tileId, s32 tileTable,
        u16 *tilemap, u32 pos0, u32 pos1, s16 count, u8 flagA, u8 flagB, u8 tpageBank) {
    u8 *c = (u8 *)storage;
    s32 heads[0x2C];       /* bucket head ptr (as int)                         */
    s32 tails[0x2C];       /* bucket tail ptr (as int)                         */
    u8  buf;
    s32 i;
    u16 pixY;

    /* ---- sub-object header ---- */
    *(void **)(c + 0x68) = colorTable;
    SUB_U16(c, 0x08) = (u16)tileId;
    *(void **)(c + 0x0C) = D_8001039C;   /* base vtable (overwritten below)   */
    *(u16 *)(c + 0x00) = 0;
    *(u16 *)(c + 0x02) = 0;
    *(u16 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x06) = 0;
    c[0x0A] = 1;
    *(void **)(c + 0x0C) = D_80010364;   /* 16x16 tilemap render vtable       */
    c[0x6C] = flagA;
    c[0x6D] = flagB;
    /* pos0 = {x@lo, y@hi}, pos1 = {w@lo, h@hi} */
    SUB_U16(c, 0x60) = (u16)(pos0 & 0xFFFF);
    SUB_U16(c, 0x62) = (u16)(pos0 >> 16);
    SUB_U16(c, 0x64) = (u16)(pos1 & 0xFFFF);
    SUB_U16(c, 0x66) = (u16)(pos1 >> 16);

    /* ---- DR_TPAGE descriptor arrays (double-buffered, 0x2C entries) ---- */
    SUB_PTR(c, 0x10) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, 0x2C, 0);
    SUB_PTR(c, 0x14) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 8, 0x2C, 0);
    for (i = 0; i < 0x2C; i++) {
        u32 sub = (u32)(i % 0xB) + 5;
        s32 tpage;
        if (10 < (i % 0x16)) {
            sub = (u32)(i % 0xB) + 0x15;
        }
        tpage = (s32)(((i / 0x16) * 0x80) & 0xFFFF) + (s32)(sub & 0xFFFF)
                + (s32)tpageBank * 0x20;
        SetDrawTPage((DR_TPAGE *)(SUB_PTR(c, 0x10) + i * 8), 0, 0, tpage);
        SetDrawTPage((DR_TPAGE *)(SUB_PTR(c, 0x14) + i * 8), 0, 0, tpage);
    }

    /* ---- SPRT_16 arrays (double-buffered, `count` entries) ---- */
    SUB_PTR(c, 0x18) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10, (s32)count, 0);
    SUB_PTR(c, 0x1C) = AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x10, (s32)count, 0);

    for (buf = 0; buf < 2; buf++) {
        u8 *tpageArr = SUB_PTR(c, 0x10 + buf * 4);
        SPRT_16 *p  = (SPRT_16 *)SUB_PTR(c, 0x18 + buf * 4);
        u16 pixX0   = (u16)(SUB_U16(c, 0x60) << 4);
        s16 row;

        for (i = 0; i < 0x2C; i++) {
            heads[i] = 0;
        }
        pixY = (u16)(SUB_U16(c, 0x62) << 4);
        row = 0;
        {
            u16 *cell = tilemap;
            while (row < (s16)SUB_U16(c, 0x66)) {           /* h */
                u16 pixX = pixX0;
                s16 col;
                for (col = 0; col < (s16)SUB_U16(c, 0x64); col++) {   /* w */
                    u16 t = *cell;
                    if (t != 0) {
                        u16 *tile = (u16 *)((u8 *)(uintptr_t)tileTable + (u32)(t & 0xFFF) * 8);
                        u8  *tint = (u8 *)colorTable + (u32)(t >> 12) * 3;
                        u32 sel, bucket;

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

                        sel = (u32)tile[0];
                        bucket = ((sel & 0x180) >> 7) * 0x16 + 0xFFFB + (sel & 0xF);
                        if (0xF < (sel & 0x1F)) {
                            bucket += 0xB;
                        }
                        bucket &= 0xFFFF;
                        if (heads[bucket] == 0) {
                            CatPrim((void *)(tpageArr + bucket * 8), p);
                            tails[bucket] = (s32)(uintptr_t)p;
                            heads[bucket] = (s32)(uintptr_t)(tpageArr + bucket * 8);
                        } else {
                            CatPrim((void *)(uintptr_t)tails[bucket], p);
                            tails[bucket] = (s32)(uintptr_t)p;
                        }
                        p++;
                    }
                    cell++;
                    pixX = (u16)(pixX + 0x10);
                }
                pixY = (u16)(pixY + 0x10);
                row++;
            }
        }

        /* ---- chain the non-empty buckets into the final render list ---- */
        *(s32 *)(c + 0x50 + buf * 4) = 0;
        for (i = 0; i < 0x2C; i++) {
            if (heads[i] != 0) {
                if (*(s32 *)(c + 0x50 + buf * 4) == 0) {
                    *(s32 *)(c + 0x50 + buf * 4) = heads[i];
                } else {
                    CatPrim(*(void **)(c + 0x58 + buf * 4), (void *)(uintptr_t)heads[i]);
                }
                *(s32 *)(c + 0x58 + buf * 4) = tails[i];
            }
        }
    }
    return storage;
}
