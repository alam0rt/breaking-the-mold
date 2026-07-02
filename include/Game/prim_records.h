#ifndef GAME_PRIM_RECORDS_H
#define GAME_PRIM_RECORDS_H

#include "common.h"

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

#endif /* GAME_PRIM_RECORDS_H */
