#ifndef GAME_SPRSET_RECORDS_H
#define GAME_SPRSET_RECORDS_H

#include "common.h"

/* Local layout view of SpriteEntity used by the pending-sprite-state setters
 * in sprset.c -- only the "pending" fields those helpers touch are named. */
typedef struct SpriteEntity_PendingView {
    u8  pad00[0xBC];
    /* 0xBC */ u32 pendingSpriteId;
    /* 0xC0 */ u32 pendingFrame;        /* 32-bit write covers s16 frame + s16 _pad */
    /* 0xC4 */ u32 pendingLoopFrame;    /* same shape as pendingFrame */
    /* 0xC8 */ u32 pendingTargetFrame;  /* 32-bit "marker"; literal 0xFFFF latched here */
    u8  padCC[0xE0 - 0xCC];
    /* 0xE0 */ u16 animChangeFlags;
    u8  padE2[0xF3 - 0xE2];
    /* 0xF3 */ u8  pendingDirection;
    /* 0xF4 */ u8  pendingLoopFlag;
    /* 0xF5 */ u8  pendingAnimActive;
} SpriteEntity_PendingView;

#endif /* GAME_SPRSET_RECORDS_H */
