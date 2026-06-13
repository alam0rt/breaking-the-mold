#include "common.h"

extern u8 g_GameStateBase[];
extern void DestroyEntityAndFreeResources(void *entity, s32 flags);
extern void InitEntityWithTableAndSprite(void *entity);

void DestroyStaticGameState(void) {
    DestroyEntityAndFreeResources(g_GameStateBase, 2);
}

void CRT_ConstructGameState(void) {
    InitEntityWithTableAndSprite(g_GameStateBase);
}
