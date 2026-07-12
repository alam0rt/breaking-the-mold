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

/* ClearEntityDefList @ blb.c: drain the singly-linked entity-def list at gs+0x28
 * (node = {next @ +0x0, data @ +0x4}, 8 bytes), freeing each node's data then the
 * node itself. */
void ClearEntityDefList(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *node = *(u8 **)(gs + 0x28);

    while (node != NULL) {
        FreeFromHeap((u8 *)g_pBlbHeapBase, *(u8 **)(node + 0x4), 0, 0);
        *(u8 **)(gs + 0x28) = *(u8 **)(node + 0x0);
        FreeFromHeap((u8 *)g_pBlbHeapBase, node, 8, 0);
        node = *(u8 **)(gs + 0x28);
    }
}

/* RemoveFromRenderList @ blb.c (0x80021DC0): unlink the first render-list node
 * (gs+0x20; node = {next @ +0x0, renderObj @ +0x4}) whose renderObj matches,
 * free the 8-byte node, return 1. Returns 0 when not found. Without this the
 * render list keeps dispatching freed objects whose memory has been recycled
 * (observed: pickup frees its HUD icon, CreateHaloEntity's icon reuses the
 * block, the stale node jumps through overwritten prim bytes). */
s32 RemoveFromRenderList(void *arg0, void *obj) {
    u8 *gs = (u8 *)arg0;
    u8 *node = *(u8 **)(gs + 0x20);
    u8 *prev = NULL;

    while (node != NULL) {
        if (*(u8 **)(node + 0x4) == (u8 *)obj) {
            if (prev == NULL) {
                *(u8 **)(gs + 0x20) = *(u8 **)(node + 0x0);
            } else {
                *(u8 **)(prev + 0x0) = *(u8 **)(node + 0x0);
            }
            FreeFromHeap((u8 *)g_pBlbHeapBase, node, 8, 0);
            return 1;
        }
        prev = node;
        node = *(u8 **)(node + 0x0);
    }
    return 0;
}
