/* =============================================================================
 * AddSwirlys.c  --  PC-port swirly-Q pickup counter + HUD notify
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/AddSwirlys.s
 * (0x800262CC, 0xD0). Adds `count` to the entity's swirly counter (+0x13),
 * clamping at 0x14, then dispatches HUD-reveal event 0x1013 (arg 3) to the HUD
 * entity (g_pGameState+0x14C) through its tagged event slot (marker@+0xA,
 * argBase@+0x8, fn@+0xC -- the shared fsm_send_event layout).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

void AddSwirlys(void *entity, s32 count) {
    u8 *e = (u8 *)entity;
    s32 total = (u8)(e[0x13] + count);
    u8 *hud;

    e[0x13] = (u8)total;
    if ((total & 0xFF) >= 0x15) {
        e[0x13] = 0x14;
    }

    hud = *(u8 **)((u8 *)g_pGameState + 0x14C);
    if (hud == NULL) {
        return;
    }
    fsm_send_event(hud, 0x1013, 3, NULL);
}
