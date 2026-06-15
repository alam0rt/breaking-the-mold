#include "common.h"

extern void *InitEntityWithTable(void *e);
extern void *D_80012100;

void *InitEntityWithTableAndSprite(void *e) {
    InitEntityWithTable(e);
    *(void **)((u8 *)e + 0x18) = &D_80012100;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/gstate", InitGameState);

INCLUDE_ASM("asm/nonmatchings/gstate", RespawnAfterDeath);

INCLUDE_ASM("asm/nonmatchings/gstate", StartLevelWithResetOrAdvance);

