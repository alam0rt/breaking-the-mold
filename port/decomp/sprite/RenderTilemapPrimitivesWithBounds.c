/* =============================================================================
 * RenderTilemapPrimitivesWithBounds.c  --  PC-port tilemap layer render callback
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c RenderTilemapPrimitivesWithBounds
 * (0x8001xxxx, INCLUDE_ASM in src/sprite.c). The render callback (vtable +0xC) of
 * a bounded 16x16 tilemap layer: off-screen culls the layer against the camera,
 * then links its pre-built SPRT_16 primitive chain into the ordering table,
 * bracketed by two DR_OFFSET packets that shift the whole layer for scrolling.
 *
 * The SPRT_16 chain + the two DR_OFFSET slots live inside the layer struct
 * (param_1), pre-allocated/pre-filled by InitTilemapLayer16x16 at level load.
 * The active frame buffer index is g_pBlbHeapBase+0xA088 (uVar1); the GPU
 * context ptr is g_pBlbHeapBase+0xA084 and its OT head is at +0x70.
 *
 * param_1 is u_short* -> param_1[N] is the short at byte N*2; primitive slots
 * are addressed as `param_1 + wordIndex` (short-pointer arithmetic).
 * Layer fields: screenX@param_1[0], screenY@param_1[1], noBoundsX@byte+0x6c
 * (param_1[0x36] lo), noBoundsY@byte+0x6d, tileX@[0x30], tileY@[0x31],
 * spanW@[0x32], spanH@[0x33].
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;
extern void  SetDrawOffset(void *p, u16 *ofs);
extern void  AddPrim(void *ot, void *prim);
extern void  AddPrims(void *ot, void *head, void *tail);

void RenderTilemapPrimitivesWithBounds(u16 *param_1) {
    u8  *heap = (u8 *)g_pBlbHeapBase;
    u32  uVar1;
    u16  local_28;
    s16  local_26;
    void *ot;

    /* ---- X-axis off-screen cull (unless noBoundsX) ---- */
    if ((char)param_1[0x36] == 0) {
        if ((s32)(s16)param_1[0x30] + (s32)(s16)param_1[0x32] <= -(s32)(s16)param_1[0] >> 4) {
            return;
        }
        if (-(s32)(s16)param_1[0] + (s32)*(s16 *)heap - 1 >> 4 < (s32)(s16)param_1[0x30]) {
            return;
        }
    }
    /* ---- Y-axis off-screen cull (unless noBoundsY) ---- */
    if (*(char *)((u8 *)param_1 + 0x6d) == 0) {
        if ((s32)(s16)param_1[0x31] + (s32)(s16)param_1[0x33] <= -(s32)(s16)param_1[1] >> 4) {
            return;
        }
        if (-(s32)(s16)param_1[1] + (s32)*(s16 *)(heap + 2) - 1 >> 4 < (s32)(s16)param_1[0x31]) {
            return;
        }
    }

    uVar1 = (u32)*(u8 *)(heap + 0xA088);          /* active buffer index */
    ot = *(void **)(*(s32 *)(heap + 0xA084) + 0x70);

    /* ---- pre-scroll DR_OFFSET + the tilemap SPRT_16 chain ---- */
    local_28 = 0;
    local_26 = (s16)((u16)*(u8 *)(heap + 0xA088) << 8);
    SetDrawOffset(param_1 + uVar1 * 6 + 0x10, &local_28);
    AddPrim(ot, param_1 + uVar1 * 6 + 0x10);
    AddPrims(ot, *(void **)(param_1 + uVar1 * 2 + 0x28),
                 *(void **)(param_1 + uVar1 * 2 + 0x2c));

    /* ---- post-scroll DR_OFFSET (restore origin, offset by layer position) ---- */
    local_28 = param_1[0];
    if ((s16)local_28 < 0) {
        local_28 = local_28 + 0x8000;
    }
    local_26 = (s16)(uVar1 * 0x100);
    if ((s32)((s32)(s16)param_1[1] + uVar1 * 0x100) < 0) {
        local_26 = local_26 + -0x8000;
    }
    local_26 = param_1[1] + local_26;
    SetDrawOffset(param_1 + uVar1 * 6 + 0x1c, &local_28);
    AddPrim(ot, param_1 + uVar1 * 6 + 0x1c);
}
