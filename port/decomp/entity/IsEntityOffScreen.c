/* =============================================================================
 * IsEntityOffScreen.c  --  PC-port entity off-screen culling predicate
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c IsEntityOffScreen (0x8001C7B0,
 * INCLUDE_ASM in src/entity.c). Returns 1 if the entity's rendered rect lies
 * entirely outside the on-screen area (with a 16px margin), else 0. Builds the
 * rect from world position + render offset/size (respecting facing/flipY),
 * projects the camera by the entity's parallax scale, and applies the entity's
 * move-transform callbacks (X then Y) to each rect edge before the compare. The
 * checks short-circuit: right-edge, then left-edge, then bottom, then top.
 *
 * Move FSM (base = entity): moveMarkerX lo@0x24/hi@0x26, moveCallbackX@0x28;
 * moveMarkerY lo@0x2C/hi@0x2E, moveCallbackY@0x30. Standard tagged-union.
 * Entity fields: worldX@0x68, worldY@0x6A, renderOffsetX@0x38, renderOffsetY@0x3A,
 * renderWidth@0x3C, renderHeight@0x3E, scaleParallaxX@0x60, scaleParallaxY@0x64,
 * facing@0x74, flipY@0x75. Screen w/h = *(s16*)g_pBlbHeapBase / +2.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;

typedef s32 (*MoveFn)(void *base, s32 coord);

/* Apply the entity's move-transform FSM for the given axis to `coord`.
 * hiOff/cbOff/loOff are the marker-hi / callback / marker-lo byte offsets. */
static s16 move_fsm(u8 *e, s16 coord, s32 hiOff, s32 cbOff, s32 loOff) {
    s16 hi = *(s16 *)(e + hiOff);
    MoveFn fn;
    s16 arg = 0;
    s32 off;
    if (hi == 0) {
        return coord;
    }
    if (hi < 1) {
        fn = *(MoveFn *)(e + cbOff);
    } else {
        s32 slot = hi * 8 + *(s32 *)(e + *(s16 *)(e + cbOff));
        arg = (s16)*(u32 *)((u8 *)(intptr_t)slot - 8);
        fn = *(MoveFn *)((u8 *)(intptr_t)slot - 4);
    }
    off = *(s16 *)(e + loOff);
    if (hi > 0) {
        off = arg + off;
    }
    return (s16)fn(e + off, (s32)coord);
}

unsigned int IsEntityOffScreen(void *entityPtr) {
    u8 *e = (u8 *)entityPtr;
    s16 worldX = *(s16 *)(e + 0x68);
    s16 worldY = *(s16 *)(e + 0x6A);
    s16 roX = *(s16 *)(e + 0x38);
    s16 roY = *(s16 *)(e + 0x3A);
    s16 rW = *(s16 *)(e + 0x3C);
    s16 rH = *(s16 *)(e + 0x3E);
    s16 left, right, top, bottom, camX, camY;

    if (*(u8 *)(e + 0x74) == 0) {              /* facing */
        left  = worldX + roX;
        right = rW + worldX + roX - 1;
    } else {
        left  = ((worldX - roX) - rW) + 1;
        right = worldX - roX;
    }
    if (*(u8 *)(e + 0x75) == 0) {              /* flipY */
        top    = worldY + roY;
        bottom = rH + worldY + roY - 1;
    } else {
        top    = ((worldY - roY) - rH) + 1;
        bottom = worldY - roY;
    }

    if (*(s32 *)(e + 0x60) == 0x10000) {       /* scaleParallaxX */
        camX = g_pGameState->camera_x;
    } else {
        camX = (s16)((u32)((s32)g_pGameState->camera_x * *(s32 *)(e + 0x60)) >> 0x10);
    }
    if (*(s32 *)(e + 0x64) == 0x10000) {       /* scaleParallaxY */
        camY = g_pGameState->camera_y;
    } else {
        camY = (s16)((u32)((s32)g_pGameState->camera_y * *(s32 *)(e + 0x64)) >> 0x10);
    }

    right = move_fsm(e, right, 0x26, 0x28, 0x24);
    if (-0x11 < (s32)right - (s32)camX) {
        left = move_fsm(e, left, 0x26, 0x28, 0x24);
        if ((s32)left - (s32)camX <= *(s16 *)g_pBlbHeapBase + 0x10) {
            bottom = move_fsm(e, bottom, 0x2E, 0x30, 0x2C);
            if (-0x11 < (s32)bottom - (s32)camY) {
                top = move_fsm(e, top, 0x2E, 0x30, 0x2C);
                if ((s32)top - (s32)camY <= *(s16 *)((u8 *)g_pBlbHeapBase + 2) + 0x10) {
                    return 0;
                }
            }
        }
    }
    return 1;
}
