#ifndef FINN_ENTITIES_H
#define FINN_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * FINN (flying-craft vehicle) ENTITY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These extend SpriteEntity (0x100 base) or overlay the basic-entity block
 * with FINN-specific scratch; padding runs cover offsets not yet traced.
 * Used exclusively by src/finn.c.
 * ============================================================================= */

typedef struct FinnSubentityStateFlags {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10D - 0x100];
    /* 0x10D */ u8 stateFlag;
    /* 0x10E */ u8 pad10E;
    /* 0x10F */ u8 stateValue;
} FinnSubentityStateFlags;

typedef struct FinnPointerEntity {
    /* 0x00 */ u8 pad00[0x24];
    /* 0x24 */ u8 *target;
    /* 0x28 */ u8 pad28[0x2C - 0x28];
    /* 0x2C */ u8 smallFlag;
} FinnPointerEntity;

typedef struct FinnPairEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x104 - 0x100];
    /* 0x104 */ s32 pairA;
    /* 0x108 */ s32 pairB;
} FinnPairEntity;

typedef struct FinnStateValueEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u32 stateValue;
} FinnStateValueEntity;

/* Used by FinnSpawnCountdownTickCallback: u8 down-counter at +0x100; when
 * it hits 0 the render slot is overwritten with FinnExitMoveRightTickCallback. */
typedef struct FinnSpawnCountdownEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 spawnCountdown;
} FinnSpawnCountdownEntity;

typedef struct FinnVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x114 - 0x100];
    /* 0x114 */ s32 voiceHandle;
} FinnVoiceEntity;

/* Used by FinnTick_LevelExitCountdown / FinnStateInit_SetTimerAndTick.
 * +0x118 is a u8 down-counter decremented each tick; when it hits 0 the
 * tick callback flips +0x11A from 0 to 1. */
typedef struct FinnLevelExitEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x118 - 0x100];
    /* 0x118 */ u8 exitTimer;
    /* 0x119 */ u8 pad119;
    /* 0x11A */ u8 exitFlag;
} FinnLevelExitEntity;

typedef struct FinnRenderPrim {
    /* 0x000 */ s16 screenX;
    /* 0x002 */ s16 screenY;
    /* 0x004 */ u8 pad4[0x1E3];
    /* 0x1E7 */ u8 phase;
} FinnRenderPrim;

typedef struct FinnScreenPosEntity {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ u16 worldX;
    /* 0x22 */ u16 worldY;
    /* 0x24 */ FinnRenderPrim *prim;
    /* 0x28 */ s32 scale; /* 16.16; 0x10000 = 1.0 */
} FinnScreenPosEntity;

typedef struct FinnAngleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8  pad100[0xC];
    /* 0x10C */ s16 rotationAngle;
    /* 0x10E */ u8  spriteBucket;
} FinnAngleEntity;

#endif /* FINN_ENTITIES_H */
