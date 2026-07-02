#ifndef GAME_PLAYDTOR_ENTITIES_H
#define GAME_PLAYDTOR_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

typedef struct EntityWithSpuVoiceAt10c {
    SpriteEntity sprite;
    u8 pad100[0x10C - 0x100];
    u32 spuVoice;
} EntityWithSpuVoiceAt10c;

#endif /* GAME_PLAYDTOR_ENTITIES_H */
