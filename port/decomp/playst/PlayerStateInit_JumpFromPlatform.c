/* =============================================================================
 * PlayerStateInit_JumpFromPlatform.c  --  PC-port conversion (0x80069200)
 * =============================================================================
 * State installer fired when the player jumps off a ridden platform:
 *   1. If a platform is latched at +0x12C, send it event 0x1005 through its
 *      FSM marker slot at +0x8/+0xA/+0xC (same {argOff, mode, fn-or-table}
 *      convention as StepAnimationSequence), passing half the player's
 *      16.16 y velocity (+0x110) when the X scale (+0x58) is 0x8000
 *      (mini-Klaymen), then clear the latch.
 *   2. Set the GameState "player airborne" byte (+0x60), pick the jump sprite
 *      by cycling +0x164 mod 3 (0x1CF99931 / 0x1DF99931 / 0x1EF99931), seed
 *      the jump physics block (+0x110..+0x120, y vel 0xFFFA0000, gravity
 *      -0x1200 at +0x118, jump-frames byte pair +0x11C/+0x11D chosen by the
 *      was-riding flag +0x13C), and install the airborne FSM slots:
 *      tick +0x0 PlayerState_CooldownTick, event +0x8
 *      PlayerCallback_CollisionHandlerWithQueue, +0x104
 *      PlayerCallback_JumpTickHandler, +0x1C
 *      PlayerCallback_FallingPhysicsMain, anim-done +0x98
 *      PlayerStateInit_BounceJumpAnimation; then SetEntitySpriteId and the
 *      hidden-idle callback pair D_800A5F14/D_800A5F18.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_CooldownTick();
extern void PlayerCallback_CollisionHandlerWithQueue();
extern void PlayerCallback_JumpTickHandler();
extern void PlayerCallback_FallingPhysicsMain();
extern void PlayerStateInit_BounceJumpAnimation();
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flag);
extern void EntitySetCallback(void *e, u32 marker, u32 callback);

extern u32 PlayerHiddenIdleMarker asm("D_800A5F14");
extern u32 PlayerHiddenIdleFn     asm("D_800A5F18");

#define INSTALL_SLOT(base, off, fn)                                   \
    do {                                                              \
        *(u32 *)((base) + (off)) = 0xFFFF0000u;                       \
        *(void **)((base) + (off) + 4) = (void *)(fn);                \
    } while (0)

void PlayerStateInit_JumpFromPlatform(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *plat = *(u8 **)(e + 0x12C);
    u32 spriteId;
    u8 variant;

    if (plat != NULL) {
        s32 vel = *(s32 *)(e + 0x110);
        s16 mode;

        if (*(s32 *)(e + 0x58) == 0x8000) {
            vel = ((s32)(vel << 16)) >> 17;   /* mini-Klaymen: half, s16 */
        }
        vel = (s16)vel;

        mode = *(s16 *)(plat + 0xA);
        if (mode != 0) {
            void (*fn)(void *, s32, s32, void *);
            s32 off;

            if (mode > 0) {
                u8 *tbl = *(u8 **)(plat + *(s16 *)(plat + 0xC));
                s32 *pair = (s32 *)(tbl + mode * 8);
                fn = (void (*)(void *, s32, s32, void *))pair[-1];
                off = (s16)pair[-2] + *(s16 *)(plat + 0x8);
            } else {
                fn = (void (*)(void *, s32, s32, void *))*(s32 *)(plat + 0xC);
                off = *(s16 *)(plat + 0x8);
            }
            fn(plat + off, 0x1005, vel, arg0);
        }
        *(u8 **)(e + 0x12C) = NULL;
    }

    ((u8 *)g_pGameState)[0x60] = 1;

    variant = (u8)(e[0x164] % 3);
    e[0x164] = variant;
    if (variant == 0) {
        spriteId = 0x1CF99931;
    } else if (variant == 1) {
        spriteId = 0x1DF99931;
    } else {
        spriteId = 0x1EF99931;
    }

    if (e[0x13C] != 0) {
        e[0x11C] = (u8)((e[0x164] == 2) ? 0xA : 5);
        e[0x11D] = 0;
    } else {
        e[0x11C] = 0;
        e[0x11D] = 5;
    }

    *(s32 *)(e + 0x118) = -0x1200;
    e[0x11E] = 0;
    e[0x11F] = 0;
    *(s32 *)(e + 0x120) = 0;
    *(s32 *)(e + 0x114) = 0;
    *(s32 *)(e + 0x110) = (s32)0xFFFA0000;
    e[0x13C] = 0;

    INSTALL_SLOT(e, 0x0, PlayerState_CooldownTick);
    INSTALL_SLOT(e, 0x8, PlayerCallback_CollisionHandlerWithQueue);
    INSTALL_SLOT(e, 0x104, PlayerCallback_JumpTickHandler);
    INSTALL_SLOT(e, 0x1C, PlayerCallback_FallingPhysicsMain);

    SetEntitySpriteId(arg0, spriteId, 1);

    INSTALL_SLOT(e, 0x98, PlayerStateInit_BounceJumpAnimation);

    EntitySetCallback(arg0, PlayerHiddenIdleMarker, PlayerHiddenIdleFn);
}
