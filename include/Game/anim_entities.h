#ifndef ANIM_ENTITIES_H
#define ANIM_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * ANIMATION-SYSTEM OVERLAY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These overlay the entity / sprite-context blocks touched by the animation
 * tick code; padding runs cover offsets not yet traced. Used exclusively by
 * src/anim.c. (The frame-advance and pending-sprite-state setter code uses
 * the canonical SpriteEntity from Game/entity.h directly.)
 *
 * NOTE: SpriteRenderContext here is an anim-local view and differs from the
 * same-named view in enemy_entities.h; keep each header included by only its
 * own translation unit.
 * ============================================================================= */

typedef struct SpriteContextCallbackTable {
    /* 0x00 */ u8 pad00[0x10];
    /* 0x10 */ s16 callbackTargetOffset;
    /* 0x12 */ u8 pad12[2];
    /* 0x14 */ void (*releaseVRAMSlot)(u8 *target, s32 mode);
} SpriteContextCallbackTable;

typedef struct SpriteRenderContext {
    /* 0x00 */ u8 pad00[0x0C];
    /* 0x0C */ SpriteContextCallbackTable *callbacks;
} SpriteRenderContext;

typedef struct LayerResourceEntity {
    /* 0x00 */ u8 pad00[0x18];
    /* 0x18 */ s32 collisionVtable;
    /* 0x1C */ u8 *resource;
    /* 0x20 */ u8 *renderContext;
} LayerResourceEntity;

#endif /* ANIM_ENTITIES_H */
