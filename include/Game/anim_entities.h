#ifndef ANIM_ENTITIES_H
#define ANIM_ENTITIES_H

#include "common.h"
#include "Game/entity.h"

/* =============================================================================
 * ANIMATION-SYSTEM OVERLAY LAYOUTS
 *
 * Clean-room RE note: all struct/field names are inferred working labels.
 * These overlay the entity / sprite-context blocks touched by the animation
 * tick and frame-advance code; padding runs cover offsets not yet traced.
 * Used exclusively by src/anim.c.
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

typedef struct AnimEntity {
    u8 pad00[0xC0];
    /* 0xC0 */ u32 pendingFrame;
    /* 0xC4 */ u32 pendingLoopFrame;
    /* 0xC8 */ u32 pendingTargetFrame; /* 32-bit slot; flag 0x020 copies to targetFrame@0xDE */
    u8 padCC[0xE0 - 0xCC];
    /* 0xE0 */ u16 animChangeFlags;
    u8 padE2[0xF3 - 0xE2];
    /* 0xF3 */ u8 pendingDirection;
    /* 0xF4 */ u8 pendingLoopFlag;
    /* 0xF5 */ u8 pendingAnimActive;
} AnimEntity;

typedef struct AdvAnimState {
    u8 pad00[0xD8];
    /* 0xD8 */ s16 frameCount;    /* Total frames in current sprite (wrap boundary) */
    /* 0xDA */ s16 currentFrame;
    /* 0xDC */ u16 loopFrame;
    /* 0xDE */ s16 targetFrame;
    u8 padE0[0xF0 - 0xE0];
    /* 0xF0 */ u8 animDirection;  /* 0 = forward, else reverse */
    /* 0xF1 */ u8 animLoopFlag;
} AdvAnimState;

#endif /* ANIM_ENTITIES_H */
