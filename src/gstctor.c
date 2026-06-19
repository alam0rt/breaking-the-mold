#include "common.h"
#include "Game/game_state.h"

extern GameState g_GameStateBase;
extern void DestroyEntityAndFreeResources(GameState *entity, s32 flags);
extern void InitEntityWithTableAndSprite(GameState *entity);

/*
 * DestroyStaticGameState @ 0x80082E90.
 * Tears down the global GameState entity at g_GameStateBase (0x8009DC40).
 * Passes flags=2 to DestroyEntityAndFreeResources, which runs the entity's
 * destroy callbacks but does NOT free the backing memory — correct here
 * because g_GameStateBase is a statically-allocated, fixed-address struct.
 */
void DestroyStaticGameState(void) {
    DestroyEntityAndFreeResources(&g_GameStateBase, 2);
}

/*
 * CRT_ConstructGameState @ 0x80082EB8.
 * One-shot constructor for the global GameState entity at g_GameStateBase.
 * Delegates to InitEntityWithTableAndSprite, which wires up the standard
 * entity vtable/sprite-id fields (full GameState init happens later in
 * InitGameState @ 0x8007CD34). The `CRT_` prefix is a guess at a C-runtime
 * style early init hook — likely should be ConstructStaticGameState.
 */
void CRT_ConstructGameState(void) {
    InitEntityWithTableAndSprite(&g_GameStateBase);
}
