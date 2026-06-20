#include "common.h"
#include "functions.h"
#include "Game/callback_slot.h"

extern void *g_pBlbHeapBase;
extern u8 DECOR_ENTITY_DESTRUCTOR_VTABLE[] asm("D_80010870");
extern u8 DECOR_SIMPLE_ALLOC_VTABLE[] asm("D_80010890");
extern void FreeEntityNoTeardown_80030cdc(Entity *e, u32 size);
extern void CollisionCheckWrapper(Entity *e, u32 a, u32 b, u32 c);
extern void DecorEntityTickWithOffscreenCheck(Entity *e);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);
extern void CheckpointSwampTickCallback(Entity *e);
extern void EntityCollisionHandler_SpecialTrigger(Entity *e);
extern void EntityCollision_FlagAndDispatch(Entity *e);

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

INCLUDE_ASM("asm/nonmatchings/pickups", DecorEntity_CollectWithSwirlyEffect);

INCLUDE_ASM("asm/nonmatchings/pickups", InitDecorEntityWithHUDIcon);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityDestructor_FreeRenderListAt120);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleOrbTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", EntityRenderWithScaledPosition);

INCLUDE_ASM("asm/nonmatchings/pickups", func_8002D978);

INCLUDE_ASM("asm/nonmatchings/pickups", TriggerFadeOut);

INCLUDE_ASM("asm/nonmatchings/pickups", InitYellowBirdCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleYellowBirdTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitClayballWithRandomColor);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleClaySingleTickCallback);

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

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleExtraLifeTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitPhoenixHandCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectiblePhoenixHandTickCallback);

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

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleHamsterShieldTickCallback);

INCLUDE_ASM("asm/nonmatchings/pickups", InitSuperWillieCollectible);

INCLUDE_ASM("asm/nonmatchings/pickups", CollectibleSuperWillieTickCallback);

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

