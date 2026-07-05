/* =============================================================================
 * gte.c  --  PC-port GTE (geometry) replacement (TARGET_PC)
 * =============================================================================
 * Software reimplementation of the PlayStation Geometry Transformation Engine
 * primitives the game touches. Skullmonkeys is a 2D sprite game: the C only
 * ever calls InitGeom (from the asm boot). The transform/matrix ops are
 * implemented anyway (cheap, and any future 3D-ish path won't abort), using the
 * documented PSX fixed-point conventions (rotation matrix is 1.3.12, screen
 * projection divides by the h coordinate). See docs/plans/pc-port.md CP-1.2.
 * ========================================================================== */
#include "psyq_pc.h"
#include "port_runtime.h"
#include <math.h>

/* Current geometry matrices (mirrors the GTE's R and TR registers). */
static MATRIX s_rot;
static MATRIX s_trans;
static long   s_geom_h = 0x100;   /* projection distance (h); default 256 */

void InitGeom(void) {
    int i, j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            s_rot.m[i][j] = (i == j) ? 0x1000 : 0;   /* identity, 1.3.12 */
        }
        s_rot.t[i] = 0;
        s_trans.t[i] = 0;
    }
}

void SetRotMatrix(MATRIX *m) {
    if (m) {
        s_rot = *m;
    }
}

void SetTransMatrix(MATRIX *m) {
    if (m) {
        s_trans = *m;
    }
}

/* v1 = m * v0  (3x3 rotate, 1.3.12 fixed point; result right-shifted 12). */
void ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1) {
    long x, y, z;
    if (!m || !v0 || !v1) {
        return;
    }
    x = v0->vx; y = v0->vy; z = v0->vz;
    v1->vx = (m->m[0][0] * x + m->m[0][1] * y + m->m[0][2] * z) >> 12;
    v1->vy = (m->m[1][0] * x + m->m[1][1] * y + m->m[1][2] * z) >> 12;
    v1->vz = (m->m[2][0] * x + m->m[2][1] * y + m->m[2][2] * z) >> 12;
}

/* Rotate + translate + perspective-project a single vertex.
 *   sxy  <- packed screen XY  (y<<16 | (x & 0xffff))
 *   p    <- projected depth factor (h / sz)   [optional]
 *   flag <- clip/overflow flags (always 0 here) [optional]
 * Uses s_rot/s_trans set via SetRotMatrix/SetTransMatrix. */
void RotTransPers(SVECTOR *v0, long *sxy, long *p, long *flag) {
    long x, y, z;
    long sx, sy;
    long q;
    if (!v0) {
        return;
    }
    x = (s_rot.m[0][0] * v0->vx + s_rot.m[0][1] * v0->vy + s_rot.m[0][2] * v0->vz) >> 12;
    y = (s_rot.m[1][0] * v0->vx + s_rot.m[1][1] * v0->vy + s_rot.m[1][2] * v0->vz) >> 12;
    z = (s_rot.m[2][0] * v0->vx + s_rot.m[2][1] * v0->vy + s_rot.m[2][2] * v0->vz) >> 12;
    x += s_trans.t[0];
    y += s_trans.t[1];
    z += s_trans.t[2];

    if (z == 0) {
        z = 1;
    }
    q = (s_geom_h << 12) / z;          /* perspective scale */
    sx = (x * q) >> 12;
    sy = (y * q) >> 12;

    if (sxy) {
        *sxy = ((sy & 0xffff) << 16) | (sx & 0xffff);
    }
    if (p) {
        *p = q >> 12;
    }
    if (flag) {
        *flag = 0;
    }
}

/* RotMatrixZ(r, m) -- build a Z-axis rotation matrix into m. On PSX this
 * right-multiplies the input matrix by Rz(r), but the only caller
 * (RenderSpriteOrScaledQuad) always passes a freshly-set identity, so writing
 * Rz(r) directly is equivalent. Angle r is in PSX units where 4096 (ONE) is a
 * full turn; the 3x3 is 1.3.12 fixed point (4096 = 1.0). */
void RotMatrixZ(long r, MATRIX *m) {
    double rad;
    short c, s;
    if (!m) {
        return;
    }
    rad = (double)(r & 0xFFF) * (2.0 * M_PI / 4096.0);
    c = (short)(cos(rad) * 4096.0);
    s = (short)(sin(rad) * 4096.0);
    m->m[0][0] = c;  m->m[0][1] = (short)-s; m->m[0][2] = 0;
    m->m[1][0] = s;  m->m[1][1] = c;         m->m[1][2] = 0;
    m->m[2][0] = 0;  m->m[2][1] = 0;         m->m[2][2] = 0x1000;
}

/* RotTrans(v0, v1, flag) -- rotate v0 by the current R matrix and add the
 * current translation (no perspective divide). v1 = s_rot * v0 + s_trans.
 * Matches libgte's RotTrans; flag (overflow) is always 0 on this software path. */
void RotTrans(SVECTOR *v0, VECTOR *v1, long *flag) {
    long x, y, z;
    if (!v0 || !v1) {
        return;
    }
    x = (s_rot.m[0][0] * v0->vx + s_rot.m[0][1] * v0->vy + s_rot.m[0][2] * v0->vz) >> 12;
    y = (s_rot.m[1][0] * v0->vx + s_rot.m[1][1] * v0->vy + s_rot.m[1][2] * v0->vz) >> 12;
    z = (s_rot.m[2][0] * v0->vx + s_rot.m[2][1] * v0->vy + s_rot.m[2][2] * v0->vz) >> 12;
    v1->vx = x + s_trans.t[0];
    v1->vy = y + s_trans.t[1];
    v1->vz = z + s_trans.t[2];
    if (flag) {
        *flag = 0;
    }
}
