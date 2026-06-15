#include "common.h"

extern void SpuSetKey(s32 onoff, s32 voiceBits);
extern void StopCDAudio(void);
extern void SpuQuit(void);
/* Tentative defs to unlock gp_rel via maspsx --use-comm-section. */
u32 D_800A6078;
u8  D_800A6081;
u8  D_800A6085;
u8  D_800A6088;

INCLUDE_ASM("asm/nonmatchings/sound", InitSPUDefaults);

INCLUDE_ASM("asm/nonmatchings/sound", UploadAudioToSPU);

INCLUDE_ASM("asm/nonmatchings/sound", PopSPUUploadBlock);

void func_8007C324(void) {
    D_800A6078 = 0;
    D_800A6081 = 0;
}

void ShutdownSPUAndResetSoundState(void) {
    SpuQuit();
    D_800A6078 = 0;
    D_800A6081 = 0;
}

void func_8007C35C(void) {
}

void func_8007C364(void) {
}

INCLUDE_ASM("asm/nonmatchings/sound", SetGameMode);

INCLUDE_ASM("asm/nonmatchings/sound", PlaySoundEffect);

INCLUDE_ASM("asm/nonmatchings/sound", PlayGameSoundById);

void StopSPUVoice(s32 voice) {
    if (voice != -1) {
        SpuSetKey(0, 1 << voice);
    }
}

void StopAllSPUVoices(void) {
    SpuSetKey(0, 0xFFFFFF);
    D_800A6088 = 0;
}

INCLUDE_ASM("asm/nonmatchings/sound", CalculateStereoVolume);

INCLUDE_ASM("asm/nonmatchings/sound", SetVoicePanning);

INCLUDE_ASM("asm/nonmatchings/sound", StartCDAudioForLevel);

void StopCDStreaming(void) {
    StopCDAudio();
    D_800A6085 = 0;
}

INCLUDE_ASM("asm/nonmatchings/sound", SaveAndMuteAllVoicePitches);

INCLUDE_ASM("asm/nonmatchings/sound", ResumeAllVoicePitches);

INCLUDE_ASM("asm/nonmatchings/sound", SetStereoMode);

INCLUDE_ASM("asm/nonmatchings/sound", SetReverbLevel);

INCLUDE_ASM("asm/nonmatchings/sound", SetAudioVolume);

INCLUDE_ASM("asm/nonmatchings/sound", TickCDStreamBuffer);

