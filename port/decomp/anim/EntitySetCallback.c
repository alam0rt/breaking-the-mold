/* =============================================================================
 * EntitySetCallback.c  --  PC-port entity pending-callback slot setter
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/anim/EntitySetCallback.s
 * (0x8001EC18, 0xA8 bytes). Companion to EntitySetState: dispatches the
 * entity's CURRENT +0xA8 tagged slot as an exit handler (if its index is
 * nonzero), then installs the new {marker, callback} pair into +0xA8/+0xAC.
 * Unlike EntitySetState it does not clear the slot before dispatching.
 * Slot encoding is the shared tagged-dispatch form (see EntitySetState.c).
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

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

void EntitySetCallback(void *entity, u32 marker, u32 callback) {
    u8 *e = (u8 *)entity;

    if (*(s16 *)(e + 0xAA) != 0) {
        entity_dispatch(e, *(s32 *)(e + 0xA8), *(s32 *)(e + 0xAC));
    }
    *(s32 *)(e + 0xA8) = (s32)marker;
    *(s32 *)(e + 0xAC) = (s32)callback;
}
