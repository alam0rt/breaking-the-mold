/* =============================================================================
 * EntityRenderWithScaledPosition.c  --  PC-port conversion (0x8002D8AC)
 * =============================================================================
 * Render callback for the orb pickup: normal sprite render via
 * UpdateEntityRender, then reposition the companion HUD-icon effect object
 * (+0x120, the rotating-star entity) to the orb's world position minus the
 * camera origin (GameState +0x44/+0x46), scaled by the entity X scale
 * (+0x58, 16.16; the ==0x10000 fast path reads the position UNSIGNED, the
 * scaled path SIGNED -- kept as in the asm). Finally sets the icon's
 * "ticked this frame" byte (+0x1E7) so Render_RotatingStarEffect animates.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void UpdateEntityRender(void *e);

void EntityRenderWithScaledPosition(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs;
    u8 *icon;
    s32 scale;

    UpdateEntityRender(arg0);

    scale = *(s32 *)(e + 0x58);
    gs = (u8 *)g_pGameState;
    if (scale == 0x10000) {
        icon = *(u8 **)(e + 0x120);
        *(s16 *)(icon + 0x0) =
            (s16)(*(u16 *)(e + 0x68) - *(s16 *)(gs + 0x44));
        icon = *(u8 **)(e + 0x120);
        *(s16 *)(icon + 0x2) =
            (s16)(*(u16 *)(e + 0x6A) - *(s16 *)(gs + 0x46));
    } else {
        icon = *(u8 **)(e + 0x120);
        *(s16 *)(icon + 0x0) =
            (s16)(((*(s16 *)(e + 0x68) * scale) >> 16) - *(s16 *)(gs + 0x44));
        icon = *(u8 **)(e + 0x120);
        *(s16 *)(icon + 0x2) =
            (s16)(((*(s16 *)(e + 0x6A) * *(s32 *)(e + 0x58)) >> 16) -
                  *(s16 *)(gs + 0x46));
    }

    icon = *(u8 **)(e + 0x120);
    icon[0x1E7] = 1;
}
