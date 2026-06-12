#include "common.h"

extern u8 g_GameStateBase[];
extern void func_8007E5D0(void *entity, s32 flags);
extern void func_8007CCFC(void *entity);

void func_80082E90(void) {
    func_8007E5D0(g_GameStateBase, 2);
}

void func_80082EB8(void) {
    func_8007CCFC(g_GameStateBase);
}
