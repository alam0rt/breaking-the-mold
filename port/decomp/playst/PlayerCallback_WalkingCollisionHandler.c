/* =============================================================================
 * PlayerCallback_WalkingCollisionHandler.c  --  functional-C body for playst.c
 * PlayerCallback_WalkingCollisionHandler (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerCallback_WalkingCollision-
 * Handler.s (0x80064B40, 0x584). Third sibling of the grounded movement family
 * (HandleMovementAndCollision / HorizontalWallCollision): momentum-driven --
 * integrates the signed 16.16 X momentum +0x10C into the position, same
 * bounce-tile probes up the body column, wall snap (via the transform-X slot
 * and scale divide), then ground resolution at y-7 and y+2 only (no deeper
 * probe and no airborne fallback -- the pickup/cooldown state keeps its
 * current Y when nothing is found).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

#define U8AT(p, off)  (*(u8 *)((u8 *)(p) + (off)))
#define U16AT(p, off) (*(u16 *)((u8 *)(p) + (off)))
#define S16AT(p, off) (*(s16 *)((u8 *)(p) + (off)))
#define S32AT(p, off) (*(s32 *)((u8 *)(p) + (off)))

extern u8   EntityApplyMovementCallbacks(void *e, s16 x, s16 y);
extern s16  TransformXCoordWithScaleSnapped(void *e, s16 x);
extern s16  PlayerApplyPositionWithCollision(void *e, u8 tile, s16 x, s16 y);
extern void PlayerProcessBounceCollision(void *e, u8 tile);

static int bounce_tile_check(u8 *e, u8 *tile) {
    u8 t = *tile;
    if ((u8)(t + 0x4B) < 3 || t == 0xC9 || t == 0xCB || (u8)(t + 0x23) < 3) {
        if (U8AT(e, 0x128) != 0) {
            *tile = 0x65;
            return 0;
        }
        return 1;
    }
    return 0;
}

static s16 transform_x_slot(u8 *e, s16 value) {
    s16 hi = S16AT(e, 0x26);
    s16 (*fn)(void *, s16);
    s32 arg = 0;
    s16 target;
    if (hi == 0) {
        return value;
    }
    if (hi > 0) {
        u8 *entry = *(u8 **)(e + S16AT(e, 0x28)) + hi * 8;
        arg = S32AT(entry, -8);
        fn = (s16 (*)(void *, s16))(uintptr_t)S32AT(entry, -4);
    } else {
        fn = *(s16 (**)(void *, s16))(e + 0x28);
    }
    target = S16AT(e, 0x24);
    if (hi > 0) {
        target += (s16)arg;
    }
    return fn(e + target, value);
}

void PlayerCallback_WalkingCollisionHandler(void *arg0) {
    u8 *e = (u8 *)arg0;
    s32 momentum = S32AT(e, 0x10C);
    s32 fx = ((s32)S16AT(e, 0x68) << 16) + U16AT(e, 0x6C) + momentum;
    s32 probeBias = (momentum < 0) ? -1 : 1;
    u8 t0, t1, t2, t3, tg;

    S16AT(e, 0x68) = (s16)(fx >> 16);
    U16AT(e, 0x6C) = (u16)fx;

    t0 = EntityApplyMovementCallbacks(e, (s16)(U16AT(e, 0x68) + probeBias),
                                      (s16)(U16AT(e, 0x6A) - 0x0F));
    t1 = EntityApplyMovementCallbacks(e, (s16)(U16AT(e, 0x68) + probeBias),
                                      (s16)(U16AT(e, 0x6A) - 0x10));
    t2 = EntityApplyMovementCallbacks(e, (s16)(U16AT(e, 0x68) + probeBias),
                                      (s16)(U16AT(e, 0x6A) - 0x20));
    t3 = EntityApplyMovementCallbacks(e, (s16)(U16AT(e, 0x68) + probeBias),
                                      (s16)(U16AT(e, 0x6A) - 0x30));
    if (bounce_tile_check(e, &t0)) { PlayerProcessBounceCollision(e, t0); return; }
    if (bounce_tile_check(e, &t1)) { PlayerProcessBounceCollision(e, t1); return; }
    if (bounce_tile_check(e, &t2)) { PlayerProcessBounceCollision(e, t2); return; }
    if (bounce_tile_check(e, &t3)) { PlayerProcessBounceCollision(e, t3); return; }

    if (t0 == 0x65 || t1 == 0x65 || t2 == 0x65 || t3 == 0x65) {
        if (momentum < 0) {
            S16AT(e, 0x68) =
                TransformXCoordWithScaleSnapped(e, (s16)(U16AT(e, 0x68) + probeBias));
        } else {
            s16 v = (s16)(U16AT(e, 0x68) + probeBias);
            if (S16AT(e, 0x26) != 0) {
                v = transform_x_slot(e, v);
            }
            S16AT(e, 0x68) =
                (s16)((s16)(((s32)((v & ~0xF)) << 16) / S32AT(e, 0x58)) - 1);
        }
        U16AT(e, 0x6C) = 0;
        return;
    }

    tg = EntityApplyMovementCallbacks(e, S16AT(e, 0x68), (s16)(U16AT(e, 0x6A) - 7));
    if (tg != 0 && tg < 0x3C) {
        S16AT(e, 0x6A) = PlayerApplyPositionWithCollision(
            e, tg, S16AT(e, 0x68), (s16)(U16AT(e, 0x6A) - 7));
        return;
    }

    tg = EntityApplyMovementCallbacks(e, S16AT(e, 0x68), (s16)(U16AT(e, 0x6A) + 2));
    if (bounce_tile_check(e, &tg)) {
        PlayerProcessBounceCollision(e, tg);
        return;
    }
    if (tg != 0 && tg < 0x3C) {
        S16AT(e, 0x6A) = PlayerApplyPositionWithCollision(
            e, tg, S16AT(e, 0x68), (s16)(U16AT(e, 0x6A) + 2));
    }
}
