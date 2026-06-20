#ifndef GAME_BLB_H
#define GAME_BLB_H

#include "common.h"

/* =============================================================================
 * BLB Header (0x1000 bytes / 2 CD sectors)
 *
 * Loaded into blbHeaderBufferBase @ 0x800AE3E0 by LoadBLBHeader @ 0x800208B0
 * and pointed to by LevelDataContext.blb_header (kept as u32 for legacy
 * arithmetic in src/blbacc.c -- cast through BlbHeader* at use sites).
 *
 * Layout source: docs/blb/header.md, docs/blb/level-metadata.md,
 * docs/blb.hexpat. Only the fields actually read from C-decompiled call
 * sites are named; the rest of the 0x1000-byte window is preserved as
 * named padding so sizeof(BlbHeader) stays accurate.
 * ============================================================================= */

/* Per-level metadata entry (0x70 bytes). Array of 26 starts at BLB+0x000.
 * Fully described in docs/blb/level-metadata.md. */
typedef struct {
    /* 0x00 */ u16  primary_sector_offset;   /* CD sector where primary data starts */
    /* 0x02 */ u16  primary_sector_count;    /* Primary data sector count */
    /* 0x04 */ u32  primary_buffer_size;     /* Total primary buffer size (returned by GetPrimaryBufferSize) */
    /* 0x08 */ u32  entry1_offset;           /* Offset of Asset 601 within primary TOC */
    /* 0x0C */ u8   level_index;             /* Level asset index (0-25); matched against the world_id arg */
    /* 0x0D */ u8   password_selectable;     /* 1 = level chooseable via password screen */
    /* 0x0E */ u16  stage_count;             /* Number of stages in this level (1-6) */
    /* 0x10 */ u16  tert_data_off[6];        /* Per-stage tertiary data offsets (size = value << 5) */
    /* 0x1C */ u16  _pad1C;
    /* 0x1E */ u16  sec_sector_off[6];       /* Per-stage secondary sector offsets */
    /* 0x2A */ u16  _pad2A;
    /* 0x2C */ u16  sec_sector_cnt[6];       /* Per-stage secondary sector counts */
    /* 0x38 */ u16  _pad38;
    /* 0x3A */ u16  tert_sector_off[6];      /* Per-stage tertiary sector offsets */
    /* 0x46 */ u16  _pad46;
    /* 0x48 */ u16  tert_sector_cnt[6];      /* Per-stage tertiary sector counts */
    /* 0x54 */ u16  _pad54;
    /* 0x56 */ char level_id[5];             /* 4-char null-terminated short ID, e.g. "MENU" */
    /* 0x5B */ char name[21];                /* Level display name */
} BlbLevelEntry;  /* 0x70 = 112 bytes */

/* Movie table entry (0x1C bytes). Array of 13 starts at BLB+0xB60.
 * docs/blb/header.md. */
typedef struct {
    /* 0x00 */ u16  _reserved;               /* always 0 */
    /* 0x02 */ u16  sector_count;
    /* 0x04 */ char movie_id[5];             /* 4-char null-terminated, e.g. "INT1" */
    /* 0x09 */ char short_name[3];           /* 2-char short ID */
    /* 0x0C */ char iso_path[16];            /* "\\MVxxx.STR;1" */
} BlbMovieEntry;  /* 0x1C = 28 bytes */

/* Sector table entry (0x10 bytes). Array of 32 starts at BLB+0xCD0.
 * docs/blb/header.md.  Note the Mode-6 sector table at 0xECC overlaps
 * sectors[31].sector_offset/count -- that overlap is not modelled here. */
typedef struct {
    /* 0x00 */ u8   level_index;
    /* 0x01 */ u8   flags;                   /* 0x00=level, 0x03=game over, 0x05=special */
    /* 0x02 */ u8   display_timeout;         /* seconds */
    /* 0x03 */ char code[5];
    /* 0x08 */ char short_name[4];
    /* 0x0C */ u16  sector_offset;
    /* 0x0E */ u16  sector_count;
} BlbSectorEntry;  /* 0x10 = 16 bytes */

/* BLB header window (0x1000 bytes). Field offsets reproduce the layout
 * documented in docs/blb/header.md. The sequence_modes / sequence_targets
 * arrays at +0xF36 / +0xF92 are parallel; they are indexed by the playback
 * state offset stored in LevelDataContext.current_sequence_index. */
typedef struct {
    /* 0x000 */ BlbLevelEntry  levels[26];              /* 0xB60 bytes */
    /* 0xB60 */ BlbMovieEntry  movies[13];              /* 0x16C bytes (13 * 0x1C); ends at 0xCCC */
    /* 0xCCC */ u8   _padCCC[0xCD0 - 0xCCC];            /* 4 bytes of padding */
    /* 0xCD0 */ BlbSectorEntry sectors[32];             /* 0x200 bytes (32 * 0x10); ends at 0xED0 */
    /* 0xED0 */ u8   _padED0[0xF30 - 0xED0];            /* 0x60 bytes: Mode-6 sector table + credits + pad */
    /* 0xF30 */ u8   sequence_count;                    /* Length of the playback-sequence arrays */
    /* 0xF31 */ u8   level_count;                       /* Number of level metadata entries (always 26) */
    /* 0xF32 */ u8   movie_count;                       /* Number of movie table entries */
    /* 0xF33 */ u8   sector_table_count;                /* Number of sector table entries */
    /* 0xF34 */ u8   _padF34[2];
    /* 0xF36 */ u8   sequence_modes[0xF92 - 0xF36];     /* 0x5C bytes: per-step PlaybackMode */
    /* 0xF92 */ u8   sequence_targets[0x1000 - 0xF92];  /* 0x6E bytes: per-step level/credits/movie index */
} BlbHeader;  /* 0x1000 = 4096 bytes */

#endif /* GAME_BLB_H */
