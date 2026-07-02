#include "common.h"
#include "functions.h"

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

extern GameGlobals *g_pBlbHeapBase[]; /* unknown size forces absolute addressing */
extern u8 PRIM_OBJECT_VTABLE[] asm("D_80010344");       /* PrimObject vtable (INIT_TABLES) */
extern u8 PRIM_OBJECT_BASE_VTABLE[] asm("D_8001039C");  /* PrimObject base vtable (INIT_TABLES) */

extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern void SetDrawOffset(u8 *p, u16 *ofs);
extern void SetDrawTPage(u8 *p, s32 dfe, s32 dtd, u16 tpage);
extern u8 *AllocPrim20_Pool3(GameGlobals *g);
extern u8 *AllocPrim24(GameGlobals *g);
extern u8 *AllocPrim28(GameGlobals *g);
extern u8 *AllocPrim36(GameGlobals *g);
extern void memcpy(u8 *dst, u8 *src, s32 len);

/*
 * Initialize a PrimObject - the per-sprite slot in the GPU primitive
 * buffer that SubmitPrimitiveBufferToGPU walks each frame. Wires up
 * the base + override vtables (D_8001039C then D_80010344 - the double
 * store is needed to byte-match), zeroes scratch coords, enables the
 * slot, and stamps the caller's id.
 */
PrimObject *InitSpriteObject(PrimObject *p, s16 id) {

    p->vtable = PRIM_OBJECT_BASE_VTABLE;
    p->enabled = 1;
    p->id = id;
    p->x = 0;
    p->y = 0;
    p->unk4 = 0;
    p->unk6 = 0;
    p->vtable = PRIM_OBJECT_VTABLE;
    p->primList = NULL;
    p->tpageMode = 0;
    return p;
}

/* SubmitPrimitiveBufferToGPU @ 0x800188E0 - matches as C except an 8-instruction
 * register-coloring diff; shelved as asm via maspsx-keep INCLUDE_ASM (mid-file,
 * before the accessors below) until the match lands. */
INCLUDE_ASM("asm/nonmatchings/spracc", SubmitPrimitiveBufferToGPU);


/* Setter for the u8 at +0x54 (tail of the render context) - likely a
 * per-sprite visibility / sort-bucket flag. */
void SetRenderSpriteFlag54(RenderSprite *spr, u8 value) {
    spr->unk54 = value;
}

/* Pack-write both vramX (+0x10) and vramY (+0x12) in a single 32-bit
 * store - used to stamp this sprite's assigned VRAM rect origin. */
void SetRenderSpriteVRAMOrigin(RenderSprite *spr, s32 value) {
    *(s32 *)&spr->vramX = value;
}

/* Getter: PSX CLUT id (16-bit palette handle for 4/8-bit textures). */
u16 GetRenderSpriteClut(RenderSprite *spr) {
    return spr->clut;
}

/* Getter: paired bytes at +0x30/+0x31. These are written together with
 * tpage in SetRenderSpriteTPageAndTexBytes, so most likely the texture sub-rect U/V (or
 * extra draw-mode bits) stored beside the GPU tpage handle. */
void GetRenderSpriteTexPageBytes(RenderSprite *spr, u8 *out30, u8 *out31) {
    *out30 = spr->unk30;
    *out31 = spr->unk31;
}

/* Getter: PSX GPU tpage handle (page index + color mode + ABR blend bits). */
u16 GetRenderSpriteTPage(RenderSprite *spr) {
    return spr->tpage;
}

/* Getter: u8 at +0x37 - one of the paired bytes immediately after the
 * RGB tint triple; role TBD (probably a per-sprite blend/draw parameter). */
u8 GetRenderSpriteByte37(RenderSprite *spr) {
    return spr->unk37;
}

/* Getter: u8 at +0x38 - second half of the +0x37/+0x38 paired bytes. */
u8 GetRenderSpriteByte38(RenderSprite *spr) {
    return spr->unk38;
}

/* Setter for u8 at +0x37 (see GetRenderSpriteByte37). */
void SetRenderSpriteByte37(RenderSprite *spr, u8 value) {
    spr->unk37 = value;
}

/* Setter for u8 at +0x38 (see GetRenderSpriteByte38). */
void SetRenderSpriteByte38(RenderSprite *spr, u8 value) {
    spr->unk38 = value;
}

/* Setter: sprite draw rectangle width/height in pixels - normally
 * pushed in from the current frame's metadata (+0xA / +0xC in the
 * 0x24-byte per-frame entry). */
void SetRenderSpriteSize(RenderSprite *spr, s16 w, s16 h) {
    spr->width = w;
    spr->height = h;
}

/* Getter: u16 at +0x28 - small render-context word adjacent to tpage/clut. */
u16 GetRenderSpriteWord28(RenderSprite *spr) {
    return spr->unk28;
}

/* Setter for the u16 at +0x28 (see GetRenderSpriteWord28). */
void SetRenderSpriteWord28(RenderSprite *spr, u16 value) {
    spr->unk28 = value;
}

/* Getter: s32 at +0x20 - second of the paired 32-bit slots +0x1C/+0x20
 * (often used as a current/target or min/max pair, see SetRenderSpriteSlotPair). */
s32 GetRenderSpriteSlot20(RenderSprite *spr) {
    return spr->unk20;
}

/* Getter: s32 at +0x1C - first of the +0x1C/+0x20 paired 32-bit slots. */
s32 GetRenderSpriteSlot1C(RenderSprite *spr) {
    return spr->unk1C;
}

/* Setter for the s32 at +0x20. */
void SetRenderSpriteSlot20(RenderSprite *spr, s32 value) {
    spr->unk20 = value;
}

/* Setter for the s32 at +0x1C. */
void SetRenderSpriteSlot1C(RenderSprite *spr, s32 value) {
    spr->unk1C = value;
}

/* Seed both paired s32 slots at +0x1C and +0x20 to the same value -
 * the classic "current = target = X" reset for an interpolation /
 * accumulator pair. */
void SetRenderSpriteSlotPair(RenderSprite *spr, s32 value) {
    spr->unk1C = value;
    spr->unk20 = value;
}

/* Setter for the u8 at +0x33 - the lone byte sitting just before the
 * RGB tint triple; specific role TBD. */
void SetRenderSpriteByte33(RenderSprite *spr, u8 value) {
    spr->unk33 = value;
}

/* Getter: sprite tint RGB at +0x34..+0x36 (the per-vertex color the GPU
 * modulates the textured quad with). */
void GetRenderSpriteColor(RenderSprite *spr, u8 *outR, u8 *outG, u8 *outB) {
    *outR = spr->r;
    *outG = spr->g;
    *outB = spr->b;
}

/* Setter: sprite tint RGB at +0x34..+0x36 (see GetRenderSpriteColor). */
void SetRenderSpriteColor(RenderSprite *spr, u8 r, u8 g, u8 b) {
    spr->r = r;
    spr->g = g;
    spr->b = b;
}

/* Setter: CLUT id (see GetRenderSpriteClut). */
void SetRenderSpriteClut(RenderSprite *spr, u16 value) {
    spr->clut = value;
}

/* Re-arm the sprite's GPU draw state after a frame/texture change:
 * mark it dirty (re-upload pending), refresh the paired bytes at
 * +0x30/+0x31, and overwrite tpage while preserving the ABR
 * (semi-transparency mode) bits 0x60 from the previous tpage. */
void SetRenderSpriteTPageAndTexBytes(RenderSprite *spr, u16 tpage, u8 val30, u8 val31) {
    spr->dirty = 1;
    spr->unk30 = val30;
    spr->unk31 = val31;
    spr->tpage = (tpage & 0xFF9F) | (spr->tpage & 0x60);
}

/* Build the GPU tpage handle from this sprite's assigned VRAM rect:
 * snaps (vramX, vramY) to its 64x256 PSX texture-page origin, with
 * the color mode taken from spr->unk32 (4/8/16-bit) and the
 * caller-supplied blend mode (abr). */
void SetSpriteTPageFromVRAMCoords(RenderSprite *spr, u8 abr) {
    s32 x = spr->vramX;
    s32 y = spr->vramY;

    spr->tpage = GetTPage(spr->unk32, abr, x & ~0x3F, y & ~0xFF);
}
