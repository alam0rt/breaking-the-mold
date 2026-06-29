---
title: "Session 11 — Full-corpus usage dig (sounds, grammar, player variants)"
category: gist/asset-system
tags: [gist, asset, system, usage, deep]
---

# Session 11 — Full-corpus usage dig (sounds, grammar, player variants)

## Full-corpus sound-trigger sweep — boundary mapped honestly
Of 114 uncracked audio ids, only **12 have any hardcoded trigger call site**; the rest are
data-table-driven (played by id from level metadata, not `PlaySoundEffect(0x..)` in code).
Trigger-augmented grammar attack on those 12 → no NEW coherent cracks beyond session 10's
FX_BUTTON_PAUSE/UNPAUSE.

## Learned FX_ grammar (position-aware, from 113 verified names)
- pos0 (category): BOSS·KLAY·AMBIENT·SKULL·PICKUP·OBJECT·MENU·BIRD·EXPLODE·FINN·PUFF·PLAYER
- pos1 (subject): KLOGG·FOOTSTEP·WIZARD·HEAD·DRIP·MEGA·LAND·DIE·FLY·RUN·WILLIE·WATERFALL...
- pos2 (action): 01·HIT·02·IDLE·TAUNT·ATTACK·LEFT·RIGHT·FIRE·FAST·QUIET·TURN
- pos3: SOFT·1·2·3·NORMAL·01·02
- co-occurrence learned (e.g. BOSS→{HEAD,KLOGG,MEGA,WIZARD}; SKULL→{BITE,EAT,FIRE,FLAP,...}).

## False-positive discipline (important)
Grammar-only attack on uncracked audio produced 2 "unique" hits
(FX_FINN_CRYSTAL_LOUD_06, FX_AMBIENT_1970_SHOOT_01) — **both REJECTED**:
- expected random collisions for a 1.2M-candidate space over 114 ids = 0.03; getting ~2 is
  pure chance.
- both are semantically incoherent (FINN has no crystals; AMBIENT doesn't SHOOT).
- neither has trigger-context validation.
Rule reaffirmed: accept a name only with (a) coherent grammar AND (b) trigger/role validation
OR true uniqueness in a TIGHT vocab. PAUSE/UNPAUSE had both; these had neither.

## Player variant → sprite map (NEW structural finding)
Klaymen has **distinct init functions per gameplay mode**, each with its own player sprite:

| init function | mode | player sprite |
|---|---|---|
| InitPlayerEntity | normal platforming | 0x21842018 |
| CreateFinnPlayerEntity | FINN swim / 3D rails | 0x3DA80D13 |
| CreateSoarPlayerEntity | SOAR flying stage | 0xCA1B20CB |
| CreateGlidePlayerEntity | GLID gliding | (table-driven, no literal) |
| CreateRunnPlayerEntity | RUNN auto-runner | (table-driven, no literal) |
| CreateBossPlayerEntity | **MENU password screen** (decompiler misname) | 0xEC95689B |

## Entity behavior wiring
Init functions connect a sprite to behavior via `EntitySetState(entity, _, PTR_Callback_*)`.
InitMonkeyMageBoss wires 0x0244655D → Callback_800479d0 (+ a second state) — the boss's 2-state
behavior tree. Most player-variant inits wire state via local pointers (computed, not literal).

## Net
- Sound corpus boundary established (only 12 trigger-anchored; rest data-driven).
- 2 grammar false-positives caught and rejected (discipline preserved).
- Player-variant→sprite→stage map produced (`player_variants.json`); CreateBossPlayerEntity
  re-confirmed as the password screen.

## Files
- `player_variants.json`
