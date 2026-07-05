/* =============================================================================
 * vram_coords.c  --  PC-port VRAM texture-region allocators (TARGET_PC)
 * =============================================================================
 * CalculateVRAMCoordinates is faithfully ported from
 * asm/nonmatchings/vram/CalculateVRAMCoordinates.s (verified offset-by-offset):
 * a linear bump allocator over the VRAM texture region used by the TILE path.
 * The sprite VRAM slot bin-packer (AllocateVRAMSlot / AllocateCLUTSlot / the
 * slab free-list + CLUT slot table) now lives faithfully in vram_alloc.c.
 *
 * VRAM LAYOUT (PSX 1024x512, mirrored by gpu_gl.c's s_vram texture):
 *   framebuffer 0 @ (0,0,320,256), framebuffer 1 @ (0,256,320,256)
 *   tile textures @ X>=320 (heap[0]=0x140), via CalculateVRAMCoordinates
 * ========================================================================== */
#include "common.h"
#include "globals.h"

/* CalculateVRAMCoordinates @ 0x80014278: bump-allocate a tile cell in the VRAM
 * texture region. The cursor is a u16 at heap+0xA08A counting 4px-wide x 16px-
 * tall cells across 176 columns (704px of texture area) x 32 rows. Writes the
 * pixel X to out[0] and Y to out[2]; advances the cursor by 1/2/4 cells for
 * size 0/1/2. Returns the pre-advance cursor. Verified against the .s. */
s16 CalculateVRAMCoordinates(void *base, void *out, s32 size) {
    u8 *b = (u8 *)base;
    u16 *cursorp = (u16 *)(b + 0xA08A);
    u16 cursor = *cursorp;
    s32 sz = size & 0xFF;
    s32 row = cursor / 176;
    s32 col = cursor % 176;

    /* Wrap to the next row if this cell won't fit (col limits 0xAF / 0xAD). */
    if ((sz == 1 && (col & 0xFF) < 0xAF) ||
        (sz == 2 && (col & 0xFF) < 0xAD)) {
        /* fits -- no wrap */
    } else if (sz == 1 || sz == 2) {
        cursor = (u16)(cursor - ((col & 0xFF) - 0xB0));
        col = 0;
        row = (row + 1) & 0xFF;
    }

    *(s16 *)((u8 *)out + 0x2) = (s16)((row & 0xFF) * 0x10);            /* Y */
    *(s16 *)((u8 *)out + 0x0) = (s16)(*(u16 *)(b + 0x0) + (col & 0xFF) * 4); /* X */

    if (sz == 0) {
        *cursorp = (u16)(cursor + 1);
    } else if (sz == 1) {
        *cursorp = (u16)(cursor + 2);
    } else {
        *cursorp = (u16)(cursor + 4);
    }
    return (s16)cursor;
}

/* FreeCLUTSlot -- no-op bring-up stub (the CLUT slot table recycles per level
 * load, which is a fresh heap on PC). AllocateCLUTSlot now lives faithfully in
 * vram_alloc.c. */
void FreeCLUTSlot(void *base, void *slot) { (void)base; (void)slot; }
