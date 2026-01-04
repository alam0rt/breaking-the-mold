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

### Verified Level Sizes (via PCSX-Redux MCP, PAL / SLES-01090)

| Level | ID | Index | Geometry (0x258) | Collision (0x259) | Palette (0x25A) | Total |
|-------|-----|-------|------------------|-------------------|-----------------|-------|
| Menu | MENU | 0 | 53,308 B | 11,420 B | 24 B | ~65 KB |
| Skullmonkey Gate | PHRO | 1 | - | - | - | - |
| Science Centre | SCIE | 2 | 524,212 B | 126,256 B | 148 B | ~650 KB |
| Monkey Shrines | TMPL | 3 | 510,108 B | 188,288 B | 196 B | ~699 KB |

*Note: TMPL is used as the demo/attract mode level when idle at menu.*

## BLB Header Reference

The BLB header (first 0x1000 bytes) contains metadata for all levels:

```
Offset   Size   Description
------   ----   -----------
0x000    0xB60  Level Metadata Table (26 entries × 0x70 bytes)
0xB60    0x16C  Movie Table (13 entries × 0x1C bytes)
0xCC8    0x008  Padding (zeros, between movie and sector tables)
0xCD0    0x200  Sector Table (32 entries × 0x10 bytes, count at 0xF33)
0xED0    0x040  Unknown u32 array (16 entries, file offsets, purpose TBD)
0xF10    0x021  Credits Sequence Table (2 complete entries × 0x0C bytes + overlap)
0xF31    0x001  Level Count (u8, value=26)
0xF32    0x001  Movie Count (u8, value=13)
0xF33    0x001  Sector Table Entry Count (u8)
0xF34    0x0CC  Playback Sequence Data (mode array at 0xF36, index array at 0xF92)
```

## Movie Table (0xB60-0xCC7)

13 FMV movie entries, 0x1C (28) bytes each:

```
Offset   Size   Description
------   ----   -----------
0x00     u16    Reserved/unused (always 0)
0x02     u16    Sector count (movie size in sectors)
0x04     char[5] Movie ID (4-char null-terminated, e.g., "DREA", "LOGO")
0x09     char[3] Short name (2-char null-terminated)
0x0C     char[16] ISO path (e.g., "\\MVLOGO.STR;1")
```

Note: Movies are external .STR files on the CD, not embedded in GAME.BLB.

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

## Sector Table (0xCD0-0xECF)

Loading screen and special sector entries, 0x10 (16) bytes each.
Entry count stored at header offset 0xF33 (typically 32 entries).

```
Offset   Size   Description
------   ----   -----------
0x00     u8     Level index (0-25 when entry_flags=0x00)
0x01     u8     Entry flags (0x00=level, 0x03=game over, 0x05=special loading)
0x02     u8     Unknown byte (0x0A for loading screens, 0x63 for game over)
0x03     char[5] Code (4-char null-terminated, e.g., "PIRA", "MENU")
0x08     char[4] Short name (truncated description)
0x0C     u16    Sector offset in BLB
0x0E     u16    Sector count
```

**Entry type patterns (PAL version):**
- `entry_flags=0x00`: Level loading screens (level_index = 0-25)
- `entry_flags=0x05`: Special loading screens (PIRA=pirates intro, LEGL=legal)
- `entry_flags=0x03`: Game over screens (unknown_byte=0x63)
- `entry_flags=0x00, level_index=0x35`: Credits screen

### Loading Screen MDEC Frame Format (VERIFIED)

Loading screens are stored as **BS v2** (Bitstream version 2) MDEC frames - the standard
PSX compressed video format also used for STR movies. Each sector table entry points to
one compressed MDEC frame.

**BS Frame Header (8 bytes):**
```
Offset   Size   Description
------   ----   -----------
0x00     u16    Frame size in 16-bit words (includes header)
0x02     u16    Magic number (0x3800 = BS format marker)
0x04     u16    Quantization scale (typically 1-63, lower = better quality)
0x06     u16    Version (0x0002 = v2, 0x0003 = v3)
0x08+    var    VLC-encoded DCT coefficient data
```

**Decoding Pipeline (from FUN_800399a8):**
1. `CdBLB_ReadSectors()` - Read raw BS frame from disc
2. `DecDCTvlcBuild()` - Build VLC lookup table (once per session)
3. `DecDCTvlc2()` - Decompress VLC to DCT coefficient blocks
4. `DecDCTin()` - Send DCT blocks to MDEC hardware
5. `DecDCTout()` - Receive decompressed 15-bit RGB pixels

**Display Configuration:**
- Resolution: 320×256 pixels (0x140 × 0x100)
- Color depth: 15-bit RGB (no 24-bit mode)
- Double buffered: Alternates Y=0 and Y=256

**Example Sector Table Entries (PAL):**
| Level | ID | Sector | Count | Bytes | Frame Size |
|-------|-----|--------|-------|-------|------------|
| SCIE | Science | 0x1395 | 10 | 20,480 | 0x34C0 words (27,008 B) |
| MENU | Menu | 0x0819 | 6 | 12,288 | ~12 KB |
| PHRO | Pharaoh | 0x0C86 | 8 | 16,384 | ~16 KB |

**RAM Locations:**
- VLC table base: Passed as param_3 to decoder
- Compressed frame: param_3 + 0x33800 (offset 211,968 bytes)
- VRAM output: Y=0 or Y=256 (double-buffered)

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

All three data segments (Primary, Secondary, Tertiary) use the same TOC format:

```
Offset   Size   Description
------   ----   -----------
0x00     u32    Entry count
0x04+    12×N   TOC entries (N = count)

Each TOC Entry (12 bytes):
  0x00   u32    Asset type ID (e.g., 0x258=600, 0x259=601, 0x25A=602)
  0x04   u32    Asset size in bytes
  0x08   u32    Offset from start of segment data
```

**Relationship to Level Metadata:**
- The `entry1_offset_lo` field at level metadata offset +0x08 contains the 
  low 16 bits of Entry[1].offset from the primary TOC (CONFIRMED 26/26 match)
- This allows quick access to Entry[1] (type 0x259) data without parsing TOC

## Asset Sub-TOC Format (VERIFIED)

Container assets (types 0x258, 0x259, 0x190) have an internal sub-TOC structure:

```
Offset   Size   Description
------   ----   -----------
0x00     u32    Sub-entry count
0x04+    12×N   Sub-entries (N = count)

Each Sub-Entry (12 bytes):
  0x00   u32    Flags (type/metadata, format TBD)
  0x04   u32    Data size in bytes
  0x08   u32    Offset from start of asset
```

**Verification:** Entry[0].offset always equals `4 + count * 12` (the header size).

**Container vs Raw Assets:**
- **Container assets** (have sub-TOC): 0x258, 0x259, 0x190
- **Raw assets** (data starts immediately): 0x25A, 0x064, 0x12C, 0x12D, 0x12E, 0x191

## Complete File Hierarchy

```
BLB File
├── Header (0x1000 bytes = 2 sectors)
│   └── Level Table: 26 entries × 0x70 bytes
│         ├── +0x00: Primary sector offset/count
│         ├── +0x1E: Secondary sector offset/count
│         └── +0x3A: Tertiary sector offsets/counts
│
└── Level Data Segments (at sector offsets from header)
    └── Segment TOC
          ├── count: u32
          └── entries[count]: {type, size, offset}
                │
                ├── Container Asset (0x258, 0x259, 0x190)
                │   └── Sub-TOC
                │         ├── count: u32
                │         └── entries[count]: {flags, size, offset}
                │               └── Raw sub-asset data
                │
                └── Raw Asset (0x25A, 0x064, 0x12C, etc.)
                    └── Raw data (no sub-TOC)
```

## Asset Types

### Primary Data (Level Geometry)
| Type | Hex | Structure | Description | Typical Size |
|------|-----|-----------|-------------|--------------|
| 600 | 0x258 | CONTAINER | Level graphics/world data | 500KB-1MB |
| 601 | 0x259 | CONTAINER | Collision/layout data | 100-300KB |
| 602 | 0x25A | RAW | Palette/color data (15-bit PSX) | 24-200 bytes |

### Secondary Data (Sprites/Tiles) - VERIFIED

| Type | Hex | Structure | Description |
|------|-----|-----------|-------------|
| 100 | 0x064 | RAW | Tile header (36 bytes, contains tile counts) |
| 101 | 0x065 | RAW | Secondary header (optional) |
| 300 | 0x12C | RAW | Tile pixel data (8bpp indexed) |
| 301 | 0x12D | RAW | Palette index per tile (1 byte/tile) |
| 302 | 0x12E | RAW | Tile size/category flag (1 byte/tile, see below) |
| 400 | 0x190 | CONTAINER | Palette container (256-color palettes) |
| 401 | 0x191 | RAW | Animation/palette configuration |
| 601 | 0x259 | CONTAINER | Secondary layout data |
| 602 | 0x25A | RAW | Secondary palette |

#### Asset 100 - Tile Header (36 bytes, CODE-VERIFIED)

Verified via Ghidra decompilation of `CopyTilePixelData` (0x8007b588) and `GetTotalTileCount` (0x8007b53c).

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    3     u8[3]  Background RGB color (written to param+0x124 on first entity)
0x03    1     u8     Padding
0x04    3     u8[3]  Secondary RGB color  
0x07    1     u8     Padding
0x08    2     u16    Level width in tiles (*16 for pixels, see GetLevelDimensions)
0x0A    2     u16    Level height in tiles (*16 for pixels, see GetLevelDimensions)
0x0C    2     u16    Unknown (not accessed in analyzed functions)
0x0E    2     u16    Unknown (not accessed in analyzed functions)
0x10    2     u16    16×16 tile count (CODE-VERIFIED: CopyTilePixelData uses for data offset)
0x12    2     u16    8×8 tile count (CODE-VERIFIED: summed in GetTotalTileCount)
0x14    2     u16    Additional tile count (summed in GetTotalTileCount with 0x10, 0x12)
0x16    6     var    Unknown
0x1C    2     u16    Unknown (read by GetAsset100Field1C)
0x1E    4     var    Remaining header data
```

**Code reference:** `GetTotalTileCount` (0x8007b53c) computes total tiles as:
`*(u16*)(asset100 + 0x10) + *(u16*)(asset100 + 0x12) + *(u16*)(asset100 + 0x14)`

#### Asset 300 - Tile Pixel Data (EXTRACTION-VERIFIED)

8-bit indexed pixel data with 16-byte row stride:

- **16×16 tiles**: 256 bytes each (16 rows × 16 bytes)
- **8×8 tiles**: 128 bytes each (8 rows × 16 bytes, only first 8 columns used)

Layout in file:
1. First `count_16x16` tiles (256 bytes each)
2. Then `count_8x8` tiles (128 bytes each)

**Extraction script:** `scripts/extract_sprites.py` successfully extracts tiles
using Assets 300+301+302+400 to produce correctly colored PNG images.

#### Asset 301 - Palette Assignment (EXTRACTION-VERIFIED)

One byte per tile, indexing into Asset 400 palette array:
- Size = `count_16x16 + count_8x8` bytes
- Value = palette index (0 to N-1, where N = number of palettes in Asset 400)

#### Asset 302 - Tile Size/Category Flag (CODE-VERIFIED)

One byte per tile indicating tile size and properties.
Verified via Ghidra decompilation of `LoadTileDataToVRAM` (0x80025240).

- Size = `count_16x16 + count_8x8` bytes (same as Asset 301)
- Accessed via `GetTileSizeFlags` (0x8007b6bc) which returns `ctx[7]` (offset 0x1C in LevelDataContext)

**Bit-level interpretation (from decompiled code):**
```c
// In LoadTileDataToVRAM (0x80025240):
byte flags = asset302[tileIndex];
if ((flags & 4) != 0) continue;  // Bit 2: skip this tile entirely
uint tp = ((flags & 2) == 0);    // Bit 1: tile page mode (0=8x8, 1=16x16)
uint size = (tp == 0) ? 8 : 16;  // Determines tile dimensions
spriteInfo[tileIndex + 6] = flags & 1;  // Bit 0: stored for semi-transparency

// In FUN_80017540 (tile rendering):
SetSemiTrans(sprite, (byte)spriteInfo[3]);  // spriteInfo[3] at ushort* = byte 6
// This enables PSX GPU alpha blending when bit 0 is set
```

| Bit | Mask | Meaning | Effect |
|-----|------|---------|--------|
| 0 | 0x01 | Semi-transparency | Enables GPU alpha blending for this tile |
| 1 | 0x02 | Tile size | 0=16×16, 1=8×8 |
| 2 | 0x04 | Skip flag | If set, tile is not loaded/rendered |

**Observed values:**
| Value | Bits | Meaning |
|-------|------|---------|
| 0 | 000 | 16×16 tile, opaque, render |
| 1 | 001 | 16×16 tile, semi-transparent, render |
| 2 | 010 | 8×8 tile, opaque, render |
| 3 | 011 | 8×8 tile, semi-transparent, render |

**Layout:** All 8×8 tiles (bit 1 set) appear at the end, starting at index `count_16x16`.
This matches the pixel data layout in Asset 300 (16×16 tiles first, then 8×8 tiles).

**Bit distribution across levels (verified):**
| Level | Total | Semi-Trans | 8×8 | Skip |
|-------|-------|------------|-----|------|
| MENU | 457 | 91 | 0 | 0 |
| TMPL | 1440 | 25 | 281 | 0 |
| GLID | 1089 | 215 | 0 | 0 |
| CLOU | 1012 | 446 | 0 | 0 |
| MOSS | 1353 | 427 | 0 | 0 |
| RUNN | 2065 | 0 | 1577 | 0 |

Semi-transparency is used for effects like translucent water, fog, or glass surfaces.

#### Asset 400 - Palette Container (VERIFIED)

Standard sub-TOC format containing 256-color palettes:

```
Offset  Size   Description
------  ----   -----------
0x00    4      u32: Palette count
0x04+   12×N   Sub-entries (N = count)

Each sub-entry (12 bytes):
  0x00  u32    Palette index (0, 1, 2, ...)
  0x04  u32    Size in bytes (always 512 = 256 colors × 2)
  0x08  u32    Offset from container start

Each palette: 512 bytes = 256 × u16 (PSX 15-bit RGB)
  - Color 0 is transparent
  - Bits 0-4: Red (×8 for 8-bit)
  - Bits 5-9: Green (×8 for 8-bit)
  - Bits 10-14: Blue (×8 for 8-bit)
  - Bit 15: STP (semi-transparency)
```

**Extraction verified**: Tiles extracted with correct palette assignments match expected game graphics.

### Tertiary Data (Audio)
| Type | Hex | Structure | Description |
|------|-----|-----------|-------------|
| 100 | 0x064 | RAW | Entry headers |
| 200 | 0x0C8 | RAW | Sound effect data |
| 201 | 0x0C9 | RAW | Sound headers/index |
| 401 | 0x191 | RAW | Audio configuration |
| 500 | 0x1F4 | Music/sequence data |
| 501 | 0x1F5 | Music configuration |

## Supplementary Graphics Containers

**VERIFIED 2026-01-04 via Ghidra analysis + tile/layer extraction**

The BLB file contains 17 supplementary graphics containers that are **not referenced** in the
main level metadata table. These containers store additional tileset and UI graphics used
by levels at runtime (e.g., end-of-level summary screens, score displays, bonus room backgrounds).

### Purpose

These are **NOT separate bonus room levels** - they are supplementary asset packs containing:
- End-of-level summary text/graphics
- Score and UI overlays  
- Background decorations for bonus areas
- Additional tile graphics not in the main level data

The graphics are layered compositions that can be rendered over the main gameplay area.

### Location and Discovery

These containers exist in "gaps" between level data - sectors not referenced by any
level's primary/secondary/tertiary offsets. They were discovered by scanning unreferenced
sectors and finding valid container TOC signatures.

| Container | File Offset | Size | World |
|-----------|-------------|------|-------|
| SCIE_bonus | 0x0098F800 | 256 KB | Science |
| TMPL_bonus | 0x00EB7000 | 252 KB | Temple |
| BOIL_bonus | 0x01355000 | 248 KB | Boiler |
| SNOW_bonus | 0x0173E800 | 245 KB | Snow |
| FOOD_bonus | 0x01AFB800 | 252 KB | Food |
| GLID_bonus | 0x01ED8800 | 255 KB | Gliding |
| CAVE_bonus | 0x02297800 | 254 KB | Cave |
| WEED_bonus | 0x025AB000 | 254 KB | Weed |
| EGGS_bonus | 0x02880800 | 253 KB | Eggs |
| CLOU_bonus | 0x02D1B000 | 252 KB | Clouds |
| SEVN_bonus | 0x032D7000 | 248 KB | 1970s |
| SOAR_bonus | 0x0357A800 | 245 KB | Soaring |
| CRYS_bonus | 0x0397F800 | 244 KB | Crystal |
| WIZZ_bonus | 0x03E93000 | 250 KB | Wizard |
| RUNN_bonus | 0x03FFC800 | 249 KB | Runner |
| KLOG_bonus | 0x044AA800 | 236 KB | Klogg |
| EVIL_bonus | 0x047DC800 | 654 KB | Evil Engine |

### Container Structure

Each bonus room container has an identical 11-asset structure:

| Asset ID | Hex | Size Range | Description |
|----------|-----|------------|-------------|
| 100 | 0x064 | 36 bytes | Tile header (same format as secondary Asset 100) |
| 200 | 0x0C8 | 1.5-1.7 KB | Tilemap data (layer definitions) |
| 201 | 0x0C9 | 460 bytes | Layer entries |
| 300 | 0x12C | 139-144 KB | **Tile pixel data (8bpp, 16×16 tiles)** |
| 301 | 0x12D | 530-675 bytes | Palette index per tile |
| 302 | 0x12E | 530-675 bytes | Tile size flags |
| 400 | 0x190 | 2.1 KB | Palette container (4 palettes) |
| 401 | 0x191 | 16-32 bytes | Palette configuration |
| 600 | 0x258 | 52 KB | Sprites (RLE encoded) |
| 601 | 0x259 | 38 KB | SPU audio samples |
| 602 | 0x25A | 32 bytes | Audio metadata |

### Tile Data Format (Asset 300)

Same format as regular level tiles, verified via Ghidra decompilation of `CopyTilePixelData`:

- **8bpp indexed pixels** (not 4bpp)
- **16×16 tiles**: 256 bytes each (16 rows × 16 bytes)
- **Tile count**: Stored at Asset 100 offset +0x10 (u16)
- **Total size**: `tile_count × 256` bytes

Example: SCIE_bonus has 562 tiles × 256 bytes = 143,872 bytes (matches Asset 300 size exactly)

### Layer Rendering (EXTRACTION-VERIFIED 2026-01-04)

Bonus room layers can be fully rendered using Assets 200+201+300+301+400.
All 17 bonus rooms render to 480×256 pixels (30×16 tiles).

**Asset 200 - Tilemap Container (Sub-TOC format):**
```
Offset  Size   Description
------  ----   -----------
0x00    u32    Layer count
0x04+   12×N   Sub-entries (N = count)

Each sub-entry:
  0x00  u32    Layer index (0, 1, 2, ...)
  0x04  u32    Tilemap data size in bytes
  0x08  u32    Tilemap data offset from Asset 200 start
```

Each tilemap is an array of u16 tile indices:
- **Bits 0-12**: Tile index (1-based, 0 = transparent)
- **Bit 13**: Horizontal flip
- **Bit 14**: Vertical flip
- **Bit 15**: Unknown

**Asset 201 - Layer Entries (92 bytes each, EXTRACTION-VERIFIED):**
```
Offset  Size   Description
------  ----   -----------
0x00    u16    X offset (in tiles)
0x02    u16    Y offset (in tiles)
0x04    u16    Layer width (in tiles)
0x06    u16    Layer height (in tiles)
0x08    u16    Level width (in tiles, from Asset 100)
0x0A    u16    Level height (in tiles)
0x10    u32    Scroll factor X (0x8000 = 1.0)
0x14    u32    Scroll factor Y
0x26    u8     Layer type (0=normal, 2=foreground?)
0x2C    u8     Background R
0x2D    u8     Background G
0x2E    u8     Background B
```

**Rendering process:**
1. Parse Asset 100 for level dimensions (offset +0x08, +0x0A) and tile count (+0x10)
2. Extract tiles from Asset 300 using palette indices from Asset 301
3. For each layer in Asset 201:
   - Get tilemap from Asset 200 sub-TOC
   - Render tiles at (x_offset + x, y_offset + y) positions
   - Apply horizontal/vertical flip from tile index high bits
4. Composite layers in order (layer 0 first, then 1, etc.)

### Observation: Identical Sprite/Audio Sizes

All 17 bonus rooms have nearly identical Asset 600 and Asset 400 sizes:
- Asset 600 (sprites): 52,368 bytes in all containers
- Asset 400 (palettes): 2,100 bytes in all containers (4 palettes × 512 bytes + header)

This suggests bonus rooms share common sprite/palette templates with world-specific tile graphics.

### Loading Mechanism (TBD)

How the game references these unreferenced containers is not yet understood:
- They are NOT in the level metadata table
- May be loaded via hardcoded sector offsets
- May be referenced by the in-level bonus room trigger code
- Requires further investigation of bonus room loading functions

## Sector Files

The sector files extracted to `sectors/` contain preview/loading graphics:
- Named by 4-character level code (e.g., `PHRO`, `SCIE`, `BOIL`)
- Smaller than primary data (10-20KB typically)
- Likely used for level select thumbnails and loading screens
- Format appears to be raw or RLE-compressed image data

## Loading Process

Based on decompiled code in `LevelDataParser.c` and **verified via PCSX-Redux MCP debugging**:

1. Game reads BLB header from sectors 0-1 (0x1000 bytes) into RAM at 0x800AE3E0
2. When loading a level:
   - Read game mode from header+0xF36 (3=level mode, 6=special mode)
   - Read level index from header+0xF92
   - Look up level metadata at header + (index × 0x70)
   - Get sector offset/count from bytes 0x00-0x03 of level entry
   - Call `CdBLB_ReadSectors` to load primary data from BLB
   - Parse TOC at loaded data to locate asset pointers:
     - Entry count at offset 0x00 (u32)
     - Each entry: type (u32), size (u32), offset (u32)
   - Store pointers in LevelDataContext structure
3. For secondary/tertiary:
   - Use secondary_offset/count and tertiary_offset/count from metadata
   - Parse similar TOC structures with different asset types

**Verified level load example (Science Centre):**
- Level index: 2
- Primary sector offset: 0x0F2F (3887)
- Primary sector count: 0x10A7 (4263)
- Loaded 3 TOC entries totaling ~650KB of level data

## Code References

- `src/LevelDataParser.c`: Main parsing logic
- `src/LoadBLBHeader.c`: Header loading
- `src/BLBHeaderAccessors.c`: Accessor functions for header fields
- `scripts/blb.py`: Python parsing library
- `scripts/extract_blb.py`: Extraction tool

## Primary.bin Internal Structure (PARTIALLY DECODED 2026-01-04)

The primary.bin files contain the main level geometry and collision data.
After parsing the TOC, the game accesses three asset types:

### Asset 0x258 - World/Level Graphics (RLE Sprite Container)

The largest asset, containing level background/decoration sprites. Uses same RLE
format as tertiary sprite data.

**VERIFIED 2025-01-XX via Ghidra analysis of GetFrameMetadata (0x8007bebc)**

**Container Structure:**
```
Offset  Size    Description
------  ----    -----------
0x00    u32     Sprite count (typically 20-82 per level)
0x04+   12×N    Entry table
```

**Entry Table (12 bytes each):**
```
Offset  Size    Description
------  ----    -----------
0x00    u32     Sprite ID
0x04    u32     Sprite data size in bytes
0x08    u32     Sprite data offset from asset start
```

**Sprite Header (24 or 36 bytes):**
```
Offset  Size    Type    Description
------  ----    ----    -----------
0x00    2       u16     Sprite type: 1=standard, 2=linked
0x02    2       u16     Header size: 24 for type 1, 36 for type 2
0x04    2       u16     Frames end offset (header_size + frame_count × frame_entry_size)
0x06    2       u16     Padding (always 0)
0x08    4       u32     RLE data size (approximate, may not match exactly)
0x0C    4       u32     Sprite ID (may not match TOC for type 2)
0x10    2       u16     Frame count
0x12    2       u16     Padding (always 0)
0x14    2       u16     Unknown flag (0 or 1)
0x16    2       u16     CLUT placeholder (always 0x815D - GARBAGE, ignored at runtime)
0x18    12      bytes   Extended header (only for type 2)
```

**Frame Entry (36 bytes for type 1, 72 bytes for type 2):**
```
Offset  Size    Type    Description
------  ----    ----    -----------
0x00    2       u16     Unknown (always 0)
0x02    2       u16     Unknown (always 0)
0x04    2       u16     Unknown (1 or 2)
0x06    2       s16     Render X offset (signed, for sprite positioning)
0x08    2       s16     Render Y offset (signed)
0x0A    2       u16     Render width (sprite visible width)
0x0C    2       u16     Render height (sprite visible height)
0x0E    2       u16     Unknown (0-10)
0x10    2       u16     Unknown (always 0)
0x12    2       s16     Hitbox X offset (signed, collision box position)
0x14    2       s16     Hitbox Y offset (signed)
0x16    2       u16     Hitbox width (collision box width)
0x18    2       u16     Hitbox height (collision box height)
0x1A    6       bytes   Padding (always 0)
0x20    4       u32     RLE data offset (from frames_end)
```

For type 2 sprites, the frame entry is 72 bytes (two 36-byte blocks for dual hitbox/layers).

**Key Formulas (verified for all 20 sprites in SCIE level):**
```
frames_end = header_size + frame_count × frame_entry_size
frame_entry_size = 36 (type 1) or 72 (type 2)
rle_absolute_offset = sprite_offset + frames_end + frame.rle_offset
```

**GetFrameMetadata (0x8007bebc) - Key offsets used:**
- `frame_index × 0x24 + base`: Frame entry lookup (0x24 = 36 bytes)
- `iVar4 + 0x0A`: Render width (offset 10)
- `iVar4 + 0x0C`: Render height (offset 12)
- `iVar4 + 0x20`: RLE data offset (offset 32)

**RLE Pixel Data:**
Located at (sprite_offset + frames_end + frame.rle_offset).
- First u16 is NOT row count (varies independently of height)
- Uses same RLE format as other sprite data
- Control word (u16): bit 15=newline, bits 8-14=skip, bits 0-7=copy count
- Raw 8bpp indexed pixels following control words

**Example (SCIE level, Asset 600 from tertiary sub-TOC):**
- 20 sprite entries
- Type 1 sprites: 17 entries (36-byte frame entries)
- Type 2 sprites: 3 entries (72-byte frame entries, linked/complex)
- Frame counts range from 1 to 15 per sprite

### Asset 0x259 - Collision/Physics Data

Contains collision geometry for the level. Structure partially understood.

**Container Structure:**
```
Offset  Size    Description
------  ----    -----------
0x00    u32     Entry count (typically 6-49 per level)
0x04+   12×N    Entry table (same format as 0x258)
```

**Entry Data Structure:**
```
Offset  Size    Description
------  ----    -----------
0x00    16      Zeros (header/padding)
0x10+   var     Collision data (format TBD)
```

The entry "flags" field appears to be an ID (like Asset 0x258).
The actual collision geometry format is not yet decoded.

**Example (PHRO level):**
- 37 collision entries
- Entry sizes: 480 - 8832 bytes
- First 16 bytes always zero

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
     - ctx+0x70: Asset 0x258 pointer (sprites)
     - ctx+0x74: Asset 0x259 pointer (audio samples)
     - ctx+0x78: Asset 0x259 size
     - ctx+0x7C: Asset 0x25A pointer (audio metadata)
4. Rendering functions access ctx+0x70 to draw sprites
5. `UploadAudioToSPU` (0x8007c088) uses ctx+0x74/0x78/0x7C to upload audio to SPU RAM
6. FindSpriteInTOC searches ctx+0x70 AND ctx+0x40 for sprite data

### LevelDataContext Structure (VERIFIED via Ghidra + PCSX-Redux MCP)

**NOTE: These addresses are for PAL version (SLES-01090).**

The context structure at GameState+0x84 (0x8009DCC4) is the central data structure for level loading.
It is initialized by `InitLevelDataContext` (0x8007A1BC) and populated by `LevelDataParser` (0x8007A62C)
and `LoadAssetContainer` (0x8007B074).

```
Offset  WIdx  Size  Type    Field                   Description
------  ----  ----  ----    -----                   -----------
# Asset Pointers (populated by LoadAssetContainer from sub-TOC)
0x00    [0]   4     int     subBlockFlag            Set to subBlockIndex or 1, indicates loaded sub-block
0x04    [1]   4     ptr     assetGeometry100        ID 100: Geometry/tiles pointer
0x08    [2]   4     ptr     assetGeometry101        ID 101: Unknown geometry pointer
0x0C    [3]   4     ptr     assetSprite200          ID 200: Sprites pointer
0x10    [4]   4     ptr     assetSprite201          ID 201: Animations pointer (0xC9)
0x14    [5]   4     ptr     asset300                ID 300: Unknown asset pointer
0x18    [6]   4     ptr     asset301                ID 301: Unknown asset pointer (0x12D)
0x1C    [7]   4     ptr     asset302                ID 302: Unknown asset pointer (0x12E)
0x20    [8]   4     ptr     assetObject400          ID 400: Object data pointer
0x24    [9]   4     ptr     assetObject401          ID 401: Object data pointer (0x191)
0x28    [10]  4     ptr     asset303                ID 303: Unknown asset pointer (0x12F)
0x2C    [11]  4     ptr     asset500                ID 500: Unknown asset pointer (0x1F4)
0x30    [12]  4     ptr     asset503                ID 503: Unknown asset pointer (0x1F7)
0x34    [13]  4     ptr     asset504                ID 504: Unknown asset pointer (0x1F8)
0x38    [14]  4     ptr     asset501                ID 501: Unknown asset pointer (0x1F5)
0x3C    [15]  4     ptr     asset502                ID 502: Unknown asset pointer (0x1F6)
0x40    [16]  4     ptr     assetLevel600           ID 600: Level geometry pointer (0x258)
0x44    [17]  4     u32     assetLevel600Size       Size of level geometry in bytes
0x48    [18]  4     ptr     assetAudio601           ID 601: Audio samples pointer (0x259) - uploaded to SPU
0x4C    [19]  4     u32     assetAudio601Size       Size of audio sample data in bytes
0x50    [20]  4     ptr     assetAudioMeta602       ID 602: Audio metadata pointer (0x25A)
0x54    [21]  4     ptr     assetAudio700           ID 700: Audio/music pointer (0x2BC)
0x58    [22]  4     u32     assetAudio700Size       Size of audio data in bytes

# Context State (set by InitLevelDataContext and LevelDataParser)
0x5C    [23]  4     ptr     blbHeaderBuffer         Pointer to BLB header (→ 0x800AE3E0)
0x60    [24]  1     u8      slidingWindowIndex      Playback index byte (init 0xFF)
0x61          3     -       pad61                   (Padding - part of word access)
0x64    [25]  4     ptr     loaderCallback          CD loader callback (→ 0x80020848)
0x68    [26]  4     ptr     primaryDataBuffer       Primary TOC buffer pointer (set by LevelDataParser)
0x6C    [27]  4     ptr     secondaryDataBuffer     Secondary container buffer pointer

# Primary TOC Asset Pointers (set by LevelDataParser, separate from sub-TOC assets)
0x70    [28]  4     ptr     primaryLevel600         Primary TOC ID 600 pointer
0x74    [29]  4     ptr     primaryAudio601         Primary TOC ID 601 pointer (audio samples)
0x78    [30]  4     u32     primaryAudio601Size     Primary 601 size
0x7C    [31]  4     ptr     primaryAudioMeta602     Primary TOC ID 602 pointer (audio metadata)
```

**Total structure size: 0x80 (128) bytes**

#### Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `InitLevelDataContext` | 0x8007A1BC | Sets blbHeaderBuffer [0x17], loaderCallback [0x19], slidingWindowIndex [0x18]=0xFF |
| `LevelDataParser` | 0x8007A62C | Clears all fields, parses primary TOC, sets [0x1A-0x1F], calls LoadAssetContainer |
| `LoadAssetContainer` | 0x8007B074 | Parses sub-TOC, populates asset pointers [0x00-0x16] based on asset IDs |
| `LoadTileDataToVRAM` | 0x80025240 | Uploads tile pixel data to VRAM after container load |
| `CdBLB_ReadSectors` | 0x80038BA0 | Low-level CD read, called via loaderCallback |

#### Key Accessor Functions (Ghidra-named)

| Function | Address | Returns | Description |
|----------|---------|---------|-------------|
| `GetLayerCount` | 0x8007B6C8 | u16 | Layer count from Asset 200 |
| `GetLayerEntry` | 0x8007B700 | ptr | Layer entry from Asset 201 (92-byte stride) |
| `GetTilemapDataPtr` | 0x8007B6DC | ptr | Tilemap data pointer from Asset 200 sub-TOC |
| `GetTotalTileCount` | 0x8007B53C | u16 | Sum of tile counts from Asset 100 |
| `CopyTilePixelData` | 0x8007B588 | void | Copy tile pixel data (8bpp) to buffer |
| `GetTileSizeFlags` | 0x8007B6BC | ptr | Asset 302 pointer (per-tile flags) |
| `GetPaletteIndices` | 0x8007B6B0 | ptr | Asset 301 pointer (palette per tile) |
| `GetPaletteDataPtr` | 0x8007B4F8 | ptr | Palette color data from Asset 400 |
| `GetPaletteGroupCount` | 0x8007B4D0 | u8 | Palette count from Asset 400 |
| `GetAnimatedTileData` | 0x8007B658 | ptr | Animated tile lookup from ctx[11] |
| `LoadTileDataToVRAM` | 0x80025240 | void | Upload tiles to VRAM, build sprite info array |
| `InitTilemapLayer16x16` | 0x80017540 | ptr | Init 16x16 tilemap layer with SPRT_16 primitives |

#### Asset ID Mapping (LoadAssetContainer)

The sub-TOC contains entries with asset IDs that map to specific context offsets:

| Asset ID | Hex | Word Index | Field Name | Description |
|----------|-----|------------|------------|-------------|
| 100 | 0x64 | [1] | tileHeader | Tile header (36 bytes, tile counts at +0x10/0x12/0x14) |
| 101 | 0x65 | [2] | unknown101 | Unknown (12 bytes, sparse: {1-4, 0-1, 0}) - only 8 levels |
| 200 | 0xC8 | [3] | tilemapContainer | Tilemap sub-TOC (layer count + data offsets) |
| 201 | 0xC9 | [4] | layerEntries | Layer definition entries (92 bytes each) |
| 300 | 0x12C | [5] | tilePixels | Tile pixel data (8bpp indexed) |
| 301 | 0x12D | [6] | paletteIndices | Palette index per tile (1 byte each) |
| 302 | 0x12E | [7] | tileSizeFlags | Per-tile flags: bit0=semi-trans, bit1=8x8, bit2=skip |
| 303 | 0x12F | [10] | animatedTileData | Animated tile lookup table |
| 400 | 0x190 | [8] | paletteContainer | Palette container (256-color palettes) |
| 401 | 0x191 | [9] | paletteAnimData | Palette animation data (4 bytes per palette) |
| 500 | 0x1F4 | [11] | spriteMetadata | Sprite metadata |
| 501 | 0x1F5 | [14] | spriteRLEData | Sprite RLE pixel data |
| 600 | 0x258 | [16-17] | spuAudioData + size | SPU audio samples (ADPCM) |
| 601 | 0x259 | [18-19] | spuAudioData2 + size | SPU audio samples (secondary container) |
| 602 | 0x25A | [20] | spuAudioConfig | SPU volume/pan per sample (4 bytes: u16 vol, u16 pan) |

#### Loader Callback Chain

```
LoadAssetContainer/LevelDataParser
    └─→ (*loaderCallback)(sectorOffset, sectorCount, destBuffer)
            │ (function pointer at ctx+0x64)
            └─→ 0x80020848 (thin wrapper)
                    └─→ CdBLB_ReadSectors(g_GameBLBSector + offset, count, buffer)
                            └─→ PSY-Q: CdIntToPos → CdControl → CdRead → CdReadSync
```

**Verified Runtime Example (Science Centre / SCIE, level index 2):**

Captured while Science Centre was loaded in PCSX-Redux:

| Field | Address/Value | Description |
|-------|--------------|-------------|
| ctx | 0x8009DCC4 | LevelDataContext base |
| header | 0x800AE3E0 | BLB header in RAM |
| headerOffset | 0x0E (14) | State window offset |
| loadCallback | 0x80020848 | CD read function |
| tocPtr | 0x800AF3E0 | Loaded TOC (3 entries) |
| asset258 | 0x800AF408 | 524,212 bytes geometry |
| asset259 | 0x8012F3BC | 126,256 bytes collision |
| asset25A | 0x8014E0EC | 148 bytes palette |

**TOC Contents for Science Centre:**
```
Entry 0: type=0x258, size=524,212 bytes, offset=0x28
Entry 1: type=0x259, size=126,256 bytes, offset=0x07FFDC  
Entry 2: type=0x25A, size=148 bytes, offset=0x09ED0C
```

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
