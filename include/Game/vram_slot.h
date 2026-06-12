#ifndef VRAM_SLOT_H
#define VRAM_SLOT_H

#include "common.h"

/* =============================================================================
 * VRAMSlotConfig (12 bytes)
 *
 * Asset 101 (0x65): VRAM texture bank allocation configuration.
 * Loaded into LevelDataContext+0x08 (vram_slot_config).
 *
 * Specifies how many VRAM texture pages are reserved for each bank.
 * Bank A holds primary tile/sprite textures; Bank B holds secondary data.
 * ============================================================================= */
typedef struct {
    /* 0x00 */ u32  bankACount;   /* Number of VRAM slots in texture bank A */
    /* 0x04 */ u32  bankBCount;   /* Number of VRAM slots in texture bank B */
    /* 0x08 */ u32  reserved08;   /* Reserved / unused */
} VRAMSlotConfig;  /* Size: 0x0C (12 bytes) */

/* =============================================================================
 * VRAMSlotXY (8 bytes)
 *
 * Constant VRAM placement entry used by InitVRAMSlotTable @ 0x80013B1C.
 * 3-entry table g_VRAMSlotXYTable @ 0x8009AE40:
 *   { 7, 0x300 }, { 7, 0x200 }, { 6, 0x1F0 }
 * ============================================================================= */
typedef struct {
    /* 0x00 */ u32  x_coord;        /* VRAM X coordinate (page units) */
    /* 0x04 */ u32  clut_or_tpage;  /* CLUT / texture page value */
} VRAMSlotXY;  /* Size: 0x08 */

#endif /* VRAM_SLOT_H */
