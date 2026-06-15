#include "common.h"

INCLUDE_ASM("asm/nonmatchings/sound", InitSPUDefaults);

INCLUDE_ASM("asm/nonmatchings/sound", UploadAudioToSPU);

INCLUDE_ASM("asm/nonmatchings/sound", PopSPUUploadBlock);

INCLUDE_ASM("asm/nonmatchings/sound", func_8007C324);

INCLUDE_ASM("asm/nonmatchings/sound", ShutdownSPUAndResetSoundState);

INCLUDE_ASM("asm/nonmatchings/sound", func_8007C35C);

INCLUDE_ASM("asm/nonmatchings/sound", func_8007C364);

INCLUDE_ASM("asm/nonmatchings/sound", SetGameMode);

INCLUDE_ASM("asm/nonmatchings/sound", PlaySoundEffect);

INCLUDE_ASM("asm/nonmatchings/sound", PlayGameSoundById);

INCLUDE_ASM("asm/nonmatchings/sound", StopSPUVoice);

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

