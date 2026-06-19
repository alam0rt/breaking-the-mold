#include "common.h"
#include "functions.h"
#include "globals.h"

extern void SetReverbLevel(u8 level);
extern void SetAudioVolume(u8 volume);
extern void SetStereoMode(u8 mode);

typedef struct OptionsMenuEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x2E];
    /* 0x12E */ u8 reverbLevel;
    /* 0x12F */ u8 audioVolume;
    /* 0x130 */ u8 stereoMode;
    /* 0x131 */ u8 pad131[0x0B];
    /* 0x13C */ u16 demoCountdown;
} OptionsMenuEntity;

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

