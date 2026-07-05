/* =============================================================================
 * InitSPUDefaults.c  --  PC-port SPU defaults (TARGET_PC)
 * =============================================================================
 * PC replacement for asm/nonmatchings/sound/InitSPUDefaults.s (sound.c). The
 * original programs PSX SPU hardware defaults (SpuSetCommonAttr master volume,
 * SsUtReverbOff). The PC SPU backend (port/spec/spu_sdl.c) models the SPU as
 * accepted per-voice state and does not need these hardware pokes, so this is a
 * no-op. Real audio config co-develops with SPU-ADPCM mixing in Phase 3.
 * See docs/plans/pc-port.md CP-1.4 / CP-2.2.
 * ========================================================================== */
#include "common.h"

void InitSPUDefaults(void) {
    /* SPU hardware defaults are subsumed by the spu_sdl.c state model. */
}
