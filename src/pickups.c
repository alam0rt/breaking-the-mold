#include "common.h"
#include "functions.h"
#include "globals.h"
#include "Game/callback_slot.h"

extern void *g_pBlbHeapBase;
extern u8 DECOR_ENTITY_DESTRUCTOR_VTABLE[] asm("D_80010870");
extern u8 DECOR_SIMPLE_ALLOC_VTABLE[] asm("D_80010890");
extern u8 SUPERWILLIE_VTABLE[] asm("D_800106B0");
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

INCLUDE_ASM("asm/nonmatchings/pickups", InitGreenBulletsCollectible);

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

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_FreeRenderListAt120);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleOrbTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityRenderWithScaledPosition);

INCLUDE_ASM("asm/nonmatchings/pickups", func_8002D978);

INCLUDE_ASM("asm/nonmatchings/pickups", TriggerFadeOut);

INCLUDE_ASM("asm/nonmatchings/pickups", InitYellowBirdCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleYellowBirdTickCallback);

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
        PlayEntityPositionSound((Entity *)e, 0x4A806042);
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

INCLUDE_ASM("asm/nonmatchings/pickups", DecorStartAnimation);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorStartAnimationAlt);

INCLUDE_ASM("asm/nonmatchings/pickups", InitTransparentDecorEntity);

/* Single-hit "extra life" pickup. Twin of CollectibleClaySingleTickCallback
 * but for the 1-up powerup. The collected/box-collision check is
 * identical; on a hit it (a) stamps `labelEntity->collisionMask = 7` to
 * activate the floating "1UP" label sub-entity, (b) awards a life via
 * AddPlayerLives(PLAYER_STATE_DATA, 1), (c) spawns the standard pickup
 * VFX at (0x118, 0xF), and (d) plays sound 0x428254E2.
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
        PlayEntityPositionSound((Entity *)e, 0x428254E2);
    }
}

INCLUDE_ASM("asm/nonmatchings/pickups", InitPhoenixHandCollectible);

/* Phoenix Hand pickup -- same shape as CollectibleExtraLifeTickCallback
 * but awards a phoenix hand via AddPhoenixHands, spawns VFX at (0x18,
 * 0xC8) and plays sound 0x44C26454. */
void CollectiblePhoenixHandTickCallback(PowerupCollectibleEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        e->labelEntity->collisionMask = 7;
        AddPhoenixHands(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x18, 0xC8, 1);
        PlayEntityPositionSound((Entity *)e, 0x44C26454);
    }
}

INCLUDE_ASM("asm/nonmatchings/pickups", InitPhartHeadCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectiblePhartHeadTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitUniverseEnemaCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleUniverseEnemaTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitSingleFrameDecorEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", TriggerCollectible100CTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitScaleResetCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", ScaleResetCollectibleTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", Init1970IconEntity);

INCLUDE_ASM("asm/nonmatchings/pickups", Collectible1970IconTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitHamsterShieldCollectible);

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

    InitEntitySprite((Entity *)e, 0x902C0002, 0x3DE, data->x, data->y, 1);
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
 * 0x88) and plays sound 0xE48744C4. */
void CollectibleSuperWillieTickCallback(PowerupCollectibleEntity *e) {
    DecorEntityTickWithOffscreenCheck((Entity *)e);
    if (e->triggerState != 0 ||
        CheckEntityBoxCollision((Entity *)e, 2) != 0) {
        e->labelEntity->collisionMask = 7;
        AddSuperWillies(PLAYER_STATE_DATA, 1);
        InitDecorEntityWithScreenOffset((Entity *)e, 0x118, 0x88, 1);
        PlayEntityPositionSound((Entity *)e, 0xE48744C4);
    }
}

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
    SetEntitySpriteId((Entity *)e, 0xA9228088, 1);
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

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityCollisionHandler);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntityCollisionHandlerExt);

INCLUDE_ASM("asm/nonmatchings/pickups", DecorPlaySoundAndAnimate);

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

