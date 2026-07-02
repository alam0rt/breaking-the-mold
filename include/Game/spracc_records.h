#ifndef GAME_SPRACC_RECORDS_H
#define GAME_SPRACC_RECORDS_H

#include "common.h"

/*
 * Tiny accessor functions for the sprite render object
 * (0x80018BD8 - 0x80018D4B, tail of Game/RENDER_5C3C).
 *
 * Field layout is only what these accessors touch; the full struct is
 * not yet known, so a local view is used (same approach as
 * src/assets/blb_memory.c).
 */

typedef struct RenderSprite {
    u8 pad00[0x10];
    s16 vramX;      /* 0x10 */
    s16 vramY;      /* 0x12 */
    u8 pad14[0x1C - 0x14];
    s32 unk1C;      /* 0x1C */
    s32 unk20;      /* 0x20 */
    u16 tpage;      /* 0x24 */
    u16 clut;       /* 0x26 */
    u16 unk28;      /* 0x28 */
    s16 width;      /* 0x2A */
    s16 height;     /* 0x2C */
    u8 dirty;       /* 0x2E */
    u8 pad2F;       /* 0x2F */
    u8 unk30;       /* 0x30 */
    u8 unk31;       /* 0x31 */
    u8 unk32;       /* 0x32 */
    u8 unk33;       /* 0x33 */
    u8 r;           /* 0x34 */
    u8 g;           /* 0x35 */
    u8 b;           /* 0x36 */
    u8 unk37;       /* 0x37 */
    u8 unk38;       /* 0x38 */
    u8 pad39[0x54 - 0x39];
    u8 unk54;       /* 0x54 */
} RenderSprite;

/*
 * The prim-buffer object initialized by InitSpriteObject and flushed by
 * SubmitPrimitiveBufferToGPU. Per-frame (double buffered) draw-env prim
 * arrays live at 0x14/0x2C (12 bytes each) and 0x44 (8 bytes each).
 */
typedef struct PrimObject {
    s16 x;              /* 0x00 */
    s16 y;              /* 0x02 */
    s16 unk4;           /* 0x04 */
    s16 unk6;           /* 0x06 */
    s16 id;             /* 0x08 */
    u8 enabled;         /* 0x0A */
    u8 padB;            /* 0x0B */
    void *vtable;       /* 0x0C */
    u8 **primList;      /* 0x10 */
    u8 envA[2][12];     /* 0x14 */
    u8 envB[2][12];     /* 0x2C */
    u8 tpagePrim[2][8]; /* 0x44 */
    u8 tpageMode;       /* 0x54 */
} PrimObject;

typedef struct GpuContext {
    u8 pad00[0x70];
    u8 *ot;             /* 0x70 */
} GpuContext;

typedef struct GameGlobals {
    u8 pad00[0xA084];
    GpuContext *gpu;    /* 0xA084 */
    u8 frameIdx;        /* 0xA088 */
} GameGlobals;

#endif /* GAME_SPRACC_RECORDS_H */
