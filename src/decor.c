#include "common.h"
#include "functions.h"

extern void *g_pBlbHeapBase;
extern void *D_80010870;
extern void *InitEntityWithSprite(void *entity, void *spriteDef, s32 z, s16 x, s16 y);
extern void *InitEntitySprite(void *entity, void *def, s32 z, s16 x, s16 y, s32 flags);
extern void InitPathFollowingDecorEntity(void *e, void *data, u8 flag);

void FreeEntityNoTeardown_8002c7d8(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

void *InitPathDecorEntity(void *e, u8 *data, void *spriteDef, u32 mask, u8 flag) {
    InitEntitySprite(e, spriteDef, 0x3DE, *(s16 *)(data + 8), *(s16 *)(data + 0xA), mask & 0xFF);
    *(void **)((u8 *)e + 0x18) = &D_80010870;
    InitPathFollowingDecorEntity(e, data, flag);
    return e;
}

void *InitPathDecorEntityWithSprite(void *e, u8 *data, void *spriteDef, u8 flag) {
    InitEntityWithSprite(e, spriteDef, 0x3DE, *(s16 *)(data + 8), *(s16 *)(data + 0xA));
    *(void **)((u8 *)e + 0x18) = &D_80010870;
    InitPathFollowingDecorEntity(e, data, flag);
    return e;
}

void *InitPathDecorNoData(void *e, s32 x, s32 y, void *spriteDef, u8 flag) {
    InitEntitySprite(e, spriteDef, 0x3DE, (s16)x, (s16)y, flag);
    *(void **)((u8 *)e + 0x18) = &D_80010870;
    InitPathFollowingDecorEntity(e, NULL, 1);
    return e;
}

void *InitPathDecorNoDataWithSprite(void *e, s32 x, s32 y, void *spriteDef) {
    InitEntityWithSprite(e, spriteDef, 0x3DE, (s16)x, (s16)y);
    *(void **)((u8 *)e + 0x18) = &D_80010870;
    InitPathFollowingDecorEntity(e, NULL, 1);
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

