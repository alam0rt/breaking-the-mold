#ifndef BOSS_ENTITIES_H
#define BOSS_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * BOSS ENTITY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * Each of these extends SpriteEntity (0x100 base) with boss-specific scratch
 * fields beginning at 0x100. Padding runs (pad100 etc.) cover offsets whose
 * meaning has not yet been traced. Used exclusively by src/bosses.c.
 * ============================================================================= */

typedef struct BossTimedSpriteEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x104 - 0x100];
    /* 0x104 */ u16 shortTimer;
} BossTimedSpriteEntity;

typedef struct BossVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x118 - 0x100];
    /* 0x118 */ s32 voiceHandle;
} BossVoiceEntity;

typedef struct BossVoice144Entity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x144 - 0x100];
    /* 0x144 */ s32 voiceHandle;
} BossVoice144Entity;

/* NOTE: distinct from enemy_entities.h's HazardTimerEntity, which is a DIFFERENT
 * entity with its timer at +0x110. Renamed here to break the name collision
 * (the two were never layout-compatible: behaviorTimer@0x114 vs timer@0x110). */
typedef struct BossHazardTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x114 - 0x100];
    /* 0x114 */ u16 behaviorTimer;
} BossHazardTimerEntity;

typedef struct ShrineyGuardEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x10C - 0x100];
    /* 0x10C */ u8 readyFlag;
    /* 0x10D */ u8 pad10D[0x111 - 0x10D];
    /* 0x111 */ u8 activeTimer;
    /* 0x112 */ u8 idleTimeout;
    /* 0x113 */ u8 stunTimer;
    /* 0x114 */ u8 attackCounter;
    /* 0x115 */ u8 stunActive;
} ShrineyGuardEntity;

typedef struct KloggTriggerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x110 - 0x100];
    /* 0x110 */ u8 triggerTimer;
    /* 0x111 */ u8 triggerActive;
    /* 0x112 */ u8 pad112[0x134 - 0x112];
    /* 0x134 */ u8 *triggerTarget;
} KloggTriggerEntity;

typedef struct GliderEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100;
    /* 0x101 */ u8 readyFlag;
    /* 0x102 */ u8 pad102[0x10D - 0x102];
    /* 0x10D */ u8 directionFlag;
} GliderEntity;

typedef struct JoeHeadJoeEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x112 - 0x100];
    /* 0x112 */ u16 attackTimer;
    /* 0x114 */ u8 pad114[0x118 - 0x114];
    /* 0x118 */ s32 voiceHandle;
} JoeHeadJoeEntity;

typedef struct KloggBossEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x104 - 0x100];
    /* 0x104 */ u16 shortTimer;
    /* 0x106 */ u8 pad106[0x110 - 0x106];
    /* 0x110 */ s32 voiceHandle;
    /* 0x114 */ u8 pad114[2];
    /* 0x116 */ u16 voicePanTimer;
} KloggBossEntity;

/* Minimal view of the sprite-render context attached to an Entity via
 * Entity.spriteContext (+0x34). Only the active/visible byte is needed
 * by the death callbacks here. */
typedef struct SpriteRenderContextRef {
    /* 0x00 */ u8 pad00[0xA];
    /* 0x0A */ u8 activeFlag;
} SpriteRenderContextRef;

/* Layout for an entity that walks through a fixed sequence of (x,y)
 * waypoints. Each entry in the path table is 4 bytes: an s16 x and an
 * s16 y stored consecutively. nextWaypointIndex is bumped on every
 * tick until it reaches waypointCount; from that point on the entity
 * falls through to its queued state instead. */
typedef struct FollowPathEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x139 - 0x100];
    /* 0x139 */ u8 nextWaypointIndex;
    /* 0x13A */ u8 waypointCount;
    /* 0x13B */ u8 pad13B;
    /* 0x13C */ s16 *path;
} FollowPathEntity;

/* Entity layout for the boss-class destructor that has an SPU voice
 * handle at +0x118 (vs +0x10C used elsewhere). */
typedef struct BossWithSpuVoiceEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[0x118 - 0x100];
    /* 0x118 */ s32 voiceHandle;
} BossWithSpuVoiceEntity;

#endif /* BOSS_ENTITIES_H */
