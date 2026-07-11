/* =============================================================================
 * EntityFloatingWithCollisionTick.c  --  functional-C body for enemies.c
 * EntityFloatingWithCollisionTick (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/enemies/EntityFloatingWithCollisionTick.s
 * (0x800416A0, 0x204). Per-frame tick for the checkpoint entity (installed by
 * InitCheckpointEntity; first reached when a level with an on-screen checkpoint
 * ticks, e.g. SCIE frame 1).
 *
 * Every second frame (GameState frame counter +0x10C & 1) it bobs the entity:
 * adds D_8009B684[e+0x10A] to the world-Y at +0x6A, mirrors the new Y into the
 * linked decoration entity (+0x100... the pointer at +0x104), and steps the
 * 16-entry table index. Then the normal sprite update, a player box-collision
 * check (hit -> set flag bit 1 in the linked entity's +0x16 and latch +0x109
 * "activated"), and staggered off-screen removal: on this entity's phase slot
 * (+0x108 == frame&3), if both it and its linked decoration are off-screen,
 * un-flag the decoration (unless activated) and send event 3 (entity removal)
 * to the GameState via the tagged event slot -- fsm_send_event exactly.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

extern void EntityUpdateCallback(void *e);
extern u8   CheckEntityBoxCollision(void *e, s32 mode);
extern u8   IsEntityOffScreen(void *e);
extern u8   IsEntityOffScreen_EntityLoop(void *gs, void *e);

/* Checkpoint bob-offset table: 16 s16 steps (8 down, 8 up), net zero. */
static const s16 s_floatBobTable[16] asm("D_8009B684") = {
    0, -1, -1, -1, -1, -1, -1, -1,
    0,  1,  1,  1,  1,  1,  1,  1,
};

void EntityFloatingWithCollisionTick(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    s32 removable;

    if (*(s32 *)(gs + 0x10C) & 1) {
        u8 idx = e[0x10A];
        u16 ny = (u16)(*(u16 *)(e + 0x6A) + (u16)s_floatBobTable[idx]);
        u8 *linked = *(u8 **)(e + 0x104);
        *(u16 *)(e + 0x6A) = ny;
        *(u16 *)(linked + 0x6A) = ny;
        e[0x10A] = (u8)(idx + 1);
        if (e[0x10A] >= 0x10) {
            e[0x10A] = 0;
        }
    }

    EntityUpdateCallback(e);

    if (CheckEntityBoxCollision(e, 2)) {
        u8 *dec = *(u8 **)(e + 0x100);
        dec[0x16] |= 2;
        e[0x109] = 1;
    }

    removable = 0;
    if ((*(s32 *)(gs + 0x10C) & 3) == e[0x108] && IsEntityOffScreen(e)) {
        u8 *dec = *(u8 **)(e + 0x100);
        if (dec == NULL || IsEntityOffScreen_EntityLoop(gs, dec)) {
            removable = 1;
        }
    }
    if (removable) {
        u8 *dec = *(u8 **)(e + 0x100);
        if (dec != NULL && e[0x109] == 0) {
            dec[0x16] &= 0xFE;
            *(u8 **)(e + 0x100) = NULL;
        }
        fsm_send_event(gs, 3, 0, e);
    }
}
