#ifndef GAME_SOUND_RECORDS_H
#define GAME_SOUND_RECORDS_H

#include "common.h"

/* SPU upload-block log entry (12 bytes/entry). Tracks one upload's byte
 * size so PopSPUUploadBlock can rewind SPU_UPLOAD_USED_BYTES on free.
 * Only `sz` is read in this file; the trailing 10 bytes are opaque. */
typedef struct {
    /* 0x00 */ u16 sz;
    /* 0x02 */ u8  _pad02[10];
} SpuUploadBlock;

/* Sound-definition table entry (12 bytes/entry). First field is the base
 * mixer volume passed to CalculateStereoVolume; the other 10 bytes hold
 * envelope / pitch / category data not touched here. */
typedef struct {
    /* 0x00 */ s16 volume;
    /* 0x02 */ u8  _pad02[10];
} SoundDefinition;

#endif /* GAME_SOUND_RECORDS_H */
