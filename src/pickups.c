#include "common.h"
#include "Game/asset_ids.h"
#include "functions.h"
#include "globals.h"
#include "Game/callback_slot.h"
#include "Game/fsm_dispatch.h"

extern void *g_pBlbHeapBase;
extern u8 DECOR_ENTITY_DESTRUCTOR_VTABLE[] asm("D_80010870");
extern u8 DECOR_SIMPLE_ALLOC_VTABLE[] asm("D_80010890");
extern u8 SUPERWILLIE_VTABLE[] asm("D_800106B0");
extern u8 PHOENIX_HAND_VTABLE[] asm("D_80010790");
extern u8 YELLOW_BIRD_VTABLE[] asm("D_80010810");
extern u8 HAMSTER_SHIELD_VTABLE[] asm("D_800106D0");
extern u8 TRANSPARENT_DECOR_VTABLE[] asm("D_800107B0");
extern u8 SCALE_RESET_VTABLE[] asm("D_80010710");
extern u8 SINGLE_FRAME_DECOR_VTABLE[] asm("D_80010730");
extern u8 ICON_1970_VTABLE[] asm("D_800106F0");
extern void FreeEntityNoTeardown_80030cdc(Entity *e, u32 size);
extern void CollisionCheckWrapper(Entity *e, u32 a, u32 b, u32 c);
extern void DecorEntityTickWithOffscreenCheck(Entity *e);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);
extern void CheckpointSwampTickCallback(Entity *e);
extern void EntityCollisionHandler_SpecialTrigger(Entity *e);
extern void EntityCollision_FlagAndDispatch(Entity *e);
extern Entity *InitEntitySprite(Entity *entity, u32 spriteId, s32 z, s16 x, s16 y, s32 flags);

typedef struct DecorSpawnData {
    /* 0x00 */ u8 pad00[8];
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
} DecorSpawnData;

extern void InitPathFollowingDecorEntity(TimedPathEntity *e, DecorSpawnData *data, u8 flag);

typedef struct InteractiveDecorEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x11D - 0x100];
    /* 0x11D */ u8 triggerState;
} InteractiveDecorEntity;

/* Animated decor whose tick fires on a random timer (e.g. blinking eyes,
 * flickering background effects). +0x120 is the down-counter byte armed
 * by DecorSetRandomTimer / DecorStartWithRandomTimer. */
typedef struct DecorRandomTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x120 - 0x100];
    /* 0x120 */ u8 randomTimer;
} DecorRandomTimerEntity;

/* gp_rel tentative defs (sdata blob owns the strong defs). */
u32 DECOR_TRIGGERED_STATE_MARKER asm("D_800A59D8");
EntityCallback DECOR_TRIGGERED_STATE_CALLBACK asm("D_800A59DC");

extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern u8 GREENBULLET_VTABLE[] asm("D_80010850");
void DecorEntity_CollectWithSwirlyEffect(InteractiveDecorEntity *e);

/* Sprite render context (Entity+0x34) view: the fields used to build the GPU
 * tpage from the sprite's reserved VRAM rect. Matches RenderSprite (spracc.c). */
typedef struct DecorSpriteCtx {
    /* 0x00 */ u8  pad00[0x10];
    /* 0x10 */ s16 vramX;
    /* 0x12 */ s16 vramY;
    /* 0x14 */ u8  pad14[0x24 - 0x14];
    /* 0x24 */ u16 tpage;
    /* 0x26 */ u8  pad26[0x32 - 0x26];
    /* 0x32 */ u8  abr;
} DecorSpriteCtx;

/* Type-002 green-bullets pickup: sprite hash 0xE8628689, path-following decor
 * shell, collected via DecorEntity_CollectWithSwirlyEffect; builds the GPU
 * tpage from the reserved VRAM rect. (Same shape as the clayball init minus the
 * random colour tint.) */
TimedPathEntity *InitGreenBulletsCollectible(TimedPathEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();
    DecorSpriteCtx *spr;
    s32 vx;
    s32 vy;

    InitEntitySprite((Entity *)e, 0xE8628689, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity(e, data, 0);
    e->sprite.base.collisionVtable = GREENBULLET_VTABLE;
    do {} while (0);
    fn = (void (*)())DecorEntity_CollectWithSwirlyEffect;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    spr = (DecorSpriteCtx *)e->sprite.base.spriteContext;
    vx = spr->vramX;
    vy = spr->vramY;
    spr->tpage = GetTPage(spr->abr, 1, vx & ~0x3F, vy & ~0xFF);
    return e;
}

extern u8 CheckEntityBoxCollision(Entity *e, u16 mask);
extern void AddPlayerOrbs(PlayerState *ps, s8 count);
extern void AddSwirlys(PlayerState *ps, s8 count);
extern void AddPlayerLives(PlayerState *ps, s8 count);
extern void AddPhoenixHands(PlayerState *ps, s8 count);
extern void AddSuperWillies(PlayerState *ps, s8 count);
extern void AwardSwirlyQsAndHamsters(PlayerState *ps, s8 count);
extern void InitDecorEntityWithScreenOffset(Entity *e, s32 dx, s32 dy, s32 flag);
extern void PlayEntityPositionSound(Entity *e, u32 soundId);
extern PlayerState *PLAYER_STATE_DATA asm("D_800A597C");

/* Powerup-collectible (extra life, phoenix hand, super willie...) shape:
 * extends InteractiveDecorEntity with a sub-entity pointer at +0x100 that
 * gets a +0x12 (collisionMask) field stamped with 7 just before the
 * award call. The stamp goes into the award's delay slot. */
typedef struct PowerupCollectibleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ Entity *labelEntity;
    /* 0x104 */ u8 pad104[0x11D - 0x104];
    /* 0x11D */ u8 triggerState;
} PowerupCollectibleEntity;

/* Twin of CollectibleClaySingleTickCallback for the swirly-orb pickup
 * variant. Same offscreen-cull + box-collision dispatch shape, but
 * grants a swirly instead of an orb, spawns the VFX at (0x98, 0x16),
 * and uses sound id 0x02847462. */
void DecorEntity_CollectWithSwirlyEffect(InteractiveDecorEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        AddSwirlys(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x98, 0x16, 1);
        PlayEntityPositionSound((Entity *)e, 0x02847462);
    }
}

INCLUDE_ASM("asm/nonmatchings/pickups", InitDecorEntityWithHUDIcon);

extern u8 DECOR_HUD_ICON_INTERMEDIATE_VTABLE[] asm("D_80010830");
extern void RemoveFromRenderList(GameState *gs, void *slot);
extern void DestroyEntityAndFreeMemory(SpriteEntity *e, s32 flags);

/* Decor entity that owns a detached render-list slot at +0x120 (the
 * HUD-icon child allocated by InitDecorEntityWithHUDIcon). */
typedef struct DecorChildSlotEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x120 - 0x100];
    /* 0x120 */ void *childSlot;
} DecorChildSlotEntity;

/* Destructor for decor entities that own a detached render-list slot at
 * +0x120 (typically allocated by InitDecorEntityWithHUDIcon). Always
 * swaps to the HUD-icon intermediate vtable D_80010830, unconditionally
 * removes the child slot from the render list and frees it, then
 * transitions to DECOR_ENTITY_DESTRUCTOR_VTABLE (D_80010870) for the
 * DestroyEntityAndFreeMemory teardown, and finally frees the wrapper
 * if flags & 1.
 *
 * Match recipe: vtable stores end up in the delay slots of the
 * RemoveFromRenderList and DestroyEntityAndFreeMemory calls (so they
 * run BEFORE each callee), achieved by writing them on the source
 * lines that immediately precede the calls. Same shape as
 * DestroyCompoundEntity but without the NULL-child guard. */
void EntityDestructor_FreeRenderListAt120(DecorChildSlotEntity *e, s32 flags) {
    e->sprite.base.collisionVtable = DECOR_HUD_ICON_INTERMEDIATE_VTABLE;
    RemoveFromRenderList(g_pGameState, e->childSlot);
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e->childSlot, 0, 0);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleOrbTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityRenderWithScaledPosition);

/* func_8002D978 @ 0x8002D978 (0xAC) — if entity flag +0x124 is set, dispatch
 * g_pGameState's event FSM callback (marker +0x8/0xA, fn +0xC) with eventId 3,
 * arg 0. Same FSM forwarder as func_80034B10 but with NO forwarded entity arg.
 * Merged its mis-split dispatch body (bogus TriggerFadeOut symbol, an internal
 * $a3-flowing continuation). SHELVED: the body never reads $a3(e), yet the
 * target keeps a dead `move a3,a0; lbu 0x124(a3)` (relic of the head having
 * been a separate function preserving e across the a0:=g_pGameState load). As
 * one C function cc1 reads the flag straight from $a0 and drops the move (2
 * instrs short) — the dead-value preservation can't be coaxed without shifting
 * the rest of the allocation. */
INCLUDE_ASM("asm/nonmatchings/pickups", func_8002D978);

void CollectibleYellowBirdTickCallback(InteractiveDecorEntity *e);

/* Yellow-bird collectible constructor. Same path-following decor shape
 * as InitSuperWillieCollectible, but the entity is set up with the
 * MASK-CLEAR variant of InitEntitySprite (final arg 0 rather than 1)
 * and overrides the collision vtable to the bird-specific D_80010810.
 * The per-frame behaviour lives in CollectibleYellowBirdTickCallback.
 * Sprite id 0xC87CA082, vtable override D_80010810.
 *
 * Match recipe: identical to InitSuperWillieCollectible — TripadSlot
 * for the 0x38 frame plus the two `do {} while (0);` boundaries that
 * pin the callback `la` into $v1 above the marker stores. */
TimedPathEntity *InitYellowBirdCollectible(TimedPathEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, 0xC87CA082, 0x3DE, data->x, data->y, 0);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity(e, data, 0);
    e->sprite.base.collisionVtable = YELLOW_BIRD_VTABLE;
    do {} while (0);
    fn = (void (*)())CollectibleYellowBirdTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleYellowBirdTickCallback);

/* InitClayballWithRandomColor @ 0x8002DBDC — type-007 clayball pickup: the
 * orange-ball sprite (CONFIRMED hash 0xB8700CA1) with a random colour tint.
 *   InitEntitySprite(e, 0xB8700CA1, 0x3DE, data->x, data->y, 1);
 *   collisionVtable D_80010870 -> InitPathFollowingDecorEntity -> D_800107F0;
 *   +0x10 = 0x458; tickMarker = CollectibleClaySingleTickCallback;
 *   spr = e->spriteContext; spr->tpage = GetTPage(spr->abr,1, vramX&~0x3F, vramY&~0xFF);
 *   SetAnimationFrameIndex(e, rand()&7);
 *   for each of R/G/B: clamp((s16)(rand()%48 + base - 0x18), 0, 255).
 * SHELVED: hand-matched to the exact instruction count (epilogue aligned); the
 * residual is pure register-coloring + minor scheduling in the 3x rand-tint tail
 * — spriteContext colored v1 (target) vs s0, clamp accumulator a2 vs a0. Blind
 * permuter plateaus (base 1305 -> best 1035, no 0); needs targeted PERM macros on
 * the tint block's coloring. Close C parked in
 * tools/decomp-permuter/nonmatchings/InitClayballWithRandomColor. */
INCLUDE_ASM("asm/nonmatchings/pickups", InitClayballWithRandomColor);

/* Single-hit clay-ball pickup tick. Runs the standard offscreen-cull
 * decor tick first; if the ball has either already been triggered
 * (+0x11D == 1) OR is currently overlapping the player's hit-box
 * (CheckEntityBoxCollision returns true), grants one orb to the player,
 * spawns a tiny VFX decor at (+0x18,+0x16) and plays the pickup sound
 * (0x4A806042).
 *
 * Match recipe: the bnez over CheckEntityBoxCollision short-circuits the
 * box-check when triggerState is already set. Naming variables and
 * keeping the boolean OR shape as `if (a || b)` produces TARGET's
 * branch-on-flag-or-call layout. */
void CollectibleClaySingleTickCallback(InteractiveDecorEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        AddPlayerOrbs(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x18, 0x16, 1);
        PlayEntityPositionSound((Entity *)e, FX_PICKUP_OBJECT);
    }
}

INCLUDE_ASM("asm/nonmatchings/pickups", InitPlatformDecorEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", CheckpointSwampTickCallback);

void DecorSetSpriteIdle(Entity *e) {
    PaddedSlotPair slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    fn = CheckpointSwampTickCallback;
    do {} while (0);
    m1 = -1;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->eventMarker = slot.s[0];
    SetEntitySpriteId(e, 0x09406D8A, 1);
}

void DecorSetSpriteActive(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);
    fn = CheckpointSwampTickCallback;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
    SetEntitySpriteId(e, 0x980861A3, 1);
}

/* Decor-entity initializer that installs the standard "offscreen tick +
 * flag-and-dispatch collision + queued sprite-active state" trio of
 * callback slots and swaps to sprite 0x880161A7. Three CallbackSlot
 * block-copies bracket a SetEntitySpriteId call; cc1 caches the
 * shared markerHi (-1) in $s0 across the SetEntitySpriteId call
 * because the same scalar `m1` value is reused on both sides.
 *
 * Match recipe: one TripadSlot scratch is reused for all three
 * installs (cc1 reloads sp+0x14 / sp+0x18 each time), and `m1 = -1;`
 * is set ONCE so the value survives the call in a callee-saved reg. */
void DecorStartAnimation(SpriteEntity *e) {
    TripadSlot u;
    s16 m1;
    void (*fn)(Entity *);

    do {} while (0);
    fn = (void (*)(Entity *))DecorEntityTickWithOffscreenCheck;
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->base.tickMarker = u.s;
    fn = (void (*)(Entity *))EntityCollision_FlagAndDispatch;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->base.eventMarker = u.s;
    SetEntitySpriteId((Entity *)e, 0x880161A7, 1);
    fn = (void (*)(Entity *))DecorSetSpriteActive;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->queuedStateMarker = u.s;
}

/* Twin of DecorStartAnimation but selects a different sprite
 * (0x91106183 instead of 0x880161A7). Same triple-slot install
 * pattern, same cached `m1 = -1` reuse across SetEntitySpriteId. */
void DecorStartAnimationAlt(SpriteEntity *e) {
    TripadSlot u;
    s16 m1;
    void (*fn)(Entity *);

    do {} while (0);
    fn = (void (*)(Entity *))DecorEntityTickWithOffscreenCheck;
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->base.tickMarker = u.s;
    fn = (void (*)(Entity *))EntityCollision_FlagAndDispatch;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->base.eventMarker = u.s;
    SetEntitySpriteId((Entity *)e, 0x91106183, 1);
    fn = (void (*)(Entity *))DecorSetSpriteActive;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->queuedStateMarker = u.s;
}

void CollectibleExtraLifeTickCallback(PowerupCollectibleEntity *e);

/* Extra-life (1-up) pickup init — the "transparent decor" vtable variant.
 * Twin of InitHamsterShieldCollectible (same +0xF6 visibility + spriteContext
 * +0x37 clears) but uses the one-up sprite (SPR_PREFIX_ONE_UP) + vtable
 * override (D_800107B0) and CollectibleExtraLifeTickCallback as the tick
 * (extra-life award). Spawned by EntityType011_ExtraLife_Init.
 *
 * Match recipe: same TripadSlot + dual `do {} while (0);` armor as the
 * other Init*Collectible siblings. */
PowerupCollectibleEntity *InitExtraLifeCollectible(PowerupCollectibleEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, SPR_PREFIX_ONE_UP, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity((TimedPathEntity *)e, data, 0);
    e->sprite.base.collisionVtable = TRANSPARENT_DECOR_VTABLE;
    do {} while (0);
    fn = (void (*)())CollectibleExtraLifeTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    e->sprite.visibility = 0;
    ((u8 *)e->sprite.base.spriteContext)[0x37] = 0;
    return e;
}

/* Single-hit "extra life" pickup. Twin of CollectibleClaySingleTickCallback
 * but for the 1-up powerup. The collected/box-collision check is
 * identical; on a hit it (a) stamps `labelEntity->collisionMask = 7` to
 * activate the floating "1UP" label sub-entity, (b) awards a life via
 * AddPlayerLives(PLAYER_STATE_DATA, 1), (c) spawns the standard pickup
 * VFX at (0x118, 0xF), and (d) plays sound FX_PICKUP_ONE_UP.
 *
 * Match recipe: keep the collisionMask store on the line BEFORE the
 * award call -- cc1 folds the `sh v0, 0x12(v1)` into the jal's delay
 * slot exactly as TARGET does. */
void CollectibleExtraLifeTickCallback(PowerupCollectibleEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        e->labelEntity->collisionMask = 7;
        AddPlayerLives(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x118, 0xF, 1);
        PlayEntityPositionSound((Entity *)e, FX_PICKUP_ONE_UP);
    }
}

void CollectiblePhoenixHandTickCallback(PowerupCollectibleEntity *e);

/* Phoenix Hand pickup constructor. Mirrors InitSuperWillieCollectible's
 * shape with two additional 16.16 fixed-point writes at +0x50/+0x54
 * (scaleRender / scaleRender2) preloaded to 0x8000 (= 0.5) so the
 * pickup spawns at half scale before the per-frame tick takes over.
 * Sprite id 0x9158A0F6, vtable override D_80010790, tick
 * CollectiblePhoenixHandTickCallback.
 *
 * Match recipe: same TripadSlot + `do {} while (0);` armor as the
 * SuperWillie twin to pin the `la` of the callback into $v1 above the
 * marker stores. */
PowerupCollectibleEntity *InitPhoenixHandCollectible(PowerupCollectibleEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, 0x9158A0F6, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity((TimedPathEntity *)e, data, 0);
    e->sprite.base.collisionVtable = PHOENIX_HAND_VTABLE;
    e->sprite.base.scaleRender = 0x8000;
    e->sprite.base.scaleRender2 = 0x8000;
    do {} while (0);
    fn = (void (*)())CollectiblePhoenixHandTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    return e;
}

/* Phoenix Hand pickup -- same shape as CollectibleExtraLifeTickCallback
 * but awards a phoenix hand via AddPhoenixHands, spawns VFX at (0x18,
 * 0xC8) and plays sound FX_PICKUP_PHOENIX. */
void CollectiblePhoenixHandTickCallback(PowerupCollectibleEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        e->labelEntity->collisionMask = 7;
        AddPhoenixHands(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x18, 0xC8, 1);
        PlayEntityPositionSound((Entity *)e, FX_PICKUP_PHOENIX);
    }
}

/* InitPhartHeadCollectible @ 0x8002EA3C — type-025 phart-head bonus pickup.
 * CONFIRMED hashes: parent sprite 0x8C510186, child HUD-label sprite 0xA9240484.
 * Shape (verified, structurally-correct C compiles):
 *   InitEntitySprite(e, 0x8C510186, 0x3DE, data->x, data->y, 0);
 *   vtable D_80010870 -> InitPathFollowingDecorEntity -> D_80010770;
 *   tickMarker = CollectiblePhartHeadTickCallback;
 *   spr=e->spriteContext; spr->{r,g,b}={0x20,0x60,0x30};
 *   spr->tpage = GetTPage(spr->abr, 3, vramX&~0x3F, vramY&~0xFF);
 *   child = AllocateFromHeap(blbHeap, 0x120, 1, 0);
 *   InitEntitySprite(child, 0xA9240484, 0x3DE, e->[+0x100]->x/y, 0);
 *   vtable D_80010870 -> InitPathFollowingDecorEntity; e->[+0x120]=child;
 *   child->worldX=e->worldX-8; child->worldY=e->worldY-0x10; child->vis=0;
 *   child->tickMarker = EntityUpdateCallback;
 *   cspr=child->spriteContext; cspr->[0x37]=0; cspr->{r,g,b}={0x20,0x60,0x30};
 *   cspr->[0x08]=0x3E0; AddEntityToSortedRenderList(g_pGameState, child).
 * SHELVED: register-allocation mismatch — TARGET colours entity->$s1/data->$s0
 * (mine swaps them) and hoists all three RGB constants into saved regs $s7/$s6/$s4
 * (mine hoists two, running one saved-reg short -> frame 0x48 vs 0x50, cascading
 * across the whole body). Needs targeted reg-coloring / a permuter pass. */
INCLUDE_ASM("asm/nonmatchings/pickups", InitPhartHeadCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectiblePhartHeadTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitUniverseEnemaCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleUniverseEnemaTickCallback);

void TriggerCollectible100CTickCallback(Entity *e);

/* "100C" trigger-collectible constructor. Same path-following decor
 * shape as the other Init*Collectible siblings; uses sprite 0x08624580,
 * vtable override D_80010730, tick TriggerCollectible100CTickCallback.
 * Differs by pre-loading targetX (+0x70) = 1 before the slot install so
 * the per-frame tick sees a non-zero target on its first call (drives
 * the trigger sweep). */
TimedPathEntity *InitSingleFrameDecorEntity(TimedPathEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, 0x08624580, 0x3DE, data->x, data->y, 0);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity(e, data, 0);
    e->sprite.base.collisionVtable = SINGLE_FRAME_DECOR_VTABLE;
    e->sprite.base.targetX = 1;
    do {} while (0);
    fn = (void (*)())TriggerCollectible100CTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/pickups", TriggerCollectible100CTickCallback);

void CollectibleScaleResetTickCallback(Entity *e);

/* Twin of InitSingleFrameDecorEntity but for the scale-reset pickup
 * variant. Sprite 0x8C30008C, vtable override D_80010710, tick
 * CollectibleScaleResetTickCallback. Same targetX=1 priming and dual
 * `do {} while (0);` armor. */
TimedPathEntity *InitScaleResetCollectible(TimedPathEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, SPR_PREFIX_GROW, 0x3DE, data->x, data->y, 0);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity(e, data, 0);
    e->sprite.base.collisionVtable = SCALE_RESET_VTABLE;
    e->sprite.base.targetX = 1;
    do {} while (0);
    fn = (void (*)())CollectibleScaleResetTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleScaleResetTickCallback);

void Collectible1970IconTickCallback(PowerupCollectibleEntity *e);

/* "1970 icon" pickup constructor. Same shape as InitHamsterShieldCollectible
 * (visibility/spriteContext[0x37] clears after the slot install) but with
 * sprite 0x88A28194, vtable override D_800106F0, tick
 * Collectible1970IconTickCallback. */
PowerupCollectibleEntity *Init1970IconEntity(PowerupCollectibleEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, 0x88A28194, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity((TimedPathEntity *)e, data, 0);
    e->sprite.base.collisionVtable = ICON_1970_VTABLE;
    do {} while (0);
    fn = (void (*)())Collectible1970IconTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    e->sprite.visibility = 0;
    ((u8 *)e->sprite.base.spriteContext)[0x37] = 0;
    return e;
}

INCLUDE_ASM("asm/nonmatchings/pickups", Collectible1970IconTickCallback);

void CollectibleHamsterShieldTickCallback(PowerupCollectibleEntity *e);

/* Hamster-shield pickup constructor. Mirrors the SuperWillie/PhoenixHand
 * shape (path-following decor, vtable override, tick install) but adds
 * two follow-up state clears so the powerup spawns "visible and
 * unflipped":
 *   - SpriteEntity.visibility (+0xF6) = 0  (0 == visible)
 *   - spriteContext[0x37] = 0              (clears flip/state byte on the
 *                                           render context)
 * Sprite id 0x80E85EA0, vtable override D_800106D0, tick
 * CollectibleHamsterShieldTickCallback. */
PowerupCollectibleEntity *InitHamsterShieldCollectible(PowerupCollectibleEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, 0x80E85EA0, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity((TimedPathEntity *)e, data, 0);
    e->sprite.base.collisionVtable = HAMSTER_SHIELD_VTABLE;
    do {} while (0);
    fn = (void (*)())CollectibleHamsterShieldTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    e->sprite.visibility = 0;
    ((u8 *)e->sprite.base.spriteContext)[0x37] = 0;
    return e;
}

/* Hamster-shield pickup -- variant of the PowerupCollectible shape that
 * awards swirly-Qs and hamsters together and picks the sound based on
 * the post-award hamster_count. When the player just hit hamster_count
 * == 3 (the cap), use the special "max-reached" sound 0xC2906565;
 * otherwise the normal pickup sound 0x42906465.
 *
 * Match recipe: the (cond ? a : b) ternary in the call argument lets
 * cc1 schedule the lui-of-the-default before the comparison and stick
 * the ori-of-the-default into the bne delay slot, exactly mirroring
 * TARGET's split lui/ori. */
void CollectibleHamsterShieldTickCallback(PowerupCollectibleEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        e->labelEntity->collisionMask = 7;
        AwardSwirlyQsAndHamsters(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0xD0, 0x1C, 1);
        PlayEntityPositionSound((Entity *)e,
            (PLAYER_STATE_DATA->hamster_count == 3) ? 0xC2906565 : 0x42906465);
    }
}

void CollectibleSuperWillieTickCallback(PowerupCollectibleEntity *e);

/* Super Willie pickup constructor. Builds the +0x100 path-following decor
 * shape (sprite id 0x902C0002, z=0x3DE), routes it through the shared
 * PATH_DECOR vtable + InitPathFollowingDecorEntity, then overrides the
 * collision vtable to the Super-Willie-specific D_800106B0 and installs
 * CollectibleSuperWillieTickCallback as the per-frame tick. Twin of the
 * other powerup-collectible Inits (PhoenixHand/HamsterShield) but with
 * no extra +0x50/+0x54/+0xF6 state writes — the pickup behaviour lives
 * entirely in the tick callback.
 *
 * Match recipe: TripadSlot pins the 0x38 frame (slot at sp+0x1C with
 * 4-byte pads either side); the named `fn`/`m1` locals + the explicit
 * `e->...collisionVtable = D_80010870` BEFORE the
 * InitPathFollowingDecorEntity call let cc1 fold the matching vtable
 * `sw $v0, 0x18($s1)` into the jal's delay slot, then re-emit the
 * D_800106B0 store as a standalone op in the post-call sequence. */
PowerupCollectibleEntity *InitSuperWillieCollectible(PowerupCollectibleEntity *e, DecorSpawnData *data) {
    TripadSlot u;
    s16 m1;
    void (*fn)();

    InitEntitySprite((Entity *)e, SPR_PREFIX_WILLIE, 0x3DE, data->x, data->y, 1);
    e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    InitPathFollowingDecorEntity((TimedPathEntity *)e, data, 0);
    e->sprite.base.collisionVtable = SUPERWILLIE_VTABLE;
    do {} while (0);
    fn = (void (*)())CollectibleSuperWillieTickCallback;
    do {} while (0);
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
    return e;
}

/* Super Willie pickup -- same shape as CollectibleExtraLifeTickCallback
 * but awards a super willie via AddSuperWillies, spawns VFX at (0x118,
 * 0x88) and plays sound FX_PICKUP_SUPER_WILLIE. */
void CollectibleSuperWillieTickCallback(PowerupCollectibleEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        e->labelEntity->collisionMask = 7;
        AddSuperWillies(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x118, 0x88, 1);
        PlayEntityPositionSound((Entity *)e, FX_PICKUP_SUPER_WILLIE);
    }
}

extern u8 PATH_DECOR2_VTABLE[] asm("D_80010690");
extern u8 D_8009B228[];
extern Entity *InitEntityWithSprite(SpriteEntity *entity, u8 *spriteDef, s32 z, s16 x, s16 y);
extern void DecorCollisionTickWithSpawnAndSound(Entity *e);
extern s32 rand(void);

/* gp_rel tentative defs for the (marker, callback) state pair installed
 * by InitEntity_PathDecor2 -- marker is 0xFFFF0000 (direct call) and the
 * callback is DecorSetRandomTimer. */
u32 DECOR_RANDOM_TIMER_STATE_MARKER asm("D_800A59C8");
EntityCallback DECOR_RANDOM_TIMER_STATE_CALLBACK asm("D_800A59CC");

/* SHELVED: register-allocation diff. Equivalent C is below but cc1
 * allocates `entity` to $s0 and `data` to $s1, while the original
 * assigns them in reverse ($s1/$s0). Function body is otherwise
 * byte-identical. Same family as InitSuperWillieCollectible (which
 * DOES match) but the trailing EntitySetState + rand + randomTimer
 * writes shift register pressure enough to flip the allocation.
 *
 *   DecorRandomTimerEntity *InitEntity_PathDecor2(
 *           DecorRandomTimerEntity *e, DecorSpawnData *data) {
 *       TripadSlot u;
 *       s16 m1;
 *       void (*fn)();
 *
 *       InitEntityWithSprite(&e->sprite, D_8009B228, 0x3DE,
 *                            data->x, data->y);
 *       e->sprite.base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
 *       InitPathFollowingDecorEntity((TimedPathEntity *)e, data, 0);
 *       e->sprite.base.collisionVtable = PATH_DECOR2_VTABLE;
 *       do {} while (0);
 *       fn = (void (*)())DecorCollisionTickWithSpawnAndSound;
 *       do {} while (0);
 *       m1 = -1;
 *       u.s.markerLo = 0;
 *       u.s.markerHi = m1;
 *       u.s.fn = fn;
 *       *(CallbackSlot *)&e->sprite.base.tickMarker = u.s;
 *       EntitySetState((Entity *)e, DECOR_RANDOM_TIMER_STATE_MARKER,
 *                      DECOR_RANDOM_TIMER_STATE_CALLBACK);
 *       e->randomTimer = (rand() & 0x3F) + 8;
 *       return e;
 *   }
 */
INCLUDE_ASM("asm/nonmatchings/pickups", InitEntity_PathDecor2);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorCollisionTickWithSpawnAndSound);

/* Re-arms the random-tick countdown (+0x120 = randomTimer) to
 * (rand() & 0x7F) + 0x30 frames (0x30..0xAF), switches sprite to
 * 0xA9228088, and re-installs EntityCollisionHandler_SpecialTrigger as
 * the collision/event handler on the +0x08 slot. Used as a re-entry
 * tick for animated decor whose state flips back to "idle" after each
 * random interval. */
void DecorSetRandomTimer(DecorRandomTimerEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    e->randomTimer = (rand() & 0x7F) + 0x30;
    SetEntitySpriteId((Entity *)e, SPR_PREFIX_HAMSTER, 1);
    fn = EntityCollisionHandler_SpecialTrigger;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
}

/* One-shot setup for an animated decor that fires on a random timer:
 * switches sprite to 0x093080C8, installs EntityCollision_FlagAndDispatch
 * on the +0x08 event slot, and queues DecorSetRandomTimer on the +0x98
 * queued-state slot so the timer arms on the next callback dispatch. */
void DecorStartWithRandomTimer(DecorRandomTimerEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();

    SetEntitySpriteId((Entity *)e, 0x093080C8, 1);
    fn = EntityCollision_FlagAndDispatch;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    fn = (void (*)())DecorSetRandomTimer;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.queuedStateMarker = slot.s;
}

INCLUDE_ASM("asm/nonmatchings/pickups", InitInteractiveDecorEntity);

void DecorEntityTickWithCollision(InteractiveDecorEntity *e) {
    CollisionCheckWrapper((Entity *)e, EVT_TICK, EVT_DAMAGE, EVT_TICK);
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0) {
        EntitySetState((Entity *)e, DECOR_TRIGGERED_STATE_MARKER, DECOR_TRIGGERED_STATE_CALLBACK);
    }
}

extern void SpawnEntityOrTriggerZone(Entity *e);

/* SHELVED: branch-layout diff. Equivalent C:
 *
 *   Entity *DecorEntityCollisionHandler(InteractiveDecorEntity *e,
 *                                       u32 event, u32 arg2) {
 *       Entity *r = NULL;
 *       event &= 0xFFFF;
 *       if (event == 0x1016) {
 *           r = (Entity *)e;
 *           e->triggerState = 1;
 *           e->sprite.base.collisionMask = 0;
 *       }
 *       if (event == 0x1001) {
 *           EntitySetState((Entity *)e,
 *                          DECOR_TRIGGERED_STATE_MARKER,
 *                          DECOR_TRIGGERED_STATE_CALLBACK);
 *       } else if (event == 1) {
 *           if (arg2 == 0x10022814) {
 *               SpawnEntityOrTriggerZone((Entity *)e);
 *           }
 *       }
 *       return r;
 *   }
 *
 * Original places the event==1 path OUT OF LINE (via beq-ahead) and
 * uses TWO registers for `r` ($v1 in the 0x1016 block, then `move s0,
 * v1` to a callee-save spill across the call sites). No source form
 * I tried (if/else-if swap, switch, goto, inverted predicates) gets
 * cc1 to pick that layout. */
INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityCollisionHandler);

/* SHELVED: decomp-permuter plateaued at score 825 (base 1010) after a long
 * sustained run on Sauron (64-core, j24) — no score-0 path found. The residual
 * is register-coloring/scheduling, not structure; randomization alone hasn't
 * cracked it. Parked for now; revisit with targeted PERM_* macros once the
 * exact blocking instructions are isolated. */
INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityCollisionHandlerExt);

extern void DecorEntityCollisionHandlerExt(Entity *e);
extern void DecorEntityDestroyWithParticles(Entity *e);

/* Plays a positional sound (0x40023E30) then installs the standard
 * three-callback decor animation set: offscreen-cull tick at +0x00,
 * the extended collision handler at +0x08, sprite 0xBB781481, and
 * a queued destroy-with-particles callback at +0x98.
 *
 * Match recipe: same shape as DecorStartAnimation but with a leading
 * PlayEntityPositionSound call. cc1 schedules `sw s0` into the call's
 * delay slot, then emits the first slot install. No `do {} while (0);`
 * armor needed since the function call provides a basic-block boundary
 * for the prologue scheduler. */
void DecorPlaySoundAndAnimate(SpriteEntity *e) {
    TripadSlot u;
    s16 m1;
    void (*fn)(Entity *);

    PlayEntityPositionSound((Entity *)e, 0x40023E30);
    fn = (void (*)(Entity *))DecorEntityTickWithOffscreenCheck;
    m1 = -1;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->base.tickMarker = u.s;
    fn = (void (*)(Entity *))DecorEntityCollisionHandlerExt;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->base.eventMarker = u.s;
    SetEntitySpriteId((Entity *)e, 0xBB781481, 1);
    fn = (void (*)(Entity *))DecorEntityDestroyWithParticles;
    u.s.markerLo = 0;
    u.s.markerHi = m1;
    u.s.fn = fn;
    *(CallbackSlot *)&e->queuedStateMarker = u.s;
}

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityDestroyWithParticles);

INCLUDE_ASM("asm/nonmatchings/pickups", SpawnEntityOrTriggerZone);

INCLUDE_ASM("asm/nonmatchings/pickups", InitEntityType0x3DE);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntitySpawnExplosionPieces);

void EntityDestructor_Vtable0x80010870_A(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_VtableAndHeapFree(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_B(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_C(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_D(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_E(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_F(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_G(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_H(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_I(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_J(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_K(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_L(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_M(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_N(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_O(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void EntityDestructor_Vtable0x80010870_P(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = DECOR_ENTITY_DESTRUCTOR_VTABLE;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void func_80030C98(void) {
}

void func_80030CA0(void) {
}

void EntityDestructor_Vtable0x80010870_Q(Entity *entity, u32 flag) {
    entity->collisionVtable = DECOR_SIMPLE_ALLOC_VTABLE;
    if (flag & 1) {
        FreeEntityNoTeardown_80030cdc(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80030cdc(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
}

