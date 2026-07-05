/* =============================================================================
 * UpdateCameraPositionSmooth.c  --  PC-port smooth camera follow (TARGET_PC)
 * =============================================================================
 * Faithful functional-C reconstruction of UpdateCameraPositionSmooth
 * (0x800233C0; reference = export/SLES_010.90.c lines 19708-20066). This is the
 * velocity-eased sibling of SetCameraPositionDirect: it resolves the player's
 * rendered X/Y through the move-callback FSM slots, computes the desired camera
 * target (camera_x/camera_y at +0x44/+0x46) with the hitbox / glide / camera-
 * mode offsets, then eases camera_velocity_x/y (16.16 fixed-point) toward that
 * target using the D_8009B074/B0BC/B104 accel tables, integrates the sub-pixel
 * accumulators, re-applies the scroll-limit clamps, adds the screen-shake
 * offset, and stores the player render offsets (+0x54/+0x56).
 *
 * The move-callback FSM dispatch (see move_fsm below) appears six times in the
 * original; it is factored into one helper here. The two final dispatches thread
 * extra register arguments the callee ignores (MIPS calling-convention spill);
 * those are harmless and dropped on the C side.
 *
 * player_entity_ptr is accessed by byte offset (it is an Entity, 0x80 bytes):
 *   +0x24 moveMarkerX lo / +0x26 hi / +0x28 moveCallbackX
 *   +0x2C moveMarkerY lo / +0x2E hi / +0x30 moveCallbackY
 *   +0x68 worldX (u16) / +0x6A worldY (u16) / +0x74 facing byte
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/game_state.h"
#include <stdint.h>

/* Camera easing / screen-shake lookup tables in PSX .sbss. Declared as weak-
 * backed absolute-address aliases; the port stub generator (gen_port_stubs.py)
 * provides zero-filled backing storage, so the camera does not smooth-
 * accelerate until these are given real data (acceptable on the title screen). */
extern u8 D_8009B038[] asm("D_8009B038");   /* screen-shake offset table (s16) */
extern u8 D_8009B074[] asm("D_8009B074");   /* accel table A (s32) */
extern u8 D_8009B0BC[] asm("D_8009B0BC");   /* accel table B (s32) */
extern u8 D_8009B104[] asm("D_8009B104");   /* accel table C (s32) */

typedef s32 (*MoveFn)(void *base, s32 coord);

/* Move-callback FSM dispatch (tagged-union). hiOff/cbOff/loOff/worldOff are the
 * entity byte offsets of the s16 marker hi-half, the callback/table-offset word,
 * the s16 marker lo-half, and the u16 world coordinate. Returns the raw world
 * coordinate (zero-extended) when the marker hi-half is 0, otherwise the
 * callback's transformed coordinate. Callers consume the result either as a
 * uint (X/Y target math) or truncated to s16 (render offsets). */
static u32 move_fsm(u8 *ent, s32 hiOff, s32 cbOff, s32 loOff, s32 worldOff)
{
    s32 idx = (s32)*(s16 *)(ent + hiOff);
    u32 result = (u32)*(u16 *)(ent + worldOff);

    if (idx != 0) {
        MoveFn fn;
        s16 arg = 0;
        s32 off;

        if (idx < 1) {
            fn = *(MoveFn *)(ent + cbOff);
        } else {
            s16 tableOff = *(s16 *)(ent + cbOff);
            s32 tableBase = *(s32 *)(ent + tableOff);
            u8 *entry = (u8 *)(uintptr_t)(tableBase + idx * 8);
            arg = (s16)*(u32 *)(entry - 8);
            fn = *(MoveFn *)(entry - 4);
        }
        off = (s32)*(s16 *)(ent + loOff);
        if (idx > 0) {                       /* 0 < (idx << 16)  ==  idx > 0 */
            off = (s32)arg + off;
        }
        result = (u32)fn(ent + off, (s32)(s16)*(u16 *)(ent + worldOff));
    }
    return result;
}

void UpdateCameraPositionSmooth(GameState *gameState)
{
    u8  *puVar14;
    u8   uVar1;
    u16  uVar2, uVar3, uVar4, uVar5;
    s16  sVar6, sVar8, sVar9;
    s32  iVar10, iVar11, iVar12, iVar13, iVar18;
    u32  uVar15, uVar16, uVar17, uVar20;

    puVar14 = (u8 *)gameState->player_entity_ptr;
    if (puVar14 == NULL) {
        return;
    }
    if (gameState->pause_freeze_flag != 0) {
        return;
    }

    uVar20 = (u32)gameState->glide_boss_flag;
    if (puVar14[0x74] == 1) {
        uVar20 = -uVar20;
    }
    if (gameState->camera_follow_direction != 0) {
        uVar20 = -uVar20;
    }
    uVar1 = gameState->camera_mode_flags;

    uVar15 = move_fsm(puVar14, 0x26, 0x28, 0x24, 0x68);   /* X world -> render */

    uVar2 = (u16)gameState->glide_boss_state_x;
    puVar14 = (u8 *)gameState->player_entity_ptr;
    uVar3 = (u16)gameState->player_hitbox_width;

    uVar16 = move_fsm(puVar14, 0x2e, 0x30, 0x2c, 0x6a);   /* Y world -> render */

    uVar4 = (u16)gameState->glide_boss_state_y;
    puVar14 = (u8 *)gameState->player_entity_ptr;
    uVar5 = (u16)gameState->player_hitbox_y_offset;

    uVar17 = move_fsm(puVar14, 0x2e, 0x30, 0x2c, 0x6a);   /* Y world -> render */

    puVar14 = (u8 *)gameState->player_entity_ptr;
    iVar10 = (s32)(uVar17 - (u16)gameState->player_render_offset_y);
    uVar17 = move_fsm(puVar14, 0x26, 0x28, 0x24, 0x68);   /* X world -> render */

    iVar11 = (s32)((u32)uVar5 + (u32)uVar4 + uVar16
                   + ((-(u32)(uVar1 != 0) & 0x60) - 0x78) + (u32)iVar10);
    uVar17 = uVar17 - (u16)gameState->player_render_offset_x;
    iVar18 = (s32)((u32)uVar3 + (u32)uVar2 + uVar15 + (uVar20 - 0xa0) + uVar17);

    if (gameState->scroll_limit_left != 0 && (s16)iVar18 < 0) {
        iVar18 = 0;
    }
    if (gameState->scroll_limit_right != 0) {
        if ((s32)gameState->camera_limit_x - (s32)*(s16 *)g_pBlbHeapBase < (s32)(s16)iVar18) {
            iVar18 = (s32)gameState->camera_limit_x - (s32)*(s16 *)g_pBlbHeapBase;
        }
        if ((s16)iVar18 < 0) {
            iVar18 = 0;
        }
    }
    if (gameState->scroll_limit_top != 0 && (s16)iVar11 < 5) {
        iVar11 = 5;
    }
    if (gameState->scroll_limit_bottom != 0) {
        if ((s32)gameState->camera_limit_y - (s32)*(s16 *)((u8 *)g_pBlbHeapBase + 2) - 5
                < (s32)(s16)iVar11) {
            iVar11 = (s32)gameState->camera_limit_y - (s32)*(s16 *)((u8 *)g_pBlbHeapBase + 2) - 5;
        }
        if ((s16)iVar11 < 0) {
            iVar11 = 0;
        }
    }

    /* ---- Camera X: ease camera_velocity_x toward the target delta ---- */
    sVar9 = gameState->camera_x;   /* threaded into dropped dispatch #5 reg args */
    sVar6 = gameState->camera_x;
    uVar20 = (u32)iVar18 - (u32)(u16)gameState->camera_x;
    iVar18 = (s32)(s16)uVar20;
    if (iVar18 == 0) {
        gameState->camera_velocity_x = 0;
    } else {
        if ((uVar17 & 0xffff) == 0) {
            if (iVar18 < 0) {
                uVar20 = -uVar20;
            }
            if (0x8f < (s16)uVar20) {
                iVar12 = 0xa0000;
            } else {
                iVar12 = *(s32 *)(D_8009B074 + ((uVar20 >> 1) & 0x7c));
            }
        } else {
            if (iVar18 < 0) {
                uVar20 = -uVar20;
            }
            if ((s16)uVar20 < 0x90) {
                iVar12 = *(s32 *)(D_8009B104 + ((uVar20 >> 1) & 0x7c));
            } else {
                iVar12 = 0xa0000;
            }
        }
        if (iVar18 < 0) {
            iVar12 = -iVar12;
        }
        iVar18 = gameState->camera_velocity_x;
        if (iVar18 < iVar12) {
            iVar13 = 0x10000;
            if (iVar18 < 0) {
                iVar13 = 0x8000;
            }
            gameState->camera_velocity_x = iVar18 + iVar13;
            if (iVar12 < gameState->camera_velocity_x) {
                gameState->camera_velocity_x = iVar12;
            }
        } else if (iVar18 > iVar12) {
            iVar13 = -0x10000;
            if (0 < iVar18) {
                iVar13 = -0x8000;
            }
            gameState->camera_velocity_x = iVar18 + iVar13;
            if (gameState->camera_velocity_x < iVar12) {
                gameState->camera_velocity_x = iVar12;
            }
        }
        /* iVar18 == iVar12: leave camera_velocity_x unchanged (goto LAB_800238e0) */
    }

    /* ---- Camera Y: ease camera_velocity_y toward the target delta ---- */
    uVar20 = (u32)iVar11 - (u32)(u16)gameState->camera_y;
    if ((uVar20 & 0xffff) == 0) {
        gameState->camera_velocity_y = 0;
        goto LAB_80023aec;
    }
    if (gameState->bounce_active_flag != 0
        && (s16)(gameState->camera_y - 0x66) <= (s16)iVar11
        && (s16)iVar11 <= (s16)(gameState->camera_y + 0x10)) {
        iVar18 = gameState->camera_velocity_y;
        if (iVar18 < 1) {
            if (-1 < iVar18) {
                goto LAB_80023aec;              /* iVar18 == 0: no change */
            }
            iVar11 = 0x10000;
            if ((s16)iVar10 <= 0) {             /* iVar10 * 0x10000 < 1 */
                iVar11 = 0x8000;
            }
        } else {
            iVar11 = -0x8000;
            if ((s16)iVar10 < 0) {              /* iVar10 * 0x10000 < 0 */
                iVar11 = -0x10000;
            }
        }
        gameState->camera_velocity_y = iVar18 + iVar11;
        goto LAB_80023aec;
    }
    iVar18 = (s32)(s16)uVar20;
    if (iVar18 < 1) {
        if ((s16)iVar10 == 0) {                 /* iVar10 * 0x10000 == 0 */
            if (iVar18 < 0) {
                uVar20 = -uVar20;
            }
            if ((s16)uVar20 < 0x90) {
                iVar10 = *(s32 *)(D_8009B074 + ((uVar20 >> 1) & 0x7c));
            } else {
                iVar10 = 0xa0000;
            }
        } else {
            if (iVar18 < 0) {
                uVar20 = -uVar20;
            }
            if ((s16)uVar20 < 0x90) {
                iVar10 = *(s32 *)(D_8009B104 + ((uVar20 >> 1) & 0x7c));
            } else {
                iVar10 = 0xa0000;
            }
        }
        if (iVar18 < 0) {
            iVar10 = -iVar10;
        }
    } else if ((s16)iVar10 == 0) {              /* iVar10 * 0x10000 == 0 */
        if (0x8f < iVar18) {
            iVar10 = 0xa0000;                   /* LAB_800239d4 */
        } else {
            iVar10 = *(s32 *)(D_8009B074 + ((uVar20 >> 1) & 0x7c));
        }
    } else if (0x8f < iVar18) {
        iVar10 = 0xa0000;                       /* LAB_800239d4 */
    } else {
        iVar10 = *(s32 *)(D_8009B0BC + ((uVar20 >> 1) & 0x7c));
    }
    iVar18 = gameState->camera_velocity_y;
    if (iVar18 < iVar10) {
        iVar11 = 0x10000;
        if (iVar18 < 0) {
            iVar11 = 0x8000;
        }
        gameState->camera_velocity_y = iVar18 + iVar11;
        if (iVar10 < gameState->camera_velocity_y) {
            gameState->camera_velocity_y = iVar10;
        }
    } else if (iVar18 > iVar10) {
        gameState->camera_velocity_y = iVar18 - 0x10000;
        if (gameState->camera_velocity_y < iVar10) {
            gameState->camera_velocity_y = iVar10;
        }
    }
    /* iVar18 == iVar10: leave camera_velocity_y unchanged (goto LAB_80023aec) */

LAB_80023aec:
    iVar18 = (s32)((u32)gameState->camera_y * 0x10000 + (u32)gameState->camera_subpixel_y
                   + (u32)gameState->camera_velocity_y);
    gameState->camera_subpixel_y = (u16)iVar18;
    iVar10 = (s32)((u32)gameState->camera_x * 0x10000 + (u32)gameState->camera_subpixel_x
                   + (u32)gameState->camera_velocity_x);
    gameState->camera_x = (s16)((u32)iVar10 >> 0x10);
    gameState->camera_y = (s16)((u32)iVar18 >> 0x10);
    gameState->camera_subpixel_x = (u16)iVar10;

    if (gameState->scroll_limit_left != 0 && gameState->camera_x < 0) {
        gameState->camera_x = 0;
    }
    if (gameState->scroll_limit_right != 0) {
        if ((s32)gameState->camera_limit_x - (s32)*(s16 *)g_pBlbHeapBase
                < (s32)gameState->camera_x) {
            gameState->camera_x = (s16)(gameState->camera_limit_x - *(s16 *)g_pBlbHeapBase);
        }
        if (gameState->camera_x < 0) {
            gameState->camera_x = 0;
        }
    }
    if (gameState->scroll_limit_top != 0 && gameState->camera_y < 5) {
        gameState->camera_y = 5;
    }
    if (gameState->scroll_limit_bottom != 0) {
        if ((s32)gameState->camera_limit_y - (s32)*(s16 *)((u8 *)g_pBlbHeapBase + 2) - 5
                < (s32)gameState->camera_y) {
            gameState->camera_y = (s16)(gameState->camera_limit_y
                                        - *(s16 *)((u8 *)g_pBlbHeapBase + 2) - 5);
        }
        if (gameState->camera_y < 0) {
            gameState->camera_y = 0;
        }
    }

    puVar14 = (u8 *)gameState->player_entity_ptr;
    gameState->camera_y = (s16)(gameState->camera_y
        + *(s16 *)(D_8009B038 + (u32)(u8)gameState->screen_shake_index * 2));

    /* dispatch #5 (X) -> player_render_offset_x. sVar9/sVar6 fed the dropped
       extra register args (sVar9-0x32, sVar6+0x32) that the callee ignores. */
    (void)sVar6;
    (void)sVar9;
    sVar8 = (s16)move_fsm(puVar14, 0x26, 0x28, 0x24, 0x68);
    puVar14 = (u8 *)gameState->player_entity_ptr;
    gameState->player_render_offset_x = sVar8;

    /* dispatch #6 (Y) -> player_render_offset_y */
    gameState->player_render_offset_y = (s16)move_fsm(puVar14, 0x2e, 0x30, 0x2c, 0x6a);
    return;
}
