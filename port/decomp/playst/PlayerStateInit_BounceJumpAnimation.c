/* =============================================================================
 * PlayerStateInit_BounceJumpAnimation.c  --  PC-port conversion (0x800694AC)
 * =============================================================================
 * Follow-up state after the platform-jump launch animation finishes: keeps the
 * airborne flag set, installs the mid-air FSM slots (tick +0x0
 * PlayerState_CooldownTick, event +0x8 PlayerEntityCollisionHandler, input
 * +0x104 PlayerCallback_JumpInputAndCounters, movement +0x1C
 * PlayerCallback_FallingPhysicsMain), switches to the airborne sprite
 * 0x1C3AA013 with render flags, loop frame 0x1084280 and sprite callback
 * 0x2421405, then arms the hidden-idle callback pair D_800A5F14/D_800A5F18.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_CooldownTick();
extern void PlayerEntityCollisionHandler();
extern void PlayerCallback_JumpInputAndCounters();
extern void PlayerCallback_FallingPhysicsMain();
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flag);
extern void EntitySetRenderFlags(void *e, s32 flags);
extern void SetAnimationLoopFrame(void *e, u32 id);
extern void SetAnimationSpriteCallback(void *e, u32 id);
extern void EntitySetCallback(void *e, u32 marker, u32 callback);

extern u32 PlayerHiddenIdleMarker asm("D_800A5F14");
extern u32 PlayerHiddenIdleFn     asm("D_800A5F18");

#define INSTALL_SLOT(base, off, fn)                                   \
    do {                                                              \
        *(u32 *)((base) + (off)) = 0xFFFF0000u;                       \
        *(void **)((base) + (off) + 4) = (void *)(fn);                \
    } while (0)

void PlayerStateInit_BounceJumpAnimation(void *arg0) {
    u8 *e = (u8 *)arg0;

    ((u8 *)g_pGameState)[0x60] = 1;

    INSTALL_SLOT(e, 0x0, PlayerState_CooldownTick);
    INSTALL_SLOT(e, 0x8, PlayerEntityCollisionHandler);
    INSTALL_SLOT(e, 0x104, PlayerCallback_JumpInputAndCounters);
    INSTALL_SLOT(e, 0x1C, PlayerCallback_FallingPhysicsMain);

    SetEntitySpriteId(arg0, 0x1C3AA013, 1);
    EntitySetRenderFlags(arg0, 1);
    SetAnimationLoopFrame(arg0, 0x1084280);
    SetAnimationSpriteCallback(arg0, 0x2421405);
    EntitySetCallback(arg0, PlayerHiddenIdleMarker, PlayerHiddenIdleFn);
}
