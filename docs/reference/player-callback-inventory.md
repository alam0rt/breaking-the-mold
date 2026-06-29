---
title: "Player Callback Inventory"
category: reference
tags: [reference, player, callback, inventory]
---

# Player Callback Inventory

Complete inventory of all player-related functions extracted from the Ghidra C
export (`docs/ghidra/SLES_010.90.c`). Used as a worklist for decompilation and
as a cross-reference when tracing state machines.

**Counts (as of 2026-04-21):**
- **101** unnamed `PlayerCallback_800XXXXX` functions (callbacks named by address only)
- **16** named `PlayerState_*` state handlers
- **4** `PlayerStateCallback_N` dispatcher slots
- **7** named utility functions
- **Total: 128 player-related functions**

KNOWLEDGE_GAPS.md lists "98 PlayerCallback functions renamed 2026-01-20"; the
101 counted here are the same set (the difference is post-Jan-20 discoveries
plus the fact that Ghidra's rename didn't deduplicate against a few special
entries).

---

## Named Player State Callbacks

Dispatcher slots in `g_PlayerStateCallbackTable @ 0x800A5D20` (4 entries × 8 bytes).
These route the per-frame tick to one of the state functions below based on
the player's current mode.

| Address | Name | Role |
|---------|------|------|
| 0x80066CE0 | `PlayerStateInit_Idle` | Respawn state |
| 0x8006A310 | `PlayerState_HideAndClearBounce` | Special mode |
| 0x8006864C | `PlayerStateCallback_2` | Normal gameplay (main) |
| 0x8006AE0C | `PlayerState_SetupBounceRight` | Alt state |

### PlayerState_* handlers (16)

These are the concrete state-machine states invoked via `EntitySetState` with
a function pointer from the callback table at `0x800A5D20+` (entries named
`_800a5dXX` below).

| State | Table slot | Description |
|-------|------------|-------------|
| `PlayerState_StandingIdle` | 0x800a5ef0 | Idle, no input |
| `PlayerState_WalkingRight` | 0x800a5e14 | Horizontal walk (+x) |
| `PlayerState_WalkingLeft` | 0x800a5e1c | Horizontal walk (-x) |
| `PlayerState_Running` | 0x800a5dd4 | Run-speed locomotion |
| `PlayerState_QuickTurn` | 0x800a5d5c | Direction reversal animation |
| `PlayerState_Jump` | 0x800a5e54 | Rising jump |
| `PlayerState_JumpApex` | 0x800a5e0c | Peak of jump |
| `PlayerState_Falling` | 0x800a5e5c | Falling (any cause) |
| `PlayerState_LandingFromRide` | 0x800a5ee0 | Dismount transition |
| `PlayerState_PickupItem` | 0x800a5db4 | Item collect animation |
| `PlayerState_SpecialIdleAnim` | 0x800a5e6c | Idle variant |
| `PlayerState_SpecialMove1` | 0x800a5ee8 | Context-specific move |
| `PlayerState_SpecialMove2` | 0x800a5e44 | Context-specific move |
| `PlayerState_CheckpointExit` | 0x800a5dc4 | Leaving checkpoint |
| `PlayerState_LevelExitTeleporter` | 0x800a5dbc | Portal exit |
| `PlayerState_Death` | 0x800a5d84 | Death animation |

---

## Named Utility Functions

| Address | Name | Purpose |
|---------|------|---------|
| — | `PlayerTickCallback` | Per-frame player entity tick (top of player update chain) |
| — | `PlayerApplyPositionWithCollision` | Commit new position, clipping against tiles |
| — | `PlayerProcessTileCollision` | Switch-statement on tile trigger attributes (see `systems/collision-complete.md`) |
| — | `PlayerProcessBounceCollision` | Bounce-physics resolution |
| — | `PlayerCallback_HandleMovementAndCollision` | Movement integrator |
| — | `PlayerBounceCallback_Primary` | Primary bounce state handler |
| — | `PlayerSetupBounceAnimation` | Configure bounce-state animation |

---

## Unnamed `PlayerCallback_800XXXXX` (101)

These callbacks have been identified by address but not yet given semantic
names. They are sorted by address; the address ranges form natural clusters
that likely correspond to state-machine groups (see cluster notes below).

### Cluster A — 0x8005bad0–0x8005c160 (10 callbacks, early-state group)

| Address | Symbol |
|---------|--------|
| 0x8005bad0 | `PlayerCallback_8005bad0` |
| 0x8005bb64 | `PlayerCallback_8005bb64` |
| 0x8005bb80 | `PlayerCallback_8005bb80` |
| 0x8005bbac | `PlayerCallback_8005bbac` |
| 0x8005bbe0 | `PlayerCallback_8005bbe0` |
| 0x8005bc3c | `PlayerCallback_8005bc3c` |
| 0x8005bd60 | `PlayerCallback_8005bd60` |
| 0x8005bdb4 | `PlayerCallback_8005bdb4` |
| 0x8005c01c | `PlayerCallback_8005c01c` |
| 0x8005c160 | `PlayerCallback_8005c160` |

### Cluster B — 0x8005cc84–0x8005d25c (5)

| Address | Symbol |
|---------|--------|
| 0x8005cc84 | `PlayerCallback_8005cc84` |
| 0x8005cce0 | `PlayerCallback_8005cce0` |
| 0x8005cec8 | `PlayerCallback_8005cec8` |
| 0x8005d0c8 | `PlayerCallback_8005d0c8` |
| 0x8005d25c | `PlayerCallback_8005d25c` |

### Cluster C — 0x8005db74–0x8005ef28 (8)

| Address | Symbol |
|---------|--------|
| 0x8005db74 | `PlayerCallback_8005db74` |
| 0x8005deb0 | `PlayerCallback_8005deb0` |
| 0x8005e248 | `PlayerCallback_8005e248` |
| 0x8005e510 | `PlayerCallback_8005e510` |
| 0x8005e650 | `PlayerCallback_8005e650` |
| 0x8005ea80 | `PlayerCallback_8005ea80` |
| 0x8005ea8c | `PlayerCallback_8005ea8c` |
| 0x8005eadc | `PlayerCallback_8005eadc` |
| 0x8005ef28 | `PlayerCallback_8005ef28` |

### Cluster D — 0x8005f0b8–0x8005ff20 (11)

| Address | Symbol |
|---------|--------|
| 0x8005f0b8 | `PlayerCallback_8005f0b8` |
| 0x8005f25c | `PlayerCallback_8005f25c` |
| 0x8005f3d4 | `PlayerCallback_8005f3d4` |
| 0x8005f430 | `PlayerCallback_8005f430` |
| 0x8005f540 | `PlayerCallback_8005f540` |
| 0x8005f604 | `PlayerCallback_8005f604` |
| 0x8005f834 | `PlayerCallback_8005f834` |
| 0x8005fb18 | `PlayerCallback_8005fb18` |
| 0x8005fccc | `PlayerCallback_8005fccc` |
| 0x8005fcf0 | `PlayerCallback_8005fcf0` |
| 0x8005fd1c | `PlayerCallback_8005fd1c` |
| 0x8005fd3c | `PlayerCallback_8005fd3c` |
| 0x8005ff20 | `PlayerCallback_8005ff20` |

### Cluster E — 0x800600e0–0x80060fe8 (15)

| Address | Symbol |
|---------|--------|
| 0x800600e0 | `PlayerCallback_800600e0` |
| 0x800602e0 | `PlayerCallback_800602e0` |
| 0x80060470 | `PlayerCallback_80060470` |
| 0x800605d8 | `PlayerCallback_800605d8` |
| 0x80060750 | `PlayerCallback_80060750` |
| 0x800607b8 | `PlayerCallback_800607b8` |
| 0x80060890 | `PlayerCallback_80060890` |
| 0x80060a5c | `PlayerCallback_80060a5c` |
| 0x80060b58 | `PlayerCallback_80060b58` |
| 0x80060c28 | `PlayerCallback_80060c28` |
| 0x80060c9c | `PlayerCallback_80060c9c` |
| 0x80060d98 | `PlayerCallback_80060d98` |
| 0x80060dc4 | `PlayerCallback_80060dc4` |
| 0x80060e24 | `PlayerCallback_80060e24` |
| 0x80060f38 | `PlayerCallback_80060f38` |
| 0x80060fe8 | `PlayerCallback_80060fe8` |

### Cluster F — 0x8006120c–0x80061934 (5)

| Address | Symbol |
|---------|--------|
| 0x8006120c | `PlayerCallback_8006120c` |
| 0x80061764 | `PlayerCallback_80061764` |
| 0x80061854 | `PlayerCallback_80061854` |
| 0x8006187c | `PlayerCallback_8006187c` |
| 0x80061934 | `PlayerCallback_80061934` |

### Cluster G — 0x8006208c–0x80062ad4 (7)

| Address | Symbol |
|---------|--------|
| 0x8006208c | `PlayerCallback_8006208c` |
| 0x800622c4 | `PlayerCallback_800622c4` |
| 0x800622d0 | `PlayerCallback_800622d0` |
| 0x800622d4 | `PlayerCallback_800622d4` |
| 0x800622ec | `PlayerCallback_800622ec` |
| 0x80062334 | `PlayerCallback_80062334` |
| 0x800625b4 | `PlayerCallback_800625b4` |
| 0x80062ad4 | `PlayerCallback_80062ad4` |

### Cluster H — 0x80063308–0x80064b40 (main movement/collision area, 10)

Likely contains the bulk of per-frame movement/physics. `0x800638d0` is the
main player movement callback referenced throughout `collision-complete.md`.

| Address | Symbol |
|---------|--------|
| 0x80063308 | `PlayerCallback_80063308` |
| 0x80063650 | `PlayerCallback_80063650` |
| 0x80063698 | `PlayerCallback_80063698` |
| 0x800638d0 | `PlayerCallback_HandleMovementAndCollision` (named) |
| 0x80063f38 | `PlayerCallback_80063f38` |
| 0x80063fdc | `PlayerCallback_80063fdc` |
| 0x80064008 | `PlayerCallback_80064008` |
| 0x800649ec | `PlayerCallback_800649ec` |
| 0x80064a34 | `PlayerCallback_80064a34` |
| 0x80064b04 | `PlayerCallback_80064b04` |
| 0x80064b40 | `PlayerCallback_80064b40` |

### Cluster I — 0x8006505c–0x800659cc (7)

| Address | Symbol |
|---------|--------|
| 0x8006505c | `PlayerCallback_8006505c` |
| 0x800650c4 | `PlayerCallback_800650c4` |
| 0x80065174 | `PlayerCallback_80065174` |
| 0x80065400 | `PlayerCallback_80065400` |
| 0x800655d4 | `PlayerCallback_800655d4` |
| 0x8006597c | `PlayerCallback_8006597c` |
| 0x800659cc | `PlayerCallback_800659cc` |

### Cluster J — 0x80065bb4–0x80066e38 (late-state group, 18)

| Address | Symbol |
|---------|--------|
| 0x80065bb4 | `PlayerCallback_80065bb4` |
| 0x80065bc0 | `PlayerCallback_80065bc0` |
| 0x80065e84 | `PlayerCallback_80065e84` |
| 0x80065e88 | `PlayerCallback_80065e88` |
| 0x80066040 | `PlayerCallback_80066040` |
| 0x8006604c | `PlayerCallback_8006604c` |
| 0x80066050 | `PlayerCallback_80066050` |
| 0x80066064 | `PlayerCallback_80066064` |
| 0x8006606c | `PlayerCallback_8006606c` |
| 0x800660bc | `PlayerCallback_800660bc` |
| 0x800660c8 | `PlayerCallback_800660c8` |
| 0x8006627c | `PlayerCallback_8006627c` |
| 0x800666cc | `PlayerCallback_800666cc` |
| 0x80066808 | `PlayerCallback_80066808` |
| 0x80066990 | `PlayerCallback_80066990` |
| 0x80066c24 | `PlayerCallback_80066c24` |
| 0x80066c40 | `PlayerCallback_80066c40` |
| 0x80066e38 | `PlayerCallback_80066e38` |

---

## How to Use This Inventory

**For decomp prioritization** (see [decompilation-guide.md](../decompilation-guide.md)):
1. Named `PlayerState_*` handlers are the highest-value targets — they are
   directly invoked by the dispatcher and have well-understood semantics.
2. Cluster H (`0x80063308`–`0x80064b40`) contains the core movement/collision
   loop. Decompiling this unlocks most other player work.
3. Unnamed callbacks in other clusters are likely *substates* invoked by the
   named `PlayerState_*` handlers via `EntitySetState`. Trace incoming call
   sites in `docs/ghidra/SLES_010.90.c` before decompiling each.

**For naming**: when decomp reveals a callback's purpose, add it to
`symbol_addrs.txt` (see `CLAUDE.md` §Decompilation Conventions) so future
Ghidra exports carry the name.

**Rebuild this inventory** (when Ghidra is updated) with:

```bash
grep -nE '^(void|undefined[0-9]*|int|s32|u32|char)\s+Player[a-zA-Z_0-9]+\s*\(' \
    docs/ghidra/SLES_010.90.c
```

---

## Related Documentation

- [Player System](../systems/player/player-system.md) — high-level overview
- [Player Physics](../systems/player/player-physics.md) — verified constants
- [Player Animation](../systems/player/player-animation.md) — sprite frames
- [Collision System](../systems/collision-complete.md) — tile triggers invoked by Player callbacks
- [decompilation-guide.md](../decompilation-guide.md) — decomp priority ordering
