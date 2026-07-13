#include "common.h"

extern void ClearOTag(u32 *ot, s32 n);
extern u32 *FntFlush(s32 id);
extern void VSync(s32 mode);

extern u8 D_800907EC[];
extern u8 D_8009AE58[];
extern u8 D_8009B14C[];
extern u8 D_8009B160[];

/* gfx .sdata (0x800A5954..0x800A5980): the shared engine-globals island at the
 * very base of sdata (gp_value points at g_pBlbHeapBase). Migrated from the
 * pooled asm sdata blob (asm/data/96154.sdata.s) to natural C definitions
 * (sdata-under-split): jsonnet maps dotsdata('96154','gfx') ->
 * build/src/gfx.o(.sdata). Defined in address order (cc1 emits initialized
 * .sdata in declaration order); exact names preserved via asm(). Owning these
 * defs in this TU is what lets maspsx emit the %gp_rel accesses the ROM's gfx
 * code uses for g_FrameReady / g_SwapInFlight (extern smalls stay absolute). */
void *g_pBlbHeapBase asm("g_pBlbHeapBase") = D_800907EC;
u16 g_GameFlags asm("g_GameFlags") = 0;
u8 g_FrameReady asm("g_FrameReady") = 0;
u8 g_SwapInFlight asm("g_SwapInFlight") = 0;
u8 *D_800A595C asm("D_800A595C") = D_8009AE58;
void *g_pGameState asm("g_pGameState") = 0;
u8 *D_800A5964 asm("D_800A5964") = D_8009B14C;
u8 *D_800A5968 asm("D_800A5968") = D_8009B160;
u16 D_800A596C asm("D_800A596C") = 0x40;
u16 D_800A596E asm("D_800A596E") = 0x20;
/* 12-byte blob split in two: a 12-byte array exceeds the -G8 small-data
 * threshold and would fall out of .sdata; two <=8-byte halves stay in .sdata
 * and land contiguously (declaration order), preserving the island bytes. */
u16 D_800A5970[4] asm("D_800A5970") = {0x80, 0, 0x4E45, 0x3244};
u16 D_800A5978[2] asm("D_800A5978") = {0, 0};
#ifdef TARGET_PC
/* Raw PSX address is meaningless as a pointer on PC; boot re-points it. */
u8 *D_800A597C asm("D_800A597C") = 0;
#else
u8 *D_800A597C asm("D_800A597C") = (u8 *)0x8009B1D8;
#endif

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

extern void SetVideoMode(s32 mode);
void SsUtReverbOn(void) {
    SetVideoMode(1);
}

INCLUDE_ASM("asm/nonmatchings/gfx", InitGraphicsSystem);

extern void builtin_delete(void *ptr);
void ConditionalFreeMemory(void *ptr, s32 flags) {
    if (flags & 1) {
        builtin_delete(ptr);
    }
}

/* Resets both double-buffers' ordering tables (OT head ptrs at base+0x74 and
 * base+0x50B4) and re-arms the swap latch: frame not ready, swap in flight. */
void ClearOrderingTables(u8 *base) {
    u32 *ot0 = *(u32 **)(base + 0x74); /* named temp: hoists the load above the flag stores */
    g_FrameReady = 0;
    g_SwapInFlight = 1;
    ClearOTag(ot0, 1);
    ClearOTag(*(u32 **)(base + 0x50B4), 1);
}

/* Flushes the debug-font stream, then marks the frame ready for the VSync ISR
 * (GetFrameReadyFlag+0xC -> TriggerBufferSwapIfReady) and clears the
 * swap-in-flight latch. */
void FlushDebugFontAndEndFrame(void) {
    FntFlush(-1);
    g_FrameReady = 1;
    g_SwapInFlight = 0;
}

/* If no buffer swap is currently in flight, blocks for one vblank. */
void WaitForVBlankIfNeeded(void) {
    if (g_SwapInFlight == 0) {
        VSync(0);
    }
}

INCLUDE_ASM("asm/nonmatchings/gfx", SwapBuffersAndClearOT);
