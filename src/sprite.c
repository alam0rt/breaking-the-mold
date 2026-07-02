#include "common.h"
#include "Game/sprite_records.h"

extern void *PRIM_OBJECT_BASE_VTABLE asm("D_8001039C");
extern u8 *g_pBlbHeapBase;
extern void RenderTilemapHorizontalScroll(TilemapLayerRenderObject *e);
extern void RenderTilemapVerticalScroll(TilemapLayerRenderObject *e);
extern void SetupTilemapPrimitives(TilemapLayerRenderObject *e);

BasicPrimObject *InitBasicEntityWithVtable(BasicPrimObject *p, u16 val) {
    p->vtable = &PRIM_OBJECT_BASE_VTABLE;
    p->id = val;
    p->x = 0;
    p->y = 0;
    p->width = 0;
    p->height = 0;
    p->enabled = 1;
    return p;
}

INCLUDE_ASM("asm/nonmatchings/sprite", PrepareSpriteVRAMSlotForContext);

INCLUDE_ASM("asm/nonmatchings/sprite", InitSpriteContextDefaults);

INCLUDE_ASM("asm/nonmatchings/sprite", FreeTextureResource);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderSpriteOrScaledQuad);

INCLUDE_ASM("asm/nonmatchings/sprite", UploadTextureToVRAM);

INCLUDE_ASM("asm/nonmatchings/sprite", UploadCLUTToVRAM);

INCLUDE_ASM("asm/nonmatchings/sprite", InitTilemapLayerRendering);

INCLUDE_ASM("asm/nonmatchings/sprite", FreeMultiAllocResource);

void RenderTilemapLayerWithScroll(TilemapLayerRenderObject *e) {
    u8 idx;
    RenderTilemapHorizontalScroll(e);
    RenderTilemapVerticalScroll(e);
    SetupTilemapPrimitives(e);
    idx = g_pBlbHeapBase[0xA088];
    e->frameScroll[idx].x = -e->scrollX >> 4;
    e->frameScroll[idx].y = -e->scrollY >> 4;
}

INCLUDE_ASM("asm/nonmatchings/sprite", RenderTilemapHorizontalScroll);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderTilemapVerticalScroll);

INCLUDE_ASM("asm/nonmatchings/sprite", SetupTilemapPrimitives);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderTilemapSprites16x16);

INCLUDE_ASM("asm/nonmatchings/sprite", InitTilemapLayer16x16);

INCLUDE_ASM("asm/nonmatchings/sprite", FreeResourceType2);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderTilemapPrimitivesWithBounds);

INCLUDE_ASM("asm/nonmatchings/sprite", InitTileLayerPrimitives);

INCLUDE_ASM("asm/nonmatchings/sprite", FreeResourceType3);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderTilemapWithWrapAround);

