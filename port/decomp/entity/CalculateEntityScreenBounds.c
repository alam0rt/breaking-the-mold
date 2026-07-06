/* =============================================================================
 * CalculateEntityScreenBounds.c  --  PC-port hitbox screen-bounds refresh
 * =============================================================================
 * Functional-C body for CalculateEntityScreenBounds (INCLUDE_ASM in
 * src/entity.c). Rebuilds screenX1/X2/Y1/Y2 (+0x48..+0x4E) from the world
 * position and hitbox rect, honouring facing (+0x74) / flipY (+0x75)
 * mirroring, with each edge projected through the axis' move-transform FSM
 * slot. No-op when boundsValid (+0x77) is already set; sets it when done.
 * The export's four inlined dispatch blobs are the same move_fsm helper used
 * by IsEntityOffScreen.c.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/entity.h"
#include <stdint.h>

typedef s32 (*MoveFn)(void *base, s32 coord);

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

void CalculateEntityScreenBounds(Entity *e) {
    u8 *p = (u8 *)e;
    s16 a, b;

    if (e->boundsValid != 0) {
        return;
    }
    if (e->facing == 0) {
        a = e->worldX + e->hitboxOffsetX;
        b = e->hitboxWidth + e->worldX + e->hitboxOffsetX - 1;
    } else {
        a = ((e->worldX - e->hitboxOffsetX) - e->hitboxWidth) + 1;
        b = e->worldX - e->hitboxOffsetX;
    }
    e->screenX1 = move_fsm(p, a, 0x26, 0x28, 0x24);
    e->screenX2 = move_fsm(p, b, 0x26, 0x28, 0x24);

    if (e->flipY == 0) {
        a = e->worldY + e->hitboxOffsetY;
        b = e->hitboxHeight + e->worldY + e->hitboxOffsetY - 1;
    } else {
        a = ((e->worldY - e->hitboxOffsetY) - e->hitboxHeight) + 1;
        b = e->worldY - e->hitboxOffsetY;
    }
    e->screenY1 = move_fsm(p, a, 0x2E, 0x30, 0x2C);
    e->screenY2 = move_fsm(p, b, 0x2E, 0x30, 0x2C);

    e->boundsValid = 1;
}
