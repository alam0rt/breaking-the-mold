#include "common.h"

typedef struct {
    u8 _pad0000[0xA640];
    void *heapStart;
    u32 heapSize;
} HeapConfigOwner;

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlot);

INCLUDE_ASM("asm/nonmatchings/vram", FreeVRAMSlot);

INCLUDE_ASM("asm/nonmatchings/vram", InitVRAMSlotTable);

INCLUDE_ASM("asm/nonmatchings/vram", AllocateCLUTSlot);

INCLUDE_ASM("asm/nonmatchings/vram", FreeCLUTSlot);

INCLUDE_ASM("asm/nonmatchings/vram", ProcessPendingVRAMSlotFrees);

INCLUDE_ASM("asm/nonmatchings/vram", CalculateVRAMCoordinates);

void InitHeapConfig(HeapConfigOwner *base, u8 *ptr, u32 size) {
    base->heapStart = ptr;
    if ((size >> 4) <= 0xFFFF) {
        base->heapSize = size;
    } else {
        base->heapSize = 0xFFFF0;
    }
}

INCLUDE_ASM("asm/nonmatchings/vram", AllocateFromHeap);

INCLUDE_ASM("asm/nonmatchings/vram", FreeFromHeap);

INCLUDE_ASM("asm/nonmatchings/vram", InitHeapFreeList);

/* Fill the entire heap region with a 32-bit pattern. Uses the heap
 * start pointer + size from base+0xA640/0xA644 (HeapConfigOwner) to
 * compute the byte-addressed end pointer, then memset32's the region.
 *
 * SHELVED (60-byte diff = register swap): structure matches with body
 *     s32 *p = base->heapStart;
 *     s32 *end = (s32 *)((u8 *)p + base->heapSize);
 *     if (p != end) { do { *p++ = fillValue; } while (p != end); }
 * but cc1 picks $v1 for `p` (loop variable) and $v0 for `end`/intermediates
 * while TARGET swaps them. The choice is binary-different but otherwise
 * cycle-equivalent; no source-order or naming trick coaxes cc1 to swap. */
INCLUDE_ASM("asm/nonmatchings/vram", func_80014928);

INCLUDE_ASM("asm/nonmatchings/vram", ClearHeapBlocks);

INCLUDE_ASM("asm/nonmatchings/vram", InitVRAMSlotArray);

INCLUDE_ASM("asm/nonmatchings/vram", FindVRAMSlotBySize);

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlotAligned);

INCLUDE_ASM("asm/nonmatchings/vram", FindFreeVRAMSlotEntry);

INCLUDE_ASM("asm/nonmatchings/vram", GetMaxVRAMSlotSize);

INCLUDE_ASM("asm/nonmatchings/vram", FreeAndCoalesceVRAMSlot);

void StubEmpty(void) {
    s32 unused[3];
    (void)&unused;
}

