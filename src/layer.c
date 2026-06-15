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

u8 *func_800194F4(u8 *base, void *needle) {
    s16 i;
    for (i = 0; i < 20; i++) {
        u8 *slot = base + i * 24;
        if (*(void **)slot == needle) {
            return slot;
        }
    }
    return NULL;
}

void *ZeroEntityField(void *e) {
    *(s32 *)e = 0;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/layer", FindLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindNextLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindOrderingTableEntryByValue);

