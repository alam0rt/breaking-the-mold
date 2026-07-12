/* =============================================================================
 * RenderRippleExpandEffect.c  --  PC-port conversion (0x80036CFC)
 * =============================================================================
 * Per-frame render callback for the expanding ripple ring (pickup/impact
 * shockwave): an 8-segment ring built from 16 gouraud triangles (POLY_G3),
 * fanned between a full-radius outer vertex ring (angles 0x100 + i*0x200,
 * radius >> 12) and a half-radius inner ring (angles 0x200 + i*0x200,
 * radius >> 13), plus one DR_TPAGE selecting the semi-trans page.
 *
 * The prims live inside the entity, double-buffered on the frame index byte
 * at heap+0xA088:
 *   +0x010 POLY_G3[2][16]  (two 0x1C0 buffers: 8 segments x {p1, p2})
 *   +0x390 DR_TPAGE[2]     (8 bytes each)
 * Entity fields:
 *   +0x000/+0x002 u16 x,y   +0x00A u8 render-enable
 *   +0x3A0 s16 radius       +0x3A2 u16 radius velocity (radius += each tick)
 *   +0x3A4 u16 radius acceleration (velocity += each tick)
 *   +0x3A6 u8 angle byte (latched from g_pGameState->0x10C; low 3 bits << 4
 *          also drive the outer rim's pulsing red)  +0x3A7 u8 ticked flag
 * The vertical radius is squished by |0x78 - y| clamped to >= 8, scaled by
 * radius/480 (the ROM uses the 0x88888889 magic-divide). ccos/csin are the
 * bit-exact CORDIC pair in port/spec/gte.c -- results land in the PSX-mirror
 * arena, so integer parity matters.
 * ========================================================================== */
#include "psyq_pc.h"
#include "port_runtime.h"

extern void *g_pBlbHeapBase;
extern void *g_pGameState;
extern long  ccos(int a);
extern long  csin(int a);
extern void  SetPolyG3(POLY_G3 *p);
extern void  SetSemiTrans(void *p, int abe);
extern void  SetDrawTPage(void *p, int dfe, int dtd, int tpage);
extern long  GetTPage(int tp, int abr, int x, int y);
extern void  AddPrim(void *ot, void *p);

void RenderRippleExpandEffect(unsigned char *e)
{
    if (*(unsigned char *)(e + 0xA) != 0) {
        unsigned char colHi = (unsigned char)((e[0x3A6] & 7) << 4);
        unsigned int buf = *((unsigned char *)g_pBlbHeapBase + 0xA088);
        unsigned int angle;
        short radius;
        short dist, vrad;
        unsigned short outer[8][2];   /* ring vertices {x, y} */
        unsigned short inner[8][2];
        int i, t;
        void *ot;

        if (e[0x3A7] != 0) {
            e[0x3A6] = (unsigned char)*(int *)((char *)g_pGameState + 0x10C);
        }

        /* vertical squish: |0x78 - y| (s16 sign test, clamp >= 8), then
         * radius * dist / 480 */
        t = 0x78 - (int)*(unsigned short *)(e + 0x2);
        if ((short)t < 0) {
            t = -t;
        }
        dist = (short)t;
        if (dist < 8) {
            dist = 8;
        }
        radius = *(short *)(e + 0x3A0);
        vrad = (short)(((int)radius * dist) / 480);

        angle = 0x100;
        for (i = 0; i < 4; i++) {
            int a = (int)(angle & 0xFFFF);
            int a2 = (int)((angle + 0x100) & 0xFFFF);
            unsigned short x = *(unsigned short *)(e + 0x0);
            unsigned short y = *(unsigned short *)(e + 0x2);
            int d;

            d = ((int)ccos(a) * radius) >> 12;
            outer[i][0] = (unsigned short)(x + d);
            outer[i + 4][0] = (unsigned short)(x - d);
            d = ((int)csin(a) * vrad) >> 12;
            outer[i][1] = (unsigned short)(y + d);
            d = ((int)csin(a + 0x800) * vrad) >> 12;
            outer[i + 4][1] = (unsigned short)(y + d);

            d = ((int)ccos(a2) * radius) >> 13;
            inner[i][0] = (unsigned short)(x + d);
            inner[i + 4][0] = (unsigned short)(x - d);
            d = ((int)csin(a2) * vrad) >> 13;
            inner[i][1] = (unsigned short)(y + d);
            d = ((int)csin(a2 + 0x800) * vrad) >> 13;
            inner[i + 4][1] = (unsigned short)(y + d);

            angle += 0x200;
        }

        for (i = 0; i < 8; i++) {
            POLY_G3 *p1 = (POLY_G3 *)(e + buf * 0x1C0 + 0x10 + i * 0x1C);
            POLY_G3 *p2 = (POLY_G3 *)((unsigned char *)p1 + 0xE0);
            int prev = (i - 1) & 7;
            int next = (i + 1) & 7;

            SetPolyG3(p1);
            SetPolyG3(p2);

            /* inner-fan triangle: inner[i-1] -> outer[i] -> inner[i];
             * dark red rim fading to bright yellow-less orange at the rim */
            SetSemiTrans(p1, 1);
            p1->x0 = (short)inner[prev][0]; p1->y0 = (short)inner[prev][1];
            p1->x1 = (short)outer[i][0];    p1->y1 = (short)outer[i][1];
            p1->x2 = (short)inner[i][0];    p1->y2 = (short)inner[i][1];
            p1->r0 = 0x40; p1->g0 = 0;    p1->b0 = 0;
            p1->r1 = 0x80; p1->g1 = 0x80; p1->b1 = 0;
            p1->r2 = 0x40; p1->g2 = 0;    p1->b2 = 0;
            ot = *(void **)(*(char **)((char *)g_pBlbHeapBase + 0xA084) + 0x70);
            AddPrim(ot, p1);

            /* outer-fan triangle: outer[i] -> outer[i+1] -> inner[i];
             * black except the pulsing far corner */
            SetSemiTrans(p2, 1);
            p2->x0 = (short)outer[i][0];    p2->y0 = (short)outer[i][1];
            p2->x1 = (short)outer[next][0]; p2->y1 = (short)outer[next][1];
            p2->x2 = (short)inner[i][0];    p2->y2 = (short)inner[i][1];
            p2->r0 = 0; p2->g0 = 0; p2->b0 = 0;
            p2->r1 = 0; p2->g1 = 0; p2->b1 = 0;
            p2->r2 = (unsigned char)(colHi + 0x60);
            p2->g2 = 0x40;
            p2->b2 = 0x30;
            ot = *(void **)(*(char **)((char *)g_pBlbHeapBase + 0xA084) + 0x70);
            AddPrim(ot, p2);
        }

        {
            void *tp = e + buf * 8 + 0x390;
            SetDrawTPage(tp, 0, 0, (int)(GetTPage(0, 1, 0, 0) & 0xFFFF));
            ot = *(void **)(*(char **)((char *)g_pBlbHeapBase + 0xA084) + 0x70);
            AddPrim(ot, tp);
        }
    }

    if (e[0x3A7] != 0) {
        e[0x3A7] = 0;
        *(unsigned short *)(e + 0x3A0) =
            (unsigned short)(*(unsigned short *)(e + 0x3A0) +
                             *(unsigned short *)(e + 0x3A2));
        *(unsigned short *)(e + 0x3A2) =
            (unsigned short)(*(unsigned short *)(e + 0x3A2) +
                             *(unsigned short *)(e + 0x3A4));
    }
}
