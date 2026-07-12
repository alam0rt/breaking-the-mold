/* =============================================================================
 * Render_RotatingStarEffect.c  --  PC-port conversion (0x80032920)
 * =============================================================================
 * Per-frame render callback for the rotating star pickup burst: 4 pairs of
 * mirrored gouraud triangles (POLY_G3) orbiting the entity origin, phase
 * spread 0x200 apart, plus one DR_TPAGE packet selecting the semi-trans page.
 *
 * The prims are NOT pool-allocated: they live inside the entity itself,
 * double-buffered on the frame index byte at heap+0xA088 --
 *   +0x10  POLY_G3[2][8]   (two 0xE0 buffers: 4 iterations x {p1, p2})
 *   +0x1D0 DR_TPAGE[2]     (8 bytes each)
 * Entity fields:
 *   +0x00/+0x02 s16 x,y     +0x0A   u8 render-enable
 *   +0x1E0 u8 scale (decays to min 2 while ticked)
 *   +0x1E1/+0x1E2 u8 secondary cos/sin amplitudes
 *   +0x1E3/+0x1E4 u8 primary cos/sin amplitudes    +0x1E5 u8 y bias
 *   +0x1E6 u8 angle byte (latched from g_pGameState->0x10C frame counter)
 *   +0x1E7 u8 "ticked this frame" flag (consumed + cleared here)
 * Angle unit: (angleByte << 5) in libgte's 4096-per-rev convention; the
 * vertex math is ccos/csin (CORDIC, port/spec/gte.c) and must stay
 * integer-exact -- results are stored into the entity (PSX-mirror arena).
 * The r0 color pulses as a triangle wave of the angle byte (negated >= 0x80).
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

void Render_RotatingStarEffect(unsigned char *e)
{
    unsigned int buf;      /* frame buffer index (heap+0xA088)   */
    unsigned int angle;    /* u16 phase accumulator              */
    unsigned char col;     /* pulsing r0 color                   */
    int i;
    void *ot;

    if (*(unsigned char *)(e + 0xA) != 0) {
        buf = *((unsigned char *)g_pBlbHeapBase + 0xA088);
        if (e[0x1E7] != 0) {
            e[0x1E6] = (unsigned char)*(int *)((char *)g_pGameState + 0x10C);
        }
        angle = (unsigned int)(unsigned short)(e[0x1E6] << 5);
        col = e[0x1E6];
        if (col >= 0x80) {
            col = (unsigned char)-col;
        }

        for (i = 0; i < 4; i++) {
            POLY_G3 *p1 = (POLY_G3 *)(e + buf * 0xE0 + 0x10 + i * 0x1C);
            POLY_G3 *p2 = (POLY_G3 *)((unsigned char *)p1 + 0x70);
            int a1, a2, a3, t;
            short x, y;
            short x0a, x0b, y0a, y0b;
            short x1a, x1b, y1a, y1b;
            short x2a, x2b, y2a, y2b;

            SetPolyG3(p1);
            SetPolyG3(p2);

            a1 = (int)((angle + 0x100) & 0xFFFF);
            t = ((int)(ccos(a1) * e[0x1E3]) * e[0x1E0]) >> 13;
            x = *(short *)(e + 0x0);
            x0a = (short)(x + t);
            x0b = (short)(x - t);
            t = ((int)(csin(a1) * e[0x1E4]) * e[0x1E0]) >> 13;
            y0a = (short)(*(short *)(e + 0x2) + t + e[0x1E5]);
            t = ((int)(csin(a1 + 0x800) * e[0x1E4]) * e[0x1E0]) >> 13;
            y0b = (short)(*(short *)(e + 0x2) + t + e[0x1E5]);

            a2 = (int)((angle + 0x80) & 0xFFFF);
            t = ((int)(ccos(a2) * e[0x1E1]) * e[0x1E0]) >> 13;
            x = *(short *)(e + 0x0);
            x1a = (short)(x + t);
            x1b = (short)(x - t);
            t = ((int)(csin(a2) * e[0x1E2]) * e[0x1E0]) >> 13;
            y = *(short *)(e + 0x2);
            y1a = (short)(y + t);
            y1b = (short)(y - t);

            a3 = (int)((angle + 0x180) & 0xFFFF);
            t = ((int)(ccos(a3) * e[0x1E1]) * e[0x1E0]) >> 13;
            x = *(short *)(e + 0x0);
            x2a = (short)(x + t);
            x2b = (short)(x - t);
            t = ((int)(csin(a3) * e[0x1E2]) * e[0x1E0]) >> 13;
            y = *(short *)(e + 0x2);
            y2a = (short)(y + t);
            y2b = (short)(y - t);

            SetSemiTrans(p1, 1);
            p1->r1 = 0; p1->g1 = 0; p1->b1 = 0;
            p1->r2 = 0; p1->g2 = 0; p1->b2 = 0;
            p1->r0 = col; p1->g0 = 0x60; p1->b0 = 0x10;
            p1->x0 = x0a; p1->y0 = y0a;
            p1->x1 = x1a; p1->y1 = y1a;
            p1->x2 = x2a; p1->y2 = y2a;
            ot = *(void **)(*(char **)((char *)g_pBlbHeapBase + 0xA084) + 0x70);
            AddPrim(ot, p1);

            SetSemiTrans(p2, 1);
            p2->r1 = 0; p2->g1 = 0; p2->b1 = 0;
            p2->r2 = 0; p2->g2 = 0; p2->b2 = 0;
            p2->r0 = col; p2->g0 = 0x60; p2->b0 = 0x10;
            p2->x0 = x0b; p2->y0 = y0b;
            p2->x1 = x1b; p2->y1 = y1b;
            p2->x2 = x2b; p2->y2 = y2b;
            angle = (angle + 0x200) & 0xFFFF;
            ot = *(void **)(*(char **)((char *)g_pBlbHeapBase + 0xA084) + 0x70);
            AddPrim(ot, p2);
        }

        {
            void *tp = e + buf * 8 + 0x1D0;
            SetDrawTPage(tp, 0, 0, (int)(GetTPage(0, 1, 0, 0) & 0xFFFF));
            ot = *(void **)(*(char **)((char *)g_pBlbHeapBase + 0xA084) + 0x70);
            AddPrim(ot, tp);
        }

        if (e[0x1E7] != 0 && e[0x1E0] >= 3) {
            e[0x1E0] = (unsigned char)(e[0x1E0] - 1);
        }
    }
    e[0x1E7] = 0;
}
