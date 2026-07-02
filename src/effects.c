#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/entity_events.h"
#include "Game/fsm_dispatch.h"

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
extern void UpdateEntityRender(Entity *e);
extern void UploadEntityTextureIfDirty(Entity *e);
extern u8 IsEntityOffScreenY(Entity *e);

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

/* RenderRotatedTexturedQuad @ 0x800310AC — builds a rotated textured POLY_FT4
 * (corner box -> RotMatrixZ -> RotTrans) and AddPrim's it. Logic-complete C
 * draft matches 200/200 instructions and opcode set; ~80 lines of scheduling /
 * stack-layout / coloring remain (needs permuter). Draft + analysis kept in
 * docs/analysis/decomp-drafts/RenderRotatedTexturedQuad.c. */
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

/* Destroy callback for a VRAM-slot entity: pins the render vtable at +0x0C,
 * releases the reserved VRAM rect back to the coalescing free-list (only if a
 * slot was actually allocated, byte at +0x10), then frees the entity when
 * (flags & 1). The slot rect is passed to FreeVRAMSlot as two packed u32s:
 * (x@0x16 | y@0x18<<16) and (0x1A | height@0x1C<<16).
 *
 * SHELVED: body is m2c-confirmed correct (right calls/fields), but two cc1
 * codegen diffs remain: (1) TARGET reserves an 8-byte frame hole (frame 0x28,
 * saves at sp+0x18/0x1C/0x20) that the natural C frame (0x20) lacks — not a
 * stack round-trip (no sh to stack), so no source form observed induces it;
 * (2) TARGET schedules the two packed-arg low halves (0x16->v0, 0x1A->v1) both
 * before the g_pBlbHeapBase load and keeps 0x1A alive in v1, while cc1 loads the
 * heap first and recomputes 0x1A into v0. Frame-hole is beyond the permuter
 * (it can't change frame size). Equivalent C:
 *   typedef struct { u8 pad00[0xC]; void *renderVtable; u8 vramAllocOk;
 *       u8 pad11[5]; u16 vramX, vramY, slotSizeLo, slotHeight; } VRAMSlotEnt;
 *   void DestroyVRAMSlotEntity(VRAMSlotEnt *e, s32 flags) {
 *       e->renderVtable = &VFX_ENTITY_VTABLE_10B68;   // D_80010B68
 *       if (e->vramAllocOk != 0)
 *           FreeVRAMSlot(g_pBlbHeapBase, e->vramX | (e->vramY << 16),
 *                        e->slotSizeLo | (e->slotHeight << 16));
 *       if (flags & 1) FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
 *   } */
INCLUDE_ASM("asm/nonmatchings/effects", DestroyVRAMSlotEntity);

INCLUDE_ASM("asm/nonmatchings/effects", RenderVRAMSlotOverlay);

/* PSY-Q RECT (libgpu): 8-byte rectangle struct used by VRAM transfer
 * primitives. Defined locally here since no project-wide header pulls
 * it in. Matches the standard libgpu layout (TYPES.H). */
typedef struct VRAMTransferRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} VRAMTransferRect;

extern void StoreImage(VRAMTransferRect *r, u_long *p);
extern void DrawSync(s32 mode);

/* VRAM slot entity layout — first 0x10 bytes are the basic-entity header
 * (initialized by InitBasicEntityWithVtable). +0x10 holds the AllocateVRAMSlot
 * return code (1=ok, 0=failed). +0x16/+0x18 are the raw VRAM pixel
 * coordinates of the allocated slot. */
typedef struct VRAMSlotEntity {
    /* 0x00 */ u8 pad0[0x10];
    /* 0x10 */ u8 vramAllocOk;
    /* 0x11 */ u8 pad11[5];
    /* 0x16 */ u16 vramX;
    /* 0x18 */ u16 vramY;
} VRAMSlotEntity;

/* SHELVED: 1-instruction diff — cc1 fuses the `sltiu`+`move a0,v0` pair
 * that TARGET emits into a single `sltiu a0,v0,1`. Specifically:
 *   TARGET:   sltiu v0,v0,1     CURRENT:  sltiu a0,v0,1
 *             move  a0,v0                 (fused into above)
 *
 * Variations tried (all produced the fused output): single-return with
 * result=0 default + conditional store, if-else with explicit result=0
 * branch, separate `cmp` and `result` locals with do-while-0 barrier
 * between them, `result = !((u16)pixel ^ 0x3C0F)` vs `result = ((..)==0)`,
 * `if (cmp == 0) result = 1;` conditional store form.
 *
 * Reads a single pixel from VRAM at the entity's allocated slot using
 * StoreImage (1x1 RECT), syncs the GPU, returns 1 iff that pixel ==
 * 0x3C0F (a sentinel "filled-slot" color in 15bpp PSX packing). Returns
 * 0 if no slot was ever allocated.
 *
 * Equivalent C body (kept for documentation - shelved to ASM include):
 *   s32 CheckVRAMSlotPixelColor(VRAMSlotEntity *e) {
 *       VRAMTransferRect r;
 *       u_long pixel;
 *       s32 result = 0;
 *       if (e->vramAllocOk != 0) {
 *           r.x = (s16)e->vramX;
 *           r.y = (s16)e->vramY;
 *           r.w = 1;
 *           r.h = 1;
 *           StoreImage(&r, &pixel);
 *           DrawSync(0);
 *           result = !((u16)pixel ^ 0x3C0F);
 *       }
 *       return result;
 *   } */
INCLUDE_ASM("asm/nonmatchings/effects", CheckVRAMSlotPixelColor);

INCLUDE_ASM("asm/nonmatchings/effects", InitPasswordEntity);

INCLUDE_ASM("asm/nonmatchings/effects", EntityDestructor_WithVRAMSlotFree);

INCLUDE_ASM("asm/nonmatchings/effects", MultiPartEntityTick);

/* Wraps UpdateEntityRender with a second pass that projects the entity's
 * worldX/worldY into screen-space and writes them into the s16 pair at
 * (e->renderScreenPos + 0/+2). (The leading nop that previously looked
 * like part of this function was MultiPartEntityTick's `jr $ra` delay
 * slot — splat had the boundary 4 bytes too early; fixed in
 * symbol_addrs.txt.) */
typedef struct EntityWithRenderTarget {
    u8 pad00[0x68];
    u16 worldX;            /* 0x68 */
    u16 worldY;            /* 0x6A */
    u8 pad6C[0x118 - 0x6C];
    s16 *renderScreenPos;  /* 0x118 */
} EntityWithRenderTarget;

void EntityRenderCallbackUpdateScreenPos(EntityWithRenderTarget *e) {
    GameState *gs;
    s32 cx, cy;
    UpdateEntityRender((Entity *)e);
    gs = g_pGameState;
    cx = gs->camera_x;
    e->renderScreenPos[0] = e->worldX - cx;
    cy = gs->camera_y;
    e->renderScreenPos[1] = e->worldY - cy;
}

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

extern void *ALTERNATE_ENTITY_INTERMEDIATE_VTABLE asm("D_80010AB8");
extern void RemoveFromRenderList(GameState *gs, void *slot);

typedef struct CompoundEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ void *child;
} CompoundEntity;

/* Compound-entity destructor: an alternate-entity wrapper that owns a
 * detached child render-list slot at +0x104 (allocated by
 * InitAlternateEntity). Always swaps the vtable to the intermediate
 * D_80010AB8 (unconditionally, via beqz delay-slot store); if a child
 * is present, removes it from the render list and frees its allocation;
 * then calls DestroyEntityAndFreeMemory for the rest of the teardown,
 * and finally frees the wrapper if flags & 1.
 *
 * Match recipe: vtable store on the line BEFORE the if-check so cc1
 * folds it into the beqz delay slot (where it runs unconditionally).
 * Two separate e->child reads bracket the RemoveFromRenderList call
 * exactly as TARGET does. */
void DestroyCompoundEntity(CompoundEntity *e, s32 flags) {
    e->sprite.base.collisionVtable = &ALTERNATE_ENTITY_INTERMEDIATE_VTABLE;
    if (e->child != NULL) {
        RemoveFromRenderList(g_pGameState, e->child);
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e->child, 0, 0);
    }
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

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

/* Per-frame tick: refresh the entity's VRAM texture if dirty, then if the
 * ready flag at +0x109 is set, notify the GameState event FSM
 * (EVT_GAME_NOTIFY, arg 0, srcEntity=e). S-reg dispatch family: the
 * pre-call pushes the slot pair into callee-saved $s0/$s1 (fn $t0,
 * self $s2). */
void EntityTick_UploadTextureWithCallback(Entity *e) {
    GS_NOTIFY_DECLS_S(Entity *);

    self = e;
    UploadEntityTextureIfDirty(self);
    if (*((u8 *)self + 0x109) != 0) {
        GS_NOTIFY_DISPATCH(0, self);
    }
}

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

/* If entity flag +0x34 is set, dispatch g_pGameState's event FSM callback
 * (marker +0x8/0xA, fn +0xC) with eventId 3, forwarding entity+0x20 as the arg
 * and the entity itself as srcEntity. Same FSM-slot forwarder shape as
 * InvokeEntityRenderCallback (fn/then-fn pinned to $t2/$t1); the gs-as-u8* and
 * slotArg-via-int copies reproduce cc1's register staging. */
typedef void (*GsEventCB)();
typedef struct { s32 arg; GsEventCB fn; } GsEventSlot;

void DispatchGsEventOnFlag34(Entity *e) {
    GameState *gs;
    s32 arg;
    u8 *gsb;
    s16 m;
    FSM_REG(GsEventCB, fn, "$10"); /* $t2 home (jalr target) */
    FSM_REG(GsEventCB, ft, "$9");  /* $t1 then-fn (relays into $t2) */
    FSM_REG(s32, slotArg, "$8");   /* $t0 slot arg */
    s32 adj;
    s16 sCopy;
    int slotArgWide;
    s32 lo;
    FSM_REG(s16, s, "$11");        /* $t3 marker survivor (staged via $v0) */

    if (*(u8 *)((u8 *)e + 0x34) == 0) {
        return;
    }
    gs = g_pGameState;
    m = ((s16 *)&gs->event_marker)[1];
    arg = *(s32 *)((u8 *)e + 0x20);
    gsb = (u8 *)gs;
    if (m == 0) {
        return;
    }
    s = m;
    sCopy = s;
    if (s > 0) {
        GsEventSlot *base =
            *(GsEventSlot **)(gsb + *(s16 *)&gs->event_callback);
        slotArg = base[sCopy - 1].arg;
        ft = base[sCopy - 1].fn;
        FSM_RELAY(fn, ft);
    } else {
        fn = (GsEventCB)gs->event_callback;
    }
    slotArgWide = slotArg;
    lo = ((s16 *)&gs->event_marker)[0];
    if (sCopy > 0) {
        adj = (s16)slotArgWide + lo;
    } else {
        adj = lo;
    }
    fn((void *)((u8 *)gs + adj), 3, arg, e);
}

INCLUDE_ASM("asm/nonmatchings/effects", InitEntity_168254b5);

INCLUDE_ASM("asm/nonmatchings/effects", EntityTick_FrictionAndParallaxScale);

INCLUDE_ASM("asm/nonmatchings/effects", InitDebrisParticleEntity);

/* Debris particle tick: standard entity update, then once the particle
 * exits the screen vertically, notify the GameState event FSM
 * (EVT_GAME_NOTIFY, arg 0, srcEntity=e) so the emitter can recycle it. */
void DebrisParticleTickCallback(Entity *e) {
    GS_NOTIFY_DECLS_S(Entity *);

    self = e;
    EntityUpdateCallback(self);
    if (IsEntityOffScreenY(self) != 0) {
        GS_NOTIFY_DISPATCH(0, self);
    }
}

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

/* If the entity's flag at +0x7C is set, dispatch g_pGameState's event FSM
 * slot (marker +0x8/0xA, fn +0xC) with EVT_GAME_NOTIFY (3), arg 1, and the
 * entity as srcEntity. First of four byte-identical game-progression
 * notifiers differing in the guard-flag offset and arg (this one passes 0) (see
 * ExpiredEntityDespawnEvent / BeamEffectDespawnEvent /
 * FadeExpireEntityDespawnEvent). Same FSM forwarder recipe as
 * DispatchGsEventOnFlag34; the constant 3/1 args free $a1/$a2, so the marker
 * survivor pins to $a2 instead of $t3. Formerly split as
 * "NotifyGameStateZero" (phantom symbol, merged 2026-07-02). */
void EntityDespawnIfFlagSet(Entity *e) {
    GS_NOTIFY_DECLS_T;

    if (*((u8 *)e + 0x7C) == 0) {
        return;
    }
    GS_NOTIFY_DISPATCH(0, e);
}


INCLUDE_ASM("asm/nonmatchings/effects", CreatePlayerParticleEntity);

/* Down-counts the u8 timer at +0x80 each tick; when it reaches zero, fires
 * the GameState event FSM with EVT_GAME_NOTIFY (3, arg 0, srcEntity=e).
 * Same a3-family template as EntityDespawnIfFlagSet. */
void EntityTimerDespawnCallback(Entity *e) {
    GS_NOTIFY_DECLS_T;
    u8 timer;

    timer = *((u8 *)e + 0x80);
    if (timer == 0) {
        return;
    }
    timer--;
    *((u8 *)e + 0x80) = timer;
    if (timer != 0) {
        return;
    }
    GS_NOTIFY_DISPATCH(0, e);
}

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

extern void *SCALED_MENU_VTABLE asm("D_80010980");

/* Sister of DestroyOscillatingScaleEntity for the scaled-menu / ripple
 * expand effect entity (initialized by InitScaledMenuEntityWithChild).
 * Same two-stage destructor shape, but the child sub-entity sits at
 * +0x20 (renderCallback slot, repurposed as a pointer) instead of +0x1C,
 * and the intermediate vtable is SCALED_MENU_VTABLE (D_80010980). */
void DestroyRippleExpandEffect(Entity *e, s32 flags) {
    e->collisionVtable = &SCALED_MENU_VTABLE;
    RemoveFromRenderList(g_pGameState, (void *)e->renderCallback);
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e->renderCallback, 0, 0);
    e->collisionVtable = &TIMED_FADE_ENTITY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

void CountdownTimerTickCallback(CountdownTimerEntity *e) {
    u8 v = e->timer - 1;
    e->timer = v;
    if (v == 0) {
        e->expiredFlag = 1;
        e->child->activeFlag = 0;
    }
}

/* Per-frame render callback for ripple-expand effects: projects the
 * entity's local world position (+0x24/+0x26) into the child primitive's
 * screen-space coords (-camera), then bumps the child's phase byte at
 * +0x3A7 to 1 so the next tick of the ripple shader sees fresh input. */
void RippleEffectRenderCallback(RippleExpandEntity *e) {
    GameState *gs = g_pGameState;
    void *child;
    s32 cx, cy;
    cx = gs->camera_x;
    *(s16 *)((u8 *)*(void **)((u8 *)e + 0x20) + 0) = *(u16 *)((u8 *)e + 0x24) - cx;
    cy = gs->camera_y;
    *(s16 *)((u8 *)*(void **)((u8 *)e + 0x20) + 2) = *(u16 *)((u8 *)e + 0x26) - cy;
    child = *(void **)((u8 *)e + 0x20);
    *((u8 *)child + 0x3A7) = 1;
}

/* Countdown-timer variant of the game-progression notifier quartet: fires
 * EVT_GAME_NOTIFY (3, arg 1, srcEntity=e) at g_pGameState's event FSM slot
 * once the timer tick has set expiredFlag (+0x29). Same recipe as
 * EntityDespawnIfFlagSet. Formerly split as "NotifyGameStateOne" +
 * "NullStubFunction" (phantom symbols, merged 2026-07-02). */
void ExpiredEntityDespawnEvent(CountdownTimerEntity *e) {
    GS_NOTIFY_DECLS_T;

    if (e->expiredFlag == 0) {
        return;
    }
    GS_NOTIFY_DISPATCH(1, e);
}



Entity *InitMenuItemEntity(Entity *e) {
    InitBasicEntityWithVtable(e, 0xBB8);
    e->eventCallback = (EntityCallback)&MENU_ITEM_ENTITY_VTABLE;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/effects", RenderSpotlightBeamEffect);

INCLUDE_ASM("asm/nonmatchings/effects", InitMenuEntityWithChildAndParams);

extern void *MENU_ENTITY_CHILD_PARAMS_VTABLE asm("D_80010950");

/* Sister of DestroyOscillatingScaleEntity for the menu/spotlight-beam
 * entity (initialized by InitMenuEntityWithChildAndParams). Same
 * two-stage destructor shape with child at +0x20 (renderCallback slot)
 * and intermediate vtable MENU_ENTITY_CHILD_PARAMS_VTABLE (D_80010950). */
void DestroySpotlightBeamEffect(Entity *e, s32 flags) {
    e->collisionVtable = &MENU_ENTITY_CHILD_PARAMS_VTABLE;
    RemoveFromRenderList(g_pGameState, (void *)e->renderCallback);
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e->renderCallback, 0, 0);
    e->collisionVtable = &TIMED_FADE_ENTITY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

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

/* Per-frame render callback for spotlight-beam effects: copies the
 * entity's local world position (+0x24/+0x26) into the linked render
 * primitive's screen-space coords (-camera) and copies a third u16
 * field (+0x28, the beam's tip-height parameter) to the primitive's
 * +0x90 slot for the shader stage to pick up. Pure projection helper -
 * the actual draw call is handled elsewhere by the primitive renderer. */
void BeamEffectRenderCallback(BeamEffectEntity *e) {
    GameState *gs = g_pGameState;
    s32 cx, cy;
    cx = gs->camera_x;
    *(s16 *)((u8 *)e->child + 0) = *(u16 *)((u8 *)e + 0x24) - cx;
    cy = gs->camera_y;
    *(s16 *)((u8 *)e->child + 2) = *(u16 *)((u8 *)e + 0x26) - cy;
    *(s16 *)((u8 *)e->child + 0x90) = *(u16 *)((u8 *)e + 0x28);
}

/* Beam-effect variant of the notifier quartet: guard is
 * BeamEffectEntity.expiredFlag (+0x1E, set by BeamEffectTickWithRotation
 * when the beam timer runs out). Formerly split as "NotifyGameStateOneAlt"
 * (phantom symbol, merged 2026-07-02). */
void BeamEffectDespawnEvent(BeamEffectEntity *e) {
    GS_NOTIFY_DECLS_T;

    if (e->expiredFlag == 0) {
        return;
    }
    GS_NOTIFY_DISPATCH(1, e);
}


INCLUDE_ASM("asm/nonmatchings/effects", InitScalableTimerEntity);

INCLUDE_ASM("asm/nonmatchings/effects", OscillateScaleAndRotationTick);

INCLUDE_ASM("asm/nonmatchings/effects", InitFadeMenuEntityWithChild);

extern void *OSCILLATING_SCALE_VTABLE asm("D_80010910");

/* Two-stage destructor for the fade-menu/oscillating-scale entity
 * (initialized by InitFadeMenuEntityWithChild). Stage 1: swap vtable to
 * OSCILLATING_SCALE_VTABLE (D_80010910 = the entity's own active vtable,
 * a no-op-looking but actually a destruction-safe normalization), then
 * tear down the child sub-entity at +0x1C by removing it from the render
 * list and freeing its allocation. Stage 2: swap to the final destroyed
 * vtable (TIMED_FADE_ENTITY_VTABLE = D_80010BC8) and conditionally free
 * the wrapper entity itself (flags & 1).
 *
 * Match recipe: write the intermediate vtable assignment on the line
 * BEFORE the RemoveFromRenderList call so cc1 folds the sw into jal's
 * delay slot. Access e->renderMarker twice (not via a cached local) so
 * cc1 emits two separate loads bracketing the function call as TARGET
 * does. */
void DestroyOscillatingScaleEntity(Entity *e, s32 flags) {
    e->collisionVtable = &OSCILLATING_SCALE_VTABLE;
    RemoveFromRenderList(g_pGameState, (void *)e->renderMarker);
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e->renderMarker, 0, 0);
    e->collisionVtable = &TIMED_FADE_ENTITY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/effects", FadeAndExpireEntityTick);

/* Fade-and-expire variant of the notifier quartet: guard byte at +0x24
 * (set by FadeAndExpireEntityTick when the fade completes). Formerly split
 * as "NotifyGameStateOneAlt2" (phantom symbol, merged 2026-07-02). */
void FadeExpireEntityDespawnEvent(Entity *e) {
    GS_NOTIFY_DECLS_T;

    if (*((u8 *)e + 0x24) == 0) {
        return;
    }
    GS_NOTIFY_DISPATCH(1, e);
}


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

extern void *FADE_OVERLAY_VTABLE asm("D_800108D0");

/* Sister of DestroyOscillatingScaleEntity for the timed-fade overlay
 * entity (e.g. created by CreateFadeOverlayEntity). Same two-stage
 * destructor shape: tear down the child at +0x1C and free it, swap the
 * vtable to TIMED_FADE_ENTITY_VTABLE, then conditionally free the
 * wrapper. Only difference is the intermediate vtable: D_800108D0
 * (FADE_OVERLAY_VTABLE) instead of D_80010910. */
void DestroyTimedFadeEntity(Entity *e, s32 flags) {
    e->collisionVtable = &FADE_OVERLAY_VTABLE;
    RemoveFromRenderList(g_pGameState, (void *)e->renderMarker);
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e->renderMarker, 0, 0);
    e->collisionVtable = &TIMED_FADE_ENTITY_VTABLE;
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

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

void SetEffectWord90(EffectWord90Entity *e, u16 val) {
    e->value90 = val;
}

void SetRippleExpandPhase(RippleExpandEntity *e, u8 val) {
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

void SetColoredOverlayRGB(ColoredOverlayEntity *e, u8 b40, u8 b41, u8 b42) {
    e->r = b40;
    e->g = b41;
    e->b = b42;
}

void SetEffectByte58Value58(EffectByte58Entity *e, u8 val) {
    e->value58 = val;
}

void SetEntityHitboxHeight(Entity *e, u16 val) {
    e->hitboxHeight = val;
}

void SetEntityHitboxWidth(Entity *e, u16 val) {
    e->hitboxWidth = val;
}

void SetEffectByte58Value5D(EffectByte58Entity *e, u8 val) {
    e->value5D = val;
}

void SetEntityHitboxHeight_80038870(Entity *e, u16 val) {
    e->hitboxHeight = val;
}

void SetEntityHitboxWidth_80038878(Entity *e, u16 val) {
    e->hitboxWidth = val;
}

void SetLargeEffectValue1E7(LargeEffectStateEntity *e, u8 val) {
    e->value1E7 = val;
}

void ArmLargeEffectValue1E0(LargeEffectStateEntity *e) {
    e->value1E0 = 0x20;
}

void EntityDestroyCallback_Vt80010B28(Entity *entity, s32 flags) {
    entity->collisionVtable = &ROPE_SEGMENT_ENTITY_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

u32 GetEntityRenderMarker_800388F8(Entity *e) {
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

