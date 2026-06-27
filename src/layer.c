#include "common.h"
#include "Game/entity.h"

typedef struct {
    Entity *entity;
    u8 _pad04[20];
} LayerRenderSlot;

extern void FreeAllLayerRenderSlots(LayerRenderSlot *base);
extern void builtin_delete(void *ptr);

INCLUDE_ASM("asm/nonmatchings/layer", ClearAllLayerRenderSlots);

INCLUDE_ASM("asm/nonmatchings/layer", ClearLayerRenderSlotsFromIndex);

/* Standard 0x80011228-style destructor for the layer-renderer object:
 * walks every render slot tearing it down, then (if flags & 1) frees
 * the object via the C++ delete entry point.
 *
 * SHELVED: 405-byte diff because cc1 emits `move v0, a0` BEFORE the
 * prologue (the C++ destructor's implicit `return this`), and cc1's
 * C frontend won't reproduce that pre-prologue write even when the
 * function signature returns the pointer. Equivalent C:
 *   LayerRenderSlot *DestroyLayerRendererObject(LayerRenderSlot *obj, s32 flags) {
 *       FreeAllLayerRenderSlots(obj);
 *       if (flags & 1) builtin_delete(obj);
 *       return obj;
 *   }
 */
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

/* Scan the 20-entry layer-render-slot table (base @ gp+8 = D_800A595C) for
 * the slot whose entity pointer matches `needle`, writing &slot (or NULL if
 * absent) through *out. Equivalent C (shelved):
 *   void FindLayerSlotByEntityPointer(LayerRenderSlot **out, Entity *needle) {
 *       LayerRenderSlot *base = g_LayerRenderSlots; // gp+8
 *       LayerRenderSlot *slot; s16 i;
 *       for (i = 0; i < 20; i++) { slot = &base[i];
 *           if (slot->entity == needle) goto found; }
 *       slot = NULL;
 *   found: *out = slot;
 *   }
 *
 * MIS-SPLIT FIXED: splat had carved the loop-continuation+epilogue off as a
 * bogus FindNextLayerSlotByEntityPointer symbol. Merged into this single
 * 0x54-byte function in symbol_addrs.txt. SHELVED as ASM (two blockers):
 *  1. gp-rel base load `lw a3,8(gp)`: D_800A595C is INITIALIZED small data
 *     (.word D_8009AE58 in the sdata segment), so the usual tentative-def +
 *     --use-comm-section trick does NOT work — it adds storage and shifts
 *     the whole sdata segment by 8 (g_pBlbHeapBase etc. move). A plain
 *     `extern` emits absolute. Needs the initialized global defined in this
 *     TU (and removed from the splat data segment) to get gp-rel.
 *  2. loop branch-sense: with the global stubbed, the body matches except
 *     TARGET uses `beq`(match)->shared-store while cc1 picks `bne`(skip);
 *     permuter floored at 160, never 0. */
INCLUDE_ASM("asm/nonmatchings/layer", FindLayerSlotByEntityPointer);

INCLUDE_ASM("asm/nonmatchings/layer", FindOrderingTableEntryByValue);

