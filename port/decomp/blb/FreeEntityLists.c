/* =============================================================================
 * FreeEntityLists.c  --  PC-port teardown of the game state's entity lists
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/FreeEntityLists.s
 * (0x800223BC, 0x114). Frees every node of the three intrusive lists on level
 * teardown: the render list (gs+0x20) and update queue (gs+0x24) are simple
 * node frees; the z-order list (gs+0x1C) additionally runs each live entity's
 * vtable destructor ((*(vt+0xC))(entity + (s16)vt[8], 3)) when the entity's
 * +0x14 alive flag is set, before freeing the 8-byte node.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);

void FreeEntityLists(void *gameState) {
    u8 *gs = (u8 *)gameState;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u8 *node;

    while ((node = *(u8 **)(gs + 0x20)) != NULL) {
        *(u8 **)(gs + 0x20) = *(u8 **)node;
        FreeFromHeap(heap, node, 8, 0);
    }
    while ((node = *(u8 **)(gs + 0x24)) != NULL) {
        *(u8 **)(gs + 0x24) = *(u8 **)node;
        FreeFromHeap(heap, node, 8, 0);
    }
    while ((node = *(u8 **)(gs + 0x1C)) != NULL) {
        u8 *entity = *(u8 **)(node + 4);
        *(u8 **)(gs + 0x1C) = *(u8 **)node;
        if (entity != NULL && *(u8 *)(entity + 0x14) != 0) {
            u8 *vt = *(u8 **)(entity + 0x18);
            void (*dtor)(void *, s32) = *(void (**)(void *, s32))(vt + 0xC);
            dtor(entity + *(s16 *)(vt + 8), 3);
        }
        FreeFromHeap(heap, node, 8, 0);
    }
}
