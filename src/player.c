#include "common.h"
#include "globals.h"

extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void DestroyEntityAndFreeMemory(void *entity, s32 mode);
extern void StopSPUVoice(s32 voice);
extern u8 EntityApplyMovementCallbacks(void *entity, s16 x, s16 y);
extern u8 D_80011804[];

s32 CheckTileCollisionOverride(void *entity, u8 *tile);

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
void EntityDestructor_WithSPUVoiceStop(void *entity, s32 flags) {
    u8 *e = (u8 *)entity;
    *(u32 *)(e + 0x18) = (u32)D_80011804;
    StopSPUVoice(*(s32 *)(e + 0x174));
    DestroyEntityAndFreeMemory(entity, 0);
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
 * passable tile 0x65 (i.e. something solid is overhead).
 */
s32 CheckCollisionAbove40(void *entity) {
    u8 tile;
    s16 x = *(s16 *)((u8 *)entity + 0x68);
    s16 y = (s16)(*(u16 *)((u8 *)entity + 0x6A) - 0x40);
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride(entity, &tile);
    return tile != 0x65;
}

/*
 * Specific tile-type probe one pixel higher than CheckCollisionAbove40
 * (y-0x41). Returns true only when the overhead tile is exactly 0x7D -
 * a special marker tile (likely water surface / hazard signal above).
 */
s32 CheckCollisionAbove41(void *entity) {
    u8 tile;
    s16 x = *(s16 *)((u8 *)entity + 0x68);
    s16 y = (s16)(*(u16 *)((u8 *)entity + 0x6A) - 0x41);
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride(entity, &tile);
    return tile == 0x7D;
}

/*
 * Tile-type probe one pixel below the player's feet (y+1). Returns true
 * when the underlying tile is exactly 0x7D - the same special marker as
 * Above41, consistent with a water-surface / submerged-hazard check.
 */
s32 CheckCollisionBelow1(void *entity) {
    u8 tile;
    s16 x = *(s16 *)((u8 *)entity + 0x68);
    s16 y = (s16)(*(u16 *)((u8 *)entity + 0x6A) + 1);
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride(entity, &tile);
    return tile == 0x7D;
}

/*
 * Runs the entity's Y animation/transform chain (callback slot at
 * sprite+0x2E/+0x30/+0x2C) then divides by the level/world scale at
 * +0x58, with a special rounding case for scale == 0xC000. Converts a
 * sprite-local Y value into world/screen Y.
 */
INCLUDE_ASM("asm/nonmatchings/player", TransformYCoordinateWithScale);

/*
 * X counterpart of TransformYCoordinateWithScale - applies the X-side
 * animation transform chain (slots at +0x24/+0x26/+0x28) and divides
 * by entity scale at +0x58. Likely should be named TransformX... for
 * symmetry; "CalculateScaled" is misleading vs. its Y-coord twin.
 */
INCLUDE_ASM("asm/nonmatchings/player", CalculateScaledXCoord);

/*
 * Mirror of TransformYCoordinateWithScale on the X axis: walks the X
 * animation-transform chain (+0x24/+0x26/+0x28) and divides by entity
 * scale (+0x58). Effectively a sibling of CalculateScaledXCoord.
 */
INCLUDE_ASM("asm/nonmatchings/player", TransformXCoordinateWithScale);

/*
 * Y counterpart of CalculateScaledXCoord (slots at +0x2C/+0x2E/+0x30).
 * Animation-transform + scale division for a Y coordinate; likely should
 * be named TransformY... matching its sibling, the existing "Scaled"
 * naming is inconsistent with the twin pair above.
 */
INCLUDE_ASM("asm/nonmatchings/player", CalculateScaledYCoord);

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
 * (0xB5-0xB7, 0xC9, 0xCB, 0xDD-0xDF) to the passable tile 0x65,
 * letting an invincible/charged player phase through them.
 */
s32 CheckTileCollisionOverride(void *entity, u8 *tile) {
    u8 t = *tile;
    if ((u8)(t + 0x4B) < 3 || (t & 0xFF) == 0xC9 || (t & 0xFF) == 0xCB || (u8)(t + 0x23) < 3) {
        if (*(u8 *)((u8 *)entity + 0x128) != 0) {
            *tile = 0x65;
            return 0;
        }
        return 1;
    }
    return 0;
}
