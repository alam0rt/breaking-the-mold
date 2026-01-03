# Skullmonkeys Runtime Behavior

This document describes runtime behavior observed via PCSX-Redux MCP debugging.

**NOTE: All addresses are for PAL version (SLES-01090).**

## Level Loading State Machine

The game uses a sliding window state machine to track concurrent operations.
Each "slot" uses paired bytes in the BLB header:
- **Mode byte**: header[0xF36 + arrayPosition]  
- **Index byte**: header[0xF92 + arrayPosition]

### Sliding Window Formula (VERIFIED)

```
arrayPosition = headerOffset - 0x0A
levelIndex = header[0xF92 + arrayPosition]
levelEntry = header + (levelIndex * 0x70)
```

Where:
- `headerOffset` is read from LevelDataContext at ctx+0x60
- `0x0A` appears to be the base offset (MENU uses 0x0A)
- The resulting `levelIndex` is the index into the level metadata table

### Verified Examples

| headerOffset | arrayPosition | levelIndex | Level |
|--------------|---------------|------------|-------|
| 0x0A (10) | 0 | 0 | MENU |
| 0x20 (32) | 22 | 9 | FOOD (Skullmonkey Brand Hot Dogs) |

### Header Offset Progression

The `headerOffset` field in LevelDataContext (at ctx+0x60) increments as the game
performs different load operations:

| headerOffset | Operation | Notes |
|--------------|-----------|-------|
| 0x0A (10) | MENU loaded | After intro movies complete |
| 0x12 (18) | Demo level loaded | TMPL (Monkey Shrines) |
| 0x0E (14) | Gameplay level loaded | When player selects a level |

## Menu Idle Demo Behavior

When the player remains idle at the main menu, the game automatically loads
a demo level after a timeout period.

**Verified via PCSX-Redux MCP (PAL / SLES-01090):**

### Demo Level: TMPL (Monkey Shrines) - Level Index 3

| Field | Value | Description |
|-------|-------|-------------|
| Level ID | "TMPL" | Monkey Shrines |
| Level Index | 3 | In BLB header level table |
| Header Offset | 0x12 (18) | State machine position |
| Geometry (0x258) | 510,108 bytes | Level graphics/world data |
| Collision (0x259) | 188,288 bytes | Physics/collision data |
| Palette (0x25A) | 196 bytes | Color palette |
| **Total Primary** | ~699 KB | Combined asset size |

### State Transition Sequence

```
MENU (headerOffset=0x0A)
    ↓ (idle timeout)
Demo: TMPL (headerOffset=0x12)
    ↓ (demo ends or button press)
MENU (headerOffset=0x0A)
```

The demo plays a pre-recorded input sequence through the Monkey Shrines level,
showcasing gameplay to idle players (attract mode).

## Level Size Comparison

Captured via runtime memory inspection:

| Level | ID | Geometry | Collision | Palette | Total |
|-------|-----|----------|-----------|---------|-------|
| Menu | MENU | 53 KB | 11 KB | 24 B | ~65 KB |
| Demo (Monkey Shrines) | TMPL | 510 KB | 188 KB | 196 B | ~699 KB |
| Science Centre | SCIE | 524 KB | 126 KB | 148 B | ~650 KB |

The menu is intentionally lightweight (~10x smaller than gameplay levels).

## LevelDataContext Address

The active level's data is tracked in the LevelDataContext structure:

- **Base address**: `0x8009DCC4`
- **BLB header pointer**: ctx+0x5C → `0x800AE3E0`
- **Header offset**: ctx+0x60 (current state machine position)
- **TOC pointer**: ctx+0x68 (points to loaded Table of Contents)
- **Asset pointers**: ctx+0x70, ctx+0x74, ctx+0x7C

See `docs/blb-data-format.md` for full structure documentation.

## Debugging Tips

When reading memory during gameplay:

1. **Always pause first** - Use `pause()` before reads to get consistent snapshots
2. **Check headerOffset** - This tells you what operation is active
3. **Check asset259Size** - Quick way to identify which level is loaded:
   - MENU: 11,420 bytes (0x2C9C)
   - SCIE: 126,256 bytes (0x1ED30)
   - TMPL: 188,288 bytes (0x2DF80)

4. **Resume after** - Don't forget to `resume()` when done
