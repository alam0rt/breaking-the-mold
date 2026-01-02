# GAME.BLB Data Format Documentation

This document describes the structure of level and asset data in Skullmonkeys' GAME.BLB archive.

## Overview

Each level in the game consists of three data segments loaded from GAME.BLB:
- **Primary**: Level geometry, collision, and palette data
- **Secondary**: Sprites and animations
- **Tertiary**: Audio and music (optional for some levels)

## BLB Header Reference

The BLB header (first 0x1000 bytes) contains metadata for all levels:

```
Offset   Size   Description
------   ----   -----------
0x000    0xB60  Level Metadata Table (26 entries × 0x70 bytes)
0xB60    0x170  Movie Table (13 entries × 0x1C bytes)
0xCD0    0x200  Sector Offset Table (32 entries × 0x10 bytes)
0xED0    0x040  Unknown u32 array (16 values - file offsets)
0xF10    0x021  Credits Table (3 entries × 12 bytes, partially overlaps counts)
0xF31    0x001  Level Count (26)
0xF32    0x001  Asset/Movie Count (13)
0xF33    0x001  Sector Table Count (32)
0xF34    0x0CC  Game State Data
```

## Level Metadata Entry (0x70 bytes)

Each level has a metadata entry at offset `index × 0x70`:

```
Offset   Size   Description
------   ----   -----------
0x00     u16    Primary sector offset
0x02     u16    Primary sector count
0x04     8      Static data (unknown)
0x0C     u8     Level index
0x0D     u8     Flags
0x0E     14     Unknown
0x1C     u16    Unknown
0x1E     u16    Secondary sector offset
0x20     u16    Unknown
0x22     10     Dynamic data 1
0x2C     u16    Secondary sector count
0x2E     16     Unknown
0x3E     u16    Unknown
0x40     u16    Tertiary sector offset
0x42     u16    Unknown
0x44     10     Dynamic data 2
0x4E     u16    Tertiary sector count
0x50     6      Unknown
0x56     5      Level ID (4-char code + null)
0x5B     21     Level name (truncated)
```

## TOC (Table of Contents) Format

All three data segments use the same TOC format:

```
Offset   Size   Description
------   ----   -----------
0x00     u16    Entry count
0x02     u16    Padding
0x04+    12×N   TOC entries (N = count)

Each TOC Entry (12 bytes):
  0x00   u32    Asset type
  0x04   u32    Asset size in bytes
  0x08   u32    Offset from start of data
```

## Asset Types

### Primary Data (Level Geometry)
| Type | Hex | Description | Typical Size |
|------|-----|-------------|--------------|
| 600 | 0x258 | Level graphics/world data | 500KB-1MB |
| 601 | 0x259 | Collision/physics data | 100-300KB |
| 602 | 0x25A | Palette/color data | 24-200 bytes |

### Secondary Data (Sprites)
| Type | Hex | Description |
|------|-----|-------------|
| 100 | 0x064 | Header/index entry |
| 101 | 0x065 | Secondary header (optional) |
| 300 | 0x12C | Main sprite graphics data |
| 301 | 0x12D | Sprite metadata |
| 400 | 0x190 | Animation frame data |
| 401 | 0x191 | Animation configuration |

### Tertiary Data (Audio)
| Type | Hex | Description |
|------|-----|-------------|
| 100 | 0x064 | Entry headers |
| 200 | 0x0C8 | Sound effect data |
| 201 | 0x0C9 | Sound headers/index |
| 401 | 0x191 | Audio configuration |
| 500 | 0x1F4 | Music/sequence data |
| 501 | 0x1F5 | Music configuration |

## Sector Files

The sector files extracted to `sectors/` contain preview/loading graphics:
- Named by 4-character level code (e.g., `PHRO`, `SCIE`, `BOIL`)
- Smaller than primary data (10-20KB typically)
- Likely used for level select thumbnails and loading screens
- Format appears to be raw or RLE-compressed image data

## Loading Process

Based on decompiled code in `LevelDataParser.c`:

1. Game reads BLB header from sectors 0-1 (0x1000 bytes)
2. When loading a level:
   - Look up level index in header at 0xF92
   - Get sector offset/count from level metadata table
   - Load primary data from BLB
   - Parse TOC to locate asset pointers (0x258, 0x259, 0x25A)
3. For secondary/tertiary:
   - Use secondary_offset/count and tertiary_offset/count from metadata
   - Parse similar TOC structures with different asset types

## Code References

- `src/LevelDataParser.c`: Main parsing logic
- `src/LoadBLBHeader.c`: Header loading
- `src/BLBHeaderAccessors.c`: Accessor functions for header fields
- `scripts/blb.py`: Python parsing library
- `scripts/extract_blb.py`: Extraction tool

## Primary.bin Internal Structure

The primary.bin files contain the main level geometry and collision data.
After parsing the TOC, the game accesses three asset types:

### Asset 0x258 - World/Level Graphics

The largest asset, containing level geometry. Structure:

```
Offset  Size    Description
------  ----    -----------
0x00    u32     Entry count
0x04+   var     Entry table (variable stride, ~12-24 bytes per entry)
...     var     Graphics data (referenced by entries)
```

Each entry appears to contain:
- Type/flags field
- Position coordinates (possibly u16 x, u16 y)
- Offset to graphics data within asset

### Asset 0x259 - Collision/Physics Data

Contains collision geometry and physics properties:

```
Offset  Size    Description
------  ----    -----------
0x00    u16     Entry count (or type?)
0x02    var     Collision geometry data
```

### Asset 0x25A - Palette/Color Data

Small palette data (24-200 bytes typically):

```
Format: Array of 15-bit RGB values (u16 each)
        Bits 0-4:   Red (0-31, multiply by 8 for 8-bit)
        Bits 5-9:   Green
        Bits 10-14: Blue
        Bit 15:     Transparency (STP bit)
```

Example: 0x3FFF = white (R=31, G=31, B=15 in 5-bit each)

## Code Flow for Level Loading

1. `LoadBLBHeader` (0x800208B0): Reads BLB header into RAM at 0x800AE3E0
2. `func_8007CD34`: Initializes game state with header reference
3. `func_8007A62C` (LevelDataParser): Parses primary.bin TOC
   - Stores asset pointers in LevelDataContext:
     - ctx+0x68: TOC pointer
     - ctx+0x6C: Data offset
     - ctx+0x70: Asset 0x258 pointer (world data)
     - ctx+0x74: Asset 0x259 pointer (collision)
     - ctx+0x78: Asset 0x259 size
     - ctx+0x7C: Asset 0x25A pointer (palette)
4. Rendering functions access ctx+0x70 to draw level graphics
5. Physics functions access ctx+0x74 for collision detection

## Example Usage

```python
from scripts.blb import BLBHeader

header = BLBHeader.from_file('disks/blb/GAME.BLB')

# Access level info
for level in header.level_entries:
    print(f"{level.index}: {level.name} @ sector {level.sector_offset}")

# Extract level data
level_data = header.extract_level_data(0)  # Options menu
```
