/* =============================================================================
 * TickEntityAnimation.c  --  PC-port sprite animation state machine
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c TickEntityAnimation (0x8001D2E8,
 * INCLUDE_ASM in src/anim.c). Advances a sprite entity's animation once per
 * (frameRateDivisor+1) frames: fires the entity's event-FSM callback when the
 * clip reaches its target frame, applies any pending sprite-state change, steps
 * the current frame toward the target (looping at loopFrame), and rebuilds the
 * sprite frame data when the visible frame actually changed.
 *
 * Event FSM (eventMarker@0x08 = {lo, hi@0x0A}, eventCallback@0x0C): same
 * tagged-union as the tick/render FSMs, base = the entity; the callback is
 * invoked with (base+offset, 2, 0, entity).
 *
 * Fields: currentFrame@0xDA, targetFrame@0xDE, animChangeFlags@0xE0(u16),
 * frameRateDivisor@0xEC(u16), animLoopFlag@0xF1, animActive@0xF2.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

typedef void (*EventFn)(void *base, int a1, int a2, void *entity);

extern void ApplyPendingSpriteState(void *entity);
extern void UpdateSpriteFrameData(void *entity);
extern void AdvanceAnimationFrame(void *entity);

void TickEntityAnimation(void *entityPtr) {
    u8 *e = (u8 *)entityPtr;
    u32 uVar9 = 0;
    u16 uVar1;
    u32 uVar7 = 0, uVar5 = 0;
    int bVar3;

    if (*(u8 *)(e + 0xF2) != 0) {                        /* animActive */
        uVar1 = *(u16 *)(e + 0xEC);                      /* frameRateDivisor */
        if ((uVar1 == 0) || (*(u16 *)(e + 0xEC) = uVar1 - 1, uVar1 == 1)) {
            if (*(s16 *)(e + 0xDA) == *(s16 *)(e + 0xDE)) {   /* current == target */
                s32 hi;
                if (((*(u16 *)(e + 0xE0) & 3) == 0) &&
                    (hi = *(s16 *)(e + 0x0A), hi != 0)) {
                    EventFn fn;
                    s16 slotArg = 0;
                    s32 off;
                    if (hi < 1) {
                        fn = *(EventFn *)(e + 0x0C);
                    } else {
                        s32 slot = hi * 8 + *(s32 *)(e + *(s16 *)(e + 0x0C));
                        slotArg = (s16)*(u32 *)((u8 *)(intptr_t)slot - 8);
                        fn = *(EventFn *)((u8 *)(intptr_t)slot - 4);
                    }
                    off = *(s16 *)(e + 0x08);
                    if (hi > 0) {
                        off = slotArg + off;
                    }
                    fn(e + off, 2, 0, e);
                }
                uVar7 = *(u16 *)(e + 0xE0);
                uVar5 = uVar7 & 3;
                if (uVar5 == 3) {
                    uVar9 = *(u16 *)(e + 0xE0) >> 3 & 1;
                    ApplyPendingSpriteState(e);
                    goto reread_flags;
                }
            } else {
reread_flags:
                uVar7 = *(u16 *)(e + 0xE0);
                uVar5 = uVar7 & 3;
            }
            if (uVar5 == 2) {
                uVar9 = uVar7 >> 3 & 1;
                ApplyPendingSpriteState(e);
            }
        }
    }

    if ((*(u16 *)(e + 0xE0) & 3) == 1) {
        uVar9 = *(u16 *)(e + 0xE0) >> 3 & 1;
        ApplyPendingSpriteState(e);
    }

    bVar3 = 0;
    if (*(s16 *)(e + 0xDA) == *(s16 *)(e + 0xDE)) {
        bVar3 = (*(u8 *)(e + 0xF1) == 0);               /* animLoopFlag */
    }

    if (*(u8 *)(e + 0xF2) != 0) {                        /* animActive */
        if (uVar9 != 0) {
            goto update_frame;
        }
        if (*(u16 *)(e + 0xEC) == 0) {
            AdvanceAnimationFrame(e);
        }
    }
    if (uVar9 == 0) {
        if (*(u16 *)(e + 0xEC) != 0) {
            return;
        }
        if (*(u8 *)(e + 0xF2) == 0) {
            return;
        }
        if (bVar3) {
            return;
        }
    }

update_frame:
    UpdateSpriteFrameData(e);
    {
        u16 f = *(u16 *)(e + 0xE0);
        if ((f & 3) == 1) {
            ApplyPendingSpriteState(e);
            if ((f >> 3 & 1) != 0) {
                UpdateSpriteFrameData(e);
            }
        }
    }
}
