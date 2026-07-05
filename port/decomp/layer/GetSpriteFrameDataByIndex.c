/* =============================================================================
 * GetSpriteFrameDataByIndex.c  --  PC-port static-sprite frame record lookup
 * =============================================================================
 * Faithful transcription of asm/nonmatchings/layer/GetSpriteFrameDataByIndex.s
 * (0x80019650, INCLUDE_ASM in src/layer.c). For a static (non-animated) sprite,
 * resolves the pointer to a frame's 0x24-byte metadata record: dereference the
 * sprite-asset slot, read its frame-table base at +4, index it by frameIndex
 * (0x14-byte entries), and return that entry's first word -- which is the
 * pointer to the actual metadata record consumed by UpdateSpriteFrameData.
 * ---------------------------------------------------------------------------*/
#include "common.h"

void *GetSpriteFrameDataByIndex(void *pFrameData, u32 frameIndex) {
    u8 *asset = *(u8 **)pFrameData;
    u8 *frameTable = *(u8 **)(asset + 4);
    return *(void **)(frameTable + (frameIndex & 0xFFFF) * 0x14);
}
