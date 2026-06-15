#include "common.h"

extern void FreeAllLayerRenderSlots(u8 *base);

INCLUDE_ASM("asm/nonmatchings/layer", ClearAllLayerRenderSlots);

INCLUDE_ASM("asm/nonmatchings/layer", ClearLayerRenderSlotsFromIndex);

INCLUDE_ASM("asm/nonmatchings/layer", DestroyLayerRendererObject);

INCLUDE_ASM("asm/nonmatchings/layer", LoadSpriteFramesToVRAM);

void LoadSpriteHashArrayToVRAM(void *heap, u8 **arr) {
    if (*arr == NULL) return;
    do {
        LoadSpriteFramesToVRAM(heap, *(u32 *)arr);
        arr++;
    } while (*arr != NULL);
}

INCLUDE_ASM("asm/nonmatchings/layer", FreeLayerRenderSlot);

extern void FreeLayerRenderSlot(u8 *base, u8 idx);

void FreeLayerSlotByEntityPointer(u8 *base, void *needle) {
    s16 i;
    for (i = 0; i < 20; i++) {
        u8 *slot = base + i * 24;
        if (*(void **)slot == needle) {
            FreeLayerRenderSlot(base, (u8)i);
            return;
        }
    }
}

void FreeLayerSlotsByEntityList(u8 *base, void **list) {
    while (1) {
        void *needle;
        s16 i;
        if (*list == NULL) return;
        needle = *list;
        for (i = 0; i < 20; i++) {
            u8 *slot = base + i * 24;
            if (*(void **)slot == needle) {
                FreeLayerRenderSlot(base, (u8)i);
                break;
            }
        }
        list++;
    }
}

void FreeAllLayerRenderSlots(u8 *base) {
    s16 i;
    for (i = 0; i < 20; i++) {
        u8 *slot = base + i * 24;
        if (*(void **)slot != NULL) {
            FreeLayerRenderSlot(base, (u8)i);
        }
    }
}

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

