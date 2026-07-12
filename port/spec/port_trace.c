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
#include <stdint.h>
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

/* ---- host->PSX pointer translation (arena plan Tier 3, generalized) -------
 * Generated table (_autoaddrmap.c): one {host symbol, PSX address} pair per
 * symbol_addrs.txt entry in the PS-EXE range. At init we keep the entries
 * that resolved to real host storage OUTSIDE the mirrored arena (functions,
 * weak stubs, weak blobs, strong globals) and sort them by host address; at
 * dump time every aligned 4-byte word equal to such a host address is
 * rewritten to its PSX value. With the arena mirrored at 0x80000000, data
 * pointers into the arena are already PSX-true, so after this pass the only
 * host-address residue is interior pointers (symbol+offset) and stale bytes
 * in freed blocks. PORT_TRACE_RAW=1 disables the pass. */
typedef struct { char *host; unsigned int size; unsigned int psx; } PortAddrMapEntry;
extern PortAddrMapEntry g_port_addrmap[];
extern unsigned int g_port_addrmap_len;

static PortAddrMapEntry *g_xlat     = NULL;
static unsigned          g_xlat_len = 0;

static int xlat_cmp(const void *a, const void *b) {
    uintptr_t ha = (uintptr_t)((const PortAddrMapEntry *)a)->host;
    uintptr_t hb = (uintptr_t)((const PortAddrMapEntry *)b)->host;
    return ha < hb ? -1 : ha > hb ? 1 : 0;
}

static void xlat_init(void) {
    unsigned i, n = 0;
    if (getenv("PORT_TRACE_RAW")) return;
    g_xlat = (PortAddrMapEntry *)malloc(g_port_addrmap_len * sizeof *g_xlat);
    if (!g_xlat) return;
    for (i = 0; i < g_port_addrmap_len; i++) {
        uintptr_t h = (uintptr_t)g_port_addrmap[i].host;
        if (h == 0) continue;
        if (h - (uintptr_t)PSX_RAM_BASE < PSX_RAM_SIZE) continue; /* in-arena */
        g_xlat[n++] = g_port_addrmap[i];
    }
    g_xlat_len = n;
    qsort(g_xlat, n, sizeof *g_xlat, xlat_cmp);
    /* Symbol sizes come from symbol_addrs.txt PSX-gap heuristics; in HOST
     * layout neighbours differ, so clip every extent at the next symbol's
     * host start. Interior pointers (symbol+offset) then translate to
     * psx+offset without one symbol's range swallowing another's base. */
    for (i = 0; i + 1 < n; i++) {
        uintptr_t next = (uintptr_t)g_xlat[i + 1].host;
        uintptr_t here = (uintptr_t)g_xlat[i].host;
        if (here + g_xlat[i].size > next) {
            g_xlat[i].size = (unsigned)(next - here);
        }
    }
    port_log("trace: host->PSX pointer map: %u/%u symbols resolved",
             n, g_port_addrmap_len);
}

static void xlat_apply(unsigned char *buf, unsigned size) {
    unsigned *w = (unsigned *)buf;
    unsigned n = size / 4, i;
    if (g_xlat_len == 0) return;
    for (i = 0; i < n; i++) {
        unsigned v = w[i];
        unsigned lo = 0, hi = g_xlat_len, e;
        /* only 4-aligned values can be stored object pointers here; unaligned
         * hits are junk words that happen to fall in a host range under this
         * run's ASLR (observed as asymmetric false translations) */
        if (v & 3) {
            continue;
        }
        /* cheap reject: outside the whole mapped host span */
        if (v < (uintptr_t)g_xlat[0].host ||
            v >= (uintptr_t)g_xlat[g_xlat_len - 1].host +
                 g_xlat[g_xlat_len - 1].size) {
            continue;
        }
        /* greatest entry with host <= v */
        while (lo < hi) {
            unsigned mid = (lo + hi) / 2;
            if ((uintptr_t)g_xlat[mid].host <= v) lo = mid + 1; else hi = mid;
        }
        if (lo == 0) continue;
        e = lo - 1;
        if (v - (uintptr_t)g_xlat[e].host < g_xlat[e].size) {
            w[i] = g_xlat[e].psx + (unsigned)(v - (uintptr_t)g_xlat[e].host);
        }
    }
}

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

    /* Bare-GameState region needs no arena at all (pre-arena fast path) --
     * but only when no pointer translation runs (xlat must not touch the
     * LIVE object, only a staged copy). */
    if (g_base == GS_ADDR && g_size == GS_SIZE && g_xlat_len == 0) {
        return g_GameStateBase;
    }

    if (g_base == GS_ADDR && g_size == GS_SIZE) {
        memcpy(g_buf, g_GameStateBase, g_size);
    } else {
        memcpy(g_buf, port_psx2host(g_base), g_size);

        /* Overlay the GameState object at its PSX address (it lives outside
         * the arena until globals move in — Tier 2 of the arena plan). */
        lo = g_base; hi = g_base + g_size;
        dlo = GS_ADDR < lo ? lo : GS_ADDR;
        dhi = GS_ADDR + GS_SIZE > hi ? hi : GS_ADDR + GS_SIZE;
        if (dlo < dhi) {
            memcpy(g_buf + (dlo - lo), g_GameStateBase + (dlo - GS_ADDR),
                   dhi - dlo);
        }
    }
    xlat_apply(g_buf, g_size);
    return g_buf;
}

static void port_trace_init(void) {
    const char *fifo = getenv("PORT_TRACE_FIFO");
    const char *dir  = getenv("PORT_TRACE_OUT");
    g_init = 1;
    if (!fifo && !dir) return;

    parse_region(getenv("PORT_TRACE_REGION"));
    xlat_init();
    if (!(g_base == GS_ADDR && g_size == GS_SIZE) || g_xlat_len != 0) {
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
