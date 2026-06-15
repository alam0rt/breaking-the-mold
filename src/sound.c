#include "common.h"

extern void SpuSetKey(s32 onoff, s32 voiceBits);

INCLUDE_ASM("asm/nonmatchings/sound", InitSPUDefaults);

INCLUDE_ASM("asm/nonmatchings/sound", UploadAudioToSPU);

INCLUDE_ASM("asm/nonmatchings/sound", PopSPUUploadBlock);

INCLUDE_ASM("asm/nonmatchings/sound", func_8007C324);

INCLUDE_ASM("asm/nonmatchings/sound", ShutdownSPUAndResetSoundState);

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

INCLUDE_ASM("asm/nonmatchings/sound", StopAllSPUVoices);

INCLUDE_ASM("asm/nonmatchings/sound", CalculateStereoVolume);

INCLUDE_ASM("asm/nonmatchings/sound", SetVoicePanning);

INCLUDE_ASM("asm/nonmatchings/sound", StartCDAudioForLevel);

INCLUDE_ASM("asm/nonmatchings/sound", StopCDStreaming);

INCLUDE_ASM("asm/nonmatchings/sound", SaveAndMuteAllVoicePitches);

INCLUDE_ASM("asm/nonmatchings/sound", ResumeAllVoicePitches);

INCLUDE_ASM("asm/nonmatchings/sound", SetStereoMode);

INCLUDE_ASM("asm/nonmatchings/sound", SetReverbLevel);

INCLUDE_ASM("asm/nonmatchings/sound", SetAudioVolume);

INCLUDE_ASM("asm/nonmatchings/sound", TickCDStreamBuffer);

