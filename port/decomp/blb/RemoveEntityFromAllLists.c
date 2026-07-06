/* =============================================================================
 * RemoveEntityFromAllLists.c  --  PC-port entity list unlink + destruct
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/RemoveEntityFromAllLists.s
 * (0x80022074, 0x1A4). Unlinks an entity from the game state's three intrusive
 * lists -- tick list (gs+0x24, node payload = entity), render list (gs+0x20,
 * node payload = entity->renderCtx @ +0x34), z-order list (gs+0x1C) -- freeing
 * each matched 8-byte node {next@0, payload@4}, then invokes the entity's
 * vtable destructor (*(vt+0xC))(entity + *(s16 *)(vt+8), 3).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);

/* Unlink the first node whose payload matches, freeing it. Head is at
 * *(gs + headOff). */
static void unlink_node(u8 *gs, s32 headOff, void *payload) {
    u8 *node = *(u8 **)(gs + headOff);
    u8 *prev = NULL;

    while (node != NULL) {
        if (*(void **)(node + 4) == payload) {
            if (prev == NULL) {
                *(u8 **)(gs + headOff) = *(u8 **)node;
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

void RemoveEntityFromAllLists(void *gameState, void *entity) {
    u8 *gs = (u8 *)gameState;
    u8 *e = (u8 *)entity;

    unlink_node(gs, 0x24, e);                       /* tick list   */
    unlink_node(gs, 0x20, *(void **)(e + 0x34));    /* render list */
    unlink_node(gs, 0x1C, e);                       /* z-order     */

    if (e != NULL) {
        u8 *vt = *(u8 **)(e + 0x18);
        void (*dtor)(void *, s32) = *(void (**)(void *, s32))(vt + 0xC);
        dtor(e + *(s16 *)(vt + 8), 3);
    }
}
