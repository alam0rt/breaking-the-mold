#ifndef GAME_CLAYBALL_ENTITIES_H
#define GAME_CLAYBALL_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* Common per-entity spawn definition header used by clayball/circular-platform
 * constructors: opaque header followed by world position (+0x8/+0xA) and a
 * u16 init parameter (+0xC) consumed by GenericSpriteEntityInitCallback. */
typedef struct ClayballSpawnDef {
    /* 0x00 */ u8  pad00[8];
    /* 0x08 */ s16 worldX;
    /* 0x0A */ u16 worldY;
    /* 0x0C */ u16 initParam;
} ClayballSpawnDef;

typedef struct SwitchClayballEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 *spawnDef;
    /* 0x104 */ u8 pad104[0x11A - 0x104];
    /* 0x11A */ u8 active;
    /* 0x11B */ u8 pad11B[0x124 - 0x11B];
    /* 0x124 */ Entity *linkedEntity;
    /* 0x128 */ s32 signalPayload;
} SwitchClayballEntity;

typedef struct SoundClayballEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x128 - 0x100];
    /* 0x128 */ s32 rollingVoice;
    /* 0x12C */ s32 impactVoice;
} SoundClayballEntity;

#endif /* GAME_CLAYBALL_ENTITIES_H */
