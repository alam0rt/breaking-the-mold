#ifndef GAME_VRAM_RECORDS_H
#define GAME_VRAM_RECORDS_H

#include "common.h"

typedef struct {
    u8 _pad0000[0xA640];
    void *heapStart;
    u32 heapSize;
} HeapConfigOwner;

#endif /* GAME_VRAM_RECORDS_H */
