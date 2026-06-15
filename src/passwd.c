#include "common.h"

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

INCLUDE_ASM("asm/nonmatchings/passwd", ApplyAudioSettings);

void UpdateBackgroundColor(void);

void UpdateBackgroundColorWrapper(void) {
    UpdateBackgroundColor();
}

INCLUDE_ASM("asm/nonmatchings/passwd", UpdateBackgroundColor);

INCLUDE_ASM("asm/nonmatchings/passwd", MenuTickCallback);

INCLUDE_ASM("asm/nonmatchings/passwd", DemoCountdownCallback);

INCLUDE_ASM("asm/nonmatchings/passwd", MenuInputHandler);

