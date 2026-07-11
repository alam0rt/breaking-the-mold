/* =============================================================================
 * port_runtime.h  --  PC-port runtime service prototypes (TARGET_PC)
 * ========================================================================== */
#ifndef PORT_RUNTIME_H
#define PORT_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

/* When non-zero, port_stub() logs and returns instead of aborting. */
extern int g_port_stub_nonfatal;

void port_log(const char *fmt, ...);
void port_panic(const char *fmt, ...) __attribute__((noreturn));
void port_stub(const char *fn);

/* Game-boot entry: weak stub in port_runtime.c, strong override in Phase 2. */
void port_game_main(void);

/* Optional per-frame GameState dump for comparing the port against a PS1
 * reference trace (port/spec/port_trace.c). No-op unless PORT_TRACE_* is set. */
void port_trace_frame(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT_RUNTIME_H */
