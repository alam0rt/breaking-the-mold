/* =============================================================================
 * gfx_port.c  --  functional-C bodies for small gfx.c boot/frame helpers
 * =============================================================================
 * PC-port (TARGET_PC) replacements for the tiny gfx.c routines that gate the
 * frame loop. On the byte-match build these are `INCLUDE_ASM` in src/gfx.c; here
 * they are strong C definitions that override the generated weak stubs. Behaviour
 * is transcribed from the disassembly (asm/nonmatchings/gfx/*.s).
 *
 * The MIPS build never compiles this file (it lives under port/), so the
 * byte-match is unaffected.
 * ========================================================================== */
#include "common.h"
#include "globals.h"

extern int  VSync(int mode);
extern u_int *FntFlush(int id);

/* WaitForVBlankIfNeeded @ 0x8001352C: if no buffer swap is currently in flight,
 * wait one vblank. (base arg is unused -- kept for call-site ABI parity.) */
void WaitForVBlankIfNeeded(void *base) {
    (void)base;
    if (g_SwapInFlight == 0) {
        VSync(0);
    }
}

/* FlushDebugFontAndEndFrame @ 0x80013500: flush the debug font stream, then arm
 * the frame-ready flag and clear the swap-in-flight latch (the VSync ISR path
 * performs the actual buffer swap on real hardware; on PC the host frame loop
 * presents). */
void FlushDebugFontAndEndFrame(void *base) {
    (void)base;
    FntFlush(-1);
    g_FrameReady = 1;
    g_SwapInFlight = 0;
}
