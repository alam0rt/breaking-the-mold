#include "common.h"
#include "functions.h"
#include "Game/callback_slot.h"
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

INCLUDE_ASM("asm/nonmatchings/finn", func_8006E084);

INCLUDE_ASM("asm/nonmatchings/finn", FINNRenderCallback_UpdateScreenPosition);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006E0CC);

INCLUDE_ASM("asm/nonmatchings/finn", FINNRenderCallback_UpdateScaledPosition);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006E130);

INCLUDE_ASM("asm/nonmatchings/finn", FINNCallback_DispatchToStateHandler);

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

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_SetIdleSpriteAndPause);

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
    e->_pad1B1[0] = 1;
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

INCLUDE_ASM("asm/nonmatchings/finn", FinnMainTickHandler);

INCLUDE_ASM("asm/nonmatchings/finn", FinnTick_NormalMovementAndInput);

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

INCLUDE_ASM("asm/nonmatchings/finn", FinnStateInit_SetSpriteByAngle);

INCLUDE_ASM("asm/nonmatchings/finn", FinnDeathExplosion);

void RunnState_SetAdvanceLevelAndHide(Entity *e) {
    g_pGameState->advance_level_flag = 1;
    EntitySetRenderFlags(e, 0);
}

/* Initializer for the level-exit state: arms the down-counter at 0x5C
 * frames, clears the exit-fired flag, and installs FinnTick_LevelExitCountdown
 * as the per-frame tick callback (marker 0xFFFF0000 = direct call).
 *
 * Equivalent to:
 *   void FinnStateInit_SetTimerAndTick(FinnLevelExitEntity *e) {
 *       PadSlot slot;
 *       e->exitFlag = 0;
 *       e->exitTimer = 0x5C;
 *       slot.s.markerLo = 0;
 *       slot.s.markerHi = -1;
 *       slot.s.fn = (void (*)())FinnTick_LevelExitCountdown;
 *       *(CallbackSlot *)&e->sprite.base.tickMarker = slot.s;
 *   }
 *
 * cc1 schedules the two byte stores (exitFlag at 0x11A, exitTimer at 0x118)
 * and the m1=-1 prep in a different order than TARGET regardless of source
 * order, volatile, or register-pin tricks — leaves a 270-byte diff that
 * couldn't be coaxed. Leaving as INCLUDE_ASM. */
INCLUDE_ASM("asm/nonmatchings/finn", FinnStateInit_SetTimerAndTick);

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

INCLUDE_ASM("asm/nonmatchings/finn", FinnSpawnCountdownTickCallback);

INCLUDE_ASM("asm/nonmatchings/finn", FinnExitMoveRightTickCallback);

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnScaledSprite);

void RunnRender_AnimOffsetWithSlide(SpriteEntity *e) {
    ApplyAnimationPositionOffsets((Entity *)e);
    if (e->currentFrame < 0x11) {
        e->base.worldX += 5;
    }
}

INCLUDE_ASM("asm/nonmatchings/finn", RunnEvent_CutsceneDebrisAndLevel);

INCLUDE_ASM("asm/nonmatchings/finn", CreateSoarPlayerEntity);

