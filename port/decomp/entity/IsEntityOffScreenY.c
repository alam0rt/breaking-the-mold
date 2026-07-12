/* =============================================================================
 * IsEntityOffScreenY.c  --  functional-C body for entity.c
 * IsEntityOffScreenY (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/entity/IsEntityOffScreenY.s
 * (0x8001C178, 0x1EC). "Fell off the bottom" test used by debris/particle
 * ticks: build the entity's world-space box from the sprite offsets
 * (+0x38/+0x3C X, +0x3A/+0x3E Y; the +0x74/+0x75 facing flags mirror it),
 * take the box TOP y, run it through the entity's Y coordinate transform
 * (the +0x2C/+0x2E/+0x30 slot == matched-C TransformYCoord), subtract the
 * parallax-scaled camera Y (gs+0x46, factor 16.16 at +0x64), and compare
 * against the screen height (heap base +0x2) + 16. Returns 1 when the top of
 * the entity is below the visible area.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"

extern void *g_pBlbHeapBase;
extern s16 TransformYCoord(Entity *e, s16 val);   /* real C, src/anim.c */

s32 IsEntityOffScreenY(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    u16 y1;
    s32 factor;
    s16 scroll;
    s16 sy;

    /* box top Y (world space); +0x75 flips the Y offsets */
    if (e[0x75] != 0) {
        y1 = (u16)(*(u16 *)(e + 0x6A) - *(u16 *)(e + 0x3A) - *(u16 *)(e + 0x3E) + 1);
    } else {
        y1 = (u16)(*(u16 *)(e + 0x6A) + *(u16 *)(e + 0x3A));
    }

    /* parallax-scaled camera scroll */
    factor = *(s32 *)(e + 0x64);
    if (factor == 0x10000) {
        scroll = (s16)*(u16 *)(gs + 0x46);
    } else {
        scroll = (s16)(((s32)*(s16 *)(gs + 0x46) * factor) >> 16);
    }

    sy = TransformYCoord((Entity *)e, (s16)y1);
    return (s16)(sy - scroll) > (s16)(*(s16 *)((u8 *)g_pBlbHeapBase + 0x2) + 0x10);
}
