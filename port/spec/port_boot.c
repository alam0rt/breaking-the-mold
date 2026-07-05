/* =============================================================================
 * port_boot.c  --  PC-port game boot / frame driver (TARGET_PC)
 * =============================================================================
 * Provides the strong port_game_main() that host_main() hands off to. It brings
 * up every HAL backend + the BLB heap, then runs the game boot spine
 * (port_game_boot_init) and drives the game frame loop (port_game_boot_frame),
 * weaving in SDL present / input / 50 Hz pacing between iterations.
 *
 * During Phase 2 bring-up the boot / frame path calls into functions that are
 * still only asm (generated weak stubs) -- the first such call aborts with its
 * name, which is the next conversion task. Set PORT_HAL_HARNESS=1 to run the
 * old HAL-only smoke loop instead (no game code), useful for isolating backend
 * issues. See docs/plans/pc-port.md CP-2.1.
 * ========================================================================== */
#include "port_runtime.h"
#include "port_hal.h"

#include <stdlib.h>

#if defined(PORT_HAVE_SDL2)
#  include <SDL2/SDL.h>
#endif

/* PAL is 50 Hz -> 20 ms/frame. The GL swap is also vsync-locked (host_main sets
 * SwapInterval 1); the delay is a floor so we don't spin on a tearing display. */
#define PORT_FRAME_MS 20

/* Bring up every HAL backend + the BLB heap. Returns 0 on success. */
static int port_hal_bringup(void) {
    if (port_cd_init() == 0) {
        static unsigned char sec0[2048];
        if (port_cd_read_sectors(0, 1, sec0) == 0) {
            port_log("cd: self-test read sector 0 ok (%02x %02x %02x %02x)",
                     sec0[0], sec0[1], sec0[2], sec0[3]);
        }
    }
    if (port_gpu_init() != 0) {
        port_panic("GPU backend failed to initialise");
    }
    port_spu_init();
    port_pad_init();
    if (port_heap_init() != 0) {
        port_panic("BLB heap allocation failed");
    }
    return 0;
}

/* HAL-only smoke loop (PORT_HAL_HARNESS=1): no game code, just clear+present. */
static void port_hal_harness(unsigned frame_cap) {
    port_log("Phase 1 HAL harness (PORT_HAL_HARNESS): no game logic.");
    for (;;) {
        port_pad_poll();
        if (port_pad_quit_requested()) break;
        port_gpu_begin_frame();
        if (getenv("PORT_GPU_SELFTEST")) port_gpu_selftest();
        port_gpu_present();
        port_bios_fire_vsync();
        port_bios_advance_frame();
        if (frame_cap && port_bios_vblank_count() >= frame_cap) break;
#if defined(PORT_HAVE_SDL2)
        SDL_Delay(PORT_FRAME_MS);
#endif
    }
}

void port_game_main(void) {
    const char *cap_env = getenv("PORT_MAX_FRAMES");
    unsigned frame_cap = cap_env ? (unsigned)strtoul(cap_env, 0, 10) : 0;

    port_log("Skullmonkeys game boot starting.");
    port_hal_bringup();

    if (getenv("PORT_HAL_HARNESS")) {
        port_hal_harness(frame_cap);
        goto done;
    }

    /* Boot prologue: aborts (with the function name) at the first still-asm
     * boot callee -- that name is the next Phase-2 conversion task. */
    port_game_boot_init();
    port_log("game boot prologue complete; entering frame loop.");

    for (;;) {
        port_pad_poll();
        if (port_pad_quit_requested()) break;

        port_gpu_begin_frame();
        port_game_boot_frame();     /* one game frame (may abort until converted) */
        /* Fire the registered VSync ISR (InitGraphicsSystem installs
         * TriggerBufferSwapIfReady): if the frame is ready it runs
         * SwapBuffersAndClearOT -> DrawOTag, rendering the OT to the GL back
         * buffer, which port_gpu_present then swaps to screen. */
        port_bios_fire_vsync();
        port_gpu_present();

        port_bios_advance_frame();
        if (frame_cap && port_bios_vblank_count() >= frame_cap) break;
#if defined(PORT_HAVE_SDL2)
        SDL_Delay(PORT_FRAME_MS);
#endif
    }

done:
    port_log("shutdown after %u frames.", port_bios_vblank_count());
    port_spu_shutdown();
    port_gpu_shutdown();
    port_heap_shutdown();
}


