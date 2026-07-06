/* =============================================================================
 * PlayerStateInit_ShrinkAndFall.c  --  PC-port player shrink-and-fall init
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/playst/
 * PlayerStateInit_ShrinkAndFall.s (0x8006D548, 0x180). Installs the shrunk
 * player's falling state: clears the FINN subentity state flag (+0x16C),
 * zeroes the +0x170 latch, arms the game-state shake/offset fields
 * (+0x60=1, +0x64=0x28, +0x66=-0x30), installs the cooldown tick /
 * collision-trail / jump-input / falling-physics FSM slots, sets the shrunk
 * falling sprite (0x987101B9), arms the +0x98 landing slot with
 * PlayerStateInit_BounceActive, and queues the restore-normal-hitbox
 * next-state pair (D_800A5F5C/D_800A5F60, strong data in src/playst.c) via
 * EntitySetCallback.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void ClearEntityStateFlag(void *flags);
extern void SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);
extern void EntitySetCallback(void *entity, u32 marker, u32 callback);

extern char PlayerState_CooldownTick[];
extern char PlayerCallback_CollisionTrailEntityUpdate[];
extern char PlayerCallback_JumpInputAndCounters[];
extern char PlayerCallback_FallingPhysicsMain[];
extern char PlayerStateInit_BounceActive[];

extern u32 PlayerRestoreShrinkNextMarker asm("D_800A5F5C");
extern void *PlayerRestoreShrinkNextFn asm("D_800A5F60");

void PlayerStateInit_ShrinkAndFall(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs;

    ClearEntityStateFlag(*(void **)(e + 0x16C));
    e[0x170] = 0;

    gs = (u8 *)g_pGameState;
    gs[0x60] = 1;
    *(s16 *)(gs + 0x64) = 0x28;
    *(s16 *)(gs + 0x66) = -0x30;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;      /* tick */
    *(void **)(e + 0x04) = (void *)PlayerState_CooldownTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;      /* event */
    *(void **)(e + 0x0C) = (void *)PlayerCallback_CollisionTrailEntityUpdate;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;     /* input */
    *(void **)(e + 0x108) = (void *)PlayerCallback_JumpInputAndCounters;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;      /* render/physics */
    *(void **)(e + 0x20) = (void *)PlayerCallback_FallingPhysicsMain;

    SetEntitySpriteId(e, 0x987101B9u, 1);

    *(u32 *)(e + 0x98) = 0xFFFF0000u;      /* landing slot */
    *(void **)(e + 0x9C) = (void *)PlayerStateInit_BounceActive;

    EntitySetCallback(e, PlayerRestoreShrinkNextMarker,
                      (u32)(uintptr_t)PlayerRestoreShrinkNextFn);
}
