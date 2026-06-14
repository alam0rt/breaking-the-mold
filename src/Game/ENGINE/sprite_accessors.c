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

extern GameGlobals *g_pBlbHeapBase[]; /* unknown size forces absolute addressing */
extern u8 D_80010344[];           /* PrimObject vtable (INIT_TABLES) */
extern u8 D_8001039C[];           /* PrimObject base vtable (INIT_TABLES) */

extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);
extern void SetDrawOffset(void *p, u16 *ofs);
extern void SetDrawTPage(void *p, s32 dfe, s32 dtd, u16 tpage);
extern void AddPrim(void *ot, void *prim);
extern void *AllocPrim20_Pool3(GameGlobals *g);
extern void *AllocPrim24(GameGlobals *g);
extern void *AllocPrim28(GameGlobals *g);
extern void *AllocPrim36(GameGlobals *g);
extern void memcpy(void *dst, void *src, s32 len);

PrimObject *InitSpriteObject(void *arg, s16 id) {
    PrimObject *p = arg;

    p->vtable = D_8001039C;
    p->enabled = 1;
    p->id = id;
    p->x = 0;
    p->y = 0;
    p->unk4 = 0;
    p->unk6 = 0;
    p->vtable = D_80010344;
    p->primList = NULL;
    p->tpageMode = 0;
    return p;
}

/* SubmitPrimitiveBufferToGPU @ 0x800188E0 - matches as C except an 8-instruction
 * register-coloring diff; shelved as asm via maspsx-keep INCLUDE_ASM (mid-file,
 * before the accessors below) until the match lands. */
INCLUDE_ASM("asm/nonmatchings/Game/ENGINE/sprite_accessors", SubmitPrimitiveBufferToGPU);


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
