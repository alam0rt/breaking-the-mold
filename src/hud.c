#include "common.h"
#include "Game/entity.h"
#include "Game/asset_ids.h"
#include "Game/hud_entities.h"

extern void PlaySoundEffect(u32 id, s32 vol, s32 ch);
extern void SetEntitySpriteId(Entity *e, u32 spriteId, s32 flags);

void UpdateHUDEntityVisibility(void);

void HUD_UpdateVisibilityWrapper(void) {
    UpdateHUDEntityVisibility();
}

INCLUDE_ASM("asm/nonmatchings/hud", UpdateHUDEntityVisibility);

INCLUDE_ASM("asm/nonmatchings/hud", EntityCollision_HUDItemActivate);

INCLUDE_ASM("asm/nonmatchings/hud", ShowPauseMenuHUD);

INCLUDE_ASM("asm/nonmatchings/hud", HidePauseMenuHUD);

/* Pause-menu cursor move: flips the 2-option selection (e.g. between the two
 * items of a yes/no toggle) and re-tints them — the newly-selected item gets
 * the bright RGB triple (0x80,0x80,0x80), the deselected item the dim one
 * (0x40,0x40,0x40). The RGB bytes live at +0x34..+0x36 within each item's
 * sprite-context block. Plays the menu cursor-move sfx (0x646c2cc0 — asset
 * hash uncracked; a navigation blip present in every level BLB). */
void ToggleMenuSelectionHighlight(PauseMenuToggleEntity *entity) {
    u8 *rgb;

    PlaySoundEffect(0x646c2cc0u, 0xa0, 1);

    entity->selection = (entity->selection + 1) & 1;

    rgb = (u8 *)entity->items[entity->selection]->spriteContext;
    rgb[0x34] = 0x80;
    rgb[0x35] = 0x80;
    rgb[0x36] = 0x80;

    rgb = (u8 *)entity->items[(entity->selection + 1) & 1]->spriteContext;
    rgb[0x34] = 0x40;
    rgb[0x35] = 0x40;
    rgb[0x36] = 0x40;
}

/* Drives the in-game PAUSE menu's quit-confirmation toggle (a 3-sprite widget:
 * a label + two option items). Cracked sprite ids make the two display states
 * explicit:
 *   un-latched ("PAUSED"):       label=PAUSED, itemA=CONTINUE (hi), itemB=QUIT (dim)
 *   latched   ("quit confirm"):  label=QUIT GAME, itemA=YES (dim), itemB=NO (hi)
 * The hi/dim tint is the RGB triple at spriteContext+0x34..0x36 (0x80=bright,
 * 0x40=dimmed); the *safe* option (CONTINUE / NO) is the bright one each time.
 * Plays FX_MENU_SELECT on entry. Return: 1 = no input yet, 2 = spurious
 * release, 0 = state toggled. Transitions by (pressLatch, pressedState):
 *   (0,0) idle -> 1 · (0,!=0) commit -> show quit-confirm, latch
 *   (!=0,0) spurious -> 2 · (!=0,!=0) release -> restore PAUSED, clear latch */
s32 ToggleMenuOptionState(OptionsMenuTripleEntity *entity) {
    u8 *rgb;

    PlaySoundEffect(FX_MENU_SELECT, 0xa0, 1);

    if (entity->pressLatch == 0) {
        if (entity->pressedState == 0) {
            return 1;
        }
        entity->pressLatch = 1;
        entity->pressedState = 1;
        SetEntitySpriteId(entity->label, SPR_QUIT_GAME, 1);
        SetEntitySpriteId(entity->itemA, SPR_YES, 1);
        SetEntitySpriteId(entity->itemB, SPR_NO, 1);
        rgb = (u8 *)entity->itemA->spriteContext;
        rgb[0x34] = 0x40;
        rgb[0x35] = 0x40;
        rgb[0x36] = 0x40;
        rgb = (u8 *)entity->itemB->spriteContext;
        rgb[0x34] = 0x80;
        rgb[0x35] = 0x80;
        rgb[0x36] = 0x80;
    } else {
        if (entity->pressedState == 0) {
            return 2;
        }
        entity->pressLatch = 0;
        entity->pressedState = 0;
        SetEntitySpriteId(entity->label, SPR_PAUSED, 1);
        SetEntitySpriteId(entity->itemA, SPR_CONTINUE, 1);
        SetEntitySpriteId(entity->itemB, SPR_QUIT, 1);
        rgb = (u8 *)entity->itemA->spriteContext;
        rgb[0x34] = 0x80;
        rgb[0x35] = 0x80;
        rgb[0x36] = 0x80;
        rgb = (u8 *)entity->itemB->spriteContext;
        rgb[0x34] = 0x40;
        rgb[0x35] = 0x40;
        rgb[0x36] = 0x40;
    }
    return 0;
}

