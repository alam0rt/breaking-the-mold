/* =============================================================================
 * port_runtime.c  --  PC-port runtime services (TARGET_PC)
 * =============================================================================
 * The bottom of the SPEC/HAL stack: logging, panic, and port_stub(). Every
 * still-unimplemented function (generated weak stub in _autostubs.c, or an
 * aborting HAL body in hal_stubs.c) funnels through port_stub(), which prints
 * the offending symbol and aborts. As real bodies land, the funnel narrows.
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "port_runtime.h"

/* Set to 1 to keep running (return a zeroed value) instead of aborting on the
 * first unimplemented call. Useful for surveying how far boot gets. */
int g_port_stub_nonfatal = 0;

/* PORT_STUBS_NONFATAL=1: log-and-continue instead of panicking on stubbed
 * functions. Diagnostic mode -- state may corrupt downstream; never rely on
 * behaviour observed under it. */
static void __attribute__((constructor)) port_stub_mode_init(void) {
    const char *v = getenv("PORT_STUBS_NONFATAL");
    if (v && *v && *v != '0') {
        g_port_stub_nonfatal = 1;
    }
}

void port_log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fputs("[port] ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void port_panic(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fputs("[port] PANIC: ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    abort();
}

void port_stub(const char *fn) {
    if (g_port_stub_nonfatal) {
        static const char *last = 0;
        if (fn != last) {           /* de-dup consecutive spam */
            port_log("unimplemented: %s() [continuing]", fn);
            last = fn;
        }
        return;
    }
    port_panic("unimplemented function reached: %s()", fn);
}

/* Weak game-boot entry. host_main() calls this after bringing up the window.
 * Phase 0: aborts on the first HAL call (no game code wired yet). Phase 2: the
 * functional-C boot loop (converted PSX `main`) provides a strong override. */
__attribute__((weak)) void port_game_main(void) {
    port_stub("port_game_main (game boot not yet wired -- Phase 2)");
}

