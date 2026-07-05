/* =============================================================================
 * anim_palette_list.c  --  PC-port animated-palette list builder (TARGET_PC)
 * =============================================================================
 * Functional-C reconstruction of asm/nonmatchings/blb/AddPreInitEntitiesToList.s
 * (0x800250C8, 0x178 bytes). Despite the generic name this builds the level's
 * ANIMATED-PALETTE update list: for every palette group flagged animated
 * (GetPaletteAnimData()[group*4] & 1) it inserts that group's sprite-render
 * context (palArray[group], built by LoadTileDataToVRAM) into a priority-sorted
 * singly-linked list rooted at GameState+0x1C. Nodes are 8 bytes
 * {next @ +0x0, item @ +0x4}, allocated from the BLB heap. The list is kept
 * ascending by the context's s16 sort key at item+0x10 (insert before the first
 * node whose key >= the new item's key; append at the tail otherwise).
 *
 * Offsets/load-widths transcribed directly from the disassembly:
 *   GameState+0x114 = palette count (u8), +0x110 = palArray base ptr,
 *   +0x1C = list head. animData[group*4] bit0 = animated. item+0x10 = s16 key.
 * ========================================================================== */
#include "common.h"
#include "globals.h"
#include "Game/level_data_context.h"

extern u8 *GetPaletteAnimData(LevelDataContext *ctx);
extern u8 *AllocateFromHeap(u8 *heap, s32 align, s32 size, s32 flags);

typedef struct AnimPalNode {
    struct AnimPalNode *next;   /* +0x0 */
    void               *item;   /* +0x4  -> sprite-render context */
} AnimPalNode;

void AddPreInitEntitiesToList(void *arg0) {
    u8 *gs = (u8 *)arg0;
    u8 *heap = (u8 *)g_pBlbHeapBase;
    u8 *animData = GetPaletteAnimData((LevelDataContext *)(gs + 0x84));
    s32 i;

    if (animData == NULL) {
        return;
    }
    for (i = 0; i < (s32)*(u8 *)(gs + 0x114); i++) {
        void *item;
        s16 key;
        AnimPalNode *cur, *prev, *node;

        if (!(animData[i * 4] & 1)) {
            continue;
        }
        item = ((void **)(*(void **)(gs + 0x110)))[i];   /* palArray[i] */
        cur = *(AnimPalNode **)(gs + 0x1C);              /* list head   */

        if (cur == NULL) {
            node = (AnimPalNode *)AllocateFromHeap(heap, 8, 1, 0);
            *(AnimPalNode **)(gs + 0x1C) = node;
            node->next = NULL;
            node->item = item;
            continue;
        }

        prev = NULL;
        key = *(s16 *)((u8 *)item + 0x10);
        while (cur != NULL) {
            if (*(s16 *)((u8 *)cur->item + 0x10) < key) {
                prev = cur;
                cur = cur->next;
            } else {
                node = (AnimPalNode *)AllocateFromHeap(heap, 8, 1, 0);
                if (prev == NULL) {
                    *(AnimPalNode **)(gs + 0x1C) = node;
                } else {
                    prev->next = node;
                }
                node->next = cur;
                node->item = item;
                break;                                   /* inserted */
            }
        }
        if (cur == NULL) {
            /* walked off the end: append after prev */
            node = (AnimPalNode *)AllocateFromHeap(heap, 8, 1, 0);
            prev->next = node;
            node->next = NULL;
            node->item = item;
        }
    }
}
