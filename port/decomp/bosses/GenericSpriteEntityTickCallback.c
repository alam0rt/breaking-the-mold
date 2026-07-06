/* =============================================================================
 * GenericSpriteEntityTickCallback.c  --  PC-port generic sprite entity tick
 * =============================================================================
 * Functional-C body for GenericSpriteEntityTickCallback (INCLUDE_ASM in
 * src/bosses.c). Per-frame tick for the generic sprite entity family
 * (platforms, level props): runs the entity update, fires the optional
 * collision probes (+0x110 flag -> event 0x1000 arg 1; else +0x111 flag ->
 * event 0x1007 with the +0x114 payload), then when both the entity and its
 * spawn record's zone (+0x100) are offscreen it clears the spawn-alive bit,
 * notifies the game state (event 3, removal funnel) and fires event 0x1009
 * ("timer done" / owner-notify) at the linked entity slot (+0x11C).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "../decor/fsm_event.h"

extern void EntityUpdateCallback(Entity *e);
extern void CollisionCheckWrapper(Entity *e, s32 a, s32 b, s32 c);
extern unsigned int IsEntityOffScreen(void *e);
extern u32 IsEntityOffScreen_EntityLoop(u8 *gs, s16 *spawnRec);

void GenericSpriteEntityTickCallback(Entity *e) {
    u8 *tail = (u8 *)e;
    u8 *spawnRec;
    u8 *linked;
    int gone = 0;

    EntityUpdateCallback(e);
    if (tail[0x110] != 0) {
        CollisionCheckWrapper(e, 2, 0x1000, 1);
    } else if (tail[0x111] != 0) {
        CollisionCheckWrapper(e, 2, 0x1007, *(s32 *)(tail + 0x114));
    }

    spawnRec = *(u8 **)(tail + 0x100);
    if (spawnRec != NULL && (IsEntityOffScreen(e) & 0xFF) != 0) {
        gone = (IsEntityOffScreen_EntityLoop((u8 *)g_pGameState,
                                             (s16 *)spawnRec) & 0xFF) != 0;
    }
    if (gone) {
        spawnRec[0x16] &= 0xFE;               /* clear spawn-alive bit */
        *(u8 **)(tail + 0x100) = NULL;
        fsm_send_event((u8 *)g_pGameState, 3, 0, e);
        linked = *(u8 **)(tail + 0x11C);
        if (linked != NULL) {
            fsm_send_event(linked, 0x1009, 0, e);
            *(u8 **)(tail + 0x11C) = NULL;
        }
    }
}
