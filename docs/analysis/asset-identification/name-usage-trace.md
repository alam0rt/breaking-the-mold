---
title: "Asset-name ↔ code-usage trace (cross-verification + misnomer audit)"
category: analysis/asset-identification
tags: [analysis, asset, identification, trace, verification, misnomer]
---

# Asset-name ↔ code-usage trace

**2026-07-02.** Every cracked name in `cracked_names.csv` was traced to its code call sites
(`asset_usage_raw.csv`, built from the Ghidra whole-program export) and checked for semantic
agreement: does `FX_KLAY_DIE_EXPLODE` actually fire on player death, is `SPR_KLOGG_SHOOT` set
when Klogg shoots, etc. 72 of the 177 named ids have direct code references (the rest are
loaded via data tables). Verdicts below.

**Reading the evidence**: `asset_usage_raw.csv` contains TWO Ghidra export generations
(`export/SLES_010.90.c` = current, `disks/SLES_010.90.c` = older). Rows that look like two
different functions at the same call shape (e.g. `InitPhartHeadCollectible` vs
`InitGroundPatrolEnemy` for `0x8c510186`) are the *same function before/after a rename* — not
a usage conflict. Only `export/` rows were used for verdicts.

## Confirmed: name matches code role (highlights)

| name | id | evidence |
|---|---|---|
| `FX_KLAY_DIE_EXPLODE` | `0x146ce002` | compared as frame-event hash in `PlayerCallback_DeathDebrisSpawner`, `BossCollision_SpawnDebrisAndLayers`, `FinnDamageEventHandler` — all death/debris moments |
| `SPR_KLOGG_SHOOT` | `0x1a2c8516` | set in `KloggSpawnProjectilesCallback` — exactly the shoot moment |
| `SPR_KLOGG_IDLE_1` / `_DIE` | `0x193ca112` / `0x18248010` | `KloggSetMoveState`+`EnemyIdleTimerState` / `EnemyDefeatState` (Klogg FSM) |
| `MEGA_*` (7 sprites) | — | used **exclusively** by `ShrineyGuard*State` functions in src/bosses.c; entity binding perfect (action-word vs FSM-role mapping in asset_ids.h comments) |
| `YNT_DIE` | `0x0c78005c` | `GlennYntisDefeatedState` |
| `FX_BOSS_HEAD_ATTACK/IDLE_*` | — | all in `JoeHeadJoe*` handlers (HEAD boss) |
| `FX_BOSS_KLOGG_IDLE` | `0x486492c4` | `InitKloggBoss` |
| `FX_BUTTON_PAUSE` / `_UNPAUSE` | `0x65281e40`/`0x4c60f249` | `PauseGameAndShowMenu` / `PauseGameWithFadeOut` — a perfect complementary pair |
| `FX_MENU_PASSWORD` | `0x00880c1e` | `PasswordDigitInputHandler`, `PasswordBackspaceHandler`, `EndingCreditsRevealTick` |
| `FX_PICKUP_*` (9 sounds) | — | each played by its matching `Collectible<Item>TickCallback` (FARTHEAD→PhartHead, GROW→ScaleReset, ONE_UP→ExtraLife+`AddPlayerOrbs`, PHOENIX→PhoenixHand, SUPER_WILLIE→SuperWillie, UNIVERSE_ENEMA→UniverseEnema, GLIDEY→YellowBird…) |
| menu text (`NO/YES/PAUSED/QUIT/QUIT GAME/CONTINUE`) | — | swapped in `ToggleMenuOptionState` at fixed menu-slot offsets (+0x94/+0x98/+0x9c) |
| `<PICKUP_PREFIX>STATUSNUMBERS` | `0x00e2f188` | `CreateMenuEntities` instantiates it at ~8 HUD x-positions = the counter digit strip |
| `FX_KLAY_FOOTSTEP_*_NORMAL`, `FX_KLAY_LAND`, `FX_KLAY_RUN_FAST` | — | compared inside `PlaySoundEffect` itself = the surface-variant remap logic (`footstep-remap-table.md`) |
| `Klaymen_idleblink_anim` | `0x5900c41e` | `PlayerState_IdleLookAround` / `PlayerState_PlayRandomIdleAnimation` |
| `FX_KLAY_HIT_HEAD` | `0x50f08207` | falling/jumping physics callbacks (head-bump on ceiling) |

## Misnomers found — the *function* names are wrong (asset names are hash+level anchored)

The BOSS-namespace sprite names carry two independent proofs (hash-exact decode + single-level
affinity), so where they clash with a function name, the **function** (a Ghidra guess) is the
misnomer. Proposed renames, all currently asm-side (fix in Ghidra, then export to
`symbol_addrs_new.txt`):

| function (current) | uses | proposed direction |
|---|---|---|
| `InitKloggChaseState` | sets `WIZARD_ATTACK_01` (`0x022e3442`, ships **WIZZ only**) | Monkey Mage (WIZZ) state, not Klogg |
| `CollectiblePickupState` / `CollectibleRiseState` / `CollectibleFloatState` | set `WIZARD_FALL_01/_02` (WIZZ only) | Monkey Mage fall/float states, not collectibles |
| `EntityDestroyWithEffects` | sets `WIZARD_IDLE_01` **and** plays `FX_BOSS_WIZARD_IDLE` | Monkey Mage idle/reset state (sprite+sound co-occurrence — the trace_sound_sprite discriminator) |
| `BossState_SpawnDeathParticle` | sets `WIZARD_HIT_01` | hit-reaction, not death-particle |
| `HazardIdleWithSound` / `HazardActivateWithSound` / `CollectibleActiveState` | set `YNT_HIT_1` / `YNT_IDLE_1` / `YNT_HIT_2` | Glenn Yntis FSM states, not hazards/collectibles |
| `EnemySpriteState` | sets `KLOGG_HIT` | Klogg hit-reaction state |
| `DecorSetRandomTimer` / `DecorCollisionTickWithSpawnAndSound` | set `<PICKUP_PREFIX>HAMSTER`, play `FX_PICKUP_OBJECT` | hamster-powerup entity, not decor |
| `GlennYntisDamageEventHandler` | plays `FX_SKULL_SCREAM_01` (`0xe0c00650`) which does **not ship in GLEN** | handler is shared with (or belongs to) skullmonkey enemies in BOIL/BRG1/CRYS/…; check before trusting the Glenn binding |

Note on role-vs-decode inside the Shriney FSM: `MEGA_SPIT` is the sprite the FSM uses as the
idle pose, `MEGA_IDLE_1` is used as attack-windup. Both facts stand — the *artist's* animation
name and the *engine's* use of it differ; `asset_ids.h` records both.

## Misnomers found — the *asset* names are wrong (demoted to candidate)

| name | id | why |
|---|---|---|
| `FX_PLAYER_SWIM_PRE` | `0x46346040` | FX-style (audio) name on a **sprite-container** id; only code use is `ClayballState_DestroyWithDebris` spawning it as a debris sprite. Likely calcHash collision from the gist audit. |
| `FX_OBJECT_ELEVATOR` | `0x06e0a824` | used by `UpdateHUDEntityVisibility`, `PlayerCallback_RestoreNormalHitbox`, `PlayerState_StartSwimming` — player/HUD context, zero elevator signal; single brute hit. |

## New cracks from suffix-gap prediction (this pass)

Method: for every known name, enumerate sibling spellings (numeric suffixes, action words per
namespace grammar) and accept only candidates whose predicted id **exists in the 658-id ROM
universe** (per-guess false-positive rate ≈ 1.5e-7) with level/family corroboration:

| id | name | corroboration |
|---|---|---|
| `0x6c351094` | `<PICKUP_PREFIX>UNIVERSE_ENEMA_2` | exact sibling of `_1`, same ship-everywhere footprint |
| `0x08624580` | `<PICKUP_PREFIX>SHRINK` | ships BRG1+SOAR **only**, exactly like… |
| `0x62000441` | `FX_PICKUP_SHRINK` | …this sound (BRG1+SOAR only) — sprite/sound pair confirms both; collision alt `FX_YNT_LEVER_02` rejected |
| `0x5101c047` | `FX_BOSS_YNT_HIT` | GLEN-only, 5-member family |
| `0x7a318245` | `FX_BOSS_YNT_IDLE_01` | GLEN-only, pairs with `_02` |
| `0x62318245` | `FX_BOSS_YNT_IDLE_02` | GLEN-only, pairs with `_01` |
| `0x7623c165` | `FX_BOSS_YNT_ATTACK` | GLEN-only, sibling `_2` exists |
| `0x7622c165` | `FX_BOSS_YNT_ATTACK_2` | GLEN-only |
| `0xc2820c70` | `FX_YNT_FLY_01` (CANDIDATE) | single hit; EGGS/GLID/WEED fits flying Ynts; no sibling |

Rejected: `FX_SKULL_ZAP_12` for `0x78100250` — suffix `_12` with no `_01..\_11` siblings =
collision noise.

**Tally after this pass: 167 verified + 8 candidate + 2 ScummVM anchors = 177/658 named.**

## Runtime follow-up (not yet done)

Static tracing can't observe data-table-driven ids (96 named ids have no code refs) or confirm
the *moment* of frame-events. `scripts/frame_event_tracer.lua` + `make record` sessions remain
the way to watch `FX_KLAY_DIE_EXPLODE` fire on an actual death; the existing
`game_watcher/logs/*.jsonl` are struct-field watches and contain no sprite/sound ids.
