#ifndef ASSET_IDS_H
#define ASSET_IDS_H

/* =============================================================================
 * Asset IDs (calcHash names) - Skullmonkeys (PAL SLES-01090)
 *
 * 32-bit asset ids are ToolX build-time hashes of the asset name, using the
 * Neverhood `calcHash` (cumulative single-bit-toggle). The id namespace is:
 *     id = SEED ^ rotl(calcHash(name), R)
 * with per-role prefix keys: RAW(0,0) audio+gameplay, TEXT(0x28C0E011,27) menu
 * glyphs, PICKUP(0x88200080,27) power-ups, BOSS(0x08280150,27) boss sprites.
 *
 * CLEAN-ROOM: every name below is a reconstructed RE label, not an original
 * string. "verified" here = cracked + cross-checked (rendered text, ScummVM
 * xref, sister-math, or a runtime data table); it is NOT externally proven
 * except the 3 ScummVM-anchored Klaymen assets. The hash/id is certain; the
 * exact human spelling (separators, zero-padding) is best-effort.
 *
 * These ids are consumed in the engine as opaque u32 immediates; the consuming
 * functions are still in asm/, so substituting these constants is documentation
 * only and does not affect codegen. Source of truth + status:
 *   docs/analysis/asset-identification/cracked_names.csv
 *   docs/reference/asset-hash-ids.md
 *   docs/analysis/asset-identification/footstep-remap-table.md
 * ============================================================================= */

/* ---- Audio (Asset 601), RAW namespace: id = calcHash("FX_..._...") ---------- */
#define FX_AMBIENT_BLIZZARD                0x80a0c240u
#define FX_AMBIENT_BOILER                  0x9980d651u
#define FX_AMBIENT_CAVERN                  0x9180d446u
#define FX_AMBIENT_CHAINS                  0x90985240u
#define FX_AMBIENT_CRYSTAL                 0x10904443u
#define FX_AMBIENT_DRIP_1                  0xb0807444u
#define FX_AMBIENT_DRIP_2                  0xb0b07444u
#define FX_AMBIENT_DRIP_3                  0xb0d07444u
#define FX_AMBIENT_DRIP_4                  0xb0107444u
#define FX_AMBIENT_DRIP_5                  0xb1907444u
#define FX_AMBIENT_DRIP_6                  0xb2907444u
#define FX_AMBIENT_FOUNTAIN                0x90d85954u
#define FX_AMBIENT_LIGHTNING               0x9cc87050u
#define FX_AMBIENT_RIVER                   0x9291d400u
#define FX_AMBIENT_TORCH                   0xd0cc5442u
#define FX_AMBIENT_WATERFALL_LARGE         0x10f0e983u
#define FX_AMBIENT_WATERFALL_SMALL         0x92f8f1c0u
#define FX_AMBIENT_WIND                    0xd0a14440u
#define FX_BIRD_FLY_01                     0xc8830661u
#define FX_BIRD_SQUISH_01                  0xd914004fu
#define FX_BOSS_HEAD_ATTACK_01             0xf03391e9u
#define FX_BOSS_HEAD_ATTACK_02             0xf033a1e9u
#define FX_BOSS_HEAD_HIT                   0x612010c9u
#define FX_BOSS_HEAD_IDLE_01               0x603588e8u
#define FX_BOSS_HEAD_TURN                  0x682080cdu
#define FX_BOSS_HEAD_WALK                  0xa06088c9u
#define FX_BOSS_KLOGG_EXPLOSION            0x5862f614u
#define FX_BOSS_KLOGG_FIRE_1               0x49609640u
#define FX_BOSS_KLOGG_FIRE_2               0x49a09640u
#define FX_BOSS_KLOGG_FIRE_3               0x48209640u
#define FX_BOSS_KLOGG_HIT                  0x4c22d240u
#define FX_BOSS_KLOGG_HIT_1                0x4c02d240u
#define FX_BOSS_KLOGG_HIT_2                0x4c62d240u
#define FX_BOSS_KLOGG_HIT_3                0x4ca2d240u
#define FX_BOSS_KLOGG_IDLE                 0x486492c4u
#define FX_BOSS_MEGA_DIE                   0x502840c3u
#define FX_BOSS_MEGA_IDLE                  0x512061c1u
#define FX_BOSS_MEGA_ROLL                  0x502141c3u
#define FX_BOSS_MEGA_SPIT                  0x4824c0c5u
#define FX_BOSS_MEGA_TURN                  0x402044c9u
#define FX_BOSS_WIZARD_ATTACK              0x08089081u
#define FX_BOSS_WIZARD_BEAM                0x48017101u
#define FX_BOSS_WIZARD_HIT                 0x40815801u
#define FX_BOSS_WIZARD_IDLE                0xc0099011u
#define FX_BOSS_WIZARD_TAUNT_1             0x0e041001u
#define FX_BOSS_WIZARD_TAUNT_2             0x08041001u
#define FX_BOSS_WIZARD_TAUNT_3             0x04041001u
#define FX_BUTTON_POWERUP                  0x760106c1u
#define FX_EXPLODE_NORMAL                  0x29808408u
#define FX_GUM_PIERCE_DN                   0xc00200c9u
#define FX_KLAY_DIE_EXPLODE                0x146ce002u
#define FX_KLAY_DIE_FALL                   0x507883c3u
#define FX_KLAY_DUCK_DOWN                  0x4428c941u
#define FX_KLAY_FART                       0x50008340u
#define FX_KLAY_FOOTSTEP_LEFT_ECHO         0x001822d8u
#define FX_KLAY_FOOTSTEP_LEFT_METAL        0x486002dbu
#define FX_KLAY_FOOTSTEP_LEFT_NORMAL       0x70d006d8u
#define FX_KLAY_FOOTSTEP_LEFT_SOFT         0x401106dau
#define FX_KLAY_FOOTSTEP_LEFT_WOOD         0x40400270u
#define FX_KLAY_FOOTSTEP_QUIET             0x44d4c8d8u
#define FX_KLAY_FOOTSTEP_QUIET_ECHO        0x4cddccd8u
#define FX_KLAY_FOOTSTEP_QUIET_METAL       0x25d2c8d8u
#define FX_KLAY_FOOTSTEP_QUIET_SOFT        0x04dce858u
#define FX_KLAY_FOOTSTEP_QUIET_WOOD        0x44d6c8cdu
#define FX_KLAY_FOOTSTEP_RIGHT_ECHO        0x0462e0bbu
#define FX_KLAY_FOOTSTEP_RIGHT_METAL       0x0478a37au
#define FX_KLAY_FOOTSTEP_RIGHT_NORMAL      0x246166fau
#define FX_KLAY_FOOTSTEP_RIGHT_SOFT        0x2470e0f2u
#define FX_KLAY_FOOTSTEP_RIGHT_WOOD        0x0120e27au
#define FX_KLAY_HIT_HEAD                   0x50f08207u
#define FX_KLAY_HURT                       0x00e49240u
#define FX_KLAY_IDLE_SCREAM                0x5d40a249u
#define FX_KLAY_JUMP                       0x4a60ca40u
#define FX_KLAY_LAND                       0x5860c640u
#define FX_KLAY_LAND_ECHO                  0x1828e640u
#define FX_KLAY_LAND_METAL                 0x5050c643u
#define FX_KLAY_LAND_SOFT                  0x5821c242u
#define FX_KLAY_LAND_WOOD                  0x5870c6e8u
#define FX_KLAY_RUN_FAST                   0x00248e52u
#define FX_KLAY_RUN_FAST_ECHO              0x04a68e56u
#define FX_KLAY_RUN_FAST_METAL             0x83248e62u
#define FX_KLAY_RUN_FAST_SOFT              0x0434ce72u
#define FX_KLAY_RUN_FAST_WOOD              0x012484d2u
#define FX_KLAY_SHOOT_NORMAL               0x53608b64u
#define FX_KLAY_UNIVERSE_ENEMA             0x412e8311u
#define FX_MENU_PASSWORD                   0x00880c1eu
#define FX_MENU_SELECT                     0x90810000u
#define FX_MENU_SKULL_SCREAM               0x421d1000u
#define FX_MENU_WILLIE_EXIT                0x42590440u
#define FX_OBJECT_CHECKPOINT               0x2b2ca36bu
#define FX_OBJECT_ELEVATOR                 0x06e0a824u
#define FX_OBJECT_GLIDEY_BIRD              0x563ba142u
#define FX_OBJECT_HAMSTER                  0x22a4aa42u
#define FX_OBJECT_PHOENIX_FLY              0x1b610064u
#define FX_OBJECT_SMOKE                    0x02102152u
#define FX_OBJECT_SPLASH                   0x0220a173u
#define FX_OBJECT_WILLIE                   0x0a409040u
#define FX_PICKUP_1970                     0x408a6461u
#define FX_PICKUP_1970_03                  0x428a6465u
#define FX_PICKUP_FARTHEAD                 0xc0034658u
#define FX_PICKUP_GLIDEY                   0x6082c120u
#define FX_PICKUP_GROW                     0x40864668u
#define FX_PICKUP_OBJECT                   0x4a806042u
#define FX_PICKUP_ONE_UP                   0x428254e2u
#define FX_PICKUP_PHOENIX                  0x44c26454u
#define FX_PICKUP_SHIELD                   0xe0880448u
#define FX_PICKUP_SUPER_WILLIE             0xe48744c4u
#define FX_PICKUP_UNIVERSE_ENEMA           0xc88a346au
#define FX_PUFF_FALL_3                     0x40e0824cu
#define FX_RAT_DASH_END                    0x71030240u
#define FX_SKULL_BITE_01                   0x74182b40u
#define FX_SKULL_EAT                       0x7000c248u
#define FX_SKULL_FIRE_01                   0x61200640u
#define FX_SKULL_FIRE_02                   0x51200640u
#define FX_SKULL_FLAP                      0x68009240u
#define FX_SKULL_FLY_01                    0x7c108242u
#define FX_SKULL_FLY_02                    0x7c108244u
#define FX_SKULL_FOOTSTEP_LEFT             0x1000c042u
#define FX_SKULL_FOOTSTEP_RIGHT            0x991003c2u
#define FX_SKULL_GRUNT_01                  0xf2810224u
#define FX_SKULL_LAND_01                   0x70404350u
#define FX_SKULL_LAND_02                   0x70204350u
#define FX_SKULL_SCREAM_01                 0xe0c00650u
#define FX_SKULL_SPRING_01                 0x302010c4u
#define FX_SKULL_SPRING_02                 0x302016c4u
#define FX_SKULL_UP                        0x30004240u
#define KLAYMEN_IDLEHEAD_SOUND             0x9d406340u

/* Role-named audio (ids verified, exact asset names still uncracked).
 * FX_BUTTON_UNPAUSE: trigger-context cracked in SESSION10 usage notes
 * (PauseGameWithFadeOut). FX_MENU_CYCLE: only call sites are the four
 * menu cycle up/down handlers in src/menu.c. */
#define FX_BUTTON_UNPAUSE                  0x4c60f249u  /* PauseGameWithFadeOut */
#define FX_MENU_CYCLE                      0x686c1c97u  /* level/option cycle */

/* ---- Sprites (Asset 600). Menu glyphs are TEXT namespace. ------------------- */
#define SPR_PREFIX_FARTHEAD                0x8c510186u  /* <PREFIX>FARTHEAD */
#define SPR_PREFIX_GROW                    0x8c30008cu  /* <PREFIX>GROW */
#define SPR_PREFIX_HAMSTER                 0xa9228088u  /* <PREFIX>HAMSTER */
#define SPR_PREFIX_ONE_UP                  0xa9240484u  /* <PREFIX>ONE_UP */
#define SPR_PREFIX_UNIVERSE_ENEMA_1        0x6a351094u  /* <PREFIX>UNIVERSE_ENEMA_1 */
#define SPR_PREFIX_WILLIE                  0x902c0002u  /* <PREFIX>WILLIE */
#define SPR_CONTINUE                       0x69c04050u  /* CONTINUE */
#define FX_PLAYER_SWIM_PRE                 0x46346040u  /* FX_PLAYER_SWIM_PRE */
#define SPR_KLAYMEN_IDLEBLINK_ANIM         0x5900c41eu  /* Klaymen_idleblink_anim */
#define SPR_NO                             0x29c0e211u  /* NO */
#define SPR_PAUSED                         0x0ad0f813u  /* PAUSED */
#define SPR_QUIT                           0x68c0f413u  /* QUIT */
#define SPR_QUIT_GAME                      0x69c8f473u  /* QUIT GAME */
#define SPR_YES                            0x2ad0f011u  /* YES */

/* Role-named MENU sprites (ids verified, exact asset names still uncracked). */
#define SPR_MENU_BUTTON_NAV                0x10094096u  /* universal menu/back button */
#define SPR_MENU_HIGHLIGHT_IDLE            0x39900619u  /* button highlight idle */
#define SPR_MENU_HIGHLIGHT_IDLE_ALT        0x33808e1bu  /* alternate highlight idle */
#define SPR_MENU_HIGHLIGHT_ACTIVE          0x63848e59u  /* button highlight flash/active */
#define SPR_MENU_SKULL_ICON                0x81100030u  /* skull/options icon */

/* Role-named gameplay sprites consumed by the EntityType*_Init dispatch
 * table in src/entinit.c. Ids are verified asset hashes (status=uncracked
 * in cracked_names.csv); names are working RE labels grouped by spawning
 * function. */
#define SPR_PLATFORM_BALL_A                0x98f8221eu  /* Type 001/049/057 generic platform/clayball */
#define SPR_PLATFORM_BALL_B                0x88783718u  /* Type 005/050/058 generic platform/clayball */
#define SPR_PLATFORM_BALL_C                0x8818a018u  /* Type 006/051/059 generic platform/clayball */
#define SPR_PLATFORM_VARIANT_D             0x9299c307u  /* Type 012 moving platform variant C */
#define SPR_ENEMY_CLUSTER_A                0x93c9a20fu  /* Type 017 enemy-cluster sprite */
#define SPR_ENEMY_CLUSTER_B                0x9ab9a209u  /* Type 018 enemy-cluster sibling */
#define SPR_CLOCK_PLATFORM_A               0x93043811u  /* Type 019 clock-platform variant A */
#define SPR_CLOCK_PLATFORM_B               0xd2801814u  /* Type 020 clock-platform variant B */
#define SPR_CLOCK_PLATFORM_C               0x12800031u  /* Type 021 clock-platform variant C */
#define SPR_CLAYBALL_DECOR_A               0x88a16190u  /* Type 062 clayball decor variant A */
#define SPR_CLAYBALL_DECOR_B               0xdcb92390u  /* Type 063 clayball decor variant B */
#define SPR_PARTICLE_DECOR                 0x80b92212u  /* Type 064 particle/passive decor */
#define SPR_CLAYBALL_PARALLAX_A            0xb69c8356u  /* Type 066 clayball parallax variant */
#define SPR_CLAYBALL_PARALLAX_B            0xb7cce25eu  /* Type 067 clayball parallax variant */
#define SPR_CLAYBALL_PARALLAX_C            0xbebce258u  /* Type 068 clayball parallax variant */
#define SPR_SWITCH_CLAYBALL_A              0xd89c319au  /* Type 090 switch-block clayball A */
#define SPR_SWITCH_CLAYBALL_B              0xc89e9158u  /* Type 091 switch-block clayball B */
#define SPR_SWITCH_CLAYBALL_C              0xc48c7158u  /* Type 092 switch-block clayball C */
#define SPR_BONUS_CLAYBALL                 0x10882010u  /* Type 093 bonus/score clayball */
#define SPR_LEVEL_FLAG2_EXTRA              0x168254b5u  /* level-flag-2 extra sprite (LoadLevelSpriteAssets) */

/* ---- Boss sprite roster, BOSS namespace (id = 0x08280150 ^ rotl(calcHash, 27))
 * Substituting these constants is documentation only — the consuming
 * functions live in asm/ and use the bare u32 literals. The grouping mirrors
 * the FSM in src/bosses.c (Shriney Guard is fully documented; the other
 * bosses' sprite slots are recorded as RE labels). */

/* SHRINEY GUARD (MEGA level 5) — see src/bosses.c "SHRINEY GUARD" section.   */
#define SPR_SHRINEY_GUARD_BASE             0xa8482860u  /* InitEntitySprite hash (0xA2 x 0xF0) */
#define SPR_SHRINEY_GUARD_IDLE             0x09382152u  /* passive idle pose                  */
#define SPR_SHRINEY_GUARD_WINDUP           0x4c106054u  /* attack-windup (bounce target)      */
#define SPR_SHRINEY_GUARD_LOOP_LINK        0x40106054u  /* first frame of looping-attack chain*/
#define SPR_SHRINEY_GUARD_LOOP_START       0x2c182010u  /* looping-attack stun-window pose    */
#define SPR_SHRINEY_GUARD_READY            0x08192250u  /* re-aim pose between slams          */
#define SPR_SHRINEY_GUARD_SLAM             0x085860d4u  /* slam animation (MoveCallback runs) */
#define SPR_SHRINEY_GUARD_DEATH            0x0a1820d4u  /* death pose                         */
/* Animation hashes used by Shriney Guard via Set{AnimationLoopFrame,Sprite Callback}. */
#define ANIM_SHRINEY_GUARD_SLAM_KEYFRAME   0x01084280u  /* slam keyframe (triggers MoveCallback install) */
#define ANIM_SHRINEY_GUARD_LOOP_KEYFRAME   0xc00200c9u  /* looping-attack second-loop frame   */
#define ANIM_SHRINEY_GUARD_FINISHED_CB     0x02421405u  /* anim-finished callback hash        */

/* GLENN YNTIS (GLEN level 2 mini-boss) — shares boss vtable family with Shriney. */
#define SPR_GLENN_YNTIS_ANIM_A             0x2d688254u  /* GlennYntisIdleAnimState (boss frame A)*/
#define SPR_GLENN_YNTIS_ANIM_B             0x2b79835du  /* GlennYntisAnimStateB (boss frame B)   */
#define SPR_GLENN_YNTIS_ANIM_C             0x69588258u  /* GlennYntisAnimStateC (boss frame C)   */

/* ---- Footstep surface variants referenced by the remap table @0x8009d0fc ----
 * modes 5/6 (ELECTRIC/SQUISH) are referenced by the table but absent from the
 * shipped asset-id list (cut/unused). Names cracked 5/5 per row; ids are real
 * calcHash outputs. See docs/analysis/asset-identification/footstep-remap-table.md */
#define FX_KLAY_FOOTSTEP_LEFT_ELECTRIC     0xca182248u  /* unshipped (mode 5) */
#define FX_KLAY_FOOTSTEP_RIGHT_ELECTRIC    0x00e4b0bbu  /* unshipped */
#define FX_KLAY_FOOTSTEP_QUIET_ELECTRIC    0x559dcccau  /* unshipped */
#define FX_KLAY_RUN_FAST_ELECTRIC          0xa4a6875au  /* unshipped */
#define FX_KLAY_LAND_ELECTRIC              0xd228e6d0u  /* unshipped */
#define FX_KLAY_FOOTSTEP_LEFT_SQUISH       0x40550a52u  /* unshipped (mode 6) */
#define FX_KLAY_FOOTSTEP_RIGHT_SQUISH      0x4030e2d2u  /* unshipped */
#define FX_KLAY_FOOTSTEP_QUIET_SQUISH      0x04d469c9u  /* unshipped */
#define FX_KLAY_RUN_FAST_SQUISH            0x007406f2u  /* unshipped */
#define FX_KLAY_LAND_SQUISH                0x5865cecau  /* unshipped */

#endif /* ASSET_IDS_H */
