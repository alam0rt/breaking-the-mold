/**
 * CdBLB.c - CD/BLB Loading Subsystem
 * 
 * This module handles reading data from the GAME.BLB file and streaming
 * XA audio from MUSIC1-5.XA files on the CD-ROM.
 * 
 * These functions use GP-relative (sdata) globals which cannot be matched
 * with GCC 2.7.2, so INCLUDE_ASM is used for matching builds.
 * 
 * CD Command codes used:
 *   0x02 = CdlSetloc   - Set read position
 *   0x08 = CdlStop     - Stop motor
 *   0x09 = CdlPause    - Pause reading
 *   0x0D = CdlSetfilter- Set XA filter
 *   0x0E = CdlSetmode  - Set mode
 *   0x11 = CdlGetlocP  - Get physical position
 *   0x15 = CdlSeekL    - Seek (data mode)
 *   0x1B = CdlReadS    - Read with retry
 */

#include "common.h"

/* PSY-Q CD functions are declared in LIBCD.H */

/* sdata globals - GP-relative access prevents matching */
extern s32 g_GameBLBSector;
extern s32 g_Music1Sector;
extern s32 g_MusicCurrentSector;
extern s32 g_MusicTargetSector;
extern u8 g_MusicFileIndex;
extern u8 g_MusicState;
extern u8 g_MusicPaused;
extern u8 g_MusicRetryCount;
extern u8 g_AllMusicFilesFound;
extern u8 D_800A5A04;
extern u8 D_800A5A0C;
extern u8 D_800A5A0D;
extern void *D_800A59FC;
extern void *g_Music1File;
extern u16 D_8009B3EC[];

/* ============================================================================
 * CdBLB_Stub - Empty stub function
 * Address: 0x80038B98 | Size: 0x8
 * ============================================================================ */
#ifdef NON_MATCHING
void CdBLB_Stub(void) {
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdBLB_Stub);
#endif

/* ============================================================================
 * CdBLB_ReadSectors - Read sectors from GAME.BLB
 * Address: 0x80038BA0 | Size: 0x74
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdBLB_ReadSectors(s32 sectorOffset, s32 numSectors, void *destBuffer) {
    u8 pos[8];  /* CdlLOC */
    CdIntToPos(g_GameBLBSector + (sectorOffset & 0xFFFF), pos);
    CdControlF(0x02, pos, 0);  /* CdlSetloc */
    CdRead(numSectors & 0xFFFF, destBuffer, 0x80);
    CdReadSync(0, 0);
    return 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdBLB_ReadSectors);
#endif

/* ============================================================================
 * CdBLB_WaitReadComplete - Wait for CD read to complete
 * Address: 0x80038C14 | Size: 0x40
 * ============================================================================ */
#ifdef NON_MATCHING
u32 CdBLB_WaitReadComplete(void) {
    if (CdDataSync(0) == 0) {
        return 0;
    }
    return (u32)~CdReadSync(0, 0) >> 31;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdBLB_WaitReadComplete);
#endif

/* ============================================================================
 * CdMusic_SetupPosition - Setup music file read position
 * Address: 0x80038C54 | Size: 0x58
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdMusic_SetupPosition(void) {
    g_MusicCurrentSector = (&g_Music1Sector)[g_MusicFileIndex];
    return CdControlF(0x1B, (u8*)&g_Music1File + (g_MusicFileIndex * 0x18), 0) == 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_SetupPosition);
#endif

/* ============================================================================
 * CdMusic_Start - Start XA music playback
 * Address: 0x80038CAC | Size: 0x160
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdMusic_Start(u8 fileIndex, s8 channel) {
    s32 retries;
    u16 sectorOffset;
    s32 targetOffset;
    
    if (g_AllMusicFilesFound == 0) return 0;
    
    g_MusicFileIndex = fileIndex;
    sectorOffset = D_8009B3EC[(fileIndex * 8) + channel];
    targetOffset = (channel == 0) ? sectorOffset - 3 : sectorOffset - 2;
    
    D_800A5A04 = 0xC0;  /* CD mode: speed + XA-ADPCM */
    g_MusicTargetSector = (&g_Music1Sector)[fileIndex] + (targetOffset * 8);
    CdControlF(0x0E, &D_800A5A04, 0);  /* CdlSetmode */
    
    for (retries = 8; retries > 0; retries--) {
        g_MusicCurrentSector = (&g_Music1Sector)[g_MusicFileIndex];
        if (CdControlF(0x1B, (u8*)&g_Music1File + (g_MusicFileIndex * 0x18), 0) == 1) {
            D_800A5A04 = 0xC8;  /* realtime mode */
            CdControlF(0x0E, &D_800A5A04, 0);
            D_800A5A0C = 1;
            D_800A5A0D = channel;
            CdControlF(0x0D, &D_800A5A0C, &D_800A59FC);
            g_MusicState = 1;
            return 1;
        }
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Start);
#endif

/* ============================================================================
 * CdMusic_Stop - Stop XA music playback
 * Address: 0x80038E0C | Size: 0x44
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdMusic_Stop(void) {
    if (g_AllMusicFilesFound == 0) return 0;
    CdControlF(0x08, 0, 0);  /* CdlStop */
    CdCallback();
    g_MusicState = 0;
    return 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Stop);
#endif

/* ============================================================================
 * CdMusic_Pause - Pause XA music playback
 * Address: 0x80038E50 | Size: 0x50
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdMusic_Pause(void) {
    if (g_AllMusicFilesFound == 0) return 0;
    if (g_MusicPaused == 0) {
        CdControlF(0x09, 0, 0);  /* CdlPause */
        g_MusicPaused = 1;
    }
    return 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Pause);
#endif

/* ============================================================================
 * CdMusic_Resume - Resume XA music playback
 * Address: 0x80038EA0 | Size: 0x50
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdMusic_Resume(void) {
    if (g_AllMusicFilesFound == 0) return 0;
    if (g_MusicPaused == 0) return 1;
    CdControlF(0x1B, 0, 0);  /* CdlReadS */
    g_MusicPaused = 0;
    return 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Resume);
#endif

/* ============================================================================
 * CdMusic_Update - Music streaming state machine
 * Address: 0x80038EF0 | Size: 0x1A4
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdMusic_Update(void) {
    u8 loc[8];
    s32 status;
    s32 sector;
    
    if (g_MusicPaused) {
        CdControlF(0x09, 0, 0);
        return 1;
    }
    if (!g_MusicState || !g_AllMusicFilesFound) return 1;
    
    status = CdStatus(1, loc);
    switch (status) {
        case 2:  /* Playing */
            if (g_MusicTargetSector < g_MusicCurrentSector) {
                /* Loop back to start */
                g_MusicCurrentSector = (&g_Music1Sector)[g_MusicFileIndex];
                CdControlF(0x1B, (u8*)&g_Music1File + (g_MusicFileIndex * 0x18), 0);
                g_MusicRetryCount = 0;
            } else {
                /* Update position */
                if (CdGetlocP(loc) == 0x11) {
                    sector = CdPosToInt(loc);
                    if (sector > 0) g_MusicCurrentSector = sector;
                }
                CdControlB(0x11, 0);
            }
            break;
        case 5:  /* Error */
            if (++g_MusicRetryCount >= 6) {
                g_MusicCurrentSector = (&g_Music1Sector)[g_MusicFileIndex];
                CdControlF(0x1B, (u8*)&g_Music1File + (g_MusicFileIndex * 0x18), 0);
                g_MusicRetryCount = 0;
            }
            return 0;
    }
    return 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdMusic_Update);
#endif

/* ============================================================================
 * CdSeekWithRetry - Seek to position with retry loop
 * Address: 0x80039094 | Size: 0x94
 * ============================================================================ */
#ifdef NON_MATCHING
s32 CdSeekWithRetry(void *pos) {
    s32 outer, inner;
    
    for (outer = 8; outer > 0; outer--) {
        for (inner = 8; inner > 0; inner--) {
            if (CdControlF(0x15, pos, 0)) goto seek_ok;  /* CdlSeekL */
        }
        return 0;
    seek_ok:
        if (CdSync(0x1E0) != 0) return 1;
    }
    return 0;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdSeekWithRetry);
#endif

/* ============================================================================
 * CdResetFilter - Reset CD XA filter
 * Address: 0x80039128 | Size: 0x28
 * ============================================================================ */
#ifdef NON_MATCHING
void CdResetFilter(void) {
    CdControlF(0x09, 0, 0);  /* CdlPause */
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/CdBLB", CdResetFilter);
#endif
