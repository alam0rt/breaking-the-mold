/* =============================================================================
 * host_main.c  --  PC-port process entry: SDL window + frame driver
 * =============================================================================
 * On PSX the game's frame loop is entered at the asm `main` (@0x800828B0). On
 * PC the host C runtime entry is THIS main(): it brings up the SDL window/GL
 * context, then hands control to the game boot spine via port_game_main().
 *
 * Phase 0: port_game_main() is a weak stub (port_runtime.c) that aborts on the
 * first HAL call, proving the ABI/toolchain/link path. Phase 2 replaces it with
 * the functional-C boot loop (the converted PSX `main`).
 *
 * SDL is optional at compile time so the CP-0.6 "-m32 -c clean" gate holds even
 * before the flake ships SDL2 (CP-0.7). With SDL present a real window opens;
 * without it, main() still links and drives the boot (and aborts on first stub).
 * ========================================================================== */
#include <stdio.h>

#include "port_runtime.h"
#include "port_hal.h"

/* SDL is used ONLY when CMake found the (32-bit) SDL2 library AND added it to
 * the link (it defines PORT_HAVE_SDL2 in that case). We deliberately do NOT
 * probe __has_include here: the SDL2 headers may be present in the environment
 * while the 32-bit library is not linked, which would compile SDL calls that
 * then fail at link. Gating on the CMake flag keeps compile and link in sync.
 * Without SDL the port still links and drives the boot (aborting on the first
 * unimplemented HAL stub) -- enough for the Phase-0 link gate. */
#if defined(PORT_HAVE_SDL2)
#  define HOST_USE_SDL 1
#  include <SDL2/SDL.h>
#else
#  define HOST_USE_SDL 0
#endif

/* PSX PAL framebuffer is 320x240 (or 256x240); present at an integer scale. */
#define HOST_W 320
#define HOST_H 240
#define HOST_SCALE 2

/* The game boot spine. Weak stub in port_runtime.c (Phase 0); strong override
 * in port_boot.c (Phase 1 HAL bring-up harness) / the converted PSX main
 * (Phase 2). host_main() brings up the window, then hands off here. */
extern void port_game_main(void);

#if HOST_USE_SDL
static SDL_Window *g_window;
static SDL_GLContext g_glctx;

static int host_sdl_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        port_log("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    g_window = SDL_CreateWindow(
        "Skullmonkeys (PC port)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        HOST_W * HOST_SCALE, HOST_H * HOST_SCALE,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!g_window) {
        port_log("SDL_CreateWindow failed: %s", SDL_GetError());
        return -1;
    }
    g_glctx = SDL_GL_CreateContext(g_window);
    if (!g_glctx) {
        port_log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        return -1;
    }
    SDL_GL_SetSwapInterval(1);   /* vsync; the boot loop also paces to 50 Hz */
    /* Hand the window to the GPU backend so it can present (SwapWindow). */
    port_gpu_set_window(g_window);
    port_log("SDL window + GL2.1 context up (%dx%d @%dx).", HOST_W, HOST_H, HOST_SCALE);
    return 0;
}

static void host_sdl_shutdown(void) {
    if (g_glctx) SDL_GL_DeleteContext(g_glctx);
    if (g_window) SDL_DestroyWindow(g_window);
    SDL_Quit();
}
#endif /* HOST_USE_SDL */

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    port_log("Skullmonkeys PC port starting.");

#if HOST_USE_SDL
    if (host_sdl_init() != 0) {
        return 1;
    }
#else
    port_log("(built without SDL2 -- no window this build)");
    port_gpu_set_window(0);
#endif

    /* Hand off to the game boot / HAL bring-up loop. */
    port_game_main();

#if HOST_USE_SDL
    host_sdl_shutdown();
#endif
    return 0;
}

