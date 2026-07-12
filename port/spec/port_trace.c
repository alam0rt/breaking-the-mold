/* =============================================================================
 * port_trace.c  --  optional per-frame RAM dump for the PC port.
 *
 * Emits a PSX-addressed RAM region once per frame in the SAME "FRM1" wire
 * format that scripts/ram_dumper.lua streams from PCSX-Redux, so the PS1
 * reference consumer (tools/pcsx_stream.py) can ingest a port capture into an
 * identical SQLite and `diffdb` the two.
 *
 * Since the PSX-mirror arena (port_heap.c, docs/plans/psx-mirror-arena.md),
 * the port's heap + level data + sub-heap live at their PSX offsets inside one
 * arena, so any slice of PSX RAM can be dumped byte-comparably. The GameState
 * object itself still lives outside the arena (a weak blob in _autoglobals.c
 * until the Tier 2 defsym work), so it is overlaid into the dump at its PSX
 * address 0x8009DC40.
 *
 * No-op unless PORT_TRACE_FIFO or PORT_TRACE_OUT is set -> zero cost normally.
 *   PORT_TRACE_FIFO=<path>    stream FRM1 records to a pipe (pair with `consume`)
 *   PORT_TRACE_OUT=<dir>      write frame_%08u.bin + manifest.json (pcsx_trace.py)
 *   PORT_TRACE_REGION=<spec>  what to dump; mirrors RAMDUMP_REGION:
 *                               full            0x80000000:0x200000 (whole RAM)
 *                               gamestate       0x8009DC40:0x13000
 *                               0xADDR:SIZE     any slice of PSX RAM
 *                             default: 0x8009DC40:0x1A0 (bare GameState object,
 *                             the pre-arena behavior; `make port-trace` always
 *                             sets the env from its REGION var, default full).
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port_hal.h"

/* GameState backing object: weak storage in _autoglobals.c, or a strong C def
 * once the boot path is converted. Same 0x1A0 layout as PS1 0x8009DC40. */
extern unsigned char g_GameStateBase[];

#define PSX_RAM_BASE 0x80000000u
#define PSX_RAM_SIZE 0x00200000u
#define GS_ADDR      0x8009DC40u
#define GS_SIZE      0x1A0u

static FILE          *g_tf   = NULL;   /* fifo/stream handle */
static const char    *g_dir  = NULL;   /* per-frame-file dir */
static int            g_mode = 0;      /* 0=off 1=stream 2=files */
static int            g_init = 0;
static unsigned       g_frame = 0;
static unsigned       g_base  = GS_ADDR;  /* region base (PSX address) */
static unsigned       g_size  = GS_SIZE;  /* region size in bytes      */
static unsigned char *g_buf   = NULL;     /* staging copy of the region */

static void put_u32le(FILE *f, unsigned v) {
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xFF);
    b[1] = (unsigned char)((v >> 8) & 0xFF);
    b[2] = (unsigned char)((v >> 16) & 0xFF);
    b[3] = (unsigned char)((v >> 24) & 0xFF);
    fwrite(b, 1, 4, f);
}

/* PORT_TRACE_REGION spec -> g_base/g_size. Must stay in sync with
 * parse_region() in scripts/ram_dumper.lua / resolve_region() in
 * tools/pcsx_stream.py (plus the bare-GameState default). */
static void parse_region(const char *spec) {
    if (!spec || !*spec) return;                    /* keep GS default */
    if (strcmp(spec, "full") == 0) {
        g_base = PSX_RAM_BASE; g_size = PSX_RAM_SIZE;
    } else if (strcmp(spec, "gamestate") == 0) {
        g_base = GS_ADDR; g_size = 0x13000;
    } else {
        const char *colon = strchr(spec, ':');
        if (colon) {
            g_base = (unsigned)strtoul(spec, NULL, 0);
            g_size = (unsigned)strtoul(colon + 1, NULL, 0);
        }
    }
    /* clamp to the mirrored 2 MB image */
    if (g_base < PSX_RAM_BASE) g_base = PSX_RAM_BASE;
    if (g_base + g_size > PSX_RAM_BASE + PSX_RAM_SIZE) {
        g_size = PSX_RAM_BASE + PSX_RAM_SIZE - g_base;
    }
}

/* Fill g_buf with the region bytes: arena slice + non-arena object overlays. */
static const unsigned char *region_bytes(void) {
    unsigned lo, hi, dlo, dhi;

    /* Bare-GameState region needs no arena at all (pre-arena fast path). */
    if (g_base == GS_ADDR && g_size == GS_SIZE) {
        return g_GameStateBase;
    }

    memcpy(g_buf, port_psx2host(g_base), g_size);

    /* Overlay the GameState object at its PSX address (it lives outside the
     * arena until globals move in — Tier 2 of the arena plan). */
    lo = g_base; hi = g_base + g_size;
    dlo = GS_ADDR < lo ? lo : GS_ADDR;
    dhi = GS_ADDR + GS_SIZE > hi ? hi : GS_ADDR + GS_SIZE;
    if (dlo < dhi) {
        memcpy(g_buf + (dlo - lo), g_GameStateBase + (dlo - GS_ADDR), dhi - dlo);
    }
    return g_buf;
}

static void port_trace_init(void) {
    const char *fifo = getenv("PORT_TRACE_FIFO");
    const char *dir  = getenv("PORT_TRACE_OUT");
    g_init = 1;
    if (!fifo && !dir) return;

    parse_region(getenv("PORT_TRACE_REGION"));
    if (!(g_base == GS_ADDR && g_size == GS_SIZE)) {
        g_buf = (unsigned char *)malloc(g_size);
        if (!g_buf) return;                        /* leave tracing off */
    }

    if (fifo && *fifo) {
        g_tf = fopen(fifo, "wb");   /* blocks until the consumer opens read end */
        g_mode = g_tf ? 1 : 0;
    } else if (dir && *dir) {
        FILE *m;
        char path[600];
        g_dir = dir;
        g_mode = 2;
        /* one-shot manifest so tools/pcsx_trace.py can decode the .bin frames */
        snprintf(path, sizeof path, "%s/manifest.json", dir);
        m = fopen(path, "w");
        if (m) {
            fprintf(m, "{\n  \"region_base\": %u,\n  \"region_size\": %u,\n"
                       "  \"gamestate_addr\": %u,\n  \"source\": \"port\"\n}\n",
                    g_base, g_size, GS_ADDR);
            fclose(m);
        }
    }
}

/* Call once per port frame. */
void port_trace_frame(void) {
    const unsigned char *bytes;

    if (!g_init) port_trace_init();
    if (g_mode == 0) return;
    g_frame++;
    bytes = region_bytes();

    if (g_mode == 1) {
        fwrite("FRM1", 1, 4, g_tf);
        put_u32le(g_tf, g_frame);
        put_u32le(g_tf, g_size);
        fwrite(bytes, 1, g_size, g_tf);
        fflush(g_tf);
    } else {
        char path[600];
        FILE *f;
        snprintf(path, sizeof path, "%s/frame_%08u.bin", g_dir, g_frame);
        f = fopen(path, "wb");
        if (f) { fwrite(bytes, 1, g_size, f); fclose(f); }
    }
}
