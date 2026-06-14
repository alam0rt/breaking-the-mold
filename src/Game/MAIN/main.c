#include "common.h"
#include "Game/game_state.h"
#include "globals.h"

extern s32 GetLevelFlags();
extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void PlaySoundEffect(u32 soundId, s32 volume, s32 param);
extern void HidePauseMenuHUD(s32 handle);
extern u8 D_80012120[];


INCLUDE_ASM("asm/nonmatchings/Game/MAIN/main", CheckCheatCodeInput);

void func_800826C0(u8 *obj, u8 *outA, u8 *outB, u8 *outC) {
    *outA = obj[0x199];
    *outB = obj[0x19A];
    *outC = obj[0x19B];
}

void func_800826E4(u8 *obj, u8 a, u8 b, u8 c) {
    obj[0x199] = a;
    obj[0x19A] = b;
    obj[0x19B] = c;
}

u8 func_800826F4(u8 *obj) {
    return obj[0x198];
}

void func_80082700(u8 *obj, u8 val) {
    obj[0x198] = val;
}

extern u8 D_800A608C[];
extern s32 FindSaveSlotByName(u8 *name, u8 *slots);

s32 FindSaveSlotForCurrentLevel(u8 *obj) {
    return FindSaveSlotByName(&obj[0x84], D_800A608C);
}

void func_80082730(u8 *obj, u8 a, u8 b) {
    obj[0x19C] = a;
    obj[0x19D] = b;
}

u8 func_8008273C(u8 *obj) {
    return obj[0x19C];
}

u8 GetLevelDebugFlag(GameState *gameState) {
    return (u16)GetLevelFlags(&gameState->level_context) >> 0xf;
}

u8 GetLevelShowHUDFlag(GameState *gameState) {
    return ((u16)GetLevelFlags(&gameState->level_context) >> 0xe) & 1;
}

u8 func_80082790(u8 *obj) {
    return obj[0x14A];
}

void func_8008279C(u8 *obj, u8 val) {
    obj[0x152] = val;
}

void func_800827A4(u8 *obj, u8 val) {
    obj[0x170] = val;
}

u8 func_800827AC(u8 *obj) {
    return obj[0x161];
}

void func_800827B8(u8 *obj, u8 val) {
    obj[0x161] = val;
}

void func_800827C0(u8 *obj, s32 val32, s16 val16) {
    *(s32 *)&obj[0x164] = val32;
    *(s16 *)&obj[0x168] = val16;
}

s32 func_800827CC(u8 *obj) {
    return *(s32 *)&obj[0x14C];
}

u8 func_800827D8(u8 *obj) {
    return obj[0x14B];
}

void func_800827E4(u8 *obj, u8 val) {
    obj[0x14B] = val;
}

void func_800827EC(u8 *obj, u8 val) {
    obj[0x144] = val;
}

u8 func_800827F4(u8 *obj) {
    return obj[0x149];
}

void func_80082800(u8 *obj, u8 val) {
    obj[0x149] = val;
}

void func_80082808(u8 *obj, u8 val) {
    obj[0x148] = val;
}

void func_80082810(u8 *obj, u8 val) {
    obj[0x146] = val;
}

u8 GetLevelAutoScrollFlag(GameState *gameState) {
    return ((u16)GetLevelFlags(&gameState->level_context) >> 0xb) & 1;
}

void func_8008283C(void) {
}

void func_80082844(void) {
}

void SpecialEntityDestroyCallback_2120(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)D_80012120;
    if (flags & 1) {
        FreeSpecialEntity2120Memory(entity, 0x1C);
    }
}

void FreeSpecialEntity2120Memory(void *ptr) {
    FreeFromHeap(g_pBlbHeapBase, ptr, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/Game/MAIN/main", main);

INCLUDE_ASM("asm/nonmatchings/Game/MAIN/main", func_80082BE8);

INCLUDE_ASM("asm/nonmatchings/Game/MAIN/main", ProcessDebugMenuInput);
