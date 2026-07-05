/* =============================================================================
 * audio_stubs.c  --  PC-port audio no-op stubs (SPU mixing deferred, Phase 3)
 * =============================================================================
 * Sound-effect playback is a Phase-3 work item (spu_sdl.c has the voice-state
 * surface but no ADPCM decode/mix yet). These return the documented failure/
 * neutral values so game logic proceeds silently.
 * ========================================================================== */
#include "common.h"

/* PlaySoundEffect @ 0x8007A5A4 (export 108204): returns the allocated SPU
 * voice index (0-23) or 0xFFFFFFFF on failure. "No free voice" is a normal
 * runtime outcome the callers already handle, so it is the safe no-op. */
unsigned int PlaySoundEffect(unsigned int soundId, short pan, char force) {
    (void)soundId; (void)pan; (void)force;
    return 0xFFFFFFFFu;
}
