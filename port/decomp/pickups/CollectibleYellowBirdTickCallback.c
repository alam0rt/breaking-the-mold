/* =============================================================================
 * CollectibleYellowBirdTickCallback.c  --  PC-port yellow-bird collectible tick
 * =============================================================================
 * Faithful transcription of CollectibleYellowBirdTickCallback (INCLUDE_ASM in
 * src/pickups.c). Per-frame tick: runs the shared decor offscreen tick, and if
 * the entity is force-collectible (+0x11D) or overlaps the player box, sets
 * bit 1 of the player-state flags (+0x17), broadcasts a "collected" event (3)
 * to the currently-interacting entity resolved from the GameState callback
 * table (fields +0x8/+0xA/+0xC, the same idx<<3 table dispatch as
 * FINNCallback_DispatchToEntityHandler), then spawns the pickup particle burst
 * and plays the collect sound.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

extern void DecorEntityTickWithOffscreenCheck(void *e);
extern u8   CheckEntityBoxCollision(void *e, s32 mode);
extern void SpawnDecorParticleEffect(void *e);
extern void PlayEntityPositionSound(void *e, u32 soundId);

extern u8 *PLAYER_STATE_PTR asm("D_800A597C");

void CollectibleYellowBirdTickCallback(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;

    DecorEntityTickWithOffscreenCheck(e);

    if (e[0x11D] != 0 || (CheckEntityBoxCollision(e, 2) & 0xFF) != 0) {
        s16 idx = *(s16 *)(gs + 0xA);

        PLAYER_STATE_PTR[0x17] |= 2;

        if (idx != 0) {
            void (*fn)(void *, s32, s32, void *);
            s32 argOff = 0;
            s16 base = *(s16 *)(gs + 0x8);
            s32 offset;

            if (idx > 0) {
                u8 *table = *(u8 **)(gs + *(s16 *)(gs + 0xC));
                u8 *entry = (idx << 3) + table;
                argOff = *(s32 *)(entry - 8);
                fn = *(void (**)(void *, s32, s32, void *))(entry - 4);
                offset = (s16)argOff + base;
            } else {
                fn = *(void (**)(void *, s32, s32, void *))(gs + 0xC);
                offset = base;
            }
            fn(gs + offset, 3, 0, e);
        }

        SpawnDecorParticleEffect(e);
        PlayEntityPositionSound(e, 0x6082C120);
    }
}
