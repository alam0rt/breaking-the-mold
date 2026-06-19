#include "common.h"
#include "functions.h"
#include "globals.h"

typedef struct BLBEntityHeader {
    u8 pad[0x18];
    void *vtable;
} BLBEntityHeader;

extern u8 BLB_ENTITY_DESTROYED_VTABLE[] asm("D_80012758");

void FreeBLBMemory(BLBEntityHeader *ptr, s32 size);

/*
 * BLBEntityDestroyCallback_2758 @ 0x80082EF0.
 * Destroy callback that sits in the rodata vtable D_80012758 (slot +0xC,
 * the "destroyed entity" vtable). Re-pins entity->vtable to D_80012758 so
 * subsequent dispatches stay no-ops, then — only when caller passes
 * flags bit 0 — frees the 0x1C-byte entity back to the BLB heap.
 */
void BLBEntityDestroyCallback_2758(BLBEntityHeader *entity, u32 flags) {
    entity->vtable = BLB_ENTITY_DESTROYED_VTABLE;
    if (flags & 1) {
        FreeBLBMemory(entity, 0x1C);
    }
}

/*
 * FreeBLBMemory @ 0x80082F24.
 * Thin wrapper that returns `ptr` to the global BLB heap (g_pBlbHeapBase).
 * The `size` parameter is unused — FreeFromHeap is invoked with 0/0 for
 * the trailing args. Name likely should be FreeFromBlbHeap.
 */
void FreeBLBMemory(BLBEntityHeader *ptr, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, ptr, 0, 0);
}
