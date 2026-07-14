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

/* Copy a static sprite frame's texture parameters into the render primitive:
 * tpage (preserving the primitive's semi-transparency bits 5-6), U/V texture
 * coords, the CLUT id from the sprite asset +0x12, and the dirty flag. */
void SetupSpriteFromFrame(SpriteContext **ppSprite, u8 *rs, u16 frameIndex) {
    u8 *frame = (u8 *)(*ppSprite)->secondary_ptr + frameIndex * 0x14;
    u16 tpage = *(u16 *)(frame + 0xC);
    u8 u = frame[0xE];
    u8 v = frame[0xF];

    rs[0x2E] = 1;
    *(u16 *)(rs + 0x24) = (tpage & 0xFF9F) | (*(u16 *)(rs + 0x24) & 0x60);
    rs[0x30] = u;
    rs[0x31] = v;
    *(u16 *)(rs + 0x26) = *(u16 *)((u8 *)*ppSprite + 0x12);
}

extern u8 D_8009AE58[];
void FreeAllLayerRenderSlotsWrapper(void) {
    FreeAllLayerRenderSlots((LayerRenderSlot *)D_8009AE58);
}

/* CRT-init variant: unconditionally zero word 0 of all 20 layer-render slots
 * (base @ D_8009AE58). Stores offset 0 directly (NOT ->entity at +4). */
void ClearAllLayerRenderSlots_CrtInit(void) {
    LayerRenderSlot *base = (LayerRenderSlot *)D_8009AE58;
    s16 i = 0;
    do {
        LayerRenderSlot *slot = &base[i];
        *(s32 *)slot = 0;
        i++;
    } while (i < 0x14);
}

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

/* PSY-Q RECT (libgpu): 8-byte rectangle for VRAM transfers (same local
 * definition pattern as effects.c). */
typedef struct TextureUploadRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} TextureUploadRect;

extern void LoadImage(TextureUploadRect *r, u_long *p);
extern void LoadClut(u_long *p, s32 x, s32 y);

/* Upload a sprite render context's data to VRAM: type 0 (+0x34) is a texture
 * (16x1 LoadImage at the context's VRAM coords +0x28/+0x2A), type 1 a CLUT. */
void UploadTextureOrClut(u8 *c, u_long *data) {
    u8 type = c[0x34];

    if (type == 0) {
        TextureUploadRect r;
        r.x = *(u16 *)(c + 0x28);
        r.y = *(u16 *)(c + 0x2A);
        r.w = 0x10;
        r.h = 1;
        LoadImage(&r, data);
    } else if (type == 1) {
        LoadClut(data, *(s16 *)(c + 0x28), *(s16 *)(c + 0x2A));
    }
}

INCLUDE_ASM("asm/nonmatchings/layer", CopyTextureData);

extern void CLUTPaletteCycleTickCallback();

/* Install the palette-animation cycle callback + params on a sprite render
 * context (only if it has a CLUT backup buffer at +0x1C). */
void SetTexturePageParams(void *ctx, u8 stepDelay, u8 startIdx, u8 count, u8 mode) {
    PadSlot slot;
    s16 m1;
    register void (*fn)() PSX_REG("$3");
    u8 *c = ctx;

    if (*(s32 *)(c + 0x1C) != 0) {
        m1 = -1;
        /* fence: keep the callback-address lui below the param stores */
        do {
            c[0x35] = startIdx;
            c[0x36] = count;
            c[0x37] = mode;
            c[0x38] = stepDelay;
            c[0x39] = stepDelay;
        } while (0);
        fn = CLUTPaletteCycleTickCallback;
        /* Pin fn to $v1 so cc1 keeps $v0 alive for m1=-1 and emits
         * `li v0,-1` in the beqz delay slot. */
        __asm__ volatile("" : "=r"(fn) : "0"(fn));
        slot.s.markerLo = 0;
        slot.s.markerHi = m1;
        slot.s.fn = fn;
        *(CallbackSlot *)c = slot.s;
    }
}

INCLUDE_ASM("asm/nonmatchings/layer", InitCLUTColorLerpEffect);


