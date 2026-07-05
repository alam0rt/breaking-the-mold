/* =============================================================================
 * layer_render_list.c  --  PC-port layer tick/render list registration
 * =============================================================================
 * Functional-C reconstruction of the three sibling functions
 *   AddLayerToRenderList_Standard @ 0x80021590
 *   AddLayerToRenderList_Medium   @ 0x80021778
 *   AddLayerToRenderList_Small    @ 0x80021960
 * (asm in src/blb.c; reference = export/SLES_010.90.c). All three are
 * BEHAVIOURALLY IDENTICAL -- they register a layer render context into the two
 * shared, z-sorted engine lists:
 *   1. tick list  (GameState+0x1C): node payload = the layer context itself,
 *      inserted ascending by the s16 priority at context+0x10.
 *   2. render list (GameState+0x20): node payload = the layer's render
 *      sub-object (*(context+0x1C)), inserted by the s16 z at sub-object+0x08
 *      using the opposite comparison direction (insert-new before the first node
 *      whose z <= new z).
 * Each node is an 8-byte {next @ +0x0, payload @ +0x4} block from the BLB heap.
 * The three PSX functions are kept as separate byte-identical routines only for
 * the MIPS build; on PC one shared body + three wrappers is equivalent.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8 *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);

typedef struct LRLNode {
    struct LRLNode *next;    /* +0x0 */
    void           *payload; /* +0x4 */
} LRLNode;

/* Sorted insert of `payload` into the list at *head.
 * keyOff = byte offset of the s16 sort key within a node's payload.
 * newBeforeWhenLeq: 1 => insert new before the first node with newKey <= nodeKey
 *                        (ascending; tick list).
 *                   0 => insert new before the first node with nodeKey <= newKey
 *                        (render list). */
static void lrl_insert(void **head, void *payload, s32 keyOff, s32 newBeforeWhenLeq) {
    u8 *heap = (u8 *)g_pBlbHeapBase;
    s16 newKey = *(s16 *)((u8 *)payload + keyOff);
    LRLNode *cur = *(LRLNode **)head;
    LRLNode *prev = NULL;
    LRLNode *node;

    if (cur == NULL) {
        node = (LRLNode *)AllocateFromHeap(heap, 8, 1, 0);
        *(LRLNode **)head = node;
        node->next = NULL;
        node->payload = payload;
        return;
    }
    while (cur != NULL) {
        s16 nodeKey = *(s16 *)((u8 *)cur->payload + keyOff);
        s32 hit = newBeforeWhenLeq ? (newKey <= nodeKey) : (nodeKey <= newKey);
        if (hit) {
            node = (LRLNode *)AllocateFromHeap(heap, 8, 1, 0);
            if (prev == NULL) {
                *(LRLNode **)head = node;
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
    /* walked off the end: append after prev */
    node = (LRLNode *)AllocateFromHeap(heap, 8, 1, 0);
    prev->next = node;
    node->next = NULL;
    node->payload = payload;
}

static void addLayerToRenderList(void *gs, void *ctx) {
    u8 *g = (u8 *)gs;
    void *renderObj = *(void **)((u8 *)ctx + 0x1C);
    lrl_insert((void **)(g + 0x1C), ctx, 0x10, 1);        /* tick list   */
    lrl_insert((void **)(g + 0x20), renderObj, 0x08, 0);  /* render list */
}

void AddLayerToRenderList_Standard(void *gs, void *ctx) { addLayerToRenderList(gs, ctx); }
void AddLayerToRenderList_Medium(void *gs, void *ctx)   { addLayerToRenderList(gs, ctx); }
void AddLayerToRenderList_Small(void *gs, void *ctx)    { addLayerToRenderList(gs, ctx); }
