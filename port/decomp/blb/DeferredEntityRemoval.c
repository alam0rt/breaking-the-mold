/* =============================================================================
 * DeferredEntityRemoval.c  --  PC-port single deferred-entity disposer
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c DeferredEntityRemoval
 * (0x80020C74, INCLUDE_ASM in src/blb.c). Called after each entity tick: if the
 * game state has a pending-removal entity (GameState+0x34), unlink it from the
 * active lists and, in the full-teardown mode, dispatch its destructor vtable
 * callback (vtable+0xC with (entity + vtable[8], 3)). Mode (GameState+0x38): 0 =
 * remove from all lists; !=0 = staged removal (update queue, then render list
 * unless mode==1, then tick list + destructor). Both slots are cleared after.
 *
 * With no entity queued for removal (the idle title screen) this returns
 * immediately, so the removal helpers are only exercised once something dies.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

typedef void (*DestructFn)(void *base, int mode);

extern void RemoveEntityFromAllLists(int gameState, int entity);
extern void RemoveEntityFromUpdateQueue(int gameState, int entity);
extern void RemoveFromRenderList(void *gameState, int arg);
extern void RemoveFromTickList(void *gameState, void *entity);

void DeferredEntityRemoval(void *gameStatePtr) {
    u8 *gs = (u8 *)gameStatePtr;
    s32 entity = *(s32 *)(gs + 0x34);

    if (entity != 0) {
        if (*(s32 *)(gs + 0x38) == 0) {
            RemoveEntityFromAllLists((int)(intptr_t)gs, entity);
            *(s32 *)(gs + 0x34) = 0;
        } else {
            RemoveEntityFromUpdateQueue((int)(intptr_t)gs, entity);
            if (*(s32 *)(gs + 0x38) != 1) {
                RemoveFromRenderList(gs, *(s32 *)(gs + 0x38));
            }
            RemoveFromTickList(gs, *(void **)(gs + 0x34));
            entity = *(s32 *)(gs + 0x34);
            if (entity != 0) {
                s32 vtable = *(s32 *)((intptr_t)entity + 0x18);
                DestructFn fn = *(DestructFn *)((intptr_t)vtable + 0xc);
                fn((u8 *)(intptr_t)entity + *(s16 *)((intptr_t)vtable + 8), 3);
            }
            *(s32 *)(gs + 0x34) = 0;
        }
        *(s32 *)(gs + 0x38) = 0;
    }
}
