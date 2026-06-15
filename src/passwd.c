#include "common.h"
#include "functions.h"

extern void SetReverbLevel(u8 level);
extern void SetAudioVolume(u8 volume);
extern void SetStereoMode(u8 mode);
extern u8 *g_pGameState;
u8 D_800A6042;
u8 D_800A6045;

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

void ApplyAudioSettings(void *e) {
    SetReverbLevel(*(u8 *)((u8 *)e + 0x12E));
    SetAudioVolume(*(u8 *)((u8 *)e + 0x12F));
    SetStereoMode((u8)(*(u8 *)((u8 *)e + 0x130) + 1));
}

void UpdateBackgroundColor(void);

void UpdateBackgroundColorWrapper(void) {
    UpdateBackgroundColor();
}

extern u8 D_8009CBAC[];
extern s16 D_800A5970;
extern s16 D_800A596C;
extern s16 D_800A596E;

void UpdateBackgroundColor(void) {
    u32 idx = D_800A6042 * 3;
    D_800A5970 = D_8009CBAC[idx];
    D_800A596C = D_8009CBAC[idx + 1];
    D_800A596E = D_8009CBAC[idx + 2];
}

INCLUDE_ASM("asm/nonmatchings/passwd", MenuTickCallback);

void DemoCountdownCallback(Entity *e) {
    u16 *ctr = (u16 *)((u8 *)e + 0x13C);
    *ctr = *ctr - 1;
    if (*ctr == 0) {
        g_pGameState[0x148] = D_800A6045;
    }
    EntityUpdateCallback(e);
}

INCLUDE_ASM("asm/nonmatchings/passwd", MenuInputHandler);

