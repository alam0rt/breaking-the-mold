/* =============================================================================
 * EntityEventHandlerWithDelayedWalk.c  --  PC-port enemy event handler
 * =============================================================================
 * Faithful transcription of EntityEventHandlerWithDelayedWalk (INCLUDE_ASM in
 * src/enemies.c). Generic enemy event handler with a randomized idle->walk
 * delay:
 *   - 0x1001 / 0x1002: latch the "hit" flag (+0x106); 0x1002 also clears the
 *     tracked source at +0x108 when it matches, and returns the entity.
 *   - 0x1008: claim the entity for `source` if unclaimed (+0x108), return 1.
 *   - 0x0002 (tick): count the +0x110 idle timer down; on expiry snap the anim
 *     (SetAnimationTargetFrameIndex -1); when it reaches 0, roll a new
 *     8..0x17-frame delay into +0x112 and start walking.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void EntityStateSetWalk(void *e);
extern void SetAnimationTargetFrameIndex(void *e, s32 frame);
extern int rand(void);

void *EntityEventHandlerWithDelayedWalk(void *arg0, s32 eventId, s32 a2, s32 source) {
    u8 *e = (u8 *)arg0;
    void *ret = NULL;
    s32 ev = eventId & 0xFFFF;

    switch (ev) {
    case 0x1002:
        e[0x106] = 1;
        if (source == *(s32 *)(e + 0x108)) {
            *(s32 *)(e + 0x108) = 0;
        }
        ret = e;
        break;
    case 0x1001:
        e[0x106] = 1;
        break;
    default:
        if (ev == 0x1008 && *(s32 *)(e + 0x108) == 0) {
            *(s32 *)(e + 0x108) = source;
            ret = (void *)1;
        }
        break;
    }

    if (ev == 2) {
        u8 cnt = e[0x110];
        if (cnt != 0) {
            e[0x110] = cnt - 1;
            if (((cnt - 1) & 0xFF) == 0) {
                SetAnimationTargetFrameIndex(e, -1);
            }
            return ret;
        }
        e[0x112] = (rand() & 0xF) + 8;
        EntityStateSetWalk(e);
    }
    return ret;
}
