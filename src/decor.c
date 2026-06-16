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

/* Two-bit collision dispatcher: on event 0x1016 marks the entity (+0x11D = 1,
 * clears the +0x12 word) and on event 2 forwards to EntityProcessCallbackQueue.
 * Returns the marked entity (only if 0x1016 fired) for chained handlers. */
void *EntityCollision_FlagAndDispatch(Entity *e, u32 event) {
    void *r = NULL;
    event &= 0xFFFF;
    if (event == 0x1016) {
        *((u8 *)e + 0x11D) = 1;
        *(s16 *)((u8 *)e + 0x12) = 0;
        r = e;
    }
    if (event == 2) {
        EntityProcessCallbackQueue(e);
    }
    return r;
}

extern void InterpolateTimedPathPosition(void *time, s16 *out, void *pathData, s16 duration, s32 unused);

void EntityTick_PathFollowUpdate(u8 *e) {
    s16 out[2];
    *(u16 *)(e + 0x10E) += 1;
    InterpolateTimedPathPosition(
        e + 0x10E,
        out,
        *(void **)(e + 0x108),
        *(s16 *)(e + 0x10C),
        8
    );
    *(s16 *)(e + 0x68) = *(u16 *)(e + 0x104) + (u16)out[0];
    *(s16 *)(e + 0x6A) = *(u16 *)(e + 0x106) + (u16)out[1];
}

INCLUDE_ASM("asm/nonmatchings/decor", EntityTick_EasedMovementInterpolation);

INCLUDE_ASM("asm/nonmatchings/decor", InitDecorEntityWithScreenOffset);

INCLUDE_ASM("asm/nonmatchings/decor", SpawnDecorParticleEffect);

