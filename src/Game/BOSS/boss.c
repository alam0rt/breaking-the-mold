#include "common.h"
#include "globals.h"

extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void DestroyEntityAndFreeMemory(void *entity, s32 mode);
extern u8 D_800116E8[];
extern u8 D_80011708[];
extern u8 EntityApplyMovementCallbacks(void *entity, s16 x, s16 y);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CircularPlatformUpdatePath);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", InitCircularPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", ClayballTickWithParticleSpawn);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CircularPlatformEventHandler);

/* CircularPlatformUpdateWithMirror @ 8005894C — arg ordering and s16 arithmetic diffs */
INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CircularPlatformUpdateWithMirror);

extern void *InitEntitySprite(void *entity, void *def, s32 spriteId, s16 x, s16 y, s32 unused);
extern void GenericSpriteEntityInitCallback(void *entity, u16 param, u8 flags, s32 zero);
extern void ClayballResetState(void *entity);
extern u8 D_80011648[];

/* InitClayballWithSwitchBlock @ 80058A00 — 2-instr load order swap (lh/lhu) */
INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", InitClayballWithSwitchBlock);

/* ClayballSwitchEventHandler @ 80058AAC — switch case layout diff;
 * cc1 puts stores in j delay slots, GCC doesn't reproduce */
INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", ClayballSwitchEventHandler);

extern void ClayballSpawnSwitchBlock(void *entity);

s32 ClayballSpawnOnSignalHandler(void *entity, u16 event, u32 param, s32 arg3) {
    if (event == 0x1014) {
        u8 *sub = *(u8 **)((u8 *)entity + 0x100);
        if (param == sub[0x15]) {
            *(s32 *)((u8 *)entity + 0x128) = arg3;
            ClayballSpawnSwitchBlock(entity);
        }
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", ClayballResetState);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", ClayballSpawnSwitchBlock);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", InitBonusClayballEntity);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", ShrineyGuardSoundUpdateTick);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", ShrineyGuardEventWithSound);

extern u8 D_80011628[];

void ShrineyGuardDestroyWithSoundCleanup(void *entity, s32 flags) {
    u8 *e = (u8 *)entity;
    *(u32 *)(e + 0x18) = (u32)D_80011628;
    StopSPUVoice(*(s32 *)(e + 0x128));
    *(s32 *)(e + 0x128) = -1;
    StopSPUVoice(*(s32 *)(e + 0x12C));
    *(s32 *)(e + 0x12C) = -1;
    *(u32 *)(e + 0x18) = (u32)D_800116E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800116E8(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_800116E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800116E8_800594a0(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_800116E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800116E8_80059504(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_800116E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800116E8_80059568(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_800116E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt800116E8_800595cc(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_800116E8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_80059630(void) {
}

void func_80059638(void) {
}

void EntityDestructor_Simple11(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_80011708;
    if (flags & 1) {
        FreeEntityNoTeardown_80059674(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80059674(void *entity, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CreatePlayerEntity);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", InitPlayerSpriteAvailability);

extern void StopSPUVoice(s32 voice);
extern u8 D_80011804[];

void EntityDestructor_WithSPUVoiceStop(void *entity, s32 flags) {
    u8 *e = (u8 *)entity;
    *(u32 *)(e + 0x18) = (u32)D_80011804;
    StopSPUVoice(*(s32 *)(e + 0x174));
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CheckWallCollision);

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

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", TransformYCoordinateWithScale);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CalculateScaledXCoord);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", TransformXCoordinateWithScale);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", CalculateScaledYCoord);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", PlayerApplyPositionWithCollision);

INCLUDE_ASM("asm/nonmatchings/Game/BOSS/boss", IsEntityNearSoundTrigger);

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
