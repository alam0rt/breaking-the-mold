---
title: "Quick Reference: Tile Collision Attributes"
category: root
tags: [tile, collision, quick, ref]
---

# Quick Reference: Tile Collision Attributes

**Source**: PlayerProcessTileCollision @ 0x8005a914  
**Format**: 1 byte per tile from Asset 500

---

## Ranges

| Range | Type | Description |
|-------|------|-------------|
| 0x00 | Empty | No collision (passthrough) |
| 0x01 | Empty | No collision (passthrough) |
| 0x02-0x11 | Flat Solid | Heights 16→1 pixels |
| 0x12-0x13 | **45° Slopes** | Full steep slopes |
| 0x14 | Empty | No collision |
| 0x15-0x28 | **Dual: Color + Slopes** | RGB tinting + 22.5°/11.25°/5.6° slopes |
| 0x29-0x38 | **5.6° Gentle Slopes** | 8-tile sequences |
| 0x39 | Flat Half | 8px height |
| 0x3A-0x3B | **~53° Steep Slopes** | Special angles |
| 0x3C+ | Triggers | Special effects (100+ values) |
| 0xB5-0xB7, 0xC9, 0xCB, 0xDD-0xDF | **Semi-solid** | One-way platforms |

**Solidity Test**: `attr != 0 && attr <= 0x3B` = solid

---

## Color Zones (0x15-0x28) - NEW

| Value Range | Hex | Effect |
|-------------|-----|--------|
| 21-40 | 0x15-0x28 | Apply RGB tint from `g_TriggerZoneColorTable[index*3]` |

Outputs to player: `+0x15d` (R), `+0x15e` (G), `+0x15f` (B)

---

## Semi-Solid Tiles - NEW

| Value | Hex | Description |
|-------|-----|-------------|
| 181-183 | 0xB5-0xB7 | Type A one-way platforms |
| 201 | 0xC9 | Type B special pass-through |
| 203 | 0xCB | Type C special pass-through |
| 221-223 | 0xDD-0xDF | Type D one-way platforms |

Pass-through enabled by `player[0x128]` flag (set when jumping from below).

---

## Trigger Tiles (0x3C+)

### Checkpoints & Progress
| Value | Hex | Effect |
|-------|-----|--------|
| 2-7 | 0x02-0x07 | World 0-5 checkpoints |

### Hazards
| Value | Hex | Effect |
|-------|-----|--------|
| 42 | 0x2A | Death zone (if falling/jumping) |

### Wind Zones
| Value | Hex | Direction | Velocity |
|-------|-----|-----------|----------|
| 61 | 0x3D | ← Left | X: -1 |
| 62 | 0x3E | → Right | X: +1 |
| 63 | 0x3F | ↙ Down-Left | X: -2, Y: -1 (cond) |
| 64 | 0x40 | ↘ Down-Right | X: +2, Y: -1 (cond) |
| 65 | 0x41 | ↓ Down | Y: -4 |

### Item Pickups
| Range | Hex | Effect |
|-------|-----|--------|
| 50-59 | 0x32-0x3B | Items 0-9, sound 0x7003474c |

### Spawn Zones
| Value | Hex | Group | Mode |
|-------|-----|-------|------|
| 81 | 0x51 | 1 | Enable |
| 82 | 0x52 | 2 | Enable |
| 101 | 0x65 | 1 | Disable |
| 102 | 0x66 | 2 | Disable |
| 121 | 0x79 | 1 | Mode 2 |
| 122 | 0x7A | 2 | Mode 2 |

---

## Player Entity Fields

| Offset | Type | Name | Purpose |
|--------|------|------|---------|
| 0x128 | u8 | semi_solid_pass | Semi-solid pass-through flag - NEW |
| 0x15D | u8 | tint_r | Color zone R - NEW |
| 0x15E | u8 | tint_g | Color zone G - NEW |
| 0x15F | u8 | tint_b | Color zone B - NEW |
| 0x160 | s16 | push_x | Wind horizontal |
| 0x162 | s16 | push_y | Wind vertical |
| 0x170 | u8 | enable_diagonal | Y component flag |
| 0x1A6 | s16 | spawn_group_1 | Group 1 state (0/1/2) |
| 0x1A8 | s16 | spawn_group_2 | Group 2 state (0/1/2) |
| 0x1AE | u8 | disable_triggers | Skip default handler |
| 0x1B3 | u8 | checkpoint_id | Current checkpoint |

---

## Slope Height Table (g_SlopeHeightTable @ 0x8009d228)

Each solid tile attribute (0x00-0x3B) has 16 height values for subpixel collision.

**Lookup**: `height = g_SlopeHeightTable[attr * 16 + (x_pixel & 0xF)]`

### Flat Tiles
| Attr | Height | Description |
|------|--------|-------------|
| 0x00-0x01 | 0 | Empty (no collision) |
| 0x02 | 16 | Full height solid |
| 0x03 | 15 | Partial solid |
| 0x04-0x11 | 14→1 | Decreasing heights |

### 45° Slopes (steep)
| Attr | Pattern | Direction |
|------|---------|-----------|
| 0x12 | 16→1 | ↘ Down-right |
| 0x13 | 1→16 | ↗ Up-right |

### 22.5° Slopes (2-tile sequence)
| Attr | Pattern | Direction |
|------|---------|-----------|
| 0x15 | 16→9 (upper half) | ↘ Down-right |
| 0x16 | 8→1 (lower half) | ↘ Down-right |
| 0x17 | 1→8 (lower half) | ↗ Up-right |
| 0x18 | 9→16 (upper half) | ↗ Up-right |

### 11.25° Slopes (3-tile sequence)
| Attr | Pattern | Direction |
|------|---------|-----------|
| 0x19 | 16→11 | ↘ Down-right (1/3) |
| 0x1A | 11→6 | ↘ Down-right (2/3) |
| 0x1B | 6→1 | ↘ Down-right (3/3) |
| 0x1C | 1→6 | ↗ Up-right (1/3) |
| 0x1D | 6→11 | ↗ Up-right (2/3) |
| 0x1E | 12→16 | ↗ Up-right (3/3) |

### 5.6° Slopes (4-tile sequence)
| Attr | Pattern | Direction |
|------|---------|-----------|
| 0x1F-0x22 | 16→13→9→5→1 | ↘ Down-right (4 tiles) |
| 0x23-0x26 | 1→5→9→13→16 | ↗ Up-right (4 tiles) |

### 2.8° Slopes (8-tile sequence)
| Attr | Pattern | Direction |
|------|---------|-----------|
| 0x29-0x30 | 16→15...→2→1 | ↘ Down-right (8 tiles) |
| 0x31-0x38 | 1→2...→15→16 | ↗ Up-right (8 tiles) |

### Special
| Attr | Height | Description |
|------|--------|-------------|
| 0x39 | 8 | Half-height flat |
| 0x3A | 15→3 | ~53° steep down |
| 0x3B | 3→15 | ~53° steep up |

---

**Note**: Attributes 0x15-0x28 serve **dual purposes**:
1. **Collision**: Slope heights for Y position calculation
2. **Visual**: Color zone tinting (RGB from `g_TriggerZoneColorTable`)

This allows underwater/cave areas with sloped floors and color effects.

---

## Sound Effects

| ID | Hex | Context |
|----|-----|---------|
| 0x248e52 | - | Jump on checkpoint |
| 0x7003474c | - | Item pickup |

---

## Asset 500 Format

```
+0x00  s16  offset_x
+0x02  s16  offset_y
+0x04  s16  width
+0x06  s16  height
+0x08  u8[] attributes (width × height)
```

**LevelDataContext (GameState+0x84):**
- +0x68: Tile data pointer
- +0x6C: offset_x
- +0x6E: offset_y
- +0x70: width
- +0x72: height

---

## Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800241f4 | GetTileAttributeAtPosition | Pixel → attr |
| 0x8005a914 | PlayerProcessTileCollision | Switch handler |
| 0x800226f8 | CheckEntityCollision | Entity collisions |
| 0x800245bc | CheckTriggerZoneCollision | Filter solid/trigger |
| 0x80024cf4 | InitTileAttributeState | Load Asset 500 |

---

## Implementation Checklist

For game engines implementing Skullmonkeys collision:

- [ ] Load Asset 500 (8-byte header + tile array)
- [ ] Implement GetTileAttributeAtPosition (pixel >> 4 = tile coords)
- [ ] Implement solid test (attr > 0 && attr <= 0x3B)
- [ ] Handle checkpoints (0x02-0x07, store ID)
- [ ] Handle death zones (0x2A, check if in air)
- [ ] Handle wind zones (0x3D-0x41, modify velocity)
- [ ] Handle item pickups (0x32-0x3B, mark collected)
- [ ] Handle spawn zones (0x51/0x52/0x65/0x66/0x79/0x7A, control groups)
- [ ] Entity collision masks (bitwise AND check)
- [ ] Box overlap detection (FUN_8001b3f0 reference)

---

**Full documentation**: `docs/systems/tile-collision-complete.md`
