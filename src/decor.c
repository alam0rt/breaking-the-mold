#include "common.h"
#include "functions.h"

extern void *g_pBlbHeapBase;
extern void *D_80010870;
extern void *InitEntityWithSprite(void *entity, void *spriteDef, s32 z, s16 x, s16 y);
extern void InitPathFollowingDecorEntity(void *e, u32 flags, u32 mode);

void FreeEntityNoTeardown_8002c7d8(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorEntity);

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorEntityWithSprite);

INCLUDE_ASM("asm/nonmatchings/decor", InitPathDecorNoData);

void *InitPathDecorNoDataWithSprite(void *e, s32 x, s32 y, void *spriteDef) {
    InitEntityWithSprite(e, spriteDef, 0x3DE, (s16)x, (s16)y);
    *(void **)((u8 *)e + 0x18) = &D_80010870;
    InitPathFollowingDecorEntity(e, 0, 1);
    return e;
}

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

