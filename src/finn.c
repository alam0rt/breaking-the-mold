#include "common.h"

extern void *g_pBlbHeapBase;
extern void FreeFromHeap(void *heap, void *ptr, s32 a2, s32 a3);
extern void DestroyEntityAndFreeMemory(void *entity, s32 mode);
extern void *D_80011784;
extern void *D_800117C4;
extern void *D_800117E4;
extern void *D_80011824;
extern void *D_80011D14;
extern void FreeEntityNoTeardown_8006ed88(void *e, u32 size);
extern void FreeEntityNoTeardown_80070548(void *e, u32 size);

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

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubEvent_ProcessQueueOnReady);

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
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_80011784;
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
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_800117C4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void FinnSubentityDestroyCallback_Vtable0x800117e4(void *entity, s32 flags) {
    *(u32 *)((u8 *)entity + 0x18) = (u32)&D_800117E4;
    DestroyEntityAndFreeMemory(entity, 0);
    if (flags & 1) {
        FreeFromHeap(g_pBlbHeapBase, entity, 0, 0);
    }
}

void func_8006EC04(void *e) {
    *(u8 *)((u8 *)e + 0x1B1) = 1;
}

void func_8006EC10(void *e) {
    *(u8 *)((u8 *)e + 0x1B0) = 1;
}

void func_8006EC1C(void *e) {
    *(u8 *)((u8 *)e + 0x1AF) = 1;
}

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EC28);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EC40);

INCLUDE_ASM("asm/nonmatchings/finn", FINNCallback_DispatchToEntityHandler);

void func_8006ECE4(u8 *e, u8 *o1, u8 *o2, u8 *o3) {
    *o1 = e[0x15A];
    *o2 = e[0x15B];
    *o3 = e[0x15C];
}

INCLUDE_ASM("asm/nonmatchings/finn", func_8006ED08);

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
    *(void **)((u8 *)entity + 0x18) = &D_80011824;
    if (flag & 1) {
        FreeEntityNoTeardown_8006ed88(entity, 0x1C);
    }
}

void FreeEntityNoTeardown_8006ed88(void *e, u32 size) {
    FreeFromHeap(g_pBlbHeapBase, e, 0, 0);
}

INCLUDE_ASM("asm/nonmatchings/finn", CreateGlidePlayerEntity);

INCLUDE_ASM("asm/nonmatchings/finn", FinnEntityDestroyWithSoundCleanup);

s32 func_8006EF48(void *e) {
    u8 a = *(u8 *)((u8 *)e + 0x68) & 0xF;
    u8 b = *(u8 *)((u8 *)e + 0x6A) & 0xF;
    return (a + b) < 0x10;
}

s32 func_8006EF64(void *e) {
    u8 a = *(u8 *)((u8 *)e + 0x68) & 0xF;
    u8 b = *(u8 *)((u8 *)e + 0x6A) & 0xF;
    return !((a + b) < 0xF);
}

s32 func_8006EF84(void *e) {
    s32 a = (s32)(*(u8 *)((u8 *)e + 0x68) & 0xF) + 0xF;
    s32 b = *(u8 *)((u8 *)e + 0x6A) & 0xF;
    return (a - b) < 0x10;
}

s32 func_8006EFA4(void *e) {
    s32 a = (s32)(*(u8 *)((u8 *)e + 0x68) & 0xF) + 0xF;
    s32 b = *(u8 *)((u8 *)e + 0x6A) & 0xF;
    return !((a - b) < 0xF);
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnMainTickHandler);

INCLUDE_ASM("asm/nonmatchings/finn", FinnTick_NormalMovementAndInput);

INCLUDE_ASM("asm/nonmatchings/finn", FinnTick_LevelExitCountdown);

INCLUDE_ASM("asm/nonmatchings/finn", FinnEvent_DamageToDeathExplosion);

INCLUDE_ASM("asm/nonmatchings/finn", FINNEventHandler_DeathExplosion);

INCLUDE_ASM("asm/nonmatchings/finn", FinnEvent_QueueOnAnimReady);

INCLUDE_ASM("asm/nonmatchings/finn", FinnVehicleMovementUpdate);

INCLUDE_ASM("asm/nonmatchings/finn", FinnHandleInput);

INCLUDE_ASM("asm/nonmatchings/finn", FinnUpdateRotationSprite);

INCLUDE_ASM("asm/nonmatchings/finn", FinnStateInit_SetSpriteByAngle);

INCLUDE_ASM("asm/nonmatchings/finn", FinnDeathExplosion);

INCLUDE_ASM("asm/nonmatchings/finn", RunnState_SetAdvanceLevelAndHide);

INCLUDE_ASM("asm/nonmatchings/finn", FinnStateInit_SetTimerAndTick);

INCLUDE_ASM("asm/nonmatchings/finn", IsTileTypeSolidOrHazard);

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
    *(void **)((u8 *)entity + 0x18) = &D_80011D14;
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

INCLUDE_ASM("asm/nonmatchings/finn", RunnRender_AnimOffsetWithSlide);

INCLUDE_ASM("asm/nonmatchings/finn", RunnEvent_CutsceneDebrisAndLevel);

INCLUDE_ASM("asm/nonmatchings/finn", CreateSoarPlayerEntity);

