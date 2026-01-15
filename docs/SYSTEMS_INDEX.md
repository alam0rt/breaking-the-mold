# Skullmonkeys Documentation Index

**Project**: Evil Engine - Skullmonkeys Reverse Engineering  
**Version**: 2.0  
**Last Updated**: January 15, 2026

This index provides quick navigation to all documentation organized by category.

---

## Quick Links

- **[Gap Analysis](GAP_ANALYSIS_CURRENT.md)** - Current documentation status (85% complete)
- **[BLB Format Overview](blb/README.md)** - Start here for file format
- **[Decompilation Guide](decompilation-guide.md)** - How to add new functions
- **[Implementation Status](../IMPLEMENTATION_STATUS.md)** - C library status

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
| [collision-complete.md](systems/collision-complete.md) | Complete tile collision reference | ✅ Complete |
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
| [enemies/README.md](systems/enemies/README.md) | **Individual enemy type documentation** | ✅ Good (10 types) |
| [boss-ai/boss-system-analysis.md](systems/boss-ai/boss-system-analysis.md) | Boss AI system architecture | ✅ Complete |
| [boss-ai/boss-behaviors.md](systems/boss-ai/boss-behaviors.md) | **Boss behaviors and attack patterns** | ✅ Good (55%) |
| [boss-ai/boss-shriney-guard.md](systems/boss-ai/boss-shriney-guard.md) | Shriney Guard (tutorial boss) | ⚠️ Estimated |
| [boss-ai/boss-glenn-yntis.md](systems/boss-ai/boss-glenn-yntis.md) | Glenn Yntis (mid-game boss) | ⚠️ Estimated |
| [boss-ai/boss-monkey-mage.md](systems/boss-ai/boss-monkey-mage.md) | Monkey Mage (late-game boss) | ⚠️ Estimated |

### Audio

| Document | Description | Status |
|----------|-------------|--------|
| [audio.md](systems/audio.md) | SPU audio sample system | ✅ Complete |
| [audio-functions-reference.md](systems/audio-functions-reference.md) | 6 audio playback functions | ✅ Complete |
| [sound-effects-reference.md](systems/sound-effects-reference.md) | Sound ID table | ⚠️ Partial |
| [sound-system.md](systems/sound-system.md) | Sound system overview | ✅ Complete |

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
| [pal-jp-comparison.md](reference/pal-jp-comparison.md) | Regional version differences | ✅ Complete |

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
| [function-batches-to-analyze.md](analysis/function-batches-to-analyze.md) | Remaining function batches | 📋 Planned |
| [password-extraction-guide.md](analysis/password-extraction-guide.md) | Password table extraction method | 📋 Planned |

### Historical Archive

**Location**: [`analysis/archive/`](analysis/archive/)

Archived research documents (superseded by current documentation):
- Gap analyses (5 historical versions)
- Password system findings (merged into systems/password-system.md)
- Physics extraction report (merged into reference/physics-constants.md)
- BLB unknown fields analysis (resolved)

---

## Deprecated Documentation

**Location**: [`deprecated/archive/`](deprecated/archive/)

Old documentation superseded by current system docs. Kept for historical reference:
- Entity system (old) → [systems/entities.md](systems/entities.md)
- Runtime behavior (old) → [systems/level-loading.md](systems/level-loading.md)
- Stage loading (old) → [systems/level-loading.md](systems/level-loading.md)
- BLB format (old) → [blb/](blb/)

---

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

**C Code Reference**: [`docs/ghidra/SLES_010.90.c`](ghidra/SLES_010.90.c) (64,363 lines)

---

## Documentation Statistics

### By Category

| Category | Files | Avg Completion |
|----------|-------|----------------|
| **BLB Format** | 5 | 98% |
| **Systems** | 28 | 85% |
| **Reference** | 8 | 88% |
| **Analysis** | 3 | 90% |
| **Guides** | 3 | 100% |

### By Status

| Status | Count | Percentage |
|--------|-------|------------|
| ✅ **Complete** (≥95%) | 35 | 73% |
| ⚠️ **Partial** (50-94%) | 10 | 21% |
| 📋 **Planned** (<50%) | 3 | 6% |

### Overall

**Total Documentation Files**: 66 active + 12 archived = 78 total  
**Overall Completion**: **90%**  
**Total Lines**: ~32,000 lines of documentation

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
4. Update [GAP_ANALYSIS_CURRENT.md](GAP_ANALYSIS_CURRENT.md) if it closes a gap

---

## External Resources

- **PCSX-Redux**: Emulator with debugging support
- **Ghidra**: Decompilation tool used for analysis
- **ImHex**: Template: `scripts/blb.hexpat` (BLB format)
- **Repository**: evil-engine (Godot 4.5 C99 core)

---

**Last Updated**: January 15, 2026  
**Maintained By**: Evil Engine Documentation Team  
**Status**: ✅ **Documentation Consolidated** (Version 2.0)

