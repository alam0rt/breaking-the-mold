/* =============================================================================
 * layer_init.c  --  PC-port layer render-context builder (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/InitLayersAndTileState.s
 * (0x80024778, 0x57C bytes) -- the level's layer/tilemap render setup, the final
 * step of InitializeAndLoadLevel.
 *
 * Prologue: stores level pixel dimensions (GameState +0x48/+0x4A = tiles*16),
 * +0x78 = GetAsset100Field1C, +0x74 = GetLevelDataContextField3C.
 *
 * Per layer entry L (GetLayerEntry): skip if L[0x28]!=0 or L[0x26]==3. Layer 0
 * seeds GameState+0x124..0x126 from L[0x2C..0x2E]. Sets GameState scroll flags
 * (+0x58..+0x5B) from L[0x1E..0x21]. Then a size-class decision picks one of
 * three render-context initialisers (each over a 0x3C-byte heap alloc):
 *   Small  : when exactly one of {L[0x24]!=0 (flagA), (L[0x20]&0xFFFF0000)!=0
 *            (flagB)} holds AND metric>=L[0xE], further gated by w/h<0x40 and
 *            flagA=>w>=0x15, flagB=>h>screenTilesW.
 *   Medium : else, when w<0x81 && h<0x81 && metric>=L[0xE], minus a 0x40/0x80
 *            special-case clear.
 *   Standard: otherwise.
 * where metric = ((screenPxW>>4)+1)*0x15 (= 441 at 320px) and screenPxW =
 * g_pBlbHeapBase[+0x2]. Each context then receives the layer's scroll/offset
 * fields (L[0x10..0x1D] -> rc[0x20..0x3B]).
 *
 * All offsets/load-widths and the exact boolean structure of the size decision
 * are transcribed instruction-by-instruction from the disassembly (the decision
 * tree is silent-corruption-prone, so it is NOT taken from m2c's paraphrase).
 * Still-asm callees (InitLayerRenderContext_* / AddLayerToRenderList_*) resolve
 * to weak stubs until converted.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void  GetLevelDimensions(u16 *out, void *ctx);
extern s16   GetAsset100Field1C(void *ctx);
extern s32   GetLevelDataContextField3C(void *ctx);
extern s32   GetLayerCount(void *ctx);
extern void *GetLayerEntry(void *ctx, u32 idx);
extern void *GetTilemapDataPtr(void *ctx, s32 idx);
extern u8   *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void *InitLayerRenderContext_Small(void *storage, void *colorPtr, s16 tileId,
        void *tileState, void *tilemap, s32 pos0, s32 pos1, s32 w, s32 h, s32 sizeE,
        s32 flagA, s32 flagB, s32 mode);
extern void *InitLayerRenderContext_Medium(void *storage, void *colorPtr, s16 tileId,
        void *tileState, void *tilemap, s32 pos0, s32 pos1, s32 w, s32 h, s32 sizeE,
        s32 flagA, s32 flagB, s32 mode);
extern void *InitLayerRenderContext_Standard(void *storage, void *colorPtr, s16 tileId,
        void *tileState, void *tilemap, s32 pos0, s32 pos1, s32 w, s32 h,
        s32 flagA, s32 flagB, s32 mode);
extern void  AddLayerToRenderList_Small(void *gs, void *rc);
extern void  AddLayerToRenderList_Medium(void *gs, void *rc);
extern void  AddLayerToRenderList_Standard(void *gs, void *rc);

void InitLayersAndTileState(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *ctx = gs + 0x84;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u16 dims[2];
    u8  screenTilesW;                 /* sp48 = screenPxW >> 4 (u8) */
    s32 layerIdx;

    screenTilesW = (u8)((*(u16 *)(heap + 2)) >> 4);
    GetLevelDimensions(dims, ctx);
    *(s16 *)(gs + 0x48) = (s16)(dims[0] << 4);
    *(s16 *)(gs + 0x4A) = (s16)(dims[1] << 4);
    *(s16 *)(gs + 0x78) = GetAsset100Field1C(ctx);
    *(s32 *)(gs + 0x74) = GetLevelDataContextField3C(ctx);

    for (layerIdx = 0;
         (u32)(layerIdx & 0xFFFF) < (u32)(GetLayerCount(ctx) & 0xFFFF);
         layerIdx++) {
        u8 *L = (u8 *)GetLayerEntry(ctx, (u32)(layerIdx & 0xFFFF));
        s16 width, height;
        s32 flagA, flagB, metric;
        s32 useSmall, useMedium;
        void *storage, *tilemap, *rc;

        if ((layerIdx & 0xFFFF) == 0) {
            gs[0x124] = L[0x2C];
            gs[0x125] = L[0x2D];
            gs[0x126] = L[0x2E];
        }
        if (*(u16 *)(L + 0x28) != 0) {
            continue;
        }
        if (*(u16 *)(L + 0x26) == 3) {
            continue;
        }

        if (L[0x1E]) gs[0x59] = 1;
        if (L[0x21]) gs[0x5A] = 1;
        if (L[0x20]) gs[0x58] = 1;
        if (L[0x1F]) gs[0x5B] = 1;

        width  = (s16)*(u16 *)(L + 0x8);
        height = (s16)*(u16 *)(L + 0xA);
        flagA  = (*(u16 *)(L + 0x24) != 0);
        flagB  = ((*(s32 *)(L + 0x20) & 0xFFFF0000) != 0);
        metric = ((((s32)(*(u16 *)(heap + 2)) << 16 >> 20) + 1) * 0x15);

        /* ---- Small candidate: exactly one flag set AND metric >= L[0xE] ---- */
        useSmall = 0;
        if (flagA ^ flagB) {
            if (metric >= (s32)*(u16 *)(L + 0xE)) {
                useSmall = 1;
            }
        }
        /* validate Small */
        if (useSmall) {
            if (width >= 0x40 || height >= 0x40
                || (flagA && width < 0x15)
                || (flagB && !((s32)screenTilesW < (s32)height))) {
                useSmall = 0;
            }
        }

        /* ---- Medium candidate ---- */
        useMedium = 0;
        if (!useSmall && width < 0x81 && height < 0x81) {
            useMedium = (metric >= (s32)*(u16 *)(L + 0xE));
        }
        /* 0x40/0x80 special-case clear of Medium */
        if (((flagA && width != 0x40) || (flagB && height != 0x40)) && width != 0x80) {
            useMedium = 0;
        }

        /* ---- allocate + init (alloc BEFORE tilemap fetch, matching the asm) ---- */
        {
            s32 pos0 = *(s32 *)(L + 0x0);   /* {L[0x0],L[0x2]} packed */
            s32 pos1 = *(s32 *)(L + 0x4);   /* {L[0x4],L[0x6]} packed */
            s16 tileId = *(s16 *)(L + 0xC);
            void *tileState = *(void **)(gs + 0x108);
            s32 mode = (s32)L[0x26];

            if (useSmall) {
                storage = AllocateFromHeap(heap, 0x3C, 1, 0);
                tilemap = GetTilemapDataPtr(ctx, layerIdx & 0xFFFF);
                rc = InitLayerRenderContext_Small(storage, L + 0x2C, tileId, tileState,
                        tilemap, pos0, pos1, (s32)width, (s32)height,
                        (s32)*(s16 *)(L + 0xE), flagA, flagB, mode);
                AddLayerToRenderList_Small(gs, rc);
            } else if (useMedium) {
                storage = AllocateFromHeap(heap, 0x3C, 1, 0);
                tilemap = GetTilemapDataPtr(ctx, layerIdx & 0xFFFF);
                rc = InitLayerRenderContext_Medium(storage, L + 0x2C, tileId, tileState,
                        tilemap, pos0, pos1, (s32)width, (s32)height,
                        (s32)*(s16 *)(L + 0xE), flagA, flagB, mode);
                AddLayerToRenderList_Medium(gs, rc);
            } else {
                storage = AllocateFromHeap(heap, 0x3C, 1, 0);
                tilemap = GetTilemapDataPtr(ctx, layerIdx & 0xFFFF);
                rc = InitLayerRenderContext_Standard(storage, L + 0x2C, tileId, tileState,
                        tilemap, pos0, pos1, (s32)width, (s32)height,
                        flagA, flagB, mode);
                AddLayerToRenderList_Standard(gs, rc);
            }
        }

        /* ---- common tail: copy layer scroll/offset fields into the context ---- */
        *(s32 *)((u8 *)rc + 0x20) = *(s32 *)(L + 0x10);
        *(s32 *)((u8 *)rc + 0x24) = *(s32 *)(L + 0x14);
        *(u16 *)((u8 *)rc + 0x30) = *(u16 *)(L + 0x18);
        *(u16 *)((u8 *)rc + 0x32) = *(u16 *)(L + 0x1A);
        ((u8 *)rc)[0x3A] = L[0x1C];
        ((u8 *)rc)[0x3B] = L[0x1D];
    }
}
