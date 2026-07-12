/* =============================================================================
 * StepAnimationSequence.c  --  PC-port conversion (0x8001E7B8)
 * =============================================================================
 * Advances a scripted animation sequence. Two FSM-slot dispatches, both using
 * the shared marker convention {argOffset s16, mode s16, fn/table word}:
 *   - mode <= 0: the word IS the callback, invoked on (e + argOffset).
 *   - mode  > 0: the low s16 of the word is a byte offset into the entity to a
 *     TABLE pointer; entry mode-1 (8-byte {extraOffset, fn}) supplies the
 *     callback and an extra argOffset added to the marker's.
 * Step 1: if the pending slot at +0xA8/+0xAA/+0xAC is armed, clear it and fire
 * it (one-shot). Step 2: copy the next sequence entry (table at +0x94, index
 * +0xE2, count +0xE4 -- table pointer zeroed when the index reaches count)
 * into the current slot at +0xA0/+0xA4 and fire that.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

typedef void (*SeqFn)(void *);

/* resolve {argOff, mode, word} to a callback + call arg, then invoke */
static void dispatch_slot(u8 *e, s16 argOff, s16 mode, s32 word) {
    SeqFn fn;
    s32 off;

    if (mode > 0) {
        u8 *tbl = *(u8 **)(e + (s16)word);
        s32 *pair = (s32 *)(tbl + mode * 8);
        s32 extra = pair[-2];
        fn = (SeqFn)pair[-1];
        off = (s16)extra + argOff;   /* sum not re-truncated (asm addu) */
    } else {
        fn = (SeqFn)word;
        off = argOff;
    }
    fn(e + off);
}

void StepAnimationSequence(void *arg0) {
    u8 *e = (u8 *)arg0;

    if (*(s16 *)(e + 0xAA) != 0) {
        s16 argOff = *(s16 *)(e + 0xA8);
        s16 mode = *(s16 *)(e + 0xAA);
        s32 word = *(s32 *)(e + 0xAC);

        *(s16 *)(e + 0xA8) = 0;
        *(s16 *)(e + 0xAA) = 0;
        *(s32 *)(e + 0xAC) = 0;
        dispatch_slot(e, argOff, mode, word);
    }

    {
        s32 *entry = (s32 *)(*(u8 **)(e + 0x94) + *(s16 *)(e + 0xE2) * 8);
        u16 idx;

        *(s32 *)(e + 0xA0) = entry[0];
        *(s32 *)(e + 0xA4) = entry[1];

        idx = (u16)(*(u16 *)(e + 0xE2) + 1);
        *(u16 *)(e + 0xE2) = idx;
        if ((s16)idx >= *(s16 *)(e + 0xE4)) {
            *(s32 *)(e + 0x94) = 0;
        }

        dispatch_slot(e, *(s16 *)(e + 0xA0), *(s16 *)(e + 0xA2),
                      *(s32 *)(e + 0xA4));
    }
}
