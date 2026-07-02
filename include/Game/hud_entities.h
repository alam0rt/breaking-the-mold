#ifndef GAME_HUD_ENTITIES_H
#define GAME_HUD_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* Pause-menu toggle entity (selection between 2 items, e.g. yes/no). */
typedef struct PauseMenuToggleEntity {
    /* 0x00 */ Entity base;
    /* 0x80 */ u8 _pad80[0x18];
    /* 0x98 */ Entity *items[2];
    /* 0xA0 */ u8 selection;
} PauseMenuToggleEntity;

/* 3-slot options-menu entity (label sprite + 2 value sprites), with a
 * pressed/idle state byte at +0xA0 and a press-latch flag at +0xA1.
 * Items live at +0x94/+0x98/+0x9C (not a 0x98-anchored array). */
typedef struct OptionsMenuTripleEntity {
    /* 0x00 */ Entity base;
    /* 0x80 */ u8 _pad80[0x14];
    /* 0x94 */ Entity *label;
    /* 0x98 */ Entity *itemA;
    /* 0x9C */ Entity *itemB;
    /* 0xA0 */ u8 pressedState;
    /* 0xA1 */ u8 pressLatch;
} OptionsMenuTripleEntity;

#endif /* GAME_HUD_ENTITIES_H */
