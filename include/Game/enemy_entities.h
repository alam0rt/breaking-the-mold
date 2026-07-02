#ifndef ENEMY_ENTITIES_H
#define ENEMY_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * ENEMY / COLLECTIBLE / HAZARD ENTITY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * Entity-extension structs extend SpriteEntity (0x100 base) with type-specific
 * scratch beginning at 0x100; padding runs cover offsets not yet traced. The
 * *Spawn / *Record structs are views over BLB spawn-data records. Used
 * exclusively by src/enemies.c.
 * ============================================================================= */

/* Parent entity with up to three linked-entity slots that the offscreen
 * cull family can disable and detach. */
typedef struct OffscreenCullEntity {
    /* 0x000 */ u8 pad0[0x100];
    /* 0x100 */ Entity *cullChild;
    /* 0x104 */ Entity *cullSpawnRef;
    /* 0x108 */ Entity *cullSpawnData;
} OffscreenCullEntity;

typedef struct CollectibleSpawnData {
    /* 0x00 */ u8 pad0[8];
    /* 0x08 */ s16 x;
    /* 0x0A */ u16 y;
} CollectibleSpawnData;

typedef struct TimedCollectibleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u16 timer;
} TimedCollectibleEntity;

typedef struct EnemyTimerStateEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u16 stateTimer;
    /* 0x106 */ u8 pad106[0xA];
    /* 0x110 */ u8 stateDelay;
    /* 0x111 */ u8 pad111;
    /* 0x112 */ u8 walkDelay;
    /* 0x113 */ u8 pad113;
    /* 0x114 */ u32 *spriteIds;     /* Per-state sprite id table (>= 6 entries: sparkle, walk, sparkleCollect, idle, ?, attack) */
} EnemyTimerStateEntity;

/* Falling-enemy state slab. The +0x110 word is cleared atomically (treated
 * as a single u32 by InitEnemyFallingState even though +0x111-0x113 are
 * never used as a single 32-bit value elsewhere) and +0x116 holds a
 * recovery/state s16 zeroed in the same sequence. */
typedef struct EnemyFallingEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ u32 stateBundle;     /* Atomic-zeroed 4-byte state slab */
    /* 0x114 */ u8 pad114[2];
    /* 0x116 */ s16 recoveryState;
    /* 0x118 */ u8 padFall118[1];
    /* 0x119 */ u8 fallVariant;      /* Selects sprite-id / direction in the if (((u8 *)e)[0x119]) chain */
} EnemyFallingEntity;

/* Entity with embedded child-entity ref + SPU voice id, used by
 * DestroyEntityWithSoundAndChild. */
typedef struct EntityWithSoundAndChild {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ s32 childEntityId;
    /* 0x114 */ u8 pad114[4];
    /* 0x118 */ s32 voiceId;
} EntityWithSoundAndChild;

/* Entity with embedded child-entity ref at +0x104, used by
 * DestroyEntityWithChildRemoval (checkpoint variants). */
typedef struct EntityWithChild104 {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ s32 childEntityId;
} EntityWithChild104;

/* Spawn record referenced by the platform-activation-style entities at
 * Entity+0x100. Field +0x04 holds a sprite-id; field +0x0C is a phase
 * offset added to g_pGameState->frame_counter for the periodic toggle. */
typedef struct PlatformActivationSpawnRef {
    /* 0x00 */ u8 pad00[4];
    /* 0x04 */ u32 spriteId;
    /* 0x08 */ u8 pad08[4];
    /* 0x0C */ s32 framePhaseOffset;
} PlatformActivationSpawnRef;

typedef struct PlatformActivationRefEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ PlatformActivationSpawnRef *spawn;
} PlatformActivationRefEntity;

typedef struct SoundEmitterEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x20];
    /* 0x120 */ u32 *spriteIdPtr;
    /* 0x124 */ u16 stunTimer;
    /* 0x126 */ u8 pad126[2];
    /* 0x128 */ s32 voiceId;
} SoundEmitterEntity;

typedef struct ConditionalPhaseEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ u8 framePhase;
} ConditionalPhaseEntity;

typedef struct HazardVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ s32 voiceId;
} HazardVoiceEntity;

typedef struct HazardTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10];
    /* 0x110 */ u16 timer;
} HazardTimerEntity;

typedef struct TimedByteEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u8 timer;
} TimedByteEntity;

typedef struct TimerTileRecord {
    /* 0x00 */ u8 pad0[0x12];
    /* 0x12 */ u16 tileId;
} TimerTileRecord;

typedef struct TimedByteWithTileEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ u8 timer;
    /* 0x105 */ u8 pad105[3];
    /* 0x108 */ TimerTileRecord *tileRecord;
} TimedByteWithTileEntity;

typedef struct DeathSpawnTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x12];
    /* 0x112 */ u16 deathTimer;
} DeathSpawnTimerEntity;

typedef struct SoundPanningEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x18];
    /* 0x118 */ u32 soundId;
} SoundPanningEntity;

typedef struct PlatformActivationSpawn {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ s32 activateFrame;
} PlatformActivationSpawn;

typedef struct PlatformActivationEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[8];
    /* 0x108 */ PlatformActivationSpawn *spawn;
} PlatformActivationEntity;

typedef struct SwitchBlockSpawn {
    /* 0x00 */ u8 pad0[0x12];
    /* 0x12 */ u16 tileId;
} SwitchBlockSpawn;

typedef struct SwitchFlagEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ SwitchBlockSpawn *spawn;
} SwitchFlagEntity;

typedef struct BackgroundSparkleEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 contactFlag;
    /* 0x101 */ u8 revealTimer;
} BackgroundSparkleEntity;

typedef struct BackgroundSparkleChildContext {
    /* 0x00 */ u8 pad0[0xA];
    /* 0x0A */ u8 activeFlag;
} BackgroundSparkleChildContext;

/* Subset of the per-entity sprite-render context (the `spriteContext` field
 * on Entity); only the fields touched by InitCollectibleEntity_Alt's TPage
 * setup are mapped here. */
typedef struct SpriteRenderContext {
    /* 0x00 */ u8  pad00[0x0A];
    /* 0x0A */ u8  activeFlag;       /* Set to 1 to enable rendering of this sprite. */
    /* 0x0B */ u8  pad0B[5];
    /* 0x10 */ s16 vramX;            /* Source X in VRAM (pixels) */
    /* 0x12 */ s16 vramY;            /* Source Y in VRAM (pixels) */
    /* 0x14 */ u8  pad14[0x10];
    /* 0x24 */ s16 tpageBits;        /* Cached GetTPage result */
    /* 0x26 */ u8  pad26[0xC];
    /* 0x32 */ u8  colorMode;        /* Color depth code passed to GetTPage */
    /* 0x33 */ u8  pad33;
    /* 0x34 */ u8  colorR;           /* Per-channel color multiplier (0x80 = neutral) */
    /* 0x35 */ u8  colorG;
    /* 0x36 */ u8  colorB;
} SpriteRenderContext;

/* Overlay view of the twinkling-collectible entity (SpriteEntity base +
 * sparkle-pulse scratch). */
typedef struct SparkleCollectibleEntity {
    u8  pad00[0x34];
    /* 0x34 */ SpriteRenderContext *ctx;
    u8  pad38[0x50 - 0x38];
    /* 0x50 */ s32 scaleRender;
    u8  pad54[0x68 - 0x54];
    /* 0x68 */ s16 worldX;
    /* 0x6A */ s16 worldY;
    u8  pad6C[0x74 - 0x6C];
    /* 0x74 */ u8  facing;
    u8  pad75[0x10C - 0x75];
    /* 0x10C */ u8  unk10C;
    u8  pad10D[0x119 - 0x10D];
    /* 0x119 */ u8  enable;
    /* 0x11A */ u8  brightness;
    /* 0x11B */ u8  phase;
    /* 0x11C */ u8  phaseTimer;
    /* 0x11D */ u8  spawnTimer;
} SparkleCollectibleEntity;

#endif /* ENEMY_ENTITIES_H */
