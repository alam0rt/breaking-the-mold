/* =============================================================================
 * RemoveEntityFromUpdateQueue.c  --  PC-port unlink entity from update queue
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/RemoveEntityFromUpdateQueue.s
 * (0x80021E50, 0x8C). Same single-list unlink as RemoveFromTickList but on the
 * queue whose head is at gameState+0x24: finds the node whose payload (node+4)
 * equals `entity`, unlinks it, and frees the 8-byte node.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);

void RemoveEntityFromUpdateQueue(void *gameState, void *entity) {
    u8 *gs = (u8 *)gameState;
    u8 *node = *(u8 **)(gs + 0x24);
    u8 *prev = NULL;

    while (node != NULL) {
        if (*(void **)(node + 4) == entity) {
            if (prev == NULL) {
                *(u8 **)(gs + 0x24) = *(u8 **)node;
            } else {
                *(u8 **)prev = *(u8 **)node;
            }
            FreeFromHeap((u8 *)g_pBlbHeapBase, node, 8, 0);
            return;
        }
        prev = node;
        node = *(u8 **)node;
    }
}
