#include "common.h"
#include "Game/game_state.h"
#include "Game/entity.h"

// INCLUDE_ASM("asm/pal/nonmatchings/EntityTickLoop", EntityTickLoop);

void EntityTickLoop(GameState *state) {
    EntityListNode *node;
    Entity *entity;
    EntityCallbackTable *callbacks;

    node = state->entity_tick_list_head;
    if (node) {
        do {
            entity = node->entity;
            callbacks = (EntityCallbackTable *)entity->tick_callback;
            callbacks->tick((Entity *)((u8 *)entity + callbacks->entity_offset));
            node = node->next;
        } while (node);
    }
}
