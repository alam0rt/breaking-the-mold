#include "common.h"
#include "functions.h"
#include "Game/callback_slot.h"
#include "globals.h"

extern void *g_pBlbHeapBase;
extern void EntitySetRenderFlags(void *e, u32 flags);
extern void *D_80011784;
extern void *D_800117C4;
extern void *D_800117E4;
extern void *D_80011824;
extern void *D_80011D14;
extern void FreeEntityNoTeardown_8006ed88(void *e, u32 size);
extern void FreeEntityNoTeardown_80070548(void *e, u32 size);
extern void EntitySetState(Entity *e, u32 marker, void *fn);
extern void ApplyAnimationPositionOffsets(Entity *e);
extern void FinnCheckTriggerZones(Entity *e);
extern void FinnHandleInput(Entity *e);
extern void FinnVehicleMovementUpdate(Entity *e);
extern void FinnUpdateRotationSprite(Entity *e);
extern void EntityUpdateCallback(Entity *e);

/* gp_rel tentative defs (sdata blob owns the strong defs). */
u32   D_800A5F8C;
void *D_800A5F90;

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

void ClearEntityStateFlag(void *e) {
    *(u8 *)((u8 *)e + 0x10D) = 0;
}

void SetEntityStateFlagWithValue(void *e, u8 val) {
    *(u8 *)((u8 *)e + 0x10F) = val;
    *(u8 *)((u8 *)e + 0x10D) = 1;
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_SetIdleSpriteAndPause);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_FaceRightAndAnimate);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_AnimateAndQueueIdle);

void FinnSubentityDestroyCallback_Vtable0x80011784(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_80011784;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_8006EB14(void *e) {
    *(u8 *)((u8 *)e + 0x2C) = 1;
}

void func_8006EB20(void *e) {
    u8 *p = *(u8 **)((u8 *)e + 0x24);
    p[0x1E0] = 0x20;
}

u32 func_8006EB30(void *e) {
    return *(u32 *)((u8 *)e + 0x24);
}

void FinnSubentityDestroyCallback_Vtable0x800117c4(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800117C4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void FinnSubentityDestroyCallback_Vtable0x800117e4(void *entity, s32 flags) {
    ((Entity *)entity)->collisionVtable = &D_800117E4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_8006EC04(void *e) {
    *(u8 *)((u8 *)e + 0x1B1) = 1;
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
void func_8006EC40(void *e, func_8006EC40_Pair p) {
    *(func_8006EC40_Pair *)((u8 *)e + 0x104) = p;
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
    t1 = *(volatile u8 *)&e->currentRGB[0];
    e->currentRGB[1] = g;
    t2 = *(volatile u8 *)&e->currentRGB[1];
    e->currentRGB[2] = b;
    ptr[0x34] = t1;
    ptr[0x35] = t2;
    ptr[0x36] = b;
}

u32 func_8006ED30(void *e) {
    return *(u32 *)((u8 *)e + 0x100);
}

void func_8006ED3C(void *e, u32 val) {
    *(u32 *)((u8 *)e + 0x100) = val;
}

void func_8006ED44(void) {
}

void func_8006ED4C(void) {
}

void FinnSubentityDestroyCallback_Simple(void *entity, u32 flag) {
    ((Entity *)entity)->collisionVtable = &D_80011824;
    if (flag & 1) {
        FreeEntityNoTeardown_8006ed88(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_8006ed88(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/finn", CreateGlidePlayerEntity);

extern void StopSPUVoice(s32 voice);
extern void *D_80011CF4;

void FinnEntityDestroyWithSoundCleanup(u8 *e, s32 mode) {
    s32 voice = *(s32 *)(e + 0x114);
    *(void **)(e + 0x18) = &D_80011CF4;
    if (voice >= 0) {
        StopSPUVoice(voice);
        *(s32 *)(e + 0x114) = -1;
    }
    DestroyEntityAndFreeMemory((SpriteEntity *)e, 0);
    if (mode & 1) {
        FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
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

s32 FinnEvent_DamageToDeathExplosion(void *e, u32 ev, u32 a2, u32 a3) {
    if ((ev & 0xFFFF) == EVT_DAMAGE) {
        EntitySetState(e, D_800A5F8C, D_800A5F90);
    }
    return 0;
}

s32 FINNEventHandler_DeathExplosion(void *e, u32 ev) {
    u32 m = ev & 0xFFFF;
    if (m == EVT_DAMAGE) {
        EntitySetState(e, D_800A5F8C, D_800A5F90);
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

void RunnState_SetAdvanceLevelAndHide(void *e) {
    g_pGameState->advance_level_flag = 1;
    EntitySetRenderFlags(e, 0);
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnStateInit_SetTimerAndTick);

s32 IsTileTypeSolidOrHazard(s32 unused, u8 *tilePtr) {
    u8 tile = *tilePtr;
    if ((u8)(tile + 0x4B) < 3) return 1;
    if (tile == 0xC9 || tile == 0xCB || (u8)(tile + 0x23) < 3) return 1;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnCheckTriggerZones);

INCLUDE_ASM("asm/nonmatchings/finn", SpawnAngledProjectile);

u32 func_800704F0(void *e) {
    return *(u32 *)((u8 *)e + 0x100);
}

void func_800704FC(void *e, u32 val) {
    *(u32 *)((u8 *)e + 0x100) = val;
}

void func_80070504(void) {
}

void func_8007050C(void) {
}

void EntityDestructor_Simple13(void *entity, u32 flag) {
    ((Entity *)entity)->collisionVtable = &D_80011D14;
    if (flag & 1) {
        FreeEntityNoTeardown_80070548(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_80070548(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnSpawnCountdown);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSpawnCountdownTickCallback);

INCLUDE_ASM("asm/nonmatchings/finn", FinnExitMoveRightTickCallback);

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnScaledSprite);

void RunnRender_AnimOffsetWithSlide(Entity *e) {
    ApplyAnimationPositionOffsets(e);
    if (*(s16 *)((u8 *)e + 0xDA) < 0x11) {
        e->worldX += 5;
    }
}

INCLUDE_ASM("asm/nonmatchings/finn", RunnEvent_CutsceneDebrisAndLevel);

INCLUDE_ASM("asm/nonmatchings/finn", CreateSoarPlayerEntity);

