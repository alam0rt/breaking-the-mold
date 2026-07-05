/* =============================================================================
 * ProcessCDStreamState.c  --  PC-port CD-DA music streaming state poll
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c ProcessCDStreamState
 * (0x8007xxxx, INCLUDE_ASM in src/gamecd.c). Polled every frame by the sound
 * tick to keep a looping CD-audio track playing: when a track is active it
 * watches CdSync, and on completion either re-issues a report (CdControlF 0x11)
 * or re-seeks/plays the track (CdControl 0x1B) once its retry budget lapses.
 *
 * At boot / on the title screen all three gate flags (CD_AUDIO_PAUSED,
 * CD_AUDIO_PLAYING, CD_AUDIO_ENABLED) are zero, so this returns 1 immediately
 * without touching the drive -- CD-DA music is a later milestone. The full state
 * machine is transcribed for fidelity.
 *
 * State globals (named in src/gamecd.c): CD_AUDIO_ENABLED @0x800A59E8,
 * CD_AUDIO_PLAYING @0x800A59E9, CD_AUDIO_PAUSED @0x800A59EA,
 * CD_CURRENT_TRACK @0x800A59EC, CD_CURRENT_TRACK_START_SECTOR @0x800A59F4.
 * Retry counter @0x800A59EB and the target-sector compare value @0x800A59F8 are
 * only referenced here; the per-track start-sector table (s32[] @0x8009B3D8) and
 * the per-track CdlLOC seek args (0x18-byte records @0x8009B43C) are read only
 * when a track is active.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern u8  CD_AUDIO_ENABLED  asm("D_800A59E8");
extern u8  CD_AUDIO_PLAYING  asm("D_800A59E9");
extern u8  CD_AUDIO_PAUSED   asm("D_800A59EA");
extern u8  CD_AUDIO_RETRIES  asm("D_800A59EB");
extern u8  CD_CURRENT_TRACK  asm("D_800A59EC");
extern s32 CD_TRACK_SECTOR   asm("D_800A59F4");
extern s32 CD_TRACK_SECTOR_END asm("D_800A59F8");
extern s32 CD_TRACK_START_TABLE[] asm("D_8009B3D8");   /* s32 per track */
extern u8  CD_TRACK_SEEK_ARGS[] asm("D_8009B43C");     /* 0x18-byte records */

extern s32 CdSync(s32 mode, u8 *result);
extern u8  CdLastCom(void);
extern int CdPosToInt(void *loc);
extern int CdControlF(u_char com, u_char *param);
extern int CdControl(u_char com, u_char *param, u_char *result);

unsigned int ProcessCDStreamState(void) {
    u8 result[8];

    if (CD_AUDIO_PAUSED == 0) {
        if (CD_AUDIO_PLAYING == 0) {
            return 1;
        }
        if (CD_AUDIO_ENABLED != 0) {
            s32 sync = CdSync(1, result);
            if (sync == 2) {
                if (CD_TRACK_SECTOR <= CD_TRACK_SECTOR_END) {
                    int pos = 0;
                    int advanced = 0;
                    if (CdLastCom() == 0x11) {
                        pos = CdPosToInt(result + 5);
                        advanced = (0 < pos);
                    }
                    if (advanced) {
                        CD_TRACK_SECTOR = pos;
                    }
                    return (unsigned int)CdControlF(0x11, (u_char *)0);
                }
                CD_TRACK_SECTOR = CD_TRACK_START_TABLE[CD_CURRENT_TRACK];
                {
                    unsigned int r = (unsigned int)CdControl(
                        0x1B, &CD_TRACK_SEEK_ARGS[(unsigned int)CD_CURRENT_TRACK * 0x18],
                        (u_char *)0);
                    CD_AUDIO_RETRIES = 0;
                    return r;
                }
            }
            if (sync < 3) {
                return (unsigned int)(sync < 3);
            }
            if (sync != 5) {
                return 5;
            }
            CD_AUDIO_RETRIES = CD_AUDIO_RETRIES + 1;
            if (CD_AUDIO_RETRIES < 6) {
                return 0;
            }
            CD_TRACK_SECTOR = CD_TRACK_START_TABLE[CD_CURRENT_TRACK];
            CdControl(0x1B, &CD_TRACK_SEEK_ARGS[(unsigned int)CD_CURRENT_TRACK * 0x18],
                      (u_char *)0);
            CD_AUDIO_RETRIES = 0;
            return 0;
        }
    } else {
        CdControl(9, (u_char *)0, (u_char *)0);
    }
    return 1;
}
