/* =============================================================================
 * InitScaledMenuEntityWithChild.c  --  functional-C body for effects.c
 * InitScaledMenuEntityWithChild (TARGET_PC)
 * =============================================================================
 * Transcribed from asm/nonmatchings/effects/InitScaledMenuEntityWithChild.s
 * (0x800371D4, 0x144). One-shot screen-space effect used on collectible
 * pickup: a 0x44C menu entity (vtable D_80010980) that owns a 0x3A8 child
 * prim object (vtable D_800109A0 at +0xC) registered on the X-position list.
 * The child's three shorts at +0x3A0..+0x3A4 hold a step value derived from
 * the parent entity's render scale (0x10000 -> 4, 0xC000 -> 3, 0x8000 -> 2),
 * +0x3A6 latches the current frame counter, +0x3A7 is the active flag.
 * Tick = CountdownTimerTickCallback (matched C, src/effects.c). The zOrder
 * argument (0x3D3 at the SpawnCollectibleParticles call site) is accepted but
 * never read -- faithful to the asm.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void *g_pBlbHeapBase;
extern u8  *AllocateFromHeap(void *heap, u32 size, u32 count, u8 flag);
extern void InitMenuEntityWithVtable(void *entity, s32 allocSize);
extern void *InitBasicEntityWithVtable(void *p, u16 val);
extern void AddToXPositionList(void *gs, void *obj);
extern void CountdownTimerTickCallback();       /* matched C, src/effects.c */

extern u8 VT_SCALED_MENU_EFFECT[]   asm("D_80010980");
extern u8 VT_SCALED_MENU_CHILD[]    asm("D_800109A0");

void *InitScaledMenuEntityWithChild(void *arg0, s16 x, s16 y, s32 zOrder,
                                    s32 scale) {
    u8 *e = (u8 *)arg0;
    u8 *child;
    s32 step = 4;

    (void)zOrder;   /* never read (asm passes 0x3D3 but ignores it) */

    InitMenuEntityWithVtable(e, 0x44C);
    *(void **)(e + 0x18) = VT_SCALED_MENU_EFFECT;

    if (scale == 0xC000) {
        step = 3;
    } else if (scale == 0x8000) {
        step = 2;
    }

    child = AllocateFromHeap(g_pBlbHeapBase, 0x3A8, 1, 0);
    InitBasicEntityWithVtable(child, 0x3CA);
    child[0x3A7] = 0;
    *(void **)(child + 0xC) = VT_SCALED_MENU_CHILD;
    *(s16 *)(child + 0x3A0) = (s16)(step & 0xFF);
    *(s16 *)(child + 0x3A2) = (s16)(step & 0xFF);
    *(s16 *)(child + 0x3A4) = (s16)(step & 0xFF);
    child[0x3A6] = (u8)*(s32 *)((u8 *)g_pGameState + 0x10C);  /* frame counter */
    *(u8 **)(e + 0x20) = child;
    child[0x3A7] = 1;
    AddToXPositionList(g_pGameState, *(u8 **)(e + 0x20));

    /* tick slot: direct callback, marker 0xFFFF0000 */
    *(s32 *)(e + 0x0) = (s32)0xFFFF0000;
    *(void **)(e + 0x4) = (void *)CountdownTimerTickCallback;

    *(s16 *)(e + 0x24) = x;
    *(s16 *)(e + 0x26) = y;
    e[0x28] = 0x10;
    e[0x29] = 0;
    return e;
}
