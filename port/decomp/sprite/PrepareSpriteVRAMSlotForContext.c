/* =============================================================================
 * PrepareSpriteVRAMSlotForContext.c  --  PC-port sprite VRAM-slot setup
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/sprite/PrepareSpriteVRAMSlotFor
 * Context.s (0x8001546C, 0x1A8 bytes). Initialises the 0x3C-byte sprite render
 * context (vtable D_80010384, white tint 0x40, unit scales 0x10000, dims), then
 * reserves a VRAM slot (AllocateVRAMSlot into ctx+0x10) and, unless the sprite is
 * 16-bit (mode==2), a CLUT slot (AllocateCLUTSlot into ctx+0x18). On a successful
 * VRAM reservation it derives the texture page (GetTPage over the slot rounded to
 * 64x256) and the in-page U coordinate (VRAM x & 0x3F shifted by bit-depth:
 * <<2 for 4-bit/mode0, <<1 for 8-bit/mode1, <<0 for 16-bit/mode>=2); the CLUT id
 * comes from GetClut over the reserved CLUT rect. Returns the context pointer.
 *
 * Context field map (from the disassembly): +0x04 width, +0x06 height, +0x08
 * zOrder, +0x0A active=1, +0x0C vtable, +0x10/+0x12 VRAM x/y (slot),
 * +0x18/+0x1A CLUT x/y (slot), +0x1C/+0x20 scaleX/Y=0x10000, +0x24 tpage,
 * +0x26 clut id, +0x2E vram-ok flag, +0x2F=1, +0x30 U, +0x31 V(low), +0x32 mode,
 * +0x33=1, +0x34..0x36 tint=0x40.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern u8      AllocateVRAMSlot(void *base, u16 *outRect, u8 mode, s16 w, s16 h);
extern s32     AllocateCLUTSlot(void *base, u32 out, u8 type);
extern u16     GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern u16     GetClut(s32 x, s32 y);

/* Prim/sprite base vtables (weak-backed rodata; installed then finalised). */
extern u8 D_8001039C[] asm("D_8001039C");
extern u8 D_80010384[] asm("D_80010384");

u8 *PrepareSpriteVRAMSlotForContext(void *param_1, s16 zOrder, s16 width, s16 height, u8 mode) {
    u8 *c = (u8 *)param_1;
    u8 *heap = (u8 *)g_pBlbHeapBase;

    *(s16 *)(c + 0x08) = zOrder;
    *(void **)(c + 0x0C) = D_8001039C;
    *(s16 *)(c + 0x00) = 0;
    *(s16 *)(c + 0x02) = 0;
    *(s16 *)(c + 0x04) = 0;
    *(s16 *)(c + 0x06) = 0;
    *(void **)(c + 0x0C) = D_80010384;
    c[0x34] = 0x40; c[0x35] = 0x40; c[0x36] = 0x40;
    *(s16 *)(c + 0x06) = height;
    c[0x0A] = 1;
    c[0x37] = 0; c[0x38] = 0;
    c[0x2F] = 1;
    *(s16 *)(c + 0x04) = width;
    c[0x33] = 1;
    c[0x32] = mode;
    *(s32 *)(c + 0x1C) = 0x10000;
    *(s32 *)(c + 0x20) = 0x10000;
    *(s16 *)(c + 0x28) = 0;
    *(s16 *)(c + 0x2A) = 0;
    *(s16 *)(c + 0x2C) = 0;

    c[0x2E] = AllocateVRAMSlot(heap, (u16 *)(c + 0x10), c[0x32], width, height);
    if ((c[0x2E] & 0xFF) != 0) {
        s16 vx = *(s16 *)(c + 0x10);
        s16 vy = *(s16 *)(c + 0x12);
        u8  m  = c[0x32];
        u32 u;
        *(s16 *)(c + 0x24) = (s16)GetTPage((s32)m, 0, vx & ~0x3F, vy & ~0xFF);
        if (m == 0) {
            u = (u32)(*(u8 *)(c + 0x10) & 0x3F) << 2;
        } else if (m == 1) {
            u = (u32)(*(u8 *)(c + 0x10) & 0x3F) << 1;
        } else {
            u = (u32)(*(u8 *)(c + 0x10) & 0x3F);
        }
        c[0x30] = (u8)u;
        c[0x31] = (u8)*(u16 *)(c + 0x12);
    }

    if (c[0x32] != 2) {
        if ((AllocateCLUTSlot(heap, (u32)(uintptr_t)(c + 0x18), c[0x32]) & 0xFF) != 0) {
            *(s16 *)(c + 0x26) = (s16)GetClut(*(s16 *)(c + 0x18), *(s16 *)(c + 0x1A));
            return c;
        }
    }
    *(s16 *)(c + 0x26) = 0;
    return c;
}
