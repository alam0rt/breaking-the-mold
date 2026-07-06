/* =============================================================================
 * EntityProcessCallbackQueue.c  --  PC-port queued state-callback consumer
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/anim/EntityProcessCallbackQueue.s
 * (0x8001E928, 0x184). Consumes the entity's queued FSM transitions:
 *   1. If the pending/exit slot (+0xA8/+0xAC) is armed, clear it and dispatch
 *      it (the state's exit handler).
 *   2. If the queued next-state slot (+0x98/+0x9C) is armed, promote it into
 *      the active anim-state slot (+0xA0/+0xA4), clear the queue, and dispatch
 *      the promoted enter handler.
 *   3. Otherwise, if an animation sequence (+0x94) is active, step it;
 *      else clear the active slot (+0xA0..+0xA4).
 * Slot encoding is the shared tagged-dispatch form (see EntitySetState.c).
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern void StepAnimationSequence(void *entity);

static void entity_dispatch(u8 *e, s32 markerWord, s32 callbackWord) {
    s16 index = (s16)(markerWord >> 16);
    s16 argBase = (s16)markerWord;
    void (*fn)(void *);

    if (index > 0) {
        s16 tableOff = (s16)callbackWord;
        s32 tableBase = *(s32 *)(e + tableOff);
        u8 *entry = (u8 *)(uintptr_t)(tableBase + (s32)index * 8);
        s16 argOff = (s16)*(s32 *)(entry - 8);
        fn = *(void (**)(void *))(entry - 4);
        argBase = (s16)(argOff + argBase);
    } else {
        fn = (void (*)(void *))(uintptr_t)callbackWord;
    }
    fn(e + argBase);
}

void EntityProcessCallbackQueue(void *entity) {
    u8 *e = (u8 *)entity;

    if (*(s16 *)(e + 0xAA) != 0) {
        s32 m = *(s32 *)(e + 0xA8);
        s32 f = *(s32 *)(e + 0xAC);
        *(s16 *)(e + 0xA8) = 0;
        *(s16 *)(e + 0xAA) = 0;
        *(s32 *)(e + 0xAC) = 0;
        entity_dispatch(e, m, f);
    }

    if (*(s16 *)(e + 0x9A) != 0) {
        *(s32 *)(e + 0xA0) = *(s32 *)(e + 0x98);
        *(s32 *)(e + 0xA4) = *(s32 *)(e + 0x9C);
        *(s16 *)(e + 0x98) = 0;
        *(s16 *)(e + 0x9A) = 0;
        *(s32 *)(e + 0x9C) = 0;
        entity_dispatch(e, *(s32 *)(e + 0xA0), *(s32 *)(e + 0xA4));
    } else if (*(s32 *)(e + 0x94) != 0) {
        StepAnimationSequence(e);
    } else {
        *(s16 *)(e + 0xA0) = 0;
        *(s16 *)(e + 0xA2) = 0;
        *(s32 *)(e + 0xA4) = 0;
    }
}
