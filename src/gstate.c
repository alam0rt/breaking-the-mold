#include "common.h"
#include "Game/entity.h"
#include "Game/player_state.h"

extern Entity *InitEntityWithTable(Entity *e);
extern void *DEAD_ENTITY_VTABLE asm("D_80012100");

Entity *InitEntityWithTableAndSprite(Entity *e) {
    InitEntityWithTable(e);
    e->collisionVtable = &DEAD_ENTITY_VTABLE;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/gstate", InitGameState);

INCLUDE_ASM("asm/nonmatchings/gstate", RespawnAfterDeath);

extern void AdvanceLevelSequence(u8 *p);
extern void SeekToLevelInSequence(u8 *p, s32 a, s32 b, s32 c);
extern u8 SetupAndStartLevel(u8 *p, s32 mode);
extern void initPlayerState(PlayerState *p);
extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");

void StartLevelWithResetOrAdvance(u8 *p, u32 mode) {
    if (mode & 0xFF) {
        AdvanceLevelSequence(p + 0x84);
    } else {
        SeekToLevelInSequence(p + 0x84, 0, 1, 0);
    }
    if (SetupAndStartLevel(p, 0x63) == 0) {
        initPlayerState(PLAYER_STATE_DATA);
    }
}

