/* =============================================================================
 * PlayerEntityEventHandler.c  --  functional-C body for playst.c
 * PlayerEntityEventHandler (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerEntityEventHandler.s
 * (0x8005C1C4, 0x23C; jump table jtbl_80011B74 supplied to m2c). The player's
 * main event slot during normal play.
 *
 * First switch (0x100C..0x1017): shrink-zone enter/exit, zone-enter state
 * (0x1010, only when not riding/carried: +0x178 and +0x1B2 clear), teleporter
 * warp arrival (0x100F: snap to the portal's position +0x20 down, then the
 * warp state pair), and 0x1017 "can the player be grabbed" query (returns 1
 * when free). Second stage: 0x1000 damage (pick knockback/respawn/bounce-down
 * state by powerup bit 0 of PlayerState+0x17, facing, and +0x144), 0x1006
 * accumulate +0x110 and enter swim-exit, 0x1007 enemy-contact check.
 * All state pairs are the strong sdata marker/fn globals in src/playst.c.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerEnterShrinkZone(void *e);
extern void PlayerExitShrinkZone(void *e);
extern void EntitySetState(void *e, s32 marker, s32 fn);
extern void CheckPlayerHitByEnemy(void *e, void *other, s32 arg);
extern u8  *PLAYER_STATE_DATA asm("D_800A597C");

extern s32 D_800A5D30 asm("D_800A5D30");  /* swim-exit pair            */
extern s32 D_800A5D34 asm("D_800A5D34");
extern s32 D_800A5D50 asm("D_800A5D50");  /* bounce-down pair          */
extern s32 D_800A5D54 asm("D_800A5D54");
extern s32 D_800A5D58 asm("D_800A5D58");  /* bounce-down (facing) pair */
extern s32 D_800A5D5C asm("D_800A5D5C");
extern s32 D_800A5D78 asm("D_800A5D78");  /* respawn pair              */
extern s32 D_800A5D7C asm("D_800A5D7C");
extern s32 D_800A5D80 asm("D_800A5D80");  /* damage-knockback pair     */
extern s32 D_800A5D84 asm("D_800A5D84");
extern s32 D_800A5DB8 asm("D_800A5DB8");  /* zone-enter pair           */
extern s32 D_800A5DBC asm("D_800A5DBC");
extern s32 D_800A5DC0 asm("D_800A5DC0");  /* zone-warp pair            */
extern s32 D_800A5DC4 asm("D_800A5DC4");

s32 PlayerEntityEventHandler(void *arg0, s32 eventId, s32 arg2, void *arg3) {
    u8 *e = (u8 *)arg0;
    u8 *src = (u8 *)arg3;
    s32 result = 0;
    s32 ev = eventId & 0xFFFF;

    switch (ev) {
    case 0x100C:
        PlayerEnterShrinkZone(e);
        break;
    case 0x100D:
        PlayerExitShrinkZone(e);
        break;
    case 0x1010:
        if (e[0x178] == 0 && e[0x1B2] == 0) {
            EntitySetState(e, D_800A5DB8, D_800A5DBC);
        }
        break;
    case 0x100F:
        {
            extern char *getenv(const char *);
            extern void port_log(const char *fmt, ...);
            if (getenv("PORT_TRACE_PLAYER")) {
                port_log("warp 0x100F: src=%p src68/6A=(%d,%d) player was (%d,%d)",
                         src, *(s16 *)(src + 0x68), *(s16 *)(src + 0x6A),
                         *(s16 *)(e + 0x68), *(s16 *)(e + 0x6A));
            }
        }
        *(u16 *)(e + 0x68) = *(u16 *)(src + 0x68);
        *(s16 *)(e + 0x6A) = (s16)(*(s16 *)(src + 0x6A) + 0x20);
        EntitySetState(e, D_800A5DC0, D_800A5DC4);
        break;
    case 0x1017:
        result = (e[0x178] == 0 && e[0x1B2] == 0);
        break;
    default:
        break;
    }

    if (ev == 0x1006) {
        *(s32 *)(e + 0x110) += arg2;
        EntitySetState(e, D_800A5D30, D_800A5D34);
    } else if (ev == 0x1000) {
        if (arg2 != 2) {
            result = 0;
            if (e[0x128] == 0) {
                if (PLAYER_STATE_DATA[0x17] & 1) {
                    if (*(s16 *)(e + 0x68) < *(s16 *)(src + 0x68)) {
                        EntitySetState(e, D_800A5D58, D_800A5D5C);
                    } else {
                        EntitySetState(e, D_800A5D50, D_800A5D54);
                    }
                } else if (*(u16 *)(e + 0x144) != 0) {
                    EntitySetState(e, D_800A5D78, D_800A5D7C);
                } else {
                    EntitySetState(e, D_800A5D80, D_800A5D84);
                }
                result = 1;
            }
        }
    } else if (ev == 0x1007) {
        CheckPlayerHitByEnemy(e, src, arg2);
    }
    return result;
}
