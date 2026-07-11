/* =============================================================================
 * EnableDemoPlaybackMode.c -- functional-C bodies for the blb.c demo-playback
 * setup pair (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/blb/{InitEntityDataPointers,
 * EnableDemoPlaybackMode}.s (0x80025B7C / 0x80025BC0). Together with the
 * already-converted UpdateInputState these complete the attract-demo input
 * playback machinery: SetupAndStartLevel's demo branch (gs->demo_return_flag)
 * calls srand(1), points the InputState at the level's asset-700 replay data
 * (GetDemoDataPtr), and flips playback on; UpdateInputState then feeds
 * buttons_held from the RLE stream each frame until a real button cancels it.
 *
 * Replay data layout (asset 700 payload, after its 0x10-byte header):
 *   +0x00 u16 entryCount            (read via InputState.pFrameCount)
 *   +0x02 u16 (unused padding)
 *   +0x04 RLE entries: [u16 buttons][u16 durationFrames] * entryCount
 * ========================================================================== */
#include "common.h"
#include "globals.h"

/* Aim the InputState's replay pointers at a demo/replay data base.
 * (Despite the historical name this is an InputState method, not an entity
 * one -- the same asm blob also serves InitHUDEntity in src/blb.c.) */
void InitEntityDataPointers(InputState *in, void *dataBase) {
    in->pFrameCount = dataBase;
    in->pReplayBuffer = (u8 *)dataBase + 4;
}

/* Toggle demo playback. Enabling resets the stream cursor and primes the
 * first entry's duration; re-enabling while already active is a no-op. */
void EnableDemoPlaybackMode(InputState *in, u8 enable) {
    if (in->playback_active && enable) {
        return;
    }
    in->playback_active = enable;
    if (!enable) {
        return;
    }
    in->record_active = 0;
    in->playback_index = 0;
    in->playback_timer = *(u16 *)((u8 *)in->pReplayBuffer + 2);
}
