---
title: "Skullmonkeys Documentation Index"
category: root
tags: [systems]
---

# Skullmonkeys Documentation Index

**Project**: Evil Engine - Skullmonkeys Reverse Engineering

This index provides quick navigation to all documentation organized by category.

---

## Quick Links

- **[Decompilation Guide](decompilation-guide.md)** - How to add new functions + dependency-ordered decomp priority
- **[Knowledge Gaps](KNOWLEDGE_GAPS.md)** - What we still don't understand
- **[BLB Format Overview](blb/README.md)** - Start here for file format
- **[GameState Fields](systems/gamestate-field-analysis.md)** - GameState structure analysis
- **[Player Callback Inventory](reference/player-callback-inventory.md)** - 128 player-related functions (NEW)

---

## Architecture

**Location**: [`architecture/`](architecture/)

| Document | Description | Status |
|----------|-------------|--------|
| [memory-layout.md](architecture/memory-layout.md) | **Section organization, PSY-Q layout (NEW)** | ✅ Complete |
| [source-directory-hypothesis.md](architecture/source-directory-hypothesis.md) | Ghidra-backed high-level source tree / architecture hypothesis | 🔬 Research |

---

## BLB File Format

**Location**: [`blb/`](blb/)

| Document | Description | Status |
|----------|-------------|--------|
| [README.md](blb/README.md) | BLB format overview and hierarchy | ✅ Complete |
| [header.md](blb/header.md) | Header structure (0x1000 bytes) | ✅ Complete |
| [level-metadata.md](blb/level-metadata.md) | Level entry format (0x70 bytes each) | ✅ Complete |
| [asset-types.md](blb/asset-types.md) | Complete asset type reference (100-700) | ✅ Complete |
| [toc-format.md](blb/toc-format.md) | TOC and sub-TOC structures | ✅ Complete |

**Completion**: 98%

---

## Runtime Systems

**Location**: [`systems/`](systems/)

### Core Engine

| Document | Description | Status |
|----------|-------------|--------|
| [game-loop.md](systems/game-loop.md) | Main loop and mode callbacks | ✅ Complete |
| [fsm-callback-patterns.md](systems/fsm-callback-patterns.md) | Ghidra-backed FSM/callback slot architecture | 🔬 Research |
| [level-loading.md](systems/level-loading.md) | Stage loading state machine | ✅ Complete |
| [rendering-order.md](systems/rendering-order.md) | Layer/entity z-ordering | ✅ Complete |

### Graphics & Animation

| Document | Description | Status |
|----------|-------------|--------|
| [tiles-and-tilemaps.md](systems/tiles-and-tilemaps.md) | Tile graphics and tilemap rendering | ✅ Complete |
| [sprites.md](systems/sprites.md) | RLE sprite format and lookup | ✅ Complete |
| [animation-framework.md](systems/animation-framework.md) | 5-layer animation system | ✅ Complete |
| [animation-setters-reference.md](systems/animation-setters-reference.md) | 8 animation property setters | ✅ Complete |

### Collision & Physics

| Document | Description | Status |
|----------|-------------|--------|
| [collision-complete.md](systems/collision-complete.md) | **Canonical** tile collision reference | ✅ Complete |
| [collision.md](systems/collision.md) | Pointer → collision-complete.md | 🔗 Index |
| [tile-collision-complete.md](systems/tile-collision-complete.md) | ⚠️ Superseded → collision-complete.md | 🔗 Stub |
| [player/player-physics.md](systems/player/player-physics.md) | Player physics constants (verified) | ✅ Complete |
| [camera.md](systems/camera.md) | Camera smooth scrolling system | ✅ Complete |

### Entities & Objects

| Document | Description | Status |
|----------|-------------|--------|
| [entities.md](systems/entities.md) | Entity system and spawn data | ✅ Complete |
| [entity-identification.md](systems/entity-identification.md) | Entity type identification guide | ✅ Complete |

### Player System

**Location**: [`systems/player/`](systems/player/)

| Document | Description | Status |
|----------|-------------|--------|
| [player-system.md](systems/player/player-system.md) | Player mechanics overview | ✅ Complete |
| [player-physics.md](systems/player/player-physics.md) | Physics constants (verified) | ✅ Complete |
| [player-state-fsm.md](systems/player/player-state-fsm.md) | Ghidra-backed normal player FSM slot table | 🔬 Research |
| [player-animation.md](systems/player/player-animation.md) | Player-specific animations | ✅ Complete |
| [player-normal.md](systems/player/player-normal.md) | Normal platforming mode | ✅ Complete |
| [player-finn.md](systems/player/player-finn.md) | FINN vehicle mechanics | ⚠️ Partial |
| [player-bounce-mechanics.md](systems/player/player-bounce-mechanics.md) | Bounce physics | ✅ Complete |
| [trace-findings.md](systems/player/trace-findings.md) | Runtime trace analysis | ✅ Complete |

### Combat & Weapons

| Document | Description | Status |
|----------|-------------|--------|
| [projectiles.md](systems/projectiles.md) | Projectile & weapon system | ✅ Complete |
| [combat-system.md](systems/combat-system.md) | Damage mechanics | ✅ Complete |
| [checkpoint-system.md](systems/checkpoint-system.md) | Save/restore flow | ✅ Complete |

### AI & Behaviors

| Document | Description | Status |
|----------|-------------|--------|
| [enemy-ai-overview.md](systems/enemy-ai-overview.md) | **Enemy AI patterns and behaviors** | ✅ Good (50%) |
| [enemies/README.md](systems/enemies/README.md) | **Individual enemy type documentation** | ✅ Good (27 type docs) |
| [bosses.md](systems/bosses.md) | **Complete boss reference (5 bosses, 80+ functions)** | ✅ Complete |
| [boss-ai/boss-klogg.md](systems/boss-ai/boss-klogg.md) | Klogg (final boss) | ✅ Complete |
| [boss-entity-pattern.md](systems/boss-entity-pattern.md) | Multi-sprite boss init pattern | ✅ Complete |
| [boss-ai/boss-system-analysis.md](systems/boss-ai/boss-system-analysis.md) | Boss AI system architecture | ✅ Complete |
| [boss-ai/boss-behaviors.md](systems/boss-ai/boss-behaviors.md) | **Boss behaviors and attack patterns** | ✅ Good (55%) |
| [boss-ai/boss-shriney-guard.md](systems/boss-ai/boss-shriney-guard.md) | Shriney Guard (tutorial boss) | ✅ Complete |
| [boss-ai/boss-glenn-yntis.md](systems/boss-ai/boss-glenn-yntis.md) | Glenn Yntis (mid-game boss) | ✅ Complete |
| [boss-ai/boss-monkey-mage.md](systems/boss-ai/boss-monkey-mage.md) | Monkey Mage (late-game boss) | ✅ Complete |

### Audio

| Document | Description | Status |
|----------|-------------|--------|
| [audio.md](systems/audio.md) | Audio/sound system overview (SPU SFX, CD-XA, settings) | ✅ Complete |
| [audio-functions-reference.md](systems/audio-functions-reference.md) | 6 audio playback functions | ✅ Complete |
| [sound-effects-reference.md](systems/sound-effects-reference.md) | Sound ID table | ⚠️ Partial |

### Input & Menus

| Document | Description | Status |
|----------|-------------|--------|
| [input-system-complete.md](systems/input-system-complete.md) | Input handling (complete) | ✅ Complete |
| [password-system.md](systems/password-system.md) | Password entry system | ✅ Complete |
| [demo-attract-mode.md](systems/demo-attract-mode.md) | Demo playback system | ✅ Complete |

---

## Technical Reference

**Location**: [`reference/`](reference/)

| Document | Description | Status |
|----------|-------------|--------|
| [physics-constants.md](reference/physics-constants.md) | Complete physics constants (verified) | ✅ Complete |
| [items.md](reference/items.md) | Item/powerup reference | ✅ Complete |
| [entity-types.md](reference/entity-types.md) | Entity callback table (121 types) | ✅ Complete |
| [sprite-ids-complete.md](reference/sprite-ids-complete.md) | **Complete sprite ID extraction (NEW)** | ✅ Good (30+ IDs) |
| [sound-ids-complete.md](reference/sound-ids-complete.md) | **Complete sound ID extraction (NEW)** | ✅ Good (35 IDs) |
| [rom-data-tables.md](reference/rom-data-tables.md) | **ROM table extraction guide (NEW)** | ✅ Complete |
| [entity-sprite-id-mapping.md](reference/entity-sprite-id-mapping.md) | 30+ sprite IDs mapped | ⚠️ Partial |
| [cheat-codes.md](reference/cheat-codes.md) | All 22 cheat codes | ✅ Complete |
| [level-data-context.md](reference/level-data-context.md) | LevelDataContext structure | ✅ Complete |
| [game-functions.md](reference/game-functions.md) | Key function addresses | ✅ Complete |
| [player-callback-inventory.md](reference/player-callback-inventory.md) | **All 128 player-related functions with cluster analysis (NEW)** | ✅ Complete |
| [ENTITY_REMAPPING_CORRECTION.md](reference/ENTITY_REMAPPING_CORRECTION.md) | ⚠️ Layer-dependent entity type mapping (correctness-critical) | ✅ Complete |
| [pal-jp-comparison.md](reference/pal-jp-comparison.md) | Regional version differences | ✅ Complete |
| [source-structure.md](reference/source-structure.md) | **Compilation unit analysis (NEW)** | ✅ Complete |
| [data-section-map.md](reference/data-section-map.md) | **Data section memory map (UPDATED)** | ✅ Complete |

---

## Development Guides

**Location**: [`docs/`](./)

| Document | Description | Status |
|----------|-------------|--------|
| [decompilation-guide.md](decompilation-guide.md) | How to add new functions | ✅ Complete |
| [game-watcher-usage.md](game-watcher-usage.md) | Game watcher tool usage | ✅ Complete |
| [function-pointer-patterns.md](function-pointer-patterns.md) | Function pointer analysis | ✅ Complete |

### Quick References

| Document | Description | Status |
|----------|-------------|--------|
| [PHYSICS_QUICK_REFERENCE.md](PHYSICS_QUICK_REFERENCE.md) | Copy-paste ready constants | ✅ Complete |
| [tile-collision-quick-ref.md](tile-collision-quick-ref.md) | Quick collision reference | ✅ Complete |

---

## Analysis & Research

**Location**: [`analysis/`](analysis/)

### Active Research

| Document | Description | Status |
|----------|-------------|--------|
| [unconfirmed-findings.md](analysis/unconfirmed-findings.md) | Observations awaiting verification | 🔬 Research |
| [struct-field-cleanup-2026-06-12.md](analysis/struct-field-cleanup-2026-06-12.md) | Ghidra-backed cleanup of placeholder struct field names | 🔬 Research |
| [function-batches-to-analyze.md](analysis/function-batches-to-analyze.md) | Remaining function batches | 📋 Planned |
| [password-extraction-guide.md](analysis/password-extraction-guide.md) | Password table extraction method | 📋 Planned |

## Documentation by Topic

### Understanding the Game Data

**Start Here**: [BLB Format README](blb/README.md)

**Follow-up**:
1. [Asset Types](blb/asset-types.md) - What each asset contains
2. [Level Metadata](blb/level-metadata.md) - Level entry structure
3. [TOC Format](blb/toc-format.md) - How assets are indexed

### Implementing Game Logic

**Start Here**: [Game Loop](systems/game-loop.md)

**Core Systems**:
1. [Level Loading](systems/level-loading.md) - How levels load
2. [Entities](systems/entities.md) - Game object system
3. [Player Physics](systems/player/player-physics.md) - Movement constants
4. [Collision](systems/collision-complete.md) - Tile collision
5. [Animation Framework](systems/animation-framework.md) - How sprites animate

### Reimplementing in Godot

**Physics Constants**: [reference/physics-constants.md](reference/physics-constants.md)

**Essential Systems**:
1. [Player Physics](systems/player/player-physics.md) - Movement speeds
2. [Collision](systems/collision-complete.md) - Tile attributes
3. [Camera](systems/camera.md) - Smooth scrolling
4. [Animation](systems/animation-framework.md) - 5-layer system

### Reverse Engineering Reference

**Key Tools**:
- [Decompilation Guide](decompilation-guide.md) - Adding functions
- [Game Functions](reference/game-functions.md) - Important addresses
- [Entity Types](reference/entity-types.md) - Callback table

**C Code Reference**: regenerate the Ghidra decompilation export with `make ghidra-export` (no C export is committed to the repo).

---

## Documentation Statistics

`docs/` currently holds **223 markdown files** (~60,000 lines). Approximate
distribution by area:

| Area | Files |
|------|-------|
| `systems/` (incl. enemies, player, boss-ai) | ~90 |
| `analysis/` (incl. asset-identification) | ~37 |
| `gist/asset-system/` | 32 |
| `reference/` | 22 |
| root-level (`docs/*.md`) | 24 |
| `plans/` | 7 |
| `blb/` | 6 |
| `ghidra/` | 5 |
| `architecture/` | 3 |
| `archive/sessions/` | 3 |

Per-doc status badges (✅ Complete / ⚠️ Partial / 🔬 Research / 📋 Planned)
appear in the tables above. Treat them as the author's estimate at time of
writing, not a measured figure.

### Decompilation progress (ground truth)

Derived from `symbol_addrs.txt` and the `INCLUDE_ASM` count in `src/`:

- ~2,270 function-range symbols are named in `symbol_addrs.txt`.
- **731** functions remain as `INCLUDE_ASM` stubs (not yet byte-matched to C);
  the rest are decompiled to matching C across the `src/*.c` files.

---

## Finding Information

### By Game System

**Graphics**:
- Tiles: [tiles-and-tilemaps.md](systems/tiles-and-tilemaps.md)
- Sprites: [sprites.md](systems/sprites.md)
- Animation: [animation-framework.md](systems/animation-framework.md)

**Physics**:
- Constants: [physics-constants.md](reference/physics-constants.md)
- Player: [player-physics.md](systems/player/player-physics.md)
- Collision: [collision-complete.md](systems/collision-complete.md)

**Audio**:
- Format: [audio.md](systems/audio.md)
- Functions: [audio-functions-reference.md](systems/audio-functions-reference.md)

**AI & Behaviors**:
- Enemy AI: [enemy-ai-overview.md](systems/enemy-ai-overview.md)
- Boss AI: [boss-ai/boss-behaviors.md](systems/boss-ai/boss-behaviors.md)
- Boss System: [boss-ai/boss-system-analysis.md](systems/boss-ai/boss-system-analysis.md)
- Entities: [entities.md](systems/entities.md)

### By Data Format

- **BLB Archive**: [blb/README.md](blb/README.md)
- **Assets 100-700**: [blb/asset-types.md](blb/asset-types.md)
- **Level Data**: [blb/level-metadata.md](blb/level-metadata.md)
- **Sprites**: [systems/sprites.md](systems/sprites.md)
- **Audio**: [systems/audio.md](systems/audio.md)

### By Implementation Need

**Must Have** (BLB library):
- [BLB Format](blb/README.md)
- [Asset Types](blb/asset-types.md)
- [Level Loading](systems/level-loading.md)

**Should Have** (Godot port):
- [Physics Constants](reference/physics-constants.md)
- [Collision](systems/collision-complete.md)
- [Player Physics](systems/player/player-physics.md)
- [Animation](systems/animation-framework.md)

**Nice to Have** (Polish):
- [Camera](systems/camera.md)
- [Audio](systems/audio.md)
- [Boss AI](systems/boss-ai/boss-system-analysis.md)

---

## Contributing to Documentation

### Documentation Standards

1. **File naming**: lowercase-with-hyphens.md
2. **Headers**: ATX style (`#` not underlines)
3. **Code blocks**: Specify language (```c, ```gdscript)
4. **Verification**: Mark as ✅ CODE-VERIFIED, ⚠️ ESTIMATED, or ❌ UNKNOWN
5. **Cross-references**: Link related documents

### Where to Add New Findings

- **BLB format details** → `blb/asset-types.md`
- **Game systems** → `systems/[system-name].md`
- **Constants/tables** → `reference/[topic].md`
- **Unverified research** → `analysis/unconfirmed-findings.md`
- **Function analysis** → `reference/game-functions.md`

### Updating This Index

When adding new documentation:
1. Add entry to appropriate category above
2. Update statistics section
3. Add to "Finding Information" if it's a key document
4. Update [KNOWLEDGE_GAPS.md](KNOWLEDGE_GAPS.md) if it closes a gap

---

## External Resources

- **PCSX-Redux**: Emulator with debugging support
- **Ghidra**: Decompilation tool used for analysis
- **ImHex**: Template: `docs/blb.hexpat` (BLB format)
- **Repository**: evil-engine (Godot 4.5 C99 core)

