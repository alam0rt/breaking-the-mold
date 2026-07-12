/* =============================================================================
 * PlayerStateInit_BounceActive.c  --  functional-C body for playst.c
 * PlayerStateInit_BounceActive (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerStateInit_BounceActive.s
 * (0x80069600, 0x154). Installed in the +0x98 slot by the checkpoint-exit /
 * pickup / landing states and invoked when the landing animation completes:
 * installs the airborne slot set (tick = CooldownTick, event =
 * PlayerEntityCollisionHandler, +0x104 = JumpInputAndCounters, physics =
 * FallingPhysicsMain), the falling sprite 0x1C3AA013 with loop-frame/callback
 * anim ids (opaque hashes 0x01084280 / 0x02421405), render flags on, and the
 * PlayerHiddenIdle callback pair D_800A5F14/18.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_CooldownTick(void);
extern void PlayerEntityCollisionHandler(void);
extern void PlayerCallback_JumpInputAndCounters(void);
extern void PlayerCallback_FallingPhysicsMain(void);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 mode);
extern void EntitySetRenderFlags(void *e, s32 flags);
extern void SetAnimationLoopFrame(void *e, u32 frameId);
extern void SetAnimationSpriteCallback(void *e, u32 frameId);
extern void EntitySetCallback(void *e, s32 marker, s32 fn);
extern s32  D_800A5F14 asm("D_800A5F14");
extern s32  D_800A5F18 asm("D_800A5F18");

void PlayerStateInit_BounceActive(void *arg0) {
    u8 *e = (u8 *)arg0;

    ((u8 *)g_pGameState)[0x60] = 1;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)PlayerState_CooldownTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)PlayerEntityCollisionHandler;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;          /* jump/input slot */
    *(void **)(e + 0x108) = (void *)PlayerCallback_JumpInputAndCounters;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;           /* physics slot */
    *(void **)(e + 0x20) = (void *)PlayerCallback_FallingPhysicsMain;

    SetEntitySpriteId(e, 0x1C3AA013u, 1);
    EntitySetRenderFlags(e, 1);
    SetAnimationLoopFrame(e, 0x01084280u);
    SetAnimationSpriteCallback(e, 0x02421405u);
    EntitySetCallback(e, D_800A5F14, D_800A5F18);
}
