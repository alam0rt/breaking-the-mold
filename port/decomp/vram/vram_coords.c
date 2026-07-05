/* =============================================================================
 * vram_coords.c  --  PC-port VRAM texture-region allocators (TARGET_PC)
 * =============================================================================
 * CalculateVRAMCoordinates is faithfully ported from
 * asm/nonmatchings/vram/CalculateVRAMCoordinates.s (verified offset-by-offset):
 * a linear bump allocator over the VRAM texture region. AllocateCLUTSlot is a
 * SIMPLIFIED PC-native replacement (the real 700-line VRAM bin-packer is only
 * needed for sprite allocation; on PC, VRAM is a full 1024x512 GL texture, so a
 * trivial non-overlapping bump allocator suffices -- see docs/plans/pc-port.md
 * CP-2.2 "use a simplified PC-native atlas allocator").
 *
 * VRAM LAYOUT (PSX 1024x512, mirrored by gpu_gl.c's s_vram texture):
 *   framebuffer 0 @ (0,0,320,256), framebuffer 1 @ (0,256,320,256)
 *   tile textures @ X>=320 (heap[0]=0x140), via CalculateVRAMCoordinates
 *   CLUTs @ a dedicated band at X>=896 (this file)
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern u_short GetClut(int x, int y);

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

/* AllocateCLUTSlot -- SIMPLIFIED PC-native CLUT allocator.
 * The game's real AllocateCLUTSlot bin-packs CLUTs into a slot table at
 * heap+0xA2A0 (backed by the 700-line VRAM allocator). On PC we just bump-
 * allocate CLUTs into a reserved band at X>=896, 16px wide (4-bit CLUT) or 256px
 * (8-bit), 1px tall, wrapping down rows. Output contract (matches the real one):
 *   out[0] = CLUT X (u16), out[2] = CLUT Y (u16).
 * mode (arg2): 0/2 => 4-bit CLUT (16 wide); 1 => 8-bit CLUT (256 wide, rare here).
 * The exact packing differs from PSX but is consistent + non-overlapping, which
 * is all the render path needs (GetClut encodes it, gpu_gl.c decodes it back). */
static u16 s_clut_x = 896;   /* CLUT band start X (past tiles) */
static u16 s_clut_y = 496;   /* CLUT band start Y (bottom rows) */

s32 AllocateCLUTSlot(void *base, void *out, s32 mode) {
    u16 width = (mode == 1) ? 256 : 16;
    (void)base;
    if ((u32)s_clut_x + width > 1024) {
        s_clut_x = 896;
        s_clut_y = (u16)(s_clut_y + 1);
        if (s_clut_y >= 512) {
            s_clut_y = 496;    /* wrap (bring-up: CLUT band recycles) */
        }
    }
    *(u16 *)((u8 *)out + 0x0) = s_clut_x;
    *(u16 *)((u8 *)out + 0x2) = s_clut_y;
    s_clut_x = (u16)(s_clut_x + width);
    return 0;   /* 0 == success (matches the real allocator's return contract) */
}

/* FreeCLUTSlot / FreeVRAMSlot -- no-ops on the simplified bump allocator (the
 * CLUT/tile bands recycle per level load, which is a fresh heap on PC). */
void FreeCLUTSlot(void *base, void *slot) { (void)base; (void)slot; }
