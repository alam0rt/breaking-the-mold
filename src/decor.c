#include "common.h"
#include "functions.h"
#include "Game/decor_entities.h"

extern u8 *g_pBlbHeapBase;
extern u8 PATH_DECOR_COLLISION_VTABLE[] asm("D_80010870");
extern SpriteEntity *InitEntityWithSprite(SpriteEntity *entity, u8 *spriteDef, s32 z, s16 x, s16 y);
extern SpriteEntity *InitEntitySprite(SpriteEntity *entity, u8 *def, s32 z, s16 x, s16 y, s32 flags);
extern void InitPathFollowingDecorEntity(TimedPathEntity *e, DecorSpawnData *data, u8 flag);

void FreeEntityNoTeardown_8002c7d8(u8 *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

TimedPathEntity *InitPathDecorEntity(TimedPathEntity *e, DecorSpawnData *data, u8 *spriteDef, u32 mask, u8 flag) {
    InitEntitySprite(&e->sprite, spriteDef, 0x3DE, data->x, data->y, mask & 0xFF);
    e->sprite.base.collisionVtable = PATH_DECOR_COLLISION_VTABLE;
    InitPathFollowingDecorEntity(e, data, flag);
    return e;
}

TimedPathEntity *InitPathDecorEntityWithSprite(TimedPathEntity *e, DecorSpawnData *data, u8 *spriteDef, u8 flag) {
    InitEntityWithSprite(&e->sprite, spriteDef, 0x3DE, data->x, data->y);
    e->sprite.base.collisionVtable = PATH_DECOR_COLLISION_VTABLE;
    InitPathFollowingDecorEntity(e, data, flag);
    return e;
}

TimedPathEntity *InitPathDecorNoData(TimedPathEntity *e, s32 x, s32 y, u8 *spriteDef, u8 flag) {
    InitEntitySprite(&e->sprite, spriteDef, 0x3DE, (s16)x, (s16)y, flag);
    e->sprite.base.collisionVtable = PATH_DECOR_COLLISION_VTABLE;
    InitPathFollowingDecorEntity(e, NULL, 1);
    return e;
}

TimedPathEntity *InitPathDecorNoDataWithSprite(TimedPathEntity *e, s32 x, s32 y, u8 *spriteDef) {
    InitEntityWithSprite(&e->sprite, spriteDef, 0x3DE, (s16)x, (s16)y);
    e->sprite.base.collisionVtable = PATH_DECOR_COLLISION_VTABLE;
    InitPathFollowingDecorEntity(e, NULL, 1);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/decor", InitPathFollowingDecorEntity);

INCLUDE_ASM("asm/nonmatchings/decor", UpdateDecorEntityTriggerColors);

INCLUDE_ASM("asm/nonmatchings/decor", DecorEntityTickWithOffscreenCheck);

Entity *EntityCollisionHandler_SpecialTrigger(SpecialTriggerEntity *e, u16 event) {
    Entity *r = NULL;
    if ((event & 0xFFFF) == EVT_SET_STATE_FLAG) {
        r = &e->sprite.base;
        e->triggered = 1;
        e->sprite.base.collisionMask = 0;
    }
    return r;
}

/* Two-bit collision dispatcher: on event 0x1016 marks the entity (+0x11D = 1,
 * clears the +0x12 word) and on event 2 forwards to EntityProcessCallbackQueue.
 * Returns the marked entity (only if 0x1016 fired) for chained handlers. */
Entity *EntityCollision_FlagAndDispatch(SpecialTriggerEntity *e, u32 event) {
    Entity *r = NULL;
    event &= 0xFFFF;
    if (event == EVT_SET_STATE_FLAG) {
        e->triggered = 1;
        e->sprite.base.collisionMask = 0;
        r = &e->sprite.base;
    }
    if (event == EVT_TICK) {
        EntityProcessCallbackQueue(&e->sprite.base);
    }
    return r;
}

extern void InterpolateTimedPathPosition(u16 *time, s16 *out, u8 *pathData, s16 duration, s32 unused);

void EntityTick_PathFollowUpdate(TimedPathEntity *e) {
    s16 out[2];
    e->pathTime += 1;
    InterpolateTimedPathPosition(
        &e->pathTime,
        out,
        e->pathData,
        e->pathDuration,
        8
    );
    e->sprite.base.worldX = e->pathOriginX + (u16)out[0];
    e->sprite.base.worldY = e->pathOriginY + (u16)out[1];
}

INCLUDE_ASM("asm/nonmatchings/decor", EntityTick_EasedMovementInterpolation);

INCLUDE_ASM("asm/nonmatchings/decor", InitDecorEntityWithScreenOffset);

INCLUDE_ASM("asm/nonmatchings/decor", SpawnDecorParticleEffect);

/* .data migration (Phase 4): misaligned island; array holds the 2-byte
   alignment pad at 0x8009B240, real table symbol aliased at +2. */
u8 D_8009B240[132] asm("D_8009B240") = {
    0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0xFF, 0xFF,
    0x06, 0x00, 0xFF, 0xFF, 0x0A, 0x00, 0x00, 0x00,
    0x0F, 0x00, 0x02, 0x00, 0x15, 0x00, 0x06, 0x00,
    0x19, 0x00, 0x0D, 0x00, 0x1B, 0x00, 0x18, 0x00,
    0x17, 0x00, 0x24, 0x00, 0x0F, 0x00, 0x2D, 0x00,
    0x03, 0x00, 0x32, 0x00, 0xF6, 0xFF, 0x33, 0x00,
    0xE8, 0xFF, 0x30, 0x00, 0xDB, 0xFF, 0x2B, 0x00,
    0xD0, 0xFF, 0x24, 0x00, 0xC3, 0xFF, 0x1B, 0x00,
    0xBA, 0xFF, 0x11, 0x00, 0xB0, 0xFF, 0x05, 0x00,
    0xA7, 0xFF, 0xFA, 0xFF, 0x9D, 0xFF, 0xEA, 0xFF,
    0x96, 0xFF, 0xDC, 0xFF, 0x8F, 0xFF, 0xCB, 0xFF,
    0x89, 0xFF, 0xBB, 0xFF, 0x85, 0xFF, 0xAB, 0xFF,
    0x82, 0xFF, 0x9B, 0xFF, 0x81, 0xFF, 0x94, 0xFF,
    0x81, 0xFF, 0x8D, 0xFF, 0x80, 0xFF, 0x89, 0xFF,
    0x80, 0xFF, 0x86, 0xFF, 0x80, 0xFF, 0x84, 0xFF,
    0x80, 0xFF, 0x82, 0xFF, 0x80, 0xFF, 0x81, 0xFF,
    0x80, 0xFF, 0x80, 0xFF,
};
asm(".globl D_8009B242\n.set D_8009B242, D_8009B240 + 2\n");
