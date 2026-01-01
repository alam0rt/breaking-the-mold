/**
 * LoadGameAssetLocations.c - Locate GAME.BLB and MUSIC?.XA files on CD
 * 
 * Initialises the CD subsystem and searches for the archive files:
 *   \\GAME.BLB;1  - Main game data archive
 *   \\MUSIC1.XA;1 - \\MUSIC5.XA;1  - XA audio streams
 * 
 * For each file found the starting sector is recorded so that subsequent
 * reads (via CdBLB_ReadSectors, CdMusic_* helpers) can use offsets relative
 * to the file start.
 * 
 * NON-MATCHING: GP-relative stores for sdata globals.
 */

#include "common.h"

/* Rodata strings (disc paths) ---------------------------------------------- */
extern const char D_80010BE8[];  /* "\\GAME.BLB;1" */
extern const char D_80010BF4[];  /* "\\MUSIC1.XA;1" */
extern const char D_80010C04[];  /* "\\MUSIC2.XA;1" */
extern const char D_80010C14[];  /* "\\MUSIC3.XA;1" */
extern const char D_80010C24[];  /* "\\MUSIC4.XA;1" */
extern const char D_80010C34[];  /* "\\MUSIC5.XA;1" */

/* Data / sdata globals ----------------------------------------------------- */
extern CdlFILE g_GameBLBFile;
extern CdlFILE g_Music1File;   /* also g_Music2File..g_Music5File follow at +0x18 each */

extern s32 g_GameBLBSector;    /* sdata: starting sector of GAME.BLB */
extern s32 g_Music1Sector;     /* sdata: starting sector of MUSIC1.XA */
extern s32 g_Music2Sector;
extern s32 g_Music3Sector;
extern s32 g_Music4Sector;
extern s32 g_Music5Sector;
extern u8  g_AllMusicFilesFound; /* sdata: set to 1 when all 5 music files located */

/* ========================================================================== */
#ifdef NON_MATCHING
s32 LoadGameAssetLocations(void) {
    CdlFILE *musicFile;
    u8 foundMusic1 = 0;
    u8 foundMusic2 = 0;
    u8 foundMusic3 = 0;
    u8 foundMusic4 = 0;
    u8 foundMusic5 = 0;

    CdInit();

    /* Locate GAME.BLB ---------------------------------------------------- */
    if (CdSearchFile(&g_GameBLBFile, D_80010BE8) == 0) {
        return 0;  /* fatal: main archive missing */
    }
    g_GameBLBSector = CdPosToInt(&g_GameBLBFile);

    /* Locate MUSIC1.XA --------------------------------------------------- */
    musicFile = &g_Music1File;
    if (CdSearchFile(musicFile, D_80010BF4)) {
        foundMusic1 = 1;
        g_Music1Sector = CdPosToInt(musicFile);
    }
    musicFile = (CdlFILE *)((u8 *)&g_Music1File + 0x18);

    /* Locate MUSIC2.XA --------------------------------------------------- */
    if (CdSearchFile(musicFile, D_80010C04)) {
        foundMusic2 = 1;
        g_Music2Sector = CdPosToInt(musicFile);
    }
    musicFile = (CdlFILE *)((u8 *)&g_Music1File + 0x30);

    /* Locate MUSIC3.XA --------------------------------------------------- */
    if (CdSearchFile(musicFile, D_80010C14)) {
        foundMusic3 = 1;
        g_Music3Sector = CdPosToInt(musicFile);
    }
    musicFile = (CdlFILE *)((u8 *)&g_Music1File + 0x48);

    /* Locate MUSIC4.XA --------------------------------------------------- */
    if (CdSearchFile(musicFile, D_80010C24)) {
        foundMusic4 = 1;
        g_Music4Sector = CdPosToInt(musicFile);
    }
    musicFile = (CdlFILE *)((u8 *)&g_Music1File + 0x60);

    /* Locate MUSIC5.XA --------------------------------------------------- */
    if (CdSearchFile(musicFile, D_80010C34)) {
        foundMusic5 = 1;
        g_Music5Sector = CdPosToInt(musicFile);
    }

    /* All music files present? ------------------------------------------- */
    if ((foundMusic1 & 0xFF) &&
        (foundMusic2 & 0xFF) &&
        (foundMusic3 & 0xFF) &&
        (foundMusic4 & 0xFF) &&
        (foundMusic5 & 0xFF)) {
        g_AllMusicFilesFound = 1;
    }

    return 1;
}
#else
INCLUDE_ASM("asm/pal/nonmatchings/LoadGameAssetLocations", LoadGameAssetLocations);
#endif
