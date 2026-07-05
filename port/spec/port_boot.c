/* =============================================================================
 * port_boot.c  --  PC-port game boot / HAL bring-up harness (TARGET_PC)
 * =============================================================================
 * Provides the strong port_game_main() that host_main() hands off to. In
 * Phase 1 this is a HAL BRING-UP HARNESS: it initialises every backend and runs
 * a real 50 Hz PAL frame loop (clear -> present -> input -> vsync-callback ->
 * pace) so the whole HAL can be exercised and watched live, before any game
 * logic exists. It is also the skeleton the Phase-2 converted PSX `main` grows
 * from (the render-dispatch + tick calls slot into the marked spot below).
 *
 * See docs/plans/pc-port.md CP-1.7 (Phase 1 exit gate: boot drives the HAL
 * loop without aborting).
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

void port_game_main(void) {
    unsigned frames;
    /* Test hook: auto-exit after N frames (0 = run until quit). Lets the loop
     * be validated non-interactively / headless without a quit event. */
    const char *cap_env = getenv("PORT_MAX_FRAMES");
    unsigned frame_cap = cap_env ? (unsigned)strtoul(cap_env, 0, 10) : 0;

    port_log("Phase 1 HAL bring-up harness (no game logic yet).");

    if (port_cd_init() == 0) {
        /* Read the BLB header sector back through the full Cd* surface (the
         * exact CdIntToPos->CdControl(Setloc)->CdRead->CdReadSync sequence the
         * game's CdBLB_ReadSectors uses) as a live end-to-end CD self-test. */
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

    for (;;) {
        port_pad_poll();
        if (port_pad_quit_requested()) {
            break;
        }

        port_gpu_begin_frame();
        /* ---- Phase 2: game render dispatch (RenderEntities + layer OT flush)
         * slots in here; the harness just shows the clear color for now. ---- */
        port_gpu_present();

        port_bios_fire_vsync();
        port_bios_advance_frame();

        if (frame_cap && port_bios_vblank_count() >= frame_cap) {
            break;
        }

#if defined(PORT_HAVE_SDL2)
        SDL_Delay(PORT_FRAME_MS);
#endif
    }

    frames = port_bios_vblank_count();
    port_log("harness exited after %u frames.", frames);

    port_spu_shutdown();
    port_gpu_shutdown();
}

