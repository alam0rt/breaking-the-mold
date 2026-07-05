/* =============================================================================
 * LookupSpriteById.c  --  PC-port sprite-id resolver (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blbacc/LookupSpriteById.s
 * (0x8007BB10, 0xB0 bytes; reference = export/SLES_010.90.c 107595). Resolves a
 * sprite hash to its asset-header pointer. First tries the current level's TOC
 * (FindSpriteInTOC over SPRITE_DATA_TABLE @ D_800A6064, set by SetSpriteTables);
 * on a miss, linearly scans the secondary sprite bank (SPRITE_TOC_TABLE @
 * D_800A6060): id array at bank+0x10 (count at bank+0xC), and the matching
 * entry's header = bank + *(u32*)(bank + count*4 + 0x10 + i*0x14 + 8).
 * Returns the header pointer, or NULL when unresolved.
 * ========================================================================== */
#include "common.h"

extern u8 *SPRITE_DATA_TABLE asm("D_800A6064");   /* level LevelDataContext ptr */
extern u8 *SPRITE_TOC_TABLE  asm("D_800A6060");   /* secondary sprite bank      */

extern void *FindSpriteInTOC(void *levelCtx, u32 spriteId);

void *LookupSpriteById(u32 spriteId) {
    u8 *bank;
    u32 count;
    u32 *ids;
    u8 *offTable;
    u32 i;

    if (SPRITE_DATA_TABLE != NULL) {
        void *hit = FindSpriteInTOC(SPRITE_DATA_TABLE, spriteId);
        if (hit != NULL) {
            return hit;
        }
    }

    bank = SPRITE_TOC_TABLE;
    if (bank == NULL) {
        return NULL;
    }
    count = *(u32 *)(bank + 0xC);
    ids = (u32 *)(bank + 0x10);
    offTable = bank + count * 4 + 0x10;
    for (i = 0; i < count; i++) {
        if (ids[i] == spriteId) {
            return bank + *(u32 *)(offTable + i * 0x14 + 8);
        }
    }
    return NULL;
}
