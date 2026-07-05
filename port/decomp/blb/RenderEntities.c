/* =============================================================================
 * RenderEntities.c  --  PC-port per-frame entity/layer render walk (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/RenderEntities.s
 * (0x80020E80, 0xC8 bytes). Runs once per frame from the main loop:
 *   1. If the background-colour-dirty flag (GameState+0x130) is set, copy the
 *      RGB triple (GameState+0x131..0x133) into both draw-environment clear
 *      colours in the GPU context (heap+0x1D..0x1F for buffer 0, heap+0x505D..
 *      0x505F for buffer 1, stride 0x5040), then clear the dirty flag.
 *   2. Walk the tick list (GameState+0x1C) to its end -- a bare traversal in the
 *      original (no per-node action); reproduced for fidelity.
 *   3. Walk the render list (GameState+0x20): for each 8-byte node
 *      {next @ +0x0, renderObj @ +0x4}, fetch the render object's vtable
 *      (renderObj+0xC), read its arg offset (*(s16*)(vtable+0x8)) and render
 *      callback (*(vtable+0xC)), and invoke callback(renderObj + argOffset).
 *
 * NOTE (CP-2.4): the render callbacks live in rodata vtables (e.g. D_80010364
 * for 16x16 tilemap layers). Those tables are weak-zeroed on PC until the
 * render-callback functions are converted and the vtables populated, so this
 * walk will dispatch a NULL callback until that render-subsystem work lands.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

void RenderEntities(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u8 *node;

    if (gs[0x130] != 0) {
        u8 r = gs[0x131], g = gs[0x132], b = gs[0x133];
        heap[0x1D]   = r; heap[0x1E]   = g; heap[0x1F]   = b;
        heap[0x505D] = r; heap[0x505E] = g; heap[0x505F] = b;
        gs[0x130] = 0;
    }

    /* tick-list traversal (no per-node action in the original) */
    {
        u8 *t = *(u8 **)(gs + 0x1C);
        while (t != NULL) {
            t = *(u8 **)(t + 0x0);
        }
    }

    /* render-list dispatch */
    node = *(u8 **)(gs + 0x20);
    while (node != NULL) {
        u8 *obj = *(u8 **)(node + 0x4);
        u8 *vtable = *(u8 **)(obj + 0xC);
        s16 argOff = *(s16 *)(vtable + 0x8);
        void (*fn)(void *) = *(void (**)(void *))(vtable + 0xC);
        fn(obj + argOff);
        node = *(u8 **)(node + 0x0);
    }
}
