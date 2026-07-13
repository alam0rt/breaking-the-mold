/* =============================================================================
 * PlayerCallback_JumpWallCollisionInput.c  --  PC-port conversion (playst.c)
 * =============================================================================
 * Input slot for the pushing-against-a-wall pose. Body taken verbatim from
 * the VERIFIED C parked at src/playst.c:1180 (shelved over a one-slot codegen
 * residual only). Priorities: special-move button (queue while active),
 * look/duck button (blocked under a low ceiling), powerup, turn-away (with
 * wall re-check), then -- once the button pressing INTO the wall is
 * released -- exit to the stand state pair. This stub being missing left
 * Klaymen stuck in the wall-push animation.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8  EntityApplyMovementCallbacks(void *e, s16 x, s16 y);
extern s32 CheckWallCollision(void *entity, s32 dir);
extern s32 CheckTileCollisionOverride(void *entity, u8 *tile);
extern s32 TryActivatePowerup(void *e);
extern void EntitySetState(void *e, u32 marker, u32 fn);

extern u8 *PLAYER_STATE_DATA asm("D_800A597C");
extern u16 g_JumpLookInputMask asm("D_800A596E");
extern u16 g_JumpDropInputMask asm("D_800A596C");
extern u32 JumpSpecialLookMarker asm("D_800A5E38");
extern u32 JumpSpecialLookFn     asm("D_800A5E3C");
extern u32 JumpDropSpecialMarker asm("D_800A5E00");
extern u32 JumpDropSpecialFn     asm("D_800A5E04");
extern u32 JumpWallSpecialMarker asm("D_800A5E08");
extern u32 JumpWallSpecialFn     asm("D_800A5E0C");
extern u32 JumpWallTurnMarker    asm("D_800A5E18");
extern u32 JumpWallTurnFn        asm("D_800A5E1C");
extern u32 JumpLandSpecialMarker asm("D_800A5E40");
extern u32 JumpLandSpecialFn     asm("D_800A5E44");
extern u32 JumpLandMarker        asm("D_800A5E60");
extern u32 JumpLandFn            asm("D_800A5E64");
extern u32 PICKUP_MARKER         asm("D_800A5DB0");
extern u32 PICKUP_FN             asm("D_800A5DB4");

void PlayerCallback_JumpWallCollisionInput(PlayerEntity *e) {
    u32 fwdMask;
    u32 backMask;
    u8 tile;
    u32 marker;
    u32 fn;
    u16 held;

    backMask = 0x2000;
    fwdMask = 0x8000;
    if (e->sprite.base.facing != 0) {
        backMask = 0x8000;
        fwdMask = 0x2000;
    }
    if (PLAYER_STATE_DATA[0x13] != 0) {
        if ((e->pInput->buttons_pressed & g_JumpLookInputMask) != 0) {
            if (e->specialMoveMode != 0) {
                e->specialMoveQueued = 1;
                return;
            }
            marker = JumpSpecialLookMarker;
            fn = JumpSpecialLookFn;
            goto setState;
        }
    }
    if ((e->pInput->buttons_pressed & g_JumpDropInputMask) != 0) {
        tile = EntityApplyMovementCallbacks(e, e->sprite.base.worldX,
                                            (s16)(e->sprite.base.worldY - 0x40));
        CheckTileCollisionOverride(e, &tile);
        if (tile == 0x65) {
            return;
        }
        if (e->specialMoveMode != 0) {
            marker = JumpDropSpecialMarker;
            fn = JumpDropSpecialFn;
        } else {
            marker = PICKUP_MARKER;
            fn = PICKUP_FN;
        }
        goto setState;
    }
    if ((u8)TryActivatePowerup(e) != 0) {
        return;
    }
    held = e->pInput->buttons_held;
    if ((fwdMask & held) != 0) {
        if ((u8)CheckWallCollision(e, e->sprite.base.facing < 1) == 0) {
            return;
        }
        {
            u8 newFacing = e->sprite.base.facing < 1;
            u8 sm = e->specialMoveMode;
            e->sprite.base.facing = newFacing;
            if (sm != 0) {
                marker = JumpWallSpecialMarker;
                fn = JumpWallSpecialFn;
            } else {
                marker = JumpWallTurnMarker;
                fn = JumpWallTurnFn;
            }
        }
        goto setState;
    }
    if ((backMask & held) != 0) {
        return;
    }
    if (e->specialMoveMode != 0) {
        marker = JumpLandSpecialMarker;
        fn = JumpLandSpecialFn;
    } else {
        marker = JumpLandMarker;
        fn = JumpLandFn;
    }
setState:
    EntitySetState(e, marker, fn);
}
