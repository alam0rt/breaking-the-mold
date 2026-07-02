#ifndef GAME_PASSWD_ENTITIES_H
#define GAME_PASSWD_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

typedef struct OptionsMenuEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x2E];
    /* 0x12E */ u8 reverbLevel;
    /* 0x12F */ u8 audioVolume;
    /* 0x130 */ u8 stereoMode;
    /* 0x131 */ u8 pad131[0x0B];
    /* 0x13C */ u16 demoCountdown;
} OptionsMenuEntity;

#endif /* GAME_PASSWD_ENTITIES_H */
