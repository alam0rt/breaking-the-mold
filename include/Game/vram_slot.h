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

#endif /* VRAM_SLOT_H */
