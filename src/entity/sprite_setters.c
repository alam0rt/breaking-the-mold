#include "common.h"

/*
 * Pending-sprite-state setters for SpriteEntity.
 *
 * These two tiny helpers latch a new sprite-id / set of animation
 * flags into the SpriteEntity "pending" fields (0xBC..0xCB / 0xE0,
 * 0xF3..0xF5). The real animation update happens later inside
 * UpdateEntityRender @ 0x8001D988, which consumes the pending fields
 * and clears the dirty bits in animChangeFlags.
 *
 * Local layout view — only fields these two functions touch are
 * named. Bits 0x004..0x100 of animChangeFlags are the per-pending
 * "dirty" bits set by the various Set* helpers in this file.
 */

typedef struct SpriteEntity_PendingView {
    u8  pad00[0xBC];
    /* 0xBC */ u32 pendingSpriteId;
    /* 0xC0 */ u32 pendingFrame;        /* 32-bit write covers s16 frame + s16 _pad */
    /* 0xC4 */ u32 pendingLoopFrame;    /* same shape as pendingFrame */
    /* 0xC8 */ u32 pendingTargetFrame;  /* 32-bit "marker"; literal 0xFFFF latched here */
    u8  padCC[0xE0 - 0xCC];
    /* 0xE0 */ u16 animChangeFlags;
    u8  padE2[0xF3 - 0xE2];
    /* 0xF3 */ u8  pendingDirection;
    /* 0xF4 */ u8  pendingLoopFlag;
    /* 0xF5 */ u8  pendingAnimActive;
} SpriteEntity_PendingView;

/*
 * SetEntitySpriteId @ 0x8001D080
 *
 * Latch a new sprite-id. ORs 0x1FC into animChangeFlags - that is all
 * the "needs update" bits (sprite + currentFrame + loopFrame +
 * targetFrame + direction + loopFlag + animActive). pendingTargetFrame
 * is set to the literal 0xFFFF marker (sentinel meaning "use sprite
 * default"), pendingFrame / pendingLoopFrame to 0, pendingDirection
 * to 0, and the loop/active flags to 1.
 */
void SetEntitySpriteId(SpriteEntity_PendingView *entity, u32 spriteId, u16 flags)
{
    entity->animChangeFlags = flags | 0x1FC;
    entity->pendingTargetFrame = 0xFFFF;
    entity->pendingSpriteId = spriteId;
    entity->pendingFrame = 0;
    entity->pendingLoopFrame = 0;
    entity->pendingDirection = 0;
    entity->pendingLoopFlag = 1;
    entity->pendingAnimActive = 1;
}

/*
 * SetAnimationSpriteFlags @ 0x8001D0B0
 *
 * Latch a new sprite-id only - ORs just the 0x004 "sprite changed"
 * bit into animChangeFlags. Caller is expected to keep the existing
 * frame / loop / direction state untouched.
 */
void SetAnimationSpriteFlags(SpriteEntity_PendingView *entity, u32 spriteId, u16 flags)
{
    entity->animChangeFlags = flags | 0x4;
    entity->pendingSpriteId = spriteId;
}
