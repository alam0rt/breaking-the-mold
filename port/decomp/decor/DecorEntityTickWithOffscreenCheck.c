/* =============================================================================
 * DecorEntityTickWithOffscreenCheck.c  --  PC-port decor offscreen-cull tick
 * =============================================================================
 * Functional-C body for DecorEntityTickWithOffscreenCheck (INCLUDE_ASM in
 * src/decor.c). Standard decor per-frame tick: runs the entity update, then on
 * this entity's stagger phase (frame&3 == +0x11C) checks whether both the
 * entity AND its spawn record's zone (+0x100) are offscreen; if so it clears
 * the spawn record's alive bit (+0x16 bit0), notifies the game state (event 3,
 * the removal funnel) and marks the entity dead (+0x11E).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "fsm_event.h"

extern void EntityUpdateCallback(Entity *e);
extern unsigned int IsEntityOffScreen(void *e);
extern u32 IsEntityOffScreen_EntityLoop(u8 *gs, s16 *spawnRec);

void DecorEntityTickWithOffscreenCheck(Entity *e) {
    u8 *tail = (u8 *)e;
    u8 *spawnRec = *(u8 **)(tail + 0x100);
    int gone = 0;

    EntityUpdateCallback(e);
    if ((g_pGameState->frame_counter & 3) == tail[0x11C] &&
        (IsEntityOffScreen(e) & 0xFF) != 0 &&
        (spawnRec == NULL ||
         (IsEntityOffScreen_EntityLoop((u8 *)g_pGameState, (s16 *)spawnRec) & 0xFF) != 0)) {
        gone = 1;
    }
    if (gone) {
        if (spawnRec != NULL) {
            spawnRec[0x16] &= 0xFE;               /* clear spawn-alive bit */
            *(u8 **)(tail + 0x100) = NULL;
        }
        fsm_send_event((u8 *)g_pGameState, 3, 0, e);
        tail[0x11E] = 1;
    }
}
