/* =============================================================================
 * RenderTilemapWithWrapAround.c  --  PC-port bounded/wrap tile-layer render
 * =============================================================================
 * PC bring-up version of RenderTilemapWithWrapAround (0x80017AF8-family,
 * export/SLES_010.90.c; the per-frame render callback of layers built by
 * InitTileLayerPrimitives, vtable D_80010354 slot 3). The original culls the
 * layer against the visible window and submits only the visible row (or
 * column) chains, duplicating them with shifted DR_OFFSETs for wrap copies.
 *
 * SIMPLIFICATION (documented deviation): this version submits ALL row/column
 * chains every frame with a single scroll offset -- the GL scissor/window
 * clips offscreen prims, so output for non-wrapping layers is identical.
 * Wrap layers currently draw ONE copy (no seam duplication yet); revisit
 * against emulator captures when a wrap layer is visibly short.
 * PSX-isms dropped: fb-half Y bias (buf*0x100) and the 0x8000 11-bit offset
 * encoding (see tilemap_scroll.c).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void SetDrawOffset(void *p, u16 *ofs);
extern void AddPrim(void *ot, void *prim);
extern void AddPrims(void *ot, void *head, void *tail);

void RenderTilemapWithWrapAround(void *layer) {
    u8  *c = (u8 *)layer;
    u32  buf = *(u8 *)((u8 *)g_pBlbHeapBase + 0xA088);
    u32 *ot = *(u32 **)(*(u8 **)((u8 *)g_pBlbHeapBase + 0xA084) + 0x70);
    u8  *offReset  = c + buf * 12 + 0x20;
    u8  *offScroll = c + buf * 12 + 0x38;
    u8  *chains = *(u8 **)(c + 0x68 + buf * 4);
    s32  rows = (*(u16 *)(c + 0x7C) == 0) ? *(s16 *)(c + 0x76) : *(s16 *)(c + 0x74);
    u16  ofs[2];
    s32  i;

    ofs[0] = 0;
    ofs[1] = 0;
    SetDrawOffset(offReset, ofs);
    AddPrim(ot, offReset);
    for (i = 0; i < rows; i++) {
        s32 *chain = (s32 *)(chains + i * 8);
        if (chain[0] != 0) {
            AddPrims(ot, (void *)(intptr_t)chain[0], (void *)(intptr_t)chain[1]);
        }
    }
    ofs[0] = *(u16 *)(c + 0x00);    /* scrollX */
    ofs[1] = *(u16 *)(c + 0x02);    /* scrollY */
    SetDrawOffset(offScroll, ofs);
    AddPrim(ot, offScroll);
}
