/* =============================================================================
 * EntityTick_EasedMovementInterpolation.c  --  PC-port collected-pickup flight
 * =============================================================================
 * Functional-C body for EntityTick_EasedMovementInterpolation (INCLUDE_ASM in
 * src/decor.c). The 33-step eased screen-space flight a collected pickup makes
 * toward its HUD target: per tick, looks up the s16 ease pair in the
 * D_8009B240 table (stride 4: xEase, yEase) and positions the entity between
 * start (+0x114/+0x116) and target (+0x110/+0x112); after step 0x20 it
 * notifies the game state (event 3, removal funnel). Installed as the RENDER
 * callback by InitDecorEntityWithScreenOffset.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include "fsm_event.h"
#include <string.h>

extern u8 D_8009B240[132];   /* strong data, src/decor.c */

void EntityTick_EasedMovementInterpolation(Entity *e) {
    u8 *tail = (u8 *)e;
    u16 stepIdx = *(u16 *)(tail + 0x11A);

    if (stepIdx < 0x21) {
        s16 ex, ey;
        u16 startX = *(u16 *)(tail + 0x114);
        u16 startY = *(u16 *)(tail + 0x116);
        u16 targetX = *(u16 *)(tail + 0x110);
        u16 targetY = *(u16 *)(tail + 0x112);
        memcpy(&ex, D_8009B240 + stepIdx * 4, 2);
        memcpy(&ey, D_8009B240 + stepIdx * 4 + 2, 2);
        e->worldX = (s16)(startX + (s16)((-(s32)ex * (s16)(targetX - startX)) >> 7));
        *(u16 *)(tail + 0x11A) = stepIdx + 1;
        e->worldY = (s16)(startY + (s16)((-(s32)ey * (s16)(targetY - startY)) >> 7));
    } else {
        fsm_send_event((u8 *)g_pGameState, 3, 0, e);
    }
}
