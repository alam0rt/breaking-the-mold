/* =============================================================================
 * FindSpriteInTOC.c  --  PC-port level-TOC sprite lookup (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blbacc/FindSpriteInTOC.s
 * (reference = export/SLES_010.90.c 107312). Searches the level's two sprite
 * tables-of-contents (LevelDataContext+0x70 then +0x40) for a sprite id. Each
 * TOC is a word array: [0] = entry count, then entries of stride 3 words where
 * entry[+1] = sprite id and entry[+3] = the sprite header's byte offset from the
 * TOC base. Returns TOC_base + entry[+3] on a hit, else NULL.
 * ========================================================================== */
#include "common.h"

static void *toc_search(u8 *ctx, s32 fieldOff, u32 spriteId) {
    u32 *toc = *(u32 **)(ctx + fieldOff);
    if (toc != NULL && toc[0] != 0) {
        u32 i;
        for (i = 0; (i & 0xFFFF) < toc[0]; i++) {
            u32 *entry = toc + (i & 0xFFFF) * 3;
            if (entry[1] == spriteId) {
                return (u8 *)toc + entry[3];
            }
        }
    }
    return NULL;
}

void *FindSpriteInTOC(void *param_1, u32 spriteId) {
    u8 *ctx = (u8 *)param_1;
    void *hit = toc_search(ctx, 0x70, spriteId);
    if (hit != NULL) {
        return hit;
    }
    return toc_search(ctx, 0x40, spriteId);
}
