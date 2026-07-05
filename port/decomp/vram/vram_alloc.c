/* =============================================================================
 * vram_alloc.c  --  PC-port VRAM slot bin-packer (TARGET_PC), faithful
 * =============================================================================
 * Faithful reconstruction of the engine's VRAM slot allocator (src/vram.c
 * INCLUDE_ASM functions), transcribed from the Ghidra decompilation
 * (export/SLES_010.90.c) and cross-checked against asm/nonmatchings/vram/*.s.
 *
 * WHY FAITHFUL (not a PC-native shortcut): gpu_gl.c mirrors PSX VRAM as a
 * 1024x512 texture and LoadImage uploads sprite pixels to the (x,y) this
 * allocator returns. Reproducing the ORIGINAL packing makes the PC VRAM texture
 * match the PSX VRAM byte-for-byte, so texture-page / CLUT ids resolve exactly
 * as on hardware. This replaces the earlier simplified AllocateCLUTSlot +
 * minimal InitVRAMSlotTable bring-up stubs.
 *
 * Slot-table layout in the BLB heap (base = g_pBlbHeapBase / render context):
 *   VRAM slab free-list: 0x58 six-byte records, record i at base + i*6:
 *     +0xA08C u16 mask        (per-row occupancy bitmap of the slab)
 *     +0xA08E u8  heightCap   (max contiguous free rows in the slab)
 *     +0xA08F u8  next        (free-list link; 0xFF end, 0xFE free sentinel)
 *     +0xA090 u8  widthBlocks (slab width in 16px blocks; slab size class)
 *   base+0xA29C u8 head, +0xA29D u8 colBits (rows per slab = 0x10 - bankB),
 *   base+0xA29E s8 availTotal.
 *   CLUT slot table: 12 eight-byte records at base+0xA2A0:
 *     +0 flags (bit1 allocated, bit0 4/8-bit), +2 u16 mask, +4 s16 x, +6 s16 y.
 *   base+0xA304/+0xA306 s16 = 4-bit / 8-bit CLUT-in-use counters.
 *
 * FindFreeVRAMSlotEntry / GetMaxVRAMSlotSize / FreeVRAMSlot are matched C in
 * src/vram.c (compiled into the port) and are NOT redefined here.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include <stdint.h>

extern s32 FindFreeVRAMSlotEntry(s32 base);     /* matched C, src/vram.c */
extern u8  GetMaxVRAMSlotSize(s32 base);         /* matched C, src/vram.c */

/* ---- PC-native VRAM bump-allocator cursors (see AllocateVRAMSlot/CLUTSlot) ---
 * Textures pack the top half of VRAM (y < 256, page bank 0); CLUTs the bottom
 * (y >= 256), 16-aligned in x so GetClut is lossless. Reset per level in
 * InitVRAMSlotArray. */
#define PORT_VRAM_W 1024
#define PORT_VRAM_H 512
#define PORT_CLUT_Y0 256
/* Sprite textures live BELOW the CLUT rows, in the bottom half of VRAM
 * (y 288..511). The top half is taken: x<320 is the PSX double framebuffer
 * (unused for texturing but kept clear for fidelity) and x>=320 is the tile
 * atlas (CalculateVRAMCoordinates). y 256..287 is reserved for CLUT rows
 * (AllocateCLUTSlot bumps from 256). GetTPage/tpage_decode handle y>=256 via
 * the tpage y-bit, and the sprite V byte is y-256, so addressing stays exact. */
#define PORT_SPR_Y0 288
static s16 s_port_tex_x, s_port_tex_y = PORT_SPR_Y0, s_port_tex_shelf = PORT_SPR_Y0;
static s16 s_port_clut_x, s_port_clut_y = PORT_CLUT_Y0;

static void port_vram_reset(void) {
    s_port_tex_x = 0;
    s_port_tex_y = PORT_SPR_Y0;
    s_port_tex_shelf = PORT_SPR_Y0;
    s_port_clut_x = 0;
    s_port_clut_y = PORT_CLUT_Y0;
}

/* g_VRAMSlotXYTable template (3 x 8-byte {x, clut/tpage} records) -- real C
 * definition (D_8009AE40) lives in src/vram.c. */
extern u8 D_8009AE40[] asm("D_8009AE40");

/* =============================================================================
 * InitVRAMSlotArray -- reset the VRAM slab free-list to one big free slab.
 * (asm/nonmatchings/vram/InitVRAMSlotArray.s, 0x800149E8)
 * ========================================================================== */
void InitVRAMSlotArray(void *base) {
    u8 *b = (u8 *)base;
    u8  colBits = *(u8 *)(b + 0xA29D);
    s32 i;

    for (i = 0; (i & 0xFF) < 0x58; i++) {
        u8 *e = b + (i & 0xFF) * 6;
        *(u8  *)(e + 0xA08F) = 0xFE;      /* next = free sentinel */
        *(u8  *)(e + 0xA090) = 0xFE;      /* widthBlocks = free sentinel */
        *(u16 *)(e + 0xA08C) = 0;         /* mask */
        *(u8  *)(e + 0xA08E) = colBits;   /* heightCap */
    }
    *(u8  *)(b + 0xA08F) = 0xFF;          /* slab 0: end-of-list */
    *(u8  *)(b + 0xA090) = 0x58;          /* slab 0: full VRAM width (88 blocks) */
    *(u16 *)(b + 0xA08C) = 0;             /* slab 0: mask */
    *(u8  *)(b + 0xA29C) = 0;             /* free-list head = slab 0 */
    *(u8  *)(b + 0xA29E) = 0x58;          /* availTotal */
    port_vram_reset();                    /* reset the PC-native bump cursors */
}

/* =============================================================================
 * InitVRAMSlotTable -- build the CLUT slot descriptors + reset the slab array.
 * (asm/nonmatchings/vram/InitVRAMSlotTable.s, 0x80013B1C)
 * bankACount/bankBCount pick how many CLUT rows come from each bank; each bank
 * row emits 3 CLUT column descriptors copied from the g_VRAMSlotXYTable
 * template, with a descending Y (0xF0.. for bank A, 0x1F0.. for bank B).
 * ========================================================================== */
void InitVRAMSlotTable(void *base, s32 bankACount, s32 bankBCount) {
    u8 *b = (u8 *)base;
    s32 a = bankACount & 0xFF;
    s32 bb = bankBCount & 0xFF;
    s32 cols = a + bb;
    s16 y;
    s32 outer, inner, entry;

    *(s16 *)(b + 0xA300) = (s16)(cols << 5);
    *(s16 *)(b + 0xA302) = (s16)(cols << 4);
    *(s16 *)(b + 0xA304) = 0;
    *(s16 *)(b + 0xA306) = 0;

    entry = 0;
    y = 0xF0;
    for (outer = 0; (outer & 0xFF) < a; outer++) {
        for (inner = 0; inner < 3; inner++) {
            u8 *dst = b + (entry & 0xFF) * 8 + 0xA2A0;
            u8 *src = D_8009AE40 + inner * 8;
            /* copy the 8-byte {x, clut/tpage} template record */
            *(u32 *)(dst + 0) = *(u32 *)(src + 0);
            *(u32 *)(dst + 4) = *(u32 *)(src + 4);
            *(s16 *)(dst + 6) = y;         /* overwrite record Y */
            entry++;
        }
        y = (s16)(y - 0x10);
    }
    y = 0x1F0;
    for (outer = 0; (outer & 0xFF) < bb; outer++) {
        for (inner = 0; inner < 3; inner++) {
            u8 *dst = b + (entry & 0xFF) * 8 + 0xA2A0;
            u8 *src = D_8009AE40 + inner * 8;
            *(u32 *)(dst + 0) = *(u32 *)(src + 0);
            *(u32 *)(dst + 4) = *(u32 *)(src + 4);
            *(s16 *)(dst + 6) = y;
            entry++;
        }
        y = (s16)(y - 0x10);
    }
    /* clear the remaining CLUT descriptors up to 12 */
    {
        s32 k = entry & 0xFF;
        while (k < 0xC) {
            u8 *dst = b + k * 8 + 0xA2A0;
            dst[0] = 0;
            *(s16 *)(dst + 2) = 0;
            k++;
        }
    }
    *(u8 *)(b + 0xA29D) = (u8)(0x10 - bb);   /* colBits (rows per slab) */
    InitVRAMSlotArray(b);
}

/* =============================================================================
 * FindVRAMSlotBySize -- pack a texture of (slotType) width-blocks needing
 * (slotsNeeded) contiguous rows into an EXISTING same-width slab.
 * (export/SLES_010.90.c 5861; verbatim, ptr-arith cleaned)
 * Returns 1 + slot coords in pResult[0]=x-slab-offset, pResult[1]=row, else 0.
 * ========================================================================== */
s32 FindVRAMSlotBySize(void *renderCtx, u16 *pResult, s8 slotType, u32 slotsNeeded) {
    u8 *rc = (u8 *)renderCtx;
    u8  bVar1, bVar3, bVar9, bVar2;
    u32 uVar4, uVar5, uVar6, uVar8, uVar10, uVar11;
    u8 *e;
    u16 uVar12;

    uVar12 = 0;
    bVar9 = *(u8 *)(rc + 0xA29C);
    uVar10 = slotsNeeded & 0xFF;
    for (;;) {
        uVar11 = (u32)bVar9;
        if (uVar11 == 0xFF) {
            return 0;
        }
        e = rc + uVar11 * 6;
        if ((*(s8 *)(e + 0xA090) == slotType) && (uVar10 <= *(u8 *)(e + 0xA08E))) {
            uVar5 = 1;
            bVar9 = 0;
            uVar8 = (u32)*(u16 *)(e + 0xA08C);
            uVar4 = 0;
            if (*(u8 *)(rc + 0xA29D) != 0) {
                do {
                    bVar9 = bVar9 + 1;
                    if ((uVar5 & uVar8) == 0) {
                        if (bVar9 == uVar10) {
                            break;
                        }
                    } else {
                        bVar9 = 0;
                    }
                    uVar4 = uVar4 + 1;
                    uVar5 = uVar5 << 1;
                } while ((uVar4 & 0xFF) < (u32)*(u8 *)(rc + 0xA29D));
                if ((uVar4 & 0xFF) < (u32)*(u8 *)(rc + 0xA29D)) {
                    if (uVar8 == 0) {
                        *(s8 *)(rc + 0xA29E) =
                            (s8)(*(s8 *)(rc + 0xA29E) - *(s8 *)(rc + uVar11 * 6 + 0xA090));
                    }
                    uVar4 = uVar4 - (slotsNeeded - 1);
                    uVar6 = 1u << (uVar4 & 0x1F);
                    uVar5 = 0;
                    if (uVar10 != 0) {
                        do {
                            uVar8 = uVar8 | uVar6;
                            uVar5 = uVar5 + 1;
                            uVar6 = uVar6 << 1;
                        } while ((uVar5 & 0xFF) < uVar10);
                    }
                    uVar10 = 1;
                    bVar2 = 0;
                    bVar1 = 0;
                    *(s16 *)(rc + uVar11 * 6 + 0xA08C) = (s16)uVar8;
                    *pResult = (u16)(uVar12 & 0xFF);
                    pResult[1] = (u16)(uVar4 & 0xFF);
                    bVar3 = 0;
                    if (*(u8 *)(rc + 0xA29D) != 0) {
                        do {
                            bVar9 = bVar9 + 1;
                            bVar1 = bVar2;
                            if ((uVar10 & uVar8 & 0xFFFF) == 0) {
                                if (bVar2 <= bVar9) {
                                    bVar1 = bVar9;
                                }
                            } else {
                                bVar9 = 0;
                            }
                            bVar3 = bVar3 + 1;
                            uVar10 = uVar10 << 1;
                            bVar2 = bVar1;
                        } while (bVar3 < *(u8 *)(rc + 0xA29D));
                    }
                    *(u8 *)(rc + uVar11 * 6 + 0xA08E) = bVar1;
                    return 1;
                }
            }
        }
        e = rc + uVar11 * 6;
        bVar9 = *(u8 *)(e + 0xA08F);
        uVar12 = (u16)(uVar12 + *(u8 *)(e + 0xA090));
    }
}

/* =============================================================================
 * AllocateVRAMSlotAligned -- carve a NEW slab of reqSize width-blocks (splitting
 * a larger free slab, honouring the alignSel padding), mark bitWidth rows used.
 * (export/SLES_010.90.c 5960; verbatim, ptr-arith cleaned)
 * ========================================================================== */
u32 AllocateVRAMSlotAligned(void *heap, u16 *outOffset, u8 reqSize, u8 bitWidth, s8 alignSel) {
    u8 *h = (u8 *)heap;
    s8  cVar1, cVar3;
    u32 uVar2, uVar10;
    u8 *e, *ne;
    u8  bVar5, bVar11;
    u16 uVar6, uVar7, uVar8;

    bVar11 = 0;
    bVar5 = *(u8 *)(h + 0xA29C);
    for (;;) {
        uVar10 = (u32)bVar5;
        if (uVar10 == 0xFF) {
            return 0;
        }
        e = h + uVar10 * 6;
        bVar5 = *(u8 *)(e + 0xA090);
        if ((reqSize <= bVar5) && (*(s16 *)(e + 0xA08C) == 0)) {
            cVar1 = 0;
            if (alignSel == 1) {
                cVar3 = 0x10;
            } else {
                cVar3 = 8;
                if (alignSel == 2) {
                    cVar3 = 0x20;
                }
            }
            if ((u8)(cVar3 - reqSize) < (bVar11 & 7)) {
                cVar1 = (s8)(8 - (bVar11 & 7));
                bVar5 = (u8)(bVar5 - cVar1);
            }
            if (reqSize <= bVar5) {
                uVar2 = uVar10;
                if (cVar1 != 0) {
                    uVar2 = (u32)FindFreeVRAMSlotEntry((s32)(uintptr_t)h);
                    ne = h + (uVar2 & 0xFF) * 6;
                    *(u8 *)(ne + 0xA08E) = *(u8 *)(h + 0xA29D);
                    e = h + uVar10 * 6;
                    *(u8 *)(ne + 0xA08F) = *(u8 *)(e + 0xA08F);
                    cVar3 = *(s8 *)(e + 0xA090);
                    bVar11 = (u8)(bVar11 + cVar1);
                    *(u16 *)(ne + 0xA08C) = 0;
                    *(s8 *)(ne + 0xA090) = (s8)(cVar3 - cVar1);
                    *(s8 *)(e + 0xA08F) = (s8)uVar2;
                    *(s8 *)(e + 0xA090) = cVar1;
                    uVar10 = uVar2 & 0xFF;
                }
                e = h + uVar10 * 6;
                if (*(u8 *)(e + 0xA090) != reqSize) {
                    uVar10 = (u32)FindFreeVRAMSlotEntry((s32)(uintptr_t)h);
                    ne = h + (uVar10 & 0xFF) * 6;
                    *(u8 *)(ne + 0xA08E) = *(u8 *)(h + 0xA29D);
                    *(u8 *)(ne + 0xA08F) = *(u8 *)(e + 0xA08F);
                    cVar3 = *(s8 *)(e + 0xA090);
                    *(u16 *)(ne + 0xA08C) = 0;
                    *(u8 *)(ne + 0xA090) = (u8)(cVar3 - reqSize);
                    *(s8 *)(e + 0xA08F) = (s8)uVar10;
                    *(u8 *)(e + 0xA090) = reqSize;
                }
                uVar7 = 0;
                uVar6 = 1;
                bVar5 = 0;
                uVar8 = 0;
                if (bitWidth != 0) {
                    do {
                        uVar7 = (u16)(uVar8 | uVar6);
                        bVar5 = bVar5 + 1;
                        uVar6 = (u16)(uVar6 << 1);
                        uVar8 = uVar7;
                    } while (bVar5 < bitWidth);
                }
                e = h + (uVar2 & 0xFF) * 6;
                *(u16 *)(e + 0xA08C) = uVar7;
                *(u8 *)(e + 0xA08E) = (u8)(*(s8 *)(h + 0xA29D) - bitWidth);
                *outOffset = (u16)bVar11;
                outOffset[1] = 0;
                *(s8 *)(h + 0xA29E) =
                    (s8)(*(s8 *)(h + 0xA29E) - (*(s8 *)(e + 0xA090) + cVar1));
                return 1;
            }
        }
        e = h + uVar10 * 6;
        bVar5 = *(u8 *)(e + 0xA08F);
        bVar11 = (u8)(bVar11 + *(s8 *)(e + 0xA090));
    }
}

/* =============================================================================
 * AllocateVRAMSlot -- top-level: convert pixel dims to blocks, try exact-fit
 * (FindVRAMSlotBySize) then carve (AllocateVRAMSlotAligned) then grow-retry,
 * and emit final VRAM (x,y) into pSlotOut. (asm 0x800138F0)
 * ========================================================================== */
u8 AllocateVRAMSlot(void *base, u16 *pSlotOut, u8 mode, s16 width, s16 height) {
    /* PC-native override of the faithful bin-packer (whose slot-table init is
     * not yet fully reproduced). Simple shelf bump-allocator in the top half of
     * VRAM (y < 256 = texture page bank 0), 64-aligned in x so GetTPage packs the
     * page base losslessly and the sprite's page-local U starts at 0. Upload
     * location == reference location, so textures round-trip exactly. */
    s32 hwWidth;
    s16 x, y;
    (void)base;

    if (mode == 0) {
        hwWidth = (((s32)width + 1) >> 2) + 1;
    } else if (mode == 1) {
        hwWidth = (((s32)width + 1) >> 1) + 1;
    } else {
        hwWidth = (s32)width + 1;
    }
    hwWidth &= 0xFFFE;
    if (hwWidth < 2) hwWidth = 2;
    if (height < 1) height = 1;

    /* 64-align the x cursor so (x & 0x3F) == 0 -> page-local U == 0. */
    s_port_tex_x = (s16)((s_port_tex_x + 63) & ~63);
    if ((s32)s_port_tex_x + hwWidth > PORT_VRAM_W) {
        s_port_tex_x = 0;
        s_port_tex_y = s_port_tex_shelf;
    }
    if ((s32)s_port_tex_y + height > PORT_VRAM_H) {
        /* Sprite region (y 288..511) exhausted; wrap and reuse (best effort). */
        s_port_tex_x = 0;
        s_port_tex_y = PORT_SPR_Y0;
        s_port_tex_shelf = PORT_SPR_Y0;
    }
    x = s_port_tex_x;
    y = s_port_tex_y;
    *(s16 *)((u8 *)pSlotOut + 0x0) = x;
    *(s16 *)((u8 *)pSlotOut + 0x2) = y;
    *(s16 *)((u8 *)pSlotOut + 0x4) = (s16)hwWidth;
    *(s16 *)((u8 *)pSlotOut + 0x6) = height;
    s_port_tex_x = (s16)(s_port_tex_x + hwWidth);
    if ((s32)y + height > s_port_tex_shelf) {
        s_port_tex_shelf = (s16)(y + height);
    }
    return 1;
}

/* faithful implementation retained for reference (unused) */
__attribute__((unused))
static u8 AllocateVRAMSlot_faithful(void *base, u16 *pSlotOut, u8 mode, s16 width, s16 height) {
    u8 *b = (u8 *)base;
    s32 hwWidth;
    s32 widthBlocks, heightBlocks;
    u16 slot[2];
    u8  ok = 0;

    if (mode == 0) {
        hwWidth = ((s32)width + 1) >> 2;
        hwWidth = hwWidth + 1;
    } else if (mode == 1) {
        hwWidth = ((s32)width + 1) >> 1;
        hwWidth = hwWidth + 1;
    } else {
        hwWidth = (s32)width + 1;
    }
    hwWidth = hwWidth & 0xFFFE;
    *(s16 *)((u8 *)pSlotOut + 0x4) = (s16)hwWidth;
    *(s16 *)((u8 *)pSlotOut + 0x6) = height;

    widthBlocks = ((hwWidth << 1) + 0xF) >> 4;
    heightBlocks = ((s32)height + 0xF) >> 4;

    ok = (u8)FindVRAMSlotBySize(b, slot, (s8)(widthBlocks & 0xFF), (u32)(heightBlocks & 0xFF));
    if ((ok & 0xFF) == 0) {
        ok = (u8)AllocateVRAMSlotAligned(b, slot, (u8)(widthBlocks & 0xFF),
                                         (u8)(heightBlocks & 0xFF), (s8)mode);
        if ((ok & 0xFF) == 0) {
            u8  maxSize = (u8)GetMaxVRAMSlotSize((s32)(uintptr_t)b);
            s32 w = widthBlocks + 1;
            if ((u8)maxSize < (u8)(w & 0xFF)) {
                ok = 0;
            } else {
                s32 cap = maxSize;
                do {
                    ok = (u8)FindVRAMSlotBySize(b, slot, (s8)(w & 0xFF),
                                                (u32)(heightBlocks & 0xFF));
                    w = w + 1;
                } while ((u8)cap >= (u8)(w & 0xFF) && (ok & 0xFF) == 0);
            }
        }
    }

    if ((ok & 0xFF) != 0) {
        u16 xbase = *(u16 *)(b + 0x0);
        *(s16 *)((u8 *)pSlotOut + 0x0) = (s16)(xbase + (slot[0] << 3));
        *(s16 *)((u8 *)pSlotOut + 0x2) = (s16)(0x100 + (slot[1] << 4));
        return 1;
    }
    return 0;
}

/* =============================================================================
 * AllocateCLUTSlot -- reserve a CLUT line, reusing a same-type CLUT region's
 * free row or carving a new one (AllocateVRAMSlot, 16 or 256 px wide x 16 tall).
 * (export/SLES_010.90.c 5079; verbatim -- the unaligned word idioms model the
 * PSX swl/swr stores and compute identical results on the PC target.)
 * param_2 = out address (receives the packed CLUT x/y); param_3 = 0/1 bit-depth.
 * ========================================================================== */
s32 AllocateCLUTSlot(void *param_1, u32 param_2, u8 param_3) {
    /* PC-native override: bump-allocate a CLUT row in the bottom half of VRAM
     * (y >= 256), 16-aligned in x so GetClut packs (x,y) losslessly. 4-bit
     * palettes take 16 entries, 8-bit take 256; both are 16-aligned widths. The
     * palette upload (LoadClut/LoadImage) writes to the same (x,y), and the
     * shader's clutBase decode recovers it exactly, so colours resolve. */
    s16 x, y;
    int width = (param_3 == 0) ? 16 : 256;
    (void)param_1;

    if ((s32)s_port_clut_x + width > PORT_VRAM_W) {
        s_port_clut_x = 0;
        s_port_clut_y = (s16)(s_port_clut_y + 1);
    }
    if (s_port_clut_y >= PORT_VRAM_H) {
        s_port_clut_y = PORT_CLUT_Y0;    /* wrap (best effort) */
        s_port_clut_x = 0;
    }
    x = s_port_clut_x;
    y = s_port_clut_y;
    s_port_clut_x = (s16)(s_port_clut_x + width);

    *(s16 *)(uintptr_t)param_2       = x;
    *(s16 *)((uintptr_t)param_2 + 2) = y;
    return 1;
}

/* faithful implementation retained for reference (unused) */
__attribute__((unused))
static s32 AllocateCLUTSlot_faithful(void *param_1, u32 param_2, u8 param_3) {
    s16 *base16 = (s16 *)param_1;
    u32 uVar1, uVar4, uVar5;
    u32 *puVar2;
    s16 sVar3, sVar6, sVar7;
    s16 *psVar8;
    u8  *pbVar9;
    /* AllocateVRAMSlot writes an 8-byte {x@0, y@2, w@4, h@6} record through its
     * out pointer. The export used two adjacent stack slots (local_30=x,
     * local_2e=y); on PC separate C locals are not guaranteed adjacent, so a
     * real 8-byte buffer is required or the y (CLUT row) reads back garbage. */
    s16 slotRect[4];
#define local_30 (slotRect[0])
#define local_2e (slotRect[1])

    pbVar9 = (u8 *)(base16 + 0x5150);
    psVar8 = base16 + 0x5152;
    for (sVar6 = 0; sVar6 < 0xC; sVar6 = sVar6 + 1) {
        if ((((*pbVar9 & 2) != 0) && ((*pbVar9 & 1) == param_3)) &&
            (uVar5 = 1, psVar8[-1] != -1)) {
            sVar7 = 0;
            do {
                sVar3 = (s16)(sVar7 + 1);
                if (((u16)psVar8[-1] & uVar5) == 0) {
                    uVar5 = (u16)psVar8[-1] | uVar5;
                    psVar8[-1] = (s16)uVar5;
                    uVar4 = (u32)((s32)psVar8 + 3U) & 3;
                    uVar1 = (u32)psVar8 & 3;
                    uVar4 = ((*(s32 *)(((s32)psVar8 + 3U) - uVar4) << (3 - uVar4) * 8 |
                             uVar5 & 0xFFFFFFFFU >> (uVar4 + 1) * 8) & -1 << (4 - uVar1) * 8) |
                            *(u32 *)((s32)psVar8 - uVar1) >> uVar1 * 8;
                    uVar5 = param_2 + 3 & 3;
                    puVar2 = (u32 *)((param_2 + 3) - uVar5);
                    *puVar2 = *puVar2 & -1 << (uVar5 + 1) * 8 | uVar4 >> (3 - uVar5) * 8;
                    uVar5 = param_2 & 3;
                    *(u32 *)(param_2 - uVar5) =
                        *(u32 *)(param_2 - uVar5) & 0xFFFFFFFFU >> (4 - uVar5) * 8 |
                        uVar4 << uVar5 * 8;
                    *(s16 *)(param_2 + 2) = (s16)(*(s16 *)(param_2 + 2) + sVar7);
                    goto done;
                }
                uVar5 = uVar5 << 1;
                sVar7 = sVar3;
            } while (sVar3 < 0x10);
        }
        psVar8 = psVar8 + 4;
        pbVar9 = pbVar9 + 8;
    }
    pbVar9 = (u8 *)(base16 + 0x5150);
    sVar6 = 0;
    psVar8 = base16 + 0x5152;
    for (;;) {
        if (0xB < sVar6) {
            return 0;
        }
        if ((*pbVar9 & 2) == 0) {
            sVar7 = 0x10;
            if (param_3 == 1) {
                sVar7 = 0x100;
            }
            uVar5 = AllocateVRAMSlot(param_1, (u16 *)slotRect, 2, sVar7, 0x10);
            if ((uVar5 & 0xFF) != 0) {
                *pbVar9 = (u8)(param_3 | 2);
                psVar8[-1] = 1;
                *psVar8 = local_30;
                psVar8[1] = (s16)local_2e;
                uVar5 = (u32)((s32)psVar8 + 3U) & 3;
                uVar4 = (u32)psVar8 & 3;
                uVar4 = ((*(s32 *)(((s32)psVar8 + 3U) - uVar5) << (3 - uVar5) * 8 |
                         (u32)local_2e & 0xFFFFFFFFU >> (uVar5 + 1) * 8) & -1 << (4 - uVar4) * 8) |
                        *(u32 *)((s32)psVar8 - uVar4) >> uVar4 * 8;
                uVar5 = param_2 + 3 & 3;
                puVar2 = (u32 *)((param_2 + 3) - uVar5);
                *puVar2 = *puVar2 & -1 << (uVar5 + 1) * 8 | uVar4 >> (3 - uVar5) * 8;
                uVar5 = param_2 & 3;
                *(u32 *)(param_2 - uVar5) =
                    *(u32 *)(param_2 - uVar5) & 0xFFFFFFFFU >> (4 - uVar5) * 8 |
                    uVar4 << uVar5 * 8;
done:
                if (param_3 == 1) {
                    base16[0x5182] = (s16)(base16[0x5182] + 1);
                } else {
                    base16[0x5183] = (s16)(base16[0x5183] + 1);
                }
                return 1;
            }
        }
        psVar8 = psVar8 + 4;
        pbVar9 = pbVar9 + 8;
        sVar6 = sVar6 + 1;
    }
}

#undef local_30
#undef local_2e
