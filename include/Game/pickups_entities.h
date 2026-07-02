#ifndef PICKUPS_ENTITIES_H
#define PICKUPS_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * PICKUP / DECOR ENTITY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These extend SpriteEntity (0x100 base) with decor/pickup scratch (trigger
 * state, random timer, child slot) or overlay spawn/sprite-context records;
 * padding runs cover offsets not yet traced. Used exclusively by src/pickups.c.
 * ============================================================================= */

typedef struct DecorSpawnData {
    /* 0x00 */ u8 pad00[8];
    /* 0x08 */ s16 x;
    /* 0x0A */ s16 y;
} DecorSpawnData;

typedef struct InteractiveDecorEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x11D - 0x100];
    /* 0x11D */ u8 triggerState;
} InteractiveDecorEntity;

/* Animated decor whose tick fires on a random timer (e.g. blinking eyes,
 * flickering background effects). +0x120 is the down-counter byte armed
 * by DecorSetRandomTimer / DecorStartWithRandomTimer. */
typedef struct DecorRandomTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x120 - 0x100];
    /* 0x120 */ u8 randomTimer;
} DecorRandomTimerEntity;

/* Sprite render context (Entity+0x34) view: the fields used to build the GPU
 * tpage from the sprite's reserved VRAM rect. Matches RenderSprite (spracc.c). */
typedef struct DecorSpriteCtx {
    /* 0x00 */ u8  pad00[0x10];
    /* 0x10 */ s16 vramX;
    /* 0x12 */ s16 vramY;
    /* 0x14 */ u8  pad14[0x24 - 0x14];
    /* 0x24 */ u16 tpage;
    /* 0x26 */ u8  pad26[0x32 - 0x26];
    /* 0x32 */ u8  abr;
} DecorSpriteCtx;

/* Powerup-collectible (extra life, phoenix hand, super willie...) shape:
 * extends InteractiveDecorEntity with a sub-entity pointer at +0x100 that
 * gets a +0x12 (collisionMask) field stamped with 7 just before the
 * award call. The stamp goes into the award's delay slot. */
typedef struct PowerupCollectibleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ Entity *labelEntity;
    /* 0x104 */ u8 pad104[0x11D - 0x104];
    /* 0x11D */ u8 triggerState;
} PowerupCollectibleEntity;

/* Decor entity that owns a detached render-list slot at +0x120 (the
 * HUD-icon child allocated by InitDecorEntityWithHUDIcon). */
typedef struct DecorChildSlotEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x120 - 0x100];
    /* 0x120 */ void *childSlot;
} DecorChildSlotEntity;

#endif /* PICKUPS_ENTITIES_H */
