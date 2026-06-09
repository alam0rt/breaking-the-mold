#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include "common.h"

/* =============================================================================
 * InputState (20 bytes)
 *
 * Tracks controller buttons and demo playback/recording state.
 * Pointed to by GameState+0x140 (input_state_ptr) and Entity+0x40 in
 * the player entity.
 * Also referenced via g_pPlayer1Input (0x800A5764) and g_pCurrentInputState.
 *
 * Initialized by: input init code
 * ============================================================================= */
typedef struct {
    /* 0x00 */ u16  buttons_held;      /* Bitmask of currently held buttons (PSX pad format) */
    /* 0x02 */ u16  buttons_pressed;   /* Bitmask of newly pressed buttons this frame */
    /* 0x04 */ u8   record_active;     /* Non-zero = RLE recording in progress */
    /* 0x05 */ u8   playback_active;   /* Non-zero = demo playback in progress */
    /* 0x06 */ u16  pad06;
    /* 0x08 */ void *pFrameCount;      /* Pointer to replay frame count (u16) */
    /* 0x0C */ void *pReplayBuffer;    /* Pointer to RLE replay entry array ([u16 buttons][u16 duration]) */
    /* 0x10 */ u16  playback_index;    /* Current index into replay buffer */
    /* 0x12 */ u16  playback_timer;    /* Ticks remaining on current replay entry */
} InputState;  /* Size: 0x14 (20 bytes) */

/* PSX controller button bitmasks (LIBPAD) */
#define PAD_SELECT   0x0001
#define PAD_L3       0x0002
#define PAD_R3       0x0004
#define PAD_START    0x0008
#define PAD_UP       0x0010
#define PAD_RIGHT    0x0020
#define PAD_DOWN     0x0040
#define PAD_LEFT     0x0080
#define PAD_L2       0x0100
#define PAD_R2       0x0200
#define PAD_L1       0x0400
#define PAD_R1       0x0800
#define PAD_TRIANGLE 0x1000
#define PAD_CIRCLE   0x2000
#define PAD_CROSS    0x4000
#define PAD_SQUARE   0x8000

#endif /* INPUT_STATE_H */
