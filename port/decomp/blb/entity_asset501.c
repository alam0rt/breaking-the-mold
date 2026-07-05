/* =============================================================================
 * entity_asset501.c  --  PC-port asset-501 entity-list loader (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/LoadEntitiesFromAsset501.s
 * (0x80024DC8, 0x160 bytes). Rebuilds the level's pre-init entity list rooted at
 * GameState+0x28:
 *   1. Tear down any existing list: for each 8-byte node {next @ +0x0,
 *      item @ +0x4}, free the item (FreeFromHeap size 0 -- the block header
 *      carries the real size) then the node (size 8).
 *   2. For each of GetEntityCount() asset-501 entity records (0x18 bytes each at
 *      GetEntityDataPtr()), allocate a 0x18-byte copy, then an 8-byte list node
 *      pointing at it, prepended to the list (so the list ends up reversed).
 * Offsets/sizes transcribed from the disassembly (6 word copies = 0x18 bytes;
 * alloc align/size = {0x18,1} for records, {8,1} for nodes).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u16  GetEntityCount(void *ctx);
extern void *GetEntityDataPtr(void *ctx);
extern u8  *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);
extern void FreeFromHeap(u8 *heap, void *ptr, s32 size, s32 flags);

typedef struct Asset501Node {
    struct Asset501Node *next;   /* +0x0 */
    void                *item;   /* +0x4  -> 0x18-byte entity record copy */
} Asset501Node;

void LoadEntitiesFromAsset501(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *ctx = gs + 0x84;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u16 count;
    s32 i;

    /* ---- tear down the existing list ---- */
    while (*(Asset501Node **)(gs + 0x28) != NULL) {
        Asset501Node *node = *(Asset501Node **)(gs + 0x28);
        FreeFromHeap(heap, node->item, 0, 0);
        *(Asset501Node **)(gs + 0x28) = node->next;
        FreeFromHeap(heap, node, 8, 0);
    }

    /* ---- rebuild from asset 501 ---- */
    count = GetEntityCount(ctx);
    if ((count & 0xFFFF) != 0) {
        u8 *dataPtr = (u8 *)GetEntityDataPtr(ctx);
        for (i = 0; (u32)(i & 0xFFFF) < (u32)(count & 0xFFFF); i++) {
            u8 *src = dataPtr + (u32)(i & 0xFFFF) * 0x18;
            s32 *dst = (s32 *)AllocateFromHeap(heap, 0x18, 1, 0);
            Asset501Node *node;
            dst[0] = ((s32 *)src)[0];
            dst[1] = ((s32 *)src)[1];
            dst[2] = ((s32 *)src)[2];
            dst[3] = ((s32 *)src)[3];
            dst[4] = ((s32 *)src)[4];
            dst[5] = ((s32 *)src)[5];
            node = (Asset501Node *)AllocateFromHeap(heap, 8, 1, 0);
            node->item = dst;
            node->next = *(Asset501Node **)(gs + 0x28);
            *(Asset501Node **)(gs + 0x28) = node;
        }
    }
}
