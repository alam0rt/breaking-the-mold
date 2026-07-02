#include "common.h"
#include "functions.h"
#include "Game/fsm_dispatch.h"
#include "Game/anim_entities.h"

extern u8 *g_pBlbHeapBase;
extern u8 g_EntityVtable_Destroyed[];
extern u8 g_EntityVtable_ResourceType1[];
extern u8 g_EntityVtable_ResourceType2[];
extern u8 g_EntityVtable_ResourceType3[];
extern u8 g_EntityVtable_ResourceType4[];
extern u8 g_EntityVtable_SpriteBase[];
extern u8 g_EntityVtable_PartialDestroy[];
extern u8 g_EntityVtable_SimpleDestruct[];
extern u8 g_EntityVtable_LevelDestroy[];

extern void FreeMultiAllocResource(u8 *ptr, s32 type);
extern void FreeResourceType2(u8 *ptr, s32 type);
extern void FreeResourceType3(u8 *ptr, s32 type);

typedef struct { s32 a; s32 b; } S32Pair;

/* Pending-frame setter: queues `value` as the next animation frame index
 * and raises ANIM_CHG_FRAME (0x008) on animChangeFlags. The latched value
 * is committed into +0xDA currentFrame on the next entity tick by
 * ApplyPendingSpriteState. Bit 0x200 is cleared so the value is taken
 * as a literal index (not a callback-resolved key). */
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

/* Pending-frame setter, callback variant: same as SetAnimationFrameIndex
 * but also sets bit 0x200, telling ApplyPendingSpriteState to resolve
 * the stored value via FindFrameIndexByValue rather than using it as a
 * literal index. Lets gameplay code reference frames by symbolic tag. */
void SetAnimationFrameCallback(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x208;

    entity->pendingFrame = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x209;
    }
}

/* NB: name MISLEADING -- writes to pendingLoopFrame (+0xC4, flag
 * ANIM_CHG_LOOP_FRAME 0x010), NOT pendingTargetFrame. Likely should be
 * SetAnimationLoopFrameIndex. Queues the frame the animation loops back
 * to. If a sprite change is also queued (bit 0x004), the value is mirrored
 * into pendingFrame so the new sprite starts at this loop point. */
void SetEntityTargetFrame(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;
    u16 orFlags = flags | 0x10;
    u16 newFlags = orFlags & 0xFBFF;

    entity->pendingLoopFrame = (u16)value;
    entity->animChangeFlags = newFlags;
    if (orFlags & 0x4) {
        /* @hack: volatile reload forces cc1 to re-fetch animChangeFlags after the store above, matching the target's lhu-after-sh sequence. */
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

/* Pending-loop-frame setter, callback variant: writes pendingLoopFrame
 * (+0xC4) and sets bits 0x410 (LOOP_FRAME | callback-lookup), so
 * ApplyPendingSpriteState resolves the value via callback. Mirrors the
 * sprite-change-primes-pendingFrame branch from SetEntityTargetFrame. */
void SetAnimationLoopFrame(AnimEntity *entity, u32 value) {
    u16 flags = entity->animChangeFlags;

    entity->pendingLoopFrame = value;
    flags |= 0x410;
    entity->animChangeFlags = flags;
    if (flags & 0x4) {
        /* @hack: volatile reload forces cc1 to re-fetch animChangeFlags after the store above, matching the target's lhu-after-sh sequence. */
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

/* NB: name MISLEADING -- writes to pendingTargetFrame (+0xC8, flag
 * ANIM_CHG_TARGET_FRAME 0x020), NOT a sprite ID. The real sprite-id setter
 * is SetEntitySpriteId @ 0x8001D080 (which uses +0xBC and flag 0x004).
 * This queues the end/stop frame for the current animation; likely should
 * be SetAnimationTargetFrameIndex. */
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

/* NB: name MISLEADING -- same pendingTargetFrame slot as SetAnimationSpriteId,
 * but with ANIM_CHG_TARGET_BY_VALUE (0x800) set so the stored value is
 * treated as a lookup key resolved via callback rather than a literal
 * frame index. Likely should be SetAnimationTargetFrameByValue. */
void SetAnimationSpriteCallback(AnimEntity *entity, u8 *callback) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x820;

    entity->pendingSpriteSource = (u32)callback;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x821;
    }
}

/* Pending-anim-active setter: queues an on/off toggle for the animation
 * state machine (+0xF5 pendingAnimActive, flag ANIM_CHG_ANIM_ACTIVE 0x100).
 * When committed to +0xF2 animActive, TickEntityAnimation gates the
 * frame-advance loop on it. */
void SetAnimationActive(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x100;

    entity->pendingAnimActive = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x101;
    }
}

/* NB: name MISLEADING -- writes to pendingLoopFlag (+0xF4, flag
 * ANIM_CHG_LOOP_FLAG 0x080), the "loop animation on completion" toggle,
 * not any render flags. Likely should be SetAnimationLoopFlag. */
void EntitySetRenderFlags(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x80;

    entity->pendingLoopFlag = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x81;
    }
}

/* Pending-direction setter (+0xF3 pendingDirection, flag
 * ANIM_CHG_DIRECTION 0x040). 0 = forward playback, 1 = reverse. */
void SetAnimationDirection(AnimEntity *entity, u8 value) {
    u16 flags = entity->animChangeFlags;
    u16 newFlags = flags | 0x40;

    entity->pendingDirection = value;
    entity->animChangeFlags = newFlags;
    if ((newFlags & 3) == 0) {
        entity->animChangeFlags = flags | 0x41;
    }
}

/* Per-entity animation tick. Gated by animActive (+0xF2); decrements
 * frameCountdown (+0xEE) and, on zero, fires the "anim complete" callback
 * (message 2) if currentFrame has reached targetFrame, otherwise calls
 * AdvanceAnimationFrame to step the frame and reloads metadata. First
 * stage of every sprite-entity's per-tick update, before
 * ApplyPendingSpriteState / UpdateSpriteFrameData. */
INCLUDE_ASM("asm/nonmatchings/anim", TickEntityAnimation);

/* Steps currentFrame (+0xDA) toward targetFrame (+0xDE) respecting
 * animDirection (+0xF0) and the animLoopFlag (+0xF1): when the target
 * is reached it returns to loopFrame (+0xDC) if looping is enabled,
 * otherwise stops. Wraps at the total frameCount boundary. The actual
 * frame-index integrator driving every sprite animation. */
void AdvanceAnimationFrame(AdvAnimState *e) {
    s16 current = e->field_DA;
    s16 target = e->field_DE;

    if (current == target) {
        if (e->field_F1 != 0) {
            e->field_DA = e->field_DC;
        }
        return;
    }

    if (e->field_F0 == 0) {
        s16 newVal = current + 1;
        e->field_DA = newVal;
        if (newVal >= e->field_D8) {
            e->field_DA = 0;
        }
    } else {
        s16 newVal = current - 1;
        e->field_DA = newVal;
        if (newVal < 0) {
            e->field_DA = e->field_D8 - 1;
        }
    }
}

/* Commit half of the request-now/apply-later animation pipeline. Walks
 * the bits set in animChangeFlags by the SetAnimation* setters and copies
 * each pending* field into its live counterpart (sprite id, frame, loop
 * frame, target frame, direction, loop flag, active). Resolves the
 * callback-lookup variants via FindFrameIndexByValue when bits
 * 0x200/0x400/0x800 are set, then clears animChangeFlags. */
INCLUDE_ASM("asm/nonmatchings/anim", ApplyPendingSpriteState);

/* Loads metadata for the now-current frame from the entity's frame
 * table at +0x78 (0x24 bytes/frame): per-frame width/height/duration,
 * render-bbox/hitbox deltas, frame timer reset, and the 16.16 per-frame
 * motion vectors frameMotionX/Y at +0xB4/+0xB8. Runs after
 * ApplyPendingSpriteState whenever the active frame changes. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateSpriteFrameData);

/* Main per-entity render+update routine (~3 KiB, the biggest function
 * in the unit). Drives the full sprite pipeline per tick:
 * TickEntityAnimation -> ApplyPendingSpriteState -> UpdateSpriteFrameData,
 * evaluates the world->screen transform with parallax and powerup/scale
 * factors, triggers RLE sprite decode when needed, and submits the
 * sprite primitive. Installed as the tick slot of every sprite entity's
 * vtable -- the heart of the per-frame render loop. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateEntityRender);

/* When the textureDirty byte (+0x76) is set, re-uploads the entity's
 * decoded pixel buffer to VRAM and reloads its CLUT/palette via the
 * embedded sprite render context. Called once per frame from each
 * entity's render path (texture slot of the standard vtable). */
INCLUDE_ASM("asm/nonmatchings/anim", UploadEntityTextureIfDirty);

/* Searches the entity's frame-metadata table for the first frame whose
 * tag/key (word at +0x00 of each entry) equals `value`, returning its
 * index, or -1 if not found. When staticSpriteFlag (+0xF7) is set it
 * walks via GetSpriteFrameDataByIndex instead of indexing pFrameTable
 * directly. Used by ApplyPendingSpriteState to resolve the callback-
 * lookup variants of the pending-frame setters. */
INCLUDE_ASM("asm/nonmatchings/anim", FindFrameIndexByValue);

extern void StepAnimationSequence(u8 *entity);

/* Begins playback of a scripted animation callback sequence. Zeroes
 * sequenceStep (+0xE2), records sequenceLength (+0xE4) and the table
 * pointer (+0x94), then dispatches the first entry via
 * StepAnimationSequence. Used for death/transformation/cutscene scripts
 * where a single state plays a series of [marker, fn] entries in turn. */
void StartAnimationSequence(u8 *entity, s32 animData, s16 startFrame) {
    *(s16 *)&entity[0xE2] = 0;
    *(s16 *)&entity[0xE4] = startFrame;
    *(s32 *)&entity[0x94] = animData;
    StepAnimationSequence(entity);
}

/* Advances the active animation sequence: drains any pending exit/
 * finalizer hook (+0xA8), copies the current sequence-table entry into
 * the active-state slot (+0xA0/+0xA4), invokes it, then bumps
 * sequenceStep. When the sequence completes the table pointer at +0x94
 * is cleared. */
INCLUDE_ASM("asm/nonmatchings/anim", StepAnimationSequence);

/* Per-tick dispatcher for an entity's state-machine callback queue.
 * Promotes the queued slot at +0x98 into the active slot at +0xA0,
 * steps an active sequence via StepAnimationSequence, runs the current
 * activeStateCallback, and dispatches the exit/finalizer hook at +0xA8
 * around state changes. The actual driver for every sprite-entity FSM. */
INCLUDE_ASM("asm/nonmatchings/anim", EntityProcessCallbackQueue);

/* Replaces the entity's current state with a new [marker, fn] pair.
 * Dispatches and clears the existing exit/finalizer hook (+0xA8) first;
 * if that hook didn't itself install another state, clears the sequence
 * and queued slots, installs the new pair at +0xA0/+0xA4, and immediately
 * dispatches the new state-enter handler. The primary state-transition
 * primitive used by player FSM, enemy AI, bosses, menus, and pickups
 * (heavily called -- 189 refs from 146 callers). */
INCLUDE_ASM("asm/nonmatchings/anim", EntitySetState);

/* Installs an exit/finalizer callback into the +0xA8/+0xAC slot. The
 * existing hook (if any) is dispatched and cleared before replacement.
 * Used for cleanup that must run before the next state change: stopping
 * sounds, clearing flags, restoring hitboxes, removing helper entities,
 * re-enabling input, completing checkpoint/powerup effects. */
/* EntitySetCallback @ 0x8001EC18 (0xA8) — fires the entity's pending
 * exit-callback (InvokeEntityRenderCallback forwarder on the +0xA8/0xAA/0xAC
 * slot, fn pinned $a1, then-fn relays via $t1), then installs the new
 * (marker, fn) from arg1/arg2. SHELVED: dispatch half matches, but cc1 promotes
 * the two new-slot args to callee-saved $s1/$s2 (frame 0x28) whereas the target
 * spills them to the incoming-arg stack area and reloads after the call (frame
 * 0x20) — a 4-instr storage-strategy gap the permuter can't bridge (extra
 * instrs). Dispatch half follows the [[invoke-entity-render-callback-wall]]
 * recipe: marker s16@0xAA, table base @0xAC, lo@0xA8, fn pinned $a1 forwarded
 * as 2nd arg, then-fn relays via $t1, call fn(e+adj, fn, marker). The blocker
 * is forcing cc1 to spill the two new-slot args to stack instead of $s1/$s2. */
INCLUDE_ASM("asm/nonmatchings/anim", EntitySetCallback);

/* Constructor for a "standard" (large, >128x128) parallax background
 * layer entity. Initializes the entity header, allocates a tilemap
 * rendering context via InitTilemapLayerRendering, installs
 * UpdateParallaxScrollWithWrap_Standard as the per-frame tick, and wires
 * up the EntityVtable_ResourceType1 destructor slot. */
INCLUDE_ASM("asm/nonmatchings/anim", InitLayerRenderContext_Standard);

/* Destructor for entities backed by a multi-allocation resource bundle
 * (sprite/anim/CLUT pack at +0x1C). Calls FreeMultiAllocResource, walks
 * the destruction-chain vtables (ResourceType1 -> Destroyed), and
 * optionally frees the entity body from the BLB heap when flag bit 0 is
 * set. Installed in the destroy slot of the standard layer vtable. */
void EntityDestructor_FreeMultiAlloc(LayerResourceEntity *entity, s32 flags) {
    u8 *resource;
    resource = entity->resource;
    entity->collisionVtable = (s32)g_EntityVtable_ResourceType1;
    if (resource) {
        FreeMultiAllocResource(resource, 3);
    }
    entity->collisionVtable = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Per-frame tick for "standard" parallax background layers. Reads camera
 * X from g_pGameState, multiplies by the layer's scroll factor, advances
 * a scroll accumulator with horizontal wrap-around, and re-blits the
 * visible tiles for the new scroll position. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateParallaxScrollWithWrap_Standard);

/* Constructor for a "medium" (<=128x128) parallax background layer.
 * Same overall shape as InitLayerRenderContext_Standard but allocates
 * the medium sprite-context size and installs
 * UpdateParallaxScrollWithWrap_Medium as the tick callback. */
INCLUDE_ASM("asm/nonmatchings/anim", InitLayerRenderContext_Medium);

/* Destructor for ResourceType2-backed entities. Mirrors
 * EntityDestructor_FreeMultiAlloc but calls FreeResourceType2 on the
 * resource pointer at +0x1C. Installed in the destroy slot of the medium
 * layer vtable. */
void EntityDestructor_FreeResourceType2(LayerResourceEntity *entity, s32 flags) {
    u8 *resource;
    resource = entity->resource;
    entity->collisionVtable = (s32)g_EntityVtable_ResourceType2;
    if (resource) {
        FreeResourceType2(resource, 3);
    }
    entity->collisionVtable = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Medium-layer counterpart to UpdateParallaxScrollWithWrap_Standard. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateParallaxScrollWithWrap_Medium);

/* Constructor for a "small" (<=64x64) parallax background layer.
 * Installs UpdateParallaxScrollWithWrap_Small as the tick callback. */
INCLUDE_ASM("asm/nonmatchings/anim", InitLayerRenderContext_Small);

/* Destructor for ResourceType3-backed entities. Calls FreeResourceType3
 * on the resource pointer at +0x1C; installed in the destroy slot of
 * the small layer vtable. */
void EntityDestructor_FreeResourceType3(LayerResourceEntity *entity, s32 flags) {
    u8 *resource;
    resource = entity->resource;
    entity->collisionVtable = (s32)g_EntityVtable_ResourceType3;
    if (resource) {
        FreeResourceType3(resource, 3);
    }
    entity->collisionVtable = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Small-layer counterpart to UpdateParallaxScrollWithWrap_Standard. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateParallaxScrollWithWrap_Small);

/* Constructor for an animated foreground sprite-layer entity (the
 * scrolling sprite strips: clouds, boats, banners, etc.). Allocates a
 * ResourceType4-backed entity, installs EntityTick_AnimationFrameAdvance
 * as the tick callback, then bootstraps the first frame by copying frame
 * metadata into +0x24..+0x2F. */
INCLUDE_ASM("asm/nonmatchings/anim", InitLayerScrollContext);

/* Destructor for ResourceType4 (animated sprite-layer) entities. Frees
 * the sprite-render allocation at +0x20 directly through the BLB heap
 * (no per-resource cleanup hook), then vtable-swaps to Destroyed and
 * optionally frees the entity body. */
void FreeResourceType4(LayerResourceEntity *entity, s32 flags) {
    u8 *resource;
    resource = entity->renderContext;
    entity->collisionVtable = (s32)g_EntityVtable_ResourceType4;
    if (resource) {
        FreeFromHeap(g_pBlbHeapBase, resource, 0, 0);
    }
    entity->collisionVtable = (s32)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* Per-frame tick for animated sprite layers (resources using the +0x1C
 * frame table). Decrements the per-frame timer at +0x31; on rollover,
 * advances the +0x30 frame index, fires the "anim complete" callback
 * (message 2) when the new slot is empty, then reloads +0x24..+0x2F
 * from the next frame entry. The poor cousin of TickEntityAnimation,
 * specialised for the simpler layer struct. */
INCLUDE_ASM("asm/nonmatchings/anim", EntityTick_AnimationFrameAdvance);

/* Per-frame screen-position update for sprite-layer entities. If the
 * +0x20 render context exists and the +0x32 palette-dirty byte is clear,
 * adds world position +0x2C/+0x2E to scroll offset +0x24/+0x26 and stores
 * the result as the render context's screen XY; otherwise tails into
 * UpdateEntityScreenPositionWithPalette which additionally refreshes the
 * palette pointer. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateEntityScreenPosition);

/* Computes the on-screen position from world coords minus camera, writes
 * it into the layer's render context (+0x20), and indexes into the
 * palette table at (+0x1C base + +0x30*16) to install the current
 * palette pointer at render-context +0x10. The palette-aware variant of
 * UpdateEntityScreenPosition. */
INCLUDE_ASM("asm/nonmatchings/anim", UpdateEntityScreenPositionWithPalette);

/* Constructor for the platformer PlayerEntity (the 0x3E8/0x44C alloc).
 * Runs the base Entity init, clears the sprite sub-struct,
 * InitEntityAnimationState's it, allocates the sprite/render context
 * with the well-known player sprite ID 0x21842018 (cf. physics-constants),
 * installs EntityUpdateCallback as the per-frame tick, and wires the
 * moveCallbackX/Y FSM slots to GetWorldPositionX/Y. */
INCLUDE_ASM("asm/nonmatchings/anim", InitPlayerEntity);

/* Runs the entity's moveCallbackX (+0x24/+0x28) and moveCallbackY
 * (+0x2C/+0x30) FSM slots in turn to transform (x, y) into world
 * coordinates, then samples the tile attribute at the transformed point
 * via GetTileAttributeAtPosition. The "what tile am I standing on / about
 * to hit" query used by player physics, bounce detection, and enemy
 * ground checks. */
INCLUDE_ASM("asm/nonmatchings/anim", EntityApplyMovementCallbacks);

/* Composite destructor for entities that own both a sprite/pixel
 * buffer (+0xB0) and a child entity reference (+0x34). Frees the pixel
 * buffer, frees the +0x90 frame-table allocation, then dispatches the
 * child's own destructor through its vtable before vtable-swapping self
 * to Destroyed and optionally freeing the entity body. Used by the
 * player entity and other sprite-with-shadow / sprite-with-halo setups. */
void EntityDestructor_FreeWithChildRef(SpriteEntity *entity, s32 flags) {
    SpriteRenderContext *child;
    SpriteContextCallbackTable *childCallbacks;
    u8 *resource;

    resource = entity->pPixelBuffer;
    entity->base.collisionVtable = g_EntityVtable_SpriteBase;
    if (resource) {
        FreeFromHeap(g_pBlbHeapBase, resource, 0, 0);
    }
    FreeFromHeap(g_pBlbHeapBase, (u8 *)entity->pSpriteAsset, 4, 0);
    child = entity->base.spriteContext;
    entity->base.collisionVtable = g_EntityVtable_PartialDestroy;
    if (child) {
        childCallbacks = child->callbacks;
        childCallbacks->releaseVRAMSlot(&((u8 *)child)[childCallbacks->callbackTargetOffset], 3);
    }
    entity->base.collisionVtable = g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

/* First of three identical accessor sets covering the entity FSM/render
 * slot fields (+0x1C..+0x3B). Each tiny function exists as a standalone
 * symbol because callers hold its address (slot-installer functions wire
 * them into vtables / FSM tables so they can be invoked through a
 * uniform fn-pointer signature). Returns the renderCallback fn ptr at
 * +0x20. */
s32 GetEntityRenderCallbackPtr(SpriteEntity *e) {
    return (s32)e->base.renderCallback;
}

/* Setter trampoline: u8 -> entity+0x3B. */
void SetEntityStateByte3B(SpriteEntity *e, u8 value) {
    ((u8 *)&e->base.renderOffsetY)[1] = value;
}

/* Setter trampoline: u8 -> entity+0x3A. */
void SetEntityStateByte3A(SpriteEntity *e, u8 value) {
    ((u8 *)&e->base.renderOffsetY)[0] = value;
}

/* Setter trampoline: s16 -> entity+0x32 (moveMarkerY low half). */
void SetEntityMoveMarkerYOffset(SpriteEntity *e, s16 value) {
    ((s16 *)&e->base.moveCallbackY)[1] = value;
}

/* Setter trampoline: s16 -> entity+0x30 (moveCallbackY low half). */
void SetEntityMoveCallbackYOffset(SpriteEntity *e, s16 value) {
    ((s16 *)&e->base.moveCallbackY)[0] = value;
}

/* Setter trampoline: s32 -> entity+0x24 (moveMarkerX). */
void SetEntityMoveMarkerX(SpriteEntity *e, s32 value) {
    e->base.moveMarkerX = value;
}

/* Setter trampoline: s32 -> entity+0x20 (renderCallback fn ptr). */
void SetEntityRenderCallbackPtr(SpriteEntity *e, s32 value) {
    e->base.renderCallback = (EntityCallback)value;
}

/* Getter trampoline: returns entity+0x1C (renderMarker). */
s32 GetEntityRenderMarker(SpriteEntity *e) {
    return e->base.renderMarker;
}

/* Second of three identical accessor sets (see GetEntityRenderCallbackPtr). Setter
 * trampoline: u8 -> entity+0x3B. */
void SetEntityStateByte3B_80020124(SpriteEntity *e, u8 value) {
    ((u8 *)&e->base.renderOffsetY)[1] = value;
}

/* Setter trampoline: u8 -> entity+0x3A. */
void SetEntityStateByte3A_8002012c(SpriteEntity *e, u8 value) {
    ((u8 *)&e->base.renderOffsetY)[0] = value;
}

/* Setter trampoline: s16 -> entity+0x32. */
void SetEntityMoveMarkerYOffset_80020134(SpriteEntity *e, s16 value) {
    ((s16 *)&e->base.moveCallbackY)[1] = value;
}

/* Setter trampoline: s16 -> entity+0x30. */
void SetEntityMoveCallbackYOffset_8002013c(SpriteEntity *e, s16 value) {
    ((s16 *)&e->base.moveCallbackY)[0] = value;
}

/* Setter trampoline: s32 -> entity+0x24. */
void SetEntityMoveMarkerX_80020144(SpriteEntity *e, s32 value) {
    e->base.moveMarkerX = value;
}

/* Setter trampoline: s32 -> entity+0x20. */
void SetEntityRenderCallbackPtr_8002014c(SpriteEntity *e, s32 value) {
    e->base.renderCallback = (EntityCallback)value;
}

/* Getter trampoline: returns entity+0x1C. */
s32 GetEntityRenderMarker_80020154(SpriteEntity *e) {
    return e->base.renderMarker;
}

/* Third of three identical accessor sets (see GetEntityRenderCallbackPtr). Setter
 * trampoline: u8 -> entity+0x3B. */
void SetEntityStateByte3B_80020160(SpriteEntity *e, u8 value) {
    ((u8 *)&e->base.renderOffsetY)[1] = value;
}

/* Setter trampoline: u8 -> entity+0x3A. */
void SetEntityStateByte3A_80020168(SpriteEntity *e, u8 value) {
    ((u8 *)&e->base.renderOffsetY)[0] = value;
}

/* Setter trampoline: s16 -> entity+0x32. */
void SetEntityMoveMarkerYOffset_80020170(SpriteEntity *e, s16 value) {
    ((s16 *)&e->base.moveCallbackY)[1] = value;
}

/* Setter trampoline: s16 -> entity+0x30. */
void SetEntityMoveCallbackYOffset_80020178(SpriteEntity *e, s16 value) {
    ((s16 *)&e->base.moveCallbackY)[0] = value;
}

/* Setter trampoline: s32 -> entity+0x24. */
void SetEntityMoveMarkerX_80020180(SpriteEntity *e, s32 value) {
    e->base.moveMarkerX = value;
}

/* Setter trampoline: s32 -> entity+0x20. */
void SetEntityRenderCallbackPtr_80020188(SpriteEntity *e, s32 value) {
    e->base.renderCallback = (EntityCallback)value;
}

/* Getter trampoline: returns entity+0x1C. */
s32 GetEntityRenderMarker_80020190(SpriteEntity *e) {
    return e->base.renderMarker;
}

/* Getter: returns the entity's decoded pixel buffer pointer (+0xB0
 * pPixelBuffer). Used by render code to access the RLE-decoded bitmap. */
s32 GetEntityPixelBuffer(SpriteEntity *e) {
    return (s32)e->pPixelBuffer;
}

/* Setter: u8 -> entity+0xF6 (visibility byte, 0 = visible).
 * The render-time "hide this entity this frame" toggle. */
void SetEntityHidden(SpriteEntity *e, u8 value) {
    e->visibility = value;
}

/* Installs an 8-byte [marker, fn] pair into the queued-state slot at
 * SpriteEntity+0x98 (queuedStateMarker/queuedStateCallback). Promoted
 * into the active slot by EntityProcessCallbackQueue on the next tick. */
void QueueEntityStateCallback(SpriteEntity *e, S32Pair val) {
    *(S32Pair *)&e->queuedStateMarker = val;
}

/* Getter: returns currentFrame at +0xDA (the active animation frame
 * index). The "what frame am I on" query for gameplay logic. */
s16 GetEntityCurrentFrame(SpriteEntity *e) {
    return e->currentFrame;
}

/* Getter: returns currentSpriteId at +0xCC (the active sprite-bank hash). */
s32 GetEntityCurrentSpriteId(SpriteEntity *e) {
    return (s32)e->currentSpriteId;
}

/* Getter: returns targetX at +0x70. */
u16 GetEntityTargetX(SpriteEntity *e) {
    return e->base.targetX;
}

/* Getter: returns targetY at +0x72. */
u16 GetEntityTargetY(SpriteEntity *e) {
    return e->base.targetY;
}

/* Setter: u16 -> entity+0x72 (targetY). */
void SetEntityTargetY(SpriteEntity *e, u16 value) {
    e->base.targetY = value;
}

/* Getter: returns scalePowerupY at +0x5C (16.16 fixed-point shrink/grow
 * Y scale). */
s32 GetEntityScalePowerupY(SpriteEntity *e) {
    return e->base.scalePowerupY;
}

/* Getter: returns scalePowerupX at +0x58 (16.16 fixed-point shrink/grow
 * X scale). */
s32 GetEntityScalePowerupX(SpriteEntity *e) {
    return e->base.scalePowerupX;
}

/* Setter: s32 -> entity+0x5C (scalePowerupY). */
void SetEntityScalePowerupY(SpriteEntity *e, s32 value) {
    e->base.scalePowerupY = value;
}

/* Setter: s32 -> entity+0x58 (scalePowerupX). */
void SetEntityScalePowerupX(SpriteEntity *e, s32 value) {
    e->base.scalePowerupX = value;
}

/* Setter: writes the same scalar to both scalePowerupX (+0x58) and
 * scalePowerupY (+0x5C) -- uniform powerup scale. */
void SetEntityScalePowerupUniform(SpriteEntity *e, s32 value) {
    e->base.scalePowerupX = value;
    e->base.scalePowerupY = value;
}

/* Getter: returns scaleRender2 at +0x54 (secondary render scale). */
s32 GetEntityScaleRender2(SpriteEntity *e) {
    return e->base.scaleRender2;
}

/* Getter: returns scaleRender at +0x50 (primary render scale). */
s32 GetEntityScaleRender(SpriteEntity *e) {
    return e->base.scaleRender;
}

/* Setter: s32 -> entity+0x54 (scaleRender2). */
void SetEntityScaleRender2(SpriteEntity *e, s32 value) {
    e->base.scaleRender2 = value;
}

/* Setter: s32 -> entity+0x50 (scaleRender). */
void SetEntityScaleRender(SpriteEntity *e, s32 value) {
    e->base.scaleRender = value;
}

/* Setter: writes the same scalar to both scaleRender (+0x50) and
 * scaleRender2 (+0x54) -- uniform render scale. */
void SetEntityScaleRenderUniform(SpriteEntity *e, s32 value) {
    e->base.scaleRender = value;
    e->base.scaleRender2 = value;
}

/* Getter: returns flipY at +0x75. */
u8 GetEntityFlipY(SpriteEntity *e) {
    return e->base.flipY;
}

/* Getter: returns facing at +0x74 (0 = right, 1 = left). */
u8 GetEntityFacing(SpriteEntity *e) {
    return e->base.facing;
}

/* No-op callback that returns 0. Installed in unused FSM slots (event
 * handler, state callbacks) so dispatch sites can call unconditionally
 * without a null check. Default eventCallback for freshly initialised
 * entities. */
s32 StubReturnZero(void) {
    return 0;
}

/* Atomic position setter: writes worldX (+0x68) and worldY (+0x6A) in
 * one call. */
void SetEntityWorldPosition(SpriteEntity *e, s16 x, s16 y) {
    e->base.worldX = x;
    e->base.worldY = y;
}

/* Setter: s16 -> entity+0x6A (worldY). */
void SetEntityWorldY(SpriteEntity *e, s16 value) {
    e->base.worldY = value;
}

/* Setter: s16 -> entity+0x68 (worldX). */
void SetEntityWorldX(SpriteEntity *e, s16 value) {
    e->base.worldX = value;
}

typedef struct Vec4s16 {
    s16 x;  /* 0x0 */
    s16 y;  /* 0x2 */
    s16 w;  /* 0x4 */
    s16 h;  /* 0x6 */
} Vec4s16;

/* In-place scales a screen-space Vec4s16 (x,y,w,h) by the entity's
 * powerup-scale fractions: x/w divided by scalePowerupX (+0x58),
 * y/h divided by scalePowerupY (+0x5C). The (val << 16) / scale pattern
 * is the inverse of a 16.16 multiply. Used to project a sprite
 * rectangle into a shrunken/grown bounding box. */
void ScaleRectByEntityPowerup(SpriteEntity *e, Vec4s16 *v) {
    v->x = (v->x << 16) / e->base.scalePowerupX;
    v->w = (v->w << 16) / e->base.scalePowerupX;
    v->y = (v->y << 16) / e->base.scalePowerupY;
    v->h = (v->h << 16) / e->base.scalePowerupY;
}

/* Inverse-scale a single Y coordinate by scalePowerupY (+0x5C). */
s32 InverseScaleYByEntityPowerup(SpriteEntity *e, s32 value) {
    return (value << 16) / e->base.scalePowerupY;
}

/* Inverse-scale a single X coordinate by scalePowerupX (+0x58). */
s32 InverseScaleXByEntityPowerup(SpriteEntity *e, s32 value) {
    return (value << 16) / e->base.scalePowerupX;
}

/* Copies 8 bytes from src+0x38 into dst -- the render-bounds block
 * (renderOffsetX/Y + renderWidth/Height). Returns dst. "Snapshot the
 * sprite's render bbox" helper. */
Vec4s16 *CopyEntityRenderBounds(Vec4s16 *dst, Entity *src) {
    __builtin_memcpy(dst, &src->renderOffsetX, 8);
    return dst;
}

/* The public "where is this entity Y" query. Dispatches through the
 * moveCallbackY FSM slot (+0x2C/+0x30) so the answer respects any
 * platform-rider / transform callbacks installed on the entity; falls
 * back to the raw worldY (+0x6A) if no callback is installed. Adds the
 * marker low s16 as an offset before calling. Same FSM-slot dispatch +
 * register-pin match as TransformYCoord, but val is the loaded worldY. */
typedef s16 (*XformCB)();
typedef struct { s32 arg; XformCB fn; } XformSlot;

s16 GetEntityYPosition(Entity *e) {
    s16 m = ((s16 *)&e->moveMarkerY)[1];
    s32 val = *(u16 *)&e->worldY;
    FSM_REG(XformCB, fn, "$6"); /* $a2 home */
    FSM_REG(XformCB, ft, "$9"); /* $t1 then-fn (relays) */
    FSM_REG(s32, r, "$2");      /* $v0 home: forces move v0,a1; sll v0,v0 */
    s32 arg;
    s32 adj;
    s32 lo;
    s16 s;
    if (m != 0) {
        s = m;
        if (s > 0) {
            XformSlot *base =
                *(XformSlot **)((u8 *)e + *(s16 *)&e->moveCallbackY);
            arg = base[s - 1].arg;
            ft = base[s - 1].fn;
            FSM_RELAY(fn, ft); /* emits move $a2,$t1 */
        } else {
            fn = (XformCB)e->moveCallbackY;
        }
        lo = ((s16 *)&e->moveMarkerY)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        return (s16)fn((u8 *)e + adj, (s16)val);
    }
    r = val;
    FSM_KEEP_LIVE(r); /* materialize r in $v0 -> sll v0,v0 */
    return (s16)r;
}

/* The public "where is this entity X" query. Mirror of GetEntityYPosition
 * over the moveCallbackX FSM slot (+0x24/+0x28); returns the raw worldX
 * (+0x68) when no callback is installed. */
s16 GetEntityXPosition(Entity *e) {
    s16 m = ((s16 *)&e->moveMarkerX)[1];
    s32 val = *(u16 *)&e->worldX;
    FSM_REG(XformCB, fn, "$6"); /* $a2 home */
    FSM_REG(XformCB, ft, "$9"); /* $t1 then-fn (relays) */
    FSM_REG(s32, r, "$2");      /* $v0 home: forces move v0,a1; sll v0,v0 */
    s32 arg;
    s32 adj;
    s32 lo;
    s16 s;
    if (m != 0) {
        s = m;
        if (s > 0) {
            XformSlot *base =
                *(XformSlot **)((u8 *)e + *(s16 *)&e->moveCallbackX);
            arg = base[s - 1].arg;
            ft = base[s - 1].fn;
            FSM_RELAY(fn, ft); /* emits move $a2,$t1 */
        } else {
            fn = (XformCB)e->moveCallbackX;
        }
        lo = ((s16 *)&e->moveMarkerX)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        return (s16)fn((u8 *)e + adj, (s16)val);
    }
    r = val;
    FSM_KEEP_LIVE(r); /* materialize r in $v0 -> sll v0,v0 */
    return (s16)r;
}

/* Getter: returns worldY at +0x6A (raw, no callback dispatch). */
s16 GetEntityWorldYRaw(SpriteEntity *e) {
    return e->base.worldY;
}

/* Getter: returns worldX at +0x68 (raw, no callback dispatch). */
s16 GetEntityWorldXRaw(SpriteEntity *e) {
    return e->base.worldX;
}

/* Getter: returns spriteContext at +0x34 (entity's BasicEntity render
 * context, i.e. the +0x10-byte sprite buffer used by the renderer). */
s32 GetEntitySpriteContext(SpriteEntity *e) {
    return (s32)e->base.spriteContext;
}

/* Installs an 8-byte [marker, fn] pair into the moveCallbackY FSM slot
 * at entity+0x2C/+0x30. Subsequent GetEntityYPosition / TransformYCoord
 * calls will route through this callback. */
void SetEntityMoveCallbackY(SpriteEntity *e, S32Pair val) {
    *(S32Pair *)&e->base.moveMarkerY = val;
}

/* Y-coordinate transformer. Dispatches an arbitrary Y value through the
 * moveCallbackY FSM slot (+0x2C/+0x30) and returns the transformed value;
 * if no callback is installed, returns the input unchanged. Used by
 * collision/render code to project an arbitrary Y into the entity's
 * local space (e.g. platform-rider offset).
 *
 * Same FSM-slot dispatch shape as InvokeEntityRenderCallback, matched with the
 * same register-pinning trick: `fn` pinned to $a2 (call's fn home), `ft` pinned
 * to $t1 (the then-branch slot fn that relays into $a2 via a delay-slot move),
 * a volatile barrier on `ft` to block the coalesce, and a separate `s16 s = m;`
 * to route the survivor through $v0. The 2-arg callback signature
 * (s16 (*)(u8*, s16)) and the s16 passthrough/return are the only differences. */
s16 TransformYCoord(Entity *e, s16 val) {
    s16 m = ((s16 *)&e->moveMarkerY)[1];
    FSM_REG(XformCB, fn, "$6"); /* $a2 home */
    FSM_REG(XformCB, ft, "$9"); /* $t1 then-fn (relays) */
    s32 arg;
    s32 adj;
    s32 lo;
    s16 s;
    if (m != 0) {
        s = m;
        if (s > 0) {
            XformSlot *base =
                *(XformSlot **)((u8 *)e + *(s16 *)&e->moveCallbackY);
            arg = base[s - 1].arg;
            ft = base[s - 1].fn;
            FSM_RELAY(fn, ft); /* emits move $a2,$t1 */
        } else {
            fn = (XformCB)e->moveCallbackY;
        }
        lo = ((s16 *)&e->moveMarkerY)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        return (s16)fn((u8 *)e + adj, (s16)val);
    }
    return val;
}

/* Installs an 8-byte [marker, fn] pair into the moveCallbackX FSM slot
 * at entity+0x24/+0x28. Counterpart to SetEntityMoveCallbackY for the X axis. */
void SetEntityMoveCallbackX(SpriteEntity *e, S32Pair val) {
    *(S32Pair *)&e->base.moveMarkerX = val;
}

/* X-coordinate transformer; mirror of TransformYCoord over the
 * moveCallbackX FSM slot (+0x24/+0x28). Same register-pinning match recipe. */
s16 TransformXCoord(Entity *e, s16 val) {
    s16 m = ((s16 *)&e->moveMarkerX)[1];
    FSM_REG(XformCB, fn, "$6"); /* $a2 home */
    FSM_REG(XformCB, ft, "$9"); /* $t1 then-fn (relays) */
    s32 arg;
    s32 adj;
    s32 lo;
    s16 s;
    if (m != 0) {
        s = m;
        if (s > 0) {
            XformSlot *base =
                *(XformSlot **)((u8 *)e + *(s16 *)&e->moveCallbackX);
            arg = base[s - 1].arg;
            ft = base[s - 1].fn;
            FSM_RELAY(fn, ft); /* emits move $a2,$t1 */
        } else {
            fn = (XformCB)e->moveCallbackX;
        }
        lo = ((s16 *)&e->moveMarkerX)[0];
        if (s > 0) {
            adj = (s16)arg + lo;
        } else {
            adj = lo;
        }
        return (s16)fn((u8 *)e + adj, (s16)val);
    }
    return val;
}

/* Installs an 8-byte [marker, fn] pair into the renderCallback FSM slot
 * at entity+0x1C/+0x20. The function called by
 * InvokeEntityRenderCallback / the render-list walker each frame. */
void SetEntityRenderCallbackSlot(SpriteEntity *e, S32Pair val) {
    *(S32Pair *)&e->base.renderMarker = val;
}

/* Dispatches the entity's render callback through the FSM slot at
 * +0x1C/+0x20: resolves the marker (direct call or slot-table entry),
 * adjusts the entity pointer by the encoded offset, and invokes the
 * stored fn. The per-frame "draw this entity" entry point. */
/* MATCHED. This is a 4-ARGUMENT FORWARDER (not the 1-arg `fn(entity)` Ghidra
 * shows): a2/a3 are live inputs forwarded to the resolved callback. The match
 * required forcing the register-coloring that cc1's allocator wouldn't pick on
 * its own:
 *   - `fn` pinned to $a1 (call's fn-pointer home), `ft` pinned to $a3 (the
 *     then-branch slot-table fn that relays into $a1).
 *   - the empty `volatile` asm barrier on `ft` keeps it live past its def so
 *     the coalescer can't merge it away — this forces the `move $a1,$a3` relay
 *     in the `j` delay slot instead of a direct load into $a1.
 *   - the separate `s16 s = m;` copy reproduces the survivor routed through
 *     $v0 first (`move $v0,$a1; move $t0,$v0`) rather than straight to $t0. */
typedef void (*RenderCB)();
typedef struct { s32 arg; RenderCB fn; } RenderFrameSlot;

void InvokeEntityRenderCallback(Entity *e, RenderCB a1_, s32 a2, RenderCB a3_) {
    s16 m = ((s16 *)&e->renderMarker)[1];
    FSM_REG(RenderCB, fn, "$5"); /* $a1 home */
    FSM_REG(RenderCB, ft, "$7"); /* $a3 then-fn (relays) */
    s32 adj;
    s32 lo;
    s16 s;
    if (m == 0) {
        return;
    }
    s = m;
    if (s > 0) {
        RenderFrameSlot *base =
            *(RenderFrameSlot **)((u8 *)e + *(s16 *)&e->renderCallback);
        a2 = base[s - 1].arg;
        ft = base[s - 1].fn;
        FSM_RELAY(fn, ft); /* emits move $a1,$a3 */
    } else {
        fn = (RenderCB)e->renderCallback;
    }
    lo = ((s16 *)&e->renderMarker)[0];
    if (s > 0) {
        adj = (s16)a2 + lo;
    } else {
        adj = lo;
    }
    fn((Entity *)((u8 *)e + adj), fn, a2, ft);
}

/* Getter: returns the low 16 bits of moveMarkerY at +0x2C as u16
 * (the FSM marker's offset half). */
u16 GetEntityMoveMarkerYOffset(SpriteEntity *e) {
    return e->base.moveMarkerY;
}

/* Empty function (jr ra / nop). Acts as a no-op tick/event slot;
 * compiled body intentionally bare so dispatch sites that always call
 * the slot have a safe default. */
void NopStub_800207d4(void) {
}

/* Empty function (jr ra / nop). Second of two no-op default slots
 * adjacent in the rom; likely installed in a different vtable entry. */
void NopStub_800207dc(void) {
}

void FreeEntityNoTeardown_80020818(Entity *entity, s32 size);

/* Minimal entity destructor: swap the vtable at +0x18 to Destroyed and,
 * if flag bit 0 is set, free the body via FreeEntityNoTeardown_80020818.
 * Used by entities that hold no owned resources beyond their own struct
 * (HUD helpers, particle slots, etc.). */
void EntityDestructor_Simple(Entity *entity, s32 flags) {
    entity->collisionVtable = (void *)g_EntityVtable_Destroyed;
    if (flags & 1) {
        FreeEntityNoTeardown_80020818(entity, 0x1C);
    }
}

/* Thin shim around FreeFromHeap(g_pBlbHeapBase, entity, 0, 0). The
 * `size` arg is ignored -- naming reflects that it doesn't run any
 * destructor chain, just releases the bytes. Tail end of
 * EntityDestructor_Simple and friends. */
void FreeEntityNoTeardown_80020818(Entity *entity, s32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
}
