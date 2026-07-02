#ifndef GAME_SPRACC_RECORDS_H
#define GAME_SPRACC_RECORDS_H

#include "common.h"

/*
 * Tiny accessor functions for the sprite render object
 * (0x80018BD8 - 0x80018D4B, tail of Game/RENDER_5C3C).
 *
 * Field semantics verified against the GPU primitive builder
 * RenderSpriteOrScaledQuad @ 0x80015764 (see export/SLES_010.90.c). The
 * object shares the base 2D-prim head (x/y/width/height/id/enabled/vtable)
 * with PrimObject/BasicPrimObject: the draw width/height are +0x04/+0x06
 * (SPRT p->w/p->h), NOT +0x2A/+0x2C (which are the rotation pivot). The
 * builder reads +0x1C/+0x20 as the 16.16 render scale, +0x28 as the
 * rotation angle, +0x2A/+0x2C as the rotation pivot, +0x30/+0x31 as the
 * SPRT u0/v0 texture coords, +0x33 as the color-modulation toggle, and
 * +0x37/+0x38 as semi-transparency flags.
 */

typedef struct RenderSprite {
    s16 x;          /* 0x00 screen X (SPRT x0) */
    s16 y;          /* 0x02 screen Y (SPRT y0) */
    s16 width;      /* 0x04 sprite/texture width in px (SPRT p->w) */
    s16 height;     /* 0x06 sprite/texture height in px (SPRT p->h) */
    s16 id;         /* 0x08 */
    u8  enabled;    /* 0x0A drawn only when nonzero */
    u8  padB;       /* 0x0B */
    void *vtable;   /* 0x0C draw-callback table */
    s16 vramX;      /* 0x10 */
    s16 vramY;      /* 0x12 */
    u8 pad14[0x1C - 0x14];
    s32 scaleX;     /* 0x1C 16.16 render scale X (1.0 = 0x10000) */
    s32 scaleY;     /* 0x20 16.16 render scale Y (1.0 = 0x10000) */
    u16 tpage;      /* 0x24 */
    u16 clut;       /* 0x26 */
    u16 rotationAngle; /* 0x28 RotMatrixZ angle (0 = unrotated fast path) */
    s16 pivotX;     /* 0x2A rotation pivot X (POLY_FT4 path only) */
    s16 pivotY;     /* 0x2C rotation pivot Y */
    u8 dirty;       /* 0x2E */
    u8 pad2F;       /* 0x2F */
    u8 texU;        /* 0x30 SPRT u0 texture X within page */
    u8 texV;        /* 0x31 SPRT v0 texture Y within page */
    u8 colorMode;   /* 0x32 GetTPage color mode (4/8/16-bit) */
    u8 colorModulate; /* 0x33 nonzero = modulate texture with RGB tint (SetShadeTex) */
    u8 r;           /* 0x34 */
    u8 g;           /* 0x35 */
    u8 b;           /* 0x36 */
    u8 semiTransFlag;  /* 0x37 nonzero = draw semi-transparent (SetSemiTrans) */
    u8 semiTransFlag2; /* 0x38 alternate semi-transparency request (OR'd with 0x37) */
    u8 pad39[0x54 - 0x39];
    u8 unk54;       /* 0x54 likely per-sprite visibility / sort-bucket flag */
} RenderSprite;

/*
 * The prim-buffer object initialized by InitSpriteObject and flushed by
 * SubmitPrimitiveBufferToGPU. Per-frame (double buffered) draw-env prim
 * arrays live at 0x14/0x2C (12 bytes each) and 0x44 (8 bytes each).
 */
typedef struct PrimObject {
    s16 x;              /* 0x00 */
    s16 y;              /* 0x02 */
    s16 width;          /* 0x04 (SPRT p->w) */
    s16 height;         /* 0x06 (SPRT p->h) */
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
