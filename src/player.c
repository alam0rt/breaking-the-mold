#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/fsm_dispatch.h"

extern u8 PLAYER_CALLBACK_TABLE[] asm("g_PlayerCallbackTable");

/* player .sdata (0x800A5D20..0x800A5D38): three {marker=0xFFFF0000, state-init
 * callback} descriptor pairs consumed by CreatePlayerEntity and the playst
 * player-state callbacks. Migrated from the pooled asm sdata blob
 * (sdata-under-split Phase 4). Address order == declaration order (cc1 2.7.2
 * emits initialized .sdata in decl order). The state-init callbacks are defined
 * in playst.c. D_800A5D30/D_800A5D34 also carry friendly names
 * (PlayerSwimExitMarker / PlayerSwimExitFn) via tentative defs in playst.c that
 * merge with these strong defs. */
extern void PlayerStateInit_Idle(PlayerEntity *e);
extern void PlayerState_HideAndClearBounce(PlayerEntity *e);
extern void PlayerStateCallback_2(PlayerEntity *e);
u32 D_800A5D20 asm("D_800A5D20") = 0xFFFF0000;
EntityCallback D_800A5D24 asm("D_800A5D24") = (EntityCallback)PlayerStateInit_Idle;
u32 D_800A5D28 asm("D_800A5D28") = 0xFFFF0000;
EntityCallback D_800A5D2C asm("D_800A5D2C") = (EntityCallback)PlayerState_HideAndClearBounce;
u32 D_800A5D30 asm("D_800A5D30") = 0xFFFF0000;
EntityCallback D_800A5D34 asm("D_800A5D34") = (EntityCallback)PlayerStateCallback_2;

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
 *
 * A candidate is "available" when its freshly-built SpriteContext has both
 * a nonzero width (+0xC) and nonzero height (+0xE). currentSpriteIndex
 * (+0x19D) is seeded to 0xFF; availableSpriteCount (+0x19C) starts at 0.
 * The candidate sprite-id table is u32 D_8009C3A8[7]; the id (not the
 * context) is what gets stored into availableSpriteIds[].
 *
 * SHELVED: the algorithm below is verified, but the target reads the
 * dims word (ctx+0xC) with an UNALIGNED load (lwl/lwr) and copies it to a
 * scratch stack slot TWICE -- once to test width (low s16), once to test
 * height (high s16). cc1 2.7.2 only emits that unaligned double-copy when
 * the 4-byte dims access comes through a pointer whose alignment it cannot
 * prove; a plain `ctx.width`/`ctx.height` field read (or an aligned
 * `*(s32*)&ctx.width`) yields aligned `lh` and collapses the double copy.
 * Reproducing the exact lwl/lwr provenance is a compiler-idiom detail, not
 * cleanly source-reachable, so this stays INCLUDE_ASM.
 *
 *   extern SpriteContext *InitSpriteContextWrapper(SpriteContext *ctx,
 *                                                  u32 spriteId);
 *   extern u32 D_8009C3A8[7];
 *   void InitPlayerSpriteAvailability(PlayerEntity *e) {
 *       SpriteContext ctx;
 *       s16 i;
 *       e->currentSpriteIndex = 0xFF;
 *       e->availableSpriteCount = 0;
 *       for (i = 0; i < 7; i++) {
 *           s32 available = 0;
 *           InitSpriteContextWrapper(&ctx, D_8009C3A8[i]);
 *           if (ctx.width != 0) {
 *               available = ctx.height != 0;
 *           }
 *           if (available) {
 *               e->availableSpriteIds[e->availableSpriteCount] = D_8009C3A8[i];
 *               e->availableSpriteCount++;
 *           }
 *       }
 *   }
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
 *
 * SHELVED: the algorithm below is verified and compiles to within THREE
 * instructions of the target - the only diff is instruction scheduling of
 * the per-call `worldX` reload. The target hoists `lhu a1,0x68(s0)`
 * (worldX) ABOVE the previous call's result store (`sb v0,tiles[i-1]`),
 * splitting it from the `worldY` (a2) load; cc1 2.7.2 here keeps both
 * dim loads together after the store. The two accesses are independent
 * (entity vs stack), so this is a scheduler coin-flip not controllable
 * from source order (verified: hoisting the arg computation into explicit
 * worldX-first temps does not change the schedule). Stays INCLUDE_ASM.
 *
 *   s32 CheckWallCollision(Entity *entity, s32 dir) {
 *       u8 tiles[4];
 *       s16 dx = 2;
 *       if ((dir & 0xFF) != 0) { dx = -2; }
 *       tiles[0] = EntityApplyMovementCallbacks(entity, entity->worldX + dx,
 *                                               entity->worldY - 0xF);
 *       tiles[1] = EntityApplyMovementCallbacks(entity, entity->worldX + dx,
 *                                               entity->worldY - 0x10);
 *       tiles[2] = EntityApplyMovementCallbacks(entity, entity->worldX + dx,
 *                                               entity->worldY - 0x20);
 *       tiles[3] = EntityApplyMovementCallbacks(entity, entity->worldX + dx,
 *                                               entity->worldY - 0x30);
 *       CheckTileCollisionOverride((PlayerEntity *)entity, &tiles[0]);
 *       CheckTileCollisionOverride((PlayerEntity *)entity, &tiles[1]);
 *       CheckTileCollisionOverride((PlayerEntity *)entity, &tiles[2]);
 *       CheckTileCollisionOverride((PlayerEntity *)entity, &tiles[3]);
 *       return tiles[0] != PLAYER_TILE_PASSABLE &&
 *              tiles[1] != PLAYER_TILE_PASSABLE &&
 *              tiles[2] != PLAYER_TILE_PASSABLE &&
 *              tiles[3] != PLAYER_TILE_PASSABLE;
 *   }
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
