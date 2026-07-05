/* =============================================================================
 * sound_stub.c  --  PC-port SPU upload no-ops (TARGET_PC)
 * =============================================================================
 * UploadAudioToSPU (asm/nonmatchings/<seg>/UploadAudioToSPU.s, 164 lines) DMAs
 * level ADPCM audio into PSX SPU RAM and maintains the sample-address
 * bookkeeping tables (D_8009CC60 / D_8009CFA8..). The PC SPU backend
 * (port/spec/spu_sdl.c) has no SPU RAM and models audio as accepted per-voice
 * state, so the upload is a NO-OP here. The sample tables stay weak-zeroed, so
 * downstream sound triggers read zero addresses -> silence, the correct
 * "no audio yet" behaviour. Real SPU-ADPCM sample management co-develops with
 * audio mixing in Phase 3. See docs/plans/pc-port.md CP-1.4 / CP-2.2.
 * ========================================================================== */
#include "common.h"

void UploadAudioToSPU(s32 *header, void *data, s32 size, s16 flags) {
    (void)header; (void)data; (void)size; (void)flags;
}

/* PlayCDAudioTrack (asm/nonmatchings/gamecd, CD-DA Red-Book playback) starts a
 * background-music track via the CD drive. On PSX it early-returns when the
 * CD_AUDIO_ENABLED flag (D_800A59E8) is 0; that flag is weak-zeroed on PC, so
 * the faithful result here is "return 0, no track started". Real CD-DA / streamed
 * music is a Phase-3 audio concern (the PC build would source music from files,
 * not the disc's Red-Book tracks). */
u32 PlayCDAudioTrack(u8 track, u8 channel) {
    (void)track; (void)channel;
    return 0;
}
