#include "common.h"

u8 GetAssetCount(void *gameState) {
    return ((u8 *)(*(s32 *)((u8 *)gameState + 0x5C)))[0xF32];
}
