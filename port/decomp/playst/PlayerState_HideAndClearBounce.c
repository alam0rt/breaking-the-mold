/* =============================================================================
 * PlayerState_HideAndClearBounce.c  --  PC-port hidden/parked player state
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/playst/
 * PlayerState_HideAndClearBounce.s (0x8006A310, 0xE8; equivalent C parked in
 * src/playst.c). The player's spawn state on levels that start it hidden (e.g.
 * level 2, checkpoint entry): sets the game-state bounce-active flag, installs
 * EntityUpdateCallback as the tick slot and PlayerEvent_ZoneTriggerHandler as
 * the event slot, clears the input (+0x104) and render (+0x1C) slots, freezes
 * the current anim frame, hides the render flags, and queues the hidden-idle
 * next state (PlayerHiddenIdle pair, D_800A5F14/D_800A5F18) via
 * EntitySetCallback -- finally disabling the sprite context (+0xA).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void EntityUpdateCallback(void *e);
extern void PlayerEvent_ZoneTriggerHandler(void);
extern void SetAnimationTargetFrameIndex(void *e, s16 frame);
extern void EntitySetRenderFlags(void *e, s32 flags);
extern void EntitySetCallback(void *entity, u32 marker, u32 callback);

extern u32   PlayerHiddenIdleMarker asm("D_800A5F14");
extern void *PlayerHiddenIdleFn     asm("D_800A5F18");

void PlayerState_HideAndClearBounce(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    u8 *ctx;

    gs[0x60] = 1;                          /* bounce_active_flag */

    *(u32 *)(e + 0x00) = 0xFFFF0000u;      /* tick slot */
    *(void **)(e + 0x04) = (void *)EntityUpdateCallback;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;      /* event slot */
    *(void **)(e + 0x0C) = (void *)PlayerEvent_ZoneTriggerHandler;
    *(u32 *)(e + 0x104) = 0;               /* input slot cleared */
    *(void **)(e + 0x108) = NULL;
    *(u32 *)(e + 0x1C) = 0;                /* render slot cleared */
    *(void **)(e + 0x20) = NULL;

    SetAnimationTargetFrameIndex(e, *(s16 *)(e + 0xDA));
    EntitySetRenderFlags(e, 0);
    ctx = *(u8 **)(e + 0x34);
    EntitySetCallback(e, PlayerHiddenIdleMarker,
                      (u32)(uintptr_t)PlayerHiddenIdleFn);
    ctx[0xA] = 0;                          /* spriteContext->enabled = 0 */
}
