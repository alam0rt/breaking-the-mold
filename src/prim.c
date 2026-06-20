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

/* GPU primitive pool window inside the GpuContext. Each pool is a byte-array
 * of `cap * stride` followed by an s16 count and 2 bytes of alignment padding.
 * Offsets are verified -- bump-allocator pattern in every Alloc*Prim*. */
typedef struct {
    /* 0x0000 */ u8  _draw_envs[0x0074];           /* GPU draw/display envs preceding the pools */
    /* 0x0074 */ u8  pool0_data[200 * 20];          /* Pool 0: SPRT (most likely) */
    /* 0x1014 */ s16 pool0_count;
    /* 0x1016 */ s16 _pad1016;
    /* 0x1018 */ u8  pool1_data[200 * 8];           /* Pool 1: DR_TPAGE (likely) */
    /* 0x1658 */ s16 pool1_count;
    /* 0x165A */ s16 _pad165A;
    /* 0x165C */ u8  pool2_data[100 * 40];          /* Pool 2: POLY_FT4 / POLY_GT3 */
    /* 0x25FC */ s16 pool2_count;
    /* 0x25FE */ s16 _pad25FE;
    /* 0x2600 */ u8  pool3_data[100 * 20];          /* Pool 3: 20-byte (second SPRT slot) */
    /* 0x2DD0 */ s16 pool3_count;
    /* 0x2DD2 */ s16 _pad2DD2;
    /* 0x2DD4 */ u8  pool4_data[100 * 24];          /* Pool 4: POLY_F4 */
    /* 0x3734 */ s16 pool4_count;
    /* 0x3736 */ s16 _pad3736;
    /* 0x3738 */ u8  pool5_data[100 * 28];          /* Pool 5: POLY_G3 / LINE_F4 */
    /* 0x4228 */ s16 pool5_count;
    /* 0x422A */ s16 _pad422A;
    /* 0x422C */ u8  pool6_data[100 * 36];          /* Pool 6: POLY_G4 */
    /* 0x503C */ s16 pool6_count;
    /* 0x503E */ s16 _pad503E;
} GpuPrimitiveContext;

/*
 * Pool 0: 20-byte primitive slot (large pool, cap 200). Stride matches
 * PSY-Q SPRT or POLY_F3; given the sprite-heavy renderer this is most
 * likely the main SPRT pool used by world / actor draws.
 */
u8 *AllocPrim20(Globals *g) {
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool0_count;

    if (count == 200) {
        return NULL;
    }
    gpu->pool0_count = count + 1;
    pool = gpu->pool0_data;
    return pool + count * 20;
}

/*
 * Pool 1: 8-byte primitive slot (cap 200). PSY-Q DR_TPAGE is the only
 * common 8-byte packet; this pool almost certainly holds the tpage /
 * drawmode switches AddPrim'd between sprite batches.
 */
u8 *AllocPrim8(Globals *g) {
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool1_count;

    if (count == 200) {
        return NULL;
    }
    gpu->pool1_count = count + 1;
    pool = gpu->pool1_data;
    return pool + count * 8;
}

/*
 * Pool 2: 40-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_FT4 (textured quad) or POLY_GT3; the FT4 reading fits a 2D
 * platformer's tilemap / large-sprite quads.
 */
u8 *AllocPrim40(Globals *g) {
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool2_count;

    if (count == 100) {
        return NULL;
    }
    gpu->pool2_count = count + 1;
    pool = gpu->pool2_data;
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
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool3_count;

    if (count == 100) {
        return NULL;
    }
    gpu->pool3_count = count + 1;
    pool = gpu->pool3_data;
    return pool + count * 20;
}

/*
 * Pool 4: 24-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_F4 (flat-shaded untextured quad) - the usual primitive for
 * solid-color fills, fade rects, and HUD backdrops.
 */
u8 *AllocPrim24(Globals *g) {
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool4_count;

    if (count == 100) {
        return NULL;
    }
    gpu->pool4_count = count + 1;
    pool = gpu->pool4_data;
    return pool + count * 24;
}

/*
 * Pool 5: 28-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_G3 (gouraud-shaded untextured triangle); could also be LINE_F4.
 */
u8 *AllocPrim28(Globals *g) {
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool5_count;

    if (count == 100) {
        return NULL;
    }
    gpu->pool5_count = count + 1;
    pool = gpu->pool5_data;
    return pool + count * 28;
}

/*
 * Pool 6: 36-byte primitive slot (cap 100). Stride matches PSY-Q
 * POLY_G4 (gouraud-shaded untextured quad) - used for shaded
 * backgrounds, palette-tinted overlays, or gradient fills.
 */
u8 *AllocPrim36(Globals *g) {
    GpuPrimitiveContext *gpu = (GpuPrimitiveContext *)g->gpu;
    u8 *pool;
    s16 count = gpu->pool6_count;

    if (count == 100) {
        return NULL;
    }
    gpu->pool6_count = count + 1;
    pool = gpu->pool6_data;
    return pool + count * 36;
}
