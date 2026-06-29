---
title: "Source Code Structure Plan"
category: root
tags: [source, structure, plan]
---

# Source Code Structure Plan

**Status**: Proposed  
**Date**: January 15, 2026

## Current State

Currently, source files are organized flatly in `src/`:
- Individual function files (e.g., `GetAssetCount.c`, `GetLevelCount.c`)
- Module files with INCLUDE_ASM (e.g., `StateAccessors.c`, `BLBHeaderAccessors.c`)
- Some logical grouping exists but is inconsistent

## Desired State

Organize source to match the original game's logical architecture, making the codebase more maintainable and easier to understand.

### Proposed Directory Structure

```
src/
├── core/                          # Core engine systems
│   ├── main.c                     # Entry point, game loop @ 0x800828B0
│   ├── game_state.c              # GameState structure management @ 0x8009DC40
│   └── initialization.c          # System initialization
│
├── blb/                          # BLB file handling @ 0x8007A9B0 - 0x8007B073
│   ├── blb_header.c              # Header parsing (LoadBLBHeader @ 0x800208B0)
│   ├── blb_accessors.c           # Generic accessors (GetAssetCount, GetLevelCount)
│   ├── level_accessors.c         # Level-specific (StateAccessors.c @ 0x8007A9B0-0x8007ABCB)
│   ├── movie_accessors.c         # Movie-specific (BLBHeaderAccessors.c @ 0x8007AE14-0x8007B073)
│   ├── credits_accessors.c       # Credits-specific (CreditsAccessors.c @ 0x8007ABCC-0x8007AE13)
│   └── asset_loading.c           # LoadAssetContainer @ 0x8007B074, container parsing
│
├── cd/                           # CD-ROM interface @ 0x80038BA0 - 0x80039128
│   ├── cd_blb.c                  # CdBLB functions (read/wait/seek)
│   ├── cd_music.c                # Music streaming (Setup/Start/Stop/Update)
│   └── cd_wrapper.c              # PSY-Q SDK wrappers (CdIntToPos, CdControlF, etc.)
│
├── level/                        # Level loading & management @ 0x8007A294 - 0x8007D1D0
│   ├── level_loading.c           # InitializeAndLoadLevel @ 0x8007D1D0, main orchestrator
│   ├── level_parser.c            # func_8007A62C @ 0x8007A62C, data structure init
│   ├── playback_sequence.c       # AdvancePlaybackSequence @ 0x8007A294, SeekToLevel
│   └── level_data_context.c      # LevelDataContext management
│
├── entities/                     # Entity system @ 0x80020E1C - 0x800250C8
│   ├── entity_core.c             # Lists, Add/Remove functions
│   ├── entity_tick.c             # EntityTickLoop @ 0x80020E1C, callbacks
│   ├── entity_render.c           # RenderEntities @ 0x80020E80
│   ├── entity_collision.c        # CheckEntityCollision @ 0x800226F8, BBox overlap
│   ├── entity_spawn.c            # LoadEntities @ 0x80024DC4, SpawnPlayer, RemapTypes @ 0x8008150C
│   └── entity_types/             # Individual entity implementations
│       ├── clayball.c            # ClayballTickCallback @ 0x80056518
│       ├── klogg.c               # Klogg boss entity
│       ├── shrimp.c              # Shrimp enemy
│       ├── joe.c                 # Joe enemy (JOE_HEAD + JOE_COMPLETE)
│       └── ...                   # Other enemy/object types
│
├── player/                       # Player character system
│   ├── player_state.c            # initPlayerState @ 0x800260D0, state machine
│   ├── player_input.c            # UpdateInputState @ 0x800259D4, input handling
│   ├── player_normal.c           # Normal platforming callbacks @ 0x8006864C
│   ├── player_finn.c             # FINN vehicle mode
│   ├── player_bounce.c           # Bounce mechanics
│   └── player_physics.c          # Physics constants and helpers
│
├── rendering/                    # Graphics systems
│   ├── tiles.c                   # Tile rendering
│   ├── tilemaps.c                # Tilemap layer rendering
│   ├── sprites.c                 # Sprite system
│   ├── animation.c               # Animation framework (5-layer system)
│   └── camera.c                  # UpdateCameraPosition @ 0x80023DBC, scrolling
│
├── collision/                    # Collision detection
│   ├── tile_collision.c          # Tile attribute collision
│   └── entity_collision.c        # Entity-entity collision
│
├── sound/                        # Audio systems
│   ├── sound_setup.c             # SetupSound initialization
│   ├── sfx.c                     # Sound effects playback
│   └── music.c                   # Music management (wraps cd_music)
│
└── utils/                        # Utility functions
    ├── memory.c                  # AllocateFromHeap, memory management
    ├── checkpoint.c              # SaveCheckpoint @ 0x8007EAAC, RestoreCheckpoint @ 0x8007EAEC
    └── math.c                    # Fixed-point math, helpers
```

## Address Range to Module Mapping

Based on `symbol_addrs.txt` comments:

| Address Range | Module | Description |
|---------------|--------|-------------|
| 0x8007A9B0 - 0x8007AA27 | `blb/level_accessors.c` | Level table accessors |
| 0x8007AA28 - 0x8007ABCB | `blb/level_accessors.c` | Current level accessors |
| 0x8007ABCC - 0x8007AE13 | `blb/credits_accessors.c` | Credits/sector accessors |
| 0x8007AE14 - 0x8007B073 | `blb/movie_accessors.c` | Movie table accessors |
| 0x8007A294 - 0x8007D1D0 | `level/` | Level loading orchestration |
| 0x80038BA0 - 0x80039128 | `cd/cd_blb.c` | CD-ROM BLB operations |
| 0x80038C54 - 0x80039094 | `cd/cd_music.c` | Music streaming |
| 0x80020E1C - 0x800250C8 | `entities/` | Entity system core |
| 0x800259D4 - 0x8002615B | `player/` | Player system |
| 0x80023DBC - 0x8002413C | `rendering/camera.c` | Camera system |
| 0x80056518 - ... | `entities/entity_types/` | Individual entity types |

## Implementation Strategy

### Phase 1: Mapping Table
Create `scripts/module_map.yaml` that maps:
- Address ranges → module paths
- Function name patterns → modules
- Comment patterns in symbol_addrs.txt → modules

### Phase 2: Enhanced decompile.py
Update `scripts/decompile.py` to:
1. Read module mapping table
2. Determine target directory/file from address or function name
3. Support `--module` flag to override automatic detection
4. Group related functions into shared files instead of individual files

### Phase 3: Migration
Gradually migrate existing src/ files:
1. Create directory structure
2. Move functions by address range
3. Update Makefile/build system
4. Ensure build still matches

### Phase 4: Documentation
Update documentation to reference new structure:
- Update `symbol_addrs.txt` comments with file paths
- Update SYSTEMS_INDEX.md with source file references
- Create module READMEs explaining responsibilities

## Benefits

1. **Logical Organization**: Code grouped by system, not arbitrary file boundaries
2. **Easier Navigation**: Find code by looking at address range or system
3. **Better Understanding**: Directory structure reflects game architecture
4. **Maintainability**: Related code stays together
5. **Documentation Alignment**: Docs can reference specific modules

## Migration Considerations

### Critical Issues Discovered

1. **Binary Matching Requirement**: Any reorganization of source files MUST maintain exact binary matching (SHA1: 5a14b65cb44813bfed1ee53c6a3f4456bc230f97). Changing file paths changes linker order, which breaks matching.

2. **Splat Subdirectory Support**: Splat DOES support subdirectories in segment names (e.g., `blb/LoadBLBHeader`). However:
   - INCLUDE_ASM paths must match exactly: `INCLUDE_ASM("asm/pal/nonmatchings/blb/LoadBLBHeader", LoadBLBHeader)`
   - Object file paths in linker script will reflect the subdirectory structure
   - ALL files using INCLUDE_ASM must be updated when moving files

3. **Splat Configuration Requirements**:
   - Segment offsets MUST be integers, not hex strings
   - ❌ Wrong: `["0x161D4", "c", "UpdateInputState"]`
   - ✅ Right: `[90580, "c", "UpdateInputState"]` or `[0x161D4, c, UpdateInputState]` (YAML hex literals)
   - Python will auto-convert YAML hex literals (0x...) to integers

4. **Function Size Detection Issues**:
   - Ghidra's function size may be incorrect if control flow extends beyond function boundary
   - Example: UpdateInputState had branches to .L80025B70 which was outside its detected range
   - This creates unresolved symbols during linking
   - Always verify function boundaries before splitting

5. **ASM File Generation**:
   - After running splat, verify ASM files exist before proceeding
   - Missing ASM files indicate segment name mismatch or empty functions

### Safe Migration Strategy

Instead of wholesale directory restructuring, use this incremental approach:

**Phase 1: No File Movement** (CURRENT - SAFE)
- Keep all C files in flat `src/` directory
- Use clear, descriptive filenames that indicate module (e.g., `BLB_LoadHeader.c`, `Player_Input.c`)
- Use symbol_addrs.txt comments to group related functions

**Phase 2: Module Mapping** (DOCUMENTATION ONLY)
- Create `docs/module_map.yaml` documenting logical organization
- Map address ranges to conceptual modules
- Use this for navigation and understanding, NOT for actual file moves

**Phase 3: New Functions Only** (LOW RISK)
- Future decompiled functions can go into subdirectories
- Use decompile.py with module path: `python3 scripts/decompile.py NewFunction --module player/NewFunction`
- Old functions stay in place to maintain binary matching

**Phase 4: Post-100% Decompilation** (FUTURE)
- Only after 100% decompilation and binary match is no longer critical
- Then reorganize freely using build system that doesn't depend on link order
- Consider switching to more modern build system (ninja, CMake)

## Next Steps

1. Create module_map.yaml configuration
2. Test with a few functions to verify build still matches
3. Update decompile.py to use mapping
4. Gradually migrate existing code
5. Document the new structure
