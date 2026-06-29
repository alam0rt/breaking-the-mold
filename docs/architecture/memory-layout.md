---
title: "Memory Layout Architecture"
category: architecture
tags: [architecture, memory, layout]
---

# Memory Layout Architecture

**Project**: Skullmonkeys (SLES-01090) PAL  
**Last Updated**: January 19, 2026  
**Source**: Ghidra analysis and symbol map

## Overview

The Skullmonkeys PAL binary follows standard PSY-Q SDK memory layout for PSX executables.
Understanding this layout is essential for:

1. Finding functions and data during reverse engineering
2. Understanding why data tables appear "scattered"
3. Identifying compilation unit boundaries
4. Mapping decompiled code back to original source structure

## Section Organization

### Memory Map

```
┌──────────────────────────────────────────────────────────────┐
│ 0x80000000 - 0x8000FFFF: PSX Kernel / Reserved               │
├──────────────────────────────────────────────────────────────┤
│ 0x80010000: EXE Load Address                                 │
│ ├── 0x80010000 - 0x800107FF: PSX-EXE Header (not loaded)    │
│ ├── 0x80010800: Code entry point (_start)                   │
│ │                                                            │
│ │   .text section (~570KB)                                   │
│ │   ├── Game code functions                                 │
│ │   ├── Interspersed .rodata (string literals)             │
│ │   └── PSY-Q library functions (~48KB at end)             │
│ │                                                            │
│ ├── 0x8009C270: End of .text, major gap (47KB)              │
│ │                                                            │
├──────────────────────────────────────────────────────────────┤
│ 0x8009B000: .data section (~43KB)                           │
│ │   Static initialized data:                                 │
│ │   ├── Sprite lookup tables                                │
│ │   ├── Entity callback table (121 entries)                 │
│ │   ├── Cheat code patterns                                 │
│ │   ├── GameState structure (416 bytes)                     │
│ │   └── Menu/UI data tables                                 │
│ │                                                            │
├──────────────────────────────────────────────────────────────┤
│ 0x800A5950: .sdata section (~2.9KB)                         │
│ │   Small data (≤8 bytes, GP-relative access):              │
│ │   ├── Global pointers (g_pGameState, etc.)             │
│ │   ├── Flags and counters                                  │
│ │   └── State variables                                     │
│ │                                                            │
├──────────────────────────────────────────────────────────────┤
│ 0x800A60C4: .rodata section                                  │
│ │   Read-only data:                                          │
│ │   ├── Debug menu strings                                  │
│ │   └── Constant tables                                     │
│ │                                                            │
├──────────────────────────────────────────────────────────────┤
│ 0x800A6500: .bss section (~36KB)                            │
│ │   Uninitialized globals:                                   │
│ │   ├── BLB header buffer (0x800AE3E0, 4KB)                │
│ │   ├── Scratch buffers                                     │
│ │   └── CD streaming state                                  │
│ │                                                            │
├──────────────────────────────────────────────────────────────┤
│ 0x800B0000+: Heap / Dynamic allocation                       │
│     └── Entity allocations, loaded assets                    │
├──────────────────────────────────────────────────────────────┤
│ 0x801F8000: Stack (grows downward)                           │
└──────────────────────────────────────────────────────────────┘
```

### Section Sizes

| Section | Start | End | Size | Notes |
|---------|-------|-----|------|-------|
| .text | 0x80010800 | 0x8009C270 | 573KB | Executable code |
| .data | 0x8009B000 | 0x800A5950 | 43KB | Static initialized |
| .sdata | 0x800A5950 | 0x800A60C4 | 2.9KB | GP-relative data |
| .rodata | 0x800A60C4 | 0x800A6500 | 1KB | Read-only |
| .bss | 0x800A6500 | ~0x800B0000 | 36KB | Uninitialized |

## PSY-Q Linker Behavior

### Why Data Appears "Scattered"

When examining the .data section, you'll notice that logically related tables are NOT
always contiguous. This is due to PSY-Q linker behavior:

1. **Compilation Unit Ordering**: The linker places .data from each .o file in link order
2. **Static Data Stays Local**: `static` variables stay with their compilation unit
3. **Extern Data Placement**: `extern` variables get separate placement based on access patterns
4. **Alignment Padding**: Arrays get 4-byte or 16-byte alignment, creating gaps

**Example**: Entity callback table (0x8009D5F8) appears AFTER player sprite table (0x8009C174)
even though entity code comes BEFORE player code in .text. The linker optimized for access
locality, not source file order.

### Small Data Optimization (-G8)

The `-G8` compiler flag puts data items ≤8 bytes into .sdata for faster GP-relative access:

```c
// These go to .sdata (≤8 bytes):
int g_FrameCounter;           // 4 bytes → .sdata
GameState* g_pGameState;    // 4 bytes → .sdata
u8 g_CurrentMode;             // 1 byte → .sdata

// These go to .data (>8 bytes):
GameState g_GameState;        // 416 bytes → .data
u32 g_EntityCallbacks[121];   // 484 bytes → .data
```

### Interspersed .rodata

String literals and const data are placed in .rodata, but the PSY-Q linker often
interleaves .rodata within .text at compilation unit boundaries:

```
0x80067000: PlayerState_Idle function code
0x80067A00: String literal "Player state error"  ← .rodata
0x80067A20: PlayerState_Walk function code       ← .text continues
```

This is why you see "gaps" between functions - they contain .rodata for that CU.

## Key Data Structures in Memory

### GameState (0x8009DC40)

The main game state is a 416-byte structure containing:

```
+0x00: Mode system (8 bytes)
+0x08: Layer list heads (20 bytes)
+0x1C: Entity lists (24 bytes)
+0x44: Camera state (36 bytes)
+0x68: Tile collision (16 bytes)
+0x84: LevelDataContext embedded (128 bytes)
+0x104: Render state (28 bytes)
+0x134: Checkpoint system (28 bytes)
+0x150: Pause system (20 bytes)
+0x17C: Cheat input buffer (16 bytes)
+0x199: Background color (7 bytes)
```

See `include/Game/game_state.h` for complete definition.

### Entity Callback Table (0x8009D5F8)

121 entries × 8 bytes mapping entity types to init functions:

```c
struct EntityTypeEntry {
    u16 flags;        // +0x00: Entity flags
    u16 padding;      // +0x02: Always 0xFFFF
    void* callback;   // +0x04: InitEntity_* function pointer
};
// Total: 968 bytes
```

### BLB Header Buffer (0x800AE3E0)

Loaded at runtime from GAME.BLB sector 0:

```
+0x000: Magic "BLB\0"
+0x056: Level entries (26 × 112 bytes)
+0xF31: Level count (26)
+0xF32: Movie count (13)
+0xF36: Content mode (3=level, 6=special)
+0xF92: Current level index
```

## Compilation Unit Detection

Large gaps (>512 bytes) between functions often indicate CU boundaries.
The `scripts/detect_compilation_units.py` tool analyzes these patterns.

### Major Boundaries

| Address | Gap | Likely Source |
|---------|-----|---------------|
| 0x80013000 | 2KB | gpu.c start |
| 0x80019000 | 1.5KB | entity.c start |
| 0x80020000 | 1.2KB | game_loop.c start |
| 0x80059000 | 2.5KB | player.c start |
| 0x80083000 | - | PSY-Q library start |

### PSY-Q Library Region

The address range **0x80083000 - 0x8008F000** (~48KB) contains PSY-Q SDK functions.
These show high code locality (functions grouped tightly) and standard library
calling conventions.

## Related Documentation

- [Source Structure Analysis](../reference/source-structure.md) - Inferred source files
- [Data Section Map](../reference/data-section-map.md) - Detailed .data contents
- [GameState Field Analysis](../systems/gamestate-field-analysis.md) - GameState offsets
- [Entity System](../systems/entities.md) - Entity callback table usage

## Notes

This analysis is based on binary structure, not original source code. The actual
memory layout may vary slightly between PAL/NTSC/JP versions due to different
library versions and compiler optimizations.
