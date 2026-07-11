/* =============================================================================
 * PlayerCallback_HandleMovementAndCollision.c  --  PC-port grounded movement
 * =============================================================================
 * Functional-C conversion of PlayerCallback_HandleMovementAndCollision
 * (0x800638D0, 0x738) from m2c over asm/nonmatchings/playst/, structure kept,
 * artifacts cleaned (same recipe as PlayerCallback_FallingPhysicsMain.c).
 * The walking-state physics core, ticked while the player is on the ground.
 *
 * 1. Integrate the 16.16 subpixel position: velocity magnitudes +0xB4/+0xB8
 *    signed by the facing bytes +0x74/+0x75, plus the conveyor/platform push
 *    +0x160 on X.
 * 2. Probe the tile column ahead (y-0xF/-0x10/-0x20/-0x30): bounce-class
 *    tiles ((t+0x4B)&0xFF<3 | 0xC9 | 0xCB | (t+0x23)&0xFF<3) hand off to
 *    PlayerProcessBounceCollision (suppressed to wall 0x65 while the
 *    invincibility timer +0x128 runs); a wall tile 0x65 snaps X back to the
 *    tile edge (transform-X slot + scale divide), zeroes the X subpixel and
 *    -- unless mid-special-move (+0x135) or already in the walk/resume-walk
 *    state -- enters the wall-stop state pair D_800A5E20/24.
 * 3. Ground resolution at y-7 (death tile 0x53 -> D_800A5ECC/D0), then y+2,
 *    then y+0x10: slope tiles (0<t<0x3C) run PlayerApplyPositionWithCollision
 *    to land; a clear column enters the airborne pair D_800A5D30/34.
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
extern u8   CheckWallCollision(void *e, u8 facing);
extern void EntitySetState(void *e, u32 marker, u32 fn);
extern void PlayerStateInit_Walking(void);
extern void PlayerStateInit_ResumeWalking(void);

extern u32 D_800A5D30 asm("D_800A5D30");  /* airborne/fall pair  */
extern u32 D_800A5D34 asm("D_800A5D34");
extern u32 D_800A5E20 asm("D_800A5E20");  /* wall-stop pair      */
extern u32 D_800A5E24 asm("D_800A5E24");
extern u32 D_800A5ECC asm("D_800A5ECC");  /* death-tile pair     */
extern u32 D_800A5ED0 asm("D_800A5ED0");

/* Bounce-class tile test shared by all probes: [0xB5..0xB7], 0xC9, 0xCB,
 * [0xDD..0xDF]. While the invincibility timer runs the tile is downgraded to
 * a plain wall (0x65) instead. Returns 1 when the bounce path should run. */
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

/* Tagged transform-X slot dispatch ({lo,hi}@+0x24, fn/off@+0x28), same
 * encoding as fsm_event.h; value passes through when the slot is empty. */
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

void PlayerCallback_HandleMovementAndCollision(void *arg0) {
    u8 *e = (u8 *)arg0;
    s16 oldX = S16AT(e, 0x68);
    s32 fx = ((s32)oldX << 16) + U16AT(e, 0x6C);
    s32 fy = ((s32)S16AT(e, 0x6A) << 16) + U16AT(e, 0x6E);
    s32 probeBias;
    s16 dx;
    u8 t0, t1, t2, t3, tg;

    /* 16.16 integration, velocity sign from the facing bytes */
    if (U8AT(e, 0x74) != 0) {
        fx -= S32AT(e, 0xB4);
    } else {
        fx += S32AT(e, 0xB4);
    }
    if (U8AT(e, 0x75) != 0) {
        fy -= S32AT(e, 0xB8);
    } else {
        fy += S32AT(e, 0xB8);
    }
    S16AT(e, 0x68) = (s16)(fx >> 16);
    S16AT(e, 0x6A) = (s16)(fy >> 16);
    U16AT(e, 0x6C) = (u16)fx;
    U16AT(e, 0x6E) = (u16)fy;
    S16AT(e, 0x68) = (s16)(U16AT(e, 0x68) + U16AT(e, 0x160));

    dx = (s16)(S16AT(e, 0x68) - oldX);
    probeBias = (dx == 0) ? 0 : (dx < 0 ? -2 : 2);

    /* wall / bounce probes up the body column, in travel direction */
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
        /* wall ahead: snap X back to the tile edge */
        if (dx < 0) {
            S16AT(e, 0x68) =
                TransformXCoordWithScaleSnapped(e, (s16)(U16AT(e, 0x68) + probeBias));
        } else if (dx > 0) {
            s16 v = (s16)(U16AT(e, 0x68) + probeBias);
            if (S16AT(e, 0x26) != 0) {
                v = transform_x_slot(e, v);
            }
            S16AT(e, 0x68) =
                (s16)((s16)(((s32)((v & ~0xF)) << 16) / S32AT(e, 0x58)) - 1);
        }
        U16AT(e, 0x6C) = 0;
        if (U8AT(e, 0x135) == 0) {
            void *stateFn = *(void **)(e + 0xA4);
            if ((S16AT(e, 0xA2) != -1 ||
                 (stateFn != (void *)PlayerStateInit_Walking &&
                  stateFn != (void *)PlayerStateInit_ResumeWalking)) &&
                !CheckWallCollision(e, U8AT(e, 0x74))) {
                EntitySetState(e, D_800A5E20, D_800A5E24);
                return;
            }
        }
    } else {
        /* free horizontal move: refresh the signed X momentum snapshot */
        if (dx < 0) {
            S32AT(e, 0x10C) = ((s32)S16AT(e, 0x160) << 16) - S32AT(e, 0xB4);
        } else {
            S32AT(e, 0x10C) = ((s32)S16AT(e, 0x160) << 16) + S32AT(e, 0xB4);
        }
    }

    /* ground resolution: probe at y-7, y+2, then y+0x10 */
    tg = EntityApplyMovementCallbacks(e, S16AT(e, 0x68), (s16)(U16AT(e, 0x6A) - 7));
    if (tg == 0x53) {
        EntitySetState(e, D_800A5ECC, D_800A5ED0);
        return;
    }
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
        return;
    }
    if (tg == 0) {
        tg = EntityApplyMovementCallbacks(e, S16AT(e, 0x68),
                                          (s16)(U16AT(e, 0x6A) + 0x10));
        if (tg != 0 && tg < 0x3C) {
            S16AT(e, 0x6A) = PlayerApplyPositionWithCollision(
                e, tg, S16AT(e, 0x68), (s16)(U16AT(e, 0x6A) + 0x10));
            return;
        }
        /* nothing under the feet: go airborne */
        EntitySetState(e, D_800A5D30, D_800A5D34);
    }
}
