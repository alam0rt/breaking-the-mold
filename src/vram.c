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

INCLUDE_ASM("asm/nonmatchings/vram", func_80014928);

INCLUDE_ASM("asm/nonmatchings/vram", ClearHeapBlocks);

INCLUDE_ASM("asm/nonmatchings/vram", InitVRAMSlotArray);

INCLUDE_ASM("asm/nonmatchings/vram", FindVRAMSlotBySize);

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlotAligned);

INCLUDE_ASM("asm/nonmatchings/vram", FindFreeVRAMSlotEntry);

INCLUDE_ASM("asm/nonmatchings/vram", GetMaxVRAMSlotSize);

INCLUDE_ASM("asm/nonmatchings/vram", FreeAndCoalesceVRAMSlot);

INCLUDE_ASM("asm/nonmatchings/vram", StubEmpty);

