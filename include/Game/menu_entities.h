#ifndef MENU_ENTITIES_H
#define MENU_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * MENU / FRONT-END ENTITY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These extend SpriteEntity (0x100 base) with menu-widget scratch (highlight
 * ref, active flag, selection tables); padding runs cover offsets not yet
 * traced. Used exclusively by src/menu.c.
 * ============================================================================= */

typedef struct TimedMenuEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u16 timer;
} TimedMenuEntity;

typedef struct MenuButtonEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ Entity *highlight;
    /* 0x104 */ u8 active;
    /* 0x105 */ u8 pad105[3];
    /* 0x108 */ u8 typeByte;
    /* 0x109 */ u8 backFlag;
} MenuButtonEntity;

typedef struct MenuPoint {
    /* 0x00 */ s16 x;
    /* 0x02 */ s16 y;
} MenuPoint;

typedef struct MenuLevelSelectEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ Entity *highlight;
    /* 0x104 */ u8 active;
    /* 0x105 */ u8 pad105[3];
    /* 0x108 */ MenuPoint *positionTable;
    /* 0x10C */ u8 levelCount;
    /* 0x10D */ u8 pad10D[3];
    /* 0x110 */ u8 *currentIndex;
    /* 0x114 */ Entity *levelIcon;
    /* 0x118 */ u8 *audioSettings;
} MenuLevelSelectEntity;

typedef struct MenuSkullIconEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ Entity *highlight;
    /* 0x104 */ u8 active;
    /* 0x105 */ u8 pad105[3];
    /* 0x108 */ u8 *value;
    /* 0x10C */ Entity *icon;
} MenuSkullIconEntity;

#endif /* MENU_ENTITIES_H */
