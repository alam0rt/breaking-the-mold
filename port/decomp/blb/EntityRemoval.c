/* =============================================================================
 * EntityRemoval.c  --  PC-port per-frame post-render entity pass
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c EntityRemoval (0x80020D28,
 * INCLUDE_ASM in src/blb.c). Despite the (clean-room, guessed) name, this is
 * the frame loop's POST-RENDER pass: main dispatches the GameState vtable's
 * +0x1C slot after RenderEntities/DrawSync, and the GameState vtable
 * (D_80012100) routes it here. It walks the tick list dispatching each
 * entity's vtable +0x1C slot -- for standard sprite entities (D_800103AC)
 * that is UploadEntityTextureIfDirty, i.e. this pass is what flushes dirty
 * textures/CLUTs to VRAM every frame. After each dispatch it drains the
 * deferred-removal slot exactly like the tick loop does.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

typedef void (*SlotFn)(void *base);

extern void DeferredEntityRemoval(void *gameState);

void EntityRemoval(void *gameStatePtr) {
    u8  *gs = (u8 *)gameStatePtr;
    s32 *node = *(s32 **)(gs + 0x1C);      /* tick_list_head */

    while (node != NULL) {
        u8 *entity = (u8 *)(intptr_t)node[1];
        u8 *vtable = *(u8 **)(entity + 0x18);
        SlotFn fn = *(SlotFn *)(vtable + 0x1C);

        fn(entity + *(s16 *)(vtable + 0x18));
        /* advance before the removal drain can free the node (export order) */
        node = (s32 *)(intptr_t)node[0];
        DeferredEntityRemoval(gs);
    }
}
