#include "common.h"
#include "functions.h"
#include "Game/spracc_records.h"

/*
 * Tiny accessor functions for the sprite render object
 * (0x80018BD8 - 0x80018D4B, tail of Game/RENDER_5C3C).
 *
 * Field layout is only what these accessors touch; the full struct is
 * not yet known, so a local view is used (same approach as
 * src/assets/blb_memory.c).
 */

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
 * store is needed to byte-match), zeroes width/height, enables the
 * slot, and stamps the caller's id.
 */
PrimObject *InitSpriteObject(PrimObject *p, s16 id) {

    p->vtable = PRIM_OBJECT_BASE_VTABLE;
    p->enabled = 1;
    p->id = id;
    p->x = 0;
    p->y = 0;
    p->width = 0;
    p->height = 0;
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

/* Getter: paired bytes at +0x30/+0x31 - the SPRT u0/v0 texture-page
 * coordinates written together with tpage in SetRenderSpriteTPageAndTexBytes. */
void GetRenderSpriteTexPageBytes(RenderSprite *spr, u8 *out30, u8 *out31) {
    *out30 = spr->texU;
    *out31 = spr->texV;
}

/* Getter: PSX GPU tpage handle (page index + color mode + ABR blend bits). */
u16 GetRenderSpriteTPage(RenderSprite *spr) {
    return spr->tpage;
}

/* Getter: u8 at +0x37 - semi-transparency enable flag (drives SetSemiTrans
 * in RenderSpriteOrScaledQuad). */
u8 GetRenderSpriteSemiTrans(RenderSprite *spr) {
    return spr->semiTransFlag;
}

/* Getter: u8 at +0x38 - alternate semi-transparency request, OR'd with +0x37. */
u8 GetRenderSpriteSemiTrans2(RenderSprite *spr) {
    return spr->semiTransFlag2;
}

/* Setter for u8 at +0x37 (see GetRenderSpriteSemiTrans). */
void SetRenderSpriteSemiTrans(RenderSprite *spr, u8 value) {
    spr->semiTransFlag = value;
}

/* Setter for u8 at +0x38 (see GetRenderSpriteSemiTrans2). */
void SetRenderSpriteSemiTrans2(RenderSprite *spr, u8 value) {
    spr->semiTransFlag2 = value;
}

/* Setter: rotation pivot (+0x2A/+0x2C) - the center the sprite rotates
 * about on the POLY_FT4 slow path. Only consulted when rotationAngle
 * (+0x28) is nonzero; ignored on the axis-aligned SPRT fast path. */
void SetRenderSpriteRotationPivot(RenderSprite *spr, s16 pivotX, s16 pivotY) {
    spr->pivotX = pivotX;
    spr->pivotY = pivotY;
}

/* Getter: u16 at +0x28 - the RotMatrixZ rotation angle applied when the
 * sprite is drawn as a rotated POLY_FT4 (0 = unrotated fast path). */
u16 GetRenderSpriteRotation(RenderSprite *spr) {
    return spr->rotationAngle;
}

/* Setter for the u16 at +0x28 (see GetRenderSpriteRotation). */
void SetRenderSpriteRotation(RenderSprite *spr, u16 value) {
    spr->rotationAngle = value;
}

/* Getter: s32 at +0x20 - the 16.16 render scale Y (1.0 = 0x10000). */
s32 GetRenderSpriteScaleY(RenderSprite *spr) {
    return spr->scaleY;
}

/* Getter: s32 at +0x1C - the 16.16 render scale X (1.0 = 0x10000). */
s32 GetRenderSpriteScaleX(RenderSprite *spr) {
    return spr->scaleX;
}

/* Setter for the s32 at +0x20. */
void SetRenderSpriteScaleY(RenderSprite *spr, s32 value) {
    spr->scaleY = value;
}

/* Setter for the s32 at +0x1C. */
void SetRenderSpriteScaleX(RenderSprite *spr, s32 value) {
    spr->scaleX = value;
}

/* Seed both 16.16 scale slots at +0x1C/+0x20 to the same value - the
 * uniform-scale reset (scaleX = scaleY = X). */
void SetRenderSpriteScaleUniform(RenderSprite *spr, s32 value) {
    spr->scaleX = value;
    spr->scaleY = value;
}

/* Setter for the u8 at +0x33 - the color-modulation toggle: nonzero blends
 * the RGB tint into the textured quad (SetShadeTex), zero draws raw texture. */
void SetRenderSpriteColorModulate(RenderSprite *spr, u8 value) {
    spr->colorModulate = value;
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
 * mark it dirty (re-upload pending), refresh the SPRT u0/v0 texture
 * coords at +0x30/+0x31, and overwrite tpage while preserving the ABR
 * (semi-transparency mode) bits 0x60 from the previous tpage. */
void SetRenderSpriteTPageAndTexBytes(RenderSprite *spr, u16 tpage, u8 val30, u8 val31) {
    spr->dirty = 1;
    spr->texU = val30;
    spr->texV = val31;
    spr->tpage = (tpage & 0xFF9F) | (spr->tpage & 0x60);
}

/* Build the GPU tpage handle from this sprite's assigned VRAM rect:
 * snaps (vramX, vramY) to its 64x256 PSX texture-page origin, with
 * the color mode taken from spr->colorMode (4/8/16-bit) and the
 * caller-supplied blend mode (abr). */
void SetSpriteTPageFromVRAMCoords(RenderSprite *spr, u8 abr) {
    s32 x = spr->vramX;
    s32 y = spr->vramY;

    spr->tpage = GetTPage(spr->colorMode, abr, x & ~0x3F, y & ~0xFF);
}
