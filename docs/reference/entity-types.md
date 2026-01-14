# Entity Types Reference

Comprehensive reference for entity types in Skullmonkeys (PAL SLES-01090).

## Overview

Entity types are 16-bit IDs stored in Asset 501 (entity placement data). The game uses a **callback table** to dispatch entity initialization based on internal type IDs.

**Key concepts:**
- **BLB Type**: The entity_type field in Asset 501 data (0-255+)
- **Internal Type**: Remapped type (0-120) used to index callback table
- **Callback Table**: 121 entries at `g_EntityTypeCallbackTable` (0x8009d5f8)

## Entity Type Callback Table

Located at `g_EntityTypeCallbackTable` (0x8009d5f8), this table contains 121 entries of 8 bytes each.

### Table Structure

```c
struct EntityTypeEntry {
    u32 flags;       // Usually 0xFFFF0000, 0x00000000 for unused
    u32 callback;    // Function pointer to init/tick handler
};
```

### Full Callback Table (121 entries)

Extracted from ROM at 0x8009d5f8:

| Type | Flags | Callback | Function Name |
|------|-------|----------|---------------|
| 0 | 0xFFFF0000 | 0x8007efd0 | EntityCallback_Type00 |
| 1 | 0xFFFF0000 | 0x8007f730 | EntityCallback_Type01 |
| 2 | 0xFFFF0000 | 0x80080328 | EntityCallback_Type02 (Clayball) |
| 3 | 0xFFFF0000 | 0x8007efd0 | EntityCallback_Type03 |
| 4 | 0xFFFF0000 | 0x8007efd0 | EntityCallback_Type04 |
| 5 | 0xFFFF0000 | 0x8007f7b0 | EntityCallback_Type05 |
| 6 | 0xFFFF0000 | 0x8007f830 | EntityCallback_Type06 |
| 7 | 0xFFFF0000 | 0x80080408 | EntityCallback_Type07 |
| 8 | 0xFFFF0000 | 0x80081504 | EntityCallback_Type08 (Item) |
| 9 | 0xFFFF0000 | 0x800804e8 | EntityCallback_Type09 |
| 10 | 0xFFFF0000 | 0x8007f244 | EntityCallback_Type10 |
| 11 | 0xFFFF0000 | 0x80080478 | EntityCallback_Type11 |
| 12 | 0xFFFF0000 | 0x8007f8b0 | EntityCallback_Type12 |
| 13 | 0x00000000 | 0x00000000 | (unused) |
| 14 | 0x00000000 | 0x00000000 | (unused) |
| 15 | 0x00000000 | 0x00000000 | (unused) |
| 16 | 0x00000000 | 0x00000000 | (unused) |
| 17 | 0xFFFF0000 | 0x8007f930 | EntityCallback_Type17 |
| 18 | 0xFFFF0000 | 0x8007f9b0 | EntityCallback_Type18 |
| 19 | 0xFFFF0000 | 0x8007fa30 | EntityCallback_Type19 |
| 20 | 0xFFFF0000 | 0x8007faac | EntityCallback_Type20 |
| 21 | 0xFFFF0000 | 0x8007fb28 | EntityCallback_Type21 |
| 22 | 0xFFFF0000 | 0x80080398 | EntityCallback_Type22 |
| 23 | 0xFFFF0000 | 0x80080558 | EntityCallback_Type23 |
| 24 | 0xFFFF0000 | 0x8007f460 | EntityCallback_Type24 (SpecialAmmo) |
| 25 | 0xFFFF0000 | 0x800805c8 | EntityCallback_Type25 (EnemyA) |
| 26 | 0xFFFF0000 | 0x8007f2cc | EntityCallback_Type26 |
| 27 | 0xFFFF0000 | 0x8007f354 | EntityCallback_Type27 (EnemyB) |
| 28 | 0xFFFF0000 | 0x80080638 | EntityCallback_Type28 (PlatformA) |
| 29 | 0xFFFF0000 | 0x800806a8 | EntityCallback_Type29 |
| 30 | 0xFFFF0000 | 0x80080a98 | EntityCallback_Type30 |
| 31 | 0xFFFF0000 | 0x80080af8 | EntityCallback_Type31 |
| 32 | 0xFFFF0000 | 0x80080af8 | EntityCallback_Type32 |
| 33 | 0xFFFF0000 | 0x80080af8 | EntityCallback_Type33 |
| 34 | 0xFFFF0000 | 0x80080b60 | EntityCallback_Type34 |
| 35 | 0xFFFF0000 | 0x80080b60 | EntityCallback_Type35 |
| 36 | 0xFFFF0000 | 0x80080b60 | EntityCallback_Type36 |
| 37 | 0xFFFF0000 | 0x80080bc8 | EntityCallback_Type37 |
| 38 | 0xFFFF0000 | 0x80080bc8 | EntityCallback_Type38 |
| 39 | 0xFFFF0000 | 0x80080c8c | EntityCallback_Type39 |
| 40 | 0xFFFF0000 | 0x80080cfc | EntityCallback_Type40 |
| 41 | 0xFFFF0000 | 0x80080d6c | EntityCallback_Type41 |
| 42 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type42 (Portal) |
| 43 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type43 |
| 44 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type44 |
| 45 | 0xFFFF0000 | 0x80080f1c | EntityCallback_Type45 (Message) |
| 46 | 0xFFFF0000 | 0x80080c2c | EntityCallback_Type46 |
| 47 | 0xFFFF0000 | 0x80080e4c | EntityCallback_Type47 |
| 48 | 0xFFFF0000 | 0x80080e4c | EntityCallback_Type48 (PlatformB) |
| 49 | 0xFFFF0000 | 0x8007fba4 | EntityCallback_Type49 |
| 50 | 0xFFFF0000 | 0x8007fc20 | EntityCallback_Type50 (Boss) |
| 51 | 0xFFFF0000 | 0x8007fc9c | EntityCallback_Type51 (BossPart) |
| 52 | 0xFFFF0000 | 0x80080c8c | EntityCallback_Type52 |
| 53 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type53 |
| 54 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type54 |
| 55 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type55 |
| 56 | 0x00000000 | 0x00000000 | (unused) |
| 57 | 0xFFFF0000 | 0x8007fd18 | EntityCallback_Type57 |
| 58 | 0xFFFF0000 | 0x8007fd94 | EntityCallback_Type58 |
| 59 | 0xFFFF0000 | 0x8007fe10 | EntityCallback_Type59 |
| 60 | 0xFFFF0000 | 0x80080ddc | EntityCallback_Type60 (Particle) |
| 61 | 0xFFFF0000 | 0x80080718 | EntityCallback_Type61 (Sparkle) |
| 62 | 0xFFFF0000 | 0x8007fe8c | EntityCallback_Type62 |
| 63 | 0xFFFF0000 | 0x8007fefc | EntityCallback_Type63 |
| 64 | 0xFFFF0000 | 0x8007ff6c | EntityCallback_Type64 |
| 65 | 0xFFFF0000 | 0x80080f8c | EntityCallback_Type65 |
| 66 | 0xFFFF0000 | 0x8007ffdc | EntityCallback_Type66 |
| 67 | 0xFFFF0000 | 0x80080050 | EntityCallback_Type67 |
| 68 | 0xFFFF0000 | 0x800800c4 | EntityCallback_Type68 |
| 69 | 0xFFFF0000 | 0x80080788 | EntityCallback_Type69 |
| 70 | 0xFFFF0000 | 0x800807f8 | EntityCallback_Type70 |
| 71 | 0xFFFF0000 | 0x80080fec | EntityCallback_Type71 |
| 72 | 0xFFFF0000 | 0x80080868 | EntityCallback_Type72 |
| 73 | 0x00000000 | 0x00000000 | (unused) |
| 74 | 0x00000000 | 0x00000000 | (unused) |
| 75 | 0xFFFF0000 | 0x800808d8 | EntityCallback_Type75 |
| 76 | 0xFFFF0000 | 0x8007f3dc | EntityCallback_Type76 |
| 77 | 0x00000000 | 0x00000000 | (unused) |
| 78 | 0x00000000 | 0x00000000 | (unused) |
| 79 | 0xFFFF0000 | 0x8008121c | EntityCallback_Type79 |
| 80 | 0xFFFF0000 | 0x80080ebc | EntityCallback_Type80 |
| 81 | 0xFFFF0000 | 0x80080948 | EntityCallback_Type81 |
| 82 | 0xFFFF0000 | 0x8008127c | EntityCallback_Type82 |
| 83 | 0xFFFF0000 | 0x800809b8 | EntityCallback_Type83 |
| 84 | 0xFFFF0000 | 0x8007f5b0 | EntityCallback_Type84 |
| 85 | 0xFFFF0000 | 0x800812ec | EntityCallback_Type85 |
| 86 | 0xFFFF0000 | 0x8007f050 | EntityCallback_Type86 |
| 87 | 0xFFFF0000 | 0x8007f050 | EntityCallback_Type87 |
| 88 | 0xFFFF0000 | 0x8007f050 | EntityCallback_Type88 |
| 89 | 0xFFFF0000 | 0x8008134c | EntityCallback_Type89 |
| 90 | 0xFFFF0000 | 0x80080138 | EntityCallback_Type90 |
| 91 | 0xFFFF0000 | 0x800801b4 | EntityCallback_Type91 |
| 92 | 0xFFFF0000 | 0x80080230 | EntityCallback_Type92 |
| 93 | 0xFFFF0000 | 0x800802ac | EntityCallback_Type93 |
| 94 | 0xFFFF0000 | 0x80081428 | EntityCallback_Type94 |
| 95 | 0xFFFF0000 | 0x800814a4 | EntityCallback_Type95 |
| 96 | 0xFFFF0000 | 0x8007f638 | EntityCallback_Type96 |
| 97 | 0xFFFF0000 | 0x8008134c | EntityCallback_Type97 |
| 98 | 0xFFFF0000 | 0x8008134c | EntityCallback_Type98 |
| 99 | 0xFFFF0000 | 0x8007f4d0 | EntityCallback_Type99 |
| 100 | 0xFFFF0000 | 0x8008105c | EntityCallback_Type100 |
| 101 | 0xFFFF0000 | 0x800810cc | EntityCallback_Type101 |
| 102 | 0xFFFF0000 | 0x8008113c | EntityCallback_Type102 |
| 103 | 0xFFFF0000 | 0x800811ac | EntityCallback_Type103 |
| 104 | 0xFFFF0000 | 0x800812ec | EntityCallback_Type104 |
| 105 | 0xFFFF0000 | 0x800812ec | EntityCallback_Type105 |
| 106 | 0xFFFF0000 | 0x8007f0d0 | EntityCallback_Type106 |
| 107 | 0xFFFF0000 | 0x8007f0d0 | EntityCallback_Type107 |
| 108 | 0xFFFF0000 | 0x8007f0d0 | EntityCallback_Type108 |
| 109 | 0xFFFF0000 | 0x8007f540 | EntityCallback_Type109 |
| 110 | 0xFFFF0000 | 0x8008134c | EntityCallback_Type110 |
| 111 | 0xFFFF0000 | 0x8008134c | EntityCallback_Type111 |
| 112 | 0xFFFF0000 | 0x8007f140 | EntityCallback_Type112 |
| 113 | 0xFFFF0000 | 0x8007f140 | EntityCallback_Type113 |
| 114 | 0xFFFF0000 | 0x8007f140 | EntityCallback_Type114 |
| 115 | 0xFFFF0000 | 0x8007f1c0 | EntityCallback_Type115 |
| 116 | 0xFFFF0000 | 0x8007f1c0 | EntityCallback_Type116 |
| 117 | 0xFFFF0000 | 0x8007f1c0 | EntityCallback_Type117 |
| 118 | 0xFFFF0000 | 0x8007f460 | EntityCallback_Type118 |
| 119 | 0xFFFF0000 | 0x80080a28 | EntityCallback_Type119 |
| 120 | 0xFFFF0000 | 0x8007f6c0 | EntityCallback_Type120 |

### Shared Callbacks

Several entity types share the same callback function:

| Callback | Types | Count | Notes |
|----------|-------|-------|-------|
| 0x8007efd0 | 0, 3, 4 | 3 | Default handler |
| 0x8007f050 | 86, 87, 88 | 3 | |
| 0x8007f0d0 | 106, 107, 108 | 3 | |
| 0x8007f140 | 112, 113, 114 | 3 | |
| 0x8007f1c0 | 115, 116, 117 | 3 | |
| 0x80080af8 | 31, 32, 33 | 3 | |
| 0x80080b60 | 34, 35, 36 | 3 | |
| 0x80080bc8 | 37, 38 | 2 | |
| 0x80080ddc | 42, 43, 44, 53, 54, 55, 60 | 7 | Portal/Particle family |
| 0x8008134c | 89, 97, 98, 110, 111 | 5 | |
| 0x800812ec | 85, 104, 105 | 3 | |

## Known Entity Types

Entity types observed in extracted BLB data. BLB types are remapped by `RemapEntityTypesForLevel` (0x8008150c) before indexing the callback table.

### Collectibles

| BLB Type | Name | Count | Description | Sprite ID |
|----------|------|-------|-------------|-----------|
| 2 | Clayball | 5,727 | Collectible coins | 0x09406d8a |
| 3 | Ammo | 308 | Standard bullet pickup | - |
| 8 | Item | 144 | Generic item pickup | 0x0c34aa22 |
| 24 | SpecialAmmo | 227 | Special ammunition | - |

### Enemies

| BLB Type | Name | Count | Description | Sprite ID |
|----------|------|-------|-------------|-----------|
| 25 | EnemyA | 152 | Enemy type 1 | 0x1e1000b3 |
| 27 | EnemyB | 60 | Enemy type 2 | 0x182d840c |

### Platforms & Objects

| BLB Type | Name | Count | Description | Sprite ID |
|----------|------|-------|-------------|-----------|
| 10 | Object | - | Large interactive object | - |
| 28 | PlatformA | 99 | Moving platform type 1 | - |
| 48 | PlatformB | 297 | Moving platform type 2 | - |

### Interactive

| BLB Type | Name | Count | Description | Sprite ID |
|----------|------|-------|-------------|-----------|
| 42 | Portal | - | Portal/warp point | 0xb01c25f0 |
| 45 | Message | - | Message/save box | 0xa89d0ad0 |

### Boss Entities

| BLB Type | Name | Description | Sprite ID |
|----------|------|-------------|-----------|
| 50 | Boss | Boss main entity | 0x181c3854 |
| 51 | BossPart | Boss sub-entity/part | 0x8818a018 |

### Effects

| BLB Type | Name | Description | Sprite ID |
|----------|------|-------------|-----------|
| 60 | Particle | Particle effect | 0x168254b5 |
| 61 | Sparkle | Sparkle effect | 0x6a351094 |

### Unknown Types

Types observed in extracted data but not yet identified:

| BLB Type | Observed In | Count | Notes |
|----------|-------------|-------|-------|
| 9 | CSTL, MOSS | - | Uses extended layer flags |
| 11 | MOSS | 1 | |
| 12 | MOSS | 1 | |
| 13 | BOIL | 4 | |
| 15 | MOSS | 1 | |
| 16 | BOIL, MOSS | 13 | |
| 22 | MOSS | 1 | |
| 23 | MOSS | 3 | |
| 41 | MOSS | 2 | |
| 64 | MOSS | 6 | |
| 81 | MOSS | 1 | Uses extended layer flags |
| 82 | BOIL | 36 | |
| 103 | - | - | |
| 215 | BOIL | 17 | High type number, may be special |

## Type Remapping

`RemapEntityTypesForLevel` (0x8008150c) converts BLB entity types to internal types (0-120) before dispatching to the callback table.

The remapping logic depends on level flags and entity variant fields. Further analysis is needed to document the full mapping.

## Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x8008150c | RemapEntityTypesForLevel | Convert BLB types → internal types |
| 0x80024dc4 | LoadEntitiesFromAsset501 | Load 24-byte entity defs |
| 0x8007df38 | SpawnPlayerAndEntities | Create player/entities from defs |
| 0x80020e1c | EntityTickLoop | Per-frame entity updates |

## Sprite ID Lookup

Entity type → sprite ID mapping is **HARDCODED** in init functions, not in BLB data.

Sprite IDs are 32-bit hashes that index into the sprite TOC in the level's secondary/tertiary containers.

### Player Sprites

Player uses a lookup table at `g_PlayerSpriteTable` (0x8009c174):

```
Index  Sprite ID    Description
0      0x08208902   Idle/stand
1      0x48204012   Walk
2      0x8569a090   Jump
3      0x0708a4a0   Fall
...
```

See [Game Functions](game-functions.md) for full player sprite table.

## Clayball Collision System

The clayball (type 2) collision system demonstrates how entity-to-player collision works in Skullmonkeys.

### Collision Flow

```
┌─────────────────────────────────────────────────────────────────────────┐
│                   CLAYBALL TICK (ClayballTickCallback)                  │
│                                                                         │
│  1. Check entity+0x110 flag:                                            │
│     - If set: Call CollisionCheckWrapper(clayball, 2, 0x1000, 1)       │
│     - If entity+0x111 set: Call CollisionCheckWrapper(clayball, 2, 0x1007, val) │
│                                                                         │
│  2. CollisionCheckWrapper wraps CheckEntityCollision:                  │
│     - Passes clayball bounding box (entity+0x48/0x4c)                   │
│     - Type mask = 2 (identifies as clayball)                            │
│     - Message = 0x1000 or 0x1007                                        │
│                                                                         │
│  3. CheckEntityCollision (0x800226f8) special case:                     │
│     - If type_mask == 2: Check player at GameState+0x2c directly       │
│     - Uses CheckBBoxOverlap @ 0x8001b3f0 for bounding box test         │
│     - If overlap: Invoke player's state callback                        │
│                                                                         │
│  4. After collision check returns:                                      │
│     - If collision detected (FUN_8002453c returns true):                │
│       a. Clear clayball+0x100+0x16 collision flag                       │
│       b. Clear clayball+0x100 pointer                                   │
│       c. Call GameState callback with message 3 (COLLECTED)             │
│       d. If clayball+0x11c exists: send message 0x1009                  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x80056518 | ClayballTickCallback | Clayball tick callback |
| 0x800561d4 | ClayballInitCallback | Clayball init callback |
| 0x8001b47c | CollisionCheckWrapper | Collision check wrapper |
| 0x800226f8 | CheckEntityCollision | Main collision detection |
| 0x8001b3f0 | CheckBBoxOverlap | Bounding box overlap test |

### Collision Messages

| Message | Hex | Purpose |
|---------|-----|---------|
| CLAYBALL_COLLECT | 0x1000 | Standard clayball collected |
| CLAYBALL_VARIANT | 0x1007 | Special clayball variant |
| COLLECTED | 3 | Notify GameState of collection |
| NOTIFY_LINKED | 0x1009 | Notify linked entity |

### Entity Offsets Used

| Offset | Type | Purpose |
|--------|------|---------|
| +0x100 | ptr | Collision target entity (player) |
| +0x110 | u8 | Collision enabled flag |
| +0x111 | u8 | Variant collision flag |
| +0x114 | u32 | Variant collision data |
| +0x11c | ptr | Linked entity to notify |
| +0x48/+0x4c | u32 | Bounding box (x1,y1,x2,y2) |

### GameState Entity Pointers

| Offset | Purpose |
|--------|---------|
| +0x2c | Player entity (used for type==2 collision) |
| +0x24 | Collision queue list head |
| +0x30 | Main player entity reference |

### Notes

- The collision system uses a special fast path for clayballs (type mask = 2)
- Instead of iterating the collision queue, it directly checks the player entity
- This is an optimization since clayballs only ever need to collide with the player
- Collection handling occurs via GameState callback mechanism (message 3) which dispatches through GameState+0xC callback table
- See [Items Reference](items.md) for complete collection system documentation

## Related Documentation

- [Entity System](../systems/entities.md) - Entity lifecycle and structure
- [Game Functions](game-functions.md) - Function addresses and globals
- [BLB Asset Handling](../blb-asset-handling.md) - Asset 501 format details
