/* =============================================================================
 * PlayerCallback_SetIdleStateCallbacks.c  --  PC-port conversion (0x80066E38)
 * =============================================================================
 * Landing -> grounded idle transition: installs the idle FSM slots
 * (tick +0x0 PlayerState_IdleRandom, event +0x8 PlayerEntityEventHandler,
 * input +0x104 PlayerCallback_IdleInputHandler, movement +0x1C
 * PlayerCallback_RidingPlatformPhysics when a platform is latched at +0x12C,
 * else PlayerCallback_HorizontalWallCollision) and the idle sprite
 * 0x48204012.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_IdleRandom();
extern void PlayerEntityEventHandler();
extern void PlayerCallback_IdleInputHandler();
extern void PlayerCallback_HorizontalWallCollision();
extern void PlayerCallback_RidingPlatformPhysics();
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flag);

#define INSTALL_SLOT(base, off, fn)                                   \
    do {                                                              \
        *(u32 *)((base) + (off)) = 0xFFFF0000u;                       \
        *(void **)((base) + (off) + 4) = (void *)(fn);                \
    } while (0)

void PlayerCallback_SetIdleStateCallbacks(void *arg0) {
    u8 *e = (u8 *)arg0;
    void *moveFn;

    if (*(s32 *)(e + 0x12C) != 0) {
        moveFn = (void *)PlayerCallback_RidingPlatformPhysics;
    } else {
        moveFn = (void *)PlayerCallback_HorizontalWallCollision;
    }

    INSTALL_SLOT(e, 0x0, PlayerState_IdleRandom);
    INSTALL_SLOT(e, 0x8, PlayerEntityEventHandler);
    INSTALL_SLOT(e, 0x104, PlayerCallback_IdleInputHandler);
    INSTALL_SLOT(e, 0x1C, moveFn);

    SetEntitySpriteId(arg0, 0x48204012, 1);
}
