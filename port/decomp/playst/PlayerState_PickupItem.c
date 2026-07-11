/* =============================================================================
 * PlayerState_PickupItem.c  --  functional-C body for playst.c
 * PlayerState_PickupItem (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerState_PickupItem.s
 * (0x80068B48, 0x1F0). Player state after collecting a pickup: seeds the walk
 * speed (0x3.0 with the run mask D_800A5970 held else 0x2.0, +0x8000
 * subpixel), zeroes the jump/physics scratch, installs the pickup slot set
 * (tick = PlayerState_CooldownTick, event = PlayerBounceCallback, +0x104 =
 * JumpInputAndCounters, physics = WalkingCollisionHandler -- or
 * IdleOnPlatform when riding, +0x12C set), sprite 0x1C3AA013 (frame 1 kept
 * only when the previous sprite +0xCC was one of the two walk sprites), the
 * bounce-init slot and the PlayerHiddenIdle callback pair D_800A5F14/18.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_CooldownTick(void);
extern void PlayerBounceCallback(void);
extern void PlayerCallback_JumpInputAndCounters(void);
extern void PlayerCallback_WalkingCollisionHandler(void);
extern void PlayerCallback_IdleOnPlatform(void);
extern void PlayerStateInit_BounceActive(void);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 mode);
extern void SetAnimationFrameIndex(void *e, s16 frame);
extern void EntitySetCallback(void *e, s32 marker, s32 fn);
extern u16  D_800A5970 asm("D_800A5970");   /* run/dash button mask */
extern s32  D_800A5F14 asm("D_800A5F14");
extern s32  D_800A5F18 asm("D_800A5F18");

void PlayerState_PickupItem(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *input = *(u8 **)(e + 0x100);
    void *physFn;
    s32 xvel;
    u32 prevSprite;

    ((u8 *)g_pGameState)[0x60] = 1;

    xvel = (D_800A5970 & *(u16 *)(input + 0x0)) ? 0x30000 : 0x20000;
    *(s32 *)(e + 0x124) = xvel | 0x8000;
    e[0x11C] = 0;
    e[0x11E] = 0;
    e[0x11F] = 0;
    *(s32 *)(e + 0x120) = 0;
    *(s32 *)(e + 0x114) = 0;
    *(s32 *)(e + 0x118) = 0;
    e[0x13C] = 0;

    physFn = (*(void **)(e + 0x12C) != NULL)
                 ? (void *)PlayerCallback_IdleOnPlatform
                 : (void *)PlayerCallback_WalkingCollisionHandler;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)PlayerState_CooldownTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)PlayerBounceCallback;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;          /* jump/input slot */
    *(void **)(e + 0x108) = (void *)PlayerCallback_JumpInputAndCounters;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;           /* physics slot */
    *(void **)(e + 0x20) = physFn;

    SetEntitySpriteId(e, 0x1C3AA013u, 1);
    prevSprite = *(u32 *)(e + 0xCC);
    if (prevSprite == 0x1C395196u || prevSprite == 0x3838801Au) {
        SetAnimationFrameIndex(e, 1);
    }

    *(u32 *)(e + 0x98) = 0xFFFF0000u;           /* bounce-init slot */
    *(void **)(e + 0x9C) = (void *)PlayerStateInit_BounceActive;

    EntitySetCallback(e, D_800A5F14, D_800A5F18);
}
