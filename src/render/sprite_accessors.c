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

extern u16 GetTPage(s32 tp, s32 abr, s32 x, s32 y);

void func_80018BD8(RenderSprite *spr, u8 value) {
    spr->unk54 = value;
}

void func_80018BE0(RenderSprite *spr, s32 value) {
    *(s32 *)&spr->vramX = value;
}

u16 func_80018BE8(RenderSprite *spr) {
    return spr->clut;
}

void func_80018BF4(RenderSprite *spr, u8 *out30, u8 *out31) {
    *out30 = spr->unk30;
    *out31 = spr->unk31;
}

u16 func_80018C0C(RenderSprite *spr) {
    return spr->tpage;
}

u8 func_80018C18(RenderSprite *spr) {
    return spr->unk37;
}

u8 func_80018C24(RenderSprite *spr) {
    return spr->unk38;
}

void func_80018C30(RenderSprite *spr, u8 value) {
    spr->unk37 = value;
}

void func_80018C38(RenderSprite *spr, u8 value) {
    spr->unk38 = value;
}

void func_80018C40(RenderSprite *spr, s16 w, s16 h) {
    spr->width = w;
    spr->height = h;
}

u16 func_80018C4C(RenderSprite *spr) {
    return spr->unk28;
}

void func_80018C58(RenderSprite *spr, u16 value) {
    spr->unk28 = value;
}

s32 func_80018C60(RenderSprite *spr) {
    return spr->unk20;
}

s32 func_80018C6C(RenderSprite *spr) {
    return spr->unk1C;
}

void func_80018C78(RenderSprite *spr, s32 value) {
    spr->unk20 = value;
}

void func_80018C80(RenderSprite *spr, s32 value) {
    spr->unk1C = value;
}

void func_80018C88(RenderSprite *spr, s32 value) {
    spr->unk1C = value;
    spr->unk20 = value;
}

void func_80018C94(RenderSprite *spr, u8 value) {
    spr->unk33 = value;
}

void func_80018C9C(RenderSprite *spr, u8 *outR, u8 *outG, u8 *outB) {
    *outR = spr->r;
    *outG = spr->g;
    *outB = spr->b;
}

void func_80018CC0(RenderSprite *spr, u8 r, u8 g, u8 b) {
    spr->r = r;
    spr->g = g;
    spr->b = b;
}

void func_80018CD0(RenderSprite *spr, u16 value) {
    spr->clut = value;
}

void func_80018CD8(RenderSprite *spr, u16 tpage, u8 val30, u8 val31) {
    spr->dirty = 1;
    spr->unk30 = val30;
    spr->unk31 = val31;
    spr->tpage = (tpage & 0xFF9F) | (spr->tpage & 0x60);
}

void SetSpriteTPageFromVRAMCoords(RenderSprite *spr, u8 abr) {
    s32 x = spr->vramX;
    s32 y = spr->vramY;

    spr->tpage = GetTPage(spr->unk32, abr, x & ~0x3F, y & ~0xFF);
}
