/* =============================================================================
 * FindOrderingTableSlotById.c  --  PC-port sprite-frame-cache lookup by id
 * =============================================================================
 * Faithful transcription of export/SLES_010.90.c FindOrderingTableSlotById
 * (0x800195A0, INCLUDE_ASM in src/layer.c). Linear-searches the 20-entry sprite
 * frame cache (0x18-byte descriptors at g_pOrderingTableBase = D_800A595C, the
 * table LoadSpriteFramesToVRAM populates) for the entry whose id (first word)
 * matches, and writes that entry pointer (or NULL if absent) to *outSlot.
 *
 * g_pOrderingTableBase is the same .sdata pointer (D_800A595C) seeded at boot in
 * game_boot.c (as g_LayerRenderSlots), so the cache is already backed here.
 * ---------------------------------------------------------------------------*/
#include "common.h"

extern u8 *g_pOrderingTableBase asm("D_800A595C");

void FindOrderingTableSlotById(void *outSlot, int id) {
    u8 *base = g_pOrderingTableBase;
    int idx = 0;
    int *entry;

    do {
        entry = (int *)(base + idx * 0x18);
        idx++;
        if (*entry == id) {
            *(void **)outSlot = entry;
            return;
        }
    } while (idx < 0x14);

    *(void **)outSlot = (void *)0;
}
