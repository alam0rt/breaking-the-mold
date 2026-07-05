/* =============================================================================
 * psyq_pc.h  --  PC reimplementation of the PSY-Q SDK type/prototype surface
 * =============================================================================
 * The shared game headers (via common.h's __has_include) and src/*.c expect the
 * PSY-Q library headers (LIBGPU/LIBGTE/LIBSPU/LIBCD/LIBETC/LIBPAD/LIBAPI). Those
 * proprietary headers are not committed. This single header reconstructs the
 * subset the codebase touches, with the ORIGINAL 32-bit layouts (the port is
 * built -m32, so these match the game's fixed struct offsets and the ordering
 * table / primitive packing the GPU backend walks).
 *
 * The thin LIB*.H files next to this one just `#include "psyq_pc.h"` so the
 * __has_include probes in include/common.h succeed.
 *
 * Prototypes here are implemented by the SPEC/HAL layer in port/spec/*.c
 * (gpu_gl.c, gte.c, spu_sdl.c, cd_files.c, pad_sdl.c, bios.c).
 * ========================================================================== */
#ifndef PSYQ_PC_H
#define PSYQ_PC_H

/* common.h defines s8..u64, u_char..u_long before including us. Guard anyway. */
#ifndef COMMON_H
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;   /* 32-bit under -m32 */
#endif

/* -----------------------------------------------------------------------------
 * LIBGTE: geometry types (fixed 32-bit layouts)
 * -------------------------------------------------------------------------- */
typedef struct { short vx, vy; short vz, pad; } SVECTOR;
typedef struct { unsigned char r, g, b, cd;   } CVECTOR;
typedef struct { long  vx, vy, vz, pad;       } VECTOR;
typedef struct { long  vx, vy;                } DVECTOR;
typedef struct { short m[3][3]; short pad; long t[3]; } MATRIX;

/* -----------------------------------------------------------------------------
 * LIBGPU: rectangles, environments, ordering-table primitives
 * -------------------------------------------------------------------------- */
typedef struct { short x, y, w, h; } RECT;

typedef struct {
    RECT  clip;
    short ofs[2];
    RECT  tw;
    u_short tpage;
    u_char  dtd, dfe, isbg;
    u_char  r0, g0, b0;
    /* DR_ENV dr_env; (omitted; unused by shared C) */
} DRAWENV;

typedef struct {
    RECT  disp;
    RECT  screen;
    u_char isinter, isrgb24, pad0, pad1;
} DISPENV;

/* Every ordering-table primitive begins with this 4-byte tag: a 24-bit "next"
 * pointer + an 8-bit length. The GPU backend (gpu_gl.c) walks the OT via `tag`
 * and dispatches on the primitive's `code`. */
#define SETLEN(p, _len)   (((P_TAG *)(p))->len  = (u_char)(_len))
#define SETCODE(p, _code) (((P_TAG *)(p))->code = (u_char)(_code))

typedef struct { u_int addr:24, len:8; u_char r0, g0, b0, code; } P_TAG;
#define TAG_FIELDS  u_int tag; u_char r0, g0, b0, code

typedef struct { u_int tag; } DR_TPAGE_tag;

typedef struct { TAG_FIELDS; short x0, y0, x1, y1, x2, y2; } POLY_F3;
typedef struct { TAG_FIELDS; short x0, y0, x1, y1, x2, y2, x3, y3; } POLY_F4;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut;
                 short x1, y1; u_char u1, v1; u_short tpage;
                 short x2, y2; u_char u2, v2; u_short pad2; } POLY_FT3;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut;
                 short x1, y1; u_char u1, v1; u_short tpage;
                 short x2, y2; u_char u2, v2; u_short pad2;
                 short x3, y3; u_char u3, v3; u_short pad3; } POLY_FT4;
typedef struct { TAG_FIELDS; short x0, y0; u_char r1, g1, b1, p1; short x1, y1;
                 u_char r2, g2, b2, p2; short x2, y2; } POLY_G3;
typedef struct { TAG_FIELDS; short x0, y0; u_char r1, g1, b1, p1; short x1, y1;
                 u_char r2, g2, b2, p2; short x2, y2; u_char r3, g3, b3, p3;
                 short x3, y3; } POLY_G4;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut;
                 u_char r1, g1, b1, p1; short x1, y1; u_char u1, v1; u_short tpage;
                 u_char r2, g2, b2, p2; short x2, y2; u_char u2, v2; u_short pad2;
                 } POLY_GT3;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut;
                 u_char r1, g1, b1, p1; short x1, y1; u_char u1, v1; u_short tpage;
                 u_char r2, g2, b2, p2; short x2, y2; u_char u2, v2; u_short pad2;
                 u_char r3, g3, b3, p3; short x3, y3; u_char u3, v3; u_short pad3;
                 } POLY_GT4;

typedef struct { TAG_FIELDS; short x0, y0; short w, h; } TILE;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut;
                 short w, h; } SPRT;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut; } SPRT_8;
typedef struct { TAG_FIELDS; short x0, y0; u_char u0, v0; u_short clut; } SPRT_16;
typedef struct { TAG_FIELDS; short x0, y0, x1, y1; } LINE_F2;
typedef struct { TAG_FIELDS; short x0, y0, x1, y1, x2, y2, x3, y3; } LINE_F4;
typedef struct { u_int tag; } DR_TPAGE;
typedef struct { u_int tag; } DR_MODE;

/* -----------------------------------------------------------------------------
 * LIBCD: disc position / file entry
 * -------------------------------------------------------------------------- *
 * NOTE: CdlLOC / CdlFILE are intentionally NOT defined here. The only game TU
 * that uses them (src/gamecd.c) declares its own local `typedef ... CdlLOC;`,
 * and defining it here too triggers a hard "conflicting types" redefinition.
 * The SPEC CD backend (port/spec/cd_files.c) carries its own private copy.
 * -------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
 * LIBSPU: voice / common attributes (subset)
 * -------------------------------------------------------------------------- */
typedef struct { u_long voice; u_long mask; short volume; short volmode;
                 short pitch; short note; } SpuVoiceAttr;

/* =============================================================================
 * NOTE ON HAL FUNCTION PROTOTYPES
 * =============================================================================
 * This header intentionally declares NO PSY-Q function prototypes. The decomp
 * carries its own per-translation-unit `extern` declarations for every HAL call
 * (e.g. `extern void DrawSync(s32);`, `extern int FntLoad(int,int);`), and those
 * local decls are mutually inconsistent between TUs (return type / arg width
 * guesses differ). A prototype here would collide with them under modern GCC's
 * hard "conflicting types" error.
 *
 * So we expose only the TYPES above (which the shared game headers need), and
 * let each TU's own extern stand. The SPEC/HAL layer (port/spec/*.c) provides
 * the strong DEFINITIONS. Because the port is 32-bit, every HAL return is 4
 * bytes (int / long / pointer are interchangeable in cdecl), so a caller that
 * saw an implicit-int or a slightly-different local decl is still ABI-correct.
 * ========================================================================== */

#endif /* PSYQ_PC_H */
