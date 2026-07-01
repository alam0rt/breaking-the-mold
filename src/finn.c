#include "common.h"
#include "functions.h"
#include "Game/callback_slot.h"
#include "Game/fsm_dispatch.h"
#include "globals.h"

extern void *g_pBlbHeapBase;
extern void EntitySetRenderFlags(Entity *e, u32 flags);
extern u8 FINN_SUBENTITY_VTABLE_1784[] asm("D_80011784");
extern u8 FINN_SUBENTITY_VTABLE_17C4[] asm("D_800117C4");
extern u8 FINN_SUBENTITY_VTABLE_17E4[] asm("D_800117E4");
extern u8 FINN_SIMPLE_ALLOC_VTABLE_1824[] asm("D_80011824");
extern u8 FINN_SIMPLE_ALLOC_VTABLE_1D14[] asm("D_80011D14");
extern void FreeEntityNoTeardown_8006ed88(Entity *e, u32 size);
extern void FreeEntityNoTeardown_80070548(Entity *e, u32 size);
extern void EntitySetState(Entity *e, u32 marker, EntityCallback fn);
extern void ApplyAnimationPositionOffsets(Entity *e);
extern void FinnCheckTriggerZones(Entity *e);
extern void FinnHandleInput(Entity *e);
extern void FinnVehicleMovementUpdate(Entity *e);
extern void FinnUpdateRotationSprite(Entity *e);
extern void EntityUpdateCallback(Entity *e);
extern void FinnTick_LevelExitCountdown(Entity *e);
extern void FinnExitMoveRightTickCallback(Entity *e);
extern void SetAnimationActive(Entity *entity, u8 value);

typedef struct FinnSubentityStateFlags {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10D - 0x100];
    /* 0x10D */ u8 stateFlag;
    /* 0x10E */ u8 pad10E;
    /* 0x10F */ u8 stateValue;
} FinnSubentityStateFlags;

typedef struct FinnPointerEntity {
    /* 0x00 */ u8 pad00[0x24];
    /* 0x24 */ u8 *target;
    /* 0x28 */ u8 pad28[0x2C - 0x28];
    /* 0x2C */ u8 smallFlag;
} FinnPointerEntity;

typedef struct FinnPairEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x104 - 0x100];
    /* 0x104 */ s32 pairA;
    /* 0x108 */ s32 pairB;
} FinnPairEntity;

typedef struct FinnStateValueEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u32 stateValue;
} FinnStateValueEntity;

/* Used by FinnSpawnCountdownTickCallback: u8 down-counter at +0x100; when
 * it hits 0 the render slot is overwritten with FinnExitMoveRightTickCallback. */
typedef struct FinnSpawnCountdownEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 spawnCountdown;
} FinnSpawnCountdownEntity;

typedef struct FinnVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x114 - 0x100];
    /* 0x114 */ s32 voiceHandle;
} FinnVoiceEntity;

/* Used by FinnTick_LevelExitCountdown / FinnStateInit_SetTimerAndTick.
 * +0x118 is a u8 down-counter decremented each tick; when it hits 0 the
 * tick callback flips +0x11A from 0 to 1. */
typedef struct FinnLevelExitEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x118 - 0x100];
    /* 0x118 */ u8 exitTimer;
    /* 0x119 */ u8 pad119;
    /* 0x11A */ u8 exitFlag;
} FinnLevelExitEntity;

/* gp_rel tentative defs (sdata blob owns the strong defs). */
u32 FINN_DEATH_EXPLOSION_STATE_MARKER asm("D_800A5F8C");
EntityCallback FINN_DEATH_EXPLOSION_STATE_CALLBACK asm("D_800A5F90");

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubentityUpdatePositionFromParent);

/* func_8006E084: unit spans 0x8006E084..0x8006E130 — absorbs former split symbols FINNRenderCallback_UpdateScreenPosition, func_8006E0CC, FINNRenderCallback_UpdateScaledPosition (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/finn", func_8006E084);




/* If entity flag +0x2C is set, dispatch g_pGameState's event FSM callback
 * (marker +0x8/0xA, fn +0xC) with eventId 3, forwarding entity+0x24 as the arg
 * and the entity as srcEntity. Same FSM-slot forwarder as func_80034B10 (the
 * dispatch body was mis-split as a bogus FINNCallback_DispatchToStateHandler
 * symbol; merged here). fn/then-fn pinned to $t2/$t1. */
typedef void (*GsEventCB)();
typedef struct { s32 arg; GsEventCB fn; } GsEventSlot;

void func_8006E130(Entity *e) {
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

    if (*(u8 *)((u8 *)e + 0x2C) == 0) {
        return;
    }
    gs = g_pGameState;
    m = ((s16 *)&gs->event_marker)[1];
    arg = *(s32 *)((u8 *)e + 0x24);
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

INCLUDE_ASM("asm/nonmatchings/finn", CreateYellowBirdEntity);

INCLUDE_ASM("asm/nonmatchings/finn", EntityTick_FollowPlayerWithInterpolation);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubentityFollowWithDistanceCheck);

s32 FinnSubEvent_ProcessQueueOnReady(Entity *e, u32 ev) {
    if ((ev & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return 0;
}

void ClearEntityStateFlag(FinnSubentityStateFlags *e) {
    e->stateFlag = 0;
}

void SetEntityStateFlagWithValue(FinnSubentityStateFlags *e, u8 val) {
    e->stateValue = val;
    e->stateFlag = 1;
}

/* Subentity state switch: clears the event-callback slot, switches sprite
 * to 0xC87CA082 (the idle sprite), and pauses the animation if the
 * paused-flag at +0x10E is not already set. Used by FINNCallback_DispatchTo
 * to put a subentity into its "frozen idle" state. */
void FinnSubState_SetIdleSpriteAndPause(FinnSubentityStateFlags *e) {
    PaddedSlotPair slot;

    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = 0;
    slot.s[0].fn = NULL;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s[0];
    SetEntitySpriteId((Entity *)e, 0xC87CA082, 1);
    if (e->pad10E == 0) {
        SetAnimationActive((Entity *)e, 0);
    }
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_FaceRightAndAnimate);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_AnimateAndQueueIdle);

void FinnSubentityDestroyCallback_Vtable0x80011784(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = FINN_SUBENTITY_VTABLE_1784;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void func_8006EB14(FinnPointerEntity *e) {
    e->smallFlag = 1;
}

void func_8006EB20(FinnPointerEntity *e) {
    u8 *p = e->target;
    p[0x1E0] = 0x20;
}

u8 *func_8006EB30(FinnPointerEntity *e) {
    return e->target;
}

void FinnSubentityDestroyCallback_Vtable0x800117c4(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = FINN_SUBENTITY_VTABLE_17C4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void FinnSubentityDestroyCallback_Vtable0x800117e4(SpriteEntity *entity, s32 flags) {
    entity->base.collisionVtable = FINN_SUBENTITY_VTABLE_17E4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)entity, 0, 0);
    }
}

void func_8006EC04(PlayerEntity *e) {
    e->portalCheatFlag = 1;
}

void func_8006EC10(PlayerEntity *e) {
    e->shrinkFlag = 1;
}

void func_8006EC1C(PlayerEntity *e) {
    e->particleFlag = 1;
}

s32 func_8006EC28(s32 a, u8 b) {
    return b ? b < 0x3C : 0;
}

typedef struct { s32 a; s32 b; } func_8006EC40_Pair;
void func_8006EC40(FinnPairEntity *e, func_8006EC40_Pair p) {
    *(func_8006EC40_Pair *)&e->pairA = p;
}

INCLUDE_ASM("asm/nonmatchings/finn", FINNCallback_DispatchToEntityHandler);

void func_8006ECE4(PlayerEntity *e, u8 *o1, u8 *o2, u8 *o3) {
    *o1 = e->currentRGB[0];
    *o2 = e->currentRGB[1];
    *o3 = e->currentRGB[2];
}

void func_8006ED08(PlayerEntity *e, u8 r, u8 g, u8 b) {
    u8 *ptr = e->sprite.base.spriteContext;
    u8 t1, t2;
    e->currentRGB[0] = r;
    /* @hack: volatile reload forces cc1 to re-fetch the byte just stored (instead of reusing the register), matching the target's lbu after sb sequence. */
    t1 = *(volatile u8 *)&e->currentRGB[0];
    e->currentRGB[1] = g;
    /* @hack: volatile reload forces cc1 to re-fetch the byte just stored (matches target instruction order). */
    t2 = *(volatile u8 *)&e->currentRGB[1];
    e->currentRGB[2] = b;
    ptr[0x34] = t1;
    ptr[0x35] = t2;
    ptr[0x36] = b;
}

u32 func_8006ED30(FinnStateValueEntity *e) {
    return e->stateValue;
}

void func_8006ED3C(FinnStateValueEntity *e, u32 val) {
    e->stateValue = val;
}

void func_8006ED44(void) {
}

void func_8006ED4C(void) {
}

void FinnSubentityDestroyCallback_Simple(Entity *entity, u32 flag) {
    entity->collisionVtable = FINN_SIMPLE_ALLOC_VTABLE_1824;
    if (flag & 1) {
        FreeEntityNoTeardown_8006ed88(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_8006ed88(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/finn", CreateGlidePlayerEntity);

extern void StopSPUVoice(s32 voice);
extern u8 FINN_SOUND_CLEANUP_VTABLE[] asm("D_80011CF4");

void FinnEntityDestroyWithSoundCleanup(FinnVoiceEntity *e, s32 mode) {
    s32 voice = e->voiceHandle;
    e->sprite.base.collisionVtable = FINN_SOUND_CLEANUP_VTABLE;
    if (voice >= 0) {
        StopSPUVoice(voice);
        e->voiceHandle = -1;
    }
    DestroyEntityAndFreeMemory(&e->sprite, 0);
    if (mode & 1) {
        FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
    }
}

s32 func_8006EF48(Entity *e) {
    u8 a = *(u8 *)&e->worldX & 0xF;
    u8 b = *(u8 *)&e->worldY & 0xF;
    return (a + b) < 0x10;
}

s32 func_8006EF64(Entity *e) {
    u8 a = *(u8 *)&e->worldX & 0xF;
    u8 b = *(u8 *)&e->worldY & 0xF;
    return !((a + b) < 0xF);
}

s32 func_8006EF84(Entity *e) {
    s32 a = (s32)(*(u8 *)&e->worldX & 0xF) + 0xF;
    s32 b = *(u8 *)&e->worldY & 0xF;
    return (a - b) < 0x10;
}

s32 func_8006EFA4(Entity *e) {
    s32 a = (s32)(*(u8 *)&e->worldX & 0xF) + 0xF;
    s32 b = *(u8 *)&e->worldY & 0xF;
    return !((a - b) < 0xF);
}

/* FinnMainTickHandler: unit spans 0x8006EFC8..0x8006F0A8 — absorbs former split symbols FinnTick_NormalMovementAndInput (Ghidra labels with no external references; merged 2026-07-02). */
INCLUDE_ASM("asm/nonmatchings/finn", FinnMainTickHandler);


INCLUDE_ASM("asm/nonmatchings/finn", FinnTick_LevelExitCountdown);

s32 FinnEvent_DamageToDeathExplosion(Entity *e, u32 ev, u32 a2, u32 a3) {
    if ((ev & 0xFFFF) == EVT_DAMAGE) {
        EntitySetState(e, FINN_DEATH_EXPLOSION_STATE_MARKER, FINN_DEATH_EXPLOSION_STATE_CALLBACK);
    }
    return 0;
}

s32 FINNEventHandler_DeathExplosion(Entity *e, u32 ev) {
    u32 m = ev & 0xFFFF;
    if (m == EVT_DAMAGE) {
        EntitySetState(e, FINN_DEATH_EXPLOSION_STATE_MARKER, FINN_DEATH_EXPLOSION_STATE_CALLBACK);
    }
    if (m == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return 0;
}

s32 FinnEvent_QueueOnAnimReady(Entity *e, u32 ev) {
    if ((ev & 0xFFFF) == EVT_TICK) {
        EntityProcessCallbackQueue(e);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnVehicleMovementUpdate);

INCLUDE_ASM("asm/nonmatchings/finn", FinnHandleInput);

INCLUDE_ASM("asm/nonmatchings/finn", FinnUpdateRotationSprite);

extern void FinnMainTickHandler();
extern u32 FINN_ROTATION_SPRITE_TABLE[] asm("D_8009CA7C");

typedef struct FinnAngleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8  pad100[0xC];
    /* 0x10C */ s16 rotationAngle;
    /* 0x10E */ u8  spriteBucket;
} FinnAngleEntity;

/* Initializer that faces the FINN craft by its heading: installs the main tick
 * + damage/death event handlers, converts the 0x400-unit rotation angle (+0x10C)
 * into a 16-way sprite bucket ((angle+0x80)>>8 & 0xF, cached at +0x10E), stashes
 * the sign-extended low byte of the angle in targetY (+0x72), and loads the
 * bucket's sprite from the 16-entry rotation table. */
void FinnStateInit_SetSpriteByAngle(FinnAngleEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    s16 angle;
    u8 bucket;

    do {} while (0);
    fn = (void (*)())FinnMainTickHandler;
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
    fn = (void (*)())FinnEvent_DamageToDeathExplosion;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.eventMarker = slot.s;
    angle = e->rotationAngle;
    bucket = ((s32)(angle + 0x80) >> 8) & 0xF;
    e->spriteBucket = bucket;
    e->sprite.base.targetY = (s8)angle;
    SetEntitySpriteId((Entity *)e, FINN_ROTATION_SPRITE_TABLE[bucket], 1);
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnDeathExplosion);

void RunnState_SetAdvanceLevelAndHide(Entity *e) {
    g_pGameState->advance_level_flag = 1;
    EntitySetRenderFlags(e, 0);
}

/* Initializer for the level-exit state: arms the down-counter at 0x5C
 * frames, clears the exit-fired flag, and installs FinnTick_LevelExitCountdown
 * as the per-frame tick callback (marker 0xFFFF0000 = direct call). */
void FinnStateInit_SetTimerAndTick(FinnLevelExitEntity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    e->exitFlag = 0;
    e->exitTimer = 0x5C;
    do {} while (0);
    fn = (void (*)())FinnTick_LevelExitCountdown;
    do {} while (0);
    m1 = -1;
    slot.s.markerLo = 0;
    slot.s.markerHi = m1;
    slot.s.fn = fn;
    *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
}

s32 IsTileTypeSolidOrHazard(s32 unused, u8 *tilePtr) {
    u8 tile = *tilePtr;
    if ((u8)(tile + 0x4B) < 3) return 1;
    if (tile == 0xC9 || tile == 0xCB || (u8)(tile + 0x23) < 3) return 1;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnCheckTriggerZones);

INCLUDE_ASM("asm/nonmatchings/finn", SpawnAngledProjectile);

u32 func_800704F0(FinnStateValueEntity *e) {
    return e->stateValue;
}

void func_800704FC(FinnStateValueEntity *e, u32 val) {
    e->stateValue = val;
}

void func_80070504(void) {
}

void func_8007050C(void) {
}

void EntityDestructor_Simple13(Entity *entity, u32 flag) {
    entity->collisionVtable = FINN_SIMPLE_ALLOC_VTABLE_1D14;
    if (flag & 1) {
        FreeEntityNoTeardown_80070548(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80070548(Entity *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, (u8 *)e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnSpawnCountdown);

/* Per-frame tick: decrements the u8 spawn-countdown at +0x100. When it
 * reaches 0, swaps the entity's render slot to FinnExitMoveRightTickCallback
 * (the post-spawn movement driver). Also pins the entity as the current
 * player-entity pointer so HUD/camera routines see it. */
void FinnSpawnCountdownTickCallback(FinnSpawnCountdownEntity *e) {
    PadSlot slot;
    s16 m1;
    EntityCallback fn;

    g_pGameState->player_entity_ptr = e;
    if (e->spawnCountdown != 0) {
        e->spawnCountdown--;
        if (e->spawnCountdown == 0) {
            fn = FinnExitMoveRightTickCallback;
            m1 = -1;
            do {} while (0);
            slot.s.markerLo = 0;
            slot.s.markerHi = m1;
            slot.s.fn = fn;
            *(CallbackSlot *)&e->sprite.base.renderMarker = slot.s;
        }
    }
    EntityUpdateCallback((Entity *)e);
}

/* Per-frame tick after Finn has spawned: shoves the entity 5 pixels to
 * the right each frame and, once it crosses (camera_limit_x - 0xA0)
 * (the right edge of the level minus a screen-width's worth of pixels),
 * clears its own render slot to remove the post-exit sprite from the
 * frame buffer chain. */
void FinnExitMoveRightTickCallback(Entity *e) {
    PaddedSlotPair slot;

    e->worldX += 5;
    if (e->worldX >= g_pGameState->camera_limit_x - 0xA0) {
        slot.s[0].markerLo = 0;
        slot.s[0].markerHi = 0;
        slot.s[0].fn = NULL;
        *(CallbackSlot *)&e->renderMarker = slot.s[0];
    }
}

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnScaledSprite);

void RunnRender_AnimOffsetWithSlide(SpriteEntity *e) {
    ApplyAnimationPositionOffsets((Entity *)e);
    if (e->currentFrame < 0x11) {
        e->base.worldX += 5;
    }
}

INCLUDE_ASM("asm/nonmatchings/finn", RunnEvent_CutsceneDebrisAndLevel);

INCLUDE_ASM("asm/nonmatchings/finn", CreateSoarPlayerEntity);

