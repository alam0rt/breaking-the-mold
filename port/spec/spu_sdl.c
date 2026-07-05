/* =============================================================================
 * spu_sdl.c  --  PC-port SPU replacement: PSX SPU -> SDL audio (TARGET_PC)
 * =============================================================================
 * Replaces the PlayStation SPU. This slice provides the full voice-control
 * surface the game calls (SpuSetKey / SpuSetVoicePitch / SpuGetVoicePitch /
 * SpuSetVoiceVolume / SpuSetCommonCDVolume / SpuQuit / SsUtReverbOn) as state
 * the mixer will consume, plus init/shutdown hooks. ADPCM voice decode + mix to
 * an SDL_AudioStream lands in a following slice, driven by the game actually
 * keying voices on once Phase 2 boots the sound system. For now the surface is
 * accepted (no aborts) and the output is silence.
 *
 * The matched PSY-Q libvoice.c is excluded from the port build, so these are the
 * strong definitions; the weak aborting stubs in hal_stubs.c are shadowed.
 * See docs/plans/pc-port.md CP-1.4.
 * ========================================================================== */
#include "psyq_pc.h"
#include "port_runtime.h"
#include "port_hal.h"

#define SPU_NUM_VOICES 24

/* Mirror of the per-voice control state the game programs. The mixer (next
 * slice) reads this; keying a voice on latches its current pitch/volume. */
typedef struct {
    unsigned short pitch;
    short          vol_l;
    short          vol_r;
    unsigned char  keyed;
} PortVoice;

static PortVoice s_voice[SPU_NUM_VOICES];
static short     s_cd_vol_l, s_cd_vol_r;

int port_spu_init(void) {
    int i;
    for (i = 0; i < SPU_NUM_VOICES; i++) {
        s_voice[i].pitch = 0;
        s_voice[i].vol_l = 0;
        s_voice[i].vol_r = 0;
        s_voice[i].keyed = 0;
    }
    /* SDL audio device opening deferred to the mix slice; nothing to fail. */
    return 0;
}

void port_spu_shutdown(void) {
}

/* ---- PSY-Q voice-control surface (matches src/sound.c externs) ----------- *
 * SpuSetKey(on_off, voice_bits): key voices on (1) or off (0) by bitmask. */
void SpuSetKey(int on_off, int voice_bits) {
    int v;
    for (v = 0; v < SPU_NUM_VOICES; v++) {
        if (voice_bits & (1 << v)) {
            s_voice[v].keyed = (unsigned char)(on_off ? 1 : 0);
        }
    }
}

void SpuSetVoicePitch(int voice, unsigned short pitch) {
    if (voice >= 0 && voice < SPU_NUM_VOICES) {
        s_voice[voice].pitch = pitch;
    }
}

void SpuGetVoicePitch(int voice, unsigned short *out) {
    if (out && voice >= 0 && voice < SPU_NUM_VOICES) {
        *out = s_voice[voice].pitch;
    }
}

void SpuSetVoiceVolume(int voice, short l, short r) {
    if (voice >= 0 && voice < SPU_NUM_VOICES) {
        s_voice[voice].vol_l = l;
        s_voice[voice].vol_r = r;
    }
}

void SpuSetCommonCDVolume(short l, short r) {
    s_cd_vol_l = l;
    s_cd_vol_r = r;
}

void SpuQuit(void) {
    port_spu_shutdown();
}

void SsUtReverbOn(void) {
}

