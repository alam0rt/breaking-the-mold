#include "common.h"

extern void SpuSetKey(s32 onoff, s32 voiceBits);
extern void StopCDAudio(void);
extern void SpuQuit(void);
extern void SpuSetCommonCDVolume(s16 l, s16 r);
extern s32 PlaySoundEffect(s32 id, s32 a1, s32 a2);
extern void ProcessCDStreamState(void);
extern s32 GAME_SOUND_EFFECT_ID_TABLE[] asm("D_8009CE64");
/* Tentative defs to unlock gp_rel via maspsx --use-comm-section. */
u32 CD_STREAM_TICK_COUNTER asm("D_800A6074");
u32 SPU_UPLOAD_USED_BYTES asm("D_800A6078");
u8  STEREO_MODE_SETTING asm("D_800A607E");
u8  REVERB_LEVEL_SETTING asm("D_800A607F");
u8  AUDIO_VOLUME_SETTING asm("D_800A6080");
u8  SPU_UPLOAD_BLOCK_COUNT asm("D_800A6081");
u8  GAME_MODE_SETTING asm("D_800A6082");
u8  CD_STREAM_ACTIVE asm("D_800A6085");
u8  AUDIO_PITCHES_MUTED asm("D_800A6087");
u8  ACTIVE_SPU_VOICE_MASK asm("D_800A6088");

INCLUDE_ASM("asm/nonmatchings/sound", InitSPUDefaults);

INCLUDE_ASM("asm/nonmatchings/sound", UploadAudioToSPU);

extern u8 SPU_UPLOAD_BLOCK_TABLE[] asm("D_8009CFB0");

void PopSPUUploadBlock(void) {
    u8 cnt = SPU_UPLOAD_BLOCK_COUNT;
    if (cnt != 0) {
        u16 sz;
        cnt--;
        sz = *(u16 *)(SPU_UPLOAD_BLOCK_TABLE + (u8)cnt * 12);
        SPU_UPLOAD_BLOCK_COUNT = cnt;
        SPU_UPLOAD_USED_BYTES -= sz;
    }
}

void func_8007C324(void) {
    SPU_UPLOAD_USED_BYTES = 0;
    SPU_UPLOAD_BLOCK_COUNT = 0;
}

void ShutdownSPUAndResetSoundState(void) {
    SpuQuit();
    SPU_UPLOAD_USED_BYTES = 0;
    SPU_UPLOAD_BLOCK_COUNT = 0;
}

void func_8007C35C(void) {
}

void func_8007C364(void) {
}

void SetGameMode(u8 mode) {
    if ((mode & 0xFF) < 7) {
        GAME_MODE_SETTING = mode;
    }
}

INCLUDE_ASM("asm/nonmatchings/sound", PlaySoundEffect);

s32 PlayGameSoundById(u16 soundId, s32 channel) {
    if ((u16)(soundId - 0xC9) >= 0x1C) return -1;
    return PlaySoundEffect(GAME_SOUND_EFFECT_ID_TABLE[(u16)soundId], channel, 0);
}

void StopSPUVoice(s32 voice) {
    if (voice != -1) {
        SpuSetKey(0, 1 << voice);
    }
}

void StopAllSPUVoices(void) {
    SpuSetKey(0, 0xFFFFFF);
    ACTIVE_SPU_VOICE_MASK = 0;
}

INCLUDE_ASM("asm/nonmatchings/sound", CalculateStereoVolume);

extern void CalculateStereoVolume(s16 *out, s32 vol, s32 pan);
extern void SpuSetVoiceVolume(s32 voice, s16 volL, s16 volR);
extern u8 VOICE_SOUND_INDEX_TABLE[] asm("D_8009CC18");
extern u8 SOUND_DEFINITION_TABLE[] asm("D_8009CC68");

void SetVoicePanning(s32 voice_index, s16 pan_pos) {
    s16 vols[2];
    if ((u32)voice_index < 0x18) {
        CalculateStereoVolume(&vols[0],
                              *(s16 *)(SOUND_DEFINITION_TABLE + VOICE_SOUND_INDEX_TABLE[voice_index] * 12),
                              pan_pos);
        SpuSetVoiceVolume(voice_index, vols[0], vols[1]);
    }
}

INCLUDE_ASM("asm/nonmatchings/sound", StartCDAudioForLevel);

void StopCDStreaming(void) {
    StopCDAudio();
    CD_STREAM_ACTIVE = 0;
}

extern void SpuGetVoicePitch(s32 voice, u16 *out);
extern void SpuSetVoicePitch(s32 voice, u16 pitch);
extern void PauseCDAudio(void);
extern void ResumeCDAudio(void);
extern u16 SAVED_VOICE_PITCHES[] asm("D_8009CC30");

void SaveAndMuteAllVoicePitches(void) {
    s32 i;
    u16 *p;
    u16 saved;
    if (AUDIO_PITCHES_MUTED == 0) {
        i = 0;
        p = SAVED_VOICE_PITCHES;
        do {
            SpuGetVoicePitch(i, &saved);
            *p = saved;
            SpuSetVoicePitch(i, 0);
            i++;
            p++;
        } while (i < 0x18);
        PauseCDAudio();
        AUDIO_PITCHES_MUTED = 1;
    }
}

void ResumeAllVoicePitches(void) {
    s32 i;
    u16 *p;
    if (AUDIO_PITCHES_MUTED != 0) {
        i = 0;
        p = SAVED_VOICE_PITCHES;
        do {
            SpuSetVoicePitch(i, *p++);
            i++;
        } while (i < 0x18);
        ResumeCDAudio();
        AUDIO_PITCHES_MUTED = 0;
    }
}

void SetStereoMode(u8 mode) {
    if ((u8)(mode - 1) < 2) {
        STEREO_MODE_SETTING = mode;
    }
}

void SetReverbLevel(u8 level) {
    if ((u8)level < 5) {
        REVERB_LEVEL_SETTING = level;
    }
}

void SetAudioVolume(u32 vol) {
    u32 v = vol & 0xFF;
    if (v < 5) {
        s32 scaled;
        AUDIO_VOLUME_SETTING = (u8)vol;
        scaled = (s32)(v * 0x7FFF) / 4;
        SpuSetCommonCDVolume(scaled, scaled);
    }
}

void TickCDStreamBuffer(void) {
    if (CD_STREAM_ACTIVE == 0) return;
    if ((CD_STREAM_TICK_COUNTER++ & 3) == 0) {
        ProcessCDStreamState();
    }
}

