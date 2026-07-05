/* =============================================================================
 * LoadGameAssetLocations.c  --  PC replacement (TARGET_PC)
 * =============================================================================
 * On PSX (asm/nonmatchings/gamecd/LoadGameAssetLocations.s) this runs CdInit,
 * then CdSearchFile for the BLB data file + the CD-audio tracks on the ISO,
 * converting each found position to an LBA and storing it (BLB_BASE_SECTOR =
 * D_800A59F0, CD_TRACK_START_SECTOR_TABLE = D_8009B3D8, ...).
 *
 * The PC port does NOT use the ISO / a filesystem: cd_files.c serves GAME.BLB
 * DIRECTLY as a flat sector device whose sector 0 is the BLB header. So the
 * game's asset base is simply sector 0. This replacement therefore just:
 *   - initialises the CD backend (opens GAME.BLB),
 *   - sets BLB_BASE_SECTOR = 0,
 *   - leaves the CD-audio track tables zeroed (redbook audio is no-op'd).
 * The whole ISO-search machinery is intentionally dropped -- this is a HAL
 * replacement, not a faithful decompilation. See docs/plans/pc-port.md CP-1.3.
 * ========================================================================== */
#include "common.h"
#include "port_hal.h"

/* BLB_BASE_SECTOR: gamecd.c defines it as `s32 ... asm("D_800A59F0")`. */
extern s32 BLB_BASE_SECTOR asm("D_800A59F0");
extern int CdInit(void);

void LoadGameAssetLocations(void) {
    CdInit();                 /* cd_files.c: open GAME.BLB (idempotent)        */
    BLB_BASE_SECTOR = 0;      /* GAME.BLB served from its own sector 0         */
    port_cd_set_blb_base(0);  /* keep the CD backend's base in sync            */
}
