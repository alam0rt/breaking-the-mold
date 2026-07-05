/* =============================================================================
 * SetCameraPositionDirect.c  --  PC-port camera follow/clamp (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/SetCameraPositionDirect.s
 * (0x8007DBA8, 305 lines; reference = export/SLES_010.90.c 20093). Positions the
 * camera to follow the player: resolves the player's rendered X/Y via its
 * move-callback FSM slots (+0x24/0x26/0x28 for X, +0x2C/0x2E/0x30 for Y),
 * computes camera_x/camera_y (+0x44/+0x46) with the hitbox/glide/shake offsets,
 * applies the level scroll-limit clamps, and stores the player render offsets
 * (+0x54/+0x56). No-op while paused (+0x63).
 *
 * The move-callback FSM dispatch is the same tagged idiom as EntitySetState, but
 * the callback takes (entity+argBase, currentValue) and RETURNS the adjusted
 * value. For the menu player all move-slot indices are 0, so the dispatches
 * short-circuit to the raw world position.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern s16 D_8009B038[] asm("D_8009B038");   /* screen-shake offset table (real C, blb.c) */

/* Resolve a move-callback FSM slot: returns the adjusted value (or defaultVal
 * when the slot index is 0). idxOff/cbOff/baseOff are the entity byte offsets of
 * the s16 index, the callback/table-offset word, and the s16 arg base. */
static s16 move_dispatch(u8 *ent, s32 idxOff, s32 cbOff, s32 baseOff, s16 defaultVal) {
    s16 index = *(s16 *)(ent + idxOff);
    s16 (*fn)(void *, s32);
    s16 argOff = 0;
    s16 argBase;

    if (index == 0) {
        return defaultVal;
    }
    if (index < 1) {
        fn = *(s16 (**)(void *, s32))(ent + cbOff);
    } else {
        s16 tableOff = *(s16 *)(ent + cbOff);
        s32 tableBase = *(s32 *)(ent + tableOff);
        u8 *entry = (u8 *)(uintptr_t)(tableBase + (s32)index * 8);
        argOff = (s16)*(s32 *)(entry - 8);
        fn = *(s16 (**)(void *, s32))(entry - 4);
    }
    argBase = *(s16 *)(ent + baseOff);
    if (index > 0) {
        argBase = (s16)(argOff + argBase);
    }
    return fn(ent + argBase, (s32)defaultVal);
}

void SetCameraPositionDirect(void *gameState, s16 param_2, s16 param_3) {
    u8 *gs = (u8 *)gameState;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u8 *player;
    u16 flag;
    u8  camMode;
    s16 rx, ry;
    s16 hitboxW, glideX;

    if (gs[0x63] != 0) {                    /* pause_freeze_flag */
        return;
    }

    player = *(u8 **)(gs + 0x30);           /* player_entity_ptr */
    flag = (u16)gs[0x12A];                  /* glide_boss_flag */
    if (player[0x74] == 1) {
        flag = (u16)(-flag);
    }
    if (gs[0x62] != 0) {                    /* camera_follow_direction */
        flag = (u16)(-flag);
    }
    camMode = gs[0x61];                      /* camera_mode_flags */

    rx = move_dispatch(player, 0x26, 0x28, 0x24, *(s16 *)(player + 0x68));
    glideX = *(s16 *)(gs + 0x120);          /* glide_boss_state_x */
    player = *(u8 **)(gs + 0x30);
    hitboxW = *(s16 *)(gs + 0x64);          /* player_hitbox_width */
    ry = move_dispatch(player, 0x2E, 0x30, 0x2C, *(s16 *)(player + 0x6A));

    *(s16 *)(gs + 0x44) = (s16)(param_2 + hitboxW + glideX + rx + (flag - 0xA0));
    *(s16 *)(gs + 0x46) = (s16)(param_3 + *(s16 *)(gs + 0x66) + *(s16 *)(gs + 0x122)
                                + ry + ((-(u16)(camMode != 0) & 0x60) - 0x78));

    if (gs[0x58] != 0 && *(s16 *)(gs + 0x44) < 0) {          /* scroll_limit_left */
        *(s16 *)(gs + 0x44) = 0;
    }
    if (gs[0x5A] != 0) {                                     /* scroll_limit_right */
        if ((s32)*(s16 *)(gs + 0x48) - (s32)*(s16 *)heap < (s32)*(s16 *)(gs + 0x44)) {
            *(s16 *)(gs + 0x44) = (s16)(*(s16 *)(gs + 0x48) - *(s16 *)heap);
        }
        if (*(s16 *)(gs + 0x44) < 0) {
            *(s16 *)(gs + 0x44) = 0;
        }
    }
    if (gs[0x59] != 0 && *(s16 *)(gs + 0x46) < 5) {          /* scroll_limit_top */
        *(s16 *)(gs + 0x46) = 5;
    }
    if (gs[0x5B] != 0) {                                     /* scroll_limit_bottom */
        if ((s32)*(s16 *)(gs + 0x4A) - (s32)*(s16 *)(heap + 2) - 5 < (s32)*(s16 *)(gs + 0x46)) {
            *(s16 *)(gs + 0x46) = (s16)(*(s16 *)(gs + 0x4A) - *(s16 *)(heap + 2) - 5);
        }
        if (*(s16 *)(gs + 0x46) < 0) {
            *(s16 *)(gs + 0x46) = 0;
        }
    }

    player = *(u8 **)(gs + 0x30);
    *(s16 *)(gs + 0x46) = (s16)(*(s16 *)(gs + 0x46)
                                + D_8009B038[(u8)gs[0x11A]]);   /* screen_shake_index */

    rx = move_dispatch(player, 0x26, 0x28, 0x24, *(s16 *)(player + 0x68));
    player = *(u8 **)(gs + 0x30);
    *(s16 *)(gs + 0x54) = rx;                                /* player_render_offset_x */

    if (*(s16 *)(player + 0x2E) == 0) {
        *(s16 *)(gs + 0x56) = *(s16 *)(player + 0x6A);
    } else {
        *(s16 *)(gs + 0x56) = move_dispatch(player, 0x2E, 0x30, 0x2C, *(s16 *)(player + 0x6A));
    }
}
