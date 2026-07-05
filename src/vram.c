#include "common.h"
#include "Game/vram_records.h"

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
    register s32 xb PSX_REG("$2");  /* $v0 */
    register u32 lo PSX_REG("$7");  /* $a3 */
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
    register s32 *p PSX_REG("$2");   /* $v0 — loop pointer */
    register s32 *end PSX_REG("$3"); /* $v1 */
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
 * 0 if none are free (ret doubles as the found index and the loop-exit
 * condition).
 *
 * Shape is load-bearing: `idx = counter & 0xFF` must be the first statement
 * AFTER the goto label (the label blocks cse from constant-folding the andi
 * off the known-zero counter; the bnez delay-slot andi is then cc1's
 * fill-from-target with branch retargeting). `ret = idx` must live on the
 * found branch of an if/else (not before the if) so it lands in the beq
 * delay slot and $v1 dies before the bnez. The named `entry` temp yields the
 * base-first `addu v0,a0,v0` operand order (same idiom as
 * GetMaxVRAMSlotSize / FindVRAMSlotBySize). */
s32 FindFreeVRAMSlotEntry(s32 base) {
    s32 counter;
    s32 sentinel;
    s32 idx;
    s32 ret;
    s32 entry;

    counter = 0;
    sentinel = 0xFE;
loop:
    idx = counter & 0xFF;
    entry = base + idx * 6;
    if (*(u8 *)(entry + 0xA08F) == sentinel) {
        ret = idx;
    } else {
        counter++;
        ret = (u32)(counter & 0xFF) < 0x58U;
        if (ret != 0) {
            goto loop;
        }
    }
    return ret;
}

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


/* .data migration (Phase 4): pooled-blob island carved to this TU's .o(.data). */
u8 D_8009AE40[24] asm("D_8009AE40") = {
    0x07, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0xF0, 0x01, 0x00, 0x00,
};
