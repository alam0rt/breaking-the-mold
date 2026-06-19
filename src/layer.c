#include "common.h"
#include "Game/entity.h"

typedef struct {
    Entity *entity;
    u8 _pad04[20];
} LayerRenderSlot;

extern void FreeAllLayerRenderSlots(LayerRenderSlot *base);

INCLUDE_ASM("asm/nonmatchings/layer", ClearAllLayerRenderSlots);

INCLUDE_ASM("asm/nonmatchings/layer", ClearLayerRenderSlotsFromIndex);

INCLUDE_ASM("asm/nonmatchings/layer", DestroyLayerRendererObject);

INCLUDE_ASM("asm/nonmatchings/layer", LoadSpriteFramesToVRAM);

void LoadSpriteHashArrayToVRAM(u8 *heap, u8 **arr) {
    if (*arr == NULL) return;
    do {
        LoadSpriteFramesToVRAM(heap, *(u32 *)arr);
        arr++;
    } while (*arr != NULL);
}

INCLUDE_ASM("asm/nonmatchings/layer", FreeLayerRenderSlot);

extern void FreeLayerRenderSlot(u8 *base, u8 idx);

void FreeLayerSlotByEntityPointer(LayerRenderSlot *base, Entity *needle) {
    s16 i;
    for (i = 0; i < 20; i++) {
        LayerRenderSlot *slot = &base[i];
        if (slot->entity == needle) {
            FreeLayerRenderSlot((u8 *)base, (u8)i);
            return;
        }
    }
}

void FreeLayerSlotsByEntityList(LayerRenderSlot *base, Entity **list) {
    while (1) {
        Entity *needle;
        s16 i;
        if (*list == NULL) return;
        needle = *list;
        for (i = 0; i < 20; i++) {
            LayerRenderSlot *slot = &base[i];
            if (slot->entity == needle) {
                FreeLayerRenderSlot((u8 *)base, (u8)i);
                break;
            }
        }
        list++;
    }
}

void FreeAllLayerRenderSlots(LayerRenderSlot *base) {
    s16 i;
    for (i = 0; i < 20; i++) {
        LayerRenderSlot *slot = &base[i];
        if (slot->entity != NULL) {
            FreeLayerRenderSlot((u8 *)base, (u8)i);
        }
    }
}

LayerRenderSlot *func_800194F4(LayerRenderSlot *base, Entity *needle) {
    s16 i;
    for (i = 0; i < 20; i++) {
        LayerRenderSlot *slot = &base[i];
        if (slot->entity == needle) {
            return slot;
        }
    }
    return NULL;
}

Entity *ZeroEntityField(Entity *e) {
    *(s32 *)e = 0;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/layer", FindLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindNextLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindOrderingTableEntryByValue);

