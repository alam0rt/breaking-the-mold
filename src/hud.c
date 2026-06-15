#include "common.h"

void UpdateHUDEntityVisibility(void);

void HUD_UpdateVisibilityWrapper(void) {
    UpdateHUDEntityVisibility();
}

INCLUDE_ASM("asm/nonmatchings/hud", UpdateHUDEntityVisibility);

INCLUDE_ASM("asm/nonmatchings/hud", EntityCollision_HUDItemActivate);

INCLUDE_ASM("asm/nonmatchings/hud", ShowPauseMenuHUD);

INCLUDE_ASM("asm/nonmatchings/hud", HidePauseMenuHUD);

INCLUDE_ASM("asm/nonmatchings/hud", ToggleMenuSelectionHighlight);

INCLUDE_ASM("asm/nonmatchings/hud", ToggleMenuOptionState);

