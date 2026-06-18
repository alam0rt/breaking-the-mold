#ifndef ANIM_FRAME_EVENTS_H
#define ANIM_FRAME_EVENTS_H

/* =============================================================================
 * Animation frame-event ids - Skullmonkeys (PAL SLES-01090)
 *
 * A 3rd use of calcHash (besides asset ids and cheat codes). ToolX tags
 * animation frames with a calcHash event id. On playback reaching a tagged
 * frame the engine calls the entity callback as (entity, event=1, arg=hash)
 * (NM_ANIMATION_START); the handler does `if (arg == 0x<hash>) { ... }` to
 * fire a sound / projectile / hitbox / state change synced to the animation.
 *
 * CLEAN-ROOM: these are observed compare constants; only two are named (the
 * glider on/off discriminators) plus one cross-validated to a known asset.
 * The rest await an emulator-breakpoint capture of their on-screen semantic -
 * see docs/analysis/asset-identification/frame-event-breakpoint-plan.md and
 * scripts/frame_event_tracer.lua. Consuming handlers are in asm/, so these are
 * documentation only (codegen-neutral).
 * ============================================================================= */

/* ---- Confirmed ------------------------------------------------------------- */
#define FE_KLAY_DIE_EXPLODE   0x146CE002u  /* == asset FX_KLAY_DIE_EXPLODE (ScummVM xref); BossCollision_SpawnDebrisAndLayers */
#define FE_GLIDER_ENABLE      0x10D86282u  /* GliderEventHandler: entity[0x10D]=1 */
#define FE_GLIDER_DISABLE     0xB0C10420u  /* GliderEventHandler: entity[0x10D]=0 */

/* ---- Unnamed (handler / hint in comment) ----------------------------------- */
/* AnimFrameParticleHandler @0x8005CCE0 - spawn particle on tagged frame */
#define FE_PARTICLE_1102A434  0x1002A434u
#define FE_PARTICLE_1020AC7D  0x1020AC7Du
#define FE_PARTICLE_11022025  0x11022025u
#define FE_PARTICLE_20833814  0x20833814u
#define FE_PARTICLE_70032814  0x70032814u
/* EntityEventHandlerSpawnMultipleProjectiles @0x80040D74 */
#define FE_SPAWN_PROJ_10022814 0x10022814u
/* DeathAnimSetStateHandler @0x8005E650 - death-anim state cluster (..062824 family = one base name) */
#define FE_DEATH_10062824     0x10062824u
#define FE_DEATH_80062824     0x80062824u
#define FE_DEATH_92062824     0x92062824u
#define FE_DEATH_94062824     0x94062824u
#define FE_DEATH_98062824     0x98062824u
#define FE_DEATH_B0062824     0xB0062824u
#define FE_DEATH_1023281C     0x1023281Cu
#define FE_DEATH_11232C94     0x11232C94u
#define FE_DEATH_12820D34     0x12820D34u
#define FE_DEATH_30160814     0x30160814u
#define FE_DEATH_384D0812     0x384D0812u
#define FE_DEATH_404082C4     0x404082C4u
#define FE_DEATH_89038298     0x89038298u
#define FE_DEATH_8C5E41E4     0x8C5E41E4u
#define FE_DEATH_920E2062     0x920E2062u
#define FE_DEATH_92120854     0x92120854u
#define FE_DEATH_94222814     0x94222814u
#define FE_DEATH_94422814     0x94422814u
#define FE_DEATH_1C0C66F2     0x1C0C66F2u
/* TeleporterAnimHandler @0x8005D0C8 / BounceEventHandler @0x8005D25C */
#define FE_TELEPORT_221017CA  0x221017CAu
#define FE_TELEPORT_382C92C1  0x382C92C1u
#define FE_TELEPORT_2809A510  0x2809A510u
/* misc single-site handlers */
#define FE_WALK_TO_RUN_1210CD03   0x1210CD03u  /* WalkToRunTransition @0x8005F540 */
#define FE_DAMAGE_HITBOX_50900004 0x50900004u  /* CollisionDamageSetup @0x8005F430 */
#define FE_CHECKPOINT_182D840C    0x182D840Cu  /* CheckpointSaveAndSpawnHUD @0x8005DEB0 */
#define FE_BOUNCE_1A800898        0x1A800898u  /* PlayerBounceCallback @0x8005CF24 */
#define FE_PORTAL_4A212247        0x4A212247u  /* TeleporterPortalEventHandler @0x80045410 */
#define FE_ENTMSG_8CAA1108        0x8CAA1108u  /* EntityMessageHandler @0x8004858C */

#endif /* ANIM_FRAME_EVENTS_H */
