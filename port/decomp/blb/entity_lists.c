/* =============================================================================
 * entity_lists.c  --  functional-C entity-list teardown helpers (TARGET_PC)
 * =============================================================================
 * PC replacements for the small blb.c entity-list functions the level loader's
 * preamble calls, transcribed from m2c drafts of asm/nonmatchings/blb/*.s. Both
 * free BLB-heap allocations via the (already-converted) FreeFromHeap.
 *
 * Raw byte offsets keep these decoupled from the GameState struct header.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);

/* RemoveFromZOrderList @ blb.c: walk the per-instance Z-order pointer array
 * (count @ gs+0x114, base @ gs+0x110, stride 4), fire each entity's destruct
 * vtable (fn @ vtable+0xC, called with entity+vtable[0x8] and type 3), then free
 * the array and clear the count. */
void RemoveFromZOrderList(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *arr = *(u8 **)(gs + 0x110);
    s16 count = *(s16 *)(gs + 0x114);
    s16 i;

    if (*(s32 *)(gs + 0x114) != 0) {
        for (i = 0; i < count; i++) {
            u8 *ent = *(u8 **)(arr + i * 4);
            if (ent != NULL) {
                u8 *vtable = *(u8 **)(ent + 0x18);
                void (*destruct)(void *, s32) =
                    *(void (**)(void *, s32))(vtable + 0xC);
                destruct(ent + *(s16 *)(vtable + 0x8), 3);
            }
        }
    }
    if (*(u8 **)(gs + 0x110) != NULL) {
        FreeFromHeap((u8 *)g_pBlbHeapBase, *(u8 **)(gs + 0x110), 0, 0);
        *(u8 **)(gs + 0x110) = NULL;
        *(s32 *)(gs + 0x114) = 0;
    }
}

/* RemoveFromRenderList: migrated to matched C in src/blb.c */
