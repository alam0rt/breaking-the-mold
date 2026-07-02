#ifndef ENTINIT_ENTITIES_H
#define ENTINIT_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * ENTITY-INIT / SPAWN OVERLAY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These are overlay views used by the per-type entity initializers and the
 * spawn/ordering-table setup in src/entinit.c; padding runs cover offsets not
 * yet traced. Used exclusively by src/entinit.c.
 * ============================================================================= */

typedef struct LevelFlagsOwner {
    /* 0x00 */ u8 pad0[0x84];
    /* 0x84 */ u8 levelFlagsContext[1];
} LevelFlagsOwner;

typedef struct PlayerVehicleLink {
    /* 0x000 */ u8 pad0[0x100];
    /* 0x100 */ s32 speed;
} PlayerVehicleLink;

typedef struct PlayerVehicleHud {
    /* 0x00 */ u8 pad0[0x1C];
    /* 0x1C */ s32 speed;
} PlayerVehicleHud;

typedef struct PlayerVehicleEntity {
    /* 0x000 */ u8 pad0[0x2C];
    /* 0x02C */ PlayerVehicleLink *vehicle;
    /* 0x030 */ u8 pad30[0x54];
    /* 0x084 */ u8 levelFlagsContext[1];
    /* 0x085 */ u8 pad85[0xBB];
    /* 0x140 */ s32 speed;
    /* 0x144 */ u8 pad144[8];
    /* 0x14C */ PlayerVehicleHud *hud;
} PlayerVehicleEntity;

typedef struct AlternateEntitySlot {
    /* 0x00 */ u8 pad0[0x3C];
    /* 0x3C */ s32 spawned;
} AlternateEntitySlot;

typedef struct AlternateEntityState {
    /* 0x000 */ u8 pad0[0x164];
    /* 0x164 */ AlternateEntitySlot *slots;
    /* 0x168 */ u16 count;
} AlternateEntityState;

typedef struct DepthBucketNode {
    /* 0x00 */ struct DepthBucketNode *next;
    /* 0x04 */ u8 pad4[0x22];
    /* 0x26 */ u16 depth;
} DepthBucketNode;

typedef struct DepthBucketGameState {
    /* 0x000 */ u8 pad0[0x16C];
    /* 0x16C */ DepthBucketNode **depthBuckets;
} DepthBucketGameState;

typedef struct OrderingTableHeapContext {
    /* 0x00 */ u8 pad0[0x70];
    /* 0x70 */ void *orderingTable;
} OrderingTableHeapContext;

typedef struct BlbHeapState {
    /* 0x0000 */ u8 pad0[0xA084];
    /* 0xA084 */ OrderingTableHeapContext *otContext;
} BlbHeapState;

typedef struct EntityTypeSpawnDef {
    /* 0x00 */ u8 pad0[0x12];
    /* 0x12 */ u16 type;
} EntityTypeSpawnDef;

#endif /* ENTINIT_ENTITIES_H */
