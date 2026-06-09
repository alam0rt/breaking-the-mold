#ifndef SOUND_H
#define SOUND_H

#include "common.h"

/* =============================================================================
 * SoundEntry (12 bytes)
 *
 * Per-sound descriptor loaded from Asset 601/700 (audio containers).
 * Maps sound IDs to SPU addresses and playback parameters.
 *
 * Asset 601 = level audio samples (LevelDataContext+0x48 / ctx[18])
 * Asset 700 = SPU DMA audio data   (LevelDataContext+0x54 / ctx[21])
 * ============================================================================= */
typedef struct {
    /* 0x00 */ u32  sound_id;      /* Sound identifier (matched against play requests) */
    /* 0x04 */ u32  spu_address;   /* SPU RAM address where sample is uploaded */
    /* 0x08 */ u16  base_volume;   /* Default playback volume (0-0x3FFF) */
    /* 0x0A */ u16  flags;         /* Playback flags (looping, pitch, etc.) */
} SoundEntry;  /* Size: 0x0C (12 bytes) */

#endif /* SOUND_H */
