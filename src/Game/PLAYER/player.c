#include "common.h"
#include "globals.h"

extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void DestroyEntityAndFreeMemory(void *entity, s32 mode);
extern void StopSPUVoice(s32 voice);
extern u8 EntityApplyMovementCallbacks(void *entity, s16 x, s16 y);
extern u8 D_80011804[];

s32 CheckTileCollisionOverride(void *entity, u8 *tile);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", CreatePlayerEntity);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", InitPlayerSpriteAvailability);

void EntityDestructor_WithSPUVoiceStop(void *entity, s32 flags) {
    u8 *e = (u8 *)entity;
    *(u32 *)(e + 0x18) = (u32)D_80011804;
    StopSPUVoice(*(s32 *)(e + 0x174));
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", CheckWallCollision);

s32 CheckCollisionAbove40(void *entity) {
    u8 tile;
    s16 x = *(s16 *)((u8 *)entity + 0x68);
    s16 y = (s16)(*(u16 *)((u8 *)entity + 0x6A) - 0x40);
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride(entity, &tile);
    return tile != 0x65;
}

s32 CheckCollisionAbove41(void *entity) {
    u8 tile;
    s16 x = *(s16 *)((u8 *)entity + 0x68);
    s16 y = (s16)(*(u16 *)((u8 *)entity + 0x6A) - 0x41);
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride(entity, &tile);
    return tile == 0x7D;
}

s32 CheckCollisionBelow1(void *entity) {
    u8 tile;
    s16 x = *(s16 *)((u8 *)entity + 0x68);
    s16 y = (s16)(*(u16 *)((u8 *)entity + 0x6A) + 1);
    tile = EntityApplyMovementCallbacks(entity, x, y);
    CheckTileCollisionOverride(entity, &tile);
    return tile == 0x7D;
}

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", TransformYCoordinateWithScale);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", CalculateScaledXCoord);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", TransformXCoordinateWithScale);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", CalculateScaledYCoord);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", PlayerApplyPositionWithCollision);

INCLUDE_ASM("asm/nonmatchings/Game/PLAYER/player", IsEntityNearSoundTrigger);

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
