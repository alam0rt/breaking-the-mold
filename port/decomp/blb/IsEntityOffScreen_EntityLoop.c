/* =============================================================================
 * IsEntityOffScreen_EntityLoop.c  --  PC-port spawn-record offscreen predicate
 * =============================================================================
 * Functional-C body for IsEntityOffScreen_EntityLoop (INCLUDE_ASM in
 * src/blb.c). Despite the Ghidra name, this tests a spawn record's rect
 * {x1,y1,x2,y2} (four leading s16s) against the camera window with a 16px
 * margin. Screen w/h come from the s16 pair at the head of the display config
 * block g_pBlbHeapBase points at (same idiom as IsEntityOffScreen). Returns 1
 * when fully offscreen.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern void *g_pBlbHeapBase;

u32 IsEntityOffScreen_EntityLoop(u8 *gs, s16 *rec) {
    s32 camX = *(s16 *)(gs + 0x44);
    s32 camY = *(s16 *)(gs + 0x46);
    s32 scrW = *(s16 *)g_pBlbHeapBase;
    s32 scrH = *(s16 *)((u8 *)g_pBlbHeapBase + 2);

    if (rec[2] < camX - 0x10 ||
        camX + scrW + 0x10 < rec[0] ||
        rec[3] < camY - 0x10 ||
        camY + scrH + 0x10 < rec[1]) {
        return 1;
    }
    return 0;
}
