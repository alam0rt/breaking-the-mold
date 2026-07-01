#include "common.h"

typedef struct {
    u8 _pad0000[0xA640];
    void *heapStart;
    u32 heapSize;
} HeapConfigOwner;

INCLUDE_ASM("asm/nonmatchings/vram", AllocateVRAMSlot);

extern void FreeAndCoalesceVRAMSlot(s32 base, u32 packed, u8 sizeBlocks);

/* Releases a VRAM slot back to the coalescing free-list: converts the slot's
 * pixel TL corner + height to 8-/16-pixel block coords, packs them, dispatches
 * to FreeAndCoalesceVRAMSlot. xBlocks pinned $v0 and its packed-low mask pinned
 * $a3 so the andi reads $v0 / writes $a3 (target coloring; source-order alone
 * can't pick it). */
void FreeVRAMSlot(s16 *vramBase, u32 packedXY, u32 packedSize) {
    u16 blocks[6];
    s16 height = ((s16 *)&packedSize)[1];
    s32 hBlocks = (s32)height + 0xF;
    register s32 xb asm("$2");  /* $v0 */
    register u32 lo asm("$7");  /* $a3 */
    if (hBlocks < 0) {
        hBlocks = (s32)height + 0x1E;
    }
    xb = (((s16 *)&packedXY)[0] - *vramBase) >> 3;
    blocks[0] = (u16)xb;
    blocks[1] = (u16)((((s16 *)&packedXY)[1] - 0x100) >> 4);
    lo = (u16)xb;
    FreeAndCoalesceVRAMSlot((s32)vramBase, lo | ((u32)blocks[1] << 16),
                            (u8)((u32)hBlocks >> 4));
}

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

/* Scans the VRAM slot table (stride-6 entries, status byte at +0xA08F) for the
 * first index in [0, 0x58) whose status is 0xFE (free). Returns that index, or
 * 0 if none are free (the return reg v0 doubles as the found index and the
 * loop-exit condition).
 *
 * SHELVED: structurally-perfect goto-loop draft below reaches the exact 20
 * instructions and the right addressing idiom (inline lui+displacement for the
 * 0xA08F offset) and dual-use return, but two residual diffs remain — cc1 does
 * not hoist the 0xFE sentinel out of the (unrecognized) goto loop the way the
 * target does (li a2,254 pre-loop), and the counter/index coloring is a2/a1
 * instead of a1/v1. Recognized-loop forms (for/do-while) fix the hoist+coloring
 * but then LICM-hoist the 0xA08F offset into a register (li a3,0xa08f) and split
 * the shared return — net worse. Pure LICM+coloring residual → permuter job.
 * Closest draft:
 *   s32 FindFreeVRAMSlotEntry(s32 base) {
 *       s32 counter = 0, idx = 0, ret;
 *   loop:
 *       ret = idx;
 *       if (*(u8 *)(base + idx * 6 + 0xA08F) != 0xFE) {
 *           counter += 1;
 *           ret = (u32)(counter & 0xFF) < 0x58U;
 *           idx = counter & 0xFF;
 *           if (ret != 0) goto loop;
 *       }
 *       return ret;
 *   } */
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

