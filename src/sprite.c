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

/* InitSpriteContextDefaults @ 0x80015614 — zero-init a sprite render context +
 * install default vtable (+0xC), 1.0 scale (0x10000 @ +0x1C/+0x20), white tint
 * (+0x34..0x36=0x40); id@+0x8, +0xA/+0x33=1. The ROM writes +0xC/+0x4/+0x6
 * TWICE (a base-init prologue — vtable D_8001039C — then the sprite override
 * to D_80010384), but cc1 2.7.2 dead-store-eliminates literal double
 * assignments, so verbatim C collapses to one store each. The surviving dupes
 * come from an inlined base-init helper whose exact source isn't recoverable
 * here. Shelved. */
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

