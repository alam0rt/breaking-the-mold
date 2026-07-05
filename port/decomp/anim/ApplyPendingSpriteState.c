/* =============================================================================
 * ApplyPendingSpriteState.c  --  PC-port pending sprite/anim state commit
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c ApplyPendingSpriteState
 * (0x8001D3D0, INCLUDE_ASM in src/anim.c). Commits the queued sprite/animation
 * changes flagged in animChangeFlags (set by the SetAnimation* / SetEntitySpriteId
 * helpers) into the live animation fields, then clears the flags. Each frame
 * field can be set either as a literal index, as -1 (meaning "last frame"), or --
 * when the *_BY_VALUE flag is set -- resolved from a frame value via
 * FindFrameIndexByValue.
 *
 * Fields (SpriteEntity): animChangeFlags@0xE0(u16), pFrameTable@0x78,
 * frameCount@0x88, pFrameData@0x8C, curSpriteFrameCount@0xD8, currentFrame@0xDA,
 * loopFrame@0xDC, targetFrame@0xDE, pendingSpriteId@0xBC, pendingFrame@0xC0,
 * pendingLoopFrame@0xC4, pendingTargetFrame@0xC8, currentSpriteId@0xCC,
 * animDirection@0xF0, animLoopFlag@0xF1, animActive@0xF2, pendingDirection@0xF3,
 * pendingLoopFlag@0xF4, pendingAnimActive@0xF5, staticSpriteFlag@0xF7,
 * needsDecodeFlag@0xF8.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

extern s32  InitSpriteContext(void *ctx, u32 spriteId);
extern void FindOrderingTableSlotById(void *pFrameData, u32 spriteId);
extern u16  GetSpriteFrameCount(void *pFrameData);
extern int  FindFrameIndexByValue(int entity, int value);

#define ANIM_CHG_SPRITE        0x0004
#define ANIM_CHG_FRAME         0x0008
#define ANIM_CHG_LOOP_FRAME    0x0010
#define ANIM_CHG_TARGET_FRAME  0x0020
#define ANIM_CHG_DIRECTION     0x0040
#define ANIM_CHG_LOOP_FLAG     0x0080
#define ANIM_CHG_ANIM_ACTIVE   0x0100

void ApplyPendingSpriteState(void *entityPtr) {
    u8 *e = (u8 *)entityPtr;
    u16 flags;

    if ((*(u16 *)(e + 0xE0) & ANIM_CHG_SPRITE) != 0) {
        u32 id = *(u32 *)(e + 0xBC);
        *(u32 *)(e + 0xCC) = id;
        if (*(u8 *)(e + 0xF7) == 0) {
            InitSpriteContext(e + 0x78, id);
            *(s16 *)(e + 0xD8) = *(u16 *)(e + 0x88);
        } else {
            FindOrderingTableSlotById(e + 0x8C, id);
            *(s16 *)(e + 0xD8) = GetSpriteFrameCount(e + 0x8C);
        }
        *(u8 *)(e + 0xF8) = 1;
    }

    flags = *(u16 *)(e + 0xE0);
    if ((flags & ANIM_CHG_FRAME) != 0) {
        if ((flags & 0x200) == 0) {
            if (*(s32 *)(e + 0xC0) == -1) {
                *(s16 *)(e + 0xDA) = *(s16 *)(e + 0xD8) - 1;
            } else {
                *(s16 *)(e + 0xDA) = (s16)*(s32 *)(e + 0xC0);
            }
        } else {
            *(s16 *)(e + 0xDA) = (s16)FindFrameIndexByValue((int)(intptr_t)e, *(s32 *)(e + 0xC0));
        }
        flags = *(u16 *)(e + 0xE0);
    }

    if ((flags & ANIM_CHG_LOOP_FRAME) != 0) {
        if ((flags & 0x400) == 0) {
            if (*(s32 *)(e + 0xC4) == -1) {
                *(s16 *)(e + 0xDC) = *(s16 *)(e + 0xD8) - 1;
            } else {
                *(s16 *)(e + 0xDC) = (s16)*(s32 *)(e + 0xC4);
            }
        } else {
            *(s16 *)(e + 0xDC) = (s16)FindFrameIndexByValue((int)(intptr_t)e, *(s32 *)(e + 0xC4));
        }
    }

    if ((*(u16 *)(e + 0xE0) & ANIM_CHG_TARGET_FRAME) != 0) {
        if ((*(u16 *)(e + 0xE0) & 0x800) == 0) {
            if (*(s32 *)(e + 0xC8) == -1) {
                *(s16 *)(e + 0xDE) = *(s16 *)(e + 0xD8) - 1;
            } else {
                *(s16 *)(e + 0xDE) = (s16)*(s32 *)(e + 0xC8);
            }
        } else {
            *(s16 *)(e + 0xDE) = (s16)FindFrameIndexByValue((int)(intptr_t)e, *(s32 *)(e + 0xC8));
        }
    }

    if ((*(u16 *)(e + 0xE0) & ANIM_CHG_DIRECTION) != 0) {
        *(u8 *)(e + 0xF0) = *(u8 *)(e + 0xF3);
    }
    if ((*(u16 *)(e + 0xE0) & ANIM_CHG_LOOP_FLAG) != 0) {
        *(u8 *)(e + 0xF1) = *(u8 *)(e + 0xF4);
    }
    if ((*(u16 *)(e + 0xE0) & ANIM_CHG_ANIM_ACTIVE) != 0) {
        *(u8 *)(e + 0xF2) = *(u8 *)(e + 0xF5);
    }
    *(u16 *)(e + 0xE0) = 0;
}
