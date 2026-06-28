#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/fsm_dispatch.h"

extern u8 PLAYER_CALLBACK_TABLE[] asm("g_PlayerCallbackTable");

#define PLAYER_TILE_PASSABLE                 0x65
#define PLAYER_TILE_SPECIAL_MARKER           0x7D
#define PLAYER_TILE_INVINCIBLE_OVERRIDE_C9   0xC9
#define PLAYER_TILE_INVINCIBLE_OVERRIDE_CB   0xCB
#define PLAYER_TILE_INVINCIBLE_RANGE1_OFFSET 0x4B
#define PLAYER_TILE_INVINCIBLE_RANGE2_OFFSET 0x23

s32 CheckTileCollisionOverride(PlayerEntity *entity, u8 *tile);

/*
 * Constructor for the main platformer PlayerEntity (0x1B4 used, 0x3E8
 * allocated for scratch). Installs PlayerCallbackTable @ D_80011804,
 * PlayerTickCallback, position/scale callbacks, and seeds soundHandle
 * (+0x174) to -1.
 */
INCLUDE_ASM("asm/nonmatchings/player", CreatePlayerEntity);

/*
 * Probes the (up to 7) per-pose player sprite contexts via
 * InitSpriteContextWrapper and records the available ones into the
 * entity's sprite-pointer table at +0x180, with the available count
 * tracked at +0x19C. Caches which player sprites this level provides.
 */
INCLUDE_ASM("asm/nonmatchings/player", InitPlayerSpriteAvailability);

/*
 * PlayerDestroy callback (slot 0x08 of PlayerCallbackTable @ 0x80011804).
 * Re-pins the vtable, stops the active SPU voice held in soundHandle
 * (+0x174), then tears down the entity and optionally frees its heap
 * allocation when called with flags & 1.
 */
void EntityDestructor_WithSPUVoiceStop(PlayerEntity *entity, s32 flags) {
    entity->sprite.base.collisionVtable = PLAYER_CALLBACK_TABLE;
    StopSPUVoice(entity->soundHandle);
    DestroyEntityAndFreeMemory((SpriteEntity *)entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

/*
 * Horizontal wall probe: tests tiles two pixels in the player's facing
 * direction (a1=0 right/+2, non-zero left/-2) at four body heights
 * (head/chest/waist/knee, y-0xF/-0x10/-0x20/-0x30) and filters them
 * through CheckTileCollisionOverride. Used for side-wall blocking.
 */
INCLUDE_ASM("asm/nonmatchings/player", CheckWallCollision);

/*
 * Ceiling presence probe at (x, y-0x40) - roughly head height for a
 * normal-size player. Returns true when the tile there is NOT the
 * passable tile PLAYER_TILE_PASSABLE (i.e. something solid is overhead).
 */
s32 CheckCollisionAbove40(Entity *entity) {
    u8 tile;
    s16 x = entity->worldX;
    s16 y = entity->worldY - 0x40;
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride((PlayerEntity *)entity, &tile);
    return tile != PLAYER_TILE_PASSABLE;
}

/*
 * Specific tile-type probe one pixel higher than CheckCollisionAbove40
 * (y-0x41). Returns true only when the overhead tile is exactly
 * PLAYER_TILE_SPECIAL_MARKER -
 * a special marker tile (likely water surface / hazard signal above).
 */
s32 CheckCollisionAbove41(Entity *entity) {
    u8 tile;
    s16 x = entity->worldX;
    s16 y = entity->worldY - 0x41;
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride((PlayerEntity *)entity, &tile);
    return tile == PLAYER_TILE_SPECIAL_MARKER;
}

/*
 * Tile-type probe one pixel below the player's feet (y+1). Returns true
 * when the underlying tile is exactly PLAYER_TILE_SPECIAL_MARKER - the
 * same special marker as Above41, consistent with a water-surface /
 * submerged-hazard check.
 */
s32 CheckCollisionBelow1(Entity *entity) {
    u8 tile;
    s16 x = entity->worldX;
    s16 y = entity->worldY + 1;
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride((PlayerEntity *)entity, &tile);
    return tile == PLAYER_TILE_SPECIAL_MARKER;
}

typedef s32 (*ScaleCoordCB)();
typedef struct { s32 arg; ScaleCoordCB fn; } ScaleCoordSlot;

/* ---- Entity coordinate-transform + scale family ----------------------------
 * Four siblings that map a coordinate through the entity's move-callback FSM
 * slot (same dispatch as anim.c Transform{X,Y}Coord) and then divide by the
 * entity's powerup scale to project it into world/screen space. They differ
 * only in axis, in how the pre-divide value is rounded, and whether they apply
 * the 0xC000 "snap":
 *   TransformYCoordWithScale         Y  round up   (val | 0xF)          / scaleY(+0x5C)
 *   TransformXCoordWithScale         X  round down (val & ~0xF)         / scaleX(+0x58)
 *   TransformYCoordWithScaleSnapped  Y  round down (val & ~0xF)         / scaleY  + snap
 *   TransformXCoordWithScaleSnapped  X  round up   ((val & ~0xF)+0x10)  / scaleX  + snap
 * Snap: when scalePowerupX == 0xC000 (the 0.75 zoom) and the quotient mod 4 is
 * 1 or 2, nudge the result up by one to correct fixed-point rounding.
 * Match recipe (all four): $t0/$a2/$a3/$v0 register pins, s32 `val`, and the
 * player.o --expand-div flag for the signed-division trap guards.
 * ------------------------------------------------------------------------- */

/* Y axis, round DOWN, /scaleY, with 0xC000 snap. See family header above. */
s16 TransformYCoordWithScaleSnapped(Entity *e, s32 val) {
    s16 m = ((s16 *)&e->moveMarkerY)[1];
    FSM_REG(ScaleCoordCB, fn, "$8"); /* $t0 call target */
    FSM_REG(ScaleCoordCB, tf, "$7"); /* $a3 table-fn that relays into $t0 */
    FSM_REG(s32, arg, "$6");         /* $a2 */
    FSM_REG(s32, adj, "$2");         /* $v0 */
    s32 vt = val;                    /* saved copy of val used by the call path */
    s32 lo;
    s16 s;
    s32 r;
    s32 q;
    if (m != 0) {
        s = m;
        if (s > 0) {
            ScaleCoordSlot *base =
                *(ScaleCoordSlot **)((u8 *)e + *(s16 *)&e->moveCallbackY);
            arg = base[s - 1].arg;
            tf = base[s - 1].fn;
            FSM_RELAY(fn, tf); /* emits move $t0,$a3 */
        } else {
            fn = (ScaleCoordCB)e->moveCallbackY;
        }
        lo = ((s16 *)&e->moveMarkerY)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        r = fn((u8 *)e + adj, (s16)vt);
    } else {
        r = val;
    }
    q = ((r & ~0xF) << 16) / e->scalePowerupY;
    if (e->scalePowerupX == 0xC000 && (u32)((q & 3) - 1) < 2) {
        q++;
    }
    return (s16)q;
}

/* X axis, round DOWN, /scaleX, no snap. See family header above. */
s16 TransformXCoordWithScale(Entity *e, s32 val) {
    s16 m = ((s16 *)&e->moveMarkerX)[1];
    FSM_REG(ScaleCoordCB, fn, "$8"); /* $t0 call target */
    FSM_REG(ScaleCoordCB, tf, "$7"); /* $a3 table-fn that relays into $t0 */
    FSM_REG(s32, arg, "$6");         /* $a2 */
    FSM_REG(s32, adj, "$2");         /* $v0 */
    s32 vt = val;                    /* saved copy of val used by the call path */
    s32 lo;
    s16 s;
    s32 r;
    if (m != 0) {
        s = m;
        if (s > 0) {
            ScaleCoordSlot *base =
                *(ScaleCoordSlot **)((u8 *)e + *(s16 *)&e->moveCallbackX);
            arg = base[s - 1].arg;
            tf = base[s - 1].fn;
            FSM_RELAY(fn, tf); /* emits move $t0,$a3 */
        } else {
            fn = (ScaleCoordCB)e->moveCallbackX;
        }
        lo = ((s16 *)&e->moveMarkerX)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        r = fn((u8 *)e + adj, (s16)vt);
    } else {
        r = val;
    }
    return (s16)(((r & ~0xF) << 16) / e->scalePowerupX);
}

/* X axis, round UP ((val & ~0xF)+0x10), /scaleX, with 0xC000 snap. See family
 * header. The snap reuses the just-divided scalePowerupX load (not reloaded). */
s16 TransformXCoordWithScaleSnapped(Entity *e, s32 val) {
    s16 m = ((s16 *)&e->moveMarkerX)[1];
    FSM_REG(ScaleCoordCB, fn, "$8"); /* $t0 call target */
    FSM_REG(ScaleCoordCB, tf, "$7"); /* $a3 table-fn that relays into $t0 */
    FSM_REG(s32, arg, "$6");         /* $a2 */
    FSM_REG(s32, adj, "$2");         /* $v0 */
    s32 vt = val;                    /* saved copy of val used by the call path */
    s32 lo;
    s16 s;
    s32 r;
    s32 q;
    if (m != 0) {
        s = m;
        if (s > 0) {
            ScaleCoordSlot *base =
                *(ScaleCoordSlot **)((u8 *)e + *(s16 *)&e->moveCallbackX);
            arg = base[s - 1].arg;
            tf = base[s - 1].fn;
            FSM_RELAY(fn, tf); /* emits move $t0,$a3 */
        } else {
            fn = (ScaleCoordCB)e->moveCallbackX;
        }
        lo = ((s16 *)&e->moveMarkerX)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        r = fn((u8 *)e + adj, (s16)vt);
    } else {
        r = val;
    }
    q = (((r & ~0xF) + 0x10) << 16) / e->scalePowerupX;
    if (e->scalePowerupX == 0xC000 && (u32)((q & 3) - 1) < 2) {
        q++;
    }
    return (s16)q;
}

/* Y axis, round UP (val | 0xF), /scaleY, no snap. See family header above. */
s16 TransformYCoordWithScale(Entity *e, s32 val) {
    s16 m = ((s16 *)&e->moveMarkerY)[1];
    FSM_REG(ScaleCoordCB, fn, "$8"); /* $t0 call target */
    FSM_REG(ScaleCoordCB, tf, "$7"); /* $a3 table-fn that relays into $t0 */
    FSM_REG(s32, arg, "$6");         /* $a2 */
    FSM_REG(s32, adj, "$2");         /* $v0 */
    s32 vt = val;                    /* saved copy of val used by the call path */
    s32 lo;
    s16 s;
    s32 r;
    if (m != 0) {
        s = m;
        if (s > 0) {
            ScaleCoordSlot *base =
                *(ScaleCoordSlot **)((u8 *)e + *(s16 *)&e->moveCallbackY);
            arg = base[s - 1].arg;
            tf = base[s - 1].fn;
            FSM_RELAY(fn, tf); /* emits move $t0,$a3 */
        } else {
            fn = (ScaleCoordCB)e->moveCallbackY;
        }
        lo = ((s16 *)&e->moveMarkerY)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        r = fn((u8 *)e + adj, (s16)vt);
    } else {
        r = val;
    }
    r |= 0xF;
    return (s16)((r << 16) / e->scalePowerupY);
}

/*
 * Combines both X and Y animation/transform callbacks for the player,
 * then calls GetSlopeHeightAtSubpixel on g_pGameState to settle the
 * player onto the floor slope at the resulting subpixel position.
 * The post-physics "snap to ground" step.
 */
INCLUDE_ASM("asm/nonmatchings/player", PlayerApplyPositionWithCollision);

/*
 * Proximity test: runs the same X+Y transform pipeline as
 * PlayerApplyPositionWithCollision, computes a slope-relative Y, and
 * returns true when the diff against a reference Y (u16 passed as 5th
 * arg via stack) is < 10. Used by sound triggers but likely should be
 * named generically (IsEntityNearPoint/EntityWithinSlopeRange).
 */
/* IsEntityNearSoundTrigger @0x8005A3F8 — structurally-perfect C exists
 * (same 115 instrs / shape) but has a pervasive coupled saved-register
 * renumbering across the two inline FSM-slot dispatches (target homes
 * yval->$a1 / Y-survivor->$a3 / refY->$s6; natural C picks $a3/$t0/$s5).
 * Parked as a permuter base in tools/decomp-permuter/nonmatchings/
 * IsEntityNearSoundTrigger; needs a permuter run to settle the coloring. */
INCLUDE_ASM("asm/nonmatchings/player", IsEntityNearSoundTrigger);

/*
 * Powerup-aware tile filter: when the player's invincibilityTimer
 * (+0x128) is non-zero, rewrites certain destructible tile codes
 * (0xB5-0xB7, 0xC9, 0xCB, 0xDD-0xDF) to the passable tile,
 * letting an invincible/charged player phase through them.
 */
s32 CheckTileCollisionOverride(PlayerEntity *entity, u8 *tile) {
    u8 t = *tile;
    if ((u8)(t + PLAYER_TILE_INVINCIBLE_RANGE1_OFFSET) < 3 || (t & 0xFF) == PLAYER_TILE_INVINCIBLE_OVERRIDE_C9 || (t & 0xFF) == PLAYER_TILE_INVINCIBLE_OVERRIDE_CB || (u8)(t + PLAYER_TILE_INVINCIBLE_RANGE2_OFFSET) < 3) {
        if (entity->invincibilityTimer != 0) {
            *tile = PLAYER_TILE_PASSABLE;
            return 0;
        }
        return 1;
    }
    return 0;
}
