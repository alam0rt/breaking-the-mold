#ifndef EFFECT_ENTITIES_H
#define EFFECT_ENTITIES_H

#include "common.h"
#include "Game/entity.h"
#include "Game/spracc_records.h"

/* =============================================================================
 * VISUAL-EFFECT ENTITY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * Effect-extension structs extend SpriteEntity (0x100 base) or are raw-header
 * overlays over the basic-entity block; padding runs cover offsets not yet
 * traced. The *Record / *Context structs are views over auxiliary blocks
 * (spawn data, render context). Used exclusively by src/effects.c.
 * ============================================================================= */

typedef struct DecorEventEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u16 timer;
    /* 0x102 */ u8 notifyFlag;
} DecorEventEntity;

typedef struct EffectChildContext {
    /* 0x00 */ u8 pad0[0xA];
    /* 0x0A */ u8 activeFlag;
} EffectChildContext;

typedef struct MultiPartEffectEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[8];
    /* 0x108 */ u8 releaseFlag;
} MultiPartEffectEntity;

typedef struct SpawnBoundsRecord {
    /* 0x00 */ u8 pad0[0x3C];
    /* 0x3C */ s32 active;
} SpawnBoundsRecord;

typedef struct SpawnBoundsEffectEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ SpawnBoundsRecord *record;
    /* 0x104 */ u8 pad104[5];
    /* 0x109 */ u8 despawnedFlag;
} SpawnBoundsEffectEntity;

typedef struct ColoredOverlayEntity {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ void *eventVtable;
    /* 0x10 */ u8 pad10[0x30];
    /* 0x40 */ u8 r;
    /* 0x41 */ u8 g;
    /* 0x42 */ u8 b;
    /* 0x43 */ u8 alpha;
} ColoredOverlayEntity;

typedef struct OverlayCallbackEntity {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ EffectChildContext *child;
    /* 0x24 */ u8 pad24[0x10];
    /* 0x34 */ u8 hiddenFlag;
} OverlayCallbackEntity;

typedef struct RippleExpandEntity {
    /* 0x000 */ u8 pad0[0xC];
    /* 0x00C */ void *eventVtable;
    /* 0x010 */ u8 pad10[0x10];
    /* 0x020 */ void *renderPrim;  /* child render-primitive slot; freed by
                                      DestroyRippleExpandEffect (via base
                                      renderCallback), driven each frame by
                                      RippleEffectRenderCallback */
    /* 0x024 */ u16 localX;        /* effect-local world X, projected to screen */
    /* 0x026 */ u16 localY;        /* effect-local world Y, projected to screen */
    /* 0x028 */ u8 pad28[0x378];
    /* 0x3A0 */ s16 red;
    /* 0x3A2 */ s16 green;
    /* 0x3A4 */ s16 blue;
    /* 0x3A6 */ u8 frameSeed;
    /* 0x3A7 */ u8 phase;
} RippleExpandEntity;

typedef struct CountdownTimerEntity {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ EffectChildContext *child;
    /* 0x24 */ u8 pad24[4];
    /* 0x28 */ u8 timer;
    /* 0x29 */ u8 expiredFlag;
} CountdownTimerEntity;

typedef struct BeamEffectEntity {
    /* 0x00 */ u8 pad0[0x1C];
    /* 0x1C */ u16 rotationStep;
    /* 0x1E */ u8 expiredFlag;
    /* 0x1F */ u8 pad1F;
    /* 0x20 */ EffectChildContext *child;
    /* 0x24 */ u8 worldFreezeFlag;
    /* 0x25 */ u8 pad25[3];
    /* 0x28 */ u16 rotation;
    /* 0x2A */ u16 timer;
} BeamEffectEntity;

typedef struct EntityWithOwnedData {
    /* 0x00 */ u8 pad0[0x18];
    /* 0x18 */ void *collisionVtable;
    /* 0x1C */ void *ownedData;
} EntityWithOwnedData;

typedef struct EntityWithTextureMemory {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ void *eventVtable;
    /* 0x10 */ u8 pad10[0x2C];
    /* 0x3C */ void *textureData0;
    /* 0x40 */ void *textureData1;
} EntityWithTextureMemory;

typedef struct PathFollowResourceEntity {
    /* 0x00 */ u8 pad0[0xC];
    /* 0x0C */ void *eventVtable;
    /* 0x10 */ u8 pad10[0x2C];
    /* 0x3C */ void *pathData;
    /* 0x40 */ void *segmentData;
    /* 0x44 */ u8 pad44[4];
    /* 0x48 */ void *extraData;
} PathFollowResourceEntity;

typedef struct GridDistortionResourceEntity {
    /* 0x000 */ u8 pad0[0xC];
    /* 0x00C */ void *eventVtable;
    /* 0x010 */ u8 pad10[0xEC];
    /* 0x0FC */ void *gridData0;
    /* 0x100 */ void *gridData1;
} GridDistortionResourceEntity;

typedef struct ZOrderTimerEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[8];
    /* 0x108 */ u8 timer;
} ZOrderTimerEntity;

typedef struct FadeInEntity {
    /* 0x00 */ u8 pad0[0x1C];
    /* 0x1C */ ColoredOverlayEntity *overlay;
    /* 0x20 */ u8 pad20[2];
    /* 0x22 */ s16 alpha;
} FadeInEntity;

typedef struct EffectWord90Entity {
    /* 0x000 */ u8 pad0[0x90];
    /* 0x090 */ u16 value90;
} EffectWord90Entity;

typedef struct EffectByte58Entity {
    /* 0x00 */ u8 pad0[0x58];
    /* 0x58 */ u8 value58;
    /* 0x59 */ u8 pad59[4];
    /* 0x5D */ u8 value5D;
} EffectByte58Entity;

typedef struct LargeEffectStateEntity {
    /* 0x000 */ u8 pad0[0x1E0];
    /* 0x1E0 */ u8 value1E0;
    /* 0x1E1 */ u8 pad1E1[6];
    /* 0x1E7 */ u8 value1E7;
} LargeEffectStateEntity;

/* VRAM slot entity layout — first 0x10 bytes are the basic-entity header
 * (initialized by InitBasicEntityWithVtable). +0x10 holds the AllocateVRAMSlot
 * return code (1=ok, 0=failed). +0x16/+0x18 are the raw VRAM pixel
 * coordinates of the allocated slot. */
typedef struct VRAMSlotEntity {
    /* 0x00 */ u8 pad0[0x10];
    /* 0x10 */ u8 vramAllocOk;
    /* 0x11 */ u8 pad11[5];
    /* 0x16 */ u16 vramX;
    /* 0x18 */ u16 vramY;
} VRAMSlotEntity;

typedef struct EntityWithRenderTarget {
    u8 pad00[0x68];
    u16 worldX;            /* 0x68 */
    u16 worldY;            /* 0x6A */
    u8 pad6C[0x118 - 0x6C];
    s16 *renderScreenPos;  /* 0x118 */
} EntityWithRenderTarget;

typedef struct CompoundEntity {
    /* 0x000 */ SpriteEntity sprite;
    /* 0x100 */ u8 pad100[4];
    /* 0x104 */ void *child;
} CompoundEntity;

#endif /* EFFECT_ENTITIES_H */
