#ifndef GAME_DECOR_ENTITIES_H
#define GAME_DECOR_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

typedef struct DecorSpawnData {
    /* 0x00 */ u8 pad00[8];
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
} DecorSpawnData;

typedef struct SpecialTriggerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x1D];
    /* 0x11D */ u8 triggered;
} SpecialTriggerEntity;

#endif /* GAME_DECOR_ENTITIES_H */
