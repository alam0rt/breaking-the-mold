#ifndef TRIGGER_ZONE_H
#define TRIGGER_ZONE_H

#include "common.h"

/* =============================================================================
 * Trigger Zone System
 *
 * Spatial AABB triggers defined in level data (BLB asset, via
 * LevelDataContext+0x3C). The zone array pointer is stored at
 * GameState+0x74 (trigger_zone_data_ptr) and the count at GameState+0x78
 * (trigger_zone_count) by InitLayersAndTileState @ 0x80024778.
 *
 * Core test: CheckTriggerZoneCollision @ 0x800245BC iterates the array and
 * returns the first zone whose rectangle contains the query point
 * (left <= x < right, top <= y < bottom), copying attr_data/extra_data to
 * the caller. 10 call sites: EntityCheckTriggerZone @ 0x80056478,
 * FinnCheckTriggerZones @ 0x80070128, HandleGenericTriggerZone @ 0x8007EE6C,
 * PlatformEntityCheckTriggerZones @ 0x800722EC, UpdateCollectibleTriggerZone
 * @ 0x8003A9B4, etc. Used for doors, switches, warps, and hazards.
 * ============================================================================= */
typedef struct {
    /* 0x00 */ s16  left;        /* World X min */
    /* 0x02 */ s16  top;         /* World Y min */
    /* 0x04 */ s16  right;       /* World X max (exclusive) */
    /* 0x06 */ s16  bottom;      /* World Y max (exclusive) */
    /* 0x08 */ s32  attr_data;   /* Trigger type / attributes */
    /* 0x0C */ s32  extra_data;  /* Payload: target level, color, etc. */
} TriggerZone;  /* Size: 0x10 (16 bytes) */

/* -----------------------------------------------------------------------------
 * TriggerZoneRGB (3 bytes)
 *
 * RGB color triple used in arrays (20-22 entries) for per-zone and
 * per-layer color tinting.
 * ----------------------------------------------------------------------------- */
typedef struct {
    /* 0x00 */ u8  r;
    /* 0x01 */ u8  g;
    /* 0x02 */ u8  b;
} TriggerZoneRGB;  /* Size: 0x03 */

#endif /* TRIGGER_ZONE_H */
