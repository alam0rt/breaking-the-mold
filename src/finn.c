#include "common.h"

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

INCLUDE_ASM("asm/nonmatchings/finn", SetEntityStateFlagWithValue);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_SetIdleSpriteAndPause);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_FaceRightAndAnimate);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubState_AnimateAndQueueIdle);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubentityDestroyCallback_Vtable0x80011784);

void func_8006EB14(void *e) {
    *(u8 *)((u8 *)e + 0x2C) = 1;
}

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EB20);

u32 func_8006EB30(void *e) {
    return *(u32 *)((u8 *)e + 0x24);
}

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubentityDestroyCallback_Vtable0x800117c4);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubentityDestroyCallback_Vtable0x800117e4);

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

INCLUDE_ASM("asm/nonmatchings/finn", func_8006ECE4);

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

INCLUDE_ASM("asm/nonmatchings/finn", FinnSubentityDestroyCallback_Simple);

INCLUDE_ASM("asm/nonmatchings/finn", FreeEntityNoTeardown_8006ed88);

INCLUDE_ASM("asm/nonmatchings/finn", CreateGlidePlayerEntity);

INCLUDE_ASM("asm/nonmatchings/finn", FinnEntityDestroyWithSoundCleanup);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EF48);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EF64);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EF84);

INCLUDE_ASM("asm/nonmatchings/finn", func_8006EFA4);

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

INCLUDE_ASM("asm/nonmatchings/finn", EntityDestructor_Simple13);

INCLUDE_ASM("asm/nonmatchings/finn", FreeEntityNoTeardown_80070548);

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnSpawnCountdown);

INCLUDE_ASM("asm/nonmatchings/finn", FinnSpawnCountdownTickCallback);

INCLUDE_ASM("asm/nonmatchings/finn", FinnExitMoveRightTickCallback);

INCLUDE_ASM("asm/nonmatchings/finn", InitEntity_FinnScaledSprite);

INCLUDE_ASM("asm/nonmatchings/finn", RunnRender_AnimOffsetWithSlide);

INCLUDE_ASM("asm/nonmatchings/finn", RunnEvent_CutsceneDebrisAndLevel);

INCLUDE_ASM("asm/nonmatchings/finn", CreateSoarPlayerEntity);

