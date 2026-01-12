# GAME.BLB Data Format Documentation

> ⚠️ **DEPRECATED**: This document has been split into smaller, focused chapters.
> See the new documentation structure:
> - [BLB Overview](../blb/README.md)
> - [Header Format](../blb/header.md)
> - [Level Metadata](../blb/level-metadata.md)
> - [Asset Types](../blb/asset-types.md)
> - [TOC Format](../blb/toc-format.md)
> - [Tiles and Tilemaps](../systems/tiles-and-tilemaps.md)
> - [Sprites](../systems/sprites.md)
> - [Audio](../systems/audio.md)
> - [LevelDataContext](../reference/level-data-context.md)
>
> This file is kept for reference but will not be updated.

---

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
- **Primary**: Level geometry, collision, and palette data (Asset 600/601/602)
- **Secondary**: Tile graphics, palettes, and tile metadata (Asset 100/200/201/300/301/302/400/401)
- **Tertiary**: Stage-specific sprites (Asset 600), entities (Asset 501), layers (Asset 201), and audio (Asset 500/502/503/700)

### Verified Level Sizes (via PCSX-Redux MCP, PAL / SLES-01090)

| Level | ID | Index | Geometry (0x258) | Collision (0x259) | Palette (0x25A) | Total |
|-------|-----|-------|------------------|-------------------|-----------------|-------|
| Menu | MENU | 0 | 53,308 B | 11,420 B | 24 B | ~65 KB |
| Skullmonkey Gate | PHRO | 1 | - | - | - | - |
| Science Centre | SCIE | 2 | 524,212 B | 126,256 B | 148 B | ~650 KB |
| Monkey Shrines | TMPL | 3 | 510,108 B | 188,288 B | 196 B | ~699 KB |

*Note: TMPL is used as the demo/attract mode level when idle at menu.*

## BLB Header Reference

The BLB header (first 0x1000 bytes) contains metadata for all levels.

**PAL/NTSC-US Header Layout:**
```
Offset   Size   Description
------   ----   -----------
0x000    0xB60  Level Metadata Table (26 entries × 0x70 bytes)
0xB60    0x16C  Movie Table (13 entries × 0x1C bytes)
0xCC8    0x004  Padding (4 zeros, between movie and sector tables)
0xCD0    0x200  Sector Table (32 entries × 0x10 bytes, count at 0xF33)
0xECC    0x044  Mode 6 Sector Table (17 entries × 4 bytes, overlaps sectors[31])
0xF10    0x021  Credits Sequence Table (2 complete entries × 0x0C bytes + overlap)
0xF31    0x001  Level Count (u8, value=26)
0xF32    0x001  Movie Count (u8, value=13)
0xF33    0x001  Sector Table Entry Count (u8)
0xF34    0x0CC  Playback Sequence Data (mode array at 0xF36, index array at 0xF92)
```

**JP Version Differences (SLPS-01501):**
The JP version has different offsets due to a credits bug fix (see [TCRF](https://tcrf.net/Skullmonkeys)):
- **Sector table at 0xCB0** (32 bytes earlier than PAL's 0xCD0)
- **12 movies** vs PAL's 13 (intro spliced into 1 movie file)
- **CRED moved from index 2 to index 28** - avoids triggering credits between intro movies
- **Different field order** in sector table entries (offset/count at start, not end)

**Note on Mode 6 Sector Table:** The table starts at 0xECC but entry[0] overlaps with
`sectors[31].sector_offset` and `sectors[31].sector_count` (the OVER/game-over screen).
The game accesses entries as: `header + (index * 4) + 0xECC`. Entries 1-16 are stored
at 0xED0-0xF0F (64 bytes). See "Mode 6 Playback" section below.

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
0x02     u8     display_timeout - Max display time in seconds (VERIFIED)
0x03     char[5] Code (4-char null-terminated, e.g., "PIRA", "MENU")
0x08     char[4] Short name (truncated description)
0x0C     u16    Sector offset in BLB
0x0E     u16    Sector count
```

**Display Timing Fields (VERIFIED via Ghidra):**

For loading/splash screens (entry_flags 0x03 or 0x05), two fields control display timing:
- `entry_flags` (0x01): **Minimum display time** in seconds before player can skip
- `display_timeout` (0x02): **Maximum display time** (timeout) in seconds

The values are multiplied by 60 (VSync rate) to get frame counts in `DisplayLoadingScreen` (0x800399a8).

**Special case when display_timeout = 99 (0x63):**
- Enables "Game Over" mode with different button handling
- X button returns 0 (retry), Start returns 1 (continue to menu)
- Used for game over screens to allow restart/continue choice

**Observed values:**
| display_timeout | Meaning |
|-----------------|---------|
| 0x00 | No display (skip immediately, used for normal levels) |
| 0x0A (10) | 10 seconds timeout (loading screens, any button skips) |
| 0x63 (99) | 99 seconds + special Game Over button handling |

**Entry type patterns (PAL version):**
- `entry_flags=0x00`: Level loading screens (level_index = 0-25)
- `entry_flags=0x05`: Special loading screens (PIRA=pirates intro, LEGL=legal)
- `entry_flags=0x03`: Game over screens (display_timeout=0x63)

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

## Mode 6 Sector Table (0xECC-0xF0F)

The Mode 6 sector table stores CD sector locations for special playback sequences
(e.g., inter-level transitions, world intros). It has 17 entries, each 4 bytes:

```
Offset   Size   Description
------   ----   -----------
0x00     u16    Sector offset in BLB (CD sector number)
0x02     u16    Sector count (typically 109-132 sectors, ~220-270 KB)
```

**Memory Layout:**
- Entry[0] at 0xECC-0xECF overlaps with `sectors[31].sector_offset/count` (OVER screen)
- Entries[1-16] at 0xED0-0xF0F (64 bytes, formerly "unknown_ed0" array)

**Game Code Access (from LevelDataParser at 0x8007A62C):**
```c
if (mode == 6) {
    u32 addr = header_base + (level_index * 4) + 0xECC;
    u16 sector_offset = *(u16*)(addr + 0);
    u16 sector_count = *(u16*)(addr + 2);
}
```

**Playback Sequence Integration:**
- Mode array at 0xF36 contains mode values (0-6)
- Index array at 0xF92 contains level indices for mode 6 lookups
- When mode=6, the index value (0-16) is used to access the Mode 6 sector table

**Example Mode 6 Entries (PAL GAME.BLB):**
| Index | Sector | Count | Bytes | Content |
|------:|-------:|------:|------:|:--------|
| 0 | 4,895 | 118 | 241,664 | Level asset data (11 tilesets) |
| 1 | 7,534 | 116 | 237,568 | Level asset data |
| 2 | 9,898 | 115 | 235,520 | Level asset data |
| ... | ... | ... | ... | ... |
| 16 | 36,793 | 132 | 270,336 | Level asset data |

The data at these sectors appears to be level tileset/asset headers with the structure:
- u32 tileset_count (e.g., 11)
- u32 unknown (e.g., 100)
- Additional asset counts and offsets

## Level Metadata Entry (0x70 bytes)

Each level has a metadata entry at offset `index × 0x70`:

```
Offset   Size   Description
------   ----   -----------
# Primary data pointers (0x00-0x0B)
0x00     u16    Primary sector offset (CONFIRMED)
0x02     u16    Primary sector count (CONFIRMED)
0x04     u32    Primary buffer size (CONFIRMED - returned by GetPrimaryBufferSize @ 8007a5cc)
0x08     u32    Entry[1] offset (CONFIRMED - offset to Asset 601/collision in primary TOC)

# Level identification (0x0C-0x0D)
0x0C     u8     Level asset index (0-25)
0x0D     u8     Password-selectable flag (CONFIRMED 2026-01-06)
                1 = Level can be selected via password system
                0 = Not directly selectable (bosses, special modes, later worlds)
                Flag=1 levels: SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED (8 levels)
                Read by GetLevelFlagByIndex (0x8007aa28), used by InitGameState (0x8007cd34)

# Stage count and tertiary data offsets (0x0E-0x1D)
0x0E     u16    Stage count (1-6, number of stages in this level)
0x10     u16[6] Tertiary data offsets (tert_data_off[i] << 5 = size for stage i)
0x1C     u16    Padding (always 0)

# Secondary sector locations (0x1E-0x39) - per-stage arrays
0x1E     u16[6] Secondary sector offsets (sec_sector_off[i] for stage i)
0x2A     u16    Padding (always 0)
0x2C     u16[6] Secondary sector counts (sec_sector_cnt[i] for stage i)
0x38     u16    Padding (always 0)

# Tertiary sector locations (0x3A-0x55) - per-stage arrays
0x3A     u16[6] Tertiary sector offsets (tert_sector_off[i] for stage i)
0x46     u16    Padding (always 0)
0x48     u16[6] Tertiary sector counts (tert_sector_cnt[i] for stage i)
0x54     u16    Padding (always 0)

# Level strings (0x56-0x6F)
0x56     char[5]  Level ID (4-char code + null, e.g., "MENU")
0x5B     char[21] Level name (null-terminated, e.g., "Options Menu")
```

### Field Analysis (0x04-0x0B)

**u32@0x04 - Primary Buffer Size** (VERIFIED 2026-01-06):
- Returned by `GetPrimaryBufferSize()` at 0x8007a5cc
- Used in `InitializeAndLoadLevel` for memory allocation:
  ```c
  bufferSize = GetPrimaryBufferSize(ctx);
  tertiarySize = GetCurrentTertiaryDataSize(ctx);
  remaining = bufferSize - ALIGN16(tertiarySize);
  ```
- For special modes (mode 6), returns fixed 0x7d000 (512KB)
- Values range from ~510KB to ~1.3MB depending on level complexity

**u32@0x08 - Entry[1] Offset** (VERIFIED 2026-01-06):
- Offset to Asset 601 (collision/secondary TOC pointer) within primary buffer
- Used to calculate `ctx[0x1b] = primary_buffer + entry1_offset`
- Matches `Entry[1].offset` in primary TOC for all 26 levels
- Asset 601 contains the collision data TOC

### Stage Data Organization (VERIFIED 2026-01-06)

Each level contains 1-6 stages. The sector location arrays are **parallel**:
- Stage `i` uses: `sec_sector_off[i]` + `sec_sector_cnt[i]` for secondary data
- Stage `i` uses: `tert_sector_off[i]` + `tert_sector_cnt[i]` for tertiary data

Example (Options with 6 stages):
```
Stage 0: sec[233, 67 sectors], tert[300, 488 sectors]
Stage 1: sec[788, 62 sectors], tert[850, 3 sectors]
...
```

### Data Interleaving Pattern

Level data sectors are interleaved on disc for streaming:
```
PRIMARY → SECONDARY → TERT[0] → SEC_SUB[0] → TERT[1] → SEC_SUB[1] → ...
```

Example (MENU level): 
- 201-232 (PRIMARY) → 233-299 (SECONDARY) → 300-787 (TERT[0]) → 788-849 (SEC_SUB[0]) → ...

## Stage Structure (VERIFIED 2026-01-05)

Each "level" in the BLB actually contains multiple **stages** (individual gameplay areas).
The game has **104 total stages** across 26 levels.

### Stage Data Organization

| Data Type | Scope | Contents |
|-----------|-------|----------|
| **Primary** | Per-level (shared) | Level geometry container |
| **Secondary base** | Per-level (shared) | Base tileset, palettes |
| **Secondary sub-blocks** | Per-stage | Stage-specific tile overrides |
| **Tertiary sub-blocks** | Per-stage | Sprites, layers, audio |

### Stage Counts by Level

| Level | ID | Name | Stages | Pattern |
|-------|-----|------|--------|---------|
| 0 | MENU | Options | 6 | Menu screens |
| 1-3 | PHRO/SCIE/TMPL | World 1 | 3-5 | Stages + bonus |
| 4 | FINN | Swimming | 1 | Special mode |
| 5 | MEGA | Boss | 1 | Boss fight |
| 6-8 | BOIL/SNOW/FOOD | World 2 | 4-6 | Stages + bonus |
| 9 | HEAD | Boss | 1 | Boss fight |
| 10-14 | BRG1-EGGS | World 3-4 | 4-6 | Stages + bonus |
| 15 | GLEN | Boss | 1 | Boss fight |
| 16-20 | CLOU-CSTL | World 5-6 | 5-6 | Stages + bonus |
| 21 | WIZZ | Boss | 1 | Boss fight |
| 22 | RUNN | Runner | 2 | Special mode |
| 23 | MOSS | World 7 | 6 | Stages + bonus |
| 24 | KLOG | Final Boss | 1 | Boss fight |
| 25 | EVIL | Evil Engine | 5 | Final world |

### Secondary/Tertiary Sector Pairing (VERIFIED 2026-01-07)

**CRITICAL:** Each stage's tertiary block uses the secondary block that **precedes it** 
in the sector layout, NOT its corresponding stage-indexed secondary.

**Sector Layout Pattern:**
```
Base secondary    → Stage 0 tertiary → Stage 0 secondary → Stage 1 tertiary → 
Stage 1 secondary → Stage 2 tertiary → Stage 2 secondary → ... → 
Stage 4 secondary → Stage 5 tertiary (if exists)
```

**Pairing Rules:**
| Tertiary Block | Uses Secondary Block |
|----------------|---------------------|
| Stage 0 tertiary | **Base secondary** |
| Stage 1 tertiary | Stage 0 secondary |
| Stage 2 tertiary | Stage 1 secondary |
| Stage N tertiary | Stage (N-1) secondary |
| Stage 5 tertiary (if no sec_sub[4]) | Stage 4 secondary |

**Example (Level 6 BOIL, sectors):**
```
Base secondary:    8053-8200
Stage 0 tertiary:  8201-8382  ← uses base secondary (974 tiles)
Stage 0 secondary: 8383-8524
Stage 1 tertiary:  8525-8727  ← uses stage 0 secondary (783 tiles)
Stage 1 secondary: 8728-8900
...
Stage 5 tertiary:  9881-9897  ← uses stage 4 secondary (466 tiles)
```

**Why This Matters:**
Each secondary contains a different tileset with different tile counts. 
Using the wrong secondary results in "jumbled" layer rendering because the 
tilemap indices in the tertiary reference tiles from the preceding secondary.

### Identifying Bonus Rooms

Bonus rooms can be identified by:
- Small tertiary block size (< 20 sectors, < 40 KB)
- Few sprites (typically 0-3)
- Usually the last stage in a multi-stage level

### Extraction Statistics

Full extraction yields:
- **1,334 sprites** (from all tertiary blocks)
- **26,208 tiles** (from secondary segments)
- **571 layers** (from all tertiary blocks)

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

**IMPORTANT:** Asset 600 (0x258) appears in BOTH segments with different content:
- **Primary Asset 600**: Level geometry/world data (background graphics)
- **Tertiary Asset 600**: RLE-encoded character sprites with embedded palettes

This was verified via runtime analysis on 2026-01-05.

### Primary Segment Assets
| Type | Hex | Structure | Description | Typical Size |
|------|-----|-----------|-------------|--------------|
| 600 | 0x258 | CONTAINER | Level geometry/world data | 500KB-1MB |
| 601 | 0x259 | CONTAINER | **Audio sample bank** (same format as Secondary 601) | 30-150KB |
| 602 | 0x25A | RAW | Palette/color data (15-bit PSX) | 24-200 bytes |

**Note:** Asset 601 in PRIMARY segment uses the same audio sample format as Secondary 601.
The `GetAsset601Ptr` function selects between primary (ctx+0x74) and secondary (ctx+0x48)
based on mode flag at ctx+0x04. See Audio System section below for details.

### Secondary Segment Assets (Tiles) - VERIFIED

| Type | Hex | Structure | Description |
|------|-----|-----------|-------------|
| 100 | 0x064 | RAW | Tile header (36 bytes, contains tile counts) |
| 300 | 0x12C | RAW | Tile pixel data (8bpp indexed) |
| 301 | 0x12D | RAW | Palette index per tile (1 byte/tile) |
| 302 | 0x12E | RAW | Tile size/rendering flags (1 byte/tile) |
| 400 | 0x190 | CONTAINER | Palette container (256-color palettes) |
| 401 | 0x191 | RAW | Animation/palette configuration |
| 601 | 0x259 | CONTAINER | **Audio sample bank** - SPU ADPCM samples + TOC |
| 602 | 0x25A | RAW | **Audio volume/pan table** - per-sample settings |

### Tertiary Segment Assets (Sprites, Layers, Audio) - VERIFIED 2026-01-07

| Type | Hex | Structure | Description |
|------|-----|-----------|-------------|
| 100 | 0x064 | RAW | Duplicate tile header |
| 101 | 0x065 | RAW | **Segment variant flag** (12 bytes, values 1-4) |
| 200 | 0x0C8 | RAW | Tilemap container header (layer count) |
| 201 | 0x0C9 | RAW | Layer entries (92 bytes each) |
| 302 | 0x12E | RAW | Duplicate tile flags (size = total_tiles) |
| 401 | 0x191 | RAW | **Palette animation config** (4 bytes per palette) |
| 500 | 0x1F4 | RAW | **Tile attribute map** - collision/trigger data (8-byte header + 1 byte/tile) |
| 501 | 0x1F5 | RAW | **Entity placement data** (24-byte structures, count in header[0x1E]) |
| 502 | 0x1F6 | RAW | **VRAM rectangles** (16-byte entries, count in header[0x1C]) |
| 503 | 0x1F7 | RAW | **Animation offset table** - ToolX sequence data (TOC + frame data) |
| 504 | 0x1F8 | RAW | **Vehicle path data** (64-byte waypoints, FINN/RUNN only) |
| 600 | 0x258 | CONTAINER | **RLE sprites with embedded palettes** |
| 700 | 0x2BC | RAW | **SPU audio samples** (ADPCM) with metadata |

#### Asset 100 - Tile Header (36 bytes, CODE-VERIFIED)

Verified via Ghidra decompilation of multiple accessor functions (2026-01-11).

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    3     u8[3]  Background RGB color (LoadBGColorFromTileHeader @ 0x80024678)
0x03    1     u8     Padding
0x04    3     u8[3]  Secondary RGB color  
0x07    1     u8     Padding
0x08    2     u16    Level width in tiles (GetLevelDimensions @ 0x8007b434, *16 for pixels)
0x0A    2     u16    Level height in tiles (GetLevelDimensions @ 0x8007b434, *16 for pixels)
0x0C    2     u16    Spawn X position in tiles (GetSpawnPosition @ 0x8007b458)
0x0E    2     u16    Spawn Y position in tiles (GetSpawnPosition @ 0x8007b458)
0x10    2     u16    16×16 tile count (GetTotalTileCount @ 0x8007b53c)
0x12    2     u16    8×8 tile count (GetTotalTileCount @ 0x8007b53c)
0x14    2     u16    Additional tile count (GetTotalTileCount @ 0x8007b53c)
0x16    2     u16    Vehicle Waypoint Count (VERIFIED: matches Asset 504 entries for FINN/RUNN)
0x18    2     u16    Level Flags bitfield (see Level Flags Analysis below)
0x1A    2     u16    Special Level ID (VERIFIED: 99 = FINN/SEVN special modes)
0x1C    2     u16    VRAM Rect Count (GetAsset100Field1C @ 0x8007b7c8, matches Asset 502)
0x1E    2     u16    Entity Count (GetEntityCount @ 0x8007b7a8, used by LoadEntitiesFromAsset501)
0x20    2     u16    Field 0x20 (values 0-6 observed, no accessor found, purpose unknown)
0x22    2     u16    Padding
```

**Cross-Asset Relationships (VERIFIED 2026-01-10):**
- `header[0x16]` = number of 64-byte waypoints in Asset 504 (FINN=78, RUNN=1)
- `header[0x1C]` = number of VRAM rectangles in Asset 502 (all levels match)
- `header[0x1E]` = number of 24-byte entities in Asset 501 (all levels match)
- Asset 302 (tile_flags) size = total_tiles (one flag byte per tile)
- Asset 401 size = palette_count × 4 (animation config per palette)

**Code reference:** `GetTotalTileCount` (0x8007b53c) computes total tiles as:
`*(u16*)(asset100 + 0x10) + *(u16*)(asset100 + 0x12) + *(u16*)(asset100 + 0x14)`

**Level Flags Analysis (offset 0x18, TENTATIVE):**

Per-level flag values extracted from all 26 levels (stage0):
| Level | Value | Bits Set |
|-------|-------|----------|
| BOIL, BRG1, CAVE, CRYS, CSTL, MOSS, SCIE, SNOW, SOAR | 0x0000 | (none) |
| SEVN | 0x0002 | bit 1 |
| FINN | 0x0006 | bits 1,2 |
| EGGS, GLID, WEED | 0x0008 | bit 3 |
| WIZZ | 0x0040 | bit 6 |
| HEAD | 0x0048 | bits 3,6 |
| RUNN | 0x0050 | bits 4,6 |
| EVIL | 0x0080 | bit 7 |
| GLEN | 0x0140 | bits 6,8 |
| MENU | 0x0240 | bits 6,9 |
| KLOG | 0x0400 | bit 10 |
| TMPL | 0x0868 | bits 3,5,6,11 |
| PHRO | 0x1000 | bit 12 |
| MEGA | 0x1048 | bits 3,6,12 |
| CLOU | 0x4000 | bit 14 |
| FOOD | 0x8048 | bits 3,6,15 |

**Observed bit patterns:**
- Bit 1 (0x02): Special gameplay mode (FINN, SEVN)
- Bit 4 (0x10): Runner vehicle mode (RUNN only)
- Bit 6 (0x40): Common in boss and special levels

#### Asset 300 - Tile Pixel Data (EXTRACTION-VERIFIED)

8-bit indexed pixel data with 16-byte row stride:

- **16×16 tiles**: 256 bytes each (16 rows × 16 bytes)
- **8×8 tiles**: 128 bytes each (8 rows × 16 bytes, only first 8 columns used)

Layout in file:
1. First `count_16x16` tiles (256 bytes each)
2. Then `count_8x8` tiles (128 bytes each)

**Rendering Note (CODE-VERIFIED via Ghidra):** The tilemap uses a consistent 16-pixel grid.
When rendering layers, 8×8 tiles are scaled 2× to 16×16 to fill their grid cell.
This is how the game handles mixed tile sizes - all tiles occupy a 16×16 space on screen
regardless of native size.

Verified in `RenderTilemapSprites16x16` (0x8001713c):
- Always uses `SetSprt16()` (16x16 sprite primitive) for all tiles
- X position advances by 0x10 (16) pixels per tile: `local_48 = local_48 + 0x10`
- Y position advances by 0x10 (16) pixels per row: `local_68 = local_68 + 0x10`
- No separate code path for 8x8 tiles - they are simply stretched by the GPU

**Extraction script:** `scripts/extract_all_graphics.py` extracts tiles using
Assets 300+301+302+400 to produce correctly colored PNG images.

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

### Tertiary Data (Entities, Sprites, Audio)
| Type | Hex | Structure | Description |
|------|-----|-----------|-------------|
| 100 | 0x064 | RAW | Duplicate tile header |
| 200 | 0x0C8 | RAW | Tilemap container (layer data) |
| 201 | 0x0C9 | RAW | Layer entries (92 bytes each) |
| 401 | 0x191 | RAW | Animation/palette config |
| 500 | 0x1F4 | RAW | **Tile attribute map** (collision/triggers) |
| 501 | 0x1F5 | RAW | **Entity placement data** (24-byte structures) |
| 502 | 0x1F6 | RAW | VRAM rectangles (16 bytes each, count in TileHeader+0x1C) |
| 503 | 0x1F7 | RAW | Animation offset table (ToolX sequence data) |
| 600 | 0x258 | CONTAINER | RLE sprites with embedded palettes |
| 700 | 0x2BC | RAW | SPU audio data |

#### Asset 500 - Tile Attribute Map (STRUCTURE-VERIFIED)

Per-tile collision and trigger data. Each byte represents properties for one tile position.

```
Offset  Size               Field         Description
------  ----               -----         -----------
0x00    4                  flags         Varies (often 0, sometimes large values)
0x04    2                  level_width   Level width in tiles
0x06    2                  level_height  Level height in tiles
0x08    width × height     tile_data     One byte per tile (row-major order)
```

**Size verification:** `8 + (level_width × level_height)` bytes matches actual asset sizes.

**Known attribute values:**
| Value | Hex  | Meaning |
|-------|------|---------|
| 0 | 0x00 | Empty/passable (air) |
| 2 | 0x02 | Solid/collision |
| 18 | 0x12 | Unknown trigger |
| 83 | 0x53 | Checkpoint/save point? |
| 101 | 0x65 | Entity spawn zone/trigger |

**Distribution (typical level):** ~96% value 0 (passable), ~3% value 2 (solid), ~1% value 101 (entity zones).

#### Asset 501 - Entity Placement Data (24 bytes each, CODE-VERIFIED)

Entity structures loaded by `LoadEntitiesFromAsset501` @ 0x80024dc4. Count stored in TileHeader+0x1E.

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    Bounding box X1 (left, pixels)
0x02    2     u16    Bounding box Y1 (top, pixels)
0x04    2     u16    Bounding box X2 (right, pixels)
0x06    2     u16    Bounding box Y2 (bottom, pixels)
0x08    2     u16    Center X (pixels)
0x0A    2     u16    Center Y (pixels)
0x0C    2     u16    Variant (animation frame or subtype selector)
0x0E    4     u8[4]  Padding (always zero)
0x12    2     u16    Entity type ID (see Known Entity Types below)
0x14    2     u16    Layer with flags (see Layer Field below)
0x16    2     u16    Padding (always zero)
```

**Layer Field (offset 0x14) - PARTIALLY VERIFIED 2026-01-12:**
- Lower byte (bits 0-7): Collision/logic layer grouping (1, 2, or 3)
- Upper byte (bits 8-15): Render flags (purpose unverified)
- Most entities use simple values (1, 2, 3)
- CSTL level uses extended values like 0xF301 (layer 1 + 0xF3 flags)
- Entity types 9 and 81 observed with extended layer values

**IMPORTANT:** Entity render z-order is NOT determined by this layer field!
Entity z_order is HARDCODED per entity type in InitEntitySprite calls:
- Player: z_order = 10000
- UI/HUD: z_order = 10000
- Particles: z_order = 959
- General entities: z_order ≈ 1000

See `/docs/rendering-order.md` for full priority system documentation.

**Known Entity Types (verified via extraction):**
| Type | Name | Description |
|------|------|-------------|
| 2 | Clayball | Collectible coins (most common, ~5700 total) |
| 3 | Ammo | Bullet pickup for player weapon |
| 8 | Item | Generic item pickup |
| 9 | Unknown | Uses extended layer flags in CSTL |
| 24 | SpecialAmmo | Special ammunition pickup |
| 25, 27 | Enemy | Enemy entities |
| 28, 48 | Platform | Moving platforms / directional objects |
| 45 | MessageBox | In-game message display |
| 64, 103 | Unknown | Various other entity types |
| 81 | Unknown | Uses extended layer flags in CSTL |

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

**UPDATE 2026-01-06:** These are NOT bonus rooms - they are **world completion password screens**.
Skullmonkeys has no memory card support, so after completing each world, players are shown
a password screen they can use to return to that point. The final container is the "YOU WIN"
victory screen.

| Container | File Offset | Size | Content |
|-----------|-------------|------|---------|
| Password 1 | 0x00EB7000 | 252 KB | World 1 completion - Gray theme |
| Password 2 | 0x01355000 | 248 KB | World 2 completion - Gray theme |
| Password 3 | 0x0173E800 | 245 KB | World 3 completion - Magenta theme |
| Password 4 | 0x01AFB800 | 252 KB | World 4 completion - Magenta theme |
| Password 5 | 0x01ED8800 | 255 KB | World 5 completion - Magenta theme |
| Password 6 | 0x02297800 | 254 KB | World 6 completion - Magenta theme |
| Password 7 | 0x025AB000 | 254 KB | World 7 completion - Magenta theme |
| Password 8 | 0x02880800 | 253 KB | World 8 completion - Magenta theme |
| Password 9 | 0x02D1B000 | 252 KB | World 9 completion - Magenta theme |
| Password 10 | 0x032D7000 | 248 KB | World 10 completion - Magenta theme |
| Password 11 | 0x0357A800 | 245 KB | World 11 completion - Magenta theme |
| Password 12 | 0x0397F800 | 244 KB | World 12 completion - Magenta theme |
| Password 13 | 0x03E93000 | 250 KB | World 13 completion - Magenta theme |
| Password 14 | 0x03FFC800 | 249 KB | World 14 completion - Magenta theme |
| Password 15 | 0x044AA800 | 236 KB | World 15 completion - Magenta theme |
| YOU WIN | 0x047DC800 | 654 KB | **Final victory screen** - Magenta theme |

**Background colors:**
- Passwords 1-2: Gray `RGB(122-130, 122-130, 142)` (early worlds)
- Passwords 3-16: Magenta `RGB(150, 0, 106)` = `#96006A` (main game theme)

### Container Structure

Each password screen container has an identical 11-asset structure:

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

Example: Password screen 5 has 562 tiles × 256 bytes = 143,872 bytes (matches Asset 300 size exactly)

### Layer Rendering (VERIFIED 2026-01-06)

Password screen layers can be fully rendered using Assets 200+201+300+301+400.
All 16 screens render to 320×256 pixels (20×16 tiles) using tilemap 3 (the largest).

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
- **Bits 0-10**: Tile index (11 bits, 1-based, 0 = transparent)
- **Bits 11-15**: Unknown/unused (no flip flags - tiles are not flipped in game)

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
0x0C    u32    Render param - LOW 16 BITS = priority (signed short)
0x10    u32    Scroll factor X (0x10000 = 1.0, 0x8000 = 0.5)
0x14    u32    Scroll factor Y
0x26    u8     Layer type (0=normal, 3=skip render)
0x28    u16    Skip render flag (!=0 means skip)
0x2C    u8[48] Color tints (16 RGB entries for tile tinting)
```

**Priority System (VERIFIED 2026-01-12 via Ghidra):**

The render_param field at offset 0x0C contains the **layer priority** in its low 16 bits.
Lower values render behind higher values. Entities share the same priority space.

| Priority Range | Content |
|----------------|---------|
| 150-800 | Background/parallax layers |
| 900-1100 | Main gameplay layers, most entities |
| 1200-1500 | Foreground layers |
| 10000 | Player entity, UI/HUD |

See `/docs/rendering-order.md` for complete rendering system documentation.

**Rendering process:**
1. Parse Asset 100 for level dimensions (offset +0x08, +0x0A) and tile count (+0x10)
2. Extract tiles from Asset 300 using palette indices from Asset 301
3. For each layer in Asset 201:
   - Get tilemap from Asset 200 sub-TOC
   - Render tiles at (x_offset + x, y_offset + y) positions
4. Layers are sorted by priority into render lists (lower priority = further back)

### Observation: Identical Sprite/Audio Sizes

All 16 password screens have nearly identical Asset 600 and Asset 400 sizes:
- Asset 600 (sprites): 52,368 bytes in all containers
- Asset 400 (palettes): 2,100 bytes in all containers (4 palettes × 512 bytes + header)

This suggests password screens share common sprite/palette templates with screen-specific tile graphics.

### Loading Mechanism

These password screens are NOT in the level metadata table. They are loaded via a separate
code path triggered when the player completes a world:

1. World completion triggers password screen display
2. Game loads the corresponding password container by sector offset
3. Renders the tilemap showing the password for that world checkpoint
4. Player can write down password to resume later (no memory card save)

The sector offsets may be stored in a hardcoded table or calculated from world index.
Further investigation of the world completion code path is needed.

## Entity Spawn System (DISCOVERED 2026-01-05)

Layers 8-11 in tertiary data may contain **entity spawn markers** in addition to background tiles.
These are tiles with indices that exceed the normal tileset range.

### Tile Index Encoding (UPDATED 2026-01-06)

The u16 tile index in tilemap data uses the following bit layout:

```
Tilemap Entry Format (16 bits):
┌─────────────────────────────────────┐
│ 15 14 13 12 │ 11-0                 │
│ COLOR_TINT  │ TILE_INDEX           │
└─────────────────────────────────────┘

TILE_INDEX (bits 0-11): Tile index (12 bits, 0xFFF mask)
  - 0 = transparent/empty tile
  - 1+ = tile index (1-based)
  - Values > tile_count may be entity spawn markers

COLOR_TINT (bits 12-15): Color tint selector (4 bits, 0-15)
  - Indexes into layer's color_tints[16] table at LayerEntry+0x2C
  - Each entry is 3 bytes (RGB)
  - Used to tint/recolor tiles without needing duplicate graphics
  - Entry 0 is typically white (255,255,255) for no tinting
```

**DISCOVERED via Ghidra analysis of FUN_80017540:**
```c
// Tile rendering extracts color tint from upper 4 bits
puVar12 = (u_char*)(color_table_base + (tilemap_entry >> 12) * 3);
// Sets sprite RGB to: (puVar12[0], puVar12[1], puVar12[2])
```

**Reference implementation:** See `scripts/extract_all_graphics.py` function `extract_layers()`:
```python
tile_idx = raw_idx & 0xFFF  # Lower 12 bits = tile index
color_idx = (raw_idx >> 12) & 0xF  # Upper 4 bits = color tint selector
```

### Entity Detection

Entity spawn markers are detected by checking if the tile index exceeds the tileset size:
```python
if (tile_val & 0x7FF) > total_tile_count:
    # This is an entity tile, not a regular tileset tile
```

### Entity Layer Purpose

| Layer Range | Typical Content | Observed Pattern |
|-------------|-----------------|------------------|
| Layers 0-7 | Background/parallax tiles | Normal tile indices (0 to tile_count) |
| Layer 8 | Parallax decorations OR entity spawns | Large connected regions, repeating patterns |
| Layers 9-11 | Entity spawn markers | Multi-tile entity footprints |

### Entity Region Structure

Entity spawns appear as **connected regions** of entity tiles in the tilemap:

```
Example (PHRO Layer 9, World position 132,30):

Grid of entity tile indices (5×3 region):
  Y=30: [3126, 3127, 3128, 3129, 3130]  ← Sequential in X
  Y=31: [2527, 2896, 2897, 2898, 2899]  ← Different base
  Y=32: [3258, 3259, 3260, 3261, 3262]  ← Sequential in X

Each unique tile index maps to a tile in the entity graphics atlas.
All tiles in a region together form one entity's visual footprint.
```

### Entity Type Identification

Entities are identified by their **tile ID set** (the unique set of entity tile indices):

| Property | Description |
|----------|-------------|
| Tile ID Set | The set of entity tile indices comprising the entity |
| Region Size | Width × Height in tiles (e.g., 5×3, 6×6) |
| World Position | Layer offset + local tile position |

**Observed (PHRO level):**
- 28 entity instances across layers 8-11
- 23 unique entity types (by tile ID set)
- 2 entity types appear at multiple positions (duplicates)
- Entity tile index range: 9 to 1102 (after masking bit 12)

### Entity Tile Graphics

Entity tiles are stored separately from secondary tileset tiles:

| Component | Source | Purpose |
|-----------|--------|---------|
| Entity tile indices | Bits 11-0 of tilemap entries with bit 12 set | Index into entity tile atlas |
| Entity tile graphics | Tertiary Asset 600 sprite data | Visual pixels for entity tiles |
| Palettes | Embedded in each sprite (Asset 600) | Per-sprite color schemes |

**Hypothesis:** The entity tile indices (0-1102) reference pre-rendered tiles extracted
from sprite frames in Asset 600. Each sprite's frames are composited into the atlas at 
specific tile offsets, allowing the tilemap to reference individual 16×16 pieces.

### Spawn Data Summary

```
PHRO Level Entity Analysis:
  Layers with entity data: 8, 9, 10, 11
  Layer 8: 1153 entity tile positions, 63 unique tile indices
  Layer 9: 222 entity tile positions, 137 unique tile indices
  Layer 10: 70 entity tile positions
  Layer 11: 31 entity tile positions

  Total entity spawn regions: 28
  Unique entity types: 23
  Duplicate placements: 2 types appear 2× each
```

### Relationship to Sprites

The entity regions likely correspond to **static level objects** (platforms, decorations,
hazards) that use sprite graphics but don't animate or have complex runtime behavior.
True animated enemies and interactive objects may be spawned via a separate mechanism 
(likely in the collision/physics data Asset 0x259 or code-driven spawn tables).

## Audio System (VERIFIED 2026-01-07)

The game's audio system uses the PSX SPU (Sound Processing Unit) with ADPCM-encoded samples.
Audio data is distributed across **Secondary** and **Tertiary** segments.

### Audio Asset Distribution

| Segment | Asset ID | Contents |
|---------|----------|----------|
| **Secondary** | 601 (0x259) | Audio sample bank - ADPCM samples with TOC |
| **Secondary** | 602 (0x25A) | Volume/pan table - per-sample settings |
| **Tertiary** | 500 (0x1F4) | Unknown (possibly tile/sprite metadata) |
| **Tertiary** | 501 (0x1F5) | Entity data (not audio) |
| **Tertiary** | 502 (0x1F6) | VRAM rectangles (texture pages) |
| **Tertiary** | 503 (0x1F7) | Animation frame offsets |
| **Tertiary** | 700 (0x2BC) | Additional SPU samples (ADPCM) |

**Note:** Earlier documentation incorrectly stated audio was in tertiary Assets 500-503.
The primary audio samples are in **Secondary Asset 601**, with additional samples in
Tertiary Asset 700.

### Secondary Asset 601 - Audio Sample Bank (CODE-VERIFIED)

Contains audio samples for SPU upload. Verified via `UploadAudioToSPU` (0x8007c088).

```
Offset  Size   Description
------  ----   -----------
0x00    u16    Sample count
0x02    u16    Reserved (always 0)
0x04    12×N   Sample entries (N = sample_count)
0x04+12×N ...  ADPCM audio data

Sample Entry (12 bytes):
  0x00  u32    Sample ID (hash/identifier)
  0x04  u32    Sample size in SPU RAM (bytes)
  0x08  u32    Offset within audio data block
```

**Example (SCIE Stage 0, 13 samples):**
| Index | Size | SPU Offset | Notes |
|------:|-----:|-----------:|:------|
| 0 | 4,432 | 0xA0 | First sample |
| 1 | 3,296 | 0x11F0 | |
| 2 | 5,296 | 0x1ED0 | |
| ... | ... | ... | |
| 12 | 6,160 | 0xD1A0 | Last sample |

**SPU Upload Process:**
1. SPU transfer starts at base address 0x1010 (after system reserved area)
2. Each upload block is tracked in a table at `DAT_8009cfa8` (12-byte entries)
3. Sample table at `DAT_8009cc60` maps sample IDs to SPU addresses

### Secondary Asset 602 - Volume/Pan Table (CODE-VERIFIED)

Per-sample volume and pan settings, passed to `UploadAudioToSPU` as second parameter.

```
For each sample (4 bytes):
  0x00  u16    Volume (0-0x3FFF, where 0x3FFF = max)
  0x02  u16    Pan (0 = center, non-zero = offset)
```

**Example values:**
- `0x3FFF, 0x0000` - Full volume, centered
- `0x2000, 0x0000` - Half volume, centered
- `0x1333, 0x0010` - ~30% volume, slight pan offset

If Asset 602 is NULL, default values are used:
- Volume: 0x3FFF (maximum)
- Pan: 0x0000 (center)

### Tertiary Asset 700 - Additional Audio Data (UNDER INVESTIGATION)

⚠️ **Note**: Despite earlier documentation, Asset 700 may NOT be standard ADPCM samples.

**Observations:**
- Header format matches 601 (count=1, entry with ID/size/offset)
- However, data content has invalid ADPCM filter values (filter=15, valid is 0-4)
- `GetAsset601Ptr` reads from ctx+0x48/0x74, NOT ctx+0x54 where Asset 700 is stored
- Only appears in 9 of 26 levels (MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED)

**Possible interpretations:**
1. Music track selection + sound event sequence data
2. Level-specific audio configuration
3. Unused/legacy data (stored but never read)

See `docs/unconfirmed_findings.md` for detailed analysis.

### Audio Loading Flow (CODE-VERIFIED)

From `InitializeAndLoadLevel` (0x8007d1d0):

```c
// After loading secondary segment:
audioSamples = GetAsset601Ptr(ctx);    // Secondary Asset 601
volumePanTable = GetAsset602Ptr(ctx);  // Secondary Asset 602
audioSize = GetAsset601Size(ctx);
UploadAudioToSPU(audioSamples, volumePanTable, audioSize);
```

### Key Audio Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `UploadAudioToSPU` | 0x8007c088 | Upload samples to SPU RAM |
| `GetAsset601Ptr` | 0x8007ba78 | Get audio sample bank pointer |
| `GetAsset601Size` | 0x8007ba50 | Get audio bank size |
| `GetAsset602Ptr` | 0x8007baa0 | Get volume/pan table pointer |

### LevelDataContext Audio Offsets

| ctx Offset | Purpose | Set By |
|------------|---------|--------|
| +0x2C (ctx[0xB]) | Asset 500 ptr | LoadAssetContainer |
| +0x30 (ctx[0xC]) | Asset 503 ptr | LoadAssetContainer |
| +0x34 (ctx[0xD]) | Asset 504 ptr | LoadAssetContainer |
| +0x38 (ctx[0xE]) | Asset 501 ptr | LoadAssetContainer |
| +0x3C (ctx[0xF]) | Asset 502 ptr | LoadAssetContainer |
| +0x48 (ctx[0x12]) | Asset 601 ptr (secondary) | LoadAssetContainer |
| +0x4C (ctx[0x13]) | Asset 601 size | LoadAssetContainer |
| +0x50 (ctx[0x14]) | Asset 602 ptr | LoadAssetContainer |
| +0x54 (ctx[0x15]) | Asset 700 ptr | LoadAssetContainer |
| +0x58 (ctx[0x16]) | Asset 700 size | LoadAssetContainer |
| +0x74 (ctx[0x1D]) | Asset 601 ptr (primary) | LevelDataParser |
| +0x78 (ctx[0x1E]) | Asset 601 size (primary) | LevelDataParser |
| +0x7C (ctx[0x1F]) | Asset 602 ptr (primary) | LevelDataParser |

**Note:** GetAsset601Ptr/GetAsset602Ptr check `ctx[1]` to select between primary
(offsets 0x74-0x7C) and secondary (offsets 0x48-0x50) audio sources.

## Tertiary Asset 600 - Sprite Container (DETAILED 2026-01-05)

Tertiary Asset 600 contains sprite data with a different structure than Primary Asset 600.

### Container Structure

```
Offset  Size    Description
------  ----    -----------
0x00    u32     Sprite count (N)
0x04    N×12    Sprite header table

Sprite Header (12 bytes each):
  0x00  u32     Sprite ID (hash-like value, e.g., 0x5a89815f)
  0x04  u32     Data offset (relative to data section start)
  0x08  u32     Data size in bytes
```

### Sprite Definition Block

Each sprite has a definition block found by searching for the sprite ID in the data section:

```
Structure at (sprite_id_location - 8):
  -8    u32     Frame graphics size
  -4    u32     Frame graphics offset (relative to data section)
   0    u32     Sprite ID (the search key)
  +4    u32     Frame count
  +8    u32     Flags (typically 0x815d0001 or 0x815d0000)
```

### Sprite ID Patterns

Sprite IDs appear to be hash values with related sprites having similar IDs:

| Sprite ID | Hex Bytes | Likely Relationship |
|-----------|-----------|---------------------|
| 0x5a89815f | 5f 81 89 5a | Variant A |
| 0x5ab9815f | 5f 81 b9 5a | Variant B (same entity?) |
| 0x5ad9815f | 5f 81 d9 5a | Variant C (animation states?) |

**Example (PHRO tertiary, 22 sprites):**
```
  ID: 0x09406d8a  gfx_offset:   936  gfx_size:   60  frames:  1
  ID: 0x2cda4604  gfx_offset:  2724  gfx_size:  240  frames:  6
  ID: 0x5a89815f  gfx_offset:  2376  gfx_size:  276  frames:  7
  ID: 0x5ab9815f  gfx_offset:  1960  gfx_size:  276  frames:  7
  ID: 0x5ad9815f  gfx_offset:  4044  gfx_size:  384  frames: 10
```

Three sprites (0x5a89815f, 0x5ab9815f, 0x5ad9815f) share similar IDs and frame counts,
suggesting they are animation variants of the same game entity.

### Purpose

Tertiary sprites are used for:
- Entity graphics (referenced by entity spawn layers)
- Level-specific animated objects
- Interactive elements (coins, power-ups, hazards)
- Background decoration sprites

Unlike Primary Asset 600 sprites (level geometry/backgrounds), tertiary sprites
are typically smaller, animated, and associated with gameplay objects.

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

## Primary.bin Internal Structure (PARTIALLY DECODED 2026-01-04)

The primary.bin files contain the main level geometry and collision data.
After parsing the TOC, the game accesses three asset types:

### Asset 0x258 - World/Level Graphics (RLE Sprite Container)

The largest asset, containing level background/decoration sprites. Uses same RLE
format as tertiary sprite data. Each sprite contains an embedded 256-color palette.

**VERIFIED 2026-01-05 via Ghidra + sprite extraction (extract_sprites_600.py)**

**Container Structure (Sub-TOC):**
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
0x00    u32     Sprite ID (lookup key for FindSpriteInTOC)
0x04    u32     Sprite data size in bytes
0x08    u32     Sprite data offset from asset start
```

**Sprite Header (12 bytes):**
```
Offset  Size    Type    Description
------  ----    ----    -----------
0x00    2       u16     Animation count (number of animation groups)
0x02    2       u16     Frame metadata offset (from sprite start)
0x04    4       u32     RLE data offset (from sprite start)
0x08    4       u32     Palette offset (embedded 256-color palette)
```

**Embedded Palette (512 bytes at palette_offset):**
```
Format: 256 × u16 PSX 15-bit RGB colors
  - Color 0: Typically 0x0000 (transparent)
  - Bits 0-4:   Red (0-31, multiply by 8 for 8-bit)
  - Bits 5-9:   Green (0-31, multiply by 8 for 8-bit)
  - Bits 10-14: Blue (0-31, multiply by 8 for 8-bit)
  - Bit 15:     STP (semi-transparency flag)

Each sprite contains its own embedded palette, allowing per-sprite
color schemes. This differs from tiles which share Asset 400 palettes.
```

**Animation Entry (12 bytes each, starting at offset 0x0C):**
```
Offset  Size    Type    Description
------  ----    ----    -----------
0x00    4       u32     Animation ID (identifies animation type)
0x04    2       u16     Frame count (number of frames in animation)
0x06    2       u16     Frame data offset (index into frame metadata)
0x08    2       u16     Flags (animation properties, see below)
0x0A    2       u16     Extra (unknown, often 0)

Animation Flags (VERIFIED via Ghidra FUN_8001d748):
  Bit 0 (0x0001): Has frame callback - triggers FUN_8001c4a4 on frame change
                  Used for sound effects, particle spawns, etc.
  Note: Loop behavior is controlled by game code, not these flags.
```

**Frame Metadata (36 bytes = 0x24 per frame):**
```
Offset  Size    Type    Description
------  ----    ----    -----------
0x00    2       u16     Callback ID (0 = no callback, triggers FUN_8001c4a4)
0x02    2       u16     Reserved (always 0)
0x04    2       u16     Flip flags (0=normal, non-zero=horizontal mirror)
0x06    2       s16     Render X offset (signed, for sprite positioning)
0x08    2       s16     Render Y offset (signed)
0x0A    2       u16     Render width (sprite visible width)
0x0C    2       u16     Render height (sprite visible height)
0x0E    2       u16     Frame delay (timing value, used for animation speed)
0x10    2       u16     Reserved (always 0)
0x12    2       s16     Hitbox X offset (signed)
0x14    2       s16     Hitbox Y offset (signed)
0x16    2       u16     Hitbox width
0x18    2       u16     Hitbox height
0x1A    6       bytes   Padding (always 0)
0x20    4       u32     RLE data offset (from sprite's RLE base)

Frame Flip Flags (VERIFIED via Ghidra DecodeRLESprite 0x80010068):
  The flip flag at offset 0x04 controls RLE decode direction:
  - 0: Normal left-to-right decode (puVar13 = puVar13 + skip)
  - Non-zero: Mirrored right-to-left decode (puVar13 = puVar13 - skip)
  This allows sprites to be horizontally flipped without storing duplicate data.

Frame Delay (VERIFIED via Ghidra FUN_8001d748):
  Offset 0x0E is copied to entity+0xE6 and used for animation timing.
  Value 0 indicates static frame (no automatic advance).
  Non-zero values control per-frame display duration.
```

**Key Ghidra Functions:**
- **FindSpriteInTOC** (0x8007b968): Searches ctx+0x70 then ctx+0x40 for sprite ID
- **InitSpriteContext** (0x8007bc3c): Parses sprite header at offsets 0, 2, 4, 8
- **GetFrameMetadata** (0x8007bebc): Returns pointer to 36-byte frame entry
- **DecodeRLESprite** (0x80010068): RLE decoder with mirror support

**Key Formulas:**
```
animation_entry_offset = 0x0C + anim_index × 12
frame_meta_absolute = sprite_start + frame_meta_offset + frame_index × 0x24
rle_data_absolute = sprite_start + rle_offset + frame.rle_offset
```

**RLE Pixel Data Format:**
Located at sprite_start + rle_offset + frame.rle_offset:
```
Offset  Size    Description
------  ----    -----------
0x00    u16     Command count (number of RLE commands)
0x02+   u16×N   RLE commands

Command format (u16):
  bit 15:    New line flag (advance to next row)
  bits 14-8: Skip count (transparent pixels)
  bits 7-0:  Copy count (literal pixels to copy)

Pixel data follows commands as 8bpp indexed values.
```

**Example (SCIE level, Asset 600):**
- 20 sprite entries in sub-TOC
- Sprite 0: 3 animations × 5 frames each = 15 total frames
- Frame sizes: 16×16 to 128×128 pixels
- RLE compression ratio: ~40-60% of raw size
- Each sprite has embedded 256-color palette (512 bytes)

**Extraction Tool:** `scripts/extract_sprites_600.py` - extracts colored sprite sheets from BLB files

**VERIFIED 2026-01-05**: Sprites extracted with embedded palettes display correctly.

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
0x04    [1]   4     ptr     tileHeader              ID 100: Tile header (36 bytes)
0x08    [2]   4     ptr     unknown101              ID 101: Unknown (sparse, 8 levels only)
0x0C    [3]   4     ptr     tilemapContainer        ID 200: Tilemap sub-TOC
0x10    [4]   4     ptr     layerEntries            ID 201: Layer entries (92 bytes each)
0x14    [5]   4     ptr     tilePixels              ID 300: Tile pixel data (8bpp)
0x18    [6]   4     ptr     paletteIndices          ID 301: Palette index per tile
0x1C    [7]   4     ptr     tileSizeFlags           ID 302: Per-tile flags
0x20    [8]   4     ptr     paletteContainer        ID 400: Palette sub-TOC
0x24    [9]   4     ptr     paletteAnimData         ID 401: Palette animation data
0x28    [10]  4     ptr     animatedTileData        ID 303: Animated tile lookup
0x2C    [11]  4     ptr     tileAttributes          ID 500: Tile collision attributes
0x30    [12]  4     ptr     animOffsets             ID 503: ToolX animation offsets
0x34    [13]  4     ptr     vehicleData             ID 504: Vehicle data (FINN/RUNN only)
0x38    [14]  4     ptr     entityData              ID 501: **Entity placement data** (24-byte structs)
0x3C    [15]  4     ptr     vramRects               ID 502: VRAM texture page rects
0x40    [16]  4     ptr     levelGeometry           ID 600: Level geometry pointer
0x44    [17]  4     u32     levelGeometrySize       Size of level geometry in bytes
0x48    [18]  4     ptr     audioSamples            ID 601: Audio samples pointer - uploaded to SPU
0x4C    [19]  4     u32     audioSamplesSize        Size of audio sample data in bytes
0x50    [20]  4     ptr     paletteData             ID 602: Palette data (15-bit colors)
0x54    [21]  4     ptr     spuAudioData            ID 700: Additional SPU audio data
0x58    [22]  4     u32     spuAudioDataSize        Size of SPU audio data in bytes

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
| `InitTilemapLayerRendering` | 0x8001601c | void | Sets up SPRT_16 and DR_TPAGE primitives for layers |
| `RenderTilemapSprites16x16` | 0x8001713c | void | Renders tiles as SPRT_16 on 16-pixel grid |
| `InitLayersAndTileState` | 0x80024778 | void | Master layer init, sets level dimensions (width*16, height*16) |

#### Asset ID Mapping (LoadAssetContainer)

The sub-TOC contains entries with asset IDs that map to specific context offsets:

| Asset ID | Hex | Word Index | Field Name | Description |
|----------|-----|------------|------------|-------------|
| 100 | 0x64 | [1] | tileHeader | Tile header (36 bytes, BG color, spawn, tile counts) |
| 101 | 0x65 | [2] | unknown101 | Unknown (12 bytes, sparse: only 8 levels have this) |
| 200 | 0xC8 | [3] | tilemapContainer | Tilemap sub-TOC (layer count + data offsets) |
| 201 | 0xC9 | [4] | layerEntries | Layer definition entries (92 bytes per layer) |
| 300 | 0x12C | [5] | tilePixels | Tile pixel data (8bpp indexed) |
| 301 | 0x12D | [6] | paletteIndices | Palette index per tile (1 byte each) |
| 302 | 0x12E | [7] | tileSizeFlags | Per-tile flags: bit0=semi-trans, bit1=8x8, bit2=skip |
| 303 | 0x12F | [10] | animatedTileData | Animated tile lookup table |
| 400 | 0x190 | [8] | paletteContainer | Palette sub-TOC of 256-color CLUTs |
| 401 | 0x191 | [9] | paletteAnimData | Palette animation data |
| 500 | 0x1F4 | [11] | tileAttributes | Tile collision attribute map (1 byte/tile) |
| 501 | 0x1F5 | [14] | entityData | **Entity placement data (24-byte structs)** |
| 502 | 0x1F6 | [15] | vramRects | VRAM texture page definitions |
| 503 | 0x1F7 | [12] | animOffsets | ToolX animation sequence data |
| 504 | 0x1F8 | [13] | vehicleData | Vehicle data (FINN/RUNN levels only) |
| 600 | 0x258 | [16-17] | levelGeometry + size | Level geometry (Primary) or RLE sprites (Tertiary) |
| 601 | 0x259 | [18-19] | audioSamples + size | SPU ADPCM audio samples |
| 602 | 0x25A | [20] | paletteData | 15-bit PSX color palette data |
| 700 | 0x2BC | [21-22] | spuAudioData + size | Additional SPU samples (9 levels only) |

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
