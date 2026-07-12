/* =============================================================================
 * port_heap.c  --  PSX-mirror RAM arena + boot-global setup (TARGET_PC)
 * =============================================================================
 * On PSX the early-init routine (`main_8008E6E0`, the gcc `__main` slot) points
 * the global `g_pBlbHeapBase` (0x800A5954) at the game's working-RAM region at
 * 0x800907EC. Everything downstream -- the GPU context + double-buffered
 * draw/disp envs + ordering table (InitGraphicsSystem), the heap block
 * descriptors, the level/sprite buffers -- lives at fixed offsets from it
 * inside the flat 2 MB PSX RAM map.
 *
 * On PC we reproduce that map with one arena representing PSX RAM
 * 0x80000000..0x80200000 (docs/plans/psx-mirror-arena.md):
 *
 *   host = arena_base + (psx_addr - 0x80000000)          [Tier 1]
 *
 * and we first try to mmap the arena AT 0x80000000 (possible because the build
 * is -m32), which makes host pointers into the arena bit-identical to PS1 RAM
 * addresses [Tier 2's mapping]. If the fixed mapping is refused (hardened
 * kernel, address in use, PORT_NO_FIXED_ARENA=1) we fall back to an anonymous
 * mapping anywhere and Tier 1 constant-delta math still holds.
 *
 * The arena is over-allocated past the 2 MB PSX image with a slack region so
 * PC-side size paranoia (buffers sized larger than their PSX footprint while
 * struct sizes are still being confirmed) overruns into mapped memory instead
 * of crashing. Only the first 2 MB is the PSX mirror; the trace dumper
 * (port_trace.c) never emits the slack.
 * ========================================================================== */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) || defined(__APPLE__)
#  include <sys/mman.h>
#  define PORT_HAVE_MMAP 1
#endif

#include "common.h"
#include "globals.h"
#include "port_runtime.h"
#include "port_hal.h"

#define PSX_RAM_BASE  0x80000000u
#define PSX_RAM_SIZE  0x00200000u              /* 2 MB retail RAM image      */
#define ARENA_SLACK   (14u * 1024u * 1024u)    /* overrun headroom (unmirrored) */
#define ARENA_TOTAL   (PSX_RAM_SIZE + ARENA_SLACK)

/* PSX address g_pBlbHeapBase points at (start of the .data working region). */
#define PSX_HEAP_OWNER 0x800907ECu

static unsigned char *s_arena;      /* host address of PSX 0x80000000        */
static int            s_mirrored;   /* 1 = arena really sits at 0x80000000   */
static int            s_arena_mmap; /* 1 = release with munmap, 0 = free     */

unsigned char *port_ram_base(void) { return s_arena; }
int port_ram_mirrored(void) { return s_mirrored; }

void *port_psx2host(unsigned int psx_addr) {
    return s_arena + (psx_addr - PSX_RAM_BASE);
}

int port_heap_init(void) {
    if (s_arena) {
        return 0;
    }

#if PORT_HAVE_MMAP && defined(MAP_FIXED_NOREPLACE)
    if (!getenv("PORT_NO_FIXED_ARENA")) {
        void *want = (void *)(uintptr_t)PSX_RAM_BASE;
        void *got = mmap(want, ARENA_TOTAL, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE,
                         -1, 0);
        if (got == want) {
            s_arena = (unsigned char *)got;
            s_mirrored = 1;
            s_arena_mmap = 1;
        } else if (got != MAP_FAILED) {
            munmap(got, ARENA_TOTAL);   /* placed elsewhere: not useful here */
        }
    }
#endif
    if (!s_arena) {
        /* Tier 1 fallback: arena anywhere, constant-delta from PSX addrs. */
        s_arena = (unsigned char *)malloc(ARENA_TOTAL);
        if (!s_arena) {
            port_log("heap: arena alloc(%u) failed", ARENA_TOTAL);
            return -1;
        }
        memset(s_arena, 0, ARENA_TOTAL);
    }

    g_pBlbHeapBase = port_psx2host(PSX_HEAP_OWNER);
    port_log("heap: PSX arena @ %p (%s, 2MB mirror + %uMB slack), "
             "g_pBlbHeapBase = %p",
             (void *)s_arena, s_mirrored ? "MIRRORED at 0x80000000" : "offset",
             ARENA_SLACK / (1024 * 1024), (void *)g_pBlbHeapBase);

    /* Seed the arena with the PS-EXE image, exactly as the PS1 BIOS loads it
     * (t_addr/t_size from the header, payload at file +0x800). This is the
     * arena plan's "one-time init copy", generalized: the whole text/data/
     * sdata image (0x80010000..0x800A6800 for SLES-010.90) gets its ROM-true
     * initial bytes, so full-RAM dumps match a PS1 dump over every static
     * region. Game code still reads/writes its HOST globals -- the seeded
     * bytes are dump furniture until the Tier 2 defsym step migrates symbols
     * into the arena -- so this changes no behavior. Optional: missing file
     * just logs (PORT_PSX_EXE overrides the default path). */
    {
        const char *exe = getenv("PORT_PSX_EXE");
        FILE *f;
        if (!exe || !*exe) exe = "bin/SLES_010.90";
        f = fopen(exe, "rb");
        if (f) {
            unsigned char hdr[0x20];
            if (fread(hdr, 1, sizeof hdr, f) == sizeof hdr &&
                memcmp(hdr, "PS-X EXE", 8) == 0) {
                unsigned t_addr = hdr[0x18] | (hdr[0x19] << 8) |
                                  ((unsigned)hdr[0x1A] << 16) |
                                  ((unsigned)hdr[0x1B] << 24);
                unsigned t_size = hdr[0x1C] | (hdr[0x1D] << 8) |
                                  ((unsigned)hdr[0x1E] << 16) |
                                  ((unsigned)hdr[0x1F] << 24);
                if (t_addr >= PSX_RAM_BASE &&
                    t_addr + t_size <= PSX_RAM_BASE + PSX_RAM_SIZE &&
                    fseek(f, 0x800, SEEK_SET) == 0 &&
                    fread(port_psx2host(t_addr), 1, t_size, f) == t_size) {
                    port_log("heap: PS-EXE image seeded @ 0x%08x (+0x%x) from %s",
                             t_addr, t_size, exe);
                } else {
                    port_log("heap: PS-EXE image seed FAILED (bad range/read) %s", exe);
                }
            } else {
                port_log("heap: %s is not a PS-X EXE; arena not seeded", exe);
            }
            fclose(f);
        } else {
            port_log("heap: no PS-EXE at %s; arena stays zero-seeded "
                     "(set PORT_PSX_EXE)", exe);
        }
    }
    return 0;
}

void port_heap_shutdown(void) {
    if (s_arena) {
#if PORT_HAVE_MMAP
        if (s_arena_mmap) {
            munmap(s_arena, ARENA_TOTAL);
        } else
#endif
        {
            free(s_arena);
        }
        s_arena = NULL;
        s_mirrored = 0;
        s_arena_mmap = 0;
        g_pBlbHeapBase = NULL;
    }
}
