/* =============================================================================
 * AddToXPositionList.c  --  functional-C body for entity.c AddToXPositionList
 * (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/entity/AddToXPositionList.s (0x8002107C,
 * 0x114). Inserts `item` into the singly-linked position-sorted list at
 * owner+0x20, ordered descending by the s32 at item+0x8 (insert before the
 * first node whose payload's +0x8 is <= the new item's). Nodes are 8-byte
 * {next, payload} pairs from the BLB heap.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void *AllocateFromHeap(void *heap, s32 size, s32 a2, s32 a3);

void AddToXPositionList(void *arg0, void *arg1) {
    u8 *owner = (u8 *)arg0;
    u8 *item = (u8 *)arg1;
    u8 *node = *(u8 **)(owner + 0x20);
    u8 *prev = NULL;
    u8 *n;

    if (node == NULL) {
        n = AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
        *(u8 **)(owner + 0x20) = n;
        *(u8 **)(n + 0) = NULL;
        *(u8 **)(n + 4) = item;
        return;
    }

    while (node != NULL) {
        u8 *payload = *(u8 **)(node + 4);
        if (*(s32 *)(item + 0x8) >= *(s32 *)(payload + 0x8)) {
            n = AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
            if (prev == NULL) {
                *(u8 **)(owner + 0x20) = n;
            } else {
                *(u8 **)(prev + 0) = n;
            }
            *(u8 **)(n + 0) = node;
            *(u8 **)(n + 4) = item;
            return;
        }
        prev = node;
        node = *(u8 **)(node + 0);
    }

    n = AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
    *(u8 **)(prev + 0) = n;
    *(u8 **)(n + 0) = NULL;
    *(u8 **)(n + 4) = item;
}
