/* =============================================================================
 * EntityEventHandlerTimerCountdown.c  --  PC-port enemy timer event handler
 * =============================================================================
 * Faithful transcription of EntityEventHandlerTimerCountdown (INCLUDE_ASM in
 * src/enemies.c). Sibling of EntityEventHandlerWithDelayedWalk with the same
 * hit/claim event handling (0x1001/0x1002 latch +0x106; 0x1008 claims the
 * entity for a source at +0x108), but its tick path counts down the +0x111
 * timer and, on expiry, drains the entity's deferred callback queue
 * (EntityProcessCallbackQueue).
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern void EntityProcessCallbackQueue(void *e);

void *EntityEventHandlerTimerCountdown(void *arg0, s32 eventId, s32 a2, s32 source) {
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
        u8 cnt = e[0x111];
        if (cnt != 0) {
            e[0x111] = cnt - 1;
            if (((cnt - 1) & 0xFF) == 0) {
                EntityProcessCallbackQueue(e);
            }
        }
    }
    return ret;
}
