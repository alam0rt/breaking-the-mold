#include "common.h"

typedef struct {
    u8 minute;
    u8 second;
    u8 sector;
    u8 track;
} CdlLOC;

extern s32 CdControl(s32 cmd, u8 *param, u8 *result);
extern void CdFlush(void);
extern CdlLOC *CdIntToPos(s32 i, CdlLOC *p);
extern s32 CdRead(s32 sectors, u8 *buf, s32 mode);
extern s32 CdReadSync(s32 mode, u8 *result);
extern s32 CD_TRACK_START_SECTOR_TABLE[] asm("D_8009B3D8");
extern u8 CD_TRACK_LOCATION_TABLE[] asm("D_8009B43C");
/* gp_rel tentative defs (sdata blob owns the strong defs). */
u8 CD_AUDIO_ENABLED asm("D_800A59E8");
u8 CD_AUDIO_PLAYING asm("D_800A59E9");
u8 CD_AUDIO_PAUSED asm("D_800A59EA");
u8 CD_CURRENT_TRACK asm("D_800A59EC");
s32 BLB_BASE_SECTOR asm("D_800A59F0");
s32 CD_CURRENT_TRACK_START_SECTOR asm("D_800A59F4");

INCLUDE_ASM("asm/nonmatchings/gamecd", LoadGameAssetLocations);

void func_80038B98(void) {
}

/* Reads `num_sectors` raw sectors from the BLB asset table at
 * (BLB_BASE_SECTOR + sector_offset) into `buffer`. Used by the BLB
 * loader to stream a section of the asset table. Always returns 1. */
s32 CdBLB_ReadSectors(u16 sector_offset, u16 num_sectors, u8 *buffer) {
    CdlLOC pos;
    CdIntToPos(BLB_BASE_SECTOR + sector_offset, &pos);
    CdControl(2, (u8 *)&pos, 0);
    CdRead(num_sectors, buffer, 0x80);
    CdReadSync(0, 0);
    return 1;
}

extern s32 CdReadFile(u8 *fp, u8 *buf, s32 nsector);
extern s32 CdReadSync(s32 mode, u8 *result);

s32 CdReadFileSync(u8 *fp, u8 *buf) {
    if (CdReadFile(fp, buf, 0) != 0) {
        return ((u32)~CdReadSync(0, NULL)) >> 31;
    }
    return 0;
}

s32 CdSetModeAndSeek(void) {
    s32 track = CD_CURRENT_TRACK;
    CD_CURRENT_TRACK_START_SECTOR = CD_TRACK_START_SECTOR_TABLE[track];
    return CdControl(0x1B, &CD_TRACK_LOCATION_TABLE[track * 24], NULL) == 1;
}

INCLUDE_ASM("asm/nonmatchings/gamecd", PlayCDAudioTrack);

s32 StopCDAudio(void) {
    if (CD_AUDIO_ENABLED == 0) return 0;
    CdControl(8, 0, 0);
    CdFlush();
    CD_AUDIO_PLAYING = 0;
}

s32 PauseCDAudio(void) {
    if (CD_AUDIO_ENABLED == 0) return 0;
    if (CD_AUDIO_PAUSED != 0) return 1;
    CdControl(9, 0, 0);
    CD_AUDIO_PAUSED = 1;
}

s32 ResumeCDAudio(void) {
    if (CD_AUDIO_ENABLED == 0) return 0;
    if (CD_AUDIO_PAUSED == 0) return 1;
    CdControl(0x1B, 0, 0);
    CD_AUDIO_PAUSED = 0;
}

INCLUDE_ASM("asm/nonmatchings/gamecd", ProcessCDStreamState);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdSeekWithRetry);

INCLUDE_ASM("asm/nonmatchings/gamecd", CdPausePlayback);

