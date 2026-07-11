/* =============================================================================
 * PlayerState_CheckpointExit.c  --  functional-C body for playst.c
 * PlayerState_CheckpointExit (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerState_CheckpointExit.s
 * (0x8006A3F8, 0x18C). Player state entered when the teleporter portal
 * forwards event 0x100F (TeleporterPortalEventHandler) -- the player emerging
 * from a teleporter/checkpoint.
 *
 * Resets the motion/physics scratch block (+0x10C..+0x124, X velocity seeded
 * 0x28000), re-enables the sprite context (+0x34 byte +0xA = 1) and the
 * GameState player-active byte (+0x60 = 1), installs the normal-play slot set
 * (tick = PlayerState_CooldownTick, event = CollisionHandlerWithQueue,
 * +0x104 = JumpInputAndCounters, +0x1C = FallingPhysicsMain, +0x98 =
 * PlayerStateInit_BounceActive), switches to the default player sprite
 * (0x08208902) and finally EntitySetCallback with the PlayerHiddenIdle pair
 * (D_800A5F14/D_800A5F18, strong C data in src/playst.c).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_CooldownTick(void);
extern void PlayerCallback_CollisionHandlerWithQueue(void);
extern void PlayerCallback_JumpInputAndCounters(void);
extern void PlayerCallback_FallingPhysicsMain(void);
extern void PlayerStateInit_BounceActive(void);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 mode);
extern void EntitySetCallback(void *e, s32 marker, s32 fn);
extern s32  D_800A5F14 asm("D_800A5F14");
extern s32  D_800A5F18 asm("D_800A5F18");

void PlayerState_CheckpointExit(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *sctx = *(u8 **)(e + 0x34);
    u8 *gs = (u8 *)g_pGameState;

    *(s32 *)(e + 0x124) = 0x28000;
    e[0x11C] = 0;
    e[0x11E] = 0;
    e[0x11F] = 0;
    *(s32 *)(e + 0x120) = 0;
    *(s32 *)(e + 0x114) = 0;
    *(s32 *)(e + 0x118) = 0;
    e[0x13C] = 0;
    *(s32 *)(e + 0x10C) = 0;
    *(s32 *)(e + 0x110) = 0;
    sctx[0xA] = 1;
    gs[0x60] = 1;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)PlayerState_CooldownTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)PlayerCallback_CollisionHandlerWithQueue;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;          /* jump/input slot */
    *(void **)(e + 0x108) = (void *)PlayerCallback_JumpInputAndCounters;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;           /* physics slot */
    *(void **)(e + 0x20) = (void *)PlayerCallback_FallingPhysicsMain;

    SetEntitySpriteId(e, 0x08208902u, 1);

    *(u32 *)(e + 0x98) = 0xFFFF0000u;           /* bounce-init slot */
    *(void **)(e + 0x9C) = (void *)PlayerStateInit_BounceActive;

    EntitySetCallback(e, D_800A5F14, D_800A5F18);
}
