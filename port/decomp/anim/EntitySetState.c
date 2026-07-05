/* =============================================================================
 * EntitySetState.c  --  PC-port entity anim-state FSM setter (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/anim/EntitySetState.s
 * (0x8001EAAC, 0x16C bytes; reference = export/SLES_010.90.c 14930). Swaps an
 * entity's animation-state FSM slot: runs the OLD state's exit handler (if any),
 * clears the anim-state scratch, installs the new {marker, callback} slot, and
 * runs the NEW state's enter handler.
 *
 * Anim-state FSM slot (entity+0xA0..0xAF), same tagged-dispatch encoding used by
 * the game-mode / render FSMs:
 *   +0xA0 s32 marker  = { s16 argBase (lo), s16 index (hi) }
 *   +0xA4 s32 callback/table-offset
 *   +0xA8/+0xAC = the "pending/previous" slot (saved, dispatched as the exit)
 * Dispatch: index == 0 -> no handler; index < 0 -> call callback(entity+argBase);
 * index > 0 -> table at *(entity + (s16)callback); entry = table + index*8;
 * call *(entry-4)(entity + argBase + (s16)*(entry-8)).
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

/* Run one FSM-slot handler given its packed marker + callback/table words. */
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

void EntitySetState(void *entity, u32 newMarker, u32 newCallback) {
    u8 *e = (u8 *)entity;
    s16 oldIndex = *(s16 *)(e + 0xAA);

    *(s16 *)(e + 0xA0) = 0;
    *(s16 *)(e + 0xA2) = 0;
    *(s32 *)(e + 0xA4) = 0;

    if (oldIndex != 0) {
        s32 markerWord = *(s32 *)(e + 0xA8);
        s32 callbackWord = *(s32 *)(e + 0xAC);
        *(s16 *)(e + 0xA8) = 0;
        *(s16 *)(e + 0xAA) = 0;
        *(s32 *)(e + 0xAC) = 0;
        entity_dispatch(e, markerWord, callbackWord);
    }

    if (*(s16 *)(e + 0xA2) == 0) {
        *(s32 *)(e + 0x94) = 0;
        *(s16 *)(e + 0x98) = 0;
        *(s16 *)(e + 0x9A) = 0;
        *(s32 *)(e + 0x9C) = 0;
        *(s32 *)(e + 0xA0) = (s32)newMarker;
        *(s32 *)(e + 0xA4) = (s32)newCallback;
        if (*(s16 *)(e + 0xA2) != 0) {
            entity_dispatch(e, *(s32 *)(e + 0xA0), *(s32 *)(e + 0xA4));
        }
    }
}
