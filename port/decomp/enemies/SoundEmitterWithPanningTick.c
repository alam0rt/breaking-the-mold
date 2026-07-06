/* =============================================================================
 * SoundEmitterWithPanningTick.c  --  PC-port sound emitter panning tick
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/enemies/
 * SoundEmitterWithPanningTick.s (0x8004598C, 0x118). Per-frame tick for level
 * sound-emitter entities: re-pans the looping voice (+0x20) by the emitter's
 * screen X (+0x24 minus camera X @gs+0x44), and when the emitter's spawn
 * record (+0x1C) scrolls offscreen, clears the spawn-alive bit and notifies
 * the game state (event 3, arg 1 -> the removal funnel).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "../decor/fsm_event.h"

extern void SetVoicePanning(s32 voice, s32 pan);
extern u32  IsEntityOffScreen_EntityLoop(u8 *gs, s16 *spawnRec);

void SoundEmitterWithPanningTick(void *entity) {
    u8 *e = (u8 *)entity;
    u8 *gs = (u8 *)g_pGameState;
    u8 *spawnRec;

    SetVoicePanning(*(s32 *)(e + 0x20),
                    *(s16 *)(e + 0x24) - *(s16 *)(gs + 0x44));

    spawnRec = *(u8 **)(e + 0x1C);
    if (spawnRec != NULL &&
        (IsEntityOffScreen_EntityLoop(gs, (s16 *)spawnRec) & 0xFF) != 0) {
        spawnRec[0x16] &= 0xFE;            /* clear spawn-alive bit */
        *(u8 **)(e + 0x1C) = NULL;
        fsm_send_event(gs, 3, 1, e);
    }
}
