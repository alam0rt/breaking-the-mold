#include "common.h"
#include "Game/entity.h"
#include "Game/game_state.h"

extern GameState *D_800A5960;
extern void *D_800A5954;
extern void *AllocateFromHeap(void *heap, s32 align, s32 size, s32 flags);
extern void FreeFromHeap(void *heap, void *ptr, s32 mode, s32 flags);
extern void InitEntityStruct(Entity *entity, s16 allocSize);
extern void ClearSpriteContextWrapper(void *ctx);
extern void ZeroEntityField(void *field);
extern void InitEntityAnimationState(SpriteEntity *entity);
extern u8 D_8001044C[];
extern u8 D_8001046C[];
extern u8 D_800104AC[];
extern void CalculateEntityScreenBounds(Entity *entity);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntityStruct);

void FreeWithCallback(Entity *entity, s32 flags) {
    void *ctx = entity->spriteContext;
    entity->collisionVtable = D_8001046C;
    if (ctx != NULL) {
        void *sub = ((void **)ctx)[3]; /* ctx + 0xC */
        s16 offset = *(s16 *)((u8 *)sub + 0x10);
        void (*func)(void *, s32) = *(void (**)(void *, s32))((u8 *)sub + 0x14);
        func((void *)((u8 *)ctx + offset), 3);
    }
    entity->collisionVtable = D_800104AC;
    if (flags & 1) {
        FreeFromHeap(D_800A5954, entity, 0, 0);
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
    return (s32)localX + (s32)D_800A5960->camera_x;
}

s32 GetWorldPositionY(Entity *entity, s16 localY) {
    return (s32)localY + (s32)D_800A5960->camera_y;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CalculateParallaxXOffset);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CalculateParallaxXOffsetAlt);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", WorldToScreenX);

s32 WorldToScreenYWithParallax(Entity *entity, s16 value) {
    if (entity->scalePowerupY == 0x10000) {
        return (s32)value + (s32)D_800A5960->camera_y;
    }
    return ((s32)value * entity->scalePowerupY >> 16) + (s32)D_800A5960->camera_y;
}

extern void *PrepareSpriteVRAMSlotForContext(void *ctx, s16 width, s16 height, s16 depth, u8 flags);

void CreateEntityRenderContext(Entity *entity, s16 width, s16 height, s16 depth, u8 flags) {
    void *ctx = AllocateFromHeap(D_800A5954, 0x3C, 1, 0);
    entity->spriteContext = PrepareSpriteVRAMSlotForContext(ctx, width, (s16)((height + 3) & 0xFFFCu), depth, flags);
}

void GetEntityScreenBounds(Entity *entity, s16 *out) {
    CalculateEntityScreenBounds(entity);
    __builtin_memcpy(out, &entity->screenX1, 8);
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CalculateEntityScreenBounds);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CalculateEntityRenderBounds);

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

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", UpdateParallaxLayerPosition);

void func_8001B344(Entity *entity) {
    if (entity->textureDirty) {
        entity->textureDirty = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CheckPointInBox);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CheckBoxOverlap);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CollisionCheckWrapper);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", EntityBroadcastBoxCollision);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CheckEntityPointCollisionWithOffsets);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", EntityBroadcastPointCollision);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CheckEntityBoxCollision);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsEntityOffScreen);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsEntityOffscreenLeft);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsEntityOffscreenLeftSimple);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsPositionOffscreenLeft);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsEntityOffscreenRight);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsEntityOffscreenRightSimple);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsPositionOffscreenRight);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", IsEntityOffScreenY);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", SetupEntityScaleCallbacks);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", PlayEntityPositionSound);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", UpdateEntitySoundPanning);

SpriteEntity *InitFullEntityWithAnimation(SpriteEntity *entity, s16 allocSize) {
    InitEntityStruct(&entity->base, allocSize);
    entity->base.collisionVtable = D_8001044C;
    ClearSpriteContextWrapper(&entity->base.pFrameTable);
    ZeroEntityField(&entity->pFrameData);
    InitEntityAnimationState(entity);
    return entity;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntitySprite);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntityWithSprite);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntityAnimationState);

void DestroyEntityAndFreeMemory(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = D_8001044C;
    if (entity->pPixelBuffer != NULL) {
        FreeFromHeap(D_800A5954, entity->pPixelBuffer, 0, 0);
    }
    FreeFromHeap(D_800A5954, entity->pSpriteAsset, 4, 0);
    {
        void *ctx = entity->base.spriteContext;
        entity->base.collisionVtable = D_8001046C;
        if (ctx != NULL) {
            void *sub = ((void **)ctx)[3];
            s16 offset = *(s16 *)((u8 *)sub + 0x10);
            void (*func)(void *, s32) = *(void (**)(void *, s32))((u8 *)sub + 0x14);
            func((void *)((u8 *)ctx + offset), 3);
        }
    }
    entity->base.collisionVtable = D_800104AC;
    if (flags & 1) {
        FreeFromHeap(D_800A5954, entity, 0, 0);
    }
}

void AllocateEntityPixelBuffer(SpriteEntity *entity) {
    void *heap = D_800A5954;
    s16 *ctx = (s16 *)entity->base.spriteContext;
    s32 size = (s32)ctx[2] * (s32)ctx[3];
    *(s32 *)&entity->_padD4[0] = size;
    entity->pPixelBuffer = AllocateFromHeap(heap, 1, size, 0);
}

extern void TickEntityAnimation(SpriteEntity *entity);
extern void ApplyPendingSpriteState(SpriteEntity *entity);
extern void UpdateSpriteFrameData(SpriteEntity *entity);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", EntityUpdateCallback);

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

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntitySpriteAndPixelBuffer);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", AllocSpriteRenderContext);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CreateMultiFrameRenderContext);

extern void *InitSpriteContextDefaults(void *ctx, s16 spriteId);

void AllocateSpriteContext(Entity *entity, s16 spriteId) {
    void *ctx = AllocateFromHeap(D_800A5954, 0x3C, 1, 0);
    entity->spriteContext = InitSpriteContextDefaults(ctx, spriteId);
}
