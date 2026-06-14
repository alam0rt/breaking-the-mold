#include "common.h"

extern u8 *D_800A55F4[];
extern void spu_Fw1ts(void);

void SpuSetVoiceVolume(s32 voice, s16 volL, s16 volR) {
    u16 *voiceRegs;

    volL &= 0x7FFF;
    volR &= 0x7FFF;
    voiceRegs = (u16 *)((voice << 4) + (s32)D_800A55F4[0]);

    voiceRegs[0] = volL;
    voiceRegs[1] = volR;
    __asm__ volatile ("" : : : "memory");
    spu_Fw1ts();
}

void SpuSetVoicePitch(s32 voice, u16 pitch) {
    u8 *voiceRegs;

    voiceRegs = (u8 *)((voice << 4) + (s32)D_800A55F4[0]);

    *(u16 *)(voiceRegs + 4) = pitch;
    __asm__ volatile ("" : : : "memory");
    spu_Fw1ts();
}

void SpuGetVoicePitch(s32 voice, u16 *pitch) {
    u8 *voiceRegs;

    voiceRegs = (u8 *)((voice << 4) + (s32)D_800A55F4[0]);

    *pitch = *(u16 *)(voiceRegs + 4);
}

void SpuSetCommonCDVolume(s16 left, s16 right) {
    u8 *spuBase = D_800A55F4[0];

    *(s16 *)(spuBase + 0x1B0) = left;
    *(s16 *)(spuBase + 0x1B2) = right;
    __asm__ volatile ("" : : : "memory");
}
