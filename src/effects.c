#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/entity_events.h"

extern void *g_pBlbHeapBase;
extern void FlushDepthBuckets(void *entity);
extern void *InitBasicEntityWithVtable(void *e, s16 val);
extern u8 IsEntityOutsideSpawnBounds(Entity *e);
extern void ChangeRenderZOrder(void *gs, void *layer, u32 zOrder);
extern void EntityTimerTickAndNotify(Entity *e);
extern void *D_80010A48;
extern void *D_80010A68;
extern void *D_80010AD8;
extern void *D_80010B28;
extern void *D_80010BA8;
extern void *D_80010BC8;
extern void *D_80010970;
extern void *D_800109F0;
extern void *D_80010A88;
extern void *D_80010A10;
extern void FreeResourceType4(void *e, s32 mode);
extern void FreeWithCallback(void *e, s32 mode);

INCLUDE_ASM("asm/nonmatchings/effects", InitParticleEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTimerTickAndNotify);

s32 DecorHandleCallback(void *e, u16 event) {
    u8 *p = (u8 *)e;
    if (event == EVT_TICK && *(u16 *)(p + 0x100) == 0) {
        p[0x102] = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", InitBlbOverlayEntity);

INCLUDE_ASM("asm/nonmatchings/effects", RenderRotatedTexturedQuad);

INCLUDE_ASM("asm/nonmatchings/effects", InitMenuSpriteEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyEntityWithData);

INCLUDE_ASM("asm/nonmatchings/effects", RandomizedEntityBehaviorTick);

INCLUDE_ASM("asm/nonmatchings/effects", InitVRAMSlotEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyVRAMSlotEntity);

INCLUDE_ASM("asm/nonmatchings/effects", RenderVRAMSlotOverlay);

INCLUDE_ASM("asm/nonmatchings/effects", CheckVRAMSlotPixelColor);

INCLUDE_ASM("asm/nonmatchings/effects", InitPasswordEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityDestructor_WithVRAMSlotFree);

INCLUDE_ASM("asm/nonmatchings/effects", MultiPartEntityTick);

INCLUDE_ASM("asm/nonmatchings/effects", EntityRenderCallbackUpdateScreenPos);

INCLUDE_ASM("asm/nonmatchings/effects", MultiPartEntityRenderTick);

INCLUDE_ASM("asm/nonmatchings/effects", InitPasswordSymbolEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTick_ParallaxScrollPosition);

u32 MultiPartEntityMessageHandler(void *e, s32 event) {
    switch (event & 0xFFFF) {
        case EVT_TOKEN_RELEASE:
            *(u8 *)((u8 *)e + 0x108) = 1;
            break;
        case 0x1010:
            *(u8 *)((u8 *)(*(void **)((u8 *)e + 0x34)) + 0xA) = 0;
            break;
        case EVT_ATTACH_TO_ENTITY:
            *(u8 *)((u8 *)(*(void **)((u8 *)e + 0x34)) + 0xA) = 1;
            break;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", InitHUDIconEntity);

INCLUDE_ASM("asm/nonmatchings/effects", Render_RotatingStarEffect);

INCLUDE_ASM("asm/nonmatchings/effects", func_80032DD8);

INCLUDE_ASM("asm/nonmatchings/effects", InitPathFollowEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyPathFollowEntity);

INCLUDE_ASM("asm/nonmatchings/effects", RenderPathEntitySegments);

INCLUDE_ASM("asm/nonmatchings/effects", func_800335A4);

INCLUDE_ASM("asm/nonmatchings/effects", InitPathFollowEntityAlt);

INCLUDE_ASM("asm/nonmatchings/effects", EntityDestructor_WithTextureAndMemory);

INCLUDE_ASM("asm/nonmatchings/effects", RenderRopeSegments);

void *InitEntityWithCallbackVtable(void *e, s16 val) {
    InitBasicEntityWithVtable(e, val);
    *(void **)((u8 *)e + 0xC) = &D_80010AD8;
    return e;
}

void FlushDepthBucketsGlobal(void) {
    FlushDepthBuckets(g_pGameState);
}

INCLUDE_ASM("asm/nonmatchings/effects", InitAlternateEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyCompoundEntity);

INCLUDE_ASM("asm/nonmatchings/effects", SetupAlternateEntitySpriteContext);

void EntityTickWithSpawnBoundsCheck(Entity *e) {
    EntityUpdateCallback(e);
    if ((u8)IsEntityOutsideSpawnBounds(e)) {
        *(s32 *)(*(u32 *)((u8 *)e + 0x100) + 0x3C) = 0;
        *(u32 *)((u8 *)e + 0x100) = 0;
        *(u8 *)(*(u32 *)((u8 *)e + 0x34) + 0xA) = 0;
        *(u8 *)((u8 *)e + 0x109) = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", CalculateChildEntityRenderPos);

INCLUDE_ASM("asm/nonmatchings/effects", IsEntityOutsideSpawnBounds);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTick_UploadTextureWithCallback);

extern void *D_80010AA8;

void *InitColoredOverlayEntity(u8 *e, u8 r, u8 g, u8 b, u8 alpha, s16 vtableArg) {
    InitBasicEntityWithVtable(e, vtableArg);
    *(void **)(e + 0xC) = &D_80010AA8;
    e[0x40] = r;
    e[0x41] = g;
    e[0x42] = b;
    e[0x43] = alpha;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderFullScreenTileOverlay);

INCLUDE_ASM("asm/nonmatchings/effects", InitScrollingLayerEntity);

s32 OverlayEntityCallback(Entity *e, u32 ev) {
    if ((ev & 0xFFFF) == EVT_TICK) {
        *(u8 *)(*(u32 *)((u8 *)e + 0x20) + 0xA) = 0;
        *(u8 *)((u8 *)e + 0x34) = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", func_80034B10);

INCLUDE_ASM("asm/nonmatchings/effects", InvokeGameStateCallback);

INCLUDE_ASM("asm/nonmatchings/effects", InitEntity_168254b5);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTick_FrictionAndParallaxScale);

INCLUDE_ASM("asm/nonmatchings/effects", InitDebrisParticleEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DebrisParticleTickCallback);

INCLUDE_ASM("asm/nonmatchings/effects", DebrisParticlePhysicsTick);

INCLUDE_ASM("asm/nonmatchings/effects", InitGridDistortionEffect);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyGridDistortionEffect);

INCLUDE_ASM("asm/nonmatchings/effects", RenderGridDistortionEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitGridLineEntity);

INCLUDE_ASM("asm/nonmatchings/effects", CheckChildEntityOffscreenDespawn);

INCLUDE_ASM("asm/nonmatchings/effects", InitGridSpriteContext);

INCLUDE_ASM("asm/nonmatchings/effects", EntityDespawnIfFlagSet);

INCLUDE_ASM("asm/nonmatchings/effects", NotifyGameStateZero);

INCLUDE_ASM("asm/nonmatchings/effects", CreatePlayerParticleEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTimerDespawnCallback);

INCLUDE_ASM("asm/nonmatchings/effects", UpdateAndUploadSpriteToVRAM);

INCLUDE_ASM("asm/nonmatchings/effects", InitPlayerDeathParticle);

INCLUDE_ASM("asm/nonmatchings/effects", EntityFadeOutTickCallback);

INCLUDE_ASM("asm/nonmatchings/effects", InitPlayerSparkParticle);

INCLUDE_ASM("asm/nonmatchings/effects", ParticleMovementAndFadeTick);

extern void *D_800109A0;

u8 *InitRippleExpandEffect(u8 *e, s32 color) {
    GameState *gs;
    u8 c;
    InitBasicEntityWithVtable(e, 0x3CA);
    *(void **)(e + 0xC) = &D_800109A0;
    e[0x3A7] = 0;
    gs = g_pGameState;
    c = color;
    *(s16 *)(e + 0x3A0) = c;
    *(s16 *)(e + 0x3A2) = c;
    *(s16 *)(e + 0x3A4) = c;
    e[0x3A6] = (u8)gs->frame_counter;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderRippleExpandEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitScaledMenuEntityWithChild);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyRippleExpandEffect);

void CountdownTimerTickCallback(void *e) {
    u8 *p = (u8 *)e;
    u8 v = p[0x28] - 1;
    p[0x28] = v;
    if (v == 0) {
        u8 *q = *(u8 **)(p + 0x20);
        p[0x29] = 1;
        q[0xA] = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", RippleEffectRenderCallback);

INCLUDE_ASM("asm/nonmatchings/effects", ExpiredEntityDespawnEvent);

INCLUDE_ASM("asm/nonmatchings/effects", NotifyGameStateOne);

INCLUDE_ASM("asm/nonmatchings/effects", NullStubFunction);

void *InitMenuItemEntity(void *e) {
    InitBasicEntityWithVtable(e, 0xBB8);
    *(void **)((u8 *)e + 0xC) = &D_80010970;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderSpotlightBeamEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitMenuEntityWithChildAndParams);

INCLUDE_ASM("asm/nonmatchings/effects", DestroySpotlightBeamEffect);

void BeamEffectTickWithRotation(void *e) {
    s16 timer;
    *(u16 *)((u8 *)e + 0x28) += *(u16 *)((u8 *)e + 0x1C);
    timer = (s16)(*(u16 *)((u8 *)e + 0x2A) - 1);
    *(u16 *)((u8 *)e + 0x2A) = timer;
    if (timer == 0) {
        void *p = *(void **)((u8 *)e + 0x20);
        *(u8 *)((u8 *)p + 0xA) = 0;
        *(u8 *)((u8 *)e + 0x1E) = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", BeamEffectRenderCallback);

INCLUDE_ASM("asm/nonmatchings/effects", BeamEffectDespawnEvent);

INCLUDE_ASM("asm/nonmatchings/effects", NotifyGameStateOneAlt);

INCLUDE_ASM("asm/nonmatchings/effects", InitScalableTimerEntity);

INCLUDE_ASM("asm/nonmatchings/effects", OscillateScaleAndRotationTick);

INCLUDE_ASM("asm/nonmatchings/effects", InitFadeMenuEntityWithChild);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyOscillatingScaleEntity);

INCLUDE_ASM("asm/nonmatchings/effects", FadeAndExpireEntityTick);

INCLUDE_ASM("asm/nonmatchings/effects", FadeExpireEntityDespawnEvent);

INCLUDE_ASM("asm/nonmatchings/effects", NotifyGameStateOneAlt2);

s32 HandleCollisionEvent0x1018(void *e, u16 event) {
    if (event == EVT_WORLD_FREEZE) {
        *(u8 *)((u8 *)e + 0x24) = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", InitLoadingScreenEntity);

void ZOrderChangeAndTimerTick(Entity *e) {
    u8 timer = *(u8 *)((u8 *)e + 0x108);
    if (timer != 0) {
        timer -= 1;
        *(u8 *)((u8 *)e + 0x108) = timer;
        if (timer == 0) {
            ChangeRenderZOrder(g_pGameState, *(void **)((u8 *)e + 0x34), 0x3D4);
        }
    }
    EntityTimerTickAndNotify(e);
}

INCLUDE_ASM("asm/nonmatchings/effects", HandleEventAndChangeZOrder);

INCLUDE_ASM("asm/nonmatchings/effects", CreateFadeOverlayEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyTimedFadeEntity);

void EntityFadeInTickCallback(void *e) {
    s16 v = *(s16 *)((u8 *)e + 0x22) + 8;
    *(s16 *)((u8 *)e + 0x22) = v;
    if (v >= 0x100) {
        *(s16 *)((u8 *)e + 0x22) = 0xFF;
    }
    {
        u8 c = *(u8 *)((u8 *)e + 0x22);
        u8 *q = *(u8 **)((u8 *)e + 0x1C);
        q[0x40] = c;
        q[0x41] = c;
        q[0x42] = c;
    }
}

void EntityDestroyCallback_Vt80010BA8(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010BA8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010BA8_80038510(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010BA8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_80038574(void *e, u16 val) {
    *(u16 *)((u8 *)e + 0x90) = val;
}

void func_8003857C(void *e, u8 val) {
    *(u8 *)((u8 *)e + 0x3A7) = val;
}

void EntityDestroyWithFreeCallback1(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800109F0;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyWithFreeCallback2(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800109F0;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyWithFreeCallback3(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800109F0;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyWithFreeCallback4(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010A10;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010A48(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010A48;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010A68(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010A68;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyResourceType4(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010A88;
    FreeResourceType4(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_80038840(void *e, u8 b40, u8 b41, u8 b42) {
    *(u8 *)((u8 *)e + 0x40) = b40;
    *(u8 *)((u8 *)e + 0x41) = b41;
    *(u8 *)((u8 *)e + 0x42) = b42;
}

void func_80038850(void *e, u8 val) {
    *(u8 *)((u8 *)e + 0x58) = val;
}

void func_80038858(void *e, u16 val) {
    *(u16 *)((u8 *)e + 0x46) = val;
}

void func_80038860(void *e, u16 val) {
    *(u16 *)((u8 *)e + 0x44) = val;
}

void func_80038868(void *e, u8 val) {
    *(u8 *)((u8 *)e + 0x5D) = val;
}

void func_80038870(void *e, u16 val) {
    *(u16 *)((u8 *)e + 0x46) = val;
}

void func_80038878(void *e, u16 val) {
    *(u16 *)((u8 *)e + 0x44) = val;
}

void func_80038880(void *e, u8 val) {
    *(u8 *)((u8 *)e + 0x1E7) = val;
}

void func_80038888(void *e) {
    *(u8 *)((u8 *)e + 0x1E0) = 0x20;
}

void EntityDestroyCallback_Vt80010B28(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010B28;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

u32 func_800388F8(void *e) {
    return *(u32 *)((u8 *)e + 0x1C);
}

void EntityDestroyCallback_Vt80010BA8_80038904(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80010BA8;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80038968(void) {
}

void NopStub_80038970(void) {
}

void EntityDestroyCallback_Vt80010BC8(void *entity, u32 flag) {
    ((Entity *)entity)->collisionVtable = &D_80010BC8;
    if (flag & 1) {
        FreeEntityNoTeardown_800389ac(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_800389ac(void *entity, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
}

