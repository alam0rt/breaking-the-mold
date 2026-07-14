/* =============================================================================
 * TriggerCollectible100CTickCallback.c  --  PC-port trigger-collectible tick
 * =============================================================================
 * Faithful transcription of TriggerCollectible100CTickCallback (INCLUDE_ASM in
 * src/pickups.c). Per-frame tick: on player box overlap it fires two callback-
 * table broadcasts (the idx<<3 GameState dispatch shared with YellowBird /
 * FINNCallback) -- event 0x100C to the interacting entity resolved from the
 * GameState sub-object at +0x2C, then the generic "collected" event 3 from the
 * GameState root -- and spawns the pickup particle + collect sound.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

extern void DecorEntityTickWithOffscreenCheck(void *e);
extern u8   CheckEntityBoxCollision(void *e, s32 mode);
extern void SpawnDecorParticleEffect(void *e);
extern void PlayEntityPositionSound(void *e, u32 soundId);

/* Resolve the interacting-entity handler + position offset from a callback
 * table (fields +0x8 base / +0xA count / +0xC table-or-fn) and invoke it. */
static void dispatch_gs_callback(u8 *obj, s32 event, void *self) {
    s16 idx = *(s16 *)(obj + 0xA);

    if (idx != 0) {
        void (*fn)(void *, s32, s32, void *);
        s16 base = *(s16 *)(obj + 0x8);
        s32 offset;

        if (idx > 0) {
            u8 *table = *(u8 **)(obj + *(s16 *)(obj + 0xC));
            u8 *entry = (idx << 3) + table;
            s32 argOff = *(s32 *)(entry - 8);
            fn = *(void (**)(void *, s32, s32, void *))(entry - 4);
            offset = (s16)argOff + base;
        } else {
            fn = *(void (**)(void *, s32, s32, void *))(obj + 0xC);
            offset = base;
        }
        fn(obj + offset, event, 0, self);
    }
}

void TriggerCollectible100CTickCallback(void *arg0) {
    u8 *e = (u8 *)arg0;
    u8 *gs = (u8 *)g_pGameState;

    DecorEntityTickWithOffscreenCheck(e);

    if ((CheckEntityBoxCollision(e, 2) & 0xFF) != 0) {
        dispatch_gs_callback(*(u8 **)(gs + 0x2C), 0x100C, e);
        dispatch_gs_callback(gs, 3, e);
        SpawnDecorParticleEffect(e);
        PlayEntityPositionSound(e, 0x62000441);
    }
}
