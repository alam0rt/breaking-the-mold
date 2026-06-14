#include "common.h"
#include "Game/entity.h"
#include "Game/game_state.h"
#include "globals.h"

extern void *AllocateFromHeap(void *heap, s32 align, s32 size, s32 flags);
extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void InitEntityStruct(Entity *entity, s16 allocSize);
extern void ClearSpriteContextWrapper(void *ctx);
extern void ZeroEntityField(void *field);
extern void InitEntityAnimationState(SpriteEntity *entity);
extern void CalculateEntityScreenBounds(Entity *entity);

INCLUDE_ASM("asm/nonmatchings/entity", InitEntityStruct);

void FreeWithCallback(Entity *entity, s32 flags) {
    void *ctx = entity->spriteContext;
    entity->collisionVtable = g_EntityVtable_PartialDestroy;
    if (ctx != NULL) {
        void *sub = ((void **)ctx)[3]; /* ctx + 0xC */
        s16 offset = *(s16 *)((u8 *)sub + 0x10);
        void (*func)(void *, s32) = *(void (**)(void *, s32))((u8 *)sub + 0x14);
        func((void *)((u8 *)ctx + offset), 3);
    }
    entity->collisionVtable = g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

s32 ScaleXByEntityScale(Entity *entity, s16 value) {
    if (entity->scalePowerupX == 0x10000) {
        return (s32)value;
    }
    return (s32)value * entity->scalePowerupX >> 16;
}

s32 ScaleYByEntityScale(Entity *entity, s16 value) {
    if (entity->scalePowerupY == 0x10000) {
        return (s32)value;
    }
    return (s32)value * entity->scalePowerupY >> 16;
}

s32 GetWorldPositionX(Entity *entity, s16 localX) {
    return (s32)localX + (s32)g_pGameState->camera_x;
}

s32 GetWorldPositionY(Entity *entity, s16 localY) {
    return (s32)localY + (s32)g_pGameState->camera_y;
}

INCLUDE_ASM("asm/nonmatchings/entity", CalculateParallaxXOffset);

INCLUDE_ASM("asm/nonmatchings/entity", CalculateParallaxXOffsetAlt);

INCLUDE_ASM("asm/nonmatchings/entity", WorldToScreenX);

s32 WorldToScreenYWithParallax(Entity *entity, s16 value) {
    if (entity->scalePowerupY == 0x10000) {
        return (s32)value + (s32)g_pGameState->camera_y;
    }
    return ((s32)value * entity->scalePowerupY >> 16) + (s32)g_pGameState->camera_y;
}

extern void *PrepareSpriteVRAMSlotForContext(void *ctx, s16 width, s16 height, s16 depth, u8 flags);

void CreateEntityRenderContext(Entity *entity, s16 width, s16 height, s16 depth, u8 flags) {
    void *ctx = AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    entity->spriteContext = PrepareSpriteVRAMSlotForContext(ctx, width, (s16)((height + 3) & 0xFFFCu), depth, flags);
}

void GetEntityScreenBounds(Entity *entity, s16 *out) {
    CalculateEntityScreenBounds(entity);
    __builtin_memcpy(out, &entity->screenX1, 8);
}

INCLUDE_ASM("asm/nonmatchings/entity", CalculateEntityScreenBounds);

INCLUDE_ASM("asm/nonmatchings/entity", CalculateEntityRenderBounds);

void SetEntityFacingDirection(Entity *entity, u8 direction) {
    if (direction == 2) {
        direction = entity->facing == 0;
    }
    entity->facing = direction;
    entity->textureDirty = 1;
}

void func_8001AAE4(Entity *entity, u8 direction) {
    if (direction == 2) {
        direction = entity->flipY == 0;
    }
    entity->flipY = direction;
    entity->textureDirty = 1;
}

INCLUDE_ASM("asm/nonmatchings/entity", UpdateParallaxLayerPosition);

void func_8001B344(Entity *entity) {
    if (entity->textureDirty) {
        entity->textureDirty = 0;
    }
}

s32 CheckPointInBox(Entity *entity, s16 pointX, s16 pointY) {
    CalculateEntityScreenBounds(entity);
    if (pointX < entity->screenX1) goto out;
    if (entity->screenX2 < pointX) goto out;
    if (pointY < entity->screenY1) goto out;
    if (entity->screenY2 < pointY) goto out;
    return 1;
out:
    return 0;
}

typedef struct { s16 x; s16 y; } BoxCorner;

s32 CheckBoxOverlap(Entity *entity, BoxCorner minCorner, BoxCorner maxCorner) {
    CalculateEntityScreenBounds(entity);
    if (entity->screenX1 > maxCorner.x) return 0;
    if (entity->screenX2 < minCorner.x) return 0;
    if (entity->screenY1 > maxCorner.y) return 0;
    if (entity->screenY2 < minCorner.y) return 0;
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/entity", CollisionCheckWrapper);

INCLUDE_ASM("asm/nonmatchings/entity", EntityBroadcastBoxCollision);

INCLUDE_ASM("asm/nonmatchings/entity", CheckEntityPointCollisionWithOffsets);

INCLUDE_ASM("asm/nonmatchings/entity", EntityBroadcastPointCollision);

INCLUDE_ASM("asm/nonmatchings/entity", CheckEntityBoxCollision);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffScreen);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenLeft);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenLeftSimple);

INCLUDE_ASM("asm/nonmatchings/entity", IsPositionOffscreenLeft);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenRight);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenRightSimple);

INCLUDE_ASM("asm/nonmatchings/entity", IsPositionOffscreenRight);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffScreenY);

INCLUDE_ASM("asm/nonmatchings/entity", SetupEntityScaleCallbacks);

INCLUDE_ASM("asm/nonmatchings/entity", PlayEntityPositionSound);

INCLUDE_ASM("asm/nonmatchings/entity", UpdateEntitySoundPanning);

SpriteEntity *InitFullEntityWithAnimation(SpriteEntity *entity, s16 allocSize) {
    InitEntityStruct(&entity->base, allocSize);
    entity->base.collisionVtable = g_EntityVtable_SpriteBase;
    ClearSpriteContextWrapper(&entity->base.pFrameTable);
    ZeroEntityField(&entity->pFrameData);
    InitEntityAnimationState(entity);
    return entity;
}

INCLUDE_ASM("asm/nonmatchings/entity", InitEntitySprite);

INCLUDE_ASM("asm/nonmatchings/entity", InitEntityWithSprite);

INCLUDE_ASM("asm/nonmatchings/entity", InitEntityAnimationState);

void DestroyEntityAndFreeMemory(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = g_EntityVtable_SpriteBase;
    if (entity->pPixelBuffer != NULL) {
        FreeFromHeap(g_pBlbHeapBase, entity->pPixelBuffer, 0, 0);
    }
    FreeFromHeap(g_pBlbHeapBase, entity->pSpriteAsset, 4, 0);
    {
        void *ctx = entity->base.spriteContext;
        entity->base.collisionVtable = g_EntityVtable_PartialDestroy;
        if (ctx != NULL) {
            void *sub = ((void **)ctx)[3];
            s16 offset = *(s16 *)((u8 *)sub + 0x10);
            void (*func)(void *, s32) = *(void (**)(void *, s32))((u8 *)sub + 0x14);
            func((void *)((u8 *)ctx + offset), 3);
        }
    }
    entity->base.collisionVtable = g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void AllocateEntityPixelBuffer(SpriteEntity *entity) {
    void *heap = g_pBlbHeapBase;
    s16 *ctx = (s16 *)entity->base.spriteContext;
    s32 size = (s32)ctx[2] * (s32)ctx[3];
    entity->pixelBufferSize = size;
    entity->pPixelBuffer = AllocateFromHeap(heap, 1, size, 0);
}

extern void TickEntityAnimation(SpriteEntity *entity);
extern void ApplyPendingSpriteState(SpriteEntity *entity);
extern void UpdateSpriteFrameData(SpriteEntity *entity);

INCLUDE_ASM("asm/nonmatchings/entity", EntityUpdateCallback);

void ApplyAnimationPositionOffsets(SpriteEntity *entity) {
    if (entity->base.facing) {
        entity->base.worldX -= entity->frameDeltaX;
    } else {
        entity->base.worldX += entity->frameDeltaX;
    }
    if (entity->base.flipY) {
        entity->base.worldY -= entity->frameDeltaY;
    } else {
        entity->base.worldY += entity->frameDeltaY;
    }
    entity->frameDeltaX = 0;
    entity->frameDeltaY = 0;
}

void InitEntitySpriteAndPixelBuffer(SpriteEntity *entity, s16 width, s16 height, s16 depth, u8 flags) {
    typedef struct { u8 pad00[4]; s16 width; s16 height; } SpriteContext;
    void *ctx = AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    SpriteContext *sc;
    s32 size;
    entity->base.spriteContext = PrepareSpriteVRAMSlotForContext(ctx, width, (s16)((height + 3) & 0xFFFCu), depth, flags);
    sc = (SpriteContext *)entity->base.spriteContext;
    size = (s32)sc->width * (s32)sc->height;
    entity->pixelBufferSize = size;
    entity->pPixelBuffer = AllocateFromHeap(g_pBlbHeapBase, 1, size, 0);
}

INCLUDE_ASM("asm/nonmatchings/entity", AllocSpriteRenderContext);

INCLUDE_ASM("asm/nonmatchings/entity", CreateMultiFrameRenderContext);

extern void *InitSpriteContextDefaults(void *ctx, s16 spriteId);

void AllocateSpriteContext(Entity *entity, s16 spriteId) {
    void *ctx = AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    entity->spriteContext = InitSpriteContextDefaults(ctx, spriteId);
}
