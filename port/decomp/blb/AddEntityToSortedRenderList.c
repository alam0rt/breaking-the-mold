/* =============================================================================
 * AddEntityToSortedRenderList.c  --  PC-port entity tick/render registration
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/AddEntityToSortedRenderList.s
 * (0x80021358, 144 lines; reference = export/SLES_010.90.c 17443). Registers an
 * entity into the two shared, z-sorted engine lists (each node = 8 bytes
 * {next @ +0x0, payload @ +0x4}, from the BLB heap):
 *   1. tick list (GameState+0x1C): payload = the entity, inserted ascending by
 *      the s16 allocSize at entity+0x10 (insert-new-before-first node whose
 *      payload.allocSize >= new.allocSize).
 *   2. render list (GameState+0x20): payload = entity->spriteContext
 *      (*(entity+0x34)), inserted by the s16 zOrder at payload+0x08 using the
 *      opposite direction (insert-new-before-first node whose payload.zOrder <=
 *      new.zOrder).
 * Sibling of AddLayerToRenderList_* (which sources the render payload from the
 * layer context +0x1C instead of the entity's +0x34 spriteContext).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8 *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);

typedef struct ERLNode {
    struct ERLNode *next;    /* +0x0 */
    void           *payload; /* +0x4 */
} ERLNode;

/* Sorted insert of `payload` into the list at *head, keyed by the s16 at
 * payload+keyOff. newBeforeWhenLeq: 1 => insert new before the first node with
 * newKey <= nodeKey (ascending); 0 => before the first node with nodeKey <=
 * newKey. */
static void erl_insert(void **head, void *payload, s32 keyOff, s32 newBeforeWhenLeq) {
    u8 *heap = (u8 *)g_pBlbHeapBase;
    s16 newKey = *(s16 *)((u8 *)payload + keyOff);
    ERLNode *cur = *(ERLNode **)head;
    ERLNode *prev = NULL;
    ERLNode *node;

    if (cur == NULL) {
        node = (ERLNode *)AllocateFromHeap(heap, 8, 1, 0);
        *(ERLNode **)head = node;
        node->next = NULL;
        node->payload = payload;
        return;
    }
    while (cur != NULL) {
        s16 nodeKey = *(s16 *)((u8 *)cur->payload + keyOff);
        s32 hit = newBeforeWhenLeq ? (newKey <= nodeKey) : (nodeKey <= newKey);
        if (hit) {
            node = (ERLNode *)AllocateFromHeap(heap, 8, 1, 0);
            if (prev == NULL) {
                *(ERLNode **)head = node;
            } else {
                prev->next = node;
            }
            node->next = cur;
            node->payload = payload;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
    node = (ERLNode *)AllocateFromHeap(heap, 8, 1, 0);
    prev->next = node;
    node->next = NULL;
    node->payload = payload;
}

void AddEntityToSortedRenderList(void *gameState, void *entity) {
    u8 *gs = (u8 *)gameState;
    void *spriteContext;
    /* PC bring-up: a stubbed constructor (e.g. CreateCameraEntity on level 2)
     * can hand us a NULL entity; skip rather than fault. Remove once the
     * constructor is converted. */
    if (entity == NULL) {
        return;
    }
    spriteContext = *(void **)((u8 *)entity + 0x34);
    erl_insert((void **)(gs + 0x1C), entity, 0x10, 1);           /* tick list   */
    erl_insert((void **)(gs + 0x20), spriteContext, 0x08, 0);    /* render list */
}
