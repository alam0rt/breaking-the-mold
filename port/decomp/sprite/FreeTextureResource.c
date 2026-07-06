/* =============================================================================
 * FreeTextureResource.c  --  PC-port texture/CLUT/heap teardown for a sprite
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/sprite/FreeTextureResource.s
 * (0x800156A4, 0xC0). Releases a sprite render resource's GPU + heap holdings:
 * points its +0x0C callback back at the freed-stub vtable (D_80010384); if the
 * resource owns a VRAM slot (+0x2F) it frees the texture page (+0x2E gate) and
 * any CLUT slot (+0x26 gate); and when flags bit0 is set, frees the resource
 * block itself from the heap.
 *   Texture rect: packedXY = u16@+0x10 | (u16@+0x12 << 16),
 *                 packedWH = u16@+0x14 | (u16@+0x16 << 16).
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern void FreeVRAMSlot(s16 *vramBase, u32 packedXY, u32 packedSize);
extern void FreeCLUTSlot(void *base, void *slot);
extern void FreeFromHeap(u8 *heap, u8 *ptr, s32 size, s32 flags);
extern u8   D_80010384[] asm("D_80010384");

void FreeTextureResource(void *resource, s32 flags) {
    u8 *r = (u8 *)resource;
    u8 *heap = (u8 *)g_pBlbHeapBase;

    *(void **)(r + 0x0C) = D_80010384;

    if (r[0x2F] != 0) {
        if (r[0x2E] != 0) {
            u32 packedXY = *(u16 *)(r + 0x10) | ((u32)*(u16 *)(r + 0x12) << 16);
            u32 packedWH = *(u16 *)(r + 0x14) | ((u32)*(u16 *)(r + 0x16) << 16);
            FreeVRAMSlot((s16 *)heap, packedXY, packedWH);
        }
        if (*(u16 *)(r + 0x26) != 0) {
            FreeCLUTSlot(heap, r + 0x18);
        }
    }

    if (flags & 1) {
        FreeFromHeap(heap, r, 0, 0);
    }
}
