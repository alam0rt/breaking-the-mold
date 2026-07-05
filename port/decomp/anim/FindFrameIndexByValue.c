/* =============================================================================
 * FindFrameIndexByValue.c  --  PC-port frame-tag lookup
 * =============================================================================
 * Functional-C reconstruction of FindFrameIndexByValue (0x8001E6AC, 0xE4;
 * export/SLES_010.90.c). Scans the entity's frame-metadata table (+0x78,
 * stride 0x24, count +0xD8) for the frame whose tag word (+0x00) equals
 * `targetValue`; static-sprite entities (+0xF7) go through the shared sheet
 * cache via GetSpriteFrameDataByIndex instead. Returns the index or -1.
 * ========================================================================== */
#include "common.h"
#include <stdint.h>

extern void *GetSpriteFrameDataByIndex(void *cacheRef, u32 index);

s32 FindFrameIndexByValue(void *arg0, s32 targetValue) {
    u8 *e = (u8 *)arg0;
    s16 count = *(s16 *)(e + 0xD8);
    u16 i;

    if (e[0xF7] == 0) {
        for (i = 0; (s16)i < count; i++) {
            if (*(s32 *)(*(u8 **)(e + 0x78) + (u32)i * 0x24) == targetValue) {
                return (s16)i;
            }
        }
    } else {
        for (i = 0; (s16)i < count; i++) {
            if (*(s32 *)GetSpriteFrameDataByIndex(e + 0x8C, i) == targetValue) {
                return (s16)i;
            }
        }
    }
    return -1;
}
