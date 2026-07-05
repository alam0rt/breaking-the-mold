/* =============================================================================
 * heap_alloc.c  --  functional-C BLB heap block allocator (TARGET_PC)
 * =============================================================================
 * PC replacements for the vram.c BLB-heap functions (InitHeapFreeList,
 * ClearHeapBlocks; AllocateFromHeap/FreeFromHeap live alongside), transcribed
 * from m2c drafts of asm/nonmatchings/vram/*.s. This is the block-descriptor
 * allocator the level loader carves asset buffers from.
 *
 * ---- HEAP FREE-LIST LAYOUT (verified from InitHeapFreeList disassembly) -----
 * A pool of 100 block descriptors starts at base+0xA320, stride 8:
 *     struct HeapBlock { s32 ptr; u16 size16; u16 next; }   // size in 16-byte units
 * block[i] is at base + 0xA320 + i*8; block[i].next at +0xA326 + i*8.
 * Header fields:
 *     base+0xA640 void* heapStart   (set by InitHeapConfig, matched C)
 *     base+0xA644 u32   heapSize     (bytes)
 *     base+0xA648 u16   headIndex    (allocated-block list head; 0 at init)
 *     base+0xA64A u16   (=1 at init)
 *     base+0xA64C u16   totalBlocks  (heapSize >> 4)
 *     base+0xA64E u16   (=totalBlocks at init)
 * block[0] describes the whole heap; blocks 1..99 are the free-descriptor pool
 * chained 1->2->...->99->0xFFFF. block[0].next = 0xFFFF (terminator).
 * ========================================================================== */
#include "common.h"

#define HEAP_BLOCKS_OFF 0xA320
#define HEAP_HDR_START  0xA640
#define HEAP_HDR_SIZE   0xA644
#define HEAP_HDR_HEAD   0xA648
#define HEAP_HDR_A64A   0xA64A
#define HEAP_HDR_TOTAL  0xA64C
#define HEAP_HDR_A64E   0xA64E

typedef struct HeapBlock {
    /* 0x0 */ s32 ptr;
    /* 0x4 */ u16 size16;
    /* 0x6 */ u16 next;
} HeapBlock;

static HeapBlock *heap_blocks(void *base) {
    return (HeapBlock *)((u8 *)base + HEAP_BLOCKS_OFF);
}

/* InitHeapFreeList @ vram.c: build the 100-entry free-descriptor pool over the
 * heap region set up by InitHeapConfig (heapStart/heapSize at +0xA640/+0xA644). */
void InitHeapFreeList(void *base) {
    u8 *b = (u8 *)base;
    HeapBlock *blk = heap_blocks(base);
    u16 total = (u16)(*(u32 *)(b + HEAP_HDR_SIZE) >> 4);
    s16 i;

    blk[0].next = 0xFFFF;
    *(u16 *)(b + HEAP_HDR_TOTAL) = total;
    *(u16 *)(b + HEAP_HDR_HEAD)  = 0;
    *(u16 *)(b + HEAP_HDR_A64A)  = 1;
    *(u16 *)(b + HEAP_HDR_A64E)  = total;
    blk[0].ptr    = *(s32 *)(b + HEAP_HDR_START);
    blk[0].size16 = *(u16 *)(b + HEAP_HDR_TOTAL);

    for (i = 1; (u16)i < 0x63; i++) {
        blk[i].next = (u16)(i + 1);
    }
    blk[99].next = 0xFFFF;
}

/* ClearHeapBlocks is provided as real matched-C in src/vram.c (globbed into the
 * port build), so the port copy is intentionally omitted to avoid a
 * multiple-definition link error. src/vram.c's version is behaviourally
 * identical (same base+0xA648 alloc-list walk, same base+0xA320 block layout,
 * memset32 per block). If you re-add anything here, do NOT redefine it. */

/* AllocateFromHeap @ vram.c: first-fit allocate `align*size` bytes from the free
 * list. Each allocation reserves a 4-byte size header at ptr[0] (holding the
 * block's 16-byte-unit count) and returns ptr+4 as the payload. `flags&1` skips
 * writing the header (raw mode). Descriptors that become empty are returned to
 * the free-descriptor pool. Byte arithmetic verified against the disassembly
 * (payload = ptr+4, advance = need*16). */
u8 *AllocateFromHeap(u8 *base, s32 align, s32 size, s32 flags) {
    u8 *b = base;
    HeapBlock *blk = heap_blocks(base);
    u16 prev = 0xFFFF;
    u16 cur  = *(u16 *)(b + HEAP_HDR_HEAD);
    u32 need = (u32)((align * size) + 0x13) >> 4;   /* 16-byte units incl header */

    while (cur != 0xFFFF) {
        HeapBlock *bc = &blk[cur];
        if ((u16)bc->size16 >= need) {
            u8 *oldptr = (u8 *)bc->ptr;
            u16 fc;
            if (!(flags & 0xFF)) {
                *(u32 *)oldptr = need;
            }
            bc->ptr    = (s32)(oldptr + need * 0x10);
            bc->size16 = (u16)(bc->size16 - need);
            fc = (u16)(*(u16 *)(b + HEAP_HDR_TOTAL) - need);
            *(u16 *)(b + HEAP_HDR_TOTAL) = fc;
            if (fc < *(u16 *)(b + HEAP_HDR_A64E)) {
                *(u16 *)(b + HEAP_HDR_A64E) = fc;
            }
            if (bc->size16 == 0) {
                /* Unlink empty descriptor, return it to the free-desc pool. */
                if (prev == 0xFFFF) {
                    *(u16 *)(b + HEAP_HDR_HEAD) = bc->next;
                } else {
                    blk[prev].next = bc->next;
                }
                bc->next = *(u16 *)(b + HEAP_HDR_A64A);
                *(u16 *)(b + HEAP_HDR_A64A) = cur;
            }
            return oldptr + 4;
        }
        prev = cur;
        cur = bc->next;
    }
    return NULL;
}

/* FreeFromHeap @ vram.c: return a block to the free list, coalescing with an
 * address-adjacent predecessor and/or successor. The free list is kept sorted
 * by address ascending. `flags&1` recomputes the size from `size` (raw mode);
 * otherwise it reads the 4-byte header at ptr-4. Transcribed from the m2c draft
 * with corrected byte arithmetic. */
void FreeFromHeap(u8 *base, u8 *ptr, s32 size, s32 flags) {
    u8 *b = base;
    HeapBlock *blk = heap_blocks(base);
    u8 *header = ptr - 4;
    u32 units  = (flags & 0xFF) ? ((u32)(size + 0x13) >> 4) : *(u32 *)header;
    u8 *blockEnd = header + (units & 0xFFFF) * 0x10;
    u16 prev = 0xFFFF;
    u16 cur;

    *(u32 *)header = 0;
    cur = *(u16 *)(b + HEAP_HDR_HEAD);

    while (cur != 0xFFFF && (u8 *)blk[cur].ptr <= blockEnd) {
        HeapBlock *bc = &blk[cur];
        if (header == (u8 *)bc->ptr + bc->size16 * 0x10) {
            /* Freed block sits immediately AFTER block[cur]: extend it. */
            bc->size16 = (u16)(bc->size16 + units);
            *(u16 *)(b + HEAP_HDR_TOTAL) =
                (u16)(*(u16 *)(b + HEAP_HDR_TOTAL) + units);
            {
                u16 nxt = bc->next;
                if (nxt != 0xFFFF && (u8 *)blk[nxt].ptr == blockEnd) {
                    /* ...and is now adjacent to the successor: merge it in. */
                    bc->size16 = (u16)(bc->size16 + blk[nxt].size16);
                    bc->next = blk[nxt].next;
                    blk[nxt].next = *(u16 *)(b + HEAP_HDR_A64A);
                    *(u16 *)(b + HEAP_HDR_A64A) = nxt;
                }
            }
            return;
        }
        prev = cur;
        if (blockEnd != (u8 *)bc->ptr) {
            cur = bc->next;
            continue;
        }
        /* Freed block sits immediately BEFORE block[cur]: prepend to it. */
        bc->ptr = (s32)header;
        bc->size16 = (u16)(bc->size16 + units);
        *(u16 *)(b + HEAP_HDR_TOTAL) =
            (u16)(*(u16 *)(b + HEAP_HDR_TOTAL) + units);
        return;
    }

    /* No adjacency: splice in a fresh free descriptor at the sorted position. */
    {
        u16 nd = *(u16 *)(b + HEAP_HDR_A64A);
        if (nd != 0xFFFF) {
            *(u16 *)(b + HEAP_HDR_A64A) = blk[nd].next;
            blk[nd].ptr = (s32)header;
            blk[nd].size16 = (u16)units;
            *(u16 *)(b + HEAP_HDR_TOTAL) =
                (u16)(*(u16 *)(b + HEAP_HDR_TOTAL) + units);
            if (prev != 0xFFFF) {
                blk[nd].next = blk[prev].next;
                blk[prev].next = nd;
            } else {
                blk[nd].next = *(u16 *)(b + HEAP_HDR_HEAD);
                *(u16 *)(b + HEAP_HDR_HEAD) = nd;
            }
        }
    }
}

