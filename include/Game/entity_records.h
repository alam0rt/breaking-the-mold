#ifndef GAME_ENTITY_RECORDS_H
#define GAME_ENTITY_RECORDS_H

#include "common.h"

/* NOTE: this SpriteRenderContext is entity.c's local overlay view (only the
 * fields FreeWithCallback touches). It differs from the same-named views in
 * anim.c / enemy_entities.h. Each per-file header is included by exactly one
 * translation unit so the duplicate names do not collide. */

typedef struct SpriteContextCallbackTable {
    /* 0x00 */ u8 pad00[0x10];
    /* 0x10 */ s16 callbackTargetOffset;
    /* 0x12 */ u8 pad12[2];
    /* 0x14 */ void (*releaseVRAMSlot)(u8 *target, s32 mode);
} SpriteContextCallbackTable;

typedef struct SpriteRenderContext {
    /* 0x00 */ u8 pad00[4];
    /* 0x04 */ s16 width;
    /* 0x06 */ s16 height;
    /* 0x08 */ u8 pad08[4];
    /* 0x0C */ SpriteContextCallbackTable *callbacks;
} SpriteRenderContext;

#endif /* GAME_ENTITY_RECORDS_H */
