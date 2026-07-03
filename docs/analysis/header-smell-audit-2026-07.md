---
title: "Header/Source Smell Audit — 2026-07"
category: analysis
tags: [audit, headers, cleanup, enums, duplicate-types]
---

# Header / Source Smell Audit (2026-07-03)

A parallel read-only audit of every `include/Game/*.h` and its source, looking
for enum candidates, struct/naming smells, duplicate types, pointer-arith, and
comment/signature mismatches. 12 subsystem clusters were audited independently;
each finding was adversarially verified (45 CONFIRMED / 2 DOWNGRADE / 0 INVALID
across the verified subset). Guardrail: the `make analyze` layout-assert harness
(`tools/analysis/`) proved every applied edit is layout-preserving.

**Constraint recap:** only comments, enum-constant/field/typedef renames, and
idempotent `#include` additions were applied — all compile-checked and
byte-neutral. Nothing that changes struct layout, integer literals, function
symbols (wired via `symbol_addrs.txt`), or codegen was touched.

## Applied (byte-safe, this branch)

| # | Change | File(s) |
|---|--------|---------|
| 1 | `EntityDefinition`: `u32 padding0E` → `u8 padding0E[4]` (0x0E misaligned → u32 bumped to 0x10, broke 0x18 layout) | entity.h |
| 2 | `entity_events.h`: rename `EVT_COLLECTIBLE_PICKUP` → `EVT_ENTER_SHRINK_ZONE` (0x100C is the shrink-zone message per docs/reference/entity-event-ids.md), add 5 missing documented ids (0x1007/0x100A/0x100D/0x1010/0x1017), annotate overloaded ids | entity_events.h |
| 3 | Break `HazardTimerEntity` name collision: boss variant (timer@0x114) → `BossHazardTimerEntity`; enemy variant (timer@0x110) keeps name; cross-ref comments both sides | boss_entities.h, enemy_entities.h, bosses.c |
| 4 | Rename `RenderSprite` **typedef** → `RenderSpriteObj` (shadowed the engine function symbol `RenderSprite@0x8007BDE8`); 27 call sites updated, function name untouched | spracc_records.h, spracc.c, playst.c, pickups.c, effects.c, pickups_entities.h |
| 5 | `EnemyTimerStateEntity.pad111` → `stateInitDelay` (live per-state init field, not padding) | enemy_entities.h, enemies.c |
| 6 | `SoarPlayerEntity.handle10C` → `spuVoice` (matches `RunnPlayerEntity.spuVoice` @ same offset) | entity.h |
| 7 | `FinnSubentityStateFlags.pad10E` → `animPauseFlag` (gates `SetAnimationActive(e,0)`) | finn_entities.h, finn.c |
| 8 | `EndingCreditsEntity.field140` → `scrollCounter3` (phase-3 scroll countdown, sibling of scrollCounter1/2) | ending_entities.h, ending.c |
| 9 | `fsm_dispatch.h`: add idempotent `#include "common.h"` + `"Game/game_state.h"` (used raw `s16`/`GameState`; now compiles standalone) | fsm_dispatch.h |

## The 5 duplicate typedefs (latent — no TU includes a colliding pair)

| Type | Headers | Nature |
|------|---------|--------|
| `SpriteContextCallbackTable` | anim_entities, entity_records | byte-identical |
| `DecorSpawnData` | decor_entities, pickups_entities | byte-identical |
| `SpriteContext` | blbacc_records, sprite | same size 0x14, divergent names/types |
| `SpriteRenderContext` | anim_entities, enemy_entities, entity_records | 3 divergent partial views of one struct |
| `HazardTimerEntity` | boss_entities, enemy_entities | **conflicting layout — two different entities** (RESOLVED above) |

## Report-only (NOT applied — require code changes or splat re-extract)

### Call-site enum substitutions (codegen-identical, but touch `.c` code)
- **EVT_ literals in bosses.c** (0x1001/0x1002/0x1016 @ lines 110,111,115,137,138,142) → EVT_SET_READY/EVT_TOKEN_QUERY/EVT_SET_STATE_FLAG.
- **enemies.c:1243** raw `0x1000` → `EVT_DAMAGE`; **enemies.c cull family** literal `3` → `EVT_GAME_NOTIFY` (5 sites; FSM_REG register-pinned — re-verify SHA1).
- **bosses.c:124-125,151-152** `0x10D86282`/`0xB0C10420` → `FE_GLIDER_ENABLE`/`FE_GLIDER_DISABLE` (needs `#include "Game/anim_frame_events.h"`).
- **entinit.c GridHelper_Init** 0x61/0x62/0x6E/0x6F → existing `ENTITY_TYPE_97/98/110/111_GRID_HELPER`.
- **New enums worth adding:** `PlaybackMode` (BLB sequence_modes 1..6, ~23 sites in level.c/blbacc.c); `LevelModeFlags`/`LevelFlags` (0x4/0x10/0x100/0x200/0x400/0x2000/0x4000/0x8000 in entinit.c, main.c, game_state.h prose); missing `AnimChangeFlags` bits (0x200/0x400 by-value, 0x001/0x002 apply — entity.h, "don't modify unless instructed").

### Signature conflicts (real; can change codegen — needs verification)
- **`PlaySoundEffect`** declared `void` in functions.h:43 / hud.c but returns `s32` (SPU voice index); ending.c already works around it.
- **`FindSaveSlotByName`** main.c extern returns `s32`, level.c defines `void`.
- **`AllocPrim20_Pool3/24/28/36`** declared `GameGlobals*` in spracc.c, defined `Globals*` in prim.c (same struct, two names — see below).
- (These correspond to the `conflicting-types` entries in `tools/analysis/warnings.baseline.txt`.)

### Duplicate structs beyond the known 5 (consolidation = code change)
- **`GameGlobals` (spracc_records.h) ≡ `Globals` (prim_records.h)** — same heap-base struct twice; `GpuContext` vs `GpuPrimitiveContext` two views of one GPU context.
- **`BossVoiceEntity` ≡ `BossWithSpuVoiceEntity`** (byte-identical, same header).
- **`SpriteRenderContextRef`** — a 4th spelling of `SpriteRenderContext`.
- **`ColorTintEntry` (layer_entry.h) ≡ `TriggerZoneRGB` (trigger_zone.h)** — identical `{u8 r,g,b}`.
- **`SoundEntry` (sound.h)** orphan, diverges from `SoundDefinition` (sound_records.h).
- **`TileHeader` (tile_header.h)** duplicates the live `LevelShapeHeader` (blbacc_records.h) and disagrees on +0x04..+0x07 — needs blb.hexpat ground-truth.
- **`CallbackSlot` family** re-declared locally in menu.c/effects.c instead of including callback_slot.h.

### Symbol renames (need splat re-extract — report only)
- `SetAnimationSpriteCallback` → `SetAnimationTargetFrameByValue` (sets target frame by value, not a sprite).
- `EntitySetRenderFlags` → `SetAnimationLoopFlag` (sets pending anim loop flag, not render flags).
- `GetAssetCount` → `GetMovieCount` (returns the movie-table count).

### Pointer-arith / struct-materialization candidates
- **blbacc.c:304** layer_entries walked with magic 92-byte stride + raw +0x2C → define `LevelLayerEntry` (0x5C).
- **hud.c:37** raw tint writes at 0x34/0x35/0x36 → `RenderSpriteObj.r/g/b`.
- **ShrineyGuardEntity** — raw `*(u32*)((u8*)e+0x118)` etc.; fields could be materialized inside existing padding under the layout-assert guard.
- **tile_header.h** level_flags bits 0x0400/0x2000 tested in entinit.c but undocumented; `TILE_FLAG_*` macros are unused.

### Comment/naming polish (safe, deferred)
- `AdvAnimState.frameCount` → `curSpriteFrameCount` (matches SpriteEntity@0xD8).
- `TimerTileRecord` ≡ `SwitchBlockSpawn` (identical, same header) — merge.
- `SpecialTriggerEntity` ≡ `InteractiveDecorEntity` (identical layout, field-name only diff).
- `game_state.h` scroll_limit_* labels contradict layer_entry.h mapping; `entity_pool_ptr`@0x16C is really the depth-bucket head array.
- `EVT_TOKEN_QUERY` (0x1002) / `EVT_TOKEN_RELEASE` (0x1009) comments now note the dominant-vs-overload meanings (applied in item 2).
