/* =============================================================================
 * RemoveFromTickList.c  --  PC-port unlink entity from the render/tick list
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/RemoveFromTickList.s
 * (0x80021D30, 0x90). Walks the intrusive list whose head is at gameState+0x1C
 * looking for the node whose payload (node+4) equals `entity`; on a match it
 * unlinks and frees the 8-byte node, returning 1. Returns 0 if not found.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);

s32 RemoveFromTickList(void *gameState, void *entity) {
    u8 *gs = (u8 *)gameState;
    u8 *node = *(u8 **)(gs + 0x1C);
    u8 *prev = NULL;

    while (node != NULL) {
        if (*(void **)(node + 4) == entity) {
            if (prev == NULL) {
                *(u8 **)(gs + 0x1C) = *(u8 **)node;
            } else {
                *(u8 **)prev = *(u8 **)node;
            }
            FreeFromHeap((u8 *)g_pBlbHeapBase, node, 8, 0);
            return 1;
        }
        prev = node;
        node = *(u8 **)node;
    }
    return 0;
}
