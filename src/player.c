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

/*
 * Runs the entity's Y animation/transform chain (callback slot at
 * sprite+0x2E/+0x30/+0x2C) then divides by the level/world scale at
 * +0x58, with a special rounding case for scale == 0xC000. Converts a
 * sprite-local Y value into world/screen Y.
 */
INCLUDE_ASM("asm/nonmatchings/player", TransformYCoordinateWithScale);

/*
 * X counterpart of CalculateScaledYCoord (slots at +0x24/+0x26/+0x28).
 * Runs the entity's X transform-callback chain on `val`, masks the result
 * DOWN to a 0x10 boundary (& ~0xF, vs the Y twin's | 0xF round-up), then
 * scales it into world space by dividing by scalePowerupX (+0x58).
 */
typedef s32 (*ScaleCoordCB)();
typedef struct { s32 arg; ScaleCoordCB fn; } ScaleCoordSlot;

s16 CalculateScaledXCoord(Entity *e, s32 val) {
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

/*
 * Mirror of TransformYCoordinateWithScale on the X axis: walks the X
 * animation-transform chain (+0x24/+0x26/+0x28) and divides by entity
 * scale (+0x58). Effectively a sibling of CalculateScaledXCoord.
 */
INCLUDE_ASM("asm/nonmatchings/player", TransformXCoordinateWithScale);

/*
 * Y counterpart of CalculateScaledXCoord (slots at +0x2C/+0x2E/+0x30).
 * Runs the entity's Y transform-callback chain on `val` (same FSM-slot
 * dispatch as TransformYCoord in anim.c, but the call target is pinned to
 * $t0 with arg/$a2 and table-fn/$a3 homes), nudges the result up by 0xF,
 * then scales it into world space by dividing by scalePowerupY (+0x5C).
 */
s16 CalculateScaledYCoord(Entity *e, s32 val) {
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
