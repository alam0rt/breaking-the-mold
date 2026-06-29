---
title: "Session 25 — Level-exit teleporters & branching (stage 2a/2b/bonus)"
category: gist/asset-system
tags: [gist, asset, system, teleporters]
---

# Session 25 — Level-exit teleporters & branching (stage 2a/2b/bonus)

Found the multi-destination teleporter logic — how an exit can lead to stage 2a, 2b, or bonus.

## The branch chain (traced end-to-end)
1. Teleporter = a Portal-class entity. Player touches it -> **PlayerState_LevelExitTeleporter**
   (0x...32620): StopCDStreaming, +0x1b2=1, links the teleporter entity at player[+0x5a].
2. The teleporter's callback writes the DESTINATION into GameState:
   **+0x19c = 1** (branch flag) and **+0x19d = <destination level index>**.
3. **GameStateCollisionCallback** (0x...41498) dispatches:
   - if +0x146 set AND +0x19c != 0 -> `SetupAndStartLevel(state, *(+0x19d))` = **JUMP to that
     explicit level** (the branch).
   - normal level-complete sets +0x147 -> `SeekToLevelInSequence(.,0,1,0)` ->
     `SetupAndStartLevel(99)` = **sequential next** entry in the playback sequence.

## Two destination-selection mechanisms
- **FIXED branch — Callback_8004906c (line 15783):** `dest = teleporter_entity[+0x50]` ->
  GameState+0x19d. Each teleporter is PLACED with a hardcoded destination level index in its
  Asset-501 variant data. **Two/three teleporters in one level = two/three fixed exits**
  (stage 2a / 2b / bonus). Player picks by walking into one. THIS is your branching exit.
- **RANDOM branch — Callback_800670bc (line 31210):** `rand() % count[+0x67]` indexes a
  destination table `entity[idx+0x60]`, with a "don't repeat last (+0x19d)" guard. A
  teleporter that warps to a RANDOM destination from a set (bonus roulette).

## Key fields
| field | meaning |
|---|---|
| GameState+0x19c | branch flag (1 = explicit dest, not sequence advance) |
| GameState+0x19d | destination level index (byte) -> SetupAndStartLevel param_2 |
| GameState+0x146 | level-transition trigger |
| GameState+0x147 | sequential next-level flag (normal exit) |
| teleporter+0x50 | this teleporter's FIXED destination index |
| teleporter+0x60.., +0x67 | random-teleporter destination table + count |
| player+0x5a | linked teleporter entity ref |

## How it ties together
`SetupAndStartLevel(state, byte level_index)` -> indexes the **LevelMetadata table**
(metadata[idx*0x70], Session 20) -> loads that level's primary/secondary/tertiary assets.
The branch simply OVERRIDES which metadata entry loads next, instead of walking the playback
sequence. So: linear progression = sequence walk; branching exits = explicit index from the
teleporter you entered.

## Net
- Multi-exit branching fully explained: per-teleporter destination index (fixed via placement,
  or random via table) -> GameState+0x19d -> SetupAndStartLevel jumps there.
- New GameState fields: +0x19c branch flag, +0x19d destination index, +0x146/+0x147 transition.
- Two teleporter callbacks identified (fixed 0x8004906c, random 0x800670bc).

## Files
- `teleporter_branching.json`
