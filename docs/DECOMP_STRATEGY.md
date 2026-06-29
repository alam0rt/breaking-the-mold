---
title: "Decompilation Strategy"
category: root
tags: [decomp, strategy]
---

# Decompilation Strategy

Currently the only C file in `src/Game/` is `RENDER.c`. The documentation layer is
~92% complete but the code-side decomp has not started in earnest. This document
lays out a dependency-ordered plan for moving code from `asm/` into `src/Game/`
while preserving the byte-match invariant (see `CLAUDE.md` § Build Pipeline).

---

## Prerequisites (one-time)

1. Ensure the Ghidra export is up to date: `make ghidra-export-all`, then manually
   merge `symbol_addrs_new.txt` into `symbol_addrs.txt`.
2. Re-extract: `make extract` — verify `undefined_funcs_auto.txt` and
   `undefined_syms_auto.txt` are current.
3. Baseline SHA1: `make check` must pass before starting any decomp.

---

## Dependency Graph

```
                    ┌─────────────────────┐
                    │ PSY-Q libs (libcd,  │   already linked from tools/,
                    │  libgte, libgs, …)  │   no decomp needed
                    └──────────┬──────────┘
                               │
                    ┌──────────┴──────────┐
                    │  Game loop / main    │  LEVEL 0 (infrastructure)
                    │  Heap allocator      │
                    │  GameState init      │
                    └──────────┬──────────┘
                               │
          ┌────────────────────┼────────────────────┐
          │                    │                    │
  ┌───────┴────────┐  ┌────────┴────────┐  ┌───────┴────────┐
  │ BLB loader     │  │ Tile/collision  │  │ Sprite / RLE   │  LEVEL 1 (data)
  │ (Asset 100-700 │  │ system (Asset   │  │ renderer       │
  │  parsing)      │  │  500, 100)      │  │ (Asset 600)    │
  └───────┬────────┘  └────────┬────────┘  └───────┬────────┘
          │                    │                    │
          └──────────┬─────────┴────────────────────┘
                     │
        ┌────────────┴────────────┐
        │ Entity system            │          LEVEL 2 (gameplay core)
        │ (Asset 501, callback     │
        │  table @ 0x8009d5f8)     │
        └────────────┬─────────────┘
                     │
          ┌──────────┼──────────┐
          │          │          │
  ┌───────┴──────┐ ┌─┴─────┐ ┌──┴─────────┐
  │ Player (128) │ │ Camera│ │ Collision  │  LEVEL 3 (player)
  │  callbacks   │ │       │ │  response  │
  └───────┬──────┘ └───────┘ └────────────┘
          │
   ┌──────┴────────┐
   │ Enemies /      │                        LEVEL 4 (AI)
   │ Boss AI (5)    │
   └────────────────┘
```

An arrow from A → B means B depends on A; decompile A before B, or you'll have to
keep re-stubbing A's symbols.

---

## Priority Queue

Each item is ranked by (1) number of downstream dependents and (2) unlock value
for other subsystems. Lower numbers = higher priority.

### Tier 1 — Infrastructure (unblocks everything)

| Priority | Subsystem | Functions (est.) | Notes |
|----------|-----------|------------------|-------|
| 1 | RENDER.c completion | ~90 INCLUDE_ASM | Already partially done — finish first for iteration confidence |
| 2 | Heap allocator | `AllocateFromHeap`, `blbHeaderBufferBase` | Used by every entity spawn |
| 3 | GameState init | `InitGameState`, `SpawnPlayerAndEntities` | Sets up all the pointers downstream code reads |
| 4 | Game loop | See `systems/game-loop.md` | Mode dispatch, frame cadence |

### Tier 2 — Data layer

| Priority | Subsystem | Source doc | Blocker? |
|----------|-----------|------------|----------|
| 5 | BLB loader | `blb/README.md`, `blb/asset-types.md` | YES for all gameplay |
| 6 | Tile collision (`0x800241f4`, `0x80024cf4`) | `systems/collision-complete.md` | YES for player/entity |
| 7 | Sprite/RLE renderer | `systems/sprites.md` | YES for visible anything |
| 8 | Palette / VRAM setup | `systems/tiles-and-tilemaps.md` | YES for rendering |
| 9 | Animation framework (5-layer) | `systems/animation-framework.md` | Soft blocker |

### Tier 3 — Gameplay core

| Priority | Subsystem | Key function | Notes |
|----------|-----------|--------------|-------|
| 10 | Entity spawn (`LoadEntitiesFromAsset501`) | `0x80024dc4` | Needed for *any* level to populate |
| 11 | `RemapEntityTypesForLevel` | `0x8008150c` | **Layer-dependent** — see ENTITY_REMAPPING_CORRECTION.md |
| 12 | Entity callback table (121 types) | `0x8009d5f8` | Dispatch to type-specific init |
| 13 | `CheckEntityCollision` | `0x800226f8` | Entity-vs-entity (players, enemies, pickups) |
| 14 | `PlayerProcessTileCollision` | `0x8005a914` | Tile-trigger switch (wind, spawn zones, etc.) |

### Tier 4 — Player (most code, but isolated)

| Priority | Group | Target | Count |
|----------|-------|--------|-------|
| 15 | Player dispatcher | `PlayerStateCallback_0..3` (@ 0x800A5D20) | 4 |
| 16 | Core movement/collision cluster (H) | `0x80063308–0x80064b40` | 11 |
| 17 | Named `PlayerState_*` handlers | 16 handlers | 16 |
| 18 | Remaining unnamed `PlayerCallback_*` | Clusters A–G, I, J | ~85 |

Use [`reference/player-callback-inventory.md`](reference/player-callback-inventory.md)
as the worklist. Within each cluster, start with the callback that has the most
incoming call-sites (easier to verify against the original once decomp'd).

### Tier 5 — AI / Enemies / Bosses

| Priority | Subsystem | Count | Blockers |
|----------|-----------|-------|----------|
| 19 | Enemy init callbacks (`EntityType025_…`, etc.) | ~30 | Tier 3 done |
| 20 | Enemy AI patterns (patrol/fly/shooter/chase/hop) | 5 patterns | Tier 3 done |
| 21 | Boss init + state machines | 5 bosses × ~15 fns ≈ 76 | Tier 3 + Tier 4 |

### Tier 6 — Polish

| Priority | Subsystem | Notes |
|----------|-----------|-------|
| 22 | HUD / menu / password | Standalone, not on hot path |
| 23 | Audio (SPU integration) | Most is PSY-Q library calls |
| 24 | Demo / attract mode | Uses Asset 700 replay data |

---

## Per-Function Workflow

Reminder from `CLAUDE.md`:

```bash
make context                  # regenerate ctx.c
make decompile FUNC=FuncName  # m2c on asm/nonmatchings/**/FuncName.s
# hand-edit in src/Game/<File>.c, replacing INCLUDE_ASM(...)
make                          # rebuild
make diff FUNC=FuncName       # verify byte-match
# if non-matching:
make permuter-import FILE=src/Game/<File>.c FUNC=FuncName
make permuter FUNC=FuncName
```

Always `make check` after each function lands, before moving to the next.

---

## Risk-Gated Unconfirmed Findings

Before starting a Tier, verify any `BLOCKER`-tagged findings in
[`analysis/unconfirmed-findings.md`](analysis/unconfirmed-findings.md) that apply
to that Tier:

| Tier | Required verifications |
|------|------------------------|
| Tier 2 (BLB loader) | Asset 101 default behavior when absent (12 levels) |
| Tier 3 (entity remap) | Layer upper-byte flag semantics — affects entity types 9, 81 |
| Tier 3 (entity spawn) | TileHeader field_20 purpose (used during level init) |
| Tier 4 (player) | Player entity field map beyond 0x1B0 (KNOWLEDGE_GAPS.md) |

Findings tagged `CORRECTNESS` must be resolved before that Tier lands.
Findings tagged `POLISH` can ship with a TODO comment.

---

## Hotpath Indicators

From `game_watcher/logs/` (if captured), these functions are called every frame
during gameplay and should be decompiled early for iteration speed:

- `PlayerTickCallback @ 0x80059E10`
- `PlayerCallback_HandleMovementAndCollision @ 0x800638d0`
- `EntitySetState @ 0x8001EAAC`
- `SetEntitySpriteId @ 0x8001D080`
- `CheckEntityCollision @ 0x800226f8`
- `UpdateCameraPosition @ 0x80023DBC`

All of these are in Tier 3 or Tier 4 — confirming the dependency order.

---

## When to Switch Compilers

Per `CLAUDE.md`: switching from `gcc-2.7.2-psx` to `gcc-2.5.7-psx` may improve
some functions but break others. **Do not attempt per-file compiler selection
until at least one full Tier has landed cleanly with the default compiler.**
Track any functions that refuse to match under 2.7.2 in a follow-up list for a
future `gcc-2.5.7` pass.

---

## Success Criteria

- Tier 1: `make check` still passes after each function lands, RENDER.c
  INCLUDE_ASM count drops to 0.
- Tier 2: `make check` passes; level 0 (MENU) successfully parses from BLB without
  any runtime patching.
- Tier 3: Entity spawn produces the same entity list on levels SCIE stage 0, CSTL
  stage 0 (tests layer-remapping edge cases).
- Tier 4: Player can move, jump, and die on SCIE stage 0 with no ASM stubs on the
  hot path.
- Tier 5: Each boss fight reaches its defeat sequence with no ASM stubs.

---

## Related Documentation

- [`CLAUDE.md`](../CLAUDE.md) — build pipeline, conventions
- [`decompilation-guide.md`](decompilation-guide.md) — per-function mechanics
- [`reference/player-callback-inventory.md`](reference/player-callback-inventory.md) — Tier 4 worklist
- [`KNOWLEDGE_GAPS.md`](KNOWLEDGE_GAPS.md) — what's still unknown
- [`analysis/unconfirmed-findings.md`](analysis/unconfirmed-findings.md) — risk-tagged research
