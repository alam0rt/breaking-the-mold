/* =============================================================================
 * EntityUpdateCallback.c  --  PC-port generic per-entity update
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c EntityUpdateCallback (0x8001xxxx,
 * INCLUDE_ASM in src/entity.c). The common tail every entity tick runs: advance
 * the entity's animation, dispatch its render-FSM callback (the tagged-union at
 * renderMarker/renderCallback, base = the entity), invalidate cached screen
 * bounds, and -- when the pending-sprite-state flags request it -- apply the
 * queued sprite state and (optionally) rebuild the sprite frame data.
 *
 * The export shows param2/param3 register params, but the body never reads them
 * (matching src/finn.c's 1-arg extern); they are accepted and ignored so the
 * frame-loop's tick dispatch and menu callers can pass them harmlessly.
 *
 * Render FSM (renderMarker@0x1C = {lo s16, hi s16 @0x1E}, renderCallback@0x20):
 *   hi == 0 -> no render handler.
 *   hi  < 0 -> call renderCallback directly with base + lo.
 *   hi  > 0 -> renderCallback low 16 is a slot-table offset at entity+off; the
 *              slot [hi].fn is the callback, its .arg adds to lo. base = entity.
 * Flags word entity[1].scaleParallaxX (+0xE0): (v&3)==1 -> ApplyPendingSpriteState;
 * additionally (v>>3 & 1) -> UpdateSpriteFrameData.
 * ---------------------------------------------------------------------------*/
#include "common.h"
#include <stdint.h>

typedef void (*RenderFn)(void *base);

extern void TickEntityAnimation(void *entity);
extern void ApplyPendingSpriteState(void *entity);
extern void UpdateSpriteFrameData(void *entity);

void EntityUpdateCallback(void *entityPtr, u32 param2, s16 param3) {
    u8 *entity = (u8 *)entityPtr;
    u16 flags;
    (void)param2;
    (void)param3;

    TickEntityAnimation(entity);

    {
        s32 hi = *(s16 *)(entity + 0x1E);
        if (hi != 0) {
            RenderFn fn;
            s16 slotArg = 0;
            s32 offset;
            if (hi < 1) {
                fn = *(RenderFn *)(entity + 0x20);
            } else {
                s32 slot = hi * 8 +
                           *(s32 *)(entity + *(s16 *)(entity + 0x20));
                slotArg = (s16)*(u32 *)((u8 *)(intptr_t)slot - 8);
                fn = *(RenderFn *)((u8 *)(intptr_t)slot - 4);
            }
            offset = *(s16 *)(entity + 0x1C);
            if (hi > 0) {
                offset = slotArg + offset;
            }
            fn(entity + offset);
        }
    }

    flags = *(u16 *)(entity + 0xE0);
    *(u8 *)(entity + 0x77) = 0;
    if ((flags & 3) == 1) {
        ApplyPendingSpriteState(entity);
        if ((flags >> 3 & 1) != 0) {
            UpdateSpriteFrameData(entity);
        }
    }
}
