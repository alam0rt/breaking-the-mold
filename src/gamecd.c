#include "common.h"

extern void CdControl(s32 cmd, s32 a, s32 b);
extern void CdFlush(void);
/* gp_rel tentative defs (sdata blob owns the strong defs). */
u8 D_800A59E8;
u8 D_800A59E9;
u8 D_800A59EA;

INCLUDE_ASM("asm/nonmatchings/gamecd", LoadGameAssetLocations);

void func_80038B98(void) {
}

INCLUDE_ASM("asm/nonmatchings/gamecd", CdBLB_ReadSectors);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdReadFileSync);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdSetModeAndSeek);

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

