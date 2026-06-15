#include "common.h"

extern void SpuSetKey(s32 onoff, s32 voiceBits);
extern void StopCDAudio(void);
extern void SpuQuit(void);
extern void SpuSetCommonCDVolume(s16 l, s16 r);
extern s32 PlaySoundEffect(s32 id, s32 a1, s32 a2);
extern void ProcessCDStreamState(void);
extern s32 D_8009CE64[];
/* Tentative defs to unlock gp_rel via maspsx --use-comm-section. */
u32 D_800A6074;
u32 D_800A6078;
u8  D_800A607E;
u8  D_800A607F;
u8  D_800A6080;
u8  D_800A6081;
u8  D_800A6082;
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

void SetGameMode(u8 mode) {
    if ((mode & 0xFF) < 7) {
        D_800A6082 = mode;
    }
}

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

void SetStereoMode(u8 mode) {
    if ((u8)(mode - 1) < 2) {
        D_800A607E = mode;
    }
}

INCLUDE_ASM("asm/nonmatchings/sound", SetReverbLevel);

void SetAudioVolume(u32 vol) {
    u32 v = vol & 0xFF;
    if (v < 5) {
        s32 scaled;
        D_800A6080 = (u8)vol;
        scaled = (s32)(v * 0x7FFF) / 4;
        SpuSetCommonCDVolume(scaled, scaled);
    }
}

void TickCDStreamBuffer(void) {
    if (D_800A6085 == 0) return;
    if ((D_800A6074++ & 3) == 0) {
        ProcessCDStreamState();
    }
}

