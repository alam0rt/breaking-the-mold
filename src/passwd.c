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


/* .data island 0x8009CB00..0x8009CB7C (passwd tables) migrated from asm; grouped u8[] + .set aliases. */
/* group island: 0-byte pad at 0x8009CB00, 8 aliased symbol(s); anchor D_8009CB00 (124B). */
u8 D_8009CB00[124] asm("D_8009CB00") = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x2C, 0x00, 0x63, 0x00,
    0x62, 0x00, 0x27, 0x00, 0x7E, 0x00, 0x03, 0x00,
    0x28, 0x00, 0x9A, 0x00, 0x04, 0x00, 0x22, 0x00,
    0xB8, 0x00, 0x02, 0x00, 0xC4, 0x00, 0x6B, 0x00,
    0xD4, 0x00, 0x6B, 0x00, 0xE4, 0x00, 0x6B, 0x00,
    0xF4, 0x00, 0x6B, 0x00, 0x04, 0x01, 0x6B, 0x00,
    0xC0, 0x00, 0x8A, 0x00, 0xD1, 0x00, 0x8A, 0x00,
    0xE3, 0x00, 0x8A, 0x00, 0xF4, 0x00, 0x8A, 0x00,
    0x06, 0x01, 0x8A, 0x00, 0xB2, 0x00, 0x76, 0x00,
    0xC4, 0x00, 0x75, 0x00, 0xD5, 0x00, 0x75, 0x00,
    0xE7, 0x00, 0x75, 0x00, 0xF8, 0x00, 0x76, 0x00,
    0x0B, 0x01, 0x76, 0x00, 0xB3, 0x00, 0x94, 0x00,
    0xC4, 0x00, 0x93, 0x00, 0xD6, 0x00, 0x93, 0x00,
    0xE7, 0x00, 0x93, 0x00, 0xF9, 0x00, 0x95, 0x00,
    0x0B, 0x01, 0x94, 0x00,
};
asm(".globl D_8009CB0C\n.set D_8009CB0C, D_8009CB00 + 12\n.globl D_8009CB0E\n.set D_8009CB0E, D_8009CB00 + 14\n.globl D_8009CB10\n.set D_8009CB10, D_8009CB00 + 16\n.globl D_8009CB24\n.set D_8009CB24, D_8009CB00 + 36\n.globl D_8009CB38\n.set D_8009CB38, D_8009CB00 + 56\n.globl D_8009CB4C\n.set D_8009CB4C, D_8009CB00 + 76\n.globl D_8009CB4E\n.set D_8009CB4E, D_8009CB00 + 78\n");
