#include "common.h"

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlot);

INCLUDE_ASM("asm/nonmatchings/vram", FreeVRAMSlot);

INCLUDE_ASM("asm/nonmatchings/vram", InitVRAMSlotTable);

INCLUDE_ASM("asm/nonmatchings/vram", AllocateCLUTSlot);

INCLUDE_ASM("asm/nonmatchings/vram", FreeCLUTSlot);

INCLUDE_ASM("asm/nonmatchings/vram", ProcessPendingVRAMSlotFrees);

INCLUDE_ASM("asm/nonmatchings/vram", CalculateVRAMCoordinates);

void InitHeapConfig(void *base, void *ptr, u32 size) {
    *(void **)((u8 *)base + 0xA640) = ptr;
    if ((size >> 4) <= 0xFFFF) {
        *(u32 *)((u8 *)base + 0xA644) = size;
    } else {
        *(u32 *)((u8 *)base + 0xA644) = 0xFFFF0;
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

