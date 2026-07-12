/* =============================================================================
 * gpu_gl.c  --  PC-port GPU replacement: PSX GPU -> OpenGL (TARGET_PC)
 * =============================================================================
 * Replaces the PlayStation GPU with an OpenGL 2.x (immediate-mode) backend:
 *   - VRAM is one 1024x512 16-bit framebuffer, held as a GL texture.
 *   - LoadImage/StoreImage/MoveImage are sub-rect VRAM transfers.
 *   - GetTPage/GetClut/LoadTPage/LoadClut compute PSX texture-page / CLUT
 *     addressing (exact bit-packing, so the game's tpage words decode right).
 *   - AddPrim/ClearOTag/DrawOTag manage the ordering table; DrawOTag walks it
 *     and translates each primitive (SPRT / POLY_FT4 / POLY_GT4 / TILE /
 *     POLY_F4 / DR_TPAGE / ...) into a GL quad. Primitive-decode dispatch is
 *     modelled on TOMB5's EMULATOR/LIBGPU.C ParsePrimitive.
 *
 * THE OT LINK FORMAT (PC-native): the PSX P_TAG packs the "next" pointer into
 * 24 bits, which cannot address host memory. But this port is built -m32, so a
 * host pointer is exactly 4 bytes -- we therefore store the FULL 32-bit
 * primitive pointer in the primitive's 4-byte `tag` word (the `code` byte lives
 * in a separate word and is untouched). AddPrim / ClearOTag / DrawOTag are all
 * ours and agree on this format; the game treats `tag` as opaque.
 *
 * Attribution: primitive opcode semantics adapted from TOMB5
 *   https://github.com/TOMB5/TOMB5  EMULATOR/LIBGPU.C
 *   MIT License, Copyright (c) 2017 Gh0stBlade & zdimension.
 *
 * Phase 1 brings up the plumbing + addressing + OT walk + decode dispatch, with
 * solid-colour primitives exact and textured primitives sampling the VRAM
 * texture directly (CLUT/4-bit expansion is refined in Phase 2 against emulator
 * traces). See docs/plans/pc-port.md CP-1.1.
 * ========================================================================== */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "psyq_pc.h"
#include "port_runtime.h"
#include "port_hal.h"

#if defined(PORT_HAVE_SDL2)
#  include <SDL2/SDL.h>
#  include <SDL2/SDL_opengl.h>
#  define GPU_USE_GL 1
#else
#  define GPU_USE_GL 0
#endif

/* PSX VRAM geometry + display. */
#define VRAM_W 1024
#define VRAM_H 512
#define DISP_W 320
#define DISP_H 240

/* PSX GPU primitive command codes (the `code` byte of each primitive). */
enum {
    GP_POLY_F3 = 0x20, GP_POLY_FT3 = 0x24, GP_POLY_F4 = 0x28, GP_POLY_FT4 = 0x2C,
    GP_POLY_G3 = 0x30, GP_POLY_GT3 = 0x34, GP_POLY_G4 = 0x38, GP_POLY_GT4 = 0x3C,
    GP_LINE_F2 = 0x40, GP_LINE_G2 = 0x50,
    GP_TILE = 0x60, GP_SPRT = 0x64, GP_TILE_1 = 0x68, GP_TILE_8 = 0x70,
    GP_SPRT_8 = 0x74, GP_TILE_16 = 0x78, GP_SPRT_16 = 0x7C,
    GP_DR_TPAGE = 0xE1, GP_DR_OFFSET = 0xE5
};

/* OT terminator: a tag value that ends a primitive chain. */
#define OT_TERM 0u

#if GPU_USE_GL
static SDL_Window *s_window;
static GLuint      s_vram_tex;
/* Host-side shadow of VRAM as 16-bit PSX pixels (mask + BGR555). Uploaded to
 * s_vram_tex when dirty. */
static unsigned short s_vram[VRAM_W * VRAM_H];
static int s_vram_dirty;
static unsigned s_cur_tpage;   /* current DR_TPAGE selection */
static short    s_draw_off_x;  /* current DR_OFFSET draw origin (X) */
static short    s_draw_off_y;  /* current DR_OFFSET draw origin (Y) */

/* ---- GL 2.x shader plumbing (loaded via SDL_GL_GetProcAddress) -------------
 * PSX sprites are 4-/8-bit CLUT-indexed textures; sampling the raw 16-bit VRAM
 * as colour yields garbage. A fragment shader reads the packed index from the
 * VRAM integer texture and looks up the CLUT, reproducing the GPU's texel path.
 */
#ifndef GL_FRAGMENT_SHADER
#  define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#  define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#  define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#  define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_R16UI
#  define GL_R16UI 0x8234
#endif
#ifndef GL_RED_INTEGER
#  define GL_RED_INTEGER 0x8D94
#endif

typedef unsigned int  GL_uint;
typedef int           GL_int;
typedef char          GL_char;
typedef GL_uint (*PFNglCreateShader)(GLenum);
typedef void   (*PFNglShaderSource)(GL_uint, GL_int, const GL_char *const *, const GL_int *);
typedef void   (*PFNglCompileShader)(GL_uint);
typedef void   (*PFNglGetShaderiv)(GL_uint, GLenum, GL_int *);
typedef void   (*PFNglGetShaderInfoLog)(GL_uint, GL_int, GL_int *, GL_char *);
typedef void   (*PFNglDeleteShader)(GL_uint);
typedef GL_uint (*PFNglCreateProgram)(void);
typedef void   (*PFNglAttachShader)(GL_uint, GL_uint);
typedef void   (*PFNglLinkProgram)(GL_uint);
typedef void   (*PFNglGetProgramiv)(GL_uint, GLenum, GL_int *);
typedef void   (*PFNglGetProgramInfoLog)(GL_uint, GL_int, GL_int *, GL_char *);
typedef void   (*PFNglUseProgram)(GL_uint);
typedef GL_int (*PFNglGetUniformLocation)(GL_uint, const GL_char *);
typedef void   (*PFNglUniform1i)(GL_int, GL_int);
typedef void   (*PFNglUniform2i)(GL_int, GL_int, GL_int);
typedef void   (*PFNglBlendEquation)(GLenum);
typedef void   (*PFNglBlendColor)(GLfloat, GLfloat, GLfloat, GLfloat);

static PFNglCreateShader        pglCreateShader;
static PFNglShaderSource        pglShaderSource;
static PFNglCompileShader       pglCompileShader;
static PFNglGetShaderiv         pglGetShaderiv;
static PFNglGetShaderInfoLog    pglGetShaderInfoLog;
static PFNglDeleteShader        pglDeleteShader;
static PFNglCreateProgram       pglCreateProgram;
static PFNglAttachShader        pglAttachShader;
static PFNglLinkProgram         pglLinkProgram;
static PFNglGetProgramiv        pglGetProgramiv;
static PFNglGetProgramInfoLog   pglGetProgramInfoLog;
static PFNglUseProgram          pglUseProgram;
static PFNglGetUniformLocation  pglGetUniformLocation;
static PFNglUniform1i           pglUniform1i;
static PFNglUniform2i           pglUniform2i;
static PFNglBlendEquation       pglBlendEquation;
static PFNglBlendColor          pglBlendColor;

/* Blend-equation / constant-colour constants (GL 1.4 / ARB_imaging). */
#define GL_FUNC_ADD_COMPAT                 0x8006
#define GL_FUNC_REVERSE_SUBTRACT_COMPAT    0x800B
#define GL_CONSTANT_ALPHA_COMPAT           0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_COMPAT 0x8004

static void glBlendEquation_compat(GLenum eq) {
    if (pglBlendEquation) {
        pglBlendEquation(eq);
    }
}

static GLuint s_clut_prog;          /* CLUT-expansion shader program */
static int    s_have_shader;        /* 1 once the program links */
static GL_int s_u_bitdepth, s_u_pagebase, s_u_clutbase, s_u_vram, s_u_debugtransp, s_u_stpmode;
#endif

void port_gpu_set_window(void *sdl_window) {
#if GPU_USE_GL
    s_window = (SDL_Window *)sdl_window;
#else
    (void)sdl_window;
#endif
}

/* =============================================================================
 * Texture-page / CLUT addressing (pure PSX bit-packing -- exact, unit-testable)
 * ========================================================================== */
long GetTPage(int tp, int abr, int x, int y) {
    return ((tp & 3) << 7) | ((abr & 3) << 5)
         | ((y & 0x100) >> 4) | ((x & 0x3FF) >> 6) | ((y & 0x200) << 2);
}

u_short GetClut(int x, int y) {
    return (u_short)(((y & 0x1FF) << 6) | ((x >> 4) & 0x3F));
}

/* =============================================================================
 * VRAM transfers
 * ========================================================================== */
#if GPU_USE_GL
static void vram_upload(void) {
    if (!s_vram_dirty) {
        return;
    }
    glBindTexture(GL_TEXTURE_2D, s_vram_tex);
    /* Upload as a single-channel 16-bit integer texture so the shader can read
     * back the exact packed word (indices / BGR555) with texelFetch. */
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VRAM_W, VRAM_H,
                    GL_RED_INTEGER, GL_UNSIGNED_SHORT, s_vram);
    s_vram_dirty = 0;
}
#endif

int LoadImage(RECT *rect, u_int *data) {
#if GPU_USE_GL
    int x, y;
    const unsigned short *src = (const unsigned short *)data;
    if (!rect || !data) {
        return 0;
    }
    if (getenv("PORT_GPU_DEBUG") && rect->h == 1 && rect->w <= 256) {
        port_log("LoadImage(clut?) @(%d,%d) %dx%d first8: %04x %04x %04x %04x %04x %04x %04x %04x",
                 rect->x, rect->y, rect->w, rect->h,
                 src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]);
    }
    for (y = 0; y < rect->h; y++) {
        int dy = rect->y + y;
        if (dy < 0 || dy >= VRAM_H) continue;
        for (x = 0; x < rect->w; x++) {
            int dx = rect->x + x;
            if (dx < 0 || dx >= VRAM_W) continue;
            s_vram[dy * VRAM_W + dx] = src[y * rect->w + x];
        }
    }
    s_vram_dirty = 1;
#else
    (void)rect; (void)data;
#endif
    return 0;
}

int StoreImage(RECT *rect, u_int *data) {
#if GPU_USE_GL
    int x, y;
    unsigned short *dst = (unsigned short *)data;
    if (!rect || !data) {
        return 0;
    }
    for (y = 0; y < rect->h; y++) {
        int sy = rect->y + y;
        for (x = 0; x < rect->w; x++) {
            int sx = rect->x + x;
            dst[y * rect->w + x] =
                (sy >= 0 && sy < VRAM_H && sx >= 0 && sx < VRAM_W)
                    ? s_vram[sy * VRAM_W + sx] : 0;
        }
    }
#else
    (void)rect; (void)data;
#endif
    return 0;
}

int MoveImage(RECT *rect, int x, int y) {
#if GPU_USE_GL
    int row, col;
    if (!rect) {
        return 0;
    }
    for (row = 0; row < rect->h; row++) {
        for (col = 0; col < rect->w; col++) {
            int sx = rect->x + col, sy = rect->y + row;
            int dx = x + col, dy = y + row;
            if (sx >= 0 && sx < VRAM_W && sy >= 0 && sy < VRAM_H &&
                dx >= 0 && dx < VRAM_W && dy >= 0 && dy < VRAM_H) {
                s_vram[dy * VRAM_W + dx] = s_vram[sy * VRAM_W + sx];
            }
        }
    }
    s_vram_dirty = 1;
#else
    (void)rect; (void)x; (void)y;
#endif
    return 0;
}

int LoadTPage(RECT *rect, int tp, int abr, int x, int y, int w, int h) {
    (void)rect; (void)w; (void)h;
    return (int)(GetTPage(tp, abr, x, y) & 0xFFFF);
}

int LoadClut(u_int *clut, int x, int y) {
    RECT r;
    r.x = (short)x; r.y = (short)y; r.w = 256; r.h = 1;
    if (getenv("PORT_GPU_DEBUG")) {
        unsigned short *p = (unsigned short *)clut;
        port_log("LoadClut @(%d,%d) w=256  first8: %04x %04x %04x %04x %04x %04x %04x %04x",
                 x, y, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
    }
    LoadImage(&r, clut);
    return GetClut(x, y);
}

/* =============================================================================
 * Ordering table (PC-native 32-bit tag scheme)
 * ========================================================================== */
void ClearOTag(u_int *ot, int n) {
    int i;
    if (!ot) {
        return;
    }
    for (i = 0; i < n; i++) {
        ot[i] = OT_TERM;   /* every slot starts as an empty list */
    }
}

void AddPrim(u_char *ot, u_char *prim) {
    /* Prepend prim to the list headed by *ot (PSX AddPrim semantics), using the
     * full 32-bit tag word as the next-pointer. */
    if (!ot || !prim) {
        return;
    }
    *(u_int *)prim = *(u_int *)ot;              /* prim->next = *ot */
    *(u_int *)ot = (u_int)(uintptr_t)prim;      /* *ot = prim       */
}

#if GPU_USE_GL
/* Resolve a tpage word to its VRAM pixel base (X in 64px units, Y in 256px)
 * and colour-mode bit depth (0=4bit, 1=8bit, 2=16bit). */
static void tpage_decode(unsigned tpage, int *bx, int *by, int *bitdepth) {
    *bx = (int)((tpage & 0x0F) << 6);
    *by = (int)(((tpage >> 4) & 1) << 8);
    *bitdepth = (int)((tpage >> 7) & 3);
}

/* Resolve a clut word to its VRAM pixel base (X in 16px units, Y in 1px). */
static void clut_base(unsigned clut, int *cx, int *cy) {
    *cx = (int)((clut & 0x3F) << 4);
    *cy = (int)((clut >> 6) & 0x1FF);
}

static void blend_for_abe(int abe, unsigned tpage);

/* Emit one textured quad through the CLUT-expansion shader. Texcoords are
 * page-local texel coordinates (0..w within the texture page); the shader adds
 * the page base and does the index/CLUT lookup. rawTex!=0 draws the texture
 * un-modulated (PSX "texture, no shading"); otherwise it modulates by the
 * primitive colour (0x80 = neutral). */
static void quad_tex_ex(short px[4], short py[4], int pu[4], int pv[4],
                        unsigned tpage, unsigned clut,
                        u_char r, u_char g, u_char b, int rawTex, int abe) {
    int bx, by, bd, cx, cy, i;
    tpage_decode(tpage, &bx, &by, &bd);
    clut_base(clut, &cx, &cy);

    if (getenv("PORT_GPU_DEBUG")) {
        static int n = 0;
        if (n++ < 16) {
            port_log("tex#%d bd=%d page=(%d,%d) clut=(%d,%d) tint=(%d,%d,%d) raw=%d pos=(%d,%d..%d,%d) uv=(%d,%d)",
                     n, bd, bx, by, cx, cy, r, g, b, rawTex,
                     px[0], py[0], px[2], py[2], pu[0], pv[0]);
        }
    }

    if (s_have_shader) {
        pglUseProgram(s_clut_prog);
        pglUniform1i(s_u_vram, 0);
        pglUniform1i(s_u_bitdepth, bd);
        pglUniform2i(s_u_pagebase, bx, by);
        pglUniform2i(s_u_clutbase, cx, cy);
        pglUniform1i(s_u_debugtransp, getenv("PORT_SHOW_TRANSP") ? 1 : 0);
    }
    glBindTexture(GL_TEXTURE_2D, s_vram_tex);
    if (rawTex) {
        glColor4f(1, 1, 1, 1);
    } else {
        glColor4f(r / 128.0f, g / 128.0f, b / 128.0f, 1.0f);
    }
    /* PSX STP rule: with ABE set only texels whose STP bit (VRAM bit 15) is set
     * are blended; STP=0 texels draw opaque. Two passes: opaque texels first,
     * then the STP texels with the tpage's blend rate. Without ABE everything
     * non-zero is opaque. */
    { int pass, npass = (abe && s_have_shader) ? 2 : 1;
      for (pass = 0; pass < npass; pass++) {
        if (s_have_shader) {
            pglUniform1i(s_u_stpmode, npass == 1 ? 0 : (pass == 0 ? 1 : 2));
        }
        if (npass == 2 && pass == 0) {
            glBlendFunc(GL_ONE, GL_ZERO);            /* opaque texels */
        } else if (npass == 2) {
            blend_for_abe(1, tpage);                 /* STP texels */
        } else {
            blend_for_abe(abe && !s_have_shader, tpage);
        }
        glBegin(GL_TRIANGLE_STRIP);
        /* strip order: 0,1,2,3 = TL,TR,BL,BR */
        { const int order[4] = {0, 1, 3, 2};
          for (i = 0; i < 4; i++) {
            int k = order[i];
            glTexCoord2i(pu[k], pv[k]);
            glVertex2i(px[k] + s_draw_off_x, py[k] + s_draw_off_y);
          } }
        glEnd();
      } }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
    if (s_have_shader) {
        pglUseProgram(0);
    }
}

/* Axis-aligned textured rect helper (SPRT / SPRT_16). */
static void quad_tex(short x0, short y0, short x1, short y1,
                     int u0, int v0, int u1, int v1,
                     unsigned tpage, unsigned clut,
                     u_char r, u_char g, u_char b, int rawTex, int abe) {
    short px[4] = { x0, x1, x1, x0 };   /* TL, TR, BR, BL */
    short py[4] = { y0, y0, y1, y1 };
    int   pu[4] = { u0, u1, u1, u0 };
    int   pv[4] = { v0, v0, v1, v1 };
    quad_tex_ex(px, py, pu, pv, tpage, clut, r, g, b, rawTex, abe);
}

static void quad_solid(short x0, short y0, short x1, short y1,
                       u_char r, u_char g, u_char b) {
    x0 = (short)(x0 + s_draw_off_x); x1 = (short)(x1 + s_draw_off_x);
    y0 = (short)(y0 + s_draw_off_y); y1 = (short)(y1 + s_draw_off_y);
    if (s_have_shader) {
        pglUseProgram(0);
    }
    glDisable(GL_TEXTURE_2D);
    glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2i(x0, y0); glVertex2i(x1, y0);
    glVertex2i(x0, y1); glVertex2i(x1, y1);
    glEnd();
}

/* PORT_OT_LOG=N: log every primitive of OT-pass N (1-based) to stderr. */
static int s_ot_log_active;

static void ot_log_prim(u_char *p, u_char code) {
    switch (code & 0xFC) {
    case GP_SPRT: case GP_SPRT_8: case GP_SPRT_16: {
        SPRT *s = (SPRT *)p;
        port_log("ot: code=%02x SPRT xy=(%d,%d) wh=(%d,%d) uv=(%d,%d) clut=%04x tpage=%04x rgb=(%d,%d,%d)",
                 code, s->x0, s->y0,
                 (code & 0xFC) == GP_SPRT ? s->w : ((code & 0xFC) == GP_SPRT_8 ? 8 : 16),
                 (code & 0xFC) == GP_SPRT ? s->h : ((code & 0xFC) == GP_SPRT_8 ? 8 : 16),
                 s->u0, s->v0, s->clut, s_cur_tpage, s->r0, s->g0, s->b0);
        break;
    }
    case GP_POLY_FT4: {
        POLY_FT4 *q = (POLY_FT4 *)p;
        port_log("ot: code=%02x FT4 xy0=(%d,%d) xy3=(%d,%d) uv0=(%d,%d) uv3=(%d,%d) clut=%04x tpage=%04x rgb=(%d,%d,%d)",
                 code, q->x0, q->y0, q->x3, q->y3, q->u0, q->v0, q->u3, q->v3,
                 q->clut, q->tpage, q->r0, q->g0, q->b0);
        break;
    }
    case GP_POLY_GT4: {
        POLY_GT4 *q = (POLY_GT4 *)p;
        port_log("ot: code=%02x GT4 xy0=(%d,%d) xy3=(%d,%d) uv0=(%d,%d) clut=%04x tpage=%04x rgb=(%d,%d,%d)",
                 code, q->x0, q->y0, q->x3, q->y3, q->u0, q->v0,
                 q->clut, q->tpage, q->r0, q->g0, q->b0);
        break;
    }
    case GP_TILE: {
        TILE *t = (TILE *)p;
        port_log("ot: code=%02x TILE xy=(%d,%d) wh=(%d,%d) rgb=(%d,%d,%d)",
                 code, t->x0, t->y0, t->w, t->h, t->r0, t->g0, t->b0);
        break;
    }
    case GP_POLY_F4: {
        POLY_F4 *q = (POLY_F4 *)p;
        port_log("ot: code=%02x F4 xy0=(%d,%d) xy3=(%d,%d) rgb=(%d,%d,%d)",
                 code, q->x0, q->y0, q->x3, q->y3, q->r0, q->g0, q->b0);
        break;
    }
    default:
        if (code == GP_DR_TPAGE) {
            port_log("ot: code=%02x DR_TPAGE tpage=%04x", code, ((u_short *)p)[2]);
        } else if (code == GP_DR_OFFSET) {
            port_log("ot: code=%02x DR_OFFSET ofs=(%d,%d)", code, ((short *)p)[2], ((short *)p)[4]);
        } else {
            port_log("ot: code=%02x (unhandled) @%p bytes=%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x",
                     code, (void *)p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                     p[8], p[9], p[10], p[11]);
        }
        break;
    }
}

/* PSX semi-transparency (ABE): pick the GL blend for the tpage's ABR rate.
 * 0: B/2+F/2  1: B+F  2: B-F  3: B+F/4. Rate 0 is approximated with constant
 * 50% alpha; rate 3 by pre-scaling the source in the tint. */
static void blend_for_abe(int abe, unsigned tpage) {
    if (!abe) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
        return;
    }
    switch ((tpage >> 5) & 3) {
    case 0:
        glBlendFunc(GL_CONSTANT_ALPHA_COMPAT, GL_ONE_MINUS_CONSTANT_ALPHA_COMPAT);
        glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
        break;
    case 1:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
        break;
    case 2:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation_compat(GL_FUNC_REVERSE_SUBTRACT_COMPAT);
        break;
    default:
        glBlendFunc(GL_ONE, GL_ONE);   /* + F/4 approximated as additive */
        glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
        break;
    }
}

/* Decode + render one primitive; returns the next-pointer (its tag word).
 * The low two bits of poly/rect code bytes are the TGE (raw-texture) and ABE
 * (semi-transparent) flags -- dispatch on the code with them masked off, so
 * e.g. 0x66 (SPRT|ABE) and 0x7e (SPRT_16|ABE) decode as their base type. */
static u_int render_prim(u_char *p) {
    u_int next = *(u_int *)p;
    u_char rawcode = p[7];            /* code byte (TAG_FIELDS offset 7) */
    u_char code = rawcode;
    int abe = 0;
    if (rawcode >= 0x20 && rawcode < 0x80) {
        code = (u_char)(rawcode & ~0x03);
        abe = (rawcode & 0x02) != 0;
    }
    if (s_ot_log_active) {
        ot_log_prim(p, rawcode);
    }
    switch (code) {
    case GP_SPRT: {
        SPRT *s = (SPRT *)p;
        int raw = (p[7] & 0x01) != 0;
        quad_tex(s->x0, s->y0, (short)(s->x0 + s->w), (short)(s->y0 + s->h),
                 s->u0, s->v0, s->u0 + s->w, s->v0 + s->h,
                 s_cur_tpage, s->clut, s->r0, s->g0, s->b0, raw, abe);
        break;
    }
    case GP_SPRT_8: {
        SPRT_8 *s = (SPRT_8 *)p;
        int raw = (p[7] & 0x01) != 0;
        quad_tex(s->x0, s->y0, (short)(s->x0 + 8), (short)(s->y0 + 8),
                 s->u0, s->v0, s->u0 + 8, s->v0 + 8,
                 s_cur_tpage, s->clut, s->r0, s->g0, s->b0, raw, abe);
        break;
    }
    case GP_SPRT_16: {
        SPRT_16 *s = (SPRT_16 *)p;
        int raw = (p[7] & 0x01) != 0;
        /* TEMP DEBUG: PORT_TILE_DEBUG=1 draws every SPRT_16 as solid magenta
         * to localize invisible tile layers (texture vs placement issues). */
        static int s_tile_dbg = -1;
        if (s_tile_dbg < 0) s_tile_dbg = getenv("PORT_TILE_DEBUG") != 0;
        if (s_tile_dbg) {
            quad_solid(s->x0, s->y0, (short)(s->x0 + 16), (short)(s->y0 + 16),
                       255, 0, 255);
            break;
        }
        quad_tex(s->x0, s->y0, (short)(s->x0 + 16), (short)(s->y0 + 16),
                 s->u0, s->v0, s->u0 + 16, s->v0 + 16,
                 s_cur_tpage, s->clut, s->r0, s->g0, s->b0, raw, abe);
        break;
    }
    case GP_TILE: {
        TILE *t = (TILE *)p;
        blend_for_abe(abe, s_cur_tpage);
        quad_solid(t->x0, t->y0, (short)(t->x0 + t->w), (short)(t->y0 + t->h),
                   t->r0, t->g0, t->b0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
        break;
    }
    case GP_POLY_F4: {
        POLY_F4 *q = (POLY_F4 *)p;
        blend_for_abe(abe, s_cur_tpage);
        quad_solid(q->x0, q->y0, q->x3, q->y3, q->r0, q->g0, q->b0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation_compat(GL_FUNC_ADD_COMPAT);
        break;
    }
    case GP_POLY_FT4: {
        POLY_FT4 *q = (POLY_FT4 *)p;
        int raw = (p[7] & 0x01) != 0;
        short px[4] = { q->x0, q->x1, q->x3, q->x2 };   /* TL,TR,BR,BL */
        short py[4] = { q->y0, q->y1, q->y3, q->y2 };
        int   pu[4] = { q->u0, q->u1, q->u3, q->u2 };
        int   pv[4] = { q->v0, q->v1, q->v3, q->v2 };
        quad_tex_ex(px, py, pu, pv, q->tpage, q->clut, q->r0, q->g0, q->b0, raw, abe);
        break;
    }
    case GP_POLY_GT4: {
        POLY_GT4 *q = (POLY_GT4 *)p;
        int raw = (p[7] & 0x01) != 0;
        short px[4] = { q->x0, q->x1, q->x3, q->x2 };
        short py[4] = { q->y0, q->y1, q->y3, q->y2 };
        int   pu[4] = { q->u0, q->u1, q->u3, q->u2 };
        int   pv[4] = { q->v0, q->v1, q->v3, q->v2 };
        quad_tex_ex(px, py, pu, pv, q->tpage, q->clut, q->r0, q->g0, q->b0, raw, abe);
        break;
    }
    case GP_DR_TPAGE: {
        s_cur_tpage = ((u_short *)p)[2];   /* tpage in the low half-word */
        break;
    }
    case GP_DR_OFFSET: {
        /* SetDrawOffset packet: signed draw-origin translation applied to every
         * subsequent primitive until the next DR_OFFSET. Layout: x s16 @+4,
         * code @+7, y s16 @+8 (the packet slots are 12 bytes everywhere -- the
         * PSX game reserves buf*12 strides -- so +8 is safe; y CANNOT live at
         * +6 because its high byte would overwrite the code byte at +7, which
         * silently killed every draw offset before this layout). Used by the
         * tilemap layers to shift a pre-built SPRT chain for scrolling. */
        s_draw_off_x = ((short *)p)[2];
        s_draw_off_y = ((short *)p)[4];
        break;
    }
    default:
        /* Unhandled primitive type: skip (Phase 2 adds remaining decoders). */
        break;
    }
    return next;
}
#endif /* GPU_USE_GL */

void DrawOTag(u_int *ot) {
#if GPU_USE_GL
    u_int p;
    int guard = 0;
    if (!ot) {
        return;
    }
    {
        /* PORT_OT_LOG=N: dump the Nth (1-based) OT pass primitive-by-primitive. */
        static long s_ot_pass = 0;
        const char *ol = getenv("PORT_OT_LOG");
        s_ot_pass++;
        s_ot_log_active = (ol && atol(ol) == s_ot_pass);
        if (s_ot_log_active) {
            port_log("ot: ---- pass %ld ----", s_ot_pass);
        }
    }
    vram_upload();
    s_draw_off_x = 0;   /* draw origin resets each OT pass */
    s_draw_off_y = 0;
    p = *ot;
    while (p != OT_TERM && guard++ < 200000) {
        p = render_prim((u_char *)(uintptr_t)p);
    }
#else
    (void)ot;
#endif
}

/* =============================================================================
 * Primitive initialisers (Set*): stamp the code byte; game fills the rest.
 * (len is not stored -- the OT walk follows 32-bit pointers, not len.)
 * ========================================================================== */
static void set_code(void *p, u_char code) {
    if (p) {
        ((u_char *)p)[7] = code;
    }
}

void SetPolyF4(POLY_F4 *p)   { set_code(p, GP_POLY_F4); }
void SetPolyFT4(POLY_FT4 *p) { set_code(p, GP_POLY_FT4); }
void SetPolyG4(POLY_G4 *p)   { set_code(p, GP_POLY_G4); }
void SetPolyGT4(POLY_GT4 *p) { set_code(p, GP_POLY_GT4); }
void SetSprt(SPRT *p)        { set_code(p, GP_SPRT); }
void SetSprt8(SPRT_8 *p)     { set_code(p, GP_SPRT_8); }
void SetSprt16(SPRT_16 *p)   { set_code(p, GP_SPRT_16); }
void SetTile(TILE *p)        { set_code(p, GP_TILE); }

void SetSemiTrans(void *p, int abe) {
    if (p) {
        if (abe) ((u_char *)p)[7] |= 0x02; else ((u_char *)p)[7] &= ~0x02;
    }
}

void SetShadeTex(void *p, int tge) {
    if (p) {
        if (tge) ((u_char *)p)[7] |= 0x01; else ((u_char *)p)[7] &= ~0x01;
    }
}

DR_TPAGE *SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage) {
    (void)dfe; (void)dtd;
    if (p) {
        /* Do NOT touch p->tag: the PSY-Q macro only sets the tag's len byte,
         * never the next-pointer. RenderTilemapSprites16x16 calls this AFTER
         * CatPrim-chaining the packet -- zeroing the tag here severed the
         * SPRT_16 chain at every occupied tile (the level-1 scrambled-tiles
         * bug: stale s_cur_tpage for nearly every tile). */
        ((u_char *)p)[7] = GP_DR_TPAGE;
        ((u_short *)p)[2] = (u_short)tpage;
    }
    return p;
}

/* SetDrawOffset(p, ofs): build a DR_OFFSET packet holding a signed draw-origin
 * translation (ofs[0]=X, ofs[1]=Y as shorts). render_prim applies it to every
 * subsequent primitive until the next DR_OFFSET. Stores the two shorts right
 * after the tag word (offset 4/6) to match the GP_DR_OFFSET decoder. */
void SetDrawOffset(void *p, u_short *ofs) {
    if (p) {
        /* Leave the tag/next-pointer alone (same rule as SetDrawTPage): the
         * packet may already be CatPrim-chained; AddPrim (re)links it.
         * Layout: x @+4, code @+7, y @+8 -- see the GP_DR_OFFSET decoder.
         * (Writing y at +6 clobbers the code byte at +7 with y's high byte,
         * which made every DR_OFFSET packet decode as 0x00/0xFF garbage.) */
        ((u_char *)p)[7] = GP_DR_OFFSET;
        if (ofs) {
            ((short *)p)[2] = (short)ofs[0];
            ((short *)p)[4] = (short)ofs[1];
        } else {
            ((short *)p)[2] = 0;
            ((short *)p)[4] = 0;
        }
    }
}

/* AddPrims(ot, head, tail): prepend a pre-linked primitive chain (head..tail)
 * to the OT list headed by *ot. The chain's internal next-pointers are already
 * set; only the tail's next and the OT head are updated. */
void AddPrims(u_char *ot, u_char *head, u_char *tail) {
    if (!ot || !head || !tail) {
        return;
    }
    *(u_int *)tail = *(u_int *)ot;              /* tail->next = *ot */
    *(u_int *)ot = (u_int)(uintptr_t)head;      /* *ot = head       */
}


/* CatPrim(p0, p1): concatenate primitive p1 after p0 (PSX libgpu semantics).
 * In the port's OT model the 4-byte tag word (offset 0-3) is the full 32-bit
 * next-pointer that DrawOTag/render_prim follow, and the code byte lives at
 * offset 7 (a separate word), so linking is just setting p0's tag to p1. The
 * code byte and payload of both primitives are preserved. Returns p1 so callers
 * can keep a running tail. */
void *CatPrim(void *p0, void *p1) {
    if (p0) {
        *(u_int *)p0 = (u_int)(uintptr_t)p1;
    }
    return p1;
}

/* =============================================================================
 * Draw / display environment + frame sync
 * ========================================================================== */
static DRAWENV s_draw_env;
static DISPENV s_disp_env;

DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h) {
    if (!env) return env;
    memset(env, 0, sizeof(*env));
    env->clip.x = (short)x; env->clip.y = (short)y;
    env->clip.w = (short)w; env->clip.h = (short)h;
    env->ofs[0] = (short)x; env->ofs[1] = (short)y;
    env->dtd = 1;
    return env;
}

DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h) {
    if (!env) return env;
    memset(env, 0, sizeof(*env));
    env->disp.x = (short)x; env->disp.y = (short)y;
    env->disp.w = (short)w; env->disp.h = (short)h;
    return env;
}

void PutDrawEnv(DRAWENV *env) { if (env) s_draw_env = *env; }
void PutDispEnv(DISPENV *env) { if (env) s_disp_env = *env; }

int SetDispMask(int mask) { (void)mask; return 0; }
void ResetGraph(int mode) { (void)mode; }
int SetVideoMode(int mode) { (void)mode; return 0; }
int GetVideoMode(void) { return 1; }   /* 1 = PAL */

/* DrawSync(0): wait for the GPU. On PC the OT is rendered synchronously in
 * DrawOTag, so there is nothing to wait for. */
void DrawSync(int mode) { (void)mode; }

/* =============================================================================
 * Debug font (Fnt) -- minimal: accepted, no glyphs yet (Phase 2 overlay).
 * ========================================================================== */
void FntLoad(int x, int y) { (void)x; (void)y; }
int  FntOpen(int x, int y, int w, int h, int isbg, int n) {
    (void)x; (void)y; (void)w; (void)h; (void)isbg; (void)n; return 0;
}
int  FntPrint(int id, const char *fmt, ...) { (void)id; (void)fmt; return 0; }
u_int *FntFlush(int id) { (void)id; return 0; }
void SetDumpFnt(int id) { (void)id; }

/* =============================================================================
 * Backend lifecycle + frame present (port_hal.h)
 * ========================================================================== */
#if GPU_USE_GL
/* GLSL 130 CLUT-expansion shader: reads the packed VRAM word via texelFetch on
 * the R16UI integer texture, extracts the 4-/8-/16-bit texel, resolves the CLUT
 * colour, discards fully-transparent (0x0000) texels, and modulates by the
 * primitive tint. Uses compatibility built-ins (gl_Vertex / gl_MultiTexCoord0 /
 * gl_Color) so the fixed-function immediate-mode draw path still feeds it. */
static const char *k_vs_src =
    "#version 130\n"
    "out vec2 vTexel;\n"
    "out vec4 vTint;\n"
    "void main(){\n"
    "  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "  vTexel = gl_MultiTexCoord0.xy;\n"
    "  vTint = gl_Color;\n"
    "}\n";
static const char *k_fs_src =
    "#version 130\n"
    "uniform usampler2D uVram;\n"
    "uniform int   uBitDepth;\n"
    "uniform ivec2 uPageBase;\n"
    "uniform ivec2 uClutBase;\n"
    "uniform int   uDebugTransp;\n"
    "uniform int   uStpMode;\n"   /* 0=all, 1=only STP==0, 2=only STP==1 */
    "in vec2 vTexel;\n"
    "in vec4 vTint;\n"
    "out vec4 oColor;\n"
    "int vramAt(int x,int y){ return int(texelFetch(uVram, ivec2(x,y), 0).r); }\n"
    "void main(){\n"
    "  int u = int(floor(vTexel.x));\n"
    "  int v = int(floor(vTexel.y));\n"
    "  int color;\n"
    "  if (uBitDepth == 2) {\n"
    "    color = vramAt(uPageBase.x + u, uPageBase.y + v);\n"
    "  } else if (uBitDepth == 1) {\n"
    "    int w = vramAt(uPageBase.x + (u >> 1), uPageBase.y + v);\n"
    "    int idx = ((u & 1) != 0) ? ((w >> 8) & 0xFF) : (w & 0xFF);\n"
    "    color = vramAt(uClutBase.x + idx, uClutBase.y);\n"
    "  } else {\n"
    "    int w = vramAt(uPageBase.x + (u >> 2), uPageBase.y + v);\n"
    "    int idx = (w >> ((u & 3) * 4)) & 0xF;\n"
    "    color = vramAt(uClutBase.x + idx, uClutBase.y);\n"
    "  }\n"
    "  if (color == 0) { if (uDebugTransp != 0) { oColor = vec4(1.0,0.0,1.0,1.0); return; } discard; }\n"
    "  int stp = (color >> 15) & 1;\n"
    "  if (uStpMode == 1 && stp != 0) discard;\n"   /* opaque pass: skip STP texels */
    "  if (uStpMode == 2 && stp == 0) discard;\n"   /* blend pass: only STP texels */
    "  float r = float(color & 0x1F) / 31.0;\n"
    "  float g = float((color >> 5) & 0x1F) / 31.0;\n"
    "  float b = float((color >> 10) & 0x1F) / 31.0;\n"
    "  oColor = vec4(vec3(r,g,b) * vTint.rgb, 1.0);\n"
    "}\n";

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint sh = pglCreateShader(type);
    GL_int ok = 0;
    pglShaderSource(sh, 1, &src, NULL);
    pglCompileShader(sh);
    pglGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        pglGetShaderInfoLog(sh, sizeof(log), NULL, log);
        port_log("gpu: shader compile failed: %s", log);
        pglDeleteShader(sh);
        return 0;
    }
    return sh;
}

/* Load the GL 2.x entry points and build the CLUT shader. Returns 1 on success;
 * on failure the backend falls back to the (garbled) direct-sample path. */
static int gpu_init_shader(void) {
    GLuint vs, fs;
    GL_int ok = 0;

    pglCreateShader       = (PFNglCreateShader)      SDL_GL_GetProcAddress("glCreateShader");
    pglShaderSource       = (PFNglShaderSource)      SDL_GL_GetProcAddress("glShaderSource");
    pglCompileShader      = (PFNglCompileShader)     SDL_GL_GetProcAddress("glCompileShader");
    pglGetShaderiv        = (PFNglGetShaderiv)       SDL_GL_GetProcAddress("glGetShaderiv");
    pglGetShaderInfoLog   = (PFNglGetShaderInfoLog)  SDL_GL_GetProcAddress("glGetShaderInfoLog");
    pglDeleteShader       = (PFNglDeleteShader)      SDL_GL_GetProcAddress("glDeleteShader");
    pglCreateProgram      = (PFNglCreateProgram)     SDL_GL_GetProcAddress("glCreateProgram");
    pglAttachShader       = (PFNglAttachShader)      SDL_GL_GetProcAddress("glAttachShader");
    pglLinkProgram        = (PFNglLinkProgram)       SDL_GL_GetProcAddress("glLinkProgram");
    pglGetProgramiv       = (PFNglGetProgramiv)      SDL_GL_GetProcAddress("glGetProgramiv");
    pglGetProgramInfoLog  = (PFNglGetProgramInfoLog) SDL_GL_GetProcAddress("glGetProgramInfoLog");
    pglUseProgram         = (PFNglUseProgram)        SDL_GL_GetProcAddress("glUseProgram");
    pglGetUniformLocation = (PFNglGetUniformLocation)SDL_GL_GetProcAddress("glGetUniformLocation");
    pglUniform1i          = (PFNglUniform1i)         SDL_GL_GetProcAddress("glUniform1i");
    pglUniform2i          = (PFNglUniform2i)         SDL_GL_GetProcAddress("glUniform2i");
    pglBlendEquation      = (PFNglBlendEquation)     SDL_GL_GetProcAddress("glBlendEquation");
    pglBlendColor         = (PFNglBlendColor)        SDL_GL_GetProcAddress("glBlendColor");
    if (pglBlendColor) {
        pglBlendColor(0.f, 0.f, 0.f, 0.5f);   /* ABE rate 0: B/2 + F/2 */
    }

    if (!pglCreateShader || !pglShaderSource || !pglCompileShader || !pglGetShaderiv ||
        !pglCreateProgram || !pglAttachShader || !pglLinkProgram || !pglGetProgramiv ||
        !pglUseProgram || !pglGetUniformLocation || !pglUniform1i || !pglUniform2i) {
        port_log("gpu: GL 2.x shader entry points unavailable; CLUT expansion off");
        return 0;
    }

    vs = compile_shader(GL_VERTEX_SHADER, k_vs_src);
    fs = compile_shader(GL_FRAGMENT_SHADER, k_fs_src);
    if (!vs || !fs) {
        return 0;
    }
    s_clut_prog = pglCreateProgram();
    pglAttachShader(s_clut_prog, vs);
    pglAttachShader(s_clut_prog, fs);
    pglLinkProgram(s_clut_prog);
    pglGetProgramiv(s_clut_prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        pglGetProgramInfoLog(s_clut_prog, sizeof(log), NULL, log);
        port_log("gpu: shader link failed: %s", log);
        return 0;
    }
    s_u_vram     = pglGetUniformLocation(s_clut_prog, "uVram");
    s_u_bitdepth = pglGetUniformLocation(s_clut_prog, "uBitDepth");
    s_u_pagebase = pglGetUniformLocation(s_clut_prog, "uPageBase");
    s_u_clutbase = pglGetUniformLocation(s_clut_prog, "uClutBase");
    s_u_debugtransp = pglGetUniformLocation(s_clut_prog, "uDebugTransp");
    s_u_stpmode     = pglGetUniformLocation(s_clut_prog, "uStpMode");
    port_log("gpu: CLUT-expansion shader ready");
    return 1;
}
#endif

int port_gpu_init(void) {
#if GPU_USE_GL
    if (!s_window) {
        port_log("gpu: no window; GL disabled");
        return -1;
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenTextures(1, &s_vram_tex);
    glBindTexture(GL_TEXTURE_2D, s_vram_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, VRAM_W, VRAM_H, 0,
                 GL_RED_INTEGER, GL_UNSIGNED_SHORT, s_vram);
    s_vram_dirty = 0;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, DISP_W, DISP_H, 0, -1, 1);   /* y-down like the PSX framebuffer */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    s_have_shader = gpu_init_shader();

    port_log("gpu: GL backend up (%s), VRAM %dx%d tex, CLUT-shader=%s",
             (const char *)glGetString(GL_VERSION), VRAM_W, VRAM_H,
             s_have_shader ? "on" : "off");
    return 0;
#else
    return 0;
#endif
}

void port_gpu_shutdown(void) {
#if GPU_USE_GL
    if (s_vram_tex) {
        glDeleteTextures(1, &s_vram_tex);
        s_vram_tex = 0;
    }
#endif
}

void port_gpu_begin_frame(void) {
#if GPU_USE_GL
    int w = 0, h = 0;
    if (!s_window) return;
    SDL_GL_GetDrawableSize(s_window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.05f, 0.07f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
#endif
}

void port_gpu_present(void) {
#if GPU_USE_GL
    /* Optional VRAM dump: PORT_VRAM_DUMP=path.ppm writes the full 1024x512 VRAM
     * (BGR555 -> RGB, mask bit ignored) once, so textures/CLUTs can be inspected. */
    {
        const char *vd = getenv("PORT_VRAM_DUMP");
        const char *vdf = getenv("PORT_VRAM_DUMP_FRAME");
        static int vdone = 0;
        static long vframe = 0;
        vframe++;
        if (vd && vdf && vframe < atol(vdf)) {
            vd = NULL;   /* wait for the requested frame */
        }
        if (vd && !vdone) {
            FILE *f = fopen(vd, "wb");
            if (f) {
                int x, y;
                fprintf(f, "P6\n%d %d\n255\n", VRAM_W, VRAM_H);
                for (y = 0; y < VRAM_H; y++) {
                    for (x = 0; x < VRAM_W; x++) {
                        unsigned short w = s_vram[y * VRAM_W + x];
                        unsigned char rgb[3];
                        rgb[0] = (unsigned char)(((w) & 0x1F) << 3);
                        rgb[1] = (unsigned char)(((w >> 5) & 0x1F) << 3);
                        rgb[2] = (unsigned char)(((w >> 10) & 0x1F) << 3);
                        fwrite(rgb, 1, 3, f);
                    }
                }
                fclose(f);
                port_log("gpu: VRAM dumped -> %s", vd);
            }
            {
                /* Also dump the raw 16-bit words (keeps the STP/mask bit). */
                char rawpath[512];
                snprintf(rawpath, sizeof(rawpath), "%s.bin", vd);
                FILE *rf = fopen(rawpath, "wb");
                if (rf) {
                    fwrite(s_vram, 2, VRAM_W * VRAM_H, rf);
                    fclose(rf);
                }
            }
            vdone = 1;
        }
    }
    /* Optional framebuffer capture: PORT_CAPTURE=path.ppm dumps the drawable to
     * a binary PPM once PORT_CAPTURE_FRAME (default: the last frame) is reached.
     * Lets a headless run be inspected without a live window. */
    const char *cap = getenv("PORT_CAPTURE");
    if (cap && s_window) {
        static long s_frame = 0;
        long want = -1;
        const char *wf = getenv("PORT_CAPTURE_FRAME");
        const char *ev = getenv("PORT_CAPTURE_EVERY");   /* capture every N frames */
        long every = ev ? atol(ev) : 0;
        if (wf) want = atol(wf);
        s_frame++;
        if (want < 0 || s_frame == want || (every > 0 && s_frame % every == 0)) {
            char pathbuf[512];
            if (strchr(cap, '%')) {   /* e.g. PORT_CAPTURE=frame_%04ld.ppm */
                snprintf(pathbuf, sizeof(pathbuf), cap, s_frame);
                cap = pathbuf;
            }
            int w = 0, h = 0;
            SDL_GL_GetDrawableSize(s_window, &w, &h);
            if (w > 0 && h > 0) {
                unsigned char *px = (unsigned char *)malloc((size_t)w * h * 3);
                if (px) {
                    FILE *f;
                    glPixelStorei(GL_PACK_ALIGNMENT, 1);
                    glReadBuffer(GL_BACK);
                    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px);
                    f = fopen(cap, "wb");
                    if (f) {
                        int y;
                        fprintf(f, "P6\n%d %d\n255\n", w, h);
                        /* GL origin is bottom-left; flip to top-left for the PPM. */
                        for (y = h - 1; y >= 0; y--) {
                            fwrite(px + (size_t)y * w * 3, 1, (size_t)w * 3, f);
                        }
                        fclose(f);
                        port_log("gpu: captured frame %ld -> %s (%dx%d)", s_frame, cap, w, h);
                    }
                    free(px);
                }
            }
        }
    }
    if (s_window) SDL_GL_SwapWindow(s_window);
#endif
}

/* Synthetic smoke test (called from port_boot when PORT_GPU_SELFTEST set):
 * builds a 3-primitive OT by hand and walks it, exercising AddPrim -> DrawOTag
 * without any game code. */
void port_gpu_selftest(void) {
#if GPU_USE_GL
    static TILE a, b;
    static SPRT c;
    u_int ot = OT_TERM;
    memset(&a, 0, sizeof a); memset(&b, 0, sizeof b); memset(&c, 0, sizeof c);
    SetTile(&a); a.x0 = 10; a.y0 = 10; a.w = 40; a.h = 40; a.r0 = 200; a.g0 = 40; a.b0 = 40;
    SetTile(&b); b.x0 = 60; b.y0 = 60; b.w = 40; b.h = 40; b.r0 = 40; b.g0 = 200; b.b0 = 40;
    SetSprt(&c); c.x0 = 120; c.y0 = 20; c.w = 32; c.h = 32; c.u0 = 0; c.v0 = 0;
    AddPrim((u_char *)&ot, (u_char *)&a);
    AddPrim((u_char *)&ot, (u_char *)&b);
    AddPrim((u_char *)&ot, (u_char *)&c);
    DrawOTag(&ot);
    port_log("gpu: self-test walked 3-primitive OT ok");
#endif
}
