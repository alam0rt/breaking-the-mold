/* =============================================================================
 * TeleporterPortalEventHandler.c  --  functional-C body for enemies.c
 * TeleporterPortalEventHandler (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/TeleporterPortalEventHandler.s
 * (0x80045410, 0x1AC). Event slot of the idle teleporter portal (installed by
 * TeleporterExitState).
 *
 * Event 2: detach + remove the linked entity (+0x108) from all lists, then
 * funnel this entity into removal (event 3 to the GameState's tagged slot).
 * Event 1 with arg 0x4A212247 or 0x01A5B0D3 (opaque asset-hash ids): forward
 * event 0x100F to the entity pointed to by GameState+0x2C (the player) via its
 * tagged event slot. Both dispatches are exactly fsm_send_event. Returns 0.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

extern void RemoveEntityFromAllLists(void *gs, void *e);

s32 TeleporterPortalEventHandler(void *arg0, s32 eventId, s32 arg) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;

    switch (eventId & 0xFFFF) {
    case 2: {
        u8 *linked = *(u8 **)(e + 0x108);
        if (linked != NULL) {
            RemoveEntityFromAllLists(gs, linked);
            *(u8 **)(e + 0x108) = NULL;
        }
        fsm_send_event(gs, 3, 0, e);
        break;
    }
    case 1:
        if (arg == 0x4A212247 || arg == 0x01A5B0D3) {
            u8 *target = *(u8 **)(gs + 0x2C);
            fsm_send_event(target, 0x100F, 0, e);
        }
        break;
    default:
        break;
    }
    return 0;
}
