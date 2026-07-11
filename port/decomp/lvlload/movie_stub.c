/* =============================================================================
 * movie_stub.c  --  PC-port movie / loading-screen no-ops (TARGET_PC)
 * =============================================================================
 * The PSX intro/level playback sequence plays STR videos and renders loading
 * screens (DisplayLoadingScreen, PlayMovieFromCD, PlayMovieFromBLBSectors --
 * 181/289/280-line asm subsystems that decode MDEC video and stream from the
 * disc). For the PC bring-up these are NO-OP'd so the playback state machine in
 * InitializeAndLoadLevel advances straight to the level-load terminal without a
 * video decoder. Faithful STR/MDEC playback is a Phase-3 nicety.
 *
 * Return semantics (matched to InitializeAndLoadLevel's use):
 *   DisplayLoadingScreen -> NON-ZERO ("loading screen still active this frame").
 *     In InitializeAndLoadLevel the case-4/5 branch computes
 *     s7 = (DisplayLoadingScreen(...) & 0xFF) == 0, and only when s7 != 0 runs the
 *     "loading finished -> re-seek to respawn level + s6 = PLAYER_STATE[stage]"
 *     transition block. On a fresh boot the respawn PlayerState is all-zero
 *     (.bss), so that block would clobber s6 (the 1-indexed stage) to 0 and the
 *     tail LoadAssetContainer would read the level entry's padding (sector 0,
 *     count 0) instead of stage-0's real secondary container. Returning non-zero
 *     keeps the loading screen "active", the transition block is skipped, s6
 *     stays at its initial 1, and the MENU/level loads stage 0 correctly. This
 *     matches real HW, where the screen is still displaying on the frame the
 *     port collapses the whole load into. Faithful STR/MDEC playback + a real
 *     multi-frame loading screen is a Phase-3 nicety.
 *   PlayMovieFrom* -> 0  ("not playing / finished") so the sequence advances.
 * See docs/plans/pc-port.md CP-2.2.
 * ========================================================================== */
#include "common.h"

s8 DisplayLoadingScreen(s32 sectorOff, s32 sectorCnt, void *buf, s32 minTime, s32 maxTime) {
    extern char *getenv(const char *);
    (void)sectorOff; (void)sectorCnt; (void)buf; (void)minTime; (void)maxTime;
    /* PORT_LEVEL set: report "finished" so InitializeAndLoadLevel runs the
     * respawn-seek transition (SeekToLevelInSequence(RESPAWN_PLAYER_STATE[0]),
     * stage s6 = RESPAWN_PLAYER_STATE[1]) that game_boot seeded from the env.
     * Without it the boot stays at the sequence start (the MENU level). */
    if (getenv("PORT_LEVEL")) {
        return 0;
    }
    return 1;   /* loading screen still active -> skip the respawn-seek transition */
}

s8 PlayMovieFromCD(s32 filename, s32 frameField, s32 sectorCnt, void *buf) {
    (void)filename; (void)frameField; (void)sectorCnt; (void)buf;
    return 0;   /* not playing */
}

s8 PlayMovieFromBLBSectors(s32 sectorOff, s32 sectorCnt, void *buf) {
    (void)sectorOff; (void)sectorCnt; (void)buf;
    return 0;   /* not playing */
}
