/* =============================================================================
 * PlayerCallback_WalkInputWithFall.c  --  functional-C body for playst.c
 * PlayerCallback_WalkInputWithFall (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerCallback_WalkInputWithFall.s
 * (0x8005FF20, 0x1C0). Third sibling of the walk input family (see
 * PlayerCallback_WalkInputHandler / _WithJump): same fire/jump/powerup
 * priority; on direction release goes straight to the stop pair
 * (special-stop when +0x135), and when the direction is held but the
 * run/dash mask D_800A5970 is NOT -> the walk-downshift pair D_800A5E58/5C.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u8   EntityApplyMovementCallbacks(void *e, s16 x, s16 y);
extern void CheckTileCollisionOverride(void *e, u8 *tile);
extern s32  TryActivatePowerup(void *e);
extern void EntitySetState(void *e, u32 marker, u32 fn);
extern u8  *PLAYER_STATE_DATA asm("D_800A597C");
extern u16  D_800A596C asm("D_800A596C");   /* jump button mask */
extern u16  D_800A596E asm("D_800A596E");   /* fire button mask */
extern u16  D_800A5970 asm("D_800A5970");   /* run/dash button mask */

extern u32 D_800A5DB0 asm("D_800A5DB0");    /* jump pair          */
extern u32 D_800A5DB4 asm("D_800A5DB4");
extern u32 D_800A5E00 asm("D_800A5E00");    /* special-jump pair  */
extern u32 D_800A5E04 asm("D_800A5E04");
extern u32 D_800A5E38 asm("D_800A5E38");    /* fire pair          */
extern u32 D_800A5E3C asm("D_800A5E3C");
extern u32 D_800A5E40 asm("D_800A5E40");    /* special-stop pair  */
extern u32 D_800A5E44 asm("D_800A5E44");
extern u32 D_800A5E48 asm("D_800A5E48");    /* full-stop pair     */
extern u32 D_800A5E4C asm("D_800A5E4C");
extern u32 D_800A5E58 asm("D_800A5E58");    /* walk-downshift pair */
extern u32 D_800A5E5C asm("D_800A5E5C");

void PlayerCallback_WalkInputWithFall(void *arg0) {
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

    if (!(TryActivatePowerup(e) & 0xFF)) {
        if (!(dirMask & held)) {
            if (e[0x135] != 0) {
                EntitySetState(e, D_800A5E40, D_800A5E44);
            } else {
                EntitySetState(e, D_800A5E48, D_800A5E4C);
            }
            return;
        }
        if (!(D_800A5970 & held)) {
            EntitySetState(e, D_800A5E58, D_800A5E5C);
        }
    }
}
