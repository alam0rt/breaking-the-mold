#include "common.h"

typedef struct {
    u8 _pad0000[0xA640];
    void *heapStart;
    u32 heapSize;
} HeapConfigOwner;

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlot);

extern void FreeAndCoalesceVRAMSlot(s32 base, u32 packed, u8 sizeBlocks);

/* SHELVED: 10-byte diff — pure cc1 register-allocation choice for the
 * `xBlocks & 0xFFFF` mask:
 *   TARGET:  andi a3,v0,0xffff  →  or a1,a3,a1   (preserves v0, masks into a3)
 *   CURRENT: andi v0,v0,0xffff  →  or a1,v0,a1   (clobbers v0)
 *
 * No source-level expression (bitmask, (u16) cast, lhu reload, u16/u32
 * locals, explicit pack-via-tmp) has been found that nudges cc1 to pick
 * a3 over v0 for this mask. Everything else matches: frame 0x28, caller-
 * arg-save spills at sp+0x2C/0x30, dual sh barriers at sp+0x10/0x12.
 *
 * Releases a VRAM slot back to the coalescing free-list. Inputs:
 *   vramBase    - blbHeaderBufferBase (slot-table base; *vramBase is the
 *                 x-origin used to convert pixel-x into block coordinates).
 *   packedXY    - packed (x | y<<16) pixel position of the slot's TL corner.
 *   packedSize  - low half unused; HIGH half = height in pixels (signed).
 *
 * Converts pixel coords to block coords:
 *   xBlocks = (x - *vramBase) >> 3        (8-pixel blocks, signed shift)
 *   yBlocks = (y - 0x100)     >> 4        (16-pixel blocks, signed shift)
 *   hBlocks = (height + 15)   >> 4        (16-pixel block height, ceil-div
 *                                          with cc1's signed-shift dance)
 *
 * Then dispatches to FreeAndCoalesceVRAMSlot which actually merges the
 * freed region with adjacent free blocks in the slot table.
 *
 * Equivalent C (kept here for documentation - shelved to ASM include):
 *   void FreeVRAMSlot(s16 *vramBase, u32 packedXY, u32 packedSize) {
 *       u16 blocks[6];
 *       s16 height = ((s16 *)&packedSize)[1];
 *       s32 hBlocks = (s32)height + 0xF;
 *       if (hBlocks < 0) hBlocks = (s32)height + 0x1E;
 *       blocks[0] = (u16)((((s16 *)&packedXY)[0] - *vramBase) >> 3);
 *       blocks[1] = (u16)((((s16 *)&packedXY)[1] - 0x100) >> 4);
 *       FreeAndCoalesceVRAMSlot((s32)vramBase,
 *                               (u32)blocks[0] | ((u32)blocks[1] << 16),
 *                               (u8)((u32)hBlocks >> 4));
 *   } */
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
 * p/end pinned to $v0/$v1 to match the target's loop-var coloring (cc1
 * otherwise swaps them; no source-order trick works, but the pin does). */
void func_80014928(HeapConfigOwner *base, s32 fillValue) {
    register s32 *p asm("$2");   /* $v0 — loop pointer */
    register s32 *end asm("$3"); /* $v1 */
    p = base->heapStart;
    end = (s32 *)((u8 *)p + base->heapSize);
    if (p != end) {
        do {
            *p = fillValue;
            p += 1;
        } while (p != end);
    }
}

INCLUDE_ASM("asm/nonmatchings/vram", ClearHeapBlocks);

INCLUDE_ASM("asm/nonmatchings/vram", InitVRAMSlotArray);

INCLUDE_ASM("asm/nonmatchings/vram", FindVRAMSlotBySize);

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlotAligned);

INCLUDE_ASM("asm/nonmatchings/vram", FindFreeVRAMSlotEntry);

/* Walks the VRAM slot linked-list (head at base+0xA29C, indices are
 * byte values, next-pointer is the byte at +0xA08F of each entry,
 * size byte is at +0xA090). Returns the maximum slot size seen along
 * the chain. Terminator is 0xFF. */
u8 GetMaxVRAMSlotSize(s32 base) {
    u8 maxSize;
    u8 idx;
    s32 entry;

    maxSize = 0;
    idx = *(u8 *)(base + 0xA29C);
    while (idx != 0xFF) {
        entry = base + (u32)idx * 6;
        idx = *(u8 *)(entry + 0xA090);
        if (maxSize < idx) {
            maxSize = idx;
        }
        idx = *(u8 *)(entry + 0xA08F);
    }
    return maxSize;
}

INCLUDE_ASM("asm/nonmatchings/vram", FreeAndCoalesceVRAMSlot);

void StubEmpty(void) {
    s32 unused[3];
    (void)&unused;
}

