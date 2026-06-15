#include "common.h"

extern void *g_pBlbHeapBase;
extern void FreeFromHeap(void *heap, void *ptr, s32 a2, s32 a3);

void FreeEntityNoTeardown_8002c7d8(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorEntity);

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorEntityWithSprite);

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorNoData);

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorNoDataWithSprite);

INCLUDE_ASM("asm/nonmatchings/decor", InitPathFollowingDecorEntity);

INCLUDE_ASM("asm/nonmatchings/decor", UpdateDecorEntityTriggerColors);

INCLUDE_ASM("asm/nonmatchings/decor", DecorEntityTickWithOffscreenCheck);

INCLUDE_ASM("asm/nonmatchings/decor", EntityCollisionHandler_SpecialTrigger);

INCLUDE_ASM("asm/nonmatchings/decor", EntityCollision_FlagAndDispatch);

INCLUDE_ASM("asm/nonmatchings/decor", EntityTick_PathFollowUpdate);

INCLUDE_ASM("asm/nonmatchings/decor", EntityTick_EasedMovementInterpolation);

INCLUDE_ASM("asm/nonmatchings/decor", InitDecorEntityWithScreenOffset);

INCLUDE_ASM("asm/nonmatchings/decor", SpawnDecorParticleEffect);

