/* Standalone -m32 unit test for the BLB heap allocator (port/decomp/vram/heap_alloc.c).
 * Not part of the build; run manually:
 *   gcc_multi -m32 -I include -I port/include -include port/include/port_config.h \
 *     -fcommon -Wno-implicit port/decomp/vram/heap_alloc.c tools/test_heap.c -o /tmp/th && /tmp/th
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

extern void InitHeapFreeList(void *base);
extern u8  *AllocateFromHeap(u8 *base, s32 align, s32 size, s32 flags);
extern void FreeFromHeap(u8 *base, u8 *ptr, s32 size, s32 flags);

/* Mirror InitHeapConfig (vram.c): heapStart @ +0xA640, heapSize @ +0xA644. */
static void init_cfg(u8 *base, u8 *heap, u32 size) {
    *(u8 **)(base + 0xA640) = heap;
    *(u32 *)(base + 0xA644) = ((size >> 4) <= 0xFFFF) ? size : 0xFFFF0;
}

#define HEAP_HDR_TOTAL 0xA64C

int main(void) {
    u8 *base = calloc(1, 0x20000);
    u8 *heap = calloc(1, 0x100000);            /* 1 MB backing */
    u16 total_before, total_after;
    u8 *a, *b, *c;
    int fails = 0;

    init_cfg(base, heap, 0x100000);
    InitHeapFreeList(base);
    total_before = *(u16 *)(base + HEAP_HDR_TOTAL);
    /* InitHeapConfig caps heapSize at 0xFFFF0 (the count field is u16), so a
     * 1 MB heap reports 0xFFFF 16-byte units, not 0x10000. */
    printf("init: total free 16-byte units = 0x%X (expect 0xFFFF)\n", total_before);
    if (total_before != 0xFFFF) { printf("FAIL init total\n"); fails++; }

    /* Three allocations; each must be within the heap and non-overlapping. */
    a = AllocateFromHeap(base, 1, 0x100, 0);
    b = AllocateFromHeap(base, 1, 0x200, 0);
    c = AllocateFromHeap(base, 1, 0x080, 0);
    printf("alloc a=%p b=%p c=%p (heap=%p..%p)\n", (void*)a, (void*)b, (void*)c,
           (void*)heap, (void*)(heap + 0x100000));
    if (!a || !b || !c) { printf("FAIL alloc returned NULL\n"); fails++; }
    if (a && b && (a + 0x100 > b)) { printf("FAIL a/b overlap\n"); fails++; }
    if (b && c && (b + 0x200 > c)) { printf("FAIL b/c overlap\n"); fails++; }

    /* Write patterns; ensure no aliasing. */
    if (a && b && c) {
        memset(a, 0xAA, 0x100); memset(b, 0xBB, 0x200); memset(c, 0xCC, 0x80);
        if (a[0] != 0xAA || b[0] != 0xBB || c[0] != 0xCC) {
            printf("FAIL pattern aliasing\n"); fails++;
        }
    }

    /* Free all; total must return to the initial free count (full coalesce). */
    FreeFromHeap(base, c, 0, 0);
    FreeFromHeap(base, b, 0, 0);
    FreeFromHeap(base, a, 0, 0);
    total_after = *(u16 *)(base + HEAP_HDR_TOTAL);
    printf("after free-all: total free = 0x%X (expect 0x%X)\n",
           total_after, total_before);
    if (total_after != total_before) { printf("FAIL free did not restore total\n"); fails++; }

    /* Re-allocate the full original size to prove the heap coalesced back. */
    a = AllocateFromHeap(base, 1, 0x100, 0);
    if (!a) { printf("FAIL realloc after coalesce\n"); fails++; }
    else printf("realloc after coalesce ok: %p\n", (void*)a);

    printf(fails ? "\n=== %d FAILURE(S) ===\n" : "\n=== ALL PASS ===\n", fails);
    return fails ? 1 : 0;
}
