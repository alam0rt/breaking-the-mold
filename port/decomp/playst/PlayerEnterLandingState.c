/* =============================================================================
 * PlayerEnterLandingState.c  --  functional-C body for playst.c
 * PlayerEnterLandingState (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/playst/PlayerEnterLandingState.s
 * (0x80067A34, 0x15C). Pure slot-install state: tick = PlayerTickCallback,
 * event = PlayerCallback_EventHandlerWithQueue, +0x104 = IdleInputHandler,
 * physics = HandleMovementAndCollision (or WalkingOnPlatform when riding,
 * +0x12C set), landing sprite 0x282B8491, and the +0x98 idle-init slot.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerTickCallback(void);
extern void PlayerCallback_EventHandlerWithQueue(void);
extern void PlayerCallback_IdleInputHandler(void);
extern void PlayerCallback_HandleMovementAndCollision(void *e);
extern void PlayerCallback_WalkingOnPlatform(void);
extern void PlayerStateInit_Idle(void);
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 mode);

void PlayerEnterLandingState(void *arg0) {
    u8 *e = (u8 *)arg0;
    void *physFn = (*(void **)(e + 0x12C) != NULL)
                       ? (void *)PlayerCallback_WalkingOnPlatform
                       : (void *)PlayerCallback_HandleMovementAndCollision;

    *(u32 *)(e + 0x00) = 0xFFFF0000u;           /* tick slot */
    *(void **)(e + 0x04) = (void *)PlayerTickCallback;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;           /* event slot */
    *(void **)(e + 0x0C) = (void *)PlayerCallback_EventHandlerWithQueue;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;          /* input slot */
    *(void **)(e + 0x108) = (void *)PlayerCallback_IdleInputHandler;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;           /* physics slot */
    *(void **)(e + 0x20) = physFn;

    SetEntitySpriteId(e, 0x282B8491u, 1);

    *(u32 *)(e + 0x98) = 0xFFFF0000u;           /* idle-init slot */
    *(void **)(e + 0x9C) = (void *)PlayerStateInit_Idle;
}
