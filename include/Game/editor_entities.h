#ifndef GAME_EDITOR_ENTITIES_H
#define GAME_EDITOR_ENTITIES_H

#include "common.h"

typedef struct EditorEntity {
    /* 0x00 */ u8 pad00[0x1C];
    /* 0x1C */ struct {
        u8 pad00[0x0A];
        u8 enabled;
    } *primObject;
    /* 0x20 */ u8 childDisabledFlag;
} EditorEntity;

#endif /* GAME_EDITOR_ENTITIES_H */
