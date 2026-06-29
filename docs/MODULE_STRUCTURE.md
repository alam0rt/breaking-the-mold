---
title: "Skullmonkeys Module Structure"
category: root
tags: [module, structure]
---

# Skullmonkeys Module Structure

This document describes the code module organization for the Skullmonkeys (SLES-01090) decompilation project.

## Overview

The game code is split into logical modules following the pattern established by [soul-re](https://github.com/Jeehut/soul-re). Each module represents a subsystem of the game engine.

## Module Boundaries

| Module | VRAM Start | VRAM End | ROM Offset | Size | Description |
|--------|-----------|----------|------------|------|-------------|
| `core/early` | 0x80010000 | 0x800131EF | 0x800 | 12.5 KB | RLE decode, memset entrypoint |
| `core/graphics` | 0x800131F0 | 0x8001A0C7 | 0x39F0 | 28 KB | Graphics init, VRAM slots, primitives, tilemap rendering |
| `entity/core` | 0x8001A0C8 | 0x8002A377 | 0xA8C8 | 65 KB | Entity system fundamentals |
| `entity/behaviors` | 0x8002A378 | 0x80058167 | 0x1AB78 | 182 KB | Entity type behaviors, init, tick |
| `player/main` | 0x80058168 | 0x80071047 | 0x48968 | 100 KB | Player state machine, physics |
| `level/loader` | 0x80071048 | 0x80082FFF | 0x61848 | 72 KB | Level loading, BLB, game flow |
| `LIBS` | 0x80083000 | 0x80090FEB | 0x73800 | 56 KB | PSY-Q SDK libraries (libcd, libgpu, libspu) |

## Module Details

### core/early (0x80010000 - 0x800131EF)
The earliest game code that runs after the PSX BIOS. Contains:
- `memset_entrypoint` - Initial entry point
- `DecodeRLESprite` - RLE decompression for sprites
- Memory initialization routines

### core/graphics (0x800131F0 - 0x8001A0C7)
Graphics subsystem initialization and rendering primitives:
- `GetFrameReadyFlag`, `TriggerBufferSwapIfReady` - Frame sync
- `InitGraphicsSystem`, `SetVideoModePAL` - Display init
- `AllocPrimSlot_*` - GPU primitive allocation
- `AllocateVRAMSlot`, `FreeVRAMSlot` - VRAM management
- `InitHeapConfig`, `AllocateFromHeap` - Memory heap
- `RenderTilemapLayerWithScroll` - Tilemap rendering
- `LoadSpriteFramesToVRAM` - Sprite loading

### entity/core (0x8001A0C8 - 0x8002A377)
Core entity system that all game objects use:
- `InitEntityStruct` - Entity initialization
- `GetEntityXPosition`, `GetEntityYPosition` - Position accessors
- `EntityDestructor_*` - Cleanup callbacks
- `SpawnEntity*` - Entity spawning
- Collision detection foundations
- Position/velocity management

### entity/behaviors (0x8002A378 - 0x80058167)
Entity type-specific behaviors (the largest module):
- `EntityType*_Init` - Type initialization functions
- `*TickCallback` - Per-frame update callbacks
- `HomingProjectile_*` - Projectile behaviors
- `PathFollow*` - Path-following entity logic
- Enemy AI and behaviors
- Interactive object logic

### player/main (0x80058168 - 0x80071047)
Player character state machine and physics:
- `PlayerCallback_*` - Player state callbacks
- `PlayerState_*` - State machine states
- `FinnCallback_*` - Finn vehicle callbacks
- Input handling
- Movement physics
- Animation control

### level/loader (0x80071048 - 0x80082FFF)
Level loading, BLB parsing, and game flow:
- `LoadLevelFromBLB` - Main level loader
- `SpawnPlayerAndEntities` - Level initialization
- `GameModeCallback` - Game state management
- `SaveCheckpointState`, `RestoreCheckpointEntities` - Checkpoints
- Menu system functions
- `EntityTypeXXX_Init` - Entity init dispatcher

### LIBS (0x80083000 - 0x80090FEB)
PSY-Q SDK library code (not decompiled):
- libcd - CD-ROM access
- libgpu - GPU/graphics
- libspu - Sound Processing Unit
- libetc - Misc utilities

## Migration to C Code

Following the General-Workflow from splat documentation:

1. **Current State**: All game code is in `.asm` segments
2. **Next Step**: Convert modules to `_c` segments as we decompile
3. **Process**: For each function:
   - Add to `symbol_addrs.txt` with size
   - Use `INCLUDE_ASM` macro in C file
   - Progressively replace assembly with C

### Example C file structure:
```c
// src/core/graphics.c
#include "common.h"
#include "include_asm.h"

INCLUDE_ASM("asm/pal/core/graphics", GetFrameReadyFlag);
INCLUDE_ASM("asm/pal/core/graphics", TriggerBufferSwapIfReady);

// Decompiled function:
void SetVideoModePAL(void) {
    // ... decompiled code
}
```

## Function Statistics by Module

Based on symbol analysis (named functions only):

| Module | Functions | Total Size |
|--------|-----------|------------|
| core/early | ~10 | 12 KB |
| core/graphics | ~100 | 28 KB |
| entity/core | ~130 | 65 KB |
| entity/behaviors | ~550 | 182 KB |
| player/main | ~220 | 100 KB |
| level/loader | ~150 | 72 KB |

## Rodata Considerations

The binary has interleaved .text and .rodata sections. Current rodata segments:
- `E2C`, `1F28`, `2044` - Game code rodata (jump tables, constants)
- `LIBS_1`, `LIBS_2`, `LIBS_3` - PSY-Q library rodata

These will eventually be paired with their corresponding C modules using `migrate_rodata_to_functions: true`.

## Related Documentation

- `docs/C_MIGRATION_GUIDE.md` - Step-by-step migration workflow
- `docs/SOUL_RE_ANALYSIS.md` - Analysis of soul-re project structure
- `symbol_addrs.txt` - Function symbols with addresses and sizes
