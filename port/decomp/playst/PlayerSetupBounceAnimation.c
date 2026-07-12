/* =============================================================================
 * PlayerSetupBounceAnimation.c  --  PC-port conversion (0x8006AF70)
 * =============================================================================
 * Bounce launch (spring/mushroom): seeds the bounce physics (+0x128 timer
 * 0x78, +0x124 launch speed 0x28000, vx/gravity cleared, +0x10C and the
 * coyote counter +0x13C cleared), notifies a latched ride platform (+0x12C)
 * with event 0x1005 through its FSM marker slot (velocity halved for
 * mini-Klaymen scale 0x8000) and clears the latch, then branches on the
 * bounce-request bit (D_800A597C->0x17 & 1):
 *   - held: consume the bit and install the active-bounce state (slots +0x0
 *     PlayerState_CooldownTick, +0x8 PlayerCallback_CollisionHandlerWithQueue,
 *     +0x104 PlayerCallback_JumpInputAndCounters, +0x1C
 *     PlayerCallback_FallingPhysicsMain; sprite 0x393C80C2; anim-done +0x98
 *     PlayerStateInit_BounceActive; callback pair D_800A5F14/D_800A5F18).
 *   - not held: set the +0x178 latch and install the passive variant (+0x8
 *     PlayerEvent_ZoneTriggerHandlerV2, +0x104 cleared, callback pair
 *     D_800A5F24/D_800A5F28, anim-done PlayerState_LevelExitComplete).
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayerState_CooldownTick();
extern void PlayerCallback_CollisionHandlerWithQueue();
extern void PlayerCallback_JumpInputAndCounters();
extern void PlayerCallback_FallingPhysicsMain();
extern void PlayerStateInit_BounceActive();
extern void PlayerEvent_ZoneTriggerHandlerV2();
extern void PlayerState_LevelExitComplete();
extern void SetEntitySpriteId(void *e, u32 spriteId, s32 flag);
extern void EntitySetCallback(void *e, u32 marker, u32 callback);

extern u8 *D_800A597C;
extern u32 PlayerHiddenIdleMarker asm("D_800A5F14");
extern u32 PlayerHiddenIdleFn     asm("D_800A5F18");
extern u32 D_800A5F24;
extern u32 D_800A5F28;

#define INSTALL_SLOT(base, off, fn)                                   \
    do {                                                              \
        *(u32 *)((base) + (off)) = 0xFFFF0000u;                       \
        *(void **)((base) + (off) + 4) = (void *)(fn);                \
    } while (0)

void PlayerSetupBounceAnimation(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *plat;

    e[0x128] = 0x78;
    *(s32 *)(e + 0x10C) = 0;
    ((u8 *)g_pGameState)[0x60] = 1;
    *(s32 *)(e + 0x124) = 0x28000;
    *(s32 *)(e + 0x114) = 0;
    *(s32 *)(e + 0x118) = 0;
    e[0x13C] = 0;

    plat = *(u8 **)(e + 0x12C);
    if (plat != NULL) {
        s32 vel = *(s32 *)(e + 0x110);
        s16 mode;

        if (*(s32 *)(e + 0x58) == 0x8000) {
            vel = ((s32)(vel << 16)) >> 17;
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

    if (D_800A597C[0x17] & 1) {
        D_800A597C[0x17] &= 0xFE;

        INSTALL_SLOT(e, 0x0, PlayerState_CooldownTick);
        INSTALL_SLOT(e, 0x8, PlayerCallback_CollisionHandlerWithQueue);
        INSTALL_SLOT(e, 0x104, PlayerCallback_JumpInputAndCounters);
        INSTALL_SLOT(e, 0x1C, PlayerCallback_FallingPhysicsMain);

        SetEntitySpriteId(arg0, 0x393C80C2, 1);

        INSTALL_SLOT(e, 0x98, PlayerStateInit_BounceActive);
        EntitySetCallback(arg0, PlayerHiddenIdleMarker, PlayerHiddenIdleFn);
    } else {
        e[0x178] = 1;

        INSTALL_SLOT(e, 0x0, PlayerState_CooldownTick);
        INSTALL_SLOT(e, 0x8, PlayerEvent_ZoneTriggerHandlerV2);
        *(u32 *)(e + 0x104) = 0;
        *(void **)(e + 0x108) = NULL;
        INSTALL_SLOT(e, 0x1C, PlayerCallback_FallingPhysicsMain);

        SetEntitySpriteId(arg0, 0x393C80C2, 1);
        EntitySetCallback(arg0, D_800A5F24, D_800A5F28);

        INSTALL_SLOT(e, 0x98, PlayerState_LevelExitComplete);
    }
}
