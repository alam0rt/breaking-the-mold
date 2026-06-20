#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/entity_events.h"

extern void *g_pBlbHeapBase;
extern void FlushDepthBuckets(GameState *entity);
extern Entity *InitBasicEntityWithVtable(Entity *e, s16 val);
extern u8 IsEntityOutsideSpawnBounds(Entity *e);
extern void ChangeRenderZOrder(GameState *gs, Entity *layer, u32 zOrder);
extern void EntityTimerTickAndNotify(Entity *e);
extern void *PARTICLE_EFFECT_VTABLE asm("D_80010A48");
extern void *VFX_ENTITY_VTABLE_10A68 asm("D_80010A68");
extern void *GRID_DISTORTION_EVENT_VTABLE asm("D_80010A30");
extern void *CALLBACK_ENTITY_EVENT_VTABLE asm("D_80010AD8");
extern void *VFX_ENTITY_VTABLE_10AE8 asm("D_80010AE8");
extern void *PATH_ENTITY_EVENT_VTABLE asm("D_80010B00");
extern void *ROPE_SEGMENT_ENTITY_VTABLE asm("D_80010B28");
extern void *VFX_ENTITY_VTABLE_10B78 asm("D_80010B78");
extern void *FADE_EFFECT_ENTITY_VTABLE asm("D_80010BA8");
extern void *TIMED_FADE_ENTITY_VTABLE asm("D_80010BC8");
extern void *MENU_ITEM_ENTITY_VTABLE asm("D_80010970");
extern void *FREE_CALLBACK_ENTITY_VTABLE asm("D_800109F0");
extern void *RESOURCE_TYPE4_ENTITY_VTABLE asm("D_80010A88");
extern void *VFX_ENTITY_VTABLE_10A10 asm("D_80010A10");
extern void FreeResourceType4(Entity *e, s32 mode);
extern void FreeWithCallback(Entity *e, s32 mode);
extern void FreeTextureResource(Entity *e, s32 mode);

typedef struct DecorEventEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u16 timer;
    /* 0x102 */ u8 notifyFlag;
} DecorEventEntity;

typedef struct EffectChildContext {
    /* 0x00 */ u8 pad0[0xA];
    /* 0x0A */ u8 activeFlag;
} EffectChildContext;

typedef struct MultiPartEffectEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[8];
    /* 0x108 */ u8 releaseFlag;
} MultiPartEffectEntity;

typedef struct SpawnBoundsRecord {
    /* 0x00 */ u8 pad0[0x3C];
    /* 0x3C */ s32 active;
} SpawnBoundsRecord;

typedef struct SpawnBoundsEffectEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ SpawnBoundsRecord *record;
    /* 0x104 */ u8 pad104[5];
    /* 0x109 */ u8 despawnedFlag;
} SpawnBoundsEffectEntity;

typedef struct ColoredOverlayEntity {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ void *eventVtable;
    /* 0x10 */ u8 pad10[0x30];
    /* 0x40 */ u8 r;
    /* 0x41 */ u8 g;
    /* 0x42 */ u8 b;
    /* 0x43 */ u8 alpha;
} ColoredOverlayEntity;

typedef struct OverlayCallbackEntity {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ EffectChildContext *child;
    /* 0x24 */ u8 pad24[0x10];
    /* 0x34 */ u8 hiddenFlag;
} OverlayCallbackEntity;

typedef struct RippleExpandEntity {
    /* 0x000 */ u8 pad0[0xC];
    /* 0x00C */ void *eventVtable;
    /* 0x010 */ u8 pad10[0x390];
    /* 0x3A0 */ s16 red;
    /* 0x3A2 */ s16 green;
    /* 0x3A4 */ s16 blue;
    /* 0x3A6 */ u8 frameSeed;
    /* 0x3A7 */ u8 phase;
} RippleExpandEntity;

typedef struct CountdownTimerEntity {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ EffectChildContext *child;
    /* 0x24 */ u8 pad24[4];
    /* 0x28 */ u8 timer;
    /* 0x29 */ u8 expiredFlag;
} CountdownTimerEntity;

typedef struct BeamEffectEntity {
    /* 0x00 */ u8 pad0[0x1C];
    /* 0x1C */ u16 rotationStep;
    /* 0x1E */ u8 expiredFlag;
    /* 0x1F */ u8 pad1F;
    /* 0x20 */ EffectChildContext *child;
    /* 0x24 */ u8 worldFreezeFlag;
    /* 0x25 */ u8 pad25[3];
    /* 0x28 */ u16 rotation;
    /* 0x2A */ u16 timer;
} BeamEffectEntity;

typedef struct EntityWithOwnedData {
    /* 0x00 */ u8 pad0[0x18];
    /* 0x18 */ void *collisionVtable;
    /* 0x1C */ void *ownedData;
} EntityWithOwnedData;

typedef struct EntityWithTextureMemory {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ void *eventVtable;
    /* 0x10 */ u8 pad10[0x2C];
    /* 0x3C */ void *textureData0;
    /* 0x40 */ void *textureData1;
} EntityWithTextureMemory;

typedef struct PathFollowResourceEntity {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ void *eventVtable;
    /* 0x10 */ u8 pad10[0x2C];
    /* 0x3C */ void *pathData;
    /* 0x40 */ void *segmentData;
    /* 0x44 */ u8 pad44[4];
    /* 0x48 */ void *extraData;
} PathFollowResourceEntity;

typedef struct GridDistortionResourceEntity {
    /* 0x000 */ u8 pad0[0xC];
    /* 0x00C */ void *eventVtable;
    /* 0x010 */ u8 pad10[0xEC];
    /* 0x0FC */ void *gridData0;
    /* 0x100 */ void *gridData1;
} GridDistortionResourceEntity;

typedef struct ZOrderTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[8];
    /* 0x108 */ u8 timer;
} ZOrderTimerEntity;

typedef struct FadeInEntity {
    /* 0x00 */ u8 pad0[0x1C];
    /* 0x1C */ ColoredOverlayEntity *overlay;
    /* 0x20 */ u8 pad20[2];
    /* 0x22 */ s16 alpha;
} FadeInEntity;

typedef struct EffectWord90Entity {
    /* 0x000 */ u8 pad0[0x90];
    /* 0x090 */ u16 value90;
} EffectWord90Entity;

typedef struct EffectByte58Entity {
    /* 0x00 */ u8 pad0[0x58];
    /* 0x58 */ u8 value58;
    /* 0x59 */ u8 pad59[4];
    /* 0x5D */ u8 value5D;
} EffectByte58Entity;

typedef struct LargeEffectStateEntity {
    /* 0x000 */ u8 pad0[0x1E0];
    /* 0x1E0 */ u8 value1E0;
    /* 0x1E1 */ u8 pad1E1[6];
    /* 0x1E7 */ u8 value1E7;
} LargeEffectStateEntity;

INCLUDE_ASM("asm/nonmatchings/effects", InitParticleEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTimerTickAndNotify);

s32 DecorHandleCallback(DecorEventEntity *e, u16 event) {
    if (event == EVT_TICK && e->timer == 0) {
        e->notifyFlag = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", InitBlbOverlayEntity);

INCLUDE_ASM("asm/nonmatchings/effects", RenderRotatedTexturedQuad);

INCLUDE_ASM("asm/nonmatchings/effects", InitMenuSpriteEntity);

void DestroyEntityWithData(EntityWithOwnedData *e, s32 flags) {
    e->collisionVtable = &VFX_ENTITY_VTABLE_10B78;
    FreeFromHeap(g_pBlbHeapBase, e->ownedData, 0, 0);
    e->collisionVtable = &TIMED_FADE_ENTITY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

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

u32 MultiPartEntityMessageHandler(MultiPartEffectEntity *e, s32 event) {
    switch (event & 0xFFFF) {
        case EVT_TOKEN_RELEASE:
            e->releaseFlag = 1;
            break;
        case 0x1010:
            ((EffectChildContext *)e->sprite.base.spriteContext)->activeFlag = 0;
            break;
        case EVT_ATTACH_TO_ENTITY:
            ((EffectChildContext *)e->sprite.base.spriteContext)->activeFlag = 1;
            break;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", InitHUDIconEntity);

INCLUDE_ASM("asm/nonmatchings/effects", Render_RotatingStarEffect);

INCLUDE_ASM("asm/nonmatchings/effects", func_80032DD8);

INCLUDE_ASM("asm/nonmatchings/effects", InitPathFollowEntity);

void DestroyPathFollowEntity(PathFollowResourceEntity *e, s32 flags) {
    e->eventVtable = &PATH_ENTITY_EVENT_VTABLE;
    FreeFromHeap(g_pBlbHeapBase, e->pathData, 0, 0);
    FreeFromHeap(g_pBlbHeapBase, e->segmentData, 0, 0);
    FreeFromHeap(g_pBlbHeapBase, e->extraData, 0, 0);
    FreeTextureResource(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderPathEntitySegments);

INCLUDE_ASM("asm/nonmatchings/effects", func_800335A4);

INCLUDE_ASM("asm/nonmatchings/effects", InitPathFollowEntityAlt);

void EntityDestructor_WithTextureAndMemory(EntityWithTextureMemory *e, s32 flags) {
    e->eventVtable = &VFX_ENTITY_VTABLE_10AE8;
    FreeFromHeap(g_pBlbHeapBase, e->textureData0, 0, 0);
    FreeFromHeap(g_pBlbHeapBase, e->textureData1, 0, 0);
    FreeTextureResource(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderRopeSegments);

Entity *InitEntityWithCallbackVtable(Entity *e, s16 val) {
    InitBasicEntityWithVtable(e, val);
    e->eventCallback = (EntityCallback)&CALLBACK_ENTITY_EVENT_VTABLE;
    return e;
}

void FlushDepthBucketsGlobal(void) {
    FlushDepthBuckets(g_pGameState);
}

INCLUDE_ASM("asm/nonmatchings/effects", InitAlternateEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyCompoundEntity);

INCLUDE_ASM("asm/nonmatchings/effects", SetupAlternateEntitySpriteContext);

void EntityTickWithSpawnBoundsCheck(SpawnBoundsEffectEntity *e) {
    EntityUpdateCallback((Entity *)e);
    if ((u8)IsEntityOutsideSpawnBounds((Entity *)e)) {
        e->record->active = 0;
        e->record = 0;
        ((EffectChildContext *)e->sprite.base.spriteContext)->activeFlag = 0;
        e->despawnedFlag = 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", CalculateChildEntityRenderPos);

INCLUDE_ASM("asm/nonmatchings/effects", IsEntityOutsideSpawnBounds);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTick_UploadTextureWithCallback);

extern void *COLORED_OVERLAY_EVENT_VTABLE asm("D_80010AA8");

ColoredOverlayEntity *InitColoredOverlayEntity(ColoredOverlayEntity *e, u8 r, u8 g, u8 b, u8 alpha, s16 vtableArg) {
    InitBasicEntityWithVtable((Entity *)e, vtableArg);
    e->eventVtable = &COLORED_OVERLAY_EVENT_VTABLE;
    e->r = r;
    e->g = g;
    e->b = b;
    e->alpha = alpha;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderFullScreenTileOverlay);

/* 8-byte FSM callback slot: { markerLo, markerHi, fn }. { 0, -1, fn } means
 * "direct call fn(entity, ...)". Assigned by value into the entity's slot. */
typedef struct EntCallbackSlot {
    /* 0x0 */ s16 markerLo;
    /* 0x2 */ s16 markerHi;
    /* 0x4 */ void (*fn)();
} EntCallbackSlot;

/* Padded wrapper: the leading/trailing s32 pad pins the original's 0x30 stack
 * frame (the bare 8-byte slot yields a 0x28 frame). Only .s is used. */
typedef struct PaddedEntSlot {
    s32             pad;
    EntCallbackSlot s;
    s32             pad2;
} PaddedEntSlot;

/* The scroll context takes a packed pair of s16s by value in one register. */
typedef struct ScrollLayerDims {
    /* 0x0 */ s16 a;
    /* 0x2 */ s16 b;
} ScrollLayerDims;

extern void InitLayerScrollContext(Entity *e, s32 ctx, s16 a, s16 b, s16 c, u8 flags);
s32 OverlayEntityCallback(OverlayCallbackEntity *e, u32 ev);

void InitScrollingLayerEntity(Entity *e, s32 ctx, ScrollLayerDims dims, s16 a3, u8 flags) {
    PaddedEntSlot u;
    s16 m1;
    void (*fn)();
    OverlayCallbackEntity *o;

    InitLayerScrollContext(e, ctx, a3, dims.a, dims.b, flags);
    e->collisionVtable = &RESOURCE_TYPE4_ENTITY_VTABLE;
    e->allocSize = 0x44C;
    do {} while (0);
    fn = (void (*)())OverlayEntityCallback;
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(EntCallbackSlot *)&e->eventMarker = u.s;
    /* separate pointer + barrier reproduces the `move $v0,$s0` before the store */
    o = (OverlayCallbackEntity *)e;
    /* @hack: pointer barrier prevents cc1 from coalescing `o` back into $s0 so the trailing `move $v0,$s0; sb $zero,...` sequence is emitted (Quirk 6i). */
    __asm__ volatile("" : "=r"(o) : "0"(o));
    o->hiddenFlag = 0;
}

s32 OverlayEntityCallback(OverlayCallbackEntity *e, u32 ev) {
    if ((ev & 0xFFFF) == EVT_TICK) {
        e->child->activeFlag = 0;
        e->hiddenFlag = 1;
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

void DestroyGridDistortionEffect(GridDistortionResourceEntity *e, s32 flags) {
    e->eventVtable = &GRID_DISTORTION_EVENT_VTABLE;
    FreeFromHeap(g_pBlbHeapBase, e->gridData0, 0, 0);
    FreeFromHeap(g_pBlbHeapBase, e->gridData1, 0, 0);
    FreeTextureResource(e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderGridDistortionEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitGridLineEntity);

INCLUDE_ASM("asm/nonmatchings/effects", CheckChildEntityOffscreenDespawn);

INCLUDE_ASM("asm/nonmatchings/effects", InitGridSpriteContext);

INCLUDE_ASM("asm/nonmatchings/effects", EntityDespawnIfFlagSet);

INCLUDE_ASM("asm/nonmatchings/effects", NotifyGameStateZero);

INCLUDE_ASM("asm/nonmatchings/effects", CreatePlayerParticleEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTimerDespawnCallback);

INCLUDE_ASM("asm/nonmatchings/effects", UpdateAndUploadSpriteToVRAM);

/* Sprite render context (Entity.spriteContext) fields touched here. */
typedef struct ParticleRenderCtx {
    /* 0x00 */ u8  pad0[0x10];
    /* 0x10 */ s16 x;
    /* 0x12 */ s16 y;
    /* 0x14 */ u8  pad14[0x10];
    /* 0x24 */ s16 tpage;
    /* 0x26 */ u8  pad26[0xC];
    /* 0x32 */ u8  depth;
} ParticleRenderCtx;

extern Entity *CreatePlayerParticleEntity(Entity *e, s32 spawnArg);
extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern void *PARTICLE_FADE_VTABLE asm("D_800109D0");
void EntityFadeOutTickCallback(Entity *e);

/* Fading sprite fragment spawned on player death: semi-transparent (blend mode
 * 1) + a fade-out tick = the dissolving death debris. */
SpriteEntity *InitPlayerDeathParticle(SpriteEntity *e, s32 spawnArg) {
    ParticleRenderCtx *ctx;
    PaddedEntSlot u;
    s16 m1;
    void (*fn)();
    s32 cx, cy;

    CreatePlayerParticleEntity((Entity *)e, spawnArg);
    e->base.collisionVtable = &PARTICLE_FADE_VTABLE;
    e->_pad80[4] = 0x40;
    do {} while (0);
    fn = (void (*)())EntityFadeOutTickCallback;
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(EntCallbackSlot *)&e->base.tickMarker = u.s;
    ctx = (ParticleRenderCtx *)e->base.spriteContext;
    cx = ctx->x;
    cy = ctx->y;
    ctx->tpage = GetTPage(ctx->depth, 1, cx & -0x40, cy & -0x100);
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", EntityFadeOutTickCallback);

INCLUDE_ASM("asm/nonmatchings/effects", InitPlayerSparkParticle);

INCLUDE_ASM("asm/nonmatchings/effects", ParticleMovementAndFadeTick);

extern void *RIPPLE_EXPAND_EVENT_VTABLE asm("D_800109A0");

RippleExpandEntity *InitRippleExpandEffect(RippleExpandEntity *e, s32 color) {
    GameState *gs;
    u8 c;
    InitBasicEntityWithVtable((Entity *)e, 0x3CA);
    e->eventVtable = &RIPPLE_EXPAND_EVENT_VTABLE;
    e->phase = 0;
    gs = g_pGameState;
    c = color;
    e->red = c;
    e->green = c;
    e->blue = c;
    e->frameSeed = (u8)gs->frame_counter;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderRippleExpandEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitScaledMenuEntityWithChild);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyRippleExpandEffect);

void CountdownTimerTickCallback(CountdownTimerEntity *e) {
    u8 v = e->timer - 1;
    e->timer = v;
    if (v == 0) {
        e->expiredFlag = 1;
        e->child->activeFlag = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", RippleEffectRenderCallback);

INCLUDE_ASM("asm/nonmatchings/effects", ExpiredEntityDespawnEvent);

INCLUDE_ASM("asm/nonmatchings/effects", NotifyGameStateOne);

INCLUDE_ASM("asm/nonmatchings/effects", NullStubFunction);

Entity *InitMenuItemEntity(Entity *e) {
    InitBasicEntityWithVtable(e, 0xBB8);
    e->eventCallback = (EntityCallback)&MENU_ITEM_ENTITY_VTABLE;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderSpotlightBeamEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitMenuEntityWithChildAndParams);

INCLUDE_ASM("asm/nonmatchings/effects", DestroySpotlightBeamEffect);

void BeamEffectTickWithRotation(BeamEffectEntity *e) {
    s16 timer;
    e->rotation += e->rotationStep;
    timer = (s16)(e->timer - 1);
    e->timer = timer;
    if (timer == 0) {
        e->child->activeFlag = 0;
        e->expiredFlag = 1;
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

s32 HandleCollisionEvent0x1018(BeamEffectEntity *e, u16 event) {
    if (event == EVT_WORLD_FREEZE) {
        e->worldFreezeFlag = 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/effects", InitLoadingScreenEntity);

void ZOrderChangeAndTimerTick(ZOrderTimerEntity *e) {
    u8 timer = e->timer;
    if (timer != 0) {
        timer -= 1;
        e->timer = timer;
        if (timer == 0) {
            ChangeRenderZOrder(g_pGameState, e->sprite.base.spriteContext, 0x3D4);
        }
    }
    EntityTimerTickAndNotify((Entity *)e);
}

INCLUDE_ASM("asm/nonmatchings/effects", HandleEventAndChangeZOrder);

INCLUDE_ASM("asm/nonmatchings/effects", CreateFadeOverlayEntity);

INCLUDE_ASM("asm/nonmatchings/effects", DestroyTimedFadeEntity);

void EntityFadeInTickCallback(FadeInEntity *e) {
    s16 v = e->alpha + 8;
    e->alpha = v;
    if (v >= 0x100) {
        e->alpha = 0xFF;
    }
    {
        u8 c = e->alpha;
        ColoredOverlayEntity *q = e->overlay;
        q->r = c;
        q->g = c;
        q->b = c;
    }
}

void EntityDestroyCallback_Vt80010BA8(Entity *entity, s32 flags) {
    entity->collisionVtable = &FADE_EFFECT_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010BA8_80038510(Entity *entity, s32 flags) {
    entity->collisionVtable = &FADE_EFFECT_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_80038574(EffectWord90Entity *e, u16 val) {
    e->value90 = val;
}

void func_8003857C(RippleExpandEntity *e, u8 val) {
    e->phase = val;
}

void EntityDestroyWithFreeCallback1(Entity *entity, s32 flags) {
    entity->collisionVtable = &FREE_CALLBACK_ENTITY_VTABLE;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyWithFreeCallback2(Entity *entity, s32 flags) {
    entity->collisionVtable = &FREE_CALLBACK_ENTITY_VTABLE;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyWithFreeCallback3(Entity *entity, s32 flags) {
    entity->collisionVtable = &FREE_CALLBACK_ENTITY_VTABLE;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyWithFreeCallback4(Entity *entity, s32 flags) {
    entity->collisionVtable = &VFX_ENTITY_VTABLE_10A10;
    FreeWithCallback(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010A48(Entity *entity, s32 flags) {
    entity->collisionVtable = &PARTICLE_EFFECT_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyCallback_Vt80010A68(Entity *entity, s32 flags) {
    entity->collisionVtable = &VFX_ENTITY_VTABLE_10A68;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void EntityDestroyResourceType4(Entity *entity, s32 flags) {
    entity->collisionVtable = &RESOURCE_TYPE4_ENTITY_VTABLE;
    FreeResourceType4(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_80038840(ColoredOverlayEntity *e, u8 b40, u8 b41, u8 b42) {
    e->r = b40;
    e->g = b41;
    e->b = b42;
}

void func_80038850(EffectByte58Entity *e, u8 val) {
    e->value58 = val;
}

void func_80038858(Entity *e, u16 val) {
    e->hitboxHeight = val;
}

void func_80038860(Entity *e, u16 val) {
    e->hitboxWidth = val;
}

void func_80038868(EffectByte58Entity *e, u8 val) {
    e->value5D = val;
}

void func_80038870(Entity *e, u16 val) {
    e->hitboxHeight = val;
}

void func_80038878(Entity *e, u16 val) {
    e->hitboxWidth = val;
}

void func_80038880(LargeEffectStateEntity *e, u8 val) {
    e->value1E7 = val;
}

void func_80038888(LargeEffectStateEntity *e) {
    e->value1E0 = 0x20;
}

void EntityDestroyCallback_Vt80010B28(Entity *entity, s32 flags) {
    entity->collisionVtable = &ROPE_SEGMENT_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

u32 func_800388F8(Entity *e) {
    return e->renderMarker;
}

void EntityDestroyCallback_Vt80010BA8_80038904(Entity *entity, s32 flags) {
    entity->collisionVtable = &FADE_EFFECT_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void NopStub_80038968(void) {
}

void NopStub_80038970(void) {
}

void EntityDestroyCallback_Vt80010BC8(Entity *entity, u32 flag) {
    entity->collisionVtable = &TIMED_FADE_ENTITY_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_800389ac(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_800389ac(Entity *entity, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
}

