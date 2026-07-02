#ifndef GAME_SPRITE_RECORDS_H
#define GAME_SPRITE_RECORDS_H

#include "common.h"

typedef struct BasicPrimObject {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ u16 id;
    /* 0x0A */ u8 enabled;
    /* 0x0B */ u8 pad0B;
    /* 0x0C */ void *vtable;
} BasicPrimObject;

typedef struct TilemapLayerRenderObject {
    /* 0x00 */ s16 scrollX;
    /* 0x02 */ s16 scrollY;
    /* 0x04 */ u8 pad04[0x4C];
    /* 0x50 */ struct {
        s16 x;
        s16 y;
    } frameScroll[2];
} TilemapLayerRenderObject;

#endif /* GAME_SPRITE_RECORDS_H */
