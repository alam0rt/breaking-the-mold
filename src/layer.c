#include "common.h"
#include "Game/entity.h"
#include "Game/callback_slot.h"
#include "Game/layer_records.h"
#include "Game/sprite.h"

extern void FreeAllLayerRenderSlots(LayerRenderSlot *base);
extern void builtin_delete(void *ptr);

/* ClearAllLayerRenderSlots @ 0x80018D54 — merged its mis-split loop tail
 * (ClearLayerRenderSlotsFromIndex, an internal back-branch target) into one
 * 0x3C function. Body is `do { slot=&base[i]; slot->entity=NULL; i++; }
 * while (i<0x14)` (take the slot addr into a temp so the final `addu` is
 * base-first, matching cc1's operand order).
 * UNMATCHABLE AS STANDALONE C: the original is exactly 15 instrs ending in
 * `jr ra` whose delay slot bleeds into the next function (DestroyLayerRendererObject)
 * — there is NO trailing nop. cc1 emits `jr ra; nop` for a leaf, making the
 * function 0x40 and shifting everything after (breaks the INIT_TABLES vtable
 * pointer at 0x800103b8). Keep as asm. */
/* ClearAllLayerRenderSlots: unit spans 0x80018D54..0x80018DDC — absorbs former split symbols DestroyLayerRendererObject (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/layer", ClearAllLayerRenderSlots);


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

INCLUDE_ASM("asm/nonmatchings/layer", FindOrderingTableSlotById);

/* Returns the total animation frame count of the SpriteContext referenced by
 * the given slot (the slot's first member is a SpriteContext pointer). */
u16 GetSpriteFrameCount(SpriteContext **ppSprite) {
    return (*ppSprite)->total_frame_count;
}

/* Resolve the pointer to a static sprite frame's metadata record: index the
 * slot's frame table (secondary_ptr) by frameIndex (0x14-byte entries) and
 * return that entry's first word. */
void *GetSpriteFrameDataByIndex(SpriteContext **ppSprite, u16 frameIndex) {
    u8 *table = (*ppSprite)->secondary_ptr;
    return *(void **)(table + frameIndex * 0x14);
}

INCLUDE_ASM("asm/nonmatchings/layer", SetupSpriteFromFrame);

INCLUDE_ASM("asm/nonmatchings/layer", FreeAllLayerRenderSlotsWrapper);

INCLUDE_ASM("asm/nonmatchings/layer", ClearAllLayerRenderSlots_CrtInit);

extern u8 g_EntityVtable_Destroyed[];

/* Zero-initialise the 0x1C-byte MenuEntityBase header and install the
 * "destroyed" placeholder vtable; callers overwrite the vtable once the
 * entity is fully built. Returns the entity (callers ignore it). `slot` is
 * an unreferenced frame pad (0x10). */
void *InitMenuEntityWithVtable(void *entity, s32 zOrder) {
    PadSlot slot;
    MenuEntityBase *e = entity;

    /* fence: keep the `li 1` for `active` below the tick-slot stores */
    do {
        e->vtable = g_EntityVtable_Destroyed;
        e->tickMarker = 0;
        e->tickMarkerHi = 0;
    } while (0);
    e->tickCallback = NULL;
    e->eventMarker = 0;
    e->eventMarkerHi = 0;
    e->eventCallback = NULL;
    e->zOrder = zOrder;
    e->active = 1;
    e->reserved12 = 0;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/layer", InitMenuSpriteRenderContext);

INCLUDE_ASM("asm/nonmatchings/layer", MenuEntityDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/layer", CLUTPaletteCycleTickCallback);

INCLUDE_ASM("asm/nonmatchings/layer", CLUTColorLerpTickCallback);

INCLUDE_ASM("asm/nonmatchings/layer", UploadTextureOrClut);

INCLUDE_ASM("asm/nonmatchings/layer", CopyTextureData);

INCLUDE_ASM("asm/nonmatchings/layer", SetTexturePageParams);

INCLUDE_ASM("asm/nonmatchings/layer", InitCLUTColorLerpEffect);


