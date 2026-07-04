#include "common.h"
#include "functions.h"
#include "globals.h"

extern void SetReverbLevel(u8 level);
extern void SetAudioVolume(u8 volume);
extern void SetStereoMode(u8 mode);

#include "Game/passwd_entities.h"

u8 MENU_BACKGROUND_COLOR_INDEX asm("D_800A6042");
u8 MENU_SHARED_SELECTION_BYTE asm("D_800A6045");

INCLUDE_ASM("asm/nonmatchings/passwd", InitPasswordDisplayEntity);

INCLUDE_ASM("asm/nonmatchings/passwd", PasswordDigitSelectActive);

INCLUDE_ASM("asm/nonmatchings/passwd", PasswordDigitSelectInactive);

INCLUDE_ASM("asm/nonmatchings/passwd", PasswordSubmitEventHandler);

INCLUDE_ASM("asm/nonmatchings/passwd", PasswordBackspaceHandler);

INCLUDE_ASM("asm/nonmatchings/passwd", PasswordDigitInputHandler);

INCLUDE_ASM("asm/nonmatchings/passwd", InitMenuLogoEntity);

INCLUDE_ASM("asm/nonmatchings/passwd", MenuLogoAnimEventHandler);

INCLUDE_ASM("asm/nonmatchings/passwd", InitMenuEntity);

INCLUDE_ASM("asm/nonmatchings/passwd", OptionsMenuDestroyCallback);

INCLUDE_ASM("asm/nonmatchings/passwd", InitMenuStage1);

INCLUDE_ASM("asm/nonmatchings/passwd", InitMenuStage2);

INCLUDE_ASM("asm/nonmatchings/passwd", InitMenuStage3);

INCLUDE_ASM("asm/nonmatchings/passwd", InitMenuStage4);

void ApplyAudioSettings(OptionsMenuEntity *e) {
    SetReverbLevel(e->reverbLevel);
    SetAudioVolume(e->audioVolume);
    SetStereoMode((u8)(e->stereoMode + 1));
}

void UpdateBackgroundColor(void);

void UpdateBackgroundColorWrapper(void) {
    UpdateBackgroundColor();
}

extern u8 MENU_BACKGROUND_COLOR_TABLE[] asm("D_8009CBAC");
extern s16 MENU_BACKGROUND_R asm("D_800A5970");
extern s16 MENU_BACKGROUND_G asm("D_800A596C");
extern s16 MENU_BACKGROUND_B asm("D_800A596E");

void UpdateBackgroundColor(void) {
    u32 idx = MENU_BACKGROUND_COLOR_INDEX * 3;
    MENU_BACKGROUND_R = MENU_BACKGROUND_COLOR_TABLE[idx];
    MENU_BACKGROUND_G = MENU_BACKGROUND_COLOR_TABLE[idx + 1];
    MENU_BACKGROUND_B = MENU_BACKGROUND_COLOR_TABLE[idx + 2];
}

INCLUDE_ASM("asm/nonmatchings/passwd", MenuTickCallback);

void DemoCountdownCallback(OptionsMenuEntity *e) {
    e->demoCountdown--;
    if (e->demoCountdown == 0) {
        g_pGameState->direct_level_load = MENU_SHARED_SELECTION_BYTE;
    }
    EntityUpdateCallback(&e->sprite.base);
}

INCLUDE_ASM("asm/nonmatchings/passwd", MenuInputHandler);


/* .data island 0x8009CB00..0x8009CB7C (124B, passwd coordinate tables) migrated from asm. */
/* 12-byte zero header followed by s16 (x,y) screen-position grids for the
 * password entry UI; the +N aliases expose the individual sub-table columns. */
s16 D_8009CB00[62] asm("D_8009CB00") = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x002C, 0x0063,
    0x0062, 0x0027, 0x007E, 0x0003, 0x0028, 0x009A, 0x0004, 0x0022,
    0x00B8, 0x0002, 0x00C4, 0x006B, 0x00D4, 0x006B, 0x00E4, 0x006B,
    0x00F4, 0x006B, 0x0104, 0x006B, 0x00C0, 0x008A, 0x00D1, 0x008A,
    0x00E3, 0x008A, 0x00F4, 0x008A, 0x0106, 0x008A, 0x00B2, 0x0076,
    0x00C4, 0x0075, 0x00D5, 0x0075, 0x00E7, 0x0075, 0x00F8, 0x0076,
    0x010B, 0x0076, 0x00B3, 0x0094, 0x00C4, 0x0093, 0x00D6, 0x0093,
    0x00E7, 0x0093, 0x00F9, 0x0095, 0x010B, 0x0094,
};
asm(".globl D_8009CB0C\n.set D_8009CB0C, D_8009CB00 + 12\n.globl D_8009CB0E\n.set D_8009CB0E, D_8009CB00 + 14\n.globl D_8009CB10\n.set D_8009CB10, D_8009CB00 + 16\n.globl D_8009CB24\n.set D_8009CB24, D_8009CB00 + 36\n.globl D_8009CB38\n.set D_8009CB38, D_8009CB00 + 56\n.globl D_8009CB4C\n.set D_8009CB4C, D_8009CB00 + 76\n.globl D_8009CB4E\n.set D_8009CB4E, D_8009CB00 + 78\n");
