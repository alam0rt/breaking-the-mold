/* =============================================================================
 * CheckWallCollision.c  --  PC-port horizontal wall probe (4-point)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/player/CheckWallCollision.s
 * (0x80059BC8, 0x160; pseudocode already in src/player.c's plate comment).
 * Samples the tile at four heights (worldY - 0xF/0x10/0x20/0x30) two pixels to
 * the side (right when dir==0, left when dir!=0) of the player's world X,
 * running each through CheckTileCollisionOverride, and returns true only when
 * ALL four samples are solid (none == 0x65 PLAYER_TILE_PASSABLE) -- i.e. a wall
 * fully blocks that direction.
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern u8  EntityApplyMovementCallbacks(void *entity, s16 worldX, s16 worldY);
extern s32 CheckTileCollisionOverride(void *entity, u8 *tile);

#define PLAYER_TILE_PASSABLE 0x65

s32 CheckWallCollision(void *entity, s32 dir) {
    u8 *e = (u8 *)entity;
    s16 dx = ((dir & 0xFF) != 0) ? -2 : 2;
    u8 tiles[4];

    tiles[0] = EntityApplyMovementCallbacks(e, (s16)(*(u16 *)(e + 0x68) + dx),
                                            (s16)(*(u16 *)(e + 0x6A) - 0xF));
    tiles[1] = EntityApplyMovementCallbacks(e, (s16)(*(u16 *)(e + 0x68) + dx),
                                            (s16)(*(u16 *)(e + 0x6A) - 0x10));
    tiles[2] = EntityApplyMovementCallbacks(e, (s16)(*(u16 *)(e + 0x68) + dx),
                                            (s16)(*(u16 *)(e + 0x6A) - 0x20));
    tiles[3] = EntityApplyMovementCallbacks(e, (s16)(*(u16 *)(e + 0x68) + dx),
                                            (s16)(*(u16 *)(e + 0x6A) - 0x30));
    CheckTileCollisionOverride(e, &tiles[0]);
    CheckTileCollisionOverride(e, &tiles[1]);
    CheckTileCollisionOverride(e, &tiles[2]);
    CheckTileCollisionOverride(e, &tiles[3]);

    return tiles[0] != PLAYER_TILE_PASSABLE && tiles[1] != PLAYER_TILE_PASSABLE &&
           tiles[2] != PLAYER_TILE_PASSABLE && tiles[3] != PLAYER_TILE_PASSABLE;
}
