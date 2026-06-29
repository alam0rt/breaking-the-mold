---
title: "Original Source Code Structure Analysis"
category: reference
tags: [reference, source, structure]
---

# Original Source Code Structure Analysis

**Project**: Skullmonkeys (SLES-01090) PAL  
**Last Updated**: January 19, 2026  
**Method**: Compilation unit boundary detection via function gaps, rodata transitions, naming patterns

## Overview

Analysis of the binary's memory layout reveals approximately **30-40 original source files**
(compilation units). This is inferred from:

1. **Large function gaps** (>512 bytes) indicating .rodata/.data sections between CUs
2. **Naming pattern transitions** (e.g., Player_* → Entity_* → Boss_*)
3. **Section boundary markers** (.text → .data at 0x8009C270)
4. **PSY-Q library clustering** (high locality in 0x80083000-0x8008F000 range)

## Memory Section Organization

| Section | Start | End | Size | Notes |
|---------|-------|-----|------|-------|
| .text | 0x80010800 | 0x8009C270 | ~570KB | Executable code |
| .rodata | interspersed | - | ~15KB | Read-only data within .text |
| .data | 0x8009B000 | 0x800A5950 | ~43KB | Static data, tables |
| .sdata | 0x800A5950 | 0x800A60C4 | ~2.9KB | Small data (GP-relative, -G8) |
| .bss | 0x800A60C4 | ~0x800AE400 | ~36KB | Uninitialized globals |

### Major Section Boundary

The largest gap (47,800 bytes) at **0x8009C270** marks the end of .text and transition
to .data section. This is where the linker placed all accumulated .data from compilation units.

## Compilation Unit Detection

Script: `scripts/detect_compilation_units.py`

### Method

1. **Function gap analysis**: Identify large gaps (>512 bytes) between consecutive functions
2. **Rodata transitions**: Detect transitions from code to read-only data patterns
3. **Naming conventions**: Group functions by naming prefixes (Init*, Entity*, Player*, etc.)
4. **Library code detection**: Identify PSY-Q library boundaries via register usage patterns

### Major Boundaries Detected

Functions with >512 byte gaps before them (likely CU boundaries):

| Address | Gap Size | Function Name | Likely CU |
|---------|----------|---------------|-----------|
| 0x80010800 | - | _start | crt0.s |
| 0x80013000 | ~2KB | GPU_Init | gpu.c |
| 0x80019000 | ~1.5KB | Entity_Alloc | entity.c |
| 0x8001C000 | ~1KB | Sprite_Init | sprite.c |
| 0x80020000 | ~1.2KB | GameLoop_Main | game_loop.c |
| 0x80024000 | ~800 | Tile_Init | tiles.c |
| 0x80038000 | ~2KB | CD_Read | cd_stream.c |
| 0x80044000 | ~1.5KB | Boss_Init | boss.c |
| 0x80059000 | ~2.5KB | Player_Init | player.c |
| 0x80067000 | ~1KB | PlayerState_Idle | player_states.c |
| 0x80078000 | ~1.8KB | Menu_Init | menu.c |
| 0x8007A000 | ~1KB | Level_Load | level.c |
| 0x8007E000 | ~1.2KB | GameMode_Callback | game_mode.c |
| 0x80082000 | ~900 | Cheat_Check | cheats.c |
| 0x80083000 | ~1KB | LIBSN_start | PSY-Q libc |

### PSY-Q Library Code

High locality cluster at **0x80083000 - 0x8008F000** (~48KB):

This region contains PSY-Q SDK functions with characteristics:
- No custom naming patterns
- Standard library calling conventions
- Consistent register usage across functions

Likely includes: libc, libgpu, libgte, libspu portions

## Inferred Source File Structure

Based on boundary analysis and function naming:

```
src/
├── main.c                    # Entry point, main loop
├── crt0.s                    # Startup assembly
├── game/
│   ├── game_loop.c           # Main game loop, frame tick
│   ├── game_mode.c           # Mode callbacks, level transitions
│   ├── game_state.c          # GameState init/management
│   └── level.c               # Level loading, BLB access
├── entity/
│   ├── entity.c              # Entity allocation, lists
│   ├── entity_tick.c         # Entity update loop
│   ├── entity_spawn.c        # Spawn system (Asset 501)
│   └── entity_collision.c    # Collision detection
├── player/
│   ├── player.c              # Player entity init
│   ├── player_states.c       # State machine callbacks
│   ├── player_physics.c      # Movement physics
│   └── player_animation.c    # Animation control
├── enemy/
│   ├── enemy_common.c        # Shared enemy behaviors
│   └── enemy_types.c         # Per-type init functions
├── boss/
│   ├── boss.c                # Boss system base
│   └── boss_*.c              # Per-boss implementations
├── gfx/
│   ├── sprite.c              # Sprite loading, rendering
│   ├── tiles.c               # Tile graphics
│   └── gpu.c                 # GPU primitive management
├── ui/
│   ├── menu.c                # Menu system
│   ├── hud.c                 # HUD rendering
│   └── password.c            # Password entry
├── audio/
│   ├── sound.c               # Sound effect playback
│   └── music.c               # CD audio streaming
├── cd/
│   └── cd_stream.c           # CD sector streaming
└── sys/
    ├── input.c               # Controller input
    ├── cheats.c              # Cheat code detection
    └── memory.c              # Heap management
```

## Data Table Scattering Explanation

The .data section shows "scattered" tables that aren't contiguous because:

1. **Linker locality optimization**: PSY-Q linker groups data by compilation unit, then by access patterns
2. **Static vs extern**: `static` data stays in CU's .data, `extern` data gets global placement
3. **Small data threshold**: `-G8` flag moves data ≤8 bytes to .sdata for faster GP-relative access
4. **Array alignment**: Large arrays get 16-byte alignment, creating padding between smaller tables

### Example: Entity Callback Table Placement

The entity callback table at 0x8009D5F8 is placed AFTER player data tables (0x8009C174)
even though entity code comes BEFORE player code. This is because:

1. Entity callbacks are initialized late in startup
2. Player sprite tables are static and accessed earlier
3. Linker optimizes for access pattern, not source order

## Verification

To verify compilation unit boundaries:

1. **Check for .rodata between functions**: Use `xxd` to look for string literals
2. **Function calling patterns**: Functions within same CU have direct calls
3. **Symbol naming**: Original source likely used consistent prefixes per file

## Related Documentation

- [Data Section Map](data-section-map.md) - Detailed .data layout
- [Game Functions Reference](game-functions.md) - Function addresses by category
- [Decompilation Guide](../decompilation-guide.md) - Workflow for adding functions

## Notes

This analysis is **inferred** from binary structure, not from original source code.
Actual file names and organization may differ. The boundaries are useful for:

- Understanding code organization when decompiling
- Grouping related functions in documentation
- Identifying which functions likely share static data
