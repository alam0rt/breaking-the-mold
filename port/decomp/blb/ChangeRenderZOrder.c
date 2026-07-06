/* =============================================================================
 * ChangeRenderZOrder.c  --  PC-port render-list z-order move
 * =============================================================================
 * Functional-C body for ChangeRenderZOrder (INCLUDE_ASM in src/blb.c).
 * The render list at renderList+0x20 is a singly-linked list of 8-byte nodes
 * {next, spriteCtx} kept sorted by descending z (BasicEntity zOrder @+0x08).
 * Unlinks the sprite's existing node (if any), stamps the new z, and
 * re-inserts before the first node whose z <= newZ (or at the tail).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void *g_pBlbHeapBase;
extern u8 *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern void FreeFromHeap(void *heap, void *ptr, u32 size, u8 flag);

typedef struct ZNode {
    struct ZNode *next;
    u8 *spriteCtx;
} ZNode;

void ChangeRenderZOrder(void *renderList, void *node, s16 newZ) {
    ZNode **head = (ZNode **)((u8 *)renderList + 0x20);
    ZNode *prev = NULL;
    ZNode *p;
    ZNode *n;

    for (p = *head; p != NULL; p = p->next) {
        if (p->spriteCtx == (u8 *)node) {
            if (prev == NULL) {
                *head = p->next;
            } else {
                prev->next = p->next;
            }
            FreeFromHeap(g_pBlbHeapBase, p, 8, 0);
            break;
        }
        prev = p;
    }
    *(s16 *)((u8 *)node + 8) = newZ;

    if (*head == NULL) {
        n = (ZNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
        *head = n;
        n->next = NULL;
        n->spriteCtx = (u8 *)node;
        return;
    }
    prev = NULL;
    for (p = *head; p != NULL; p = p->next) {
        if (*(s16 *)(p->spriteCtx + 8) <= newZ) {
            n = (ZNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
            if (prev == NULL) {
                *head = n;
            } else {
                prev->next = n;
            }
            n->next = p;
            n->spriteCtx = (u8 *)node;
            return;
        }
        prev = p;
    }
    n = (ZNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
    prev->next = n;
    n->next = NULL;
    n->spriteCtx = (u8 *)node;
}
