/* =============================================================================
 * cd_files.c  --  PC-port CD replacement: PSX CD-ROM -> GAME.BLB (TARGET_PC)
 * =============================================================================
 * Replaces the PlayStation CD-ROM. The game streams ALL of its data out of one
 * file, GAME.BLB, via absolute-sector reads relative to BLB_BASE_SECTOR:
 *
 *     CdIntToPos(BLB_BASE_SECTOR + sector_offset, &pos);   // gamecd.c
 *     CdControl(CdlSetloc=2, &pos, 0);                     // latch the LBA
 *     CdRead(num_sectors, buffer, mode);                  // read the sectors
 *     CdReadSync(0, 0);                                   // wait (synchronous)
 *
 * On PC we do NOT parse the ISO. We open GAME.BLB directly and treat it as a
 * flat array of 2048-byte data sectors: sector 0 = start of GAME.BLB. The PC
 * boot sets BLB_BASE_SECTOR = 0 (the file starts at its own sector 0), so the
 * latched LBA IS the GAME.BLB sector index. The whole BLB asset parser
 * (blbacc.c / blbmem.c / lvlload.c) then runs unchanged on the bytes we feed it.
 *
 * CD-audio (Redbook music) commands are accepted and no-op'd for now.
 * See docs/plans/pc-port.md CP-1.3.
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psyq_pc.h"
#include "port_runtime.h"
#include "port_hal.h"

#define CD_SECTOR_BYTES 2048
#define CD_PREGAP_FRAMES 150   /* 2-second lead-in: LBA 0 == MSF 00:02:00 */

/* PSX CdlLOC / CdlFILE. psyq_pc.h intentionally omits these (src/gamecd.c has
 * its own local CdlLOC typedef, and a header copy would clash in that TU), so
 * the CD backend defines them here. CdlLOC matches gamecd.c's layout exactly
 * (u8 minute, second, sector, track) so pointers pass through unchanged. */
typedef struct { unsigned char minute, second, sector, track; } CdlLOC;
typedef struct { CdlLOC pos; u_long size; char name[16]; } CdlFILE;
typedef CdlLOC PortCdlLOC;

/* PSX Cd command codes we care about (BIOS CdlXxx). */
enum {
    CdlSetloc  = 0x02,
    CdlPlay    = 0x03,
    CdlReadN   = 0x06,
    CdlStop    = 0x08,
    CdlPause   = 0x09,
    CdlInit    = 0x0A,
    CdlSetmode = 0x0E,
    CdlSeekL   = 0x15,
    CdlReadS   = 0x1B,
};

static FILE *s_blb;
static long  s_blb_sectors;
static int   s_latched_lba;    /* set by CdControl(CdlSetloc)               */
static int   s_blb_base;       /* subtracted from latched LBA (default 0)   */
static unsigned char s_mode;

/* GAME.BLB search order (PAL build first). Override with SKULLMONKEYS_BLB. */
static const char *k_blb_candidates[] = {
    "disks/blb/GAME.BLB",
    "disks/blb/sles-01090.blb",
    "GAME.BLB",
    "disks/prototype/ext/GAME.BLB",
};

/* ---- init + core sector read (independently testable) -------------------- */

int port_cd_read_sectors(int lba, int nsectors, void *buf) {
    long off;
    size_t want, got;
    if (!s_blb || nsectors <= 0 || !buf) {
        return -1;
    }
    off = (long)(lba - s_blb_base) * CD_SECTOR_BYTES;
    if (off < 0 || (long)(lba - s_blb_base + nsectors) > s_blb_sectors) {
        port_log("cd: read out of range (lba=%d n=%d base=%d, blb=%ld sectors)",
                 lba, nsectors, s_blb_base, s_blb_sectors);
        return -1;
    }
    if (fseek(s_blb, off, SEEK_SET) != 0) {
        return -1;
    }
    want = (size_t)nsectors * CD_SECTOR_BYTES;
    got = fread(buf, 1, want, s_blb);
    return (got == want) ? 0 : -1;
}

void port_cd_set_blb_base(int base_lba) {
    s_blb_base = base_lba;
}

int port_cd_init(void) {
    const char *env = getenv("SKULLMONKEYS_BLB");
    const char *path = NULL;
    size_t i;
    long size;
    unsigned char magic[CD_SECTOR_BYTES];

    if (env && env[0]) {
        path = env;
    } else {
        for (i = 0; i < sizeof(k_blb_candidates) / sizeof(k_blb_candidates[0]); i++) {
            FILE *f = fopen(k_blb_candidates[i], "rb");
            if (f) {
                fclose(f);
                path = k_blb_candidates[i];
                break;
            }
        }
    }
    if (!path) {
        port_log("cd: GAME.BLB not found (set SKULLMONKEYS_BLB)");
        return -1;
    }

    s_blb = fopen(path, "rb");
    if (!s_blb) {
        port_log("cd: cannot open %s", path);
        return -1;
    }
    fseek(s_blb, 0, SEEK_END);
    size = ftell(s_blb);
    fseek(s_blb, 0, SEEK_SET);
    s_blb_sectors = size / CD_SECTOR_BYTES;
    s_blb_base = 0;
    s_latched_lba = 0;

    /* Self-test: sector 0 must start with the BLB header magic (0x002000C9 LE
     * for the PAL build). Soft check -- warn but proceed for other regions. */
    if (port_cd_read_sectors(0, 1, magic) != 0) {
        port_log("cd: sector-0 read failed on %s", path);
        return -1;
    }
    fseek(s_blb, 0, SEEK_SET);
    port_log("cd: GAME.BLB = %s (%ld sectors, magic %02x%02x%02x%02x)",
             path, s_blb_sectors, magic[0], magic[1], magic[2], magic[3]);
    if (!(magic[0] == 0xC9 && magic[1] == 0x00 && magic[2] == 0x20)) {
        port_log("cd: WARNING unexpected BLB magic (non-PAL build?)");
    }
    return 0;
}

/* ---- BCD MSF <-> LBA ----------------------------------------------------- */

static unsigned char to_bcd(int v) { return (unsigned char)(((v / 10) << 4) | (v % 10)); }
static int from_bcd(unsigned char b) { return ((b >> 4) * 10) + (b & 0x0F); }

CdlLOC *CdIntToPos(int i, CdlLOC *p) {
    PortCdlLOC *loc = (PortCdlLOC *)p;
    int f = i + CD_PREGAP_FRAMES;
    if (loc) {
        loc->minute = to_bcd(f / (75 * 60));
        loc->second = to_bcd((f / 75) % 60);
        loc->sector = to_bcd(f % 75);
        loc->track  = 0;
    }
    return p;
}

int CdPosToInt(CdlLOC *p) {
    PortCdlLOC *loc = (PortCdlLOC *)p;
    if (!loc) {
        return 0;
    }
    return from_bcd(loc->minute) * 75 * 60
         + from_bcd(loc->second) * 75
         + from_bcd(loc->sector)
         - CD_PREGAP_FRAMES;
}

/* ---- Cd command surface -------------------------------------------------- */

int CdInit(void) {
    return (s_blb != NULL) ? 0 : port_cd_init();
}

int CdControl(u_char com, u_char *param, u_char *result) {
    (void)result;
    switch (com) {
    case CdlSetloc:
        if (param) {
            s_latched_lba = CdPosToInt((CdlLOC *)param);
        }
        break;
    case CdlSetmode:
        if (param) {
            s_mode = param[0];
        }
        break;
    case CdlSeekL:
    case CdlReadN:
    case CdlReadS:
    case CdlPlay:
    case CdlStop:
    case CdlPause:
    case CdlInit:
        /* Seeks are latched via Setloc; audio commands are no-op'd for now. */
        break;
    default:
        break;
    }
    return 1;
}

int CdControlB(u_char com, u_char *param, u_char *result) {
    return CdControl(com, param, result);
}

int CdControlF(u_char com, u_char *param) {
    return CdControl(com, param, NULL);
}

/* Synchronous read: perform it immediately from the latched LBA; CdReadSync
 * then reports "done". mode's double-speed / sector-size bits are irrelevant
 * on the host filesystem. */
int CdRead(int sectors, u_long *buf, int mode) {
    s_mode = (unsigned char)mode;
    if (port_cd_read_sectors(s_latched_lba, sectors, buf) != 0) {
        return 0;   /* 0 == failed to start (matches CdRead's error contract) */
    }
    s_latched_lba += sectors;   /* advance like the real drive does */
    return 1;
}

int CdReadSync(int mode, u_char *result) {
    (void)mode; (void)result;
    return 0;   /* 0 == read complete (blocking mode) */
}

int CdSync(int mode, u_char *result) {
    (void)mode; (void)result;
    return 0;   /* 0 == command complete / drive ready */
}

int CdGetSector(void *madr, int size) {
    /* Read `size` 32-bit words (size*4 bytes) from the latched position. */
    int nbytes = size * 4;
    int nsectors = (nbytes + CD_SECTOR_BYTES - 1) / CD_SECTOR_BYTES;
    return (port_cd_read_sectors(s_latched_lba, nsectors, madr) == 0);
}

int CdFlush(void) {
    return 1;
}

/* Named-file access (ISO9660) is not used on the direct-BLB path; the game
 * streams everything by sector. Provide safe negatives so any incidental
 * caller degrades gracefully rather than reading garbage. */
CdlFILE *CdSearchFile(CdlFILE *fp, char *name) {
    (void)fp;
    port_log("cd: CdSearchFile(%s) unsupported on direct-BLB backend", name ? name : "?");
    return NULL;
}

int CdReadFile(u_char *fp, u_char *buf, int nsector) {
    (void)fp; (void)buf; (void)nsector;
    port_log("cd: CdReadFile unsupported on direct-BLB backend");
    return 0;
}

/* Callback registrars: no async completion events on the synchronous backend. */
void *CdDataCallback(void (*func)(int, u_char *)) { (void)func; return 0; }
void *CdReadCallback(void (*func)(int))           { (void)func; return 0; }
void *CdReadyCallback(void (*func)(int, u_char *)) { (void)func; return 0; }
