#include "common.h"

/*
 * Primitive Pool Allocators
 *
 * Seven bump allocators for GPU primitive buffers of different sizes.
 * Each function takes a GameGlobals pointer, accesses the GpuContext at
 * offset 0xA084, checks a per-pool count against its maximum, and returns
 * a pointer into the pool array (or NULL if full).
 *
 * Pool layout within GpuContext:
 *   Pool 0: 200 x 20-byte entries at 0x0074, count at 0x1014
 *   Pool 1: 200 x  8-byte entries at 0x1018, count at 0x1658
 *   Pool 2: 100 x 40-byte entries at 0x165C, count at 0x25FC
 *   Pool 3: 100 x 20-byte entries at 0x2600, count at 0x2DD0
 *   Pool 4: 100 x 24-byte entries at 0x2DD4, count at 0x3734
 *   Pool 5: 100 x 28-byte entries at 0x3738, count at 0x4228
 *   Pool 6: 100 x 36-byte entries at 0x422C, count at 0x503C
 */

typedef struct Globals {
    u8 pad[0xA084];
    void *gpu;
} Globals;

void *AllocPrim20(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x1014);

    if (count == 200) {
        return NULL;
    }
    *(s16 *)(pool + 0x1014) = count + 1;
    pool += 0x74;
    return pool + count * 20;
}

void *AllocPrim8(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x1658);

    if (count == 200) {
        return NULL;
    }
    *(s16 *)(pool + 0x1658) = count + 1;
    pool += 0x1018;
    return pool + count * 8;
}

void *AllocPrim40(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x25FC);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x25FC) = count + 1;
    pool += 0x165C;
    return pool + count * 40;
}

void *AllocPrim20_Pool3(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x2DD0);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x2DD0) = count + 1;
    pool += 0x2600;
    return pool + count * 20;
}

void *AllocPrim24(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x3734);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x3734) = count + 1;
    pool += 0x2DD4;
    return pool + count * 24;
}

void *AllocPrim28(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x4228);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x4228) = count + 1;
    pool += 0x3738;
    return pool + count * 28;
}

void *AllocPrim36(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x503C);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x503C) = count + 1;
    pool += 0x422C;
    return pool + count * 36;
}
