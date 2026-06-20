#include "common.h"
#include "Game/entity.h"

extern void PlaySoundEffect(u32 id, s32 vol, s32 ch);
extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);

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

void UpdateHUDEntityVisibility(void);

void HUD_UpdateVisibilityWrapper(void) {
    UpdateHUDEntityVisibility();
}

INCLUDE_ASM("asm/nonmatchings/hud", UpdateHUDEntityVisibility);

INCLUDE_ASM("asm/nonmatchings/hud", EntityCollision_HUDItemActivate);

INCLUDE_ASM("asm/nonmatchings/hud", ShowPauseMenuHUD);

INCLUDE_ASM("asm/nonmatchings/hud", HidePauseMenuHUD);

/* Cycle the 2-option selection: highlight the new item (r/g/b = 0x80)
 * and dim the previous item (r/g/b = 0x40). The r/g/b bytes are at +0x34,
 * +0x35, +0x36 within the item's sprite-context block. */
void ToggleMenuSelectionHighlight(PauseMenuToggleEntity *entity) {
    u8 *color;

    PlaySoundEffect(0x646c2cc0u, 0xa0, 1);

    entity->selection = (entity->selection + 1) & 1;

    color = (u8 *)entity->items[entity->selection]->spriteContext;
    color[0x34] = 0x80;
    color[0x35] = 0x80;
    color[0x36] = 0x80;

    color = (u8 *)entity->items[(entity->selection + 1) & 1]->spriteContext;
    color[0x34] = 0x40;
    color[0x35] = 0x40;
    color[0x36] = 0x40;
}

/* Confirm/cancel a "press-and-hold" options-menu entry.
 *   pressLatch=0, pressedState=0 -> no input yet, return 1 (idle).
 *   pressLatch=0, pressedState!=0 -> first commit: flip label + values to
 *       the "pressed" sprite set, dim itemA, highlight itemB, latch.
 *   pressLatch!=0, pressedState=0 -> spurious release, return 2.
 *   pressLatch!=0, pressedState!=0 -> release: restore "idle" sprites,
 *       highlight itemA, dim itemB, clear latch. */
s32 ToggleMenuOptionState(OptionsMenuTripleEntity *entity) {
    u8 *color;

    PlaySoundEffect(0x90810000u, 0xa0, 1);

    if (entity->pressLatch == 0) {
        if (entity->pressedState == 0) {
            return 1;
        }
        entity->pressLatch = 1;
        entity->pressedState = 1;
        SetEntitySpriteId(entity->label, 0x69C8F473, 1);
        SetEntitySpriteId(entity->itemA, 0x2AD0F011, 1);
        SetEntitySpriteId(entity->itemB, 0x29C0E211, 1);
        color = (u8 *)entity->itemA->spriteContext;
        color[0x34] = 0x40;
        color[0x35] = 0x40;
        color[0x36] = 0x40;
        color = (u8 *)entity->itemB->spriteContext;
        color[0x34] = 0x80;
        color[0x35] = 0x80;
        color[0x36] = 0x80;
    } else {
        if (entity->pressedState == 0) {
            return 2;
        }
        entity->pressLatch = 0;
        entity->pressedState = 0;
        SetEntitySpriteId(entity->label, 0x0AD0F813, 1);
        SetEntitySpriteId(entity->itemA, 0x69C04050, 1);
        SetEntitySpriteId(entity->itemB, 0x68C0F413, 1);
        color = (u8 *)entity->itemA->spriteContext;
        color[0x34] = 0x80;
        color[0x35] = 0x80;
        color[0x36] = 0x80;
        color = (u8 *)entity->itemB->spriteContext;
        color[0x34] = 0x40;
        color[0x35] = 0x40;
        color[0x36] = 0x40;
    }
    return 0;
}

