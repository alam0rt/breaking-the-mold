#include "common.h"

extern s32 CdControl(s32 cmd, void *param, void *result);
extern void CdFlush(void);
extern s32 D_8009B3D8[];
extern u8 D_8009B43C[];
/* gp_rel tentative defs (sdata blob owns the strong defs). */
u8 D_800A59E8;
u8 D_800A59E9;
u8 D_800A59EA;
u8 D_800A59EC;
s32 D_800A59F4;

INCLUDE_ASM("asm/nonmatchings/gamecd", LoadGameAssetLocations);

void func_80038B98(void) {
}

INCLUDE_ASM("asm/nonmatchings/gamecd", CdBLB_ReadSectors);

extern s32 CdReadFile(void *fp, void *buf, s32 nsector);
extern s32 CdReadSync(s32 mode, void *result);

s32 CdReadFileSync(void *fp, void *buf) {
    if (CdReadFile(fp, buf, 0) != 0) {
        return ((u32)~CdReadSync(0, NULL)) >> 31;
    }
    return 0;
}

s32 CdSetModeAndSeek(void) {
    s32 track = D_800A59EC;
    D_800A59F4 = D_8009B3D8[track];
    return CdControl(0x1B, &D_8009B43C[track * 24], NULL) == 1;
}

INCLUDE_ASM("asm/nonmatchings/gamecd", PlayCDAudioTrack);

s32 StopCDAudio(void) {
    if (D_800A59E8 == 0) return 0;
    CdControl(8, 0, 0);
    CdFlush();
    D_800A59E9 = 0;
}

s32 PauseCDAudio(void) {
    if (D_800A59E8 == 0) return 0;
    if (D_800A59EA != 0) return 1;
    CdControl(9, 0, 0);
    D_800A59EA = 1;
}

s32 ResumeCDAudio(void) {
    if (D_800A59E8 == 0) return 0;
    if (D_800A59EA == 0) return 1;
    CdControl(0x1B, 0, 0);
    D_800A59EA = 0;
}

INCLUDE_ASM("asm/nonmatchings/gamecd", ProcessCDStreamState);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdSeekWithRetry);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdPausePlayback);

