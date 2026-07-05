/* =============================================================================
 * PlayerStateCallback_2.c  --  PC-port player spawn-drop state init
 * =============================================================================
 * Functional-C reconstruction of PlayerStateCallback_2 (0x8006864C, 0x240;
 * export/SLES_010.90.c). The player's initial state when spawning mid-air:
 * caps velocity, installs the cooldown tick / collision / jump-input /
 * falling-physics FSM pairs, sets the falling sprite (0x388110), and if the
 * player was attached to an entity, dispatches event 0x1005 (detach) to it
 * with the current Y velocity.
 *
 * `_g_DefaultBGColorB` in the export is a junk Ghidra name AND a wrong
 * address: the .s loads the u16 run-button mask D_800A5970 (= 0x80, strong C
 * in src/gfx.c's migrated sdata island), tested against buttons_held to pick
 * the faster fall cap.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void SetEntitySpriteId(void *entity, u32 spriteId, s32 flag);
extern void SetAnimationLoopFrame(void *entity, u32 frameTag);
extern void SetAnimationFrameIndex(void *entity, s32 frame);

extern char PlayerState_CooldownTick[];
extern char PlayerEntityCollisionHandler[];
extern char PlayerCallback_JumpInputAndCounters[];
extern char PlayerCallback_FallingPhysicsMain[];

/* run-button mask (src/gfx.c sdata island, value 0x80) */
extern u16 D_800A5970_mask asm("D_800A5970");

typedef void (*EventFn)(void *base, s32 event, s32 arg, void *src);

void PlayerStateCallback_2(void *arg0) {
    u8 *e = (u8 *)arg0;
    u32 cap;
    u8 *interact;

    cap = 0x30000;
    if ((D_800A5970_mask & *(u16 *)(*(u8 **)(e + 0x100))) == 0) {   /* buttons_held */
        cap = 0x20000;
    }
    *(u32 *)(e + 0x124) = cap | 0x8000;    /* maxVelocity */
    e[0x11C] = 0;                          /* landingTimer */
    e[0x11E] = 0;                          /* bounceLockTimer */
    e[0x11F] = 0;                          /* jumpHoldCounter */
    *(s32 *)(e + 0x120) = 0;               /* altSpeed */
    *(s32 *)(e + 0x118) = 0;               /* cushionVelY */
    *(s32 *)(e + 0x114) = 0;               /* velocityX */
    e[0x13C] = 0;                          /* timer13C */
    if (*(s32 *)(e + 0x110) < -0x90000) {  /* velocityY floor */
        *(s32 *)(e + 0x110) = (s32)0xFFF70000;
    }

    *(u32 *)(e + 0x00) = 0xFFFF0000u;      /* tick */
    *(void **)(e + 0x04) = (void *)PlayerState_CooldownTick;
    *(u32 *)(e + 0x08) = 0xFFFF0000u;      /* event */
    *(void **)(e + 0x0C) = (void *)PlayerEntityCollisionHandler;
    *(u32 *)(e + 0x104) = 0xFFFF0000u;     /* input state */
    *(void **)(e + 0x108) = (void *)PlayerCallback_JumpInputAndCounters;
    *(u32 *)(e + 0x1C) = 0xFFFF0000u;      /* render/physics */
    *(void **)(e + 0x20) = (void *)PlayerCallback_FallingPhysicsMain;

    SetEntitySpriteId(e, 0x388110, 1);
    SetAnimationLoopFrame(e, 0x1084280);
    SetAnimationFrameIndex(e, 0);

    /* detach from the ridden/interacted entity: event 0x1005 with velY */
    interact = *(u8 **)(e + 0x12C);
    if (interact != NULL) {
        s16 vel = (s16)*(s32 *)(e + 0x110);
        s16 hi;
        if (*(s32 *)(e + 0x58) == 0x8000) {    /* half scale -> half velocity */
            vel = (s16)(vel >> 1);
        }
        hi = *(s16 *)(interact + 0x0A);        /* event marker hi */
        if (hi != 0) {
            EventFn fn;
            s32 adj = *(s16 *)(interact + 0x08);
            if (hi < 1) {
                fn = *(EventFn *)(interact + 0x0C);
            } else {
                s32 base = *(s32 *)(interact + *(s16 *)(interact + 0x0C));
                s32 slot = hi * 8 + base;
                adj += *(s16 *)((u8 *)(intptr_t)slot - 8);
                fn = *(EventFn *)((u8 *)(intptr_t)slot - 4);
            }
            fn(interact + adj, 0x1005, (s32)vel, e);
        }
        *(void **)(e + 0x12C) = NULL;
    }
}
