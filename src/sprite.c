#include "common.h"

INCLUDE_ASM("asm/nonmatchings/sprite", InitBasicEntityWithVtable);

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

