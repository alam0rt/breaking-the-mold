/* =============================================================================
 * port_trace.c  --  optional per-frame GameState dump for the PC port.
 *
 * Emits the port's GameState struct (g_GameStateBase, 0x1A0 bytes) once per
 * frame in the SAME "FRM1" wire format that scripts/ram_dumper.lua streams from
 * PCSX-Redux, so the PS1 reference consumer (tools/pcsx_stream.py) can ingest a
 * port capture into an identical SQLite and `diffdb` the two.
 *
 * The port has no contiguous PSX RAM image (each global is a separate weak
 * object in _autoglobals.c), so we dump the one contiguous object that matters
 * for behavior comparison: the GameState at the PSX address 0x8009DC40. Set the
 * consumer's region to 0x8009DC40:0x1A0 so field offsets line up.
 *
 * No-op unless PORT_TRACE_FIFO or PORT_TRACE_OUT is set -> zero cost normally.
 *   PORT_TRACE_FIFO=<path>   stream FRM1 records to a pipe (pair with `consume`)
 *   PORT_TRACE_OUT=<dir>     write frame_%08u.bin + manifest.json (pcsx_trace.py)
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GameState backing object: weak storage in _autoglobals.c, or a strong C def
 * once the boot path is converted. Same 0x1A0 layout as PS1 0x8009DC40. */
extern unsigned char g_GameStateBase[];

#define GS_ADDR 0x8009DC40u
#define GS_SIZE 0x1A0u

static FILE       *g_tf   = NULL;   /* fifo/stream handle */
static const char *g_dir  = NULL;   /* per-frame-file dir */
static int         g_mode = 0;      /* 0=off 1=stream 2=files */
static int         g_init = 0;
static unsigned    g_frame = 0;

static void put_u32le(FILE *f, unsigned v) {
    unsigned char b[4];
    b[0] = (unsigned char)(v & 0xFF);
    b[1] = (unsigned char)((v >> 8) & 0xFF);
    b[2] = (unsigned char)((v >> 16) & 0xFF);
    b[3] = (unsigned char)((v >> 24) & 0xFF);
    fwrite(b, 1, 4, f);
}

static void port_trace_init(void) {
    const char *fifo = getenv("PORT_TRACE_FIFO");
    const char *dir  = getenv("PORT_TRACE_OUT");
    g_init = 1;
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
                    GS_ADDR, GS_SIZE, GS_ADDR);
            fclose(m);
        }
    }
}

/* Call once per port frame. */
void port_trace_frame(void) {
    if (!g_init) port_trace_init();
    if (g_mode == 0) return;
    g_frame++;

    if (g_mode == 1) {
        fwrite("FRM1", 1, 4, g_tf);
        put_u32le(g_tf, g_frame);
        put_u32le(g_tf, GS_SIZE);
        fwrite(g_GameStateBase, 1, GS_SIZE, g_tf);
        fflush(g_tf);
    } else {
        char path[600];
        FILE *f;
        snprintf(path, sizeof path, "%s/frame_%08u.bin", g_dir, g_frame);
        f = fopen(path, "wb");
        if (f) { fwrite(g_GameStateBase, 1, GS_SIZE, f); fclose(f); }
    }
}
