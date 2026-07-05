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
    GP_DR_TPAGE = 0xE1
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
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VRAM_W, VRAM_H,
                    GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, s_vram);
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
/* Resolve a tpage word to its VRAM pixel base (X in 64px units, Y in 256px). */
static void tpage_base(unsigned tpage, int *bx, int *by) {
    *bx = (int)((tpage & 0x0F) << 6);
    *by = (int)(((tpage >> 4) & 1) << 8);
}

static void quad_tex(short x0, short y0, short x1, short y1,
                     int u0, int v0, int u1, int v1, unsigned tpage) {
    int bx, by;
    float tu0, tv0, tu1, tv1;
    tpage_base(tpage, &bx, &by);
    tu0 = (float)(bx + u0) / VRAM_W; tv0 = (float)(by + v0) / VRAM_H;
    tu1 = (float)(bx + u1) / VRAM_W; tv1 = (float)(by + v1) / VRAM_H;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, s_vram_tex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(tu0, tv0); glVertex2i(x0, y0);
    glTexCoord2f(tu1, tv0); glVertex2i(x1, y0);
    glTexCoord2f(tu0, tv1); glVertex2i(x0, y1);
    glTexCoord2f(tu1, tv1); glVertex2i(x1, y1);
    glEnd();
}

static void quad_solid(short x0, short y0, short x1, short y1,
                       u_char r, u_char g, u_char b) {
    glDisable(GL_TEXTURE_2D);
    glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2i(x0, y0); glVertex2i(x1, y0);
    glVertex2i(x0, y1); glVertex2i(x1, y1);
    glEnd();
}

/* Decode + render one primitive; returns the next-pointer (its tag word). */
static u_int render_prim(u_char *p) {
    u_int next = *(u_int *)p;
    u_char code = p[7];               /* code byte (TAG_FIELDS offset 7) */
    switch (code) {
    case GP_SPRT: {
        SPRT *s = (SPRT *)p;
        quad_tex(s->x0, s->y0, (short)(s->x0 + s->w), (short)(s->y0 + s->h),
                 s->u0, s->v0, s->u0 + s->w, s->v0 + s->h, s_cur_tpage);
        break;
    }
    case GP_SPRT_8: {
        SPRT_8 *s = (SPRT_8 *)p;
        quad_tex(s->x0, s->y0, (short)(s->x0 + 8), (short)(s->y0 + 8),
                 s->u0, s->v0, s->u0 + 8, s->v0 + 8, s_cur_tpage);
        break;
    }
    case GP_SPRT_16: {
        SPRT_16 *s = (SPRT_16 *)p;
        quad_tex(s->x0, s->y0, (short)(s->x0 + 16), (short)(s->y0 + 16),
                 s->u0, s->v0, s->u0 + 16, s->v0 + 16, s_cur_tpage);
        break;
    }
    case GP_TILE: {
        TILE *t = (TILE *)p;
        quad_solid(t->x0, t->y0, (short)(t->x0 + t->w), (short)(t->y0 + t->h),
                   t->r0, t->g0, t->b0);
        break;
    }
    case GP_POLY_F4: {
        POLY_F4 *q = (POLY_F4 *)p;
        quad_solid(q->x0, q->y0, q->x3, q->y3, q->r0, q->g0, q->b0);
        break;
    }
    case GP_POLY_FT4: {
        POLY_FT4 *q = (POLY_FT4 *)p;
        quad_tex(q->x0, q->y0, q->x3, q->y3, q->u0, q->v0, q->u3, q->v3, q->tpage);
        break;
    }
    case GP_POLY_GT4: {
        POLY_GT4 *q = (POLY_GT4 *)p;
        quad_tex(q->x0, q->y0, q->x3, q->y3, q->u0, q->v0, q->u3, q->v3, q->tpage);
        break;
    }
    case GP_DR_TPAGE: {
        s_cur_tpage = ((u_short *)p)[2];   /* tpage in the low half-word */
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
    vram_upload();
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
        p->tag = 0;
        ((u_char *)p)[7] = GP_DR_TPAGE;
        ((u_short *)p)[2] = (u_short)tpage;
    }
    return p;
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VRAM_W, VRAM_H, 0,
                 GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, s_vram);
    s_vram_dirty = 0;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, DISP_W, DISP_H, 0, -1, 1);   /* y-down like the PSX framebuffer */
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    port_log("gpu: GL backend up (%s), VRAM %dx%d tex",
             (const char *)glGetString(GL_VERSION), VRAM_W, VRAM_H);
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
