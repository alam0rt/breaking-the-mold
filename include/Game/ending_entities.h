#ifndef GAME_ENDING_ENTITIES_H
#define GAME_ENDING_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

typedef struct EndingEntityWithState {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u32 stateValue;
} EndingEntityWithState;

typedef struct EndingSequenceEntity {
    /* 0x00 */ u8 pad00[0x5C];
    /* 0x5C */ u32 argA;
    /* 0x60 */ u8 resetValue;
    /* 0x61 */ u8 pad61[3];
    /* 0x64 */ u32 argB;
} EndingSequenceEntity;

/* Extended entity used by the ending-credits tick chain. The sprite base
 * runs 0x00..0xFF; the credits-specific scroll/delay/sound state lives at
 * 0x100+. Fields used by the tick callbacks observed in asm:
 *   +0x136 scroll-counter (ScrollTick)        +0x148 delay counter
 *   +0x142 scroll-counter (ScrollTick2)       +0x149 flag (FadeOutTick)
 *   +0x144 scroll-y position                  +0x14A fade counter
 *   +0x150 SPU sound-effect handle */
typedef struct EndingCreditsEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u32          destroyFlag;
    /* 0x104 */ u8           pad104[0x32];
    /* 0x136 */ u16          scrollCounter1;
    /* 0x138 */ u8           pad138[0x8];
    /* 0x140 */ u16          field140;
    /* 0x142 */ u16          scrollCounter2;
    /* 0x144 */ u16          scrollY;
    /* 0x146 */ u8           pad146[2];
    /* 0x148 */ u8           delayCounter;
    /* 0x149 */ u8           flag149;
    /* 0x14A */ u8           fadeCounter;
    /* 0x14B */ u8           pad14B;
    /* 0x14C */ u8           flag14C;
    /* 0x14D */ u8           pad14D[3];
    /* 0x150 */ s32          soundHandle;
} EndingCreditsEntity;

#endif /* GAME_ENDING_ENTITIES_H */
