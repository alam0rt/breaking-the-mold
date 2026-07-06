/* =============================================================================
 * AddToUpdateQueue.c  --  PC-port update/collision-list sorted insert
 * =============================================================================
 * Functional-C bodies for AddToUpdateQueue and CheckBoxCollision (both
 * INCLUDE_ASM in src/blb.c). The update list at gameState+0x24 is a
 * singly-linked list of 8-byte nodes {next, entity} kept sorted ascending by
 * the entity's allocSize (+0x10) -- the same node shape as the render list.
 * CheckBoxCollision queries it: mask 2 is the fast player-only test against
 * the player entity cached at gameState+0x2C; any other mask walks the list
 * matching collisionMask (+0x12) and testing box overlap.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"

extern void *g_pBlbHeapBase;
extern u8 *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);

typedef struct QNode {
    struct QNode *next;
    u8 *entity;
} QNode;

void AddToUpdateQueue(void *gameState, void *entity) {
    QNode **head = (QNode **)((u8 *)gameState + 0x24);
    QNode *prev = NULL;
    QNode *p;
    QNode *n;

    if (*head == NULL) {
        n = (QNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
        *head = n;
        n->next = NULL;
        n->entity = (u8 *)entity;
        return;
    }
    for (p = *head; p != NULL; p = p->next) {
        if (*(s16 *)((u8 *)entity + 0x10) <= *(s16 *)(p->entity + 0x10)) {
            n = (QNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
            if (prev == NULL) {
                *head = n;
            } else {
                prev->next = n;
            }
            n->next = p;
            n->entity = (u8 *)entity;
            return;
        }
        prev = p;
    }
    n = (QNode *)AllocateFromHeap(g_pBlbHeapBase, 8, 1, 0);
    prev->next = n;
    n->next = NULL;
    n->entity = (u8 *)entity;
}

typedef struct { s16 x; s16 y; } PortBoxCorner;
extern s32 CheckBoxOverlap();   /* real C, src/entity.c (by-value corner pair) */

u32 CheckBoxCollision(void *gameState, u32 boxX1Y1, u32 boxX2Y2, u16 typeMask) {
    u8 *gs = (u8 *)gameState;
    QNode *p;

    if (typeMask == 2 && *(u8 **)(gs + 0x2C) != NULL) {
        return CheckBoxOverlap(*(u8 **)(gs + 0x2C), boxX1Y1, boxX2Y2) ? 1u : 0u;
    }
    for (p = *(QNode **)(gs + 0x24); p != NULL; p = p->next) {
        if ((typeMask & *(u16 *)(p->entity + 0x12)) != 0 &&
            CheckBoxOverlap(p->entity, boxX1Y1, boxX2Y2)) {
            return 1;
        }
    }
    return 0;
}
