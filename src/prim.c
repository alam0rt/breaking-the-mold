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

/*
 * Pool 0: 20-byte primitive slot (large pool, cap 200). Stride matches
 * PSY-Q SPRT or POLY_F3; given the sprite-heavy renderer this is most
 * likely the main SPRT pool used by world / actor draws.
 */
u8 *AllocPrim20(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x1014);

    if (count == 200) {
        return NULL;
    }
    *(s16 *)(pool + 0x1014) = count + 1;
    pool += 0x74;
    return pool + count * 20;
}

/*
 * Pool 1: 8-byte primitive slot (cap 200). PSY-Q DR_TPAGE is the only
 * common 8-byte packet; this pool almost certainly holds the tpage /
 * drawmode switches AddPrim'd between sprite batches.
 */
u8 *AllocPrim8(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x1658);

    if (count == 200) {
        return NULL;
    }
    *(s16 *)(pool + 0x1658) = count + 1;
    pool += 0x1018;
    return pool + count * 8;
}

/*
 * Pool 2: 40-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_FT4 (textured quad) or POLY_GT3; the FT4 reading fits a 2D
 * platformer's tilemap / large-sprite quads.
 */
u8 *AllocPrim40(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x25FC);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x25FC) = count + 1;
    pool += 0x165C;
    return pool + count * 40;
}

/*
 * Pool 3: second 20-byte primitive slot (cap 100). Same stride as Pool 0
 * but a separate buffer + count, suggesting a distinct usage class
 * (e.g. UI/HUD sprites kept apart from world sprites, or SPRT vs.
 * POLY_F3). The "_Pool3" suffix is purely to disambiguate from
 * AllocPrim20 - it does not reflect any original naming.
 */
u8 *AllocPrim20_Pool3(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x2DD0);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x2DD0) = count + 1;
    pool += 0x2600;
    return pool + count * 20;
}

/*
 * Pool 4: 24-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_F4 (flat-shaded untextured quad) - the usual primitive for
 * solid-color fills, fade rects, and HUD backdrops.
 */
u8 *AllocPrim24(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x3734);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x3734) = count + 1;
    pool += 0x2DD4;
    return pool + count * 24;
}

/*
 * Pool 5: 28-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_G3 (gouraud-shaded untextured triangle); could also be LINE_F4.
 */
u8 *AllocPrim28(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x4228);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x4228) = count + 1;
    pool += 0x3738;
    return pool + count * 28;
}

/*
 * Pool 6: 36-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_G4 (gouraud-shaded untextured quad) - used for shaded
 * backgrounds, palette-tinted overlays, or gradient fills.
 */
u8 *AllocPrim36(Globals *g) {
    u8 *pool = (u8 *)g->gpu;
    s16 count = *(s16 *)(pool + 0x503C);

    if (count == 100) {
        return NULL;
    }
    *(s16 *)(pool + 0x503C) = count + 1;
    pool += 0x422C;
    return pool + count * 36;
}
