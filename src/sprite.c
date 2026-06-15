#include "common.h"

extern void *D_8001039C;

void *InitBasicEntityWithVtable(void *e, u16 val) {
    u8 *p = (u8 *)e;
    *(u32 *)(p + 0xC) = (u32)&D_8001039C;
    *(u16 *)(p + 0x8) = val;
    *(u16 *)(p + 0x0) = 0;
    *(u16 *)(p + 0x2) = 0;
    *(u16 *)(p + 0x4) = 0;
    *(u16 *)(p + 0x6) = 0;
    *(u8 *)(p + 0xA) = 1;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/sprite", PrepareSpriteVRAMSlotForContext);

INCLUDE_ASM("asm/nonmatchings/sprite", InitSpriteContextDefaults);

INCLUDE_ASM("asm/nonmatchings/sprite", FreeTextureResource);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderSpriteOrScaledQuad);

INCLUDE_ASM("asm/nonmatchings/sprite", UploadTextureToVRAM);

INCLUDE_ASM("asm/nonmatchings/sprite", UploadCLUTToVRAM);

INCLUDE_ASM("asm/nonmatchings/sprite", InitTilemapLayerRendering);

INCLUDE_ASM("asm/nonmatchings/sprite", FreeMultiAllocResource);

INCLUDE_ASM("asm/nonmatchings/sprite", RenderTilemapLayerWithScroll);

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

