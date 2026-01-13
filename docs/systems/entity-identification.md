# Entity Identification Guide

This document maps Skullmonkeys entity types to their in-game names and behaviors.

## Entity Type System Overview

Entities in Skullmonkeys use a two-stage type system:
1. **BLB Type**: The type stored in the BLB file's Asset 501 entity definitions
2. **Internal Type**: The remapped type used at runtime to index the callback table

The `RemapEntityTypesForLevel` function (0x8008150c) converts BLB types to internal types based on the entity's **layer**:
- **Layer 1**: Foreground/decoration entities
- **Layer 2**: Main gameplay entities (collectibles, enemies, platforms)
- **Layer 3**: Special/effect entities (bosses, flying enemies)

## Quick Reference: Common Entities

### Collectibles (Layer 2)

| BLB Type | Internal | Name | Sprite ID | Description |
|----------|----------|------|-----------|-------------|
| 2 | 7 | Clayball | 0x09406d8a | Small collectible coins |
| 3 | 2 | Ammo | - | Standard bullet pickup |
| 8 | 22 | Item | 0x0c34aa22 | Generic item (1-up, etc.) |
| 24 | 70 | SpecialAmmo | - | Special ammunition pickup |

### Ground Enemies - "Skullmonkeys" (Layer 2)

These are the clay monkey enemies that patrol levels:

| BLB Type | Internal | Name | Sprite Table | Description |
|----------|----------|------|--------------|-------------|
| 25 | 79 | **Skullmonkey (Standard)** | 0x8009da74 | The basic walking monkey enemy. Uses sprite 0x8C510186. Found in SCIE, BOIL, CRYS levels. |
| 27 | 97 | **Skullmonkey (Fast)** | 0x8009da68 | Faster-moving monkey variant. Uses sprites from table at 0x8009da68 (0x004A981C, 0x024E981C, 0x425A399C). Also in SCIE, BOIL. |
| 10 | 9 | **Skullmonkey (Patrol)** | 0x8009da50 | Patrolling monkey that walks back and forth. Uses sprites from table at 0x8009da50 (0x04280180, 0x0408C01E). |
| 13 | 25 | **Skullmonkey (Pouncer)** | - | Jumps/pounces at player. Less common variant. |

**Note**: All "monkey" enemies share similar animation systems but use different sprite sets and movement behaviors.

### Flying Enemies - "Flying Monkeys" (Layer 3)

Airborne enemies that move in flight patterns:

| BLB Type | Internal | Name | Sprite ID | Description |
|----------|----------|------|-----------|-------------|
| 28 | 5 | **Flying Monkey (Hover)** | 0x88783718 | Hovers and swoops at player. Uses same sprite as boss entities. Common in SOAR levels. |
| 8 | 6 | **Flying Monkey (Pattern)** | 0x8818a018 | Follows fixed flight pattern. Uses unique sprite set. |
| 6 (SOAR) | 6 | **Flying Hazard** | 0x8818a018 | SOAR-specific flying enemy. Layer 3, type 8 in BLB. |

**Note**: Flying monkeys in Layer 3 use BLB type 28→Internal 5, or type 8→Internal 6. The sprite 0x88783718 appears in multiple contexts (flying enemies, boss parts).

### Platforms & Objects (Layer 2)

| BLB Type | Internal | Name | Description |
|----------|----------|------|-------------|
| 28 | 89 | Platform A | Horizontal moving platform |
| 48 | 106-108 | Platform B | Vertical moving platform |
| 42 | 85 | Portal | Level exit/entrance |
| 45 | 119 | Message Box | Save point / message display |

### Bosses (Layer 3)

| BLB Type | Internal | Name | Sprite ID | Description |
|----------|----------|------|-----------|-------------|
| 48 | 1 | Boss Main | - | Boss entity (main body) |
| 51 | 19 | Boss Part | 0x88783718 | Boss sub-component |
| 50 | - | Boss Type B | 0x88783718 | Alternate boss |

### Effects (Layer 2 & 3)

| BLB Type | Internal | Name | Sprite ID | Description |
|----------|----------|------|-----------|-------------|
| 61 | 94 | Sparkle | 0x6a351094 | Decorative sparkle effect |
| 64 | - | Snow/Particle | 0x80b92212 | Weather/particle effect |
| 42-44 | 85 | Portal/Particle | - | Warp effects |

## SEVN (1970's Secret Bonus) - Special Collectibles

The SEVN level is the hidden "1970's" themed bonus area with unique entity types:

### 1970's Bonus Collectibles

| BLB Type | Internal | Description |
|----------|----------|-------------|
| 213 | 95 | **1970's Bonus A** - Retro-themed collectible |
| 214 | 95 | **1970's Bonus B** - Alternate retro collectible |
| 221 | 95 | **1970's Special Bonus** - Rare collectible |

These types 201-228 (0xC9-0xE4) are remapped to internal type 95 (0x5F) with the original BLB type stored at entity offset +0xC as variant data. This allows a single handler to differentiate between the various 1970's themed collectibles.

### SEVN Level Entity Composition (Stage 0)

From extracted data:
- 16 Clayballs (standard)
- 8 Type 213 (1970's bonus A)
- 3 Type 51 (boss parts used as decoration)
- 3 Type 41, 2 Type 42 (portals)
- 2 Type 214 (1970's bonus B)
- 1 Type 221 (special 1970's bonus)

## Entity Type Callback Table

The callback table at `0x8009d5f8` contains 121 entries, each 8 bytes:
- **Offset +0**: Flags (typically 0xFFFF0000)
- **Offset +4**: Callback function pointer

### Callback Table Organization

| Internal Type | Callback | Entity Category |
|---------------|----------|-----------------|
| 0, 3, 4 | 0x8007efd0 | Default/ammo handler |
| 1 | 0x8007f730 | Boss entity |
| 2 | 0x80080328 | Clayball |
| 5, 50 | 0x8007f7b0, 0x8007fc20 | Flying/boss sprite |
| 6, 51 | 0x8007f830, 0x8007fc9c | Flying variant |
| 7 | 0x80080408 | Item collectible |
| 8 | 0x80081504 | NULL handler (unused) |
| 9-12 | Various | Object/decoration |
| 17-21 | Various | Platform types |
| 25-29 | Various | Enemy types |
| 30-48 | Various | Platforms/objects |
| 42-44, 53-55, 60 | 0x80080ddc | Portal/particle family |
| 45 | 0x80080f1c | Message box |
| 49-51 | Various | Boss parts |
| 61 | 0x80080718 | Sparkle effect |
| 64 | 0x8007ff6c | Particle effect |

## Shared Callback Groups

Many entity types share the same callback, indicating similar behavior:

| Callback | Types | Category |
|----------|-------|----------|
| 0x8007efd0 | 0, 3, 4 | Default pickup |
| 0x80080af8 | 31, 32, 33 | Platform group A |
| 0x80080b60 | 34, 35, 36 | Platform group B |
| 0x80080bc8 | 37, 38 | Platform group C |
| 0x80080ddc | 42-44, 53-55, 60 | Portals/particles |
| 0x8007f050 | 86, 87, 88 | Foreground decor |
| 0x8007f0d0 | 106, 107, 108 | Vertical platforms |
| 0x8007f140 | 112, 113, 114 | Decoration A |
| 0x8007f1c0 | 115, 116, 117 | Decoration B |

## Entity Init Function Patterns

Entity callbacks typically follow this pattern:
```c
void EntityTypeXXX_TickCallback(void* gameState, void* entityDef) {
    // Allocate entity structure (size varies by type)
    void* entity = AllocateFromHeap(blbHeaderBufferBase, SIZE, 1, 0);
    
    // Initialize entity (sprite ID may be passed or from table)
    entity = InitFunction(entity, entityDef, spriteID, ...);
    
    // Add to game lists
    AddEntityToSortedRenderList(gameState, entity);
    AddToUpdateQueue(gameState, entity);
}
```

### Common Init Functions

| Address | Purpose | Parameters |
|---------|---------|------------|
| func_0x800560a8 | Generic sprite entity | entity, def, spriteID, param3, param4 |
| func_0x8003c5b8 | Table-based sprite | entity, def, spriteTable, idx, param4, param5 |
| func_0x8002ea3c | Enemy walker | entity, def |
| func_0x8002edc4 | Sparkle effect | entity, def |
| func_0x8003e0fc | Portal/particle | entity, def |
| func_0x80042be0 | Message box | entity, def |

## Sprite ID Tables

Several entity types use lookup tables for sprite IDs:

### Ground Enemy Sprites (0x8009da50 - 0x8009da80)

```
0x8009da50: 0x04280180 0x0408C01E 0x00000000  // Type 10 patrol
0x8009da5c: 0x004C9138 0x40489938 0x00000000  // Type 26 flying
0x8009da68: 0x004A981C 0x024E981C 0x425A399C  // Type 27 fast walker
0x8009da74: [similar pattern]                 // Type 25 standard walker
```

## Level-Specific Entity Distributions

### Science Levels (SCIE)
Primary enemies: Types 25 (walker), 27 (fast), 10 (patrol)

### Soar Levels (SOAR)
Flying enemies: Types 6, 101-103, 226
Unique vertical gameplay with airborne hazards

### Cave Levels (CAVE)
Effects: Type 61 (sparkle), 16, 35
Heavy decoration with ambient effects

### Castle Levels (CSTL)
Platforms: Types 28, 48, 29
Complex platform puzzles

### SEVN (1970's Secret Bonus)
Special types: 213, 214, 221 (unique collectibles)
Retro-themed bonus content

## Identification by Behavior

### Damages Player on Contact
- Internal types with collision mask bit 0x80 set
- Ground enemies (25, 27), flying enemies (5, 6), hazards

### Collectible on Contact
- Internal types with collision mask bit 0x02 (clayball)
- Types 2, 3, 8, 24 (collectibles)

### Interactive
- Message boxes (45) - save game trigger
- Portals (42-44) - level transition

### Decoration Only
- Layer 1 entities (no collision)
- Effects like sparkles (61), particles (64)

## Related Documentation

- [Player System](player-system.md) - How player interacts with entities
- [Entity Types Reference](../reference/entity-types.md) - Full callback table
- [BLB Data Format](../blb-data-format.md) - Entity definition structure
