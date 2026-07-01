#include "common.h"
#include "functions.h"
#include "Game/entity.h"
#include "Game/game_state.h"
#include "Game/fsm_dispatch.h"
#include "globals.h"

extern void InitEntityStruct(Entity *entity, s16 allocSize);
extern u8 *ClearSpriteContextWrapper(u8 *ctx);
extern u8 *ZeroEntityField(u8 *field);
extern void InitEntityAnimationState(SpriteEntity *entity);
extern void CalculateEntityScreenBounds(Entity *entity);

typedef s16 (*XformCB)();
typedef struct { s32 arg; XformCB fn; } XformSlot;

typedef struct SpriteContextCallbackTable {
    /* 0x00 */ u8 pad00[0x10];
    /* 0x10 */ s16 callbackTargetOffset;
    /* 0x12 */ u8 pad12[2];
    /* 0x14 */ void (*releaseVRAMSlot)(u8 *target, s32 mode);
} SpriteContextCallbackTable;

typedef struct SpriteRenderContext {
    /* 0x00 */ u8 pad00[4];
    /* 0x04 */ s16 width;
    /* 0x06 */ s16 height;
    /* 0x08 */ u8 pad08[4];
    /* 0x0C */ SpriteContextCallbackTable *callbacks;
} SpriteRenderContext;

/* Entity constructor. Zeroes the 0x80 base header, installs the Destroyed +
 * PartialDestroy vtables, primes the four 16.16 scale fields at 0x50..0x64
 * to 1.0, and stores allocSize at +0x10. All Init* helpers chain here. */
INCLUDE_ASM("asm/nonmatchings/entity", InitEntityStruct);

/* Entity destructor (base form). Flips vtable to PartialDestroy so broadcast
 * loops drop us, runs the sprite context's destroy callback with mode=3
 * (release the reserved PSX texture-VRAM slot), then the Destroyed vtable.
 * flags&1 returns the entity itself to the BLB heap. */
void FreeWithCallback(Entity *entity, s32 flags) {
    SpriteRenderContext *ctx = entity->spriteContext;
    entity->collisionVtable = g_EntityVtable_PartialDestroy;
    if (ctx != NULL) {
        SpriteContextCallbackTable *callbacks = ctx->callbacks;
        s16 offset = callbacks->callbackTargetOffset;
        void (*func)(u8 *, s32) = callbacks->releaseVRAMSlot;
        func(&((u8 *)ctx)[offset], 3);
    }
    entity->collisionVtable = g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* 16.16 fixed-point scaler installed by SetupEntityScaleCallbacks as the
 * entity's moveCallbackX FSM slot. Identity short-circuit at 1.0. */
s32 ScaleXByEntityScale(Entity *entity, s16 value) {
    if (entity->scalePowerupX == 0x10000) {
        return (s32)value;
    }
    return (s32)value * entity->scalePowerupX >> 16;
}

/* Y axis counterpart of ScaleXByEntityScale. */
s32 ScaleYByEntityScale(Entity *entity, s16 value) {
    if (entity->scalePowerupY == 0x10000) {
        return (s32)value;
    }
    return (s32)value * entity->scalePowerupY >> 16;
}

/* Translate an entity-local pixel to camera-space by adding the camera
 * offset. No scaling. Body is byte-identical to WorldToScreenX below. */
s32 GetWorldPositionX(Entity *entity, s16 localX) {
    return (s32)localX + (s32)g_pGameState->camera_x;
}

/* Y axis counterpart of GetWorldPositionX. */
s32 GetWorldPositionY(Entity *entity, s16 localY) {
    return (s32)localY + (s32)g_pGameState->camera_y;
}

/* NB: "Parallax" is a misnomer. These two scale the input value by the
 * entity's per-axis render scale at +0x58 (NOT the layer parallax scale at
 * +0x60), then add camera_x. Real background-layer parallax lives in
 * UpdateParallaxLayerPosition, which scales the camera by +0x60. */
s32 CalculateParallaxXOffset(Entity *entity, s16 value) {
    if (entity->scalePowerupX == 0x10000) {
        return (s32)value + (s32)g_pGameState->camera_x;
    }
    return ((s32)value * entity->scalePowerupX >> 16) + (s32)g_pGameState->camera_x;
}

/* Y-axis sibling of CalculateParallaxXOffsetAlt. Scales by +0x5C (entity
 * render scale, not parallax) then adds camera_y. */
s32 WorldToScreenYWithParallax(Entity *entity, s16 value) {
    if (entity->scalePowerupY == 0x10000) {
        return (s32)value + (s32)g_pGameState->camera_y;
    }
    return ((s32)value * entity->scalePowerupY >> 16) + (s32)g_pGameState->camera_y;
}

extern SpriteRenderContext *PrepareSpriteVRAMSlotForContext(SpriteRenderContext *ctx, s16 width, s16 height, s16 depth, u8 flags);

/* Allocate a 0x3C-byte sprite-render context on the BLB heap and reserve a
 * PSX texture-VRAM rectangle. Height is rounded up to a multiple of 4
 * because PSX GPU rectangle uploads must be 4 px aligned in Y. depth
 * selects TIM bit-depth (4/8/15bpp); flags carry CLUT info. */
void CreateEntityRenderContext(Entity *entity, s16 width, s16 height, s16 depth, u8 flags) {
    SpriteRenderContext *ctx = (SpriteRenderContext *)AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    entity->spriteContext = PrepareSpriteVRAMSlotForContext(ctx, width, (s16)((height + 3) & 0xFFFCu), depth, flags);
}

/* Lazy recompute via CalculateEntityScreenBounds then memcpy the four s16
 * bounds (X1,Y1,X2,Y2 = 8 bytes) into the caller's buffer. */
void GetEntityScreenBounds(Entity *entity, s16 *out) {
    CalculateEntityScreenBounds(entity);
    __builtin_memcpy(out, &entity->screenX1, 8);
}

/* Derives screenX1/Y1/X2/Y2 from worldXY + hitbox offsets/size. Each of
 * the four edges runs through the moveCallbackX/Y FSM (which is how a
 * moving platform transports its riders without touching their stored
 * positions). Sets boundsValid so per-frame repeats are cheap. */
INCLUDE_ASM("asm/nonmatchings/entity", CalculateEntityScreenBounds);

/* Render-AABB variant. Uses renderOffset/renderWidth/renderHeight and only
 * the facing/flipY flags - no moveCallback dispatch, no scale; the renderer
 * doesn't need to know about platform riding to draw. Output is
 * {x1,y1,x2,y2} (4 s16's), mirroring facing==1/flipY==1 about worldX/worldY
 * instead of adding the offset. */
void CalculateEntityRenderBounds(Entity *entity, void *bounds) {
    s16 *out = bounds;

    if (entity->facing != 0) {
        out[0] = entity->worldX - entity->renderOffsetX - entity->renderWidth + 1;
        out[2] = entity->worldX - entity->renderOffsetX;
    } else {
        out[0] = entity->worldX + entity->renderOffsetX;
        out[2] = entity->renderWidth + (entity->worldX + entity->renderOffsetX) - 1;
    }

    if (entity->flipY != 0) {
        out[1] = entity->worldY - entity->renderOffsetY - entity->renderHeight + 1;
        out[3] = entity->worldY - entity->renderOffsetY;
    } else {
        out[1] = entity->worldY + entity->renderOffsetY;
        out[3] = entity->renderHeight + (entity->worldY + entity->renderOffsetY) - 1;
    }
}

/* Set sprite horizontal flip. dir==2 toggles current value. Marks the
 * entity textureDirty because flipping is done at upload time by
 * mirroring the UVs. */
void SetEntityFacingDirection(Entity *entity, u8 direction) {
    if (direction == 2) {
        direction = entity->facing == 0;
    }
    entity->facing = direction;
    entity->textureDirty = 1;
}

/* Vertical flip counterpart to SetEntityFacingDirection (touches flipY at
 * +0x75). dir==2 toggles. Likely real name: SetEntityFlipY. */
void func_8001AAE4(Entity *entity, u8 direction) {
    if (direction == 2) {
        direction = entity->flipY == 0;
    }
    entity->flipY = direction;
    entity->textureDirty = 1;
}

/* True background-layer parallax: positions a layer entity by scaling
 * camera_x/y by the entity's parallax scale at +0x60/+0x64 - this is the
 * "background scrolls slower than the camera" effect. */
INCLUDE_ASM("asm/nonmatchings/entity", UpdateParallaxLayerPosition);

/* Texture-callback stub: clears textureDirty if set. Almost certainly an
 * entity-vtable texture-upload slot for objects that need to participate
 * in the texture pass without actually uploading anything (invisible
 * logic entities, mute emitters, etc.). */
void func_8001B344(Entity *entity) {
    if (entity->textureDirty) {
        entity->textureDirty = 0;
    }
}

/* AABB-vs-point hit test on the entity's current on-screen bounds.
 * Used by pickup gating, trigger zones, projectile-vs-target tests. */
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

/* AABB-vs-AABB overlap on the entity's screen bounds. m2c packed the two
 * corners into a struct because GCC 2.7.2 passes them in $a1/$a2 word
 * pairs; real callers pass four loose s16's. */
s32 CheckBoxOverlap(Entity *entity, BoxCorner minCorner, BoxCorner maxCorner) {
    CalculateEntityScreenBounds(entity);
    if (entity->screenX1 > maxCorner.x) return 0;
    if (entity->screenX2 < minCorner.x) return 0;
    if (entity->screenY1 > maxCorner.y) return 0;
    if (entity->screenY2 < minCorner.y) return 0;
    return 1;
}

/* --- Broadcast collision family. Each walks a candidate list (player vs
 *     world, vs enemies, vs pickups...) and dispatches the per-entity
 *     collision FSM with a query box/point. --- */
void CollisionCheckWrapper(Entity *entity, u16 a1, u16 a2, u32 a3) {
    extern void DispatchEventToCollidingEntity(GameState *gs, BoxCorner minCorner,
                                  BoxCorner maxCorner, u32 a, u32 b, u32 c,
                                  Entity *e);
    CalculateEntityScreenBounds(entity);
    DispatchEventToCollidingEntity(g_pGameState,
                                  *(BoxCorner *)&entity->screenX1,
                                  *(BoxCorner *)&entity->screenX2,
                                  a1, a2, a3, entity);
}

void EntityBroadcastBoxCollision(Entity *entity, u16 a1, u16 a2, u32 a3) {
    extern void BroadcastBoxCollision(GameState *gs, BoxCorner minCorner,
                                  BoxCorner maxCorner, u32 a, u32 b, u32 c,
                                  Entity *e);
    CalculateEntityScreenBounds(entity);
    BroadcastBoxCollision(g_pGameState,
                                  *(BoxCorner *)&entity->screenX1,
                                  *(BoxCorner *)&entity->screenX2,
                                  a1, a2, a3, entity);
}

INCLUDE_ASM("asm/nonmatchings/entity", CheckEntityPointCollisionWithOffsets);

INCLUDE_ASM("asm/nonmatchings/entity", EntityBroadcastPointCollision);

/* Leaf primitive: pack (X1,Y1,X2,Y2) into two $a1/$a2 word pairs and call
 * the shared CheckBoxCollision AABB tester. */
u8 CheckEntityBoxCollision(Entity *entity, u16 collisionMask) {
    extern u32 CheckBoxCollision(GameState *gs, BoxCorner minCorner,
                                  BoxCorner maxCorner, u16 mask);
    CalculateEntityScreenBounds(entity);
    return (u8)CheckBoxCollision(g_pGameState,
                                  *(BoxCorner *)&entity->screenX1,
                                  *(BoxCorner *)&entity->screenX2,
                                  collisionMask);
}

/* --- Off-screen culling family. Variants:
 *     *OffScreen            : full XY test, moveCallback FSM + parallax scale
 *     *OffscreenLeft/Right/Y: single-axis test
 *     *Simple               : skip moveCallback (entity uses identity transform)
 *     IsPosition*           : raw coords, no entity (spawn-eligibility)
 *     ~16 px margin so just-off entities don't flicker visibility. --- */
INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffScreen);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenLeft);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenLeftSimple);

INCLUDE_ASM("asm/nonmatchings/entity", IsPositionOffscreenLeft);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenRight);

INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffscreenRightSimple);

INCLUDE_ASM("asm/nonmatchings/entity", IsPositionOffscreenRight);

/* Y-axis full off-screen test: same X-and-Y render-bounds calc as
 * CalculateEntityRenderBounds (inlined, only the Y1 edge is used), routed
 * through the moveCallbackY FSM slot like TransformYCoord (inlined too),
 * minus camera_y scaled by scaleParallaxY (like PlayEntityPositionSound's
 * camX), compared against a BLB-heap-header field + 0x10 px margin.
 * Data-flow verified via m2c; closest draft kept in
 * nonmatchings/IsEntityOffScreenY/ (gitignored local scratch, --no-prune
 * import). Residual is a family of small diffs (phi-shape for the X-bounds
 * calc, FSM-slot register-pin interference with the unrelated earlier
 * bounds/camera code inflating live ranges) that individually mirror
 * techniques that worked elsewhere in this file (Quirk 6l, FSM_REG) but
 * don't compose cleanly here - best hand attempt hit ~50 residual
 * instructions out of 123. Permuter attempt pending. */
INCLUDE_ASM("asm/nonmatchings/entity", IsEntityOffScreenY);

/* Reads g_pGameState->camera_scroll_speed (+0x11C). If 1.0 wipes the
 * moveCallback FSM (entity stays in identity space). Otherwise installs
 * ScaleX/YByEntityScale into moveCallbackX/Y, copies scroll_speed into
 * all four scale fields (0x50/0x54/0x58/0x5C), and pre-divides worldXY
 * by the scale so subsequent transforms round-trip. This is how the
 * zoom-camera boss levels keep entities positionally consistent. */
INCLUDE_ASM("asm/nonmatchings/entity", SetupEntityScaleCallbacks);

/* Compute SPU stereo pan from the entity's screen X (relative to camera)
 * and fire a one-shot PlaySoundEffect. Heavily used by pickups/enemies
 * for "this object made a noise where it is". Pan = worldX routed through
 * the moveCallbackX FSM slot (same dispatch shape as TransformXCoord,
 * inlined here rather than called) minus the camera X scaled by
 * scaleParallaxX (true background-layer parallax, see
 * UpdateParallaxLayerPosition). */
void PlayEntityPositionSound(Entity *entity, u32 soundId) {
    s16 camX;
    s16 m;
    u16 val;
    FSM_REG(XformCB, fn, "$6"); /* $a2 home */
    FSM_REG(XformCB, ft, "$9"); /* $t1 then-fn (relays) */
    s32 arg;
    s32 adj;
    s32 lo;
    s16 s;
    s16 panX;

    if (entity->scaleParallaxX == 0x10000) {
        camX = g_pGameState->camera_x;
    } else {
        camX = (s16)(((s32)g_pGameState->camera_x * entity->scaleParallaxX) >> 16);
    }
    m = ((s16 *)&entity->moveMarkerX)[1];
    val = entity->worldX;
    if (m != 0) {
        s = m;
        if (s > 0) {
            XformSlot *base = *(XformSlot **)((u8 *)entity + *(s16 *)&entity->moveCallbackX);
            arg = base[s - 1].arg;
            ft = base[s - 1].fn;
            FSM_RELAY(fn, ft);
        } else {
            fn = (XformCB)entity->moveCallbackX;
        }
        lo = ((s16 *)&entity->moveMarkerX)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        panX = (s16)fn((u8 *)entity + adj, (s16)val);
    } else {
        panX = val;
    }
    PlaySoundEffect(soundId, panX - camX, 0);
}

/* Caller-side prototype only (real definition in sound.c takes s16
 * panPos) - declaring it s32 here is what gets cc1 to truncate the pan
 * value and camX to s16 individually before subtracting (matching target);
 * an s16 prototype instead lets it defer to a single truncate-after-subtract,
 * which is mathematically identical mod 2^16 but 2 instructions short.
 * Bit pattern in the arg register is identical either way - MIPS o32 passes
 * both in a full word and SetVoicePanning only reads the low 16 bits. */
extern void SetVoicePanning(s32 voiceIndex, s32 panPos);

/* Same pan calc as PlayEntityPositionSound but updates SetVoicePanning on
 * an already-keyed SPU voice (voice handle in $a1; <0 skips). Used by
 * moving emitters that need live stereo follow - Shriney Guards, FINN
 * engine loop, etc. */
void UpdateEntitySoundPanning(Entity *entity, s32 voiceIndex) {
    s16 camX;
    s16 m;
    u16 val;
    FSM_REG(XformCB, fn, "$6"); /* $a2 home */
    FSM_REG(XformCB, ft, "$9"); /* $t1 then-fn (relays) */
    s32 arg;
    s32 adj;
    s32 lo;
    s16 s;
    s16 panX;

    if (voiceIndex < 0) {
        return;
    }
    if (entity->scaleParallaxX == 0x10000) {
        camX = g_pGameState->camera_x;
    } else {
        camX = (s16)(((s32)g_pGameState->camera_x * entity->scaleParallaxX) >> 16);
    }
    m = ((s16 *)&entity->moveMarkerX)[1];
    val = entity->worldX;
    if (m != 0) {
        s = m;
        if (s > 0) {
            XformSlot *base = *(XformSlot **)((u8 *)entity + *(s16 *)&entity->moveCallbackX);
            arg = base[s - 1].arg;
            ft = base[s - 1].fn;
            FSM_RELAY(fn, ft);
        } else {
            fn = (XformCB)entity->moveCallbackX;
        }
        lo = ((s16 *)&entity->moveMarkerX)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        panX = (s16)fn((u8 *)entity + adj, (s16)val);
    } else {
        panX = val;
    }
    SetVoicePanning(voiceIndex, (s16)panX - (s16)camX);
}

/* Construct a sprite-entity skeleton: base entity + animation FSM hooked
 * up, but no sprite asset or VRAM slot claimed. Used by paths that drive
 * sprite state manually rather than via SetEntitySpriteId. */
SpriteEntity *InitFullEntityWithAnimation(SpriteEntity *entity, s16 allocSize) {
    InitEntityStruct(&entity->base, allocSize);
    entity->base.collisionVtable = g_EntityVtable_SpriteBase;
    ClearSpriteContextWrapper((u8 *)&entity->base.pFrameTable);
    ZeroEntityField((u8 *)&entity->pFrameData);
    InitEntityAnimationState(entity);
    return entity;
}

/* Main sprite-entity constructor. Chains InitEntityStruct, hooks
 * EntityUpdateCallback as the tick, then either AllocSpriteRenderContext
 * (animated) or AllocateSpriteContext (staticSpriteFlag set), and latches
 * the initial sprite via SetEntitySpriteId. Also applies the GameState
 * layer0 RGB tint to the sprite render context. */
INCLUDE_ASM("asm/nonmatchings/entity", InitEntitySprite);

/* Sprite-entity constructor variant using CreateMultiFrameRenderContext
 * for sprites whose per-frame pixel size varies. Otherwise identical to
 * InitEntitySprite. */
INCLUDE_ASM("asm/nonmatchings/entity", InitEntityWithSprite);

/* Resets the animation FSM (current/loop/target frame, divisor, direction,
 * sequence step). Called by every sprite-entity constructor. Data-flow,
 * field ordering (matches SpriteEntity exactly), and frame size (0x30,
 * PaddedSlotPair-sized scratch for the queued/active/exit CallbackSlot
 * clears) all match by hand. Residual is a pure instruction-scheduling
 * swap: the a1/a2 (allocSize/align) constant loads for the AllocateFromHeap
 * call land on the wrong side of the lui/lw g_pBlbHeapBase pair no matter
 * where a do{}while(0) fence goes or whether the args are named temps or
 * inline. decomp-permuter (nonmatchings/InitEntityAnimationState/, gitignored
 * local scratch — rebuild via `python3 tools/decomp-permuter/import.py
 * src/entity.c asm/nonmatchings/entity/InitEntityAnimationState.s`) plateaus
 * at score 120 (down from 240) on the same swap after ~3k random iterations. */
INCLUDE_ASM("asm/nonmatchings/entity", InitEntityAnimationState);

/* SpriteEntity destructor. Adds main-RAM pixel decode buffer + sprite
 * asset (mode 4 = sprite-asset class on the heap) frees on top of the
 * VRAM-slot release + heap free that FreeWithCallback performs. */
void DestroyEntityAndFreeMemory(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = g_EntityVtable_SpriteBase;
    if (entity->pPixelBuffer != NULL) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity->pPixelBuffer, 0, 0);
    }
    FreeFromHeap(g_pBlbHeapBase, (u8 *)entity->pSpriteAsset, 4, 0);
    {
        SpriteRenderContext *ctx = entity->base.spriteContext;
        entity->base.collisionVtable = g_EntityVtable_PartialDestroy;
        if (ctx != NULL) {
            SpriteContextCallbackTable *callbacks = ctx->callbacks;
            s16 offset = callbacks->callbackTargetOffset;
            void (*func)(u8 *, s32) = callbacks->releaseVRAMSlot;
            func(&((u8 *)ctx)[offset], 3);
        }
    }
    entity->base.collisionVtable = g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Allocate the main-RAM scratch buffer for sprite decode. Size pulled from
 * the sprite context's width*height (ctx[2]*ctx[3], the rounded VRAM
 * dimensions). Lets us decode one frame at a time instead of keeping every
 * frame resident in VRAM. */
void AllocateEntityPixelBuffer(SpriteEntity *entity) {
    u8 *heap = g_pBlbHeapBase;
    SpriteRenderContext *ctx = entity->base.spriteContext;
    s32 size = (s32)ctx->width * (s32)ctx->height;
    entity->pixelBufferSize = size;
    entity->pPixelBuffer = AllocateFromHeap(heap, 1, size, 0);
}

extern void TickEntityAnimation(SpriteEntity *entity);
extern void ApplyPendingSpriteState(SpriteEntity *entity);
extern void UpdateSpriteFrameData(SpriteEntity *entity);

/* Per-frame sprite-entity tick. Calls TickEntityAnimation, dispatches the
 * renderCallback FSM at +0x1C/+0x20, then if animChangeFlags has the
 * apply-now bits set, commits the pending sprite state via
 * ApplyPendingSpriteState (+ optionally UpdateSpriteFrameData). The
 * "request now, commit at one deterministic point" pattern keeps the
 * renderer from seeing half-updated state. */
INCLUDE_ASM("asm/nonmatchings/entity", EntityUpdateCallback);

/* Apply the current frame's pivot delta to worldXY then reset the delta.
 * Direction respects facing/flipY so mirroring the art doesn't translate
 * the entity the wrong way. Keeps the entity rooted while its art slides
 * under it (classic "anchor point" frame metadata). */
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

/* Reserve a VRAM slot and allocate the main-RAM pixel buffer sized to the
 * rounded VRAM dimensions (width * round_up_4(height)). */
void InitEntitySpriteAndPixelBuffer(SpriteEntity *entity, s16 width, s16 height, s16 depth, u8 flags) {
    SpriteRenderContext *ctx = (SpriteRenderContext *)AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    SpriteRenderContext *sc;
    s32 size;
    entity->base.spriteContext = PrepareSpriteVRAMSlotForContext(ctx, width, (s16)((height + 3) & 0xFFFCu), depth, flags);
    sc = entity->base.spriteContext;
    size = (s32)sc->width * (s32)sc->height;
    entity->pixelBufferSize = size;
    entity->pPixelBuffer = AllocateFromHeap(g_pBlbHeapBase, 1, size, 0);
}

/* Same VRAM-slot + main-RAM pixel buffer pattern as
 * InitEntitySpriteAndPixelBuffer but the sprite-render context is the one
 * embedded in the entity at +0x78 (InitSpriteContext fills it). */
INCLUDE_ASM("asm/nonmatchings/entity", AllocSpriteRenderContext);

/* Multi-frame variant: VRAM context big enough to hold every frame of the
 * sprite at once. Used for sprites whose per-frame pixel size varies,
 * where allocating per-frame would thrash VRAM. */
INCLUDE_ASM("asm/nonmatchings/entity", CreateMultiFrameRenderContext);

extern SpriteRenderContext *InitSpriteContextDefaults(SpriteRenderContext *ctx, s16 spriteId);

/* Bare sprite context with default state - no pixel buffer because static
 * sprites blit straight from the loaded asset. Used when InitEntitySprite
 * is called with staticSpriteFlag set. */
void AllocateSpriteContext(Entity *entity, s16 spriteId) {
    SpriteRenderContext *ctx = (SpriteRenderContext *)AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    entity->spriteContext = InitSpriteContextDefaults(ctx, spriteId);
}
