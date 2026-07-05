/* =============================================================================
 * UpdateInputState.c  --  PC-port controller-state / demo replay updater
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c UpdateInputState (0x80025A44,
 * INCLUDE_ASM in src/blb.c). Called once per pad per frame from the frame loop.
 * Three modes, driven by the InputState flags:
 *   - playback_active: feed buttons from the RLE replay buffer (demo attract).
 *   - live (default): buttons_held = raw; buttons_pressed = raw & ~prevHeld.
 *   - record_active: RLE-append the live buttons into the replay buffer.
 * On the title screen both flags are 0, so only the live edge-detect path runs.
 *
 * Replay buffer entry = 4 bytes: [u16 buttons][u16 duration]; indexed by
 * playback_index. pFrameCount points at a u16 entry count.
 *
 * Uses the named InputState struct from include/Game/input_state.h (via
 * globals.h): buttons_held@0x00, buttons_pressed@0x02, record_active@0x04,
 * playback_active@0x05, pFrameCount@0x08, pReplayBuffer@0x0C,
 * playback_index@0x10, playback_timer@0x12.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include "globals.h"

void UpdateInputState(InputState *inputState, unsigned int rawButtons) {
    u16 raw = (u16)rawButtons;
    u16 *replay = (u16 *)inputState->pReplayBuffer;
    u16 *frameCount = (u16 *)inputState->pFrameCount;

    if (inputState->playback_active != 0) {
        if (((unsigned int)inputState->playback_index < (unsigned int)*frameCount) &&
            ((rawButtons & 0xffff) == 0)) {
            u16 entryButtons = replay[(unsigned int)inputState->playback_index * 2];
            u16 timer;
            inputState->buttons_pressed = entryButtons & ~inputState->buttons_held;
            timer = inputState->playback_timer - 1;
            inputState->playback_timer = timer;
            inputState->buttons_held = entryButtons;
            if (timer == 0) {
                u16 idx = inputState->playback_index + 1;
                inputState->playback_index = idx;
                inputState->playback_timer = replay[(unsigned int)idx * 2 + 1];
            }
            goto record_check;
        }
        if (inputState->playback_active != 0) {
            inputState->playback_active = 0;
            inputState->buttons_pressed = 0;
            inputState->buttons_held = 0;
            goto record_check;
        }
    }
    {
        u16 prevHeld = inputState->buttons_held;
        inputState->buttons_held = raw;
        inputState->buttons_pressed = raw & ~prevHeld;
    }

record_check:
    if ((inputState->record_active == 0) || (0x3ff < *frameCount)) {
        inputState->record_active = 0;
    } else {
        unsigned int idx;
        if (*frameCount == 0) {
            idx = (unsigned int)inputState->playback_index;
        } else {
            u16 *entry = &replay[(unsigned int)inputState->playback_index * 2];
            idx = inputState->playback_index + 1;
            if ((unsigned int)entry[0] == (rawButtons & 0xffff)) {
                entry[1] = entry[1] + 1;
                return;
            }
            inputState->playback_index = (u16)idx;
            idx = idx & 0xffff;
        }
        replay[idx * 2 + 1] = 1;
        replay[(unsigned int)inputState->playback_index * 2] = raw;
        *frameCount = *frameCount + 1;
    }
}
