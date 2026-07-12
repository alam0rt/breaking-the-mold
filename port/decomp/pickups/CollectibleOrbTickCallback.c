/* =============================================================================
 * CollectibleOrbTickCallback.c  --  functional-C body for pickups.c
 * CollectibleOrbTickCallback (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/pickups/CollectibleOrbTickCallback.s
 * (0x8002D768, 0x144). Per-frame tick for the orb pickups scattered around
 * levels. Orb tail fields:
 *   +0x100 sprite spawn record   +0x11C offscreen-check phase (frame & 3)
 *   +0x11D "always collectible" flag (skips the player box test)
 *   +0x120 prim object ptr (enabled byte at +0xA)   +0x124 "dead" latch
 *
 * Staggered offscreen path (this orb's phase slot only): release the spawn
 * record and disable the prim -- no GameState removal event; the orb object
 * is pooled, +0x124 marks it dead.
 *
 * Pickup path (player box overlap, or +0x11D forces it): the player-state
 * byte +0x17 bit 0 gates a double meaning -- first orb sets the flag,
 * subsequent orbs award 5 orbs via AddPlayerOrbs (the 100-orb -> 1UP
 * counter); then disable the prim and spawn the decor particle burst.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void EntityUpdateCallback(void *e);
extern unsigned int IsEntityOffScreen(void *e);
extern unsigned int IsEntityOffScreen_EntityLoop(void *gs, void *rec);
extern u8   CheckEntityBoxCollision(void *e, s32 mode);
extern void AddPlayerOrbs(void *ps, u8 count);
extern void SpawnDecorParticleEffect(void *e);

extern u8 *PLAYER_STATE_PTR asm("D_800A597C");

void CollectibleOrbTickCallback(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;
    int gone = 0;

    EntityUpdateCallback(e);

    if ((*(s32 *)(gs + 0x10C) & 3) == e[0x11C] &&
        (IsEntityOffScreen(e) & 0xFF) != 0) {
        u8 *rec = *(u8 **)(e + 0x100);
        if (rec == NULL ||
            (IsEntityOffScreen_EntityLoop(gs, rec) & 0xFF) != 0) {
            gone = 1;
        }
    }
    if (gone) {
        u8 *rec = *(u8 **)(e + 0x100);
        if (rec != NULL) {
            rec[0x16] &= 0xFE;
            *(u32 *)(e + 0x100) = 0;
        }
        (*(u8 **)(e + 0x120))[0xA] = 0;
        e[0x124] = 1;
    }

    if (e[0x11D] != 0 || (CheckEntityBoxCollision(e, 2) & 0xFF) != 0) {
        u8 *ps = PLAYER_STATE_PTR;
        if (ps[0x17] & 1) {
            AddPlayerOrbs(ps, 5);
        } else {
            ps[0x17] |= 1;
        }
        (*(u8 **)(e + 0x120))[0xA] = 0;
        e[0x124] = 1;
        SpawnDecorParticleEffect(e);
    }
}
