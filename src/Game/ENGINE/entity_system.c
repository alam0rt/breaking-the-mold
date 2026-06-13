#include "common.h"
#include "Game/entity.h"
#include "Game/game_state.h"

extern GameState *D_800A5960;

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntityStruct);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", FreeWithCallback);

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

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CreateEntityRenderContext);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", GetEntityScreenBounds);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CalculateEntityScreenBounds);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", CalculateEntityRenderBounds);

void SetEntityFacingDirection(Entity *entity, u8 direction) {
    if (direction == 2) {
        direction = entity->facing == 0;
    }
    entity->facing = direction;
    entity->textureDirty = 1;
}

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", func_8001AAE4);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", UpdateParallaxLayerPosition);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", func_8001B344);

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

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitFullEntityWithAnimation);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntitySprite);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntityWithSprite);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", InitEntityAnimationState);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", DestroyEntityAndFreeMemory);

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", AllocateEntityPixelBuffer);

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

INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/entity_system", AllocateSpriteContext);
