/* =============================================================================
 * PlayerState_LandingFromRide.c  --  functional-C body for playst.c
 * PlayerState_LandingFromRide (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerState_LandingFromRide.s
 * (0x80068D38, 0x248). Player state entered when dismounting a ride/carry:
 * seeds the hop-off velocities (X speed 0x3.0 when the run mask D_800A5970 is
 * held else 0x2.0, +0x8000 subpixel; Y velocity -0x1200; +0x110 momentum
 * -6.0; jump counter +0x11C = 5), notifies the ride entity (+0x12C) with
 * event 0x1005 through its tagged event slot (fsm_send_event) and detaches
 * it, then installs the same normal-play slot set as PlayerState_
 * CheckpointExit, the landing sprite 0x1C3AA013 at frame 1, and finally
 * EntitySetCallback with the PlayerHiddenIdle pair D_800A5F14/18.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

extern void PlayerState_CooldownTick(void);
extern void PlayerCallback_CollisionHandlerWithQueue(void);
extern void PlayerCallback_JumpInputAndCounters(void);
extern void PlayerCallback_FallingPhysicsMain(void);
extern void PlayerStateInit_BounceActive(void);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 mode);
extern void SetAnimationFrameIndex(void *e, s16 frame);
extern void EntitySetCallback(void *e, s32 marker, s32 fn);
extern u16  D_800A5970 asm("D_800A5970");   /* run/dash button mask */
extern s32  D_800A5F14 asm("D_800A5F14");
extern s32  D_800A5F18 asm("D_800A5F18");

void PlayerState_LandingFromRide(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *input = *(u8 **)(e + 0x100);
    u8 *ride;
    s32 xvel;

    ((u8 *)g_pGameState)[0x60] = 1;

    xvel = (D_800A5970 & *(u16 *)(input + 0x0)) ? 0x30000 : 0x20000;
    *(s32 *)(e + 0x124) = xvel | 0x8000;

    ride = *(u8 **)(e + 0x12C);
    e[0x11C] = 5;
    *(s32 *)(e + 0x118) = -0x1200;
    *(s32 *)(e + 0x114) = 0;
    e[0x11E] = 0;
    e[0x11F] = 0;
    *(s32 *)(e + 0x120) = 0;
    *(s32 *)(e + 0x110) = (s32)0xFFFA0000;

    if (ride != NULL) {
        fsm_send_event(ride, 0x1005, 0, e);
        *(u8 **)(e + 0x12C) = NULL;
    }
    e[0x13C] = 0;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)PlayerState_CooldownTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)PlayerCallback_CollisionHandlerWithQueue;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;          /* jump/input slot */
    *(void **)(e + 0x108) = (void *)PlayerCallback_JumpInputAndCounters;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;           /* physics slot */
    *(void **)(e + 0x20) = (void *)PlayerCallback_FallingPhysicsMain;

    SetEntitySpriteId(e, 0x1C3AA013u, 1);
    SetAnimationFrameIndex(e, 1);

    *(u32 *)(e + 0x98) = 0xFFFF0000u;           /* bounce-init slot */
    *(void **)(e + 0x9C) = (void *)PlayerStateInit_BounceActive;

    EntitySetCallback(e, D_800A5F14, D_800A5F18);
}
