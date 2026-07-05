/* =============================================================================
 * bios.c  --  PC-port BIOS / libapi / libetc replacement (TARGET_PC)
 * =============================================================================
 * Replaces the PlayStation kernel services the game relies on: critical
 * sections, event delivery, the interrupt/DMA/reset/vsync callback registry,
 * and VSync frame pacing. On PSX these are BIOS syscalls; here they are plain
 * host code. See docs/plans/pc-port.md CP-1.6.
 *
 * The matched PSY-Q lib C (src/libs/*.c) is excluded from the port build, so
 * this file provides the strong definitions for the whole BIOS surface. The
 * weak aborting stubs in hal_stubs.c are shadowed automatically.
 * ========================================================================== */
#include <stdint.h>

#include "psyq_pc.h"
#include "port_runtime.h"
#include "port_hal.h"

/* ---- critical sections --------------------------------------------------- *
 * The game brackets pad/CD/callback setup with Enter/ExitCriticalSection to
 * mask interrupts. The port is single-threaded and cooperative, so these only
 * need to be well-behaved no-ops. A depth counter mirrors the PSX return value
 * (EnterCriticalSection returns 1 if it actually masked, 0 if already masked). */
static int s_critical_depth;

int EnterCriticalSection(void) {
    return (s_critical_depth++ == 0) ? 1 : 0;
}

void ExitCriticalSection(void) {
    if (s_critical_depth > 0) {
        s_critical_depth--;
    }
}

/* ---- event delivery ------------------------------------------------------ *
 * DeliverEvent(class, spec) signals a kernel event. The game's CD/stream code
 * posts completion events (0xF0000003 / 0x0020|0x0040). With the synchronous
 * host CD backend (cd_files.c) there is nothing to wake, so this is a no-op
 * that simply records the last event for debugging. */
static u_long s_last_event_class;
static u_long s_last_event_spec;

long DeliverEvent(u_long ev_class, u_long spec) {
    s_last_event_class = ev_class;
    s_last_event_spec = spec;
    return 0;
}

/* ---- callback registry --------------------------------------------------- *
 * The PSX registers handlers via *Callback(func); each returns the previous
 * handler. We keep the pointers so the frame loop can fire the VSync handler,
 * and so CD data-ready callbacks can be invoked by cd_files.c if needed. */
static PortCallback s_vsync_cb;
static PortCallback s_interrupt_cb;
static PortCallback s_dma_cb[8];
static PortCallback s_reset_cb;

void *VSyncCallback(void (*f)(void)) {
    void *old = (void *)s_vsync_cb;
    s_vsync_cb = (PortCallback)f;
    return old;
}

void *InterruptCallback(int irq, void (*f)(void)) {
    void *old = (void *)s_interrupt_cb;
    (void)irq;
    s_interrupt_cb = (PortCallback)f;
    return old;
}

void *DMACallback(int dma, void (*f)(void)) {
    void *old;
    if (dma < 0 || dma >= (int)(sizeof(s_dma_cb) / sizeof(s_dma_cb[0]))) {
        return 0;
    }
    old = (void *)s_dma_cb[dma];
    s_dma_cb[dma] = (PortCallback)f;
    return old;
}

void *ResetCallback(void) {
    void *old = (void *)s_reset_cb;
    return old;
}

/* Fire the registered VSync handler (called once per presented frame by the
 * host frame loop in port_boot.c). */
void port_bios_fire_vsync(void) {
    if (s_vsync_cb) {
        s_vsync_cb();
    }
}

/* ---- VSync frame counter / pacing --------------------------------------- *
 * VSync(mode):
 *   mode == 0  : wait for the next vblank
 *   mode  > 0  : wait until `mode` vblanks have elapsed since the last call
 *   mode == -1 : return the total vblank count (no wait)
 *   mode == -2 : return the raw hsync/timer value (approximated by the counter)
 * The real vblank cadence is driven by the host frame loop advancing the
 * counter (port_bios_advance_frame); here we just report/spin cooperatively. */
static volatile unsigned s_vblank_count;

int VSync(int mode) {
    if (mode <= 0) {
        /* Report the counter; the host loop owns the actual frame wait so we
         * do not busy-block here (that would stall the single thread). */
        return (int)s_vblank_count;
    }
    return (int)s_vblank_count;
}

void port_bios_advance_frame(void) {
    s_vblank_count++;
}

unsigned port_bios_vblank_count(void) {
    return s_vblank_count;
}
