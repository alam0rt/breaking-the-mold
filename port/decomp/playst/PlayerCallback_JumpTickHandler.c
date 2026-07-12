/* =============================================================================
 * PlayerCallback_JumpTickHandler.c  --  PC-port conversion (0x800605D8)
 * =============================================================================
 * Airborne input/physics tick for the platform-jump state. Reads the collision
 * surface probe (+0x100: flags word0 @ +0x0, flags word1 @ +0x2):
 *   - any of word1 & 0xF -> play the landing sound 0x64221E61.
 *   - while the jump button context byte (D_800A597C->0x13) is held and
 *     word1 & D_800A596E: queue the double-jump (+0x134, if the +0x135
 *     inhibit is set) or EntitySetState(D_800A5E68/D_800A5E6C).
 *   - else word1 & D_800A596C latches the 10-frame coyote counter (+0x13C).
 *   - word0 & D_800A596C (ceiling/ground?): with the glide object (+0x16C)
 *     live, rising (+0x110 > 0) and not gliding (+0x170), hand off to
 *     EntitySetState(D_800A5E70/D_800A5E74); otherwise while gliding hand
 *     off to D_800A5E78/D_800A5E7C.
 *   - horizontal: launch-boost frames (+0x11F) force vx (+0x114) = +0x120;
 *     else steer from surface word0 bits 0x2000/0x8000 (facing byte +0x74,
 *     vx = +/- +0x124).
 *   - vertical: hold frames (+0x11E) pin gravity (+0x118) to -0x1200 and
 *     burn +0x11C; then +0x11C frames keep it pinned while word0 &
 *     D_800A596C; then +0x11D frames re-arm +0x11C on a word1 & D_800A596C
 *     hit ((+0x164)==2 gets +4 extra frames) with gravity re-pinned.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void PlayEntityPositionSound(void *e, u32 soundId);
extern void EntitySetState(void *e, u32 marker, u32 fn);

extern u8 *D_800A597C;
extern u16 D_800A596C;
extern u16 D_800A596E;

extern u32 D_800A5E68; extern u32 D_800A5E6C;
extern u32 D_800A5E70; extern u32 D_800A5E74;
extern u32 D_800A5E78; extern u32 D_800A5E7C;

void PlayerCallback_JumpTickHandler(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *surf = *(u8 **)(e + 0x100);
    u16 w1 = *(u16 *)(surf + 0x2);

    if (w1 & (0x4 | 0x1 | 0x8 | 0x2)) {
        PlayEntityPositionSound(arg0, 0x64221E61);
    }

    if (D_800A597C[0x13] != 0 &&
        (*(u16 *)(*(u8 **)(e + 0x100) + 0x2) & D_800A596E)) {
        if (e[0x135] != 0) {
            e[0x134] = 1;
        } else {
            EntitySetState(arg0, D_800A5E68, D_800A5E6C);
        }
    } else {
        if (*(u16 *)(*(u8 **)(e + 0x100) + 0x2) & D_800A596C) {
            e[0x13C] = 0xA;
        }
    }

    if (*(u16 *)(*(u8 **)(e + 0x100) + 0x0) & D_800A596C) {
        if (*(s32 *)(e + 0x16C) != 0 && *(s32 *)(e + 0x110) > 0 &&
            e[0x170] == 0) {
            EntitySetState(arg0, D_800A5E70, D_800A5E74);
        }
    } else {
        if (e[0x170] != 0) {
            EntitySetState(arg0, D_800A5E78, D_800A5E7C);
        }
    }

    if (e[0x11F] != 0) {
        e[0x11F] = (u8)(e[0x11F] - 1);
        *(s32 *)(e + 0x114) = *(s32 *)(e + 0x120);
    } else {
        u16 w0 = *(u16 *)(*(u8 **)(e + 0x100) + 0x0);
        if (w0 & 0x2000) {
            e[0x74] = 0;
            *(s32 *)(e + 0x114) = *(s32 *)(e + 0x124);
        } else if (w0 & 0x8000) {
            e[0x74] = 1;
            *(s32 *)(e + 0x114) = -*(s32 *)(e + 0x124);
        }
    }

    if (e[0x11E] != 0) {
        e[0x11E] = (u8)(e[0x11E] - 1);
        *(s32 *)(e + 0x118) = -0x1200;
        e[0x11C] = (u8)(e[0x11C] - 1);
    } else if (e[0x11C] != 0) {
        e[0x11C] = (u8)(e[0x11C] - 1);
        if (*(u16 *)(*(u8 **)(e + 0x100) + 0x0) & D_800A596C) {
            *(s32 *)(e + 0x118) = -0x1200;
        }
    } else if (e[0x11D] != 0) {
        u8 d = e[0x11D];
        e[0x11D] = (u8)(d - 1);
        if (*(u16 *)(*(u8 **)(e + 0x100) + 0x2) & D_800A596C) {
            if (e[0x164] == 2) {
                e[0x11C] = (u8)(d + 4);
            } else {
                e[0x11C] = (u8)(d - 1);
            }
            *(s32 *)(e + 0x118) = -0x1200;
            e[0x11D] = 0;
        }
    }
}
