#include "common.h"

typedef struct AnimEntity {
    u8 pad00[0xC0];
    /* 0xC0 */ u32 pendingFrame;
    /* 0xC4 */ u32 pendingLoopFrame;
    /* 0xC8 */ u32 pendingSpriteSource;
    u8 padCC[0xE0 - 0xCC];
    /* 0xE0 */ u16 animChangeFlags;
    u8 padE2[0xF5 - 0xE2];
    /* 0xF5 */ u8 pendingAnimActive;
} AnimEntity;

void SetAnimationFrameIndex(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags;

    entity->pendingFrame = (u16)value;
    newFlags = (flags | 0x8) & 0xFDFF;
    entity->animChangeFlags = newFlags;
    if (((flags | 0x8) & 3) == 0) {
        entity->animChangeFlags = newFlags | 0x1;
    }
}

void SetAnimationFrameCallback(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x208;

    entity->pendingFrame = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x209;
    }
}

void SetEntityTargetFrame(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 orFlags = flags | 0x10;
    u16 newFlags = orFlags & 0xFBFF;

    entity->pendingLoopFrame = (u16)value;
    entity->animChangeFlags = newFlags;
    if (orFlags & 0x4) {
        u16 flags2 = *(volatile u16 *)&entity->animChangeFlags;
        u32 loopVal = entity->pendingLoopFrame;
        entity->pendingFrame = loopVal;
        entity->animChangeFlags = (flags2 | 0x8) & 0xFDFF;
    }
    {
        u16 flags3 = entity->animChangeFlags;
        if ((flags3 & 3) == 0) {
            entity->animChangeFlags = flags3 | 0x1;
        }
    }
}

void SetAnimationLoopFrame(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;

    entity->pendingLoopFrame = value;
    flags |= 0x410;
    entity->animChangeFlags = flags;
    if (flags & 0x4) {
        u16 flags2 = *(volatile u16 *)&entity->animChangeFlags;
        u32 loopVal = entity->pendingLoopFrame;
        entity->pendingFrame = loopVal;
        entity->animChangeFlags = flags2 | 0x208;
    }
    {
        u16 flags3 = entity->animChangeFlags;
        if ((flags3 & 3) == 0) {
            entity->animChangeFlags = flags3 | 0x1;
        }
    }
}

void SetAnimationSpriteId(AnimEntity *entity, u32 spriteId) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags;

    entity->pendingSpriteSource = (u16)spriteId;
    newFlags = (flags | 0x20) & 0xF7FF;
    entity->animChangeFlags = newFlags;
    if (((flags | 0x20) & 3) == 0) {
        entity->animChangeFlags = newFlags | 0x1;
    }
}

void SetAnimationSpriteCallback(AnimEntity *entity, void *callback) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x820;

    entity->pendingSpriteSource = (u32)callback;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x821;
    }
}

void SetAnimationActive(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x100;

    entity->pendingAnimActive = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x101;
    }
}

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntitySetRenderFlags);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8001D268);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", TickEntityAnimation);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", AdvanceAnimationFrame);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", ApplyPendingSpriteState);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UpdateSpriteFrameData);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UpdateEntityRender);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UploadEntityTextureIfDirty);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", FindFrameIndexByValue);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", StartAnimationSequence);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", StepAnimationSequence);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityProcessCallbackQueue);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntitySetState);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntitySetCallback);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InitLayerRenderContext_Standard);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityDestructor_FreeMultiAlloc);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UpdateParallaxScrollWithWrap_Standard);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InitLayerRenderContext_Medium);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityDestructor_FreeResourceType2);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UpdateParallaxScrollWithWrap_Medium);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InitLayerRenderContext_Small);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityDestructor_FreeResourceType3);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UpdateParallaxScrollWithWrap_Small);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InitLayerScrollContext);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", FreeResourceType4);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityTick_AnimationFrameAdvance);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8001FC40);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", UpdateEntityScreenPositionWithPalette);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InitPlayerEntity);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityApplyMovementCallbacks);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityDestructor_FreeWithChildRef);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800200DC);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800200E8);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800200F0);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800200F8);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020100);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020108);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020110);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020118);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020124);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002012C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020134);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002013C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020144);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002014C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020154);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020160);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020168);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020170);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020178);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020180);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020188);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020190);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002019C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800201A8);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800201B0);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800201D0);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800201DC);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800201E8);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800201F4);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020200);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020208);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020214);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020220);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020228);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020230);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002023C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020248);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020254);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002025C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020264);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020270);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002027C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", StubReturnZero);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020290);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_8002029C);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800202A4);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800202AC);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800203AC);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800203E8);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020424);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", GetEntityYPosition);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", GetEntityXPosition);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020588);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020594);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800205A0);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800205AC);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", TransformYCoord);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020668);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", TransformXCoord);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_80020724);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InvokeEntityRenderCallback);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", func_800207C8);

void func_800207D4(void) {
}

void func_800207DC(void) {
}

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityDestructor_Simple);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", FreeEntityNoTeardown_80020818);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", BLB_ReadSectorsWrapper);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", InitEntityWithTable);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", LoadBLBHeader);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", DestroyEntity);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", RemoveFromUpdateQueue);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", RemoveFromZOrderList);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityTickLoopWithCamera);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", DeferredEntityRemoval);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityRemoval);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityTickLoop);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", RenderEntities);

INCLUDE_ASM("asm/nonmatchings/entity/animation_setters", EntityDestructCallback);
