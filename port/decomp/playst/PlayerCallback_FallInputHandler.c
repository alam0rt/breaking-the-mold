/* =============================================================================
 * PlayerCallback_FallInputHandler.c  --  PC-port conversion (playst.c)
 * =============================================================================
 * Input slot for the crouch/duck ("fall to ground") pose. Body taken verbatim
 * from the VERIFIED C parked at src/playst.c:1280 (shelved there over a
 * 2-instruction scheduling residual only). Priorities: special-move button
 * (queue while special active), look/duck button (blocked when the tile 0x40
 * above is solid 0x65 -- low ceiling), turn on left/right press, powerup
 * activation, then -- once DOWN (held & 0x4000) is RELEASED -- exit to the
 * stand-up state pair. This stub being missing left Klaymen stuck in the
 * crouch pose forever.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8  EntityApplyMovementCallbacks(void *e, s16 x, s16 y);
extern s32 CheckTileCollisionOverride(void *entity, u8 *tile);
extern s32 TryActivatePowerup(void *e);
extern void EntitySetState(void *e, u32 marker, u32 fn);

extern u8 *PLAYER_STATE_DATA asm("D_800A597C");
extern u16 g_PlayerFallSpecialInputMask asm("D_800A596E");
extern u16 g_PlayerFallLookInputMask asm("D_800A596C");
extern u32 PlayerFallSpecialMarker asm("D_800A5E88");
extern u32 PlayerFallSpecialFn     asm("D_800A5E8C");
extern u32 PlayerFallDuckMarker    asm("D_800A5E00");
extern u32 PlayerFallDuckFn        asm("D_800A5E04");
extern u32 PlayerFallPickupMarker  asm("D_800A5DB0");
extern u32 PlayerFallPickupFn      asm("D_800A5DB4");
extern u32 PlayerFallTurnMarker    asm("D_800A5E90");
extern u32 PlayerFallTurnFn        asm("D_800A5E94");
extern u32 PlayerFallLandMarker    asm("D_800A5E98");
extern u32 PlayerFallLandFn        asm("D_800A5E9C");
extern u32 PlayerFallLandAltMarker asm("D_800A5EA0");
extern u32 PlayerFallLandAltFn     asm("D_800A5EA4");

void PlayerCallback_FallInputHandler(PlayerEntity *e) {
    u16 buttons;
    u8 tile;
    u32 marker;
    u32 fn;

    if (PLAYER_STATE_DATA[0x13] != 0) {
        if ((e->pInput->buttons_pressed & g_PlayerFallSpecialInputMask) != 0) {
            if (e->specialMoveMode != 0) {
                e->specialMoveQueued = 1;
                return;
            }
            marker = PlayerFallSpecialMarker;
            fn = PlayerFallSpecialFn;
            goto setState;
        }
    }
    buttons = e->pInput->buttons_pressed;
    if ((buttons & g_PlayerFallLookInputMask) != 0) {
        tile = EntityApplyMovementCallbacks(e, e->sprite.base.worldX,
                                            (s16)(e->sprite.base.worldY - 0x40));
        CheckTileCollisionOverride(e, &tile);
        if (tile == 0x65) {
            return;
        }
        if (e->specialMoveMode != 0) {
            marker = PlayerFallDuckMarker;
            fn = PlayerFallDuckFn;
        } else {
            marker = PlayerFallPickupMarker;
            fn = PlayerFallPickupFn;
        }
        goto setState;
    }
    if ((buttons & 0x8000) != 0) {
        if (e->sprite.base.facing != 0) {
            return;
        }
        e->sprite.base.facing = 1;
        marker = PlayerFallTurnMarker;
        fn = PlayerFallTurnFn;
        goto setState;
    }
    if ((buttons & 0x2000) != 0) {
        if (e->sprite.base.facing == 0) {
            return;
        }
        e->sprite.base.facing = 0;
        marker = PlayerFallTurnMarker;
        fn = PlayerFallTurnFn;
        goto setState;
    }
    if ((u8)TryActivatePowerup(e) != 0) {
        return;
    }
    if ((e->pInput->buttons_held & 0x4000) != 0) {
        return;
    }
    if (e->specialMoveMode != 0) {
        marker = PlayerFallLandMarker;
        fn = PlayerFallLandFn;
    } else {
        marker = PlayerFallLandAltMarker;
        fn = PlayerFallLandAltFn;
    }
setState:
    EntitySetState(e, marker, fn);
}
