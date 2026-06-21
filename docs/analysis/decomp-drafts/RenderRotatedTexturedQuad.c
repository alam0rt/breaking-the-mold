/*
 * SHELVED DRAFT — RenderRotatedTexturedQuad @ 0x800310AC (size 0x320), effects.c
 *
 * Status: LOGIC-COMPLETE, not yet byte-matching.
 *   - Compiles; produces exactly 200/200 instructions with an identical opcode
 *     set to the original. Remaining delta is ~80 normalized lines of pure
 *     instruction scheduling + stack-slot layout + register coloring:
 *       * placement of the `vN.vz = 0` store inside lhu load-delay slots,
 *       * lw(output)/lbu(offX) load order in the 8 vertex computations,
 *       * ordering of the UV / GetTPage / blb[2] tail block,
 *       * output VECTORs land at sp 0x50/0x60/0x70/0x80 (contiguous) vs the
 *         original 0x50/0x68/0x78/0x88 with `flag` at 0x60 — a stack-layout
 *         difference the source-statement order alone did not reproduce.
 *   - Tried: u8->s32 for halfW/halfH (removed spurious andi 0xFF, 278->80);
 *     flipping vertex add associativity made it worse (80->116, reverted).
 *
 * Next step: decomp-permuter (needs a hand-built dir for this project — the
 * auto-importer requires the function still in INCLUDE_ASM form). The scheduling
 * + stack layout is exactly what the permuter is good at.
 *
 * This is the project's FIRST GTE/matrix function, so it also establishes the
 * file-local PSX type convention (no PSY-Q headers in this build): RQ_SVEC /
 * RQ_VEC / RQ_MATRIX / RQ_POLY_FT4. D_800108B0 is a const identity*0x1000
 * matrix in hud.rodata.s (extern, don't redefine). Globals layout matches
 * prim.c: gpu ptr @ 0xA084, OT @ gpu+0x70, tpage-page flag byte @ 0xA088.
 */

/* PSX GTE/GPU primitive types (file-local; no PSY-Q headers in this build). */
typedef struct { short vx, vy, vz, pad; } RQ_SVEC;   /* 0x08 */
typedef struct { long vx, vy, vz; } RQ_VEC;          /* 0x0C */
typedef struct { short m[3][3]; long t[3]; } RQ_MATRIX; /* 0x20 */
typedef struct {
    /* 0x00 */ u32 tag;
    /* 0x04 */ u8 r0, g0, b0, code;
    /* 0x08 */ s16 x0, y0; /* 0x0C */ u8 u0, v0; /* 0x0E */ u16 clut;
    /* 0x10 */ s16 x1, y1; /* 0x14 */ u8 u1, v1; /* 0x16 */ u16 tpage;
    /* 0x18 */ s16 x2, y2; /* 0x1C */ u8 u2, v2; /* 0x1E */ u16 pad2;
    /* 0x20 */ s16 x3, y3; /* 0x24 */ u8 u3, v3; /* 0x26 */ u16 pad3;
} RQ_POLY_FT4; /* 0x28 */

typedef struct {
    /* 0x00 */ u16 cx;
    /* 0x02 */ u16 cy;
    /* 0x04 */ u8 _p04[0xC];
    /* 0x10 */ u8 colR;
    /* 0x11 */ u8 colG;
    /* 0x12 */ u8 colB;
    /* 0x13 */ u8 colBase;
    /* 0x14 */ u16 angle;
    /* 0x16 */ u8 offX;
    /* 0x17 */ u8 offY;
    /* 0x18 */ u8 halfW;
    /* 0x19 */ u8 halfH;
} RQ_PARAMS;

extern RQ_MATRIX D_800108B0;
extern u8 *AllocPrim40(void *g);
extern void SetPolyFT4(RQ_POLY_FT4 *p);
extern void SetShadeTex(RQ_POLY_FT4 *p, s32 tge);
extern void SetSemiTrans(RQ_POLY_FT4 *p, s32 abe);
extern void SetTransMatrix(RQ_MATRIX *m);
extern void RotMatrixZ(s32 angle, RQ_MATRIX *m);
extern void SetRotMatrix(RQ_MATRIX *m);
extern void RotTrans(RQ_SVEC *in, RQ_VEC *out, s32 *flag);
extern s32 GetTPage(s32 tp, s32 abr, s32 x, s32 y);

void RenderRotatedTexturedQuad(RQ_PARAMS *p) {
    RQ_POLY_FT4 *prim;
    RQ_MATRIX mtx;
    RQ_SVEC v0, v1, v2, v3;
    RQ_VEC o0, o1, o2, o3;
    s32 flag;
    s32 halfW, halfH;
    void *gpu;

    prim = (RQ_POLY_FT4 *)AllocPrim40(g_pBlbHeapBase);
    SetPolyFT4(prim);
    prim->r0 = p->colBase + p->colR;
    prim->g0 = p->colBase + p->colG;
    prim->b0 = p->colBase + p->colB;
    SetShadeTex(prim, 0);
    SetSemiTrans(prim, 0);

    halfW = p->halfW;
    halfH = p->halfH;

    mtx = D_800108B0;
    SetTransMatrix(&mtx);
    RotMatrixZ(p->angle, &mtx);
    SetRotMatrix(&mtx);

    v0.vx = p->cx - 0x78;
    v0.vz = 0;
    v0.vy = p->cy - 0x78;
    v1.vx = p->cx + halfW - 0x78;
    v1.vz = 0;
    v1.vy = p->cy - 0x78;
    v2.vx = p->cx - 0x78;
    v2.vz = 0;
    v2.vy = p->cy + halfH - 0x78;
    v3.vx = p->cx + halfW - 0x78;
    v3.vz = 0;
    v3.vy = p->cy + halfH - 0x78;

    RotTrans(&v0, &o0, &flag);
    RotTrans(&v1, &o1, &flag);
    RotTrans(&v2, &o2, &flag);
    RotTrans(&v3, &o3, &flag);

    prim->x0 = o0.vx + 0x78 + p->offX;
    prim->y0 = o0.vy + 0x78 + p->offY;
    prim->x1 = o1.vx + 0x78 + p->offX;
    prim->y1 = o1.vy + 0x78 + p->offY;
    prim->x2 = o2.vx + 0x78 + p->offX;
    prim->y2 = o2.vy + 0x78 + p->offY;
    prim->x3 = o3.vx + 0x78 + p->offX;
    prim->y3 = o3.vy + 0x78 + p->offY;

    prim->u0 = 0;
    prim->v0 = 0;
    prim->u1 = 0xFF;
    prim->v1 = 0;
    prim->u2 = 0;
    prim->v2 = ((u8 *)g_pBlbHeapBase)[2] - 1;
    prim->u3 = 0xFF;
    prim->clut = 0;
    prim->v3 = ((u8 *)g_pBlbHeapBase)[2] - 1;
    prim->tpage = GetTPage(2, 0, 0,
        (*((u8 *)g_pBlbHeapBase + 0xA088) == 0) ? 0x100 : 0);

    gpu = *(void **)((u8 *)g_pBlbHeapBase + 0xA084);
    AddPrim(*(u8 **)((u8 *)gpu + 0x70), (u8 *)prim);
}
