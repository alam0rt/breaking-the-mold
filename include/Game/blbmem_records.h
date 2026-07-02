#ifndef GAME_BLBMEM_RECORDS_H
#define GAME_BLBMEM_RECORDS_H

#include "common.h"

typedef struct BLBEntityHeader {
    u8 pad[0x18];
    void *vtable;
} BLBEntityHeader;

#endif /* GAME_BLBMEM_RECORDS_H */
