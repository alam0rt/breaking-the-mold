/* =============================================================================
 * PlayerCallback_IdleInputHandler.c  --  PC-port grounded/idle input dispatch
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/playst/
 * PlayerCallback_IdleInputHandler.s (0x8005F844, 0x2C4; m2c-derived structure).
 * Installed in the idle/standing player's input FSM slot: reads the pad and
 * picks the next state via EntitySetState --
 *   - special button + swirlys available -> queue/launch the swirly-Q toss;
 *   - jump button -> jump (state depends on whether the head tile is solid);
 *   - else, if no powerup consumed the press, a held direction ->
 *     walk/turn/push-against-wall, or fall off a ledge.
 * The D_800A5Dxx/5Exx marker/callback pairs are strong data in src/playst.c.
 *
 * Button masks are the src/gfx.c sdata island: D_800A596C (0x40, jump) and
 * D_800A596E (0x20, special). D_800A597C is the respawn player-state block
 * (its +0x13 byte is the live swirly-Q count).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void EntitySetState(void *entity, u32 marker, u32 callback);
extern u8   EntityApplyMovementCallbacks(void *entity, s16 worldX, s16 worldY);
extern s32  CheckTileCollisionOverride(void *entity, u8 *tile);
extern s32  TryActivatePowerup(void *entity);
extern s32  CheckWallCollision(void *entity, s32 dir);

extern char PlayerStateInit_Walking[];

/* button masks (src/gfx.c sdata island) */
extern u16 D_800A596C_jump    asm("D_800A596C");   /* 0x40 */
extern u16 D_800A596E_special asm("D_800A596E");   /* 0x20 */
/* respawn player-state block; +0x13 = live swirly-Q count */
extern u8 *D_800A597C_state   asm("D_800A597C");

/* FSM marker/callback pairs (src/playst.c data island) */
extern u32 D_800A5DB0; extern void *D_800A5DB4;   /* jump (grounded)      */
extern u32 D_800A5DF8; extern void *D_800A5DFC;   /* swirly-Q toss        */
extern u32 D_800A5E00; extern void *D_800A5E04;   /* jump (special mode)  */
extern u32 D_800A5E08; extern void *D_800A5E0C;   /* push wall (special)  */
extern u32 D_800A5E10; extern void *D_800A5E14;   /* walk (same facing)   */
extern u32 D_800A5E18; extern void *D_800A5E1C;   /* turn + walk          */
extern u32 D_800A5E20; extern void *D_800A5E24;   /* start walking        */
extern u32 D_800A5E28; extern void *D_800A5E2C;   /* down (special mode)  */
extern u32 D_800A5E30; extern void *D_800A5E34;   /* fall off ledge       */

#define SET_STATE(M, F) EntitySetState(e, (M), (u32)(uintptr_t)(F))

/* Held-direction handler shared by the right (facing 0) and left (facing 1)
 * cases; they differ only in the facing value and the wall-probe direction. */
static void handle_direction(u8 *e, s32 facing) {
    if (CheckWallCollision(e, facing) & 0xFF) {          /* pushing into a wall */
        if (e[0x135] != 0) {
            e[0x74] = (u8)facing;
            SET_STATE(D_800A5E08, D_800A5E0C);
        } else if (e[0x74] == (u8)facing) {
            SET_STATE(D_800A5E10, D_800A5E14);
        } else {
            e[0x74] = (u8)facing;
            SET_STATE(D_800A5E18, D_800A5E1C);
        }
    } else if (e[0x135] == 0 &&
               (*(s16 *)(e + 0xA2) != -1 ||
                *(void **)(e + 0xA4) != (void *)PlayerStateInit_Walking)) {
        e[0x74] = (u8)facing;
        SET_STATE(D_800A5E20, D_800A5E24);
    }
}

void PlayerCallback_IdleInputHandler(void *arg0) {
    u8  *e = (u8 *)arg0;
    u8  *input = *(u8 **)(e + 0x100);
    u16  pressed = *(u16 *)(input + 2);
    u16  held;
    u8   tile;

    /* special button + swirlys available -> swirly-Q toss */
    if (D_800A597C_state[0x13] != 0 && (D_800A596E_special & pressed)) {
        if (e[0x135] != 0) {
            e[0x134] = 1;              /* queue behind the active special move */
            return;
        }
        SET_STATE(D_800A5DF8, D_800A5DFC);
        return;
    }

    /* jump button */
    if (D_800A596C_jump & pressed) {
        tile = EntityApplyMovementCallbacks(e, (s16)*(u16 *)(e + 0x68),
                                            (s16)(*(u16 *)(e + 0x6A) - 0x40));
        CheckTileCollisionOverride(e, &tile);
        if ((tile ^ 0x65) != 0) {      /* head tile solid -> jump allowed */
            if (e[0x135] != 0) {
                SET_STATE(D_800A5E00, D_800A5E04);
            } else {
                SET_STATE(D_800A5DB0, D_800A5DB4);
            }
        }
        return;
    }

    /* no jump: let a powerup consume the press, else handle held direction */
    if (TryActivatePowerup(e) & 0xFF) {
        return;
    }
    held = *(u16 *)(input + 0);
    if (held & 0x2000) {               /* right */
        handle_direction(e, 0);
    } else if (held & 0x8000) {         /* left */
        handle_direction(e, 1);
    } else if (held & 0x4000) {         /* down -> fall off ledge */
        if (e[0x135] != 0) {
            SET_STATE(D_800A5E28, D_800A5E2C);
        } else {
            SET_STATE(D_800A5E30, D_800A5E34);
        }
    }
}
