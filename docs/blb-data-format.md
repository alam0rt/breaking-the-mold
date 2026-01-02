# GAME.BLB Data Format Documentation

This document describes the structure of level and asset data in Skullmonkeys' GAME.BLB archive.

## Game Structure Overview

Skullmonkeys contains **90 individual stages** spread across **26 level themes** stored in the BLB:

| Category | Count | BLB IDs |
|----------|-------|---------|
| Menu | 1 | MENU |
| Regular Worlds | 17 | PHRO, SCIE, TMPL, BOIL, SNOW, FOOD, BRG1, GLID, CAVE, WEED, EGGS, CLOU, SOAR, CRYS, CSTL, MOSS, EVIL |
| Bosses | 5 | MEGA (Shriney Guard), HEAD (Joe-Head-Joe), GLEN (Glenn Yntis), WIZZ (Monkey Mage), KLOG (Klogg) |
| Special Modes | 2 | FINN (swimming), RUNN (runner) |
| Secret Bonus | 1 | SEVN (1970's) |

Each "world" theme contains multiple stages (Stage 01, 02-A, 02-B, 03, etc.) plus bonus rooms.

## Data Segment Overview

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
0xB60    0x168  Movie Table (13 entries × 0x1C bytes)
0xCC8    0x008  Padding (zeros)
0xCD0    0x028  Unknown Table (looks like special level entries)
0xCF8    0x1A0  Level Order Table (26+ entries × 0x10 bytes)
0xE98    0x078  Additional entries (credits, game over, etc.)
0xF10    0x021  Unknown data
0xF31    0x001  Level Count (26)
0xF32    0x001  Movie Count (13)
0xF33    0x001  Level Order Table Count
0xF34    0x0CC  Game State Data
```

## Movie Table (0xB60-0xCC7)

13 FMV movie entries, 0x1C (28) bytes each:

```
Offset   Size   Description
------   ----   -----------
0x00     u16    Unknown (always 0)
0x02     u16    Sector count
0x04     char[4] Movie ID (e.g., "DREA", "LOGO")
0x08     char[4] Short name
0x0C     char[16] ISO path (e.g., "\MVDWI.STR;1")
```

| # | ID | Sectors | Path | Description |
|--:|:---|-------:|:-----|:------------|
| 0 | DREA | 79 | \MVDWI.STR | Dreamworks intro |
| 1 | LOGO | 105 | \MVLOGO.STR | Logo |
| 2 | ELEC | 60 | \MVEA.STR | EA logo |
| 3 | INT1 | 3091 | \MVINTRO1.STR | Intro part 1 |
| 4 | INT2 | 156 | \MVINTRO2.STR | Intro part 2 |
| 5 | GASS | 1545 | \MVGAS.STR | ? |
| 6 | YAMM | 1776 | \MVYAM.STR | ? |
| 7 | REDD | 2119 | \MVRED.STR | ? |
| 8 | YNTS | 463 | \MVYNT.STR | YNT world intro |
| 9 | EYES | 918 | \MVEYE.STR | ? |
| 10 | EVIL | 1008 | \MVEVIL.STR | Evil Engine intro |
| 11 | END1 | 1044 | \MVEND.STR | Ending part 1 |
| 12 | END2 | 793 | \MVWIN.STR | Ending part 2 |

## Level Order Table (0xCF8-0xE97)

26 entries for level loading order, 0x10 (16) bytes each:

```
Offset   Size   Description
------   ----   -----------
0x00     char[4] Short display name (3 chars + null)
0x04     u16    Sector offset
0x06     u16    Some count (4, 9, or 10)
0x08     u16    Level index
0x0A     u8     Padding
0x0B     char[5] Level ID (4 chars + null)
```

## Level Metadata Entry (0x70 bytes)

Each level has a metadata entry at offset `index × 0x70`:

```
Offset   Size   Description
------   ----   -----------
# Primary data pointers (0x00-0x0B)
0x00     u16    Primary sector offset (CONFIRMED)
0x02     u16    Primary sector count (CONFIRMED)
0x04     u16    Unknown (large values 776-65240, purpose TBD)
0x06     u16    Unknown count (7-20 range, purpose TBD)
0x08     u16    Entry[1].offset low 16 bits (CONFIRMED - 26/26 match with primary TOC)
0x0A     u16    Unknown count (0-16 range, purpose TBD)

# Level identification (0x0C-0x0D)
0x0C     u8     Level asset index (0-25)
0x0D     u8     Level flag (0 or 1, purpose TBD)

# Tertiary block configuration (0x0E-0x1B)
0x0E     u16    Tertiary block count (CONFIRMED - 1-6, matches non-zero tert sub-counts)
0x10     u16[6] Tertiary data offsets (byte offset within each tert block)
0x1C     u16    Padding (always 0)

# Secondary data structure (0x1E-0x39)
0x1E     u16    Secondary base sector offset
0x20     u16[5] Secondary sub-block sector offsets
0x2A     u16    Padding (always 0)
0x2C     u16    Secondary base sector count
0x2E     u16[5] Secondary sub-block sector counts
0x38     u16    Padding (always 0)

# Tertiary data structure (0x3A-0x55)
0x3A     u16[6] Tertiary sub-block sector offsets
0x46     u16    Padding (always 0)
0x48     u16[6] Tertiary sub-block sector counts
0x54     u16    Padding (always 0)

# Level strings (0x56-0x6F)
0x56     char[5]  Level ID (4-char code + null, e.g., "MENU")
0x5B     char[21] Level name (null-terminated, e.g., "Options Menu")
```

### Unknown Fields Analysis

**Fields 0x04, 0x06, 0x0A** remain unidentified. Observations:
- Levels with identical (0x06, 0x0A) pairs often share the first few worlds:
  - (14, 7): PHRO, SCIE, TMPL (first 3 gameplay levels)
  - But other groupings span unrelated worlds
- Values do NOT correlate with:
  - Movie indices (only 13 movies, v0A goes to 16)
  - Stage counts within worlds
  - Direct tileset/theme sharing
- Possibly related to asset loading parameters or memory layout

### Data Interleaving Pattern

Level data sectors are interleaved on disc for streaming:
```
PRIMARY → SECONDARY → TERT[0] → SEC_SUB[0] → TERT[1] → SEC_SUB[1] → ...
```

Example (MENU level): 
- 201-232 (PRIMARY) → 233-299 (SECONDARY) → 300-787 (TERT[0]) → 788-849 (SEC_SUB[0]) → ...

## TOC (Table of Contents) Format

All three data segments use the same TOC format:

```
Offset   Size   Description
------   ----   -----------
0x00     u32    Entry count (NOT u16 as previously documented)
0x04+    12×N   TOC entries (N = count)

Each TOC Entry (12 bytes):
  0x00   u32    Asset type ID (e.g., 0x258=600, 0x259=601, 0x25A=602)
  0x04   u32    Asset size in bytes
  0x08   u32    Offset from start of loaded data
```

**Relationship to Level Metadata:**
- The `entry1_offset_lo` field at level metadata offset +0x08 contains the 
  low 16 bits of Entry[1].offset from the primary TOC (CONFIRMED 26/26 match)
- This allows quick access to Entry[1] (type 0x259) data without parsing TOC

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
