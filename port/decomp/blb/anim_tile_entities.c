/* =============================================================================
 * anim_tile_entities.c  --  PC-port animated-tile entity/render-state init
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/InitAnimatedTileEntities.s
 * (0x80024F34, 0x194 bytes). Walks the level's tiles (1..GameState+0x104) and,
 * for each tile flagged animated (GetTileSizeFlags()[tile-1] & 4), fills that
 * tile's 8-byte render-state entry in the GameState+0x108 array so the tile
 * samples from the shared animated-tile sprite sheet.
 *
 * The first animated tile lazily creates ONE backing entity
 * (InitPlayerEntity over a 0x100-byte heap alloc), registers it on the sorted
 * render list, and caches its sprite context (entity+0x34) + tiles-per-row
 * ((ctx->width + 0xF) >> 4). Each tile's sheet cell = (idx % tpr, idx / tpr):
 *   entry[0x0]=ctx tpage(u16) entry[0x2]=ctx clut(u16)
 *   entry[0x4]=ctx.baseX + (idx%tpr)*16   entry[0x5]=ctx.baseY + (idx/tpr)*16
 *   entry[0x6]=1 (valid)
 * where ctx fields are +0x24 tpage,+0x26 clut,+0x30 baseX,+0x31 baseY,+0x04 w.
 * Div is unsigned (matches divu); idx = GetAnimatedTileData()-1.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8  *GetTileSizeFlags(void *ctx);
extern s32  GetAnimatedTileData(void *ctx, s16 tile);
extern void *InitPlayerEntity(void *storage);
extern void AddEntityToSortedRenderList(void *gs, void *entity);
extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);

void InitAnimatedTileEntities(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *sizeFlags = GetTileSizeFlags(gs + 0x84);
    void *entity = NULL;
    u8 *sctx = NULL;              /* entity sprite context (entity+0x34) */
    u32 tilesPerRow = 0;
    s32 i;

    for (i = 1; i < (s32)*(u16 *)(gs + 0x104); i++) {
        s32 idx;
        u32 tpr, row, col;
        u8 *entry;

        if (!(sizeFlags[i - 1] & 4)) {
            continue;
        }
        idx = GetAnimatedTileData(gs + 0x84, (s16)i) - 1;

        if (entity == NULL) {
            entity = InitPlayerEntity(AllocateFromHeap((u8 *)g_pBlbHeapBase, 0x100, 1, 0));
            AddEntityToSortedRenderList(gs, entity);
            sctx = *(u8 **)((u8 *)entity + 0x34);
            tilesPerRow = (u32)(*(s16 *)(sctx + 0x4) + 0xF) >> 4;
        }

        tpr = tilesPerRow & 0xFF;
        col = (u32)idx % tpr;
        row = (u32)idx / tpr;
        entry = *(u8 **)(gs + 0x108) + i * 8;
        *(u16 *)(entry + 0x0) = *(u16 *)(sctx + 0x24);            /* tpage */
        *(u16 *)(entry + 0x2) = *(u16 *)(sctx + 0x26);            /* clut  */
        entry[0x4] = (s8)(*(u8 *)(sctx + 0x30) + col * 16);       /* subX  */
        entry[0x5] = (s8)(*(u8 *)(sctx + 0x31) + row * 16);       /* y     */
        entry[0x6] = 1;                                           /* valid */
    }
}
