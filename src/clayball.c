#include "common.h"
#include "globals.h"

extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void DestroyEntityAndFreeMemory(void *entity, s32 mode);
extern u8 D_800116E8[];
extern u8 D_80011708[];
extern u8 EntityApplyMovementCallbacks(void *entity, s16 x, s16 y);

INCLUDE_ASM("asm/nonmatchings/clayball", CircularPlatformUpdatePath);

INCLUDE_ASM("asm/nonmatchings/clayball", InitCircularPlatformEntity);

INCLUDE_ASM("asm/nonmatchings/clayball", ClayballTickWithParticleSpawn);

INCLUDE_ASM("asm/nonmatchings/clayball", CircularPlatformEventHandler);

/* CircularPlatformUpdateWithMirror @ 8005894C — arg ordering and s16 arithmetic diffs */
INCLUDE_ASM("asm/nonmatchings/clayball", CircularPlatformUpdateWithMirror);

extern void *InitEntitySprite(void *entity, void *def, s32 spriteId, s16 x, s16 y, s32 unused);
extern void GenericSpriteEntityInitCallback(void *entity, u16 param, u8 flags, s32 zero);
extern void ClayballResetState(void *entity);
extern u8 D_80011648[];

void *InitClayballWithSwitchBlock(void *entity, u8 *def, void *spawnContext, u8 flags) {
    InitEntitySprite(entity, spawnContext, 0x3C0, *(s16 *)(def + 0x8), (s16)(*(u16 *)(def + 0xA) - 1), 0);
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_800116E8;
    *(void **)((u8 *)entity + 0x100) = def;
    GenericSpriteEntityInitCallback(entity, *(u16 *)(def + 0xC), flags, 0);
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_80011648;
    *(u8 *)((u8 *)entity + 0x11A) = 0;
    *(s32 *)((u8 *)entity + 0x124) = 0;
    ClayballResetState(entity);
    return entity;
}

/* ClayballSwitchEventHandler @ 80058AAC — switch case layout diff;
 * cc1 puts stores in j delay slots, GCC doesn't reproduce */
INCLUDE_ASM("asm/nonmatchings/clayball", ClayballSwitchEventHandler);

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

INCLUDE_ASM("asm/nonmatchings/clayball", ClayballResetState);

INCLUDE_ASM("asm/nonmatchings/clayball", ClayballSpawnSwitchBlock);

INCLUDE_ASM("asm/nonmatchings/clayball", InitBonusClayballEntity);

INCLUDE_ASM("asm/nonmatchings/clayball", ShrineyGuardSoundUpdateTick);

INCLUDE_ASM("asm/nonmatchings/clayball", ShrineyGuardEventWithSound);

extern u8 D_80011628[];
extern void StopSPUVoice(s32 voice);
extern void FreeEntityNoTeardown_80059674(void *entity, s32 size);

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
