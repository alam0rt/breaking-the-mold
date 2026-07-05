/* =============================================================================
 * port_heap.c  --  PC-port BLB heap + boot-global setup (TARGET_PC)
 * =============================================================================
 * On PSX the early-init routine (`main_8008E6E0`, the gcc `__main` slot) carves
 * the game's main work heap out of RAM and stashes its base pointer in the
 * global `g_pBlbHeapBase` (0x800A5954). Everything downstream -- the GPU context
 * + double-buffered draw/disp envs + ordering table (InitGraphicsSystem), the
 * BLB asset heap (blbmem.c), the level/sprite buffers -- lives inside it.
 *
 * On PC there is no fixed RAM map, so we simply malloc a large zeroed block and
 * point g_pBlbHeapBase at it. The exact address is irrelevant; only that it is
 * valid, writable, and large enough for the GPU context (~0xF000 bytes at the
 * base) plus the asset heap that follows. See docs/plans/pc-port.md CP-2.1.
 * ========================================================================== */
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "globals.h"
#include "port_runtime.h"
#include "port_hal.h"

/* PSX retail runs the whole game in 2 MB of RAM. We give the heap a generous
 * 16 MB so nothing downstream can overrun it while struct sizes are still being
 * confirmed; the host has memory to spare. */
#define PORT_HEAP_BYTES (16 * 1024 * 1024)

static unsigned char *s_heap;

int port_heap_init(void) {
    if (s_heap) {
        return 0;
    }
    s_heap = (unsigned char *)malloc(PORT_HEAP_BYTES);
    if (!s_heap) {
        port_log("heap: malloc(%d) failed", PORT_HEAP_BYTES);
        return -1;
    }
    memset(s_heap, 0, PORT_HEAP_BYTES);
    g_pBlbHeapBase = s_heap;
    port_log("heap: BLB heap = %p (%d MB)", (void *)s_heap,
             PORT_HEAP_BYTES / (1024 * 1024));
    return 0;
}

void port_heap_shutdown(void) {
    if (s_heap) {
        free(s_heap);
        s_heap = NULL;
        g_pBlbHeapBase = NULL;
    }
}
