#include "common.h"

INCLUDE_ASM("asm/nonmatchings/layer", ClearAllLayerRenderSlots);

INCLUDE_ASM("asm/nonmatchings/layer", ClearLayerRenderSlotsFromIndex);

INCLUDE_ASM("asm/nonmatchings/layer", DestroyLayerRendererObject);

INCLUDE_ASM("asm/nonmatchings/layer", LoadSpriteFramesToVRAM);

INCLUDE_ASM("asm/nonmatchings/layer", LoadSpriteHashArrayToVRAM);

INCLUDE_ASM("asm/nonmatchings/layer", FreeLayerRenderSlot);

INCLUDE_ASM("asm/nonmatchings/layer", FreeLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FreeLayerSlotsByEntityList);

INCLUDE_ASM("asm/nonmatchings/layer", FreeAllLayerRenderSlots);

INCLUDE_ASM("asm/nonmatchings/layer", func_800194F4);

INCLUDE_ASM("asm/nonmatchings/layer", ZeroEntityField);

INCLUDE_ASM("asm/nonmatchings/layer", FindLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindNextLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindOrderingTableEntryByValue);

