# Entity System Analysis

> ⚠️ **DEPRECATED**: This document has been reorganized.
> See the new documentation:
> - [Entities](../systems/entities.md)
> - [Sprites](../systems/sprites.md)
> - [Rendering Order](../systems/rendering-order.md)
>
> This file is kept for reference but will not be updated.

---

## Overview

Entities in Skullmonkeys are game objects that combine:
1. **Sprite graphics** from BLB assets
2. **Behavior callbacks** stored in entity struct
3. **Position/state data** in a 0x44C byte entity struct

The `InitEntitySprite` function (PAL: `0x8001c720`, JP: `0x8001cb24`) is the central bridge connecting BLB sprite data to game logic.

## Known Entity Types

### Named/Labeled Entities (Ghidra)

| Function | Address | Sprite ID(s) | Purpose | Behavior Known |
|----------|---------|--------------|---------|----------------|
| `InitPlayerEntity` | 0x8001fcf0 | 0x21842018 | Player character (Klaymen) | Partial - init only |
| `InitBossEntity` | 0x80047fb8 | 0x181c3854, 0x8818a018, 0x244655d | Boss enemy | Partial - multi-sprite, 6 sub-entities |
| `InitMenuEntity` | 0x80076928 | 0xb8700ca1 | Menu/UI elements | Yes - menu state machine |
| `InitEntity_8c510186` | 0x80027a00 | 0x8c510186 | Unknown (UI related?) | Minimal |
| `InitEntity_168254b5` | 0x80034bb8 | 0x168254b5 | Unknown | Minimal |
| `InitEntity_a89d0ad0` | 0x80052678 | 0xa89d0ad0 | Unknown | Minimal |

### Menu System Entities (from FUN_800281a4)

`FUN_800281a4` at `0x800281a4` is a **menu factory** that creates many UI entities:

| Sprite ID | Count | Description |
|-----------|-------|-------------|
| 0xb8700ca1 | 1 | Menu UI frame |
| 0xe2f188 | ~12 | Menu item text/buttons (reused) |
| 0xa9240484 | 2 | Button elements |
| 0x88a28194 | 3 | Icon elements (in loop) |
| 0x80e85ea0 | 3 | Additional icons (in loop) |
| 0xe8628689 | 1 | Unknown UI element |
| 0x9158a0f6 | 1 | Unknown UI element |
| 0x902c0002 | 1 | Unknown UI element |

### Other Callers of InitEntitySprite

91 total callers of `InitEntitySprite` were found. Major function groups:

| Address Range | Est. Count | Category |
|---------------|------------|----------|
| 0x80027xxx-0x8002axxx | ~24 | Menu/UI entities |
| 0x80030xxx-0x80037xxx | ~5 | Unknown game entities |
| 0x80040xxx-0x80058xxx | ~10 | Game entities (enemies?) |
| 0x8006dxxx-0x80079xxx | ~15 | UI/HUD entities |
| 0x80078xxx-0x80079xxx | ~20 | Font/text rendering |

## Entity Struct Layout (0x44C bytes)

Based on decompiled code analysis:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | state_high | State machine upper word (0xffff0000) |
| 0x04 | 4 | callback_main | Main update callback (entity[1]) |
| 0x08 | 4 | state2_high | Secondary state |
| 0x0C | 4 | callback2 | Secondary callback (entity[3]) |
| 0x18 | 4 | vtable? | Method pointer table |
| 0x24 | 4 | callback3 | Tertiary callback (entity[9]) |
| 0x28 | 4 | callback4 | (entity[10]) |
| 0x2C | 4 | callback5 | (entity[11]) |
| 0x30 | 4 | callback6 | (entity[12]) |
| 0x34 | 4 | sprite_ptr | Pointer to sprite/POLY structure |
| 0x68 | 2 | x_pos | X position (short) |
| 0x6A | 2 | y_pos | Y position (short) |
| 0xF6 | 1 | visibility? | Rendering flag |
| 0xF7 | 1 | load_flag | Sprite load method selector |
| 0x100+ | ... | Extended | Entity-specific data |

## Sprite ID Format

Sprite IDs are **32-bit hashes** that identify sprites in the BLB file's sprite lookup table.

Example IDs found in code:
```
0x21842018 - Player (Klaymen)
0x181c3854 - Boss main body
0x8818a018 - Boss sub-parts
0x244655d  - Boss detail
0xb8700ca1 - Menu frame
0xe2f188   - Menu text
0xa9240484 - Button
0x88a28194 - Icon
0x8c510186 - Unknown entity
```

## Entity Lifecycle

```
1. ALLOCATION
   FUN_800143f0(blbHeaderBufferBase, size, 1, 0)
   └── Allocates memory from BLB buffer

2. INITIALIZATION  
   InitEntitySprite(entity, sprite_id, z_order, x, y, flags)
   ├── FUN_8001a0c8() - Zero/init 0x44c struct
   ├── FUN_8007bbc0() - Init GPU/rendering state
   ├── FUN_8001954c() - Init animation state
   ├── FUN_8001c980() - Entity setup
   ├── FUN_8001cdac/d024() - Load sprite from BLB
   └── FUN_8001d080() - Finalize sprite setup

3. CALLBACK ASSIGNMENT
   entity[1] = update_callback;  // Main tick function
   entity[3] = secondary_callback;
   entity[10] = collision_callback?
   entity[12] = destroy_callback?

4. REGISTRATION
   FUN_800213a8(GameState, entity) - Add to entity list
   FUN_80021190(GameState, entity) - Add to update queue?

5. TICK LOOP
   EntityTickLoop (0x80020e1c) - Called each frame
   └── Iterates entities, calls entity[1] callback

6. DESTRUCTION
   (Not yet analyzed)
```

## Entity Spawn System (BLB Layers 8-11)

See `docs/blb-data-format.md` for details on how entity spawn positions are encoded in BLB tilemaps.

**Summary:**
- Layers 8-11 contain entity spawn markers
- Tile index > tileset size indicates entity tile
- Connected regions of entity tiles = one entity instance
- Entity type identified by tile ID set

## Unknown/TODO

### Behavior Not Yet Analyzed
- Player movement/physics
- Enemy AI patterns
- Collectible pickup logic
- Collision detection
- Damage/health system
- Level transition triggers
- Save/load state

### Entities Not Yet Identified
- Clay balls (collectibles)
- 1-UP lives
- Health pickups
- Moving platforms
- Hazards (spikes, pits)
- Checkpoints
- Level exits
- World map nodes

### Key Functions to Analyze
- `EntityTickLoop` (0x80020e1c) - Main entity update loop
- Entity callback functions (LAB_xxxxx addresses)
- Collision detection routines
- Entity-entity interaction

## Cross-Reference: PAL vs JP

| Entity Function | PAL Address | JP Address | Delta |
|-----------------|-------------|------------|-------|
| InitEntitySprite | 0x8001c720 | 0x8001cb24 | +0x404 |
| InitPlayerEntity | 0x8001fcf0 | ~0x800200f4? | +0x404 |
| InitBossEntity | 0x80047fb8 | ~0x800483bc? | +0x404 |
| EntityTickLoop | 0x80020e1c | ~0x80021220? | +0x404 |

(JP addresses are estimates based on +0x404 delta pattern)

## Verification Status

| Item | Status | Method |
|------|--------|--------|
| Entity struct size (0x44C) | ✓ Verified | Ghidra analysis |
| Sprite ID format | ✓ Verified | Binary search + decompilation |
| InitEntitySprite function | ✓ Verified | Full decompilation |
| Entity spawn layers | ⚠️ Partial | Tilemap extraction |
| Entity behavior callbacks | ⚠️ Minimal | Only init code analyzed |
| Collision system | ✗ Unknown | Not yet analyzed |
| Entity type identification | ⚠️ Partial | Sprite IDs known, not all named |
