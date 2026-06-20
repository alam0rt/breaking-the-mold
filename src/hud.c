#include "common.h"
#include "Game/entity.h"

extern void PlaySoundEffect(u32 id, s32 vol, s32 ch);

/* Pause-menu toggle entity (selection between 2 items, e.g. yes/no). */
typedef struct PauseMenuToggleEntity {
    /* 0x00 */ Entity base;
    /* 0x80 */ u8 _pad80[0x18];
    /* 0x98 */ Entity *items[2];
    /* 0xA0 */ u8 selection;
} PauseMenuToggleEntity;

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

INCLUDE_ASM("asm/nonmatchings/hud", ToggleMenuOptionState);

