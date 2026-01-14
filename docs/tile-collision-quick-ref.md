# Quick Reference: Tile Collision Attributes

**Source**: PlayerProcessTileCollision @ 0x8005a914  
**Format**: 1 byte per tile from Asset 500

---

## Ranges

| Range | Type | Description |
|-------|------|-------------|
| 0x00 | Empty | No collision |
| 0x01-0x3B | Solid | Floor/wall/platform (59 values) |
| 0x3C+ | Triggers | Special effects (100+ values) |

**Solidity Test**: `attr != 0 && attr <= 0x3B` = solid

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
| 0x160 | s16 | push_x | Wind horizontal |
| 0x162 | s16 | push_y | Wind vertical |
| 0x170 | u8 | enable_diagonal | Y component flag |
| 0x1A6 | s16 | spawn_group_1 | Group 1 state (0/1/2) |
| 0x1A8 | s16 | spawn_group_2 | Group 2 state (0/1/2) |
| 0x1AE | u8 | disable_triggers | Skip default handler |
| 0x1B3 | u8 | checkpoint_id | Current checkpoint |

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
