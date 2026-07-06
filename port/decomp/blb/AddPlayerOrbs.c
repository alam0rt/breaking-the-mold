/* =============================================================================
 * AddPlayerOrbs.c  --  PC-port orb pickup accounting
 * =============================================================================
 * Functional-C body for AddPlayerOrbs (INCLUDE_ASM in src/blb.c). Adds to the
 * player's orb count; every 100 orbs rolls over into an extra life (capped at
 * 99) with the 1-up jingle and the HUD lives-group reveal (event 0x1013 group
 * 2). Always ends by revealing the HUD orb-counter group (group 1).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/player_state.h"
#include "../decor/fsm_event.h"

extern void PlaySoundEffect(u32 soundId, s32 volume, u8 flag);

void AddPlayerOrbs(PlayerState *ps, u8 count) {
    u8 *hud;

    ps->orb_count = (u8)(ps->orb_count + count);
    if (ps->orb_count > 99) {
        ps->orb_count = (u8)(ps->orb_count - 99);
        ps->lives = (u8)(ps->lives + 1);
        if (ps->lives > 99) {
            ps->lives = 99;
        }
        hud = (u8 *)g_pGameState->hud_entity_ptr;
        if (hud != NULL) {
            fsm_send_event(hud, 0x1013, 2, 0);   /* reveal lives group */
        }
        PlaySoundEffect(0x428254E2, 0xA0, 0);    /* 1-up jingle */
    }
    hud = (u8 *)g_pGameState->hud_entity_ptr;
    if (hud != NULL) {
        fsm_send_event(hud, 0x1013, 1, 0);       /* reveal orb group */
    }
}
