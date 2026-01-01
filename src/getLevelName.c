#include "common.h"

char *getLevelName(void *arg0, u8 levelIndex) {
    return (char *)(*(s32 *)((u8 *)arg0 + 0x5C) + (levelIndex * 0x70) + 0x5B);
}
