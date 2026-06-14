#include "common.h"
#include "Game/game_state.h"
#include "globals.h"

extern s32 GetLevelFlags();
extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void PlaySoundEffect(u32 soundId, s32 volume, s32 param);
extern void HidePauseMenuHUD(s32 handle);
extern u8 D_80012120[];

INCLUDE_ASM("asm/nonmatchings/lvlload", InitializeAndLoadLevel);

INCLUDE_ASM("asm/nonmatchings/lvlload", SetupAndStartLevel);

INCLUDE_ASM("asm/nonmatchings/lvlload", DisplayTransitionSprite);

INCLUDE_ASM("asm/nonmatchings/lvlload", SpawnPlayerAndEntities);

extern u8 D_80012100[];
extern void DestroyEntity(void *entity, s32 arg);

void DestroyEntityAndFreeResources(void *entity, s32 flags) {
    void *buckets = *(void **)((u8 *)entity + 0x16C);
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_80012100;
    if (buckets) {
        FreeFromHeap(g_pBlbHeapBase, buckets, 0, 0);
    }
    DestroyEntity(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/lvlload", GameModeCallback);

extern void AddToZOrderList(u8 *obj, s32 zOrder);

void SaveCheckpointState(void *entity) {
    *(u32 *)((u8 *)entity + 0x138) = *(u32 *)((u8 *)entity + 0x10C);
    *(u32 *)((u8 *)entity + 0x134) = *(u32 *)((u8 *)entity + 0x1C);
    *(u8 *)((u8 *)entity + 0x14A) = 1;
    *(u8 *)((u8 *)entity + 0x63) = 1;
    *(u32 *)((u8 *)entity + 0x1C) = 0;
    AddToZOrderList(entity, *(void **)((u8 *)entity + 0x2C));
}

INCLUDE_ASM("asm/nonmatchings/lvlload", RestoreCheckpointEntities);

s32 RemoveCheckpointEntityById(s32 gameState, s32 target) {
    s32 *node = *(s32 **)(gameState + 0x134);
    s32 *prev = 0;

    while (node != 0) {
        if (*(s32 *)((s32)node + 4) == target) {
            if (prev == 0) {
                *(s32 *)(gameState + 0x134) = *node;
            } else {
                *prev = *node;
            }
            FreeFromHeap(g_pBlbHeapBase, (void *)node, 8, 0);
            return 1;
        }
        prev = node;
        node = *(s32 **)(s32)node;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/lvlload", PauseGameAndShowMenu);

void PauseGameWithFadeOut(u8 *obj) {
    PlaySoundEffect(0x4C60F249, 0xA0, 1);
    obj[0x151] = 1;
    HidePauseMenuHUD(*(s32 *)&obj[0x14C]);
    obj[0x160] = 0x16;
}

void SetNextLevelFlagWithFade(u8 *obj) {
    obj[0x151] = 1;
    obj[0x160] = 0x16;
    obj[0x147] = 1;
}

extern void ResumeAllVoicePitches(void);
extern void ClearTickList(void *entity);

void UnpauseGameAndRestoreEntities(void *entity) {
    ResumeAllVoicePitches();
    *(u32 *)((u8 *)entity + 0x10C) = *(u32 *)((u8 *)entity + 0x154);
    *(u8 *)((u8 *)entity + 0x63) = *(u8 *)((u8 *)entity + 0x158);
    *(u8 *)((u8 *)entity + 0x151) = 0;
    *(u8 *)((u8 *)entity + 0x150) = 0;
    ClearTickList(entity);
    *(u32 *)((u8 *)entity + 0x1C) = *(u32 *)((u8 *)entity + 0x15C);
    *(u32 *)((u8 *)entity + 0x15C) = 0;
    if (*(u8 *)((u8 *)entity + 0x18D)) {
        AddToZOrderList(entity, *(void **)((u8 *)entity + 0x2C));
        *(u8 *)((u8 *)entity + 0x18D) = 0;
    }
}

extern void RemoveFromTickList(void *entity, void *arg);
extern void FreeEntityLists(void *entity);

void ClearEntitiesAndFadeToBlack(void *entity) {
    void *field2c = *(void **)((u8 *)entity + 0x2C);
    *(u8 *)((u8 *)entity + 0x145) = 1;
    *(u8 *)((u8 *)entity + 0x130) = 1;
    *(u8 *)((u8 *)entity + 0x131) = 0;
    *(u8 *)((u8 *)entity + 0x132) = 0;
    *(u8 *)((u8 *)entity + 0x133) = 0;
    RemoveFromTickList(entity, field2c);
    FreeEntityLists(entity);
    AddEntityToSortedRenderList(entity, *(void **)((u8 *)entity + 0x2C));
    *(u32 *)((u8 *)entity + 0x13C) = *(u32 *)((u8 *)entity + 0x28);
    *(u32 *)((u8 *)entity + 0x28) = 0;
}

INCLUDE_ASM("asm/nonmatchings/lvlload", HandleGenericTriggerZone);

INCLUDE_ASM("asm/nonmatchings/lvlload", CleanupRespawnEntities);
