#include "common.h"

/*
 * GetFrameReadyFlag @ 0x800131F0 (0x10 bytes).
 * Dual-purpose hand-written stub:
 *   - Called from C: returns the byte g_FrameReady in $v0 (set in the
 *     `jr $ra` delay slot via lbu %gp_rel(g_FrameReady)).
 *   - Address `GetFrameReadyFlag + 0xC` (the lbu instruction itself) is
 *     installed as the VSync interrupt callback by InitGraphicsSystem;
 *     when fired it loads g_FrameReady into $v0 and falls through into
 *     TriggerBufferSwapIfReady to perform the actual buffer swap.
 * Name is partially misleading — it's both a getter and a VSync ISR head.
 */
INCLUDE_ASM("asm/nonmatchings/gfx", GetFrameReadyFlag);

/*
 * TriggerBufferSwapIfReady @ 0x80013200 (0x40 bytes).
 * VSync callback body, entered by fallthrough from GetFrameReadyFlag+0xC
 * with $v0 already loaded with g_FrameReady. If g_FrameReady is set and no
 * swap is currently in flight (g_SwapInFlight == 0), sets g_SwapInFlight=1,
 * clears g_FrameReady, and calls SwapBuffersAndClearOT(g_pBlbHeapBase).
 * Not a normal C-callable function — relies on $v0 from the lbu prologue.
 */
INCLUDE_ASM("asm/nonmatchings/gfx", TriggerBufferSwapIfReady);

/*
 * func_80013240 @ 0x80013240 (4 bytes).
 * Empty stub — a single `jr $ra` with no body. Likely a no-op placeholder
 * used as a default callback slot (purpose unconfirmed).
 */
INCLUDE_ASM("asm/nonmatchings/gfx", func_80013240);

INCLUDE_ASM("asm/nonmatchings/gfx", SetVideoModePAL);

INCLUDE_ASM("asm/nonmatchings/gfx", SsUtReverbOn);

INCLUDE_ASM("asm/nonmatchings/gfx", InitGraphicsSystem);

INCLUDE_ASM("asm/nonmatchings/gfx", ConditionalFreeMemory);

INCLUDE_ASM("asm/nonmatchings/gfx", ClearOrderingTables);

INCLUDE_ASM("asm/nonmatchings/gfx", FlushDebugFontAndEndFrame);

INCLUDE_ASM("asm/nonmatchings/gfx", WaitForVBlankIfNeeded);

INCLUDE_ASM("asm/nonmatchings/gfx", SwapBuffersAndClearOT);
