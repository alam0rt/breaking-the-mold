# Tile Collision System

This document describes the tile collision attribute system in Skullmonkeys.

## Overview

Each tile has a 1-byte collision attribute stored in Asset 500 (tile_attributes). The collision map has its own dimensions stored in the Asset 500 header.

**Key Functions** (verified via Ghidra):
- `GetTileAttributeAtPosition` @ 0x800241f4 - Get collision byte at pixel coords
- `InitTileAttributeState` @ 0x80024cf4 - Initialize collision from Asset 500
- `GetTileAttributeUnknown` @ 0x8007b758 - Read header bytes 0-3 (offsets)
- `GetTileAttributeDimensions` @ 0x8007b778 - Read header bytes 4-7 (dimensions)
- `GetTileAttributeData` @ 0x8007b79c - Get tile data pointer (+8 from header)

## Asset 500 Format (Verified via Ghidra)

```
Offset  Size  Type  Description
------  ----  ----  -----------
0x00    2     u16   offset_x (tile offset, typically 0)
0x02    2     u16   offset_y (tile offset, typically 0)
0x04    2     u16   width (collision map width in tiles)
0x06    2     u16   height (collision map height in tiles)
0x08    N     u8[]  tile data (1 byte per tile, width × height bytes)
```

## GetTileAttributeAtPosition Logic (Ghidra @ 0x800241f4)

```c
// param_1 = context pointer (GameState + 0x84)
// param_2 = pixel_x
// param_3 = pixel_y

// Context offsets (from InitTileAttributeState):
//   +0x68 = tile data pointer
//   +0x6c = offset_x (u16)
//   +0x6e = offset_y (u16)
//   +0x70 = width (s16)
//   +0x72 = height (s16)

if (ctx[0x68] == NULL) return 0;

int tile_x = (pixel_x >> 4) - ctx[0x6c];  // >> 4 = divide by 16
int tile_y = (pixel_y >> 4) - ctx[0x6e];

if (tile_x < 0 || tile_x >= ctx[0x70]) return 0;
if (tile_y < 0 || tile_y >= ctx[0x72]) return 0;

return ctx[0x68][tile_y * ctx[0x70] + tile_x];
```

## Floor Detection Logic (Ghidra PlayerCallback @ 0x800638d0)

The player movement callback checks floor solidity with a range check:

```c
// Floor is solid if: attr != 0 && attr <= 0x3B
if (attr == 0 || attr > 0x3B) {
    // Empty or trigger zone - no floor
} else {
    // Solid floor (values 0x01-0x3B)
}
```

**Range breakdown:**
- `0x00` = Empty (no collision)
- `0x01-0x3B` = Solid floor/wall (range includes 0x02 solid, slopes, etc.)
- `0x3C+` = Trigger zones (checkpoints, spawn zones, hazards)

## Known Collision Attributes

Based on extracted data analysis and Ghidra verification:

### Solid Range (0x01-0x3B)

| Value | Hex | Name | Description | Observed In |
|-------|-----|------|-------------|-------------|
| 2 | 0x02 | Solid | Full collision block | All levels |
| 9 | 0x09 | Platform? | One-way platform (jump through) | CLOU |
| 18 | 0x12 | Trigger | Level trigger/event zone | SCIE |
| 19 | 0x13 | Trigger B | Alternate trigger | CAVE |

### Trigger Range (0x3C+)

| Value | Hex | Name | Description | Observed In |
|-------|-----|------|-------------|-------------|
| 83 | 0x53 | Checkpoint | Save point trigger | SCIE, CAVE, etc. |
| 91 | 0x5B | Platform | One-way cloud platform | CLOU |
| 101 | 0x65 | Spawn Zone | Entity spawn/activity area | All levels |

### Empty

| Value | Hex | Name | Description |
|-------|-----|------|-------------|
| 0 | 0x00 | Empty | No collision, pass-through (~90-96% of tiles) |

## Wall Collision (Ghidra CheckWallCollision @ 0x80059bc8)

Wall collision checks 4 vertical points on the player's side:

```c
// direction: -1 = left, +1 = right
// Checks at Y-15, Y-16, Y-32, Y-48 (head to feet)
int check_x = player.x + direction * 8;

for (offset in [15, 16, 32, 48]) {
    attr = GetTileAttributeAtPosition(ctx, check_x, player.y - offset);
    if (attr != 0 && attr <= 0x3B) {
        return true;  // Wall hit
    }
}
return false;
```

## Floor Collision Check Points

From `PlayerCallback_800638d0`:

| Offset | Purpose |
|--------|---------|
| Y - 7  | Ledge grab detection |
| Y + 2  | Floor at feet (just below) |
| Y + 16 | One tile below feet |

## Key Functions (Verified)

| Address | Name | Purpose |
|---------|------|---------|
| 0x800241f4 | GetTileAttributeAtPosition | Get collision byte at pixel coords |
| 0x80024cf4 | InitTileAttributeState | Copy Asset 500 header to GameState |
| 0x8007b758 | GetTileAttributeUnknown | Read offset_x/offset_y from header |
| 0x8007b778 | GetTileAttributeDimensions | Read width/height from header |
| 0x8007b79c | GetTileAttributeData | Get tile data pointer (header + 8) |
| 0x8005a914 | PlayerProcessTileCollision | Process player-tile triggers |
| 0x80059bc8 | CheckWallCollision | Check wall at 4 vertical points |
| 0x800638d0 | PlayerCallback_800638d0 | Main player movement + collision |

## Level-Specific Observations

### SCIE (Science Lab)
- Simple horizontal level
- Mostly 0x00 (empty) and 0x02 (solid)
- 706 solid tiles, 281 spawn zones (0x65)
- Offset (0, 0) - no tile offset

### CAVE (Vertical Descent)
- Many slope tiles (0xB5-0xB7)
- Liquid zones (0xDE)
- Complex terrain

### CLOU (Cloud Level)
- Heavy use of 0x5B (cloud platforms)
- Emphasis on jump-through mechanics
- Minimal solid ground

## Related Documentation

- [Tile Header (Asset 100)](../blb/asset-types.md#asset-100-tile-header)
- [Tile Attributes (Asset 500)](../blb/asset-types.md#asset-500-tile-attributes)
- [Player System](player-system.md) - Collision response
