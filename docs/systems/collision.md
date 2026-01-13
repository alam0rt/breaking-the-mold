# Tile Collision System

This document describes the tile collision attribute system in Skullmonkeys.

## Overview

Each tile has a 1-byte collision attribute stored in Asset 500 (tile_attributes). The level dimensions determine total tile count: `width × height` tiles.

**Accessor Function**: `GetTileAttributeAtPosition` @ 0x800241f4

## Known Collision Attributes

Based on extracted data analysis and observed gameplay:

### Core Collision Types

| Value | Hex | Name | Description | Observed In |
|-------|-----|------|-------------|-------------|
| 0 | 0x00 | Empty | No collision, pass-through | All levels (~90-96%) |
| 2 | 0x02 | Solid | Full collision block | All levels |
| 83 | 0x53 | Checkpoint | Save point trigger | SCIE, CAVE, etc. |
| 101 | 0x65 | Spawn Zone | Entity spawn/activity area | All levels |

### Platform Types (Unverified)

| Value | Hex | Name | Theory | Notes |
|-------|-----|------|--------|-------|
| 9 | 0x09 | Platform? | One-way platform (jump through, land on top) | CLOU |
| 18 | 0x12 | Trigger | Level trigger/event zone | SCIE |
| 19 | 0x13 | Trigger B | Alternate trigger | CAVE |
| 23 | 0x17 | Unknown | Rare, possibly special interaction | SCIE |
| 33 | 0x21 | Unknown | Very rare | SCIE |
| 34 | 0x22 | Unknown | Very rare | SCIE |

### CAVE-Specific (Vertical Level)

| Value | Hex | Name | Theory | Count |
|-------|-----|------|--------|-------|
| 91 | 0x5B | Platform/Slide? | Special platform behavior | 27 |
| 181 | 0xB5 | Slope Left? | Angled collision | 37 |
| 182 | 0xB6 | Slope Right? | Angled collision | 24 |
| 183 | 0xB7 | Slope Steep? | Steeper angle | 6 |
| 201 | 0xC9 | Hazard? | Damage zone | 8 |
| 221 | 0xDD | Water/Lava? | Liquid hazard | 3 |
| 222 | 0xDE | Liquid Zone? | Water/lava area | 141 |

### CLOU-Specific (Cloud Level)

| Value | Hex | Name | Count | Notes |
|-------|-----|------|-------|-------|
| 91 | 0x5B | Cloud Platform | 357 | Jump-through platforms |

## Bit Analysis (Hypothetical)

The collision byte may encode multiple properties:

```
Bit 0 (0x01): Solid floor?
Bit 1 (0x02): Solid wall/ceiling?
Bit 2 (0x04): One-way platform?
Bit 3 (0x08): Trigger zone?
Bit 4 (0x10): Hazard/damage?
Bit 5 (0x20): Slope modifier?
Bit 6 (0x40): Entity interaction?
Bit 7 (0x80): Special/level-specific?
```

**Note**: This is speculative. Actual bit meanings need verification through decompilation.

## Gameplay Behaviors to Document

### Solid Tiles (0x02)
- Block player from all directions
- Used for floors, walls, ceilings
- Most common non-empty attribute

### One-Way Platforms (0x5B in CLOU?)
- Player can jump up through from below
- Player lands on top and walks normally
- Common in platforming sections

### Slopes (0xB5-0xB7 in CAVE?)
- Angled collision surfaces
- Player walks up/down smoothly
- May affect player speed

### Checkpoints (0x53)
- Trigger save point when touched
- Respawn location after death
- Visual indicator (message box entity nearby)

### Hazards (0xC9, 0xDD, 0xDE?)
- Damage player on contact
- May be instant-kill or gradual damage
- Lava, spikes, etc.

## Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800241f4 | GetTileAttributeAtPosition | Get collision byte at tile coords |
| 0x80024cf4 | InitTileAttributeState | Initialize collision grid |
| 0x8005a914 | PlayerProcessTileCollision | Process player-tile collision |
| 0x80059bc8 | CheckWallCollision | Check for wall collision |

## Level-Specific Observations

### SCIE (Science Lab)
- Simple horizontal level
- Mostly 0x00 (empty) and 0x02 (solid)
- Few special tiles

### CAVE (Vertical Descent)
- Many slope tiles (0xB5-0xB7)
- Liquid zones (0xDE)
- Complex terrain

### CLOU (Cloud Level)
- Heavy use of 0x5B (cloud platforms)
- Emphasis on jump-through mechanics
- Minimal solid ground

### BOIL (Boiler Room)
- Standard layout
- No special attributes observed
- Basic solid/empty

## Verification Needed

1. **Confirm platform behavior** - Test 0x5B, 0x09 as one-way platforms
2. **Identify slope mechanics** - How 0xB5-0xB7 affect movement
3. **Map hazard types** - Which values deal damage
4. **Decompile collision function** - `PlayerProcessTileCollision` 

## Related Documentation

- [Tile Header (Asset 100)](../blb/asset-types.md#asset-100-tile-header)
- [Tile Attributes (Asset 500)](../blb/asset-types.md#asset-500-tile-attributes)
- [Player System](player-system.md) - Collision response
