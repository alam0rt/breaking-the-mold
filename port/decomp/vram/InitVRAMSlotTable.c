/* =============================================================================
 * InitVRAMSlotTable.c  --  PC-port VRAM slot-table init (TARGET_PC)
 * =============================================================================
 * PC replacement for asm/nonmatchings/vram/InitVRAMSlotTable.s (0x80013B1C).
 * The original sets up the VRAM texture-atlas allocator: a header (slot bounds)
 * at base+0xA300 and a table of 8-byte slot descriptors at base+0xA2A0, copied
 * from a template (D_8009AE40) and finalised by InitVRAMSlotArray.
 *
 * On PC, VRAM is a single 1024x512 GL texture (port/spec/gpu_gl.c) and sprite
 * pixels are uploaded via LoadImage. This routine only needs to populate the
 * allocator HEADER so the slot bookkeeping struct is well-formed; the full slot
 * layout + InitVRAMSlotArray is part of the VRAM-allocator subsystem, converted
 * as a unit when the sprite loaders (AllocateVRAMSlot/AllocateCLUTSlot) are
 * first reached. Since port_heap.c zero-fills the heap, the unwritten slot
 * entries read as empty until then.
 *
 * Header fields (from the disassembly, base = arg0, cols = a1 + a2):
 *   base+0xA300 s16 = cols << 5      base+0xA302 s16 = cols << 4
 *   base+0xA304 s16 = 0              base+0xA306 s16 = 0
 *   base+0xA29D u8  = 0x10 - a2
 * ========================================================================== */
#include "common.h"

void InitVRAMSlotTable(void *base, int a1, int a2) {
    u8 *b = (u8 *)base;
    int cols = (a1 & 0xFF) + (a2 & 0xFF);

    *(s16 *)(b + 0xA300) = (s16)(cols << 5);
    *(s16 *)(b + 0xA302) = (s16)(cols << 4);
    *(s16 *)(b + 0xA304) = 0;
    *(s16 *)(b + 0xA306) = 0;
    *(u8  *)(b + 0xA29D) = (u8)(0x10 - (a2 & 0xFF));
}
