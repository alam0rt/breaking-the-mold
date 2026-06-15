#include "common.h"
#include "functions.h"

extern void *g_pBlbHeapBase;

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

void *EntityCollisionHandler_SpecialTrigger(void *e, u16 event) {
    void *r = NULL;
    if ((event & 0xFFFF) == 0x1016) {
        r = e;
        *(u8 *)((u8 *)e + 0x11D) = 1;
        *(s16 *)((u8 *)e + 0x12) = 0;
    }
    return r;
}

INCLUDE_ASM("asm/nonmatchings/decor", EntityCollision_FlagAndDispatch);

INCLUDE_ASM("asm/nonmatchings/decor", EntityTick_PathFollowUpdate);

INCLUDE_ASM("asm/nonmatchings/decor", EntityTick_EasedMovementInterpolation);

INCLUDE_ASM("asm/nonmatchings/decor", InitDecorEntityWithScreenOffset);

INCLUDE_ASM("asm/nonmatchings/decor", SpawnDecorParticleEffect);

