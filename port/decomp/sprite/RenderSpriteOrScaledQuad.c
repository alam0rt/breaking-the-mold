/* =============================================================================
 * RenderSpriteOrScaledQuad.c  --  PC-port sprite primitive emitter
 * =============================================================================
 * Faithful functional-C reconstruction of RenderSpriteOrScaledQuad
 * (export/SLES_010.90.c). This is the sprite PRIMITIVE EMITTER: it builds the
 * SPRT (unit-scaled, unrotated fast path) or POLY_FT4 (scaled and/or rotated
 * quad) that the GL backend rasterizes, links it into the ordering table, and
 * on the SPRT path additionally emits a DR_TPAGE.
 *
 * `param_1` is the BasicEntity render primitive (short *), so param_1[N] is the
 * short at byte offset N*2 and *(int*)(param_1+N) reads the 32-bit word there.
 * Byte offsets referenced below:
 *   +0x04 w (param_1[2])        +0x06 h (param_1[3])
 *   +0x0A active (param_1[5])   +0x1C/+0x20 scaleX/scaleY (param_1[0xe]/[0x10])
 *   +0x24 tpage (param_1[0x12]) +0x26 clut (param_1[0x13])
 *   +0x28 rotation (param_1[0x14]) +0x2A/+0x2C pivot x/y (param_1[0x15]/[0x16])
 *   +0x2E enable (param_1[0x17]) +0x30 U (param_1[0x18])  +0x31 V
 *   +0x33 shade-tex disable      +0x34/+0x35/+0x36 r/g/b (param_1[0x1a]/[0x1b])
 *   +0x37 semitrans              +0x38 extra semitrans ((char)param_1[0x1c])
 *
 * The ordering-table head AddPrim links into is reached via the GpuContext
 * pointer at (int)g_pBlbHeapBase + 0xa084, then +0x70 within it.
 *
 * Export field mapping notes: SPRT/POLY_FT4 `_2` = v0, POLY_FT4 `_3` = v1,
 * MATRIX `_18_2_` = the `pad` half-word. Ghidra `uint`/`ushort` are spelled with
 * the PSY-Q `u_int`/`u_short` typedefs from psyq_pc.h.
 * ========================================================================== */
#include "psyq_pc.h"
#include "port_runtime.h"
#include "port_hal.h"

extern void *g_pBlbHeapBase;
extern void *AllocPrim20(void *ctx);   /* SPRT (20 bytes)     */
extern void *AllocPrim8(void *ctx);    /* DR_TPAGE (8 bytes)  */
extern void *AllocPrim40(void *ctx);   /* POLY_FT4 (40 bytes) */
extern void  SetSprt(void *p);
extern void  SetPolyFT4(void *p);
extern void  SetShadeTex(void *p, int tex);
extern void  SetSemiTrans(void *p, int abe);
extern void  SetDrawTPage(void *p, int dfe, int dtd, int tpage);
extern void  AddPrim(void *ot, void *p);
extern void  SetTransMatrix(MATRIX *m);
extern void  SetRotMatrix(MATRIX *m);
extern void  RotMatrixZ(long r, MATRIX *m);
extern void  RotTrans(SVECTOR *v0, VECTOR *v1, long *flag);

void RenderSpriteOrScaledQuad(short *param_1)

{
  short sVar1;
  short sVar2;
  SPRT *p;
  POLY_FT4 *p_00;
  int iVar3;
  void *pvVar4;
  int iVar5;
  int abe;
  char cVar6;
  char cVar7;
  MATRIX local_a8;
  SVECTOR local_88;
  SVECTOR local_80;
  SVECTOR local_78;
  SVECTOR local_70;
  VECTOR local_68;
  long alStack_58 [2];
  VECTOR local_50;
  VECTOR local_40;
  VECTOR local_30;

  if ((char)param_1[0x17] == '\0') {
    return;
  }
  if ((char)param_1[5] == '\0') {
    return;
  }
  if (param_1[2] == 0) {
    return;
  }
  if (param_1[3] == 0) {
    return;
  }
  if (((*(int *)(param_1 + 0xe) == 0x10000) && (*(int *)(param_1 + 0x10) == 0x10000)) &&
     (param_1[0x14] == 0)) {
    if ((int)*(short *)g_pBlbHeapBase < (int)*param_1) {
      return;
    }
    if ((int)*(short *)((int)g_pBlbHeapBase + 2) < (int)param_1[1]) {
      return;
    }
    if ((int)*param_1 + (int)param_1[2] < 0) {
      return;
    }
    if ((int)param_1[1] + (int)param_1[3] < 0) {
      return;
    }
    p = AllocPrim20(g_pBlbHeapBase);
    SetSprt((void *)p);
    p->r0 = (u_char)param_1[0x1a];
    p->g0 = *(u_char *)((int)param_1 + 0x35);
    p->b0 = (u_char)param_1[0x1b];
    SetShadeTex((void *)p,(u_int)(*(char *)((int)param_1 + 0x33) == '\0'));
    iVar5 = 0;
    if ((*(char *)((int)param_1 + 0x37) != '\0') || ((char)param_1[0x1c] != '\0')) {
      iVar5 = 1;
    }
    SetSemiTrans((void *)p,iVar5);
    p->w = param_1[2];
    p->h = param_1[3];
    p->x0 = *param_1;
    p->y0 = param_1[1];
    p->u0 = (u_char)param_1[0x18];
    p->v0 = *(u_char *)((int)param_1 + 0x31);
    pvVar4 = g_pBlbHeapBase;
    p->clut = param_1[0x13];
    AddPrim(*(void **)(*(int *)((int)pvVar4 + 0xa084) + 0x70),p);
    p_00 = AllocPrim8(g_pBlbHeapBase);
    SetDrawTPage((DR_TPAGE *)p_00,0,0,(u_int)(u_short)param_1[0x12]);
    pvVar4 = *(void **)(*(int *)((int)g_pBlbHeapBase + 0xa084) + 0x70);
  }
  else {
    sVar1 = param_1[2];
    iVar5 = *(int *)(param_1 + 0xe);
    sVar2 = param_1[3];
    iVar3 = *(int *)(param_1 + 0x10);
    cVar6 = '\0';
    cVar7 = '\0';
    if ((int)*(short *)g_pBlbHeapBase < *param_1 + -0x200) {
      return;
    }
    if ((int)*(short *)((int)g_pBlbHeapBase + 2) < param_1[1] + -0x200) {
      return;
    }
    if ((int)*param_1 + (sVar1 * iVar5 >> 0x10) + 0x200 < 0) {
      return;
    }
    if ((int)param_1[1] + (sVar2 * iVar3 >> 0x10) + 0x200 < 0) {
      return;
    }
    p_00 = AllocPrim40(g_pBlbHeapBase);
    SetPolyFT4((void *)p_00);
    p_00->r0 = (u_char)param_1[0x1a];
    p_00->g0 = *(u_char *)((int)param_1 + 0x35);
    p_00->b0 = (u_char)param_1[0x1b];
    SetShadeTex((void *)p_00,(u_int)(*(char *)((int)param_1 + 0x33) == '\0'));
    abe = 0;
    if ((*(char *)((int)param_1 + 0x37) != '\0') || ((char)param_1[0x1c] != '\0')) {
      abe = 1;
    }
    SetSemiTrans((void *)p_00,abe);
    sVar2 = (short)((u_int)(sVar2 * iVar3) >> 0x10);
    sVar1 = (short)((u_int)(sVar1 * iVar5) >> 0x10);
    if (param_1[0x14] == 0) {
      p_00->x0 = *param_1 + 1;
      p_00->y0 = param_1[1] + 1;
      p_00->x1 = *param_1 + sVar1 + 1;
      p_00->y1 = param_1[1] + 1;
      p_00->x2 = *param_1 + 1;
      p_00->y2 = param_1[1] + sVar2 + 1;
      p_00->x3 = *param_1 + sVar1 + 1;
      p_00->y3 = param_1[1] + sVar2 + 1;
      if (0x1ffff < *(int *)(param_1 + 0xe)) {
        cVar6 = -1;
      }
      if (0x1ffff < *(int *)(param_1 + 0x10)) {
        cVar7 = -1;
      }
    }
    else {
      local_a8.m[0][0] = 0x1000;
      local_a8.m[0][1] = 0;
      local_a8.m[0][2] = 0;
      local_a8.m[1][0] = 0;
      local_a8.m[1][1] = 0x1000;
      local_a8.m[1][2] = 0;
      local_a8.m[2][0] = 0;
      local_a8.m[2][1] = 0;
      local_a8.m[2][2] = 0x1000;
      local_a8.pad = 0;
      local_a8.t[0] = 0;
      local_a8.t[1] = 0;
      local_a8.t[2] = 0;
      SetTransMatrix(&local_a8);
      RotMatrixZ((u_int)(u_short)param_1[0x14],&local_a8);
      SetRotMatrix(&local_a8);
      local_88.vx = *param_1 - param_1[0x15];
      local_88.vz = 0;
      local_88.vy = param_1[1] - param_1[0x16];
      local_80.vx = (*param_1 + sVar1) - param_1[0x15];
      local_80.vz = 0;
      local_80.vy = param_1[1] - param_1[0x16];
      local_78.vx = *param_1 - param_1[0x15];
      local_78.vz = 0;
      local_78.vy = (param_1[1] + sVar2) - param_1[0x16];
      local_70.vx = (*param_1 + sVar1) - param_1[0x15];
      local_70.vz = 0;
      local_70.vy = (param_1[1] + sVar2) - param_1[0x16];
      RotTrans(&local_88,&local_68,alStack_58);
      RotTrans(&local_80,&local_50,alStack_58);
      RotTrans(&local_78,&local_40,alStack_58);
      RotTrans(&local_70,&local_30,alStack_58);
      p_00->x0 = param_1[0x15] + (short)local_68.vx;
      p_00->y0 = param_1[0x16] + (short)local_68.vy;
      p_00->x1 = param_1[0x15] + (short)local_50.vx;
      p_00->y1 = param_1[0x16] + (short)local_50.vy;
      p_00->x2 = param_1[0x15] + (short)local_40.vx;
      p_00->y2 = param_1[0x16] + (short)local_40.vy;
      cVar6 = -1;
      p_00->x3 = param_1[0x15] + (short)local_30.vx;
      cVar7 = -1;
      p_00->y3 = param_1[0x16] + (short)local_30.vy;
    }
    p_00->u0 = (u_char)param_1[0x18];
    p_00->v0 = *(u_char *)((int)param_1 + 0x31);
    p_00->u1 = (char)param_1[0x18] + (char)param_1[2] + cVar6;
    p_00->v1 = *(u_char *)((int)param_1 + 0x31);
    p_00->u2 = (u_char)param_1[0x18];
    p_00->v2 = *(char *)((int)param_1 + 0x31) + (char)param_1[3] + cVar7;
    p_00->u3 = (char)param_1[0x18] + (char)param_1[2] + cVar6;
    p_00->v3 = *(char *)((int)param_1 + 0x31) + (char)param_1[3] + cVar7;
    pvVar4 = g_pBlbHeapBase;
    p_00->clut = param_1[0x13];
    p_00->tpage = param_1[0x12];
    pvVar4 = *(void **)(*(int *)((int)pvVar4 + 0xa084) + 0x70);
  }
  AddPrim(pvVar4,p_00);
  return;
}
