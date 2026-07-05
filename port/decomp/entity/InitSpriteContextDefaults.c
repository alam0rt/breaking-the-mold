/* =============================================================================
 * InitSpriteContextDefaults.c  --  PC-port sprite render-context defaults
 * =============================================================================
 * Functional-C reconstruction of InitSpriteContextDefaults (asm at 0x80015618;
 * reference = export/SLES_010.90.c 6556). Initialises a sprite
 * render context (BasicEntity-derived, >=0x3A bytes): installs the PrimObject
 * base vtable at +0xC, sets the default white tint (+0x34/0x35/0x36 = 0x40),
 * the alloc/init size at +0x8, active=1 (+0xA), the two 16.16 scale fields at
 * +0x1C/+0x20 = 1.0, and clears the remaining position/texture/CLUT fields.
 * The vtable slot at +0xC is written twice (D_8001039C then D_80010384) exactly
 * as the original; the installed vtable is D_80010384. Returns the context
 * pointer in $v0 -- the matched-C caller AllocateSpriteContext relies on it
 * (`entity->spriteContext = InitSpriteContextDefaults(ctx, size)`); the export's
 * `void` return type is wrong.
 * Offsets/widths transcribed from the disassembly.
 * ========================================================================== */
#include "common.h"

/* Sprite/prim vtables at fixed rodata (weak-backed on PC). */
extern u8 D_8001039C[] asm("D_8001039C");   /* PrimObject base vtable */
extern u8 D_80010384[] asm("D_80010384");   /* sprite render vtable   */

void *InitSpriteContextDefaults(void *pSpriteContext, u16 initSize) {
    u8 *c = (u8 *)pSpriteContext;

    *(void **)(c + 0x0C) = D_8001039C;
    *(void **)(c + 0x0C) = D_80010384;
    c[0x34] = 0x40;
    c[0x35] = 0x40;
    c[0x36] = 0x40;
    *(u16 *)(c + 0x08) = initSize;
    *(u16 *)(c + 0x00) = 0;
    *(u16 *)(c + 0x02) = 0;
    *(u16 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x06) = 0;
    c[0x0A] = 1;
    c[0x37] = 0;
    c[0x38] = 0;
    c[0x2F] = 0;
    *(u16 *)(c + 0x04) = 0;
    *(u16 *)(c + 0x06) = 0;
    c[0x32] = 0;
    c[0x33] = 1;
    *(s32 *)(c + 0x1C) = 0x10000;
    *(s32 *)(c + 0x20) = 0x10000;
    *(u16 *)(c + 0x28) = 0;
    *(u16 *)(c + 0x2A) = 0;
    *(u16 *)(c + 0x2C) = 0;
    c[0x2E] = 0;
    *(u16 *)(c + 0x24) = 0;
    c[0x30] = 0;
    c[0x31] = 0;
    return pSpriteContext;
}
