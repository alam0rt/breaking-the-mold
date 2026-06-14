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

/* Entity constructor. Zeroes the 0x80 base header, installs the Destroyed +
 * PartialDestroy vtables, primes the four 16.16 scale fields at 0x50..0x64
 * to 1.0, and stores allocSize at +0x10. All Init* helpers chain here. */
INCLUDE_ASM("asm/nonmatchings/entity", InitEntityStruct);

/* Entity destructor (base form). Flips vtable to PartialDestroy so broadcast
 * loops drop us, runs the sprite context's destroy callback with mode=3
 * (release the reserved PSX texture-VRAM slot), then the Destroyed vtable.
 * flags&1 returns the entity itself to the BLB heap. */
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
INCLUDE_ASM("asm/nonmatchings/entity", CalculateParallaxXOffset);

INCLUDE_ASM("asm/nonmatchings/entity", CalculateParallaxXOffsetAlt);

/* value + camera_x. Byte-identical to GetWorldPositionX above. Its `addu
 * v0,v0,v1; jr ra` tail (label .L8001A350) is a SHARED epilogue that
 * CalculateParallaxXOffset/Alt `j` into, so it can't be converted to C until
 * those tail-merged neighbors are decompiled too (else: undefined ref to
 * .L8001A350 at link). */
INCLUDE_ASM("asm/nonmatchings/entity", WorldToScreenX);

/* Y-axis sibling of CalculateParallaxXOffsetAlt. Scales by +0x5C (entity
 * render scale, not parallax) then adds camera_y. */
s32 WorldToScreenYWithParallax(Entity *entity, s16 value) {
    if (entity->scalePowerupY == 0x10000) {
        return (s32)value + (s32)g_pGameState->camera_y;
    }
    return ((s32)value * entity->scalePowerupY >> 16) + (s32)g_pGameState->camera_y;
}

extern void *PrepareSpriteVRAMSlotForContext(void *ctx, s16 width, s16 height, s16 depth, u8 flags);

/* Allocate a 0x3C-byte sprite-render context on the BLB heap and reserve a
 * PSX texture-VRAM rectangle. Height is rounded up to a multiple of 4
 * because PSX GPU rectangle uploads must be 4 px aligned in Y. depth
 * selects TIM bit-depth (4/8/15bpp); flags carry CLUT info. */
void CreateEntityRenderContext(Entity *entity, s16 width, s16 height, s16 depth, u8 flags) {
    void *ctx = AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
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
 * doesn't need to know about platform riding to draw. */
INCLUDE_ASM("asm/nonmatchings/entity", CalculateEntityRenderBounds);

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
INCLUDE_ASM("asm/nonmatchings/entity", CollisionCheckWrapper);

INCLUDE_ASM("asm/nonmatchings/entity", EntityBroadcastBoxCollision);

INCLUDE_ASM("asm/nonmatchings/entity", CheckEntityPointCollisionWithOffsets);

INCLUDE_ASM("asm/nonmatchings/entity", EntityBroadcastPointCollision);

/* Leaf primitive: pack (X1,Y1,X2,Y2) into two $a1/$a2 word pairs and call
 * the shared CheckBoxCollision AABB tester. */
INCLUDE_ASM("asm/nonmatchings/entity", CheckEntityBoxCollision);

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
 * for "this object made a noise where it is". */
INCLUDE_ASM("asm/nonmatchings/entity", PlayEntityPositionSound);

/* Same pan calc but updates SetVoicePanning on an already-keyed SPU voice
 * (voice handle in $a1; <0 skips). Used by moving emitters that need
 * live stereo follow - Shriney Guards, FINN engine loop, etc. */
INCLUDE_ASM("asm/nonmatchings/entity", UpdateEntitySoundPanning);

/* Construct a sprite-entity skeleton: base entity + animation FSM hooked
 * up, but no sprite asset or VRAM slot claimed. Used by paths that drive
 * sprite state manually rather than via SetEntitySpriteId. */
SpriteEntity *InitFullEntityWithAnimation(SpriteEntity *entity, s16 allocSize) {
    InitEntityStruct(&entity->base, allocSize);
    entity->base.collisionVtable = g_EntityVtable_SpriteBase;
    ClearSpriteContextWrapper(&entity->base.pFrameTable);
    ZeroEntityField(&entity->pFrameData);
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
 * sequence step). Called by every sprite-entity constructor. */
INCLUDE_ASM("asm/nonmatchings/entity", InitEntityAnimationState);

/* SpriteEntity destructor. Adds main-RAM pixel decode buffer + sprite
 * asset (mode 4 = sprite-asset class on the heap) frees on top of the
 * VRAM-slot release + heap free that FreeWithCallback performs. */
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

/* Allocate the main-RAM scratch buffer for sprite decode. Size pulled from
 * the sprite context's width*height (ctx[2]*ctx[3], the rounded VRAM
 * dimensions). Lets us decode one frame at a time instead of keeping every
 * frame resident in VRAM. */
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

/* Same VRAM-slot + main-RAM pixel buffer pattern as
 * InitEntitySpriteAndPixelBuffer but the sprite-render context is the one
 * embedded in the entity at +0x78 (InitSpriteContext fills it). */
INCLUDE_ASM("asm/nonmatchings/entity", AllocSpriteRenderContext);

/* Multi-frame variant: VRAM context big enough to hold every frame of the
 * sprite at once. Used for sprites whose per-frame pixel size varies,
 * where allocating per-frame would thrash VRAM. */
INCLUDE_ASM("asm/nonmatchings/entity", CreateMultiFrameRenderContext);

extern void *InitSpriteContextDefaults(void *ctx, s16 spriteId);

/* Bare sprite context with default state - no pixel buffer because static
 * sprites blit straight from the loaded asset. Used when InitEntitySprite
 * is called with staticSpriteFlag set. */
void AllocateSpriteContext(Entity *entity, s16 spriteId) {
    void *ctx = AllocateFromHeap(g_pBlbHeapBase, 0x3C, 1, 0);
    entity->spriteContext = InitSpriteContextDefaults(ctx, spriteId);
}
