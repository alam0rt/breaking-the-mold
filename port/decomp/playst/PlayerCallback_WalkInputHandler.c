/* =============================================================================
 * PlayerCallback_WalkInputHandler.c  --  functional-C body for playst.c
 * PlayerCallback_WalkInputHandler (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerCallback_WalkInputHandler.s
 * (0x8005FB18, 0x224). Input half of the walking state (the movement half is
 * PlayerCallback_HandleMovementAndCollision).
 *
 * Priority: fire (mask D_800A596E, needs Swirly-Q ammo PlayerState+0x13) ->
 * fire state, or just latch +0x134 while a special move (+0x135) runs; jump
 * (mask D_800A596C) if no ceiling within 0x40 above (probe + tile override,
 * 0x65 blocks) -> jump state (special-move variant when +0x135); otherwise
 * powerup activation (TryActivatePowerup), and if the direction bit for the
 * current facing (0x2000 right / 0x8000 left ... buttons_held) was released:
 * pick the stop/turn state -- direct walk states or the walk timer +0x156
 * under 12 ticks go to the short-stop pair, else the full stop pair.
 * All state pairs are strong sdata globals in src/playst.c.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8   EntityApplyMovementCallbacks(void *e, s16 x, s16 y);
extern void CheckTileCollisionOverride(void *e, u8 *tile);
extern s32  TryActivatePowerup(void *e);
extern void EntitySetState(void *e, u32 marker, u32 fn);
extern void PlayerState_WalkingLeft(void);
extern void PlayerState_WalkingRight(void);
extern u8  *PLAYER_STATE_DATA asm("D_800A597C");
extern u16  D_800A596C asm("D_800A596C");   /* jump button mask */
extern u16  D_800A596E asm("D_800A596E");   /* fire button mask */

extern u32 D_800A5D20 asm("D_800A5D20");    /* short-stop pair      */
extern u32 D_800A5D24 asm("D_800A5D24");
extern u32 D_800A5DB0 asm("D_800A5DB0");    /* jump pair            */
extern u32 D_800A5DB4 asm("D_800A5DB4");
extern u32 D_800A5E00 asm("D_800A5E00");    /* special-jump pair    */
extern u32 D_800A5E04 asm("D_800A5E04");
extern u32 D_800A5E38 asm("D_800A5E38");    /* fire pair            */
extern u32 D_800A5E3C asm("D_800A5E3C");
extern u32 D_800A5E40 asm("D_800A5E40");    /* special-stop pair    */
extern u32 D_800A5E44 asm("D_800A5E44");
extern u32 D_800A5E48 asm("D_800A5E48");    /* full-stop pair       */
extern u32 D_800A5E4C asm("D_800A5E4C");

void PlayerCallback_WalkInputHandler(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *input = *(u8 **)(e + 0x100);            /* InputState */
    u16 held = *(u16 *)(input + 0x0);
    u16 pressed = *(u16 *)(input + 0x2);
    u16 dirMask = (*(u8 *)(e + 0x74) != 0) ? 0x8000 : 0x2000;

    if (PLAYER_STATE_DATA[0x13] != 0 && (D_800A596E & pressed)) {
        if (e[0x135] != 0) {
            e[0x134] = 1;
            return;
        }
        EntitySetState(e, D_800A5E38, D_800A5E3C);
        return;
    }

    if (D_800A596C & pressed) {
        u8 tile = EntityApplyMovementCallbacks(
            e, *(s16 *)(e + 0x68), (s16)(*(s16 *)(e + 0x6A) - 0x40));
        CheckTileCollisionOverride(e, &tile);
        if (tile != 0x65) {
            if (e[0x135] != 0) {
                EntitySetState(e, D_800A5E00, D_800A5E04);
            } else {
                EntitySetState(e, D_800A5DB0, D_800A5DB4);
            }
        }
        return;
    }

    if (!(TryActivatePowerup(e) & 0xFF) && !(dirMask & held)) {
        if (e[0x135] != 0) {
            EntitySetState(e, D_800A5E40, D_800A5E44);
            return;
        }
        if (*(s16 *)(e + 0xA2) == -1) {
            void *stateFn = *(void **)(e + 0xA4);
            if (stateFn == (void *)PlayerState_WalkingRight) {
                EntitySetState(e, D_800A5D20, D_800A5D24);
                return;
            }
            if (stateFn == (void *)PlayerState_WalkingLeft) {
                goto full_stop;
            }
        }
        if (*(u16 *)(e + 0x156) < 0xC) {
            EntitySetState(e, D_800A5D20, D_800A5D24);
            return;
        }
full_stop:
        if (*(s16 *)(e + 0xA2) != -1 ||
            *(void **)(e + 0xA4) != (void *)PlayerState_WalkingLeft) {
            EntitySetState(e, D_800A5E48, D_800A5E4C);
        }
    }
}
