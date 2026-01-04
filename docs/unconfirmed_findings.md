# Unconfirmed Findings

This document contains observations and guesses about the Skullmonkeys data structures
that have not been fully verified through decompilation or runtime tracing.

---

## Primary Asset 600 Entry Structure (2026-01-04)

**Status:** CORRECTED - Asset 600 is RLE sprite container, not 92-byte entries

**CORRECTION:** Earlier analysis confused the accessor functions. The 92-byte entries
are actually in **Asset 201 (0xC9)** from TERTIARY segment, not Primary Asset 600.

Primary Asset 600 (0x258) contains **RLE-compressed background sprites**:
- Sub-TOC with sprite entries
- Each entry has 24-byte header + frame metadata + RLE pixel data
- Same format as Tertiary Asset 600

See "Tertiary Asset 201 - Sprite Display Entries" below for the 92-byte entries.

---

## Tertiary Asset 201 - Sprite Display Entries (2026-01-04)

**Status:** CODE-VERIFIED via Ghidra + data analysis

Asset 201 (0xC9) in the TERTIARY segment contains sprite display configuration.
These are the 92-byte entries accessed by `FUN_80024778`.

### Discovery Path

The accessor functions reveal the structure:
- `FUN_8007b6c8(ctx)` → returns `**(u16**)(ctx + 0xc)` = count from Asset 200
- `FUN_8007b700(ctx, i)` → returns `*(ptr*)(ctx + 0x10) + i * 0x5c` = entry from Asset 201

`LoadAssetContainer` sets:
- `ctx[3]` (offset 0x0C) = Asset 200 pointer (has count at offset 0)
- `ctx[4]` (offset 0x10) = Asset 201 pointer (92-byte entry table)

### Entry Structure (0x5C = 92 bytes)

```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    2     x_offset        X position/offset (tiles or pixels)
0x02    2     y_offset        Y position/offset
0x04    2     src_width       Source/clip width
0x06    2     src_height      Source/clip height
0x08    2     dst_width       Destination width (level units)
0x0A    2     dst_height      Destination height
0x0C    2     sprite_index    Index into Asset 200 sub-TOC
0x0E    2     priority        Render priority/depth
0x10    2     flags_10        Display flags (parallax bits?)
0x12    2     flags_12        Additional flags
0x14-1B 8     unknown         
0x1C    1     byte_1C         
0x1D    1     byte_1D         
0x1E    1     enable_flag_1   If !=0: game_state+0x59 = 1
0x1F    1     enable_flag_3   If !=0: game_state+0x5b = 1
0x20    1     flag_20         Low byte: if !=0: game_state+0x58 = 1
0x21    1     enable_flag_2   If !=0: game_state+0x5a = 1
0x22-23 2     unknown
0x24    2     scroll_factor   Parallax scroll factor (256 = 1:1)
0x26    1     object_type     Type ID (if ==3, entry is skipped)
0x27    1     padding
0x28    4     skip_flag       If !=0, entry is skipped entirely
0x2C    4     rgb_or_data     Entry[0] = background RGB color
0x30    48    padding         Filled with 0x40 ('@')
```

### Example Data (PHRO Level, Tertiary Block 0)

```
 # X_off Y_off SrcW SrcH DstW DstH  Sprite Priority  Scroll Type  Description
-- ----- ----- ---- ---- ---- ---- ------- -------- ------- ---- -----------
 0     0     2   25   14   25   25     150      265     256    0  UI element
 1     0     7   25    9   25   25     250      150     256    0  UI element
 2     0    10   25   15   25   25     350      318     256    0  UI element
 3    34    16    7    8  600   90     350       47       0    0  Parallax far
 4    18    16    5    6  600   90     350       29       0    0  Parallax far
 5     4    15    4    6  600   90     350       20       0    0  Parallax far
 6    52    17    9    9  600   90     550       70       0    0  Parallax mid
 7    71    17   11   12  600   90     650      114       0    0  Parallax mid
 8     4    22  109   41  600   90     750     3243       0    0  Parallax mid
 9     2    18  454   42  456   70     850     4619       0    0  Main BG layer
10     1    19  439   37  456   70     950     1149       0    0  Main BG layer  
11    29    19  237   37  456   70    1050      532       0    0  Main BG layer
12    94    19   15   46  600   90    1150      519       0    0  Foreground
```

### Key Observations

1. **Level-sized layers**: Entries with dst_width/height matching level dimensions
   (456x70 for PHRO) are the main background tilemap layers

2. **Parallax backgrounds**: Entries with width=600 (wider than 456-tile level)
   are parallax scrolling backgrounds

3. **Scroll factor**: Value of 256 = 1:1 scroll with camera. Lower values = slower
   parallax. Value of 0 = fixed/static.

4. **Entry[0] special**: First entry's offset 0x2C contains background clear color

5. **Priority ordering**: Higher priority values render in front

6. **Asset 200 contains tilemaps**: Each Asset 201 entry has a 1:1 correspondence with
   an Asset 200 sub-TOC entry. Asset 200 entry size = src_width × src_height × 2 bytes,
   containing u16 tile indices.

7. **Tilemap rendering**: The src dimensions define the tilemap stored in Asset 200.
   The dst dimensions define how it's rendered (stretched/tiled to fill layer).

### Processing Logic (FUN_80024778)

```c
count = **((u16**)ctx + 3);  // From Asset 200
entries = *(ptr*)(ctx + 0x10);  // Asset 201 base

for (int i = 0; i < count; i++) {
    entry = entries + i * 0x5c;
    
    if (entry->skip_flag != 0) continue;
    if (entry->object_type == 3) continue;
    
    // Entry 0 sets background color
    if (i == 0) {
        game->bg_r = entry->rgb[0];
        game->bg_g = entry->rgb[1]; 
        game->bg_b = entry->rgb[2];
    }
    
    // Size categorization for render list
    if (dst_width > 63 || dst_height > 63) {
        // Large - parallax layer
    } else if (dst_width < 129 && dst_height < 129) {
        // Medium - standard sprites
    } else {
        // Other
    }
}
```

---

## Tertiary Asset 200 - Tilemap Container (2026-01-04)

**Status:** DATA-VERIFIED via BLB analysis

Asset 200 (0xC8) in the TERTIARY segment contains tilemap data for background layers.
Each sub-TOC entry corresponds 1:1 with an Asset 201 layer definition entry.

### Structure

```
Offset  Size  Description
------  ----  -----------
0x00    4     Entry count (matches Asset 201 entry count)
0x04    12*N  Sub-TOC entries:
              - u32 flags
              - u32 size (bytes)
              - u32 offset
```

### Tilemap Data Format

Each sub-TOC entry points to a flat array of **u16 tile indices**:
- Size = src_width × src_height × 2 bytes (from corresponding Asset 201 entry)
- Tile index 0 = transparent/empty
- Non-zero indices reference tiles in Secondary segment tilesets

### Size Correlation (PHRO Level Example)

| Entry | Asset 200 Size | Asset 201 src_w×src_h | Match |
|-------|----------------|----------------------|-------|
| 0 | 700 | 25×14 = 350 tiles | ✅ 350×2=700 |
| 1 | 450 | 25×9 = 225 tiles | ✅ 225×2=450 |
| 2 | 750 | 25×15 = 375 tiles | ✅ 375×2=750 |
| 3 | 112 | 7×8 = 56 tiles | ✅ 56×2=112 |
| 4 | 60 | 5×6 = 30 tiles | ✅ 30×2=60 |
| 5 | 48 | 4×6 = 24 tiles | ✅ 24×2=48 |
| 6 | 162 | 9×9 = 81 tiles | ✅ 81×2=162 |
| 7 | 264 | 11×12 = 132 tiles | ✅ 132×2=264 |
| 8 | 8938 | 109×41 = 4469 tiles | ✅ 4469×2=8938 |
| 9 | 38136 | 454×42 = 19068 tiles | ✅ 19068×2=38136 |
| 10 | 32486 | 439×37 = 16243 tiles | ✅ 16243×2=32486 |
| 11 | 17538 | 237×37 = 8769 tiles | ✅ 8769×2=17538 |
| 12 | 1380 | 15×46 = 690 tiles | ✅ 690×2=1380 |

**All 13 entries match exactly: size = src_w × src_h × 2**

### Layer Rendering Model

Asset 201 defines how each tilemap layer is rendered:
- **src_w × src_h**: Actual tilemap dimensions stored in Asset 200
- **dst_w × dst_h**: Destination layer size for rendering
- Small src → large dst: Tilemap is tiled/repeated to fill the layer
- Similar src/dst: Layer rendered at native size

### Layer Categories (from PHRO analysis)

| Layer | src Size | dst Size | Purpose |
|-------|----------|----------|---------|
| 0-5 | 4-25×6-15 | 25×25 or 600×90 | Enemy/collectible spawn markers |
| 6 | 9×9 | 600×90 | Foreground decorative element |
| 7 | 11×12 | 600×90 | Same as 6, different parallax |
| 8 | 109×41 | 600×90 | Death/hazard zones |
| 9 | 454×42 | 456×70 | Main playable tileset (ground) |
| 10 | 439×37 | 456×70 | Solid collision platforms |
| 11 | 237×37 | 456×70 | One-way platforms (hypothesis) |
| 12 | 15×46 | 600×90 | Foreground decoration |

### Tile Index Format

Sample from Entry 9 (main level layer):
```
Offset 0x2E2: 97 70 98 70 99 70 9A 70 9B 70 9C 70
              ^^^^^ ^^^^^ ^^^^^ ^^^^^ ^^^^^ ^^^^^
              0x7097 0x7098 0x7099 0x709A 0x709B 0x709C
```

Values in range ~28000-30000 have high bits set. See "Tile Index Encoding" below.

---

## Tile Data Format (2026-01-04, UPDATED 2026-01-04)

**Status:** CODE-VERIFIED via Ghidra decompilation of `CopyTilePixelData` (0x800252C0)

### Asset 100 Header (Secondary Segment)

```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    2     unknown_00      Unknown (6160 for PHRO - not tile count!)
0x08    2     level_width     Level width in tiles (456 for PHRO)
0x0A    2     level_height    Level height in tiles (70 for PHRO)
0x10    2     n_16x16         Count of 16×16 tiles (1109 for PHRO)
0x12    2     n_8x8           Count of 8×8 tiles (540 for PHRO)
0x14    2     n_unknown       Third tile count (0 for PHRO)
```

**Total tile count = n_16x16 + n_8x8 + n_unknown**

### Tile Pixel Storage (Asset 300)

From `CopyTilePixelData` (0x800252C0):

**16×16 tiles** are stored first:
- 256 bytes each (16×16 8bpp)
- Offset = `(tile_index - 1) * 0x100`

**8×8 tiles** are stored after 16×16 tiles:
- **128 bytes each** (8×16 rows - padded to 16 rows!)
- Offset = `n_16x16 * 0x80 + (tile_index - 1) * 0x80`

**IMPORTANT:** Tile indices are **1-based**. Index 0 means empty/transparent.

### Pixel Data Size Calculation

```
total_bytes = n_16x16 * 256 + n_8x8 * 128
```

For PHRO: `1109 * 256 + 540 * 128 = 283,904 + 69,120 = 353,024 bytes` ✅ Matches!

### Tile Index Encoding (Tilemap u16 values)

For layers 0-7 and 12, tile indices are direct (no flags).

For layers 8-11, upper bits encode flags:

```
Bits 0-10 (0x7FF):  Tile index (1-based, 0 = empty)
Bits 11-15:         Flags (purpose TBD - possibly palette bank or layer info)
```

Example from Layer 8: `0x120A` → tile index = `0x20A` (522), flags = `0x12`
Example from Layer 9: `0x7097` → tile index = `0x097` (151), flags = `0x38` (but &0x7FF gives 151)

**Verification needed:** Confirm flag bit meanings via runtime tracing.

---

## Layer Purpose Analysis (2026-01-04)

**Status:** VISUAL-VERIFIED via tilemap extraction (PHRO level)

Based on visual inspection of extracted layer images:

| Layer | Dimensions | Visual Content | Hypothesized Purpose |
|-------|------------|----------------|---------------------|
| 0-5 | 25×14, etc | Small sprites/icons | Enemy spawn points or collectible markers |
| 6 | 9×9 | Foreground object | Decorative foreground element |
| 7 | 11×12 | Same as layer 6 | Same object on different parallax plane |
| 8 | 109×41 | Rectangular zones | **Death zones** or hazard areas |
| 9 | 454×42 | Main level tiles | **Playable tileset** - ground/platforms player stands on |
| 10 | 439×37 | Solid platforms | **Collision layer** - platforms player collides with |
| 11 | 237×37 | Additional platforms | **One-way platforms** (collide from bottom only?) |
| 12 | 15×46 | Tall structure | **Foreground decoration** (rendered in front of player) |

### Layer Rendering Order (Hypothesis)

Layers are likely rendered back-to-front based on priority field in Asset 201:
1. Background color (from Entry 0)
2. Far parallax layers (3-5) - tiny repeating patterns
3. Mid parallax layers (6-8)
4. Main level layers (9-11) - gameplay tiles
5. Foreground layer (12)

### Collision vs Visual Layers

The game likely uses separate layers for:
- **Visual rendering**: What the player sees
- **Collision detection**: What the player collides with

Layers 9-11 appear to be different collision/visual combinations:
- Layer 9: Main visual tileset
- Layer 10: Solid collision from all directions
- Layer 11: Platform collision (pass through from below?)

**Verification needed:** Trace collision detection code to confirm layer usage.

---

## RLE Sprite Decoder (2026-01-04)

**Status:** CODE-VERIFIED via Ghidra decompilation of `FUN_80010068`

The game uses a sparse RLE format for sprite compression, optimized for sprites
with large transparent regions.

**Decoder Function:** `FUN_80010068` at 0x80010068

**Context Structure (passed as param_2):**
```c
struct RLEContext {
    int control_count;      // [0] Number of control words remaining
    int stride;             // [1] Row stride (bytes per row in output)
    void* dest_row;         // [2] Current destination row pointer
    u16* control_ptr;       // [3] Pointer to control word array
    u8* pixel_ptr;          // [4] Pointer to pixel data
    short mirror_flag;      // [5] Non-zero = draw mirrored (right-to-left)
};
```

**Control Word Format (u16):**
```
Bit 15 (0x8000): New row flag - advance to next row
Bits 8-14 (0x7F00 >> 8): Skip count - transparent pixels to skip
Bits 0-7 (0xFF): Copy count - pixels to copy from source
```

**Algorithm:**
1. Read control word
2. If bit 15 set: advance destination to next row
3. Skip `(ctrl >> 8) & 0x7F` pixels (transparent)
4. Copy `ctrl & 0xFF` pixels from pixel data
5. Repeat until control_count == 0

**Mirror Mode:**
When mirror_flag != 0, pixels are written right-to-left instead of left-to-right.
Skip direction is also reversed.

**Used by:**
- Tertiary Asset 600 (0x258): Character sprites
- Primary Asset 600 (0x258): Background decoration sprites

**Verification needed:** Implement decoder and verify output matches VRAM captures.

---

## Asset 302 Value 0 vs 1 Meaning (2026-01-04, UPDATED 2026-01-04)

**Status:** Partially verified via Ghidra decompilation

Asset 302 (0x12E) contains one byte per tile with values 0, 1, or 2.

**CODE-VERIFIED from `LoadTileDataToVRAM` (0x80025240):**
```c
byte flags = asset302[tileIndex];
if ((flags & 4) != 0) continue;  // Bit 2: skip tile
uint tp = ((flags & 2) == 0);    // Bit 1: 0=16x16, 1=8x8
output[tileIndex + 6] = flags & 1;  // Bit 0: stored as property
```

**Confirmed:**
- Bit 1 (mask 0x02): Tile size - 0=16×16, 1=8×8 (verified by data matching)
- Bit 2 (mask 0x04): Skip flag - if set, tile is not loaded (no such tiles observed in data)

**Still unconfirmed - Bit 0 (mask 0x01):**
The value 0 vs 1 difference is stored in the tile output structure at offset +6.
This flag is preserved but its rendering effect is unknown.

**Hypothesis: Layer/Rendering property**
- Value 0 = Default rendering
- Value 1 = Special rendering (foreground layer, transparency, or priority)

**Verification needed:** Find code that reads the stored flag at offset +6 during rendering.

---

## Complete Asset Type Reference (2026-01-04)

**Status:** Documented from extraction analysis

### Segment Overview

| Segment | Purpose | Key Assets |
|---------|---------|------------|
| PRIMARY | Level geometry, collision | 600, 601, 602 |
| SECONDARY | Background tiles | 100, 300, 301, 302, 400, 401 |
| TERTIARY | Character sprites, audio | 100, 200, 201, 302, 401, 500-503, 600, 700 |

### Asset Extraction Status

| Asset | Hex | Segment(s) | Structure | Extraction |
|-------|-----|------------|-----------|------------|
| 100 | 0x064 | SEC, TERT | RAW 36B | ✅ Header with tile/sprite counts |
| 101 | 0x065 | SEC, TERT | RAW | ❓ Optional header (8 levels) |
| 200 | 0x0C8 | TERT | CONTAINER | ✅ Tilemap data (u16 tile indices) |
| 201 | 0x0C9 | TERT | RAW 92B×N | ✅ Layer definitions (92-byte entries) |
| 300 | 0x12C | SEC | RAW | ✅ Tile pixels 8bpp |
| 301 | 0x12D | SEC | RAW | ✅ Palette index per tile |
| 302 | 0x12E | SEC, TERT | RAW | ✅ Tile/sprite size flags |
| 400 | 0x190 | SEC | CONTAINER | ✅ Palette container |
| 401 | 0x191 | SEC, TERT | RAW | ⚠️ Animation config |
| 500 | 0x1F4 | TERT | CONTAINER | ❓ Unknown sub-TOC |
| 501 | 0x1F5 | TERT | RAW | ⚠️ VRAM texture coords |
| 502 | 0x1F6 | TERT | RAW | ⚠️ VRAM rectangles |
| 503 | 0x1F7 | TERT | CONTAINER | ⚠️ Animation frame offsets |
| 600 | 0x258 | PRIM, TERT | CONTAINER | ✅ RLE sprite container |
| 601 | 0x259 | PRIM, SEC | CONTAINER | ⚠️ Collision data |
| 602 | 0x25A | PRIM, SEC | RAW | ✅ Palette (15-bit RGB) |
| 700 | 0x2BC | TERT | RAW | ❓ Audio (9 levels only) |

Legend: ✅ Fully documented, ⚠️ Partially understood, ❓ Unknown

### Extraction Script

`scripts/extract_all_assets.py` extracts all known assets:
```
output/assets/
├── 00_MENU/
│   ├── primary/palette_602.bin, .png
│   ├── secondary/tileset.png, palettes.png, *.bin
│   └── tertiary/block_N/sprites/, *.bin
├── 01_PHRO/
...
└── all_stats.json
```

Total extracted (26 levels):
- 21,525 tiles (16×16) + 4,683 tiles (8×8)
- 196 palettes
- 1,498 sprites (raw RLE data)
- 104 tertiary blocks

---

## LevelDataContext Accessor Functions (2026-01-04)

**Status:** CODE-VERIFIED via Ghidra

These accessor functions operate on the LevelDataContext structure (GameState + 0x84).
They provide the link between loaded asset data and game rendering.

### Asset 600 (Level Objects) Accessors

| Function | Address | Returns | Description |
|----------|---------|---------|-------------|
| `FUN_8007b6c8` | 0x8007b6c8 | u16 | Entry count: `**(u16**)(ctx + 0xc)` |
| `FUN_8007b700` | 0x8007b700 | ptr | Entry by index: `*(ptr*)(ctx+0x10) + i * 0x5c` |
| `FUN_8007b6dc` | 0x8007b6dc | ptr | Sprite data from sub-TOC |

### Asset 100 (Tile Header) Accessors

| Function | Address | Returns | Field |
|----------|---------|---------|-------|
| `GetLevelDimensions` | 0x8007b434 | void | Writes ctx+4 offsets 0x8, 0xB (width, height) |
| `FUN_8007b458` | 0x8007b458 | void | Writes ctx+4 offsets 0xC, 0xF (alt dims?) |
| `FUN_8007b4b8` | 0x8007b4b8 | ptr | Background RGB (offset 0x00 in Asset 100) |
| `FUN_8007b4c4` | 0x8007b4c4 | ptr | Secondary RGB (offset 0x04 in Asset 100) |
| `GetPaletteGroupCount` | 0x8007b4d0 | u8 | Palette count |
| `GetTotalTileCount` | 0x8007b53c | u16 | Sum of all tile counts |
| `CopyTilePixelData` | 0x8007b588 | void | Copy tile pixels to buffer |
| `GetPaletteIndices` | 0x8007b6b0 | ptr | Asset 301 pointer |
| `GetTileSizeFlags` | 0x8007b6bc | ptr | Asset 302 pointer |
| `GetAsset100Field1C` | 0x8007b7c8 | u16 | Unknown field at +0x1C |
| `FUN_8007b7dc` | 0x8007b7dc | ptr | ctx+0x3C (Asset 501?) |

### Context Field Layout (partial)

From accessor analysis, the LevelDataContext fields at low offsets:

```
Offset  Size  Description                Source
------  ----  -----------                ------
0x04    4     Asset 100 pointer          Used by GetLevelDimensions (+0x8, +0xB, +0xC, +0xF)
0x0C    4     Asset 600 sub-TOC pointer  FUN_8007b6c8, FUN_8007b6dc
0x10    4     Asset 600 entry table ptr  FUN_8007b700 (entries are 92 bytes each)
0x3C    4     Unknown (Asset 501?)       FUN_8007b7dc
```

**Note:** These are separate from the main LevelDataContext offsets documented in
blb-data-format.md. The ctx+0x04 here refers to content WITHIN the loaded asset
buffers, accessed via the base pointer stored in the context.

---

## Primary Asset 601 - Not Pure Collision (2026-01-04)

**Status:** HYPOTHESIS - needs verification

Earlier documentation labeled Asset 601 (0x259) as "collision data", but this
may be incorrect or incomplete.

### Observations

1. **Size mismatch**: Asset 601 is 100-300KB per level, which seems large for
   simple tile-based collision in a 2D platformer. Most PSX platformers use
   1-2 bits per tile for collision, which would be ~5KB for a 400x100 tile level.

2. **Container format**: Asset 601 uses the same sub-TOC container format as
   Asset 600, suggesting it contains multiple discrete objects/entries.

3. **Game complexity**: Skullmonkeys has enemies, collectibles, hazards, warps,
   and interactive objects. These need placement data somewhere.

### Hypothesis

Asset 601 may contain:
- **Entity placement**: Positions and types of enemies, items, hazards
- **Trigger zones**: Level transitions, warp points, event triggers
- **Collision geometry**: Complex collision shapes (not just tile flags)
- **Object properties**: Speed, behavior, timing for dynamic elements

### Evidence Needed

1. Parse Asset 601 sub-TOC and analyze entry sizes/patterns
2. Find functions that read from ctx+0x74 (Asset 601 pointer)
3. Correlate entry data with visible game objects during play
4. Check if tile collision is stored elsewhere (Asset 302 bit flags?)

### Related Code

From `LevelDataParser` (0x8007a62c):
```c
ctx[0x1d] = asset601_ptr;   // Offset 0x74
ctx[0x1e] = asset601_size;  // Offset 0x78
```

The size is stored separately, unlike Asset 600, suggesting the game needs
to iterate through or bounds-check the data differently.

---

## BLB Asset File Headers (OUTDATED)

> **NOTE:** This section is outdated. See blb-data-format.md for current documentation.
> The actual format uses standard TOC structures, not the format described below.

### Primary.bin (Level Data)

**Header format:** `[type=0x03] [0x258] [data_size] [0x28]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    4     0x00000003  Type identifier (always 3 for primary)
0x04    4     0x00000258  Header end offset (600 bytes)
0x08    4     varies      Data size or related offset
0x0C    4     0x00000028  Entry size (40 bytes)
```

- Contains level geometry, tile maps, collision data (guessed)
- Has embedded TIM textures (magic `0x10000000` found at various offsets)
- Size varies: 65KB (menu) to 1.3MB (complex levels)

### Secondary.bin / Secondary_sub_*.bin (Sprite/Object Data)

**Header format:** `[count] [entry_size=0x64] [header_size=0x24] [table_size]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    4     N           Entry count (typically 6-12)
0x04    4     0x00000064  Entry size (100 bytes per entry)
0x08    4     0x00000024  Header size (36 bytes before entry table)
0x0C    4     N*12+4      Computed table size
```

The `table_size` field follows a formula: `count * 12 + 4`

| Count | Expected [3] | Hex    |
|-------|--------------|--------|
| 6     | 76           | 0x4C   |
| 7     | 88           | 0x58   |
| 8     | 100          | 0x64   |
| 9     | 112          | 0x70   |
| 10    | 124          | 0x7C   |
| 11    | 136          | 0x88   |
| 12    | 148          | 0x94   |

- Contains sprite sheets, animations, object definitions (guessed)
- Contains embedded TIM images
- Entry table starts at offset 0x24 (36 bytes)

### Tertiary_sub_*.bin (Additional Assets)

- Same header format as secondary files
- Count ranges from 7-12
- Contains additional sprite data, backgrounds (guessed)
- Size varies widely: 4KB to 1MB

### Loading Screens

**Header format:** `[compressed_size:u16] [decompressed_size:u16=0x3800] [data...]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    2     varies      Compressed data size
0x02    2     0x3800      Decompressed size (always 14,336 bytes)
0x04    4     varies      Additional header data
0x08+   ...   ...         Compressed image data
```

- Uses PSX compression (possibly LZSS or BizStream variant)
- Decompressed size always 14,336 bytes
- Likely 112×128 pixels at 8bpp, or similar dimensions
- **NOT** standard TIM format - custom compressed format

### Embedded Assets

- **TIM images** (magic `0x10000000`) found embedded in primary and secondary files
  - Multiple TIM images per file (20-40 typical)
  - Various bit depths detected (4-bit, 8-bit, 16-bit)
- **No VAG audio** found in level files
  - Audio likely handled separately (XA streaming or separate files)

## Credits Sequence Table

**Location:** BLB header offset 0xF10  
**Entry size:** 12 bytes (0x0C)  
**Max entries:** 2 complete entries before overlapping count fields

> **NOTE:** "Credits" is a GUESSED name based on the "CRD1"/"CRD2" codes found
> in entries and their relationship to the "CRED" sector entry. The actual
> purpose is not fully confirmed. JP versions don't have valid data here.

```
Offset  Size  Type    Field       Description
------  ----  ----    -----       -----------
0x00    4     str     code        4-char ID (empty for entry 0, "CRD1"/"CRD2" for others)
0x04    4     bytes   padding     Unused (zeros) - used to detect valid entries
0x08    2     u16     param_a     Entry 0: level_count; CRD1/2: unknown counter
0x0A    2     u16     param_b     Base sector offset
```

**Validation:** Valid credits entries have zero padding at bytes +4 to +8.
JP versions have non-zero values here, indicating garbage/unused data.

**Sector calculation (verified):**
```
CRED.sector_offset = param_b + level_count
Example: 171 + 26 = 197 = CRED sector entry offset
```

## JP Version Differences

### Sector Table Entry Layout

JP versions (papx-90053, slps-01501) have a different byte order for the 16-byte
sector table entries compared to PAL/NTSC versions:

**PAL/NTSC Layout:**
```
Offset  Size  Field
------  ----  -----
0x00    1     level_index
0x01    1     entry_flags
0x02    1     unknown_byte
0x03    5     code (5 chars, null-terminated)
0x08    4     short_name (4 chars)
0x0C    2     sector_offset (u16 LE)
0x0E    2     sector_count (u16 LE)
```

**JP Layout:**
```
Offset  Size  Field
------  ----  -----
0x00    2     sector_offset (u16 LE)
0x02    2     sector_count (u16 LE)
0x04    1     level_index
0x05    1     entry_flags
0x06    1     unknown_byte
0x07    5     code (5 chars, null-terminated)
0x0C    4     short_name (4 chars)
```

**Detection method:**
- PAL: byte[3] is uppercase A-Z (0x41-0x5A) - start of code field
- JP: byte[3] is 0x00 (low byte of sector_count), byte[7] is uppercase A-Z

### JP Content Differences

| Version | Levels | Movies | Sector Entries | Credits Table |
|---------|--------|--------|----------------|---------------|
| PAL/NTSC | 26 | 13 | 32 | Valid (2 entries) |
| JP Demo (papx-90053) | 6 | 1 | 4 | Invalid (garbage) |
| JP Full (slps-01501) | 6 | 1 | 4 | Invalid (garbage) |

JP versions appear to be a subset of the full game with only 6 playable levels.

---

## Level Metadata Unknown Fields Analysis

Analysis performed on all 26 levels across PAL, NTSC-US, Beta, and JP versions.

### Field 0x08: `entry1_offset_lo` - NOW CONFIRMED

**Previous understanding:** "Low 16 bits of Entry[1].offset in primary TOC"

**Actual meaning:** This is `TOC[6].offset` from the primary data blob (100% match on all 26 levels).

**Verification method:**
```python
# For each level:
toc6_offset = struct.unpack_from('<H', primary_data, 6*4)[0]
assert toc6_offset == level.entry1_offset_lo  # Always true
```

**Recommendation:** Rename field to `toc6_offset` and update documentation.

**Runtime verification:**
```bash
# Run the verification Lua script in PCSX-Redux:
make lua SCRIPT=scripts/verify_toc_fields.lua

# After level loads, check console output for TOC comparison
```

### Field 0x0A: `unknown_0A` - NOW CONFIRMED

**Actual meaning:** This is `TOC[2].count` from the primary data blob (100% match on all 26 levels).

**Verification method:**
```python
# For each level:
toc2_count = struct.unpack_from('<H', primary_data, 2*4+2)[0]
assert toc2_count == level.unknown_0A  # Always true
```

**Cross-version consistency:** Same values between PAL and JP for identical levels (PHRO, SCIE, TMPL, FINN, MEGA).

**Recommendation:** Rename field to `toc2_count` and investigate what TOC[2] points to.

**Runtime verification:**
```bash
# Run the verification Lua script in PCSX-Redux:
make lua SCRIPT=scripts/verify_toc_fields.lua

# The script will:
# 1. Dump all level metadata fields
# 2. After a level loads, compare field 0x0A with TOC[2].count
# 3. Report PASS/FAIL for each verification
```

### Field 0x06: `unknown_06` - PARTIALLY UNDERSTOOD

**Range:** 7-20

**Observations:**
- Does NOT directly match any single TOC entry count
- Consistent across regions (PAL, NTSC-US, Beta have identical values)
- Same values between PAL and JP for identical levels
- Pattern: Bosses tend to have higher values (18-20), simple/special levels have lower (7-12)

**Grouped by value:**
| Value | Levels |
|-------|--------|
| 7 | FINN |
| 9 | RUNN |
| 10 | KLOG |
| 12 | GLEN |
| 13 | FOOD |
| 14 | PHRO, SCIE, TMPL, GLID, WEED, SEVN, CRYS |
| 15 | SNOW, CAVE, BRG1, EGGS, CLOU, SOAR, CSTL, EVIL |
| 16 | MENU, BOIL, MOSS |
| 18 | MEGA, WIZZ |
| 20 | HEAD |

**Hypothesis:** May represent total asset complexity, memory requirements, or a computed value from multiple TOC entries.

### Field 0x04: `unknown_04` - UNKNOWN

**Range:** 776-65,240 (large u16 values)

**Observations:**
- Values differ slightly between PAL and JP for same levels (build-dependent)
- Points to valid data within primary blob (not garbage)
- Does NOT match any TOC entry offset directly
- Data at this offset varies (no consistent structure detected)

**Hypothesis:** Byte offset to specific asset data within primary blob (palette? collision map? geometry start?). Needs runtime tracing to confirm.

### Field 0x0D: `level_flag` - PARTIALLY UNDERSTOOD

**Values:** 0 or 1 only

**Pattern analysis:**
| Flag | Levels |
|------|--------|
| flag=0, tert=1 | FINN, MEGA, HEAD, GLEN, WIZZ, KLOG (all bosses + special) |
| flag=0, tert=2 | RUNN |
| flag=0, tert=3 | PHRO |
| flag=0, tert=4 | SOAR |
| flag=0, tert=5 | EGGS, SEVN, CSTL, EVIL |
| flag=0, tert=6 | SNOW, CLOU, CRYS, MOSS |
| flag=1, tert=4 | TMPL, FOOD, WEED |
| flag=1, tert=5 | SCIE, BRG1, CAVE |
| flag=1, tert=6 | MENU, BOIL, GLID |

**Key observation:** All bosses have `flag=0` and `tert_block_count=1`.

**Hypothesis:** May indicate level type (boss vs regular) or loading behavior. Needs runtime confirmation.

### Groupings by (0x06, 0x0A) Pairs

Levels with identical `(unknown_06, unknown_0A)` pairs often share characteristics:

| Pair | Levels | Notes |
|------|--------|-------|
| (14, 7) | PHRO, SCIE, TMPL | First 3 gameplay worlds |
| (14, 9) | GLID, WEED, SEVN, CRYS | Ynt worlds + secret level |
| (15, 9) | SNOW, CAVE, CLOU, CSTL | Ice/castle themed |
| (15, 8) | BRG1, EGGS | Bridge + Eggs |
| (16, 9) | BOIL, MOSS | Industrial themed |
| (18, 15) | MEGA | Boss: Shriney Guard |
| (20, 16) | HEAD | Boss: Joe-Head-Joe |

This suggests these fields may relate to shared asset characteristics or tileset groupings.

---

## Level Loading System Analysis

> **NOTE:** Most of this information has been verified and moved to 
> [blb-data-format.md](blb-data-format.md). See that document for:
> - Complete LevelDataContext structure (128 bytes, all fields documented)
> - Asset ID mapping table (LoadAssetContainer)
> - Playback sequence modes and string markers
> - Known functions with addresses

### Unverified: Menu Demo Rotation

When at the menu (level index 0), the game cycles through demo modes:

```c
// g_MenuDemoRotationCounter at 0x800A60A4
if (counter == 4) counter = 0;
else if (counter == 0) mode = 1;  // Normal
else if (counter == 2) mode = 6;  // End sequence?
else mode = 5;                     // Attract mode
counter++;
```

**Verification needed:** Confirm counter behavior and mode meanings.

---

## Sprite/Entity Data Accessors (2026-01-04)

**Status:** Decompiled via Ghidra

Functions that access sprite/entity data from the secondary segment:

### FUN_8007b6c8 - GetEntityCount (0x8007B6C8)

```c
u16 FUN_8007b6c8(int ctx) {
    return **(u16**)(ctx + 0x0C);  // ctx[3] → assetSprite200
}
```

Returns the entity/sprite count from the first u16 of Asset 200 (0xC8).
- `ctx + 0x0C` = LevelDataContext[3] = assetSprite200 pointer
- Reads count from the beginning of the asset data

### FUN_8007b700 - GetEntityEntry (0x8007B700)

```c
int FUN_8007b700(int ctx, uint index) {
    return *(int*)(ctx + 0x10) + (index & 0xFFFF) * 0x5C;
}
```

Returns pointer to entity entry at given index.
- `ctx + 0x10` = LevelDataContext[4] = assetSprite201 pointer
- Each entry is **0x5C (92) bytes**
- Asset 201 (0xC9) contains the entity/sprite definition table

### Entity Entry Structure (0x5C = 92 bytes, PARTIAL)

From FUN_80024778 analysis, each entity entry contains:

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    4     u32     Position X (fixed point?)
0x04    4     u32     Position Y (fixed point?)
0x08    4     var     Width/size parameter
0x0A    2     u16     Height/size parameter (short access)
0x0C    4     var     Unknown (short at +0x0C checked)
0x0E    2     u16     Priority/layer value (compared against blb header)
0x10    4     var     Unknown
0x14    4     var     Unknown
0x18    4     var     Unknown
0x1A    2     u16     Unknown (copied to output +0x32)
0x1C    4     var     Unknown
0x1E    1     u8      Flag A - sets GameState+0x59 if non-zero
0x1F    1     u8      Flag B - sets GameState+0x5B if non-zero
0x20    1     u8      Flag C - sets GameState+0x58 via puVar14[8] low byte
0x21    1     u8      Flag D - sets GameState+0x5A if non-zero
0x24    4     var     Unknown (short at +0x24 checked against 0)
0x26    1     u8      Entity type (3 = special, skipped in some processing)
0x28    4     var     Unknown
0x2C    4     u32     RGB color (bytes at +0x2C, +0x2D, +0x2E copied to GameState+0x124-126)
```

**Entity Type 3:** When `*(short*)(entry + 0x26) == 3`, the entity is skipped in FUN_80024778.

**First Entity Special Handling:** When index == 0, copies RGB from entry+0x2C to GameState+0x124 (background color).

### FUN_8007b6dc - GetEntityPaletteInfo (0x8007B6DC)

Called for each entity in FUN_80024778. Returns palette-related data for the entity.

---

## Level Background Investigation (2026-01-04)

**Status:** In progress

### MDEC Frame Loading

From NOTES.md observations:
- `0x80116450` = Compressed MDEC frame buffer in RAM
- Various BLB offsets get loaded here during level transitions

Loading screens use `FUN_800399a8` (single MDEC frame) and `FUN_8003958c` (slideshow).
These call DecDCTvlc2 and DecDCTin for decompression.

### Slideshow Frame Structure (FUN_8003a13c)

```c
bool FUN_8003a13c(uint frameIndex) {
    if (frameIndex >= (byte)buffer[0x33800]) return false;
    DecDCTvlc2(
        buffer + *(int*)(buffer + frameIndex * 6 + 0x33806) + 0x67000,
        outputBuffer,
        vlcTable
    );
    return true;
}
```

- Frame count at `buffer + 0x33800` (1 byte)
- Frame offset table at `buffer + 0x33806` (6-byte stride entries)
- Frame data starts at `buffer + 0x67000` + entry offset

### Level Background Source

**Question:** Where does the static level background come from?

**Observations:**
1. Not in Asset 600 (0x258) - that's level geometry/tilemap
2. Not in Asset 200/201 - that's sprites/entities
3. Loading screens use sector table entries with MDEC frames
4. Level backgrounds might be:
   - Embedded in tertiary data
   - Part of a sector table entry loaded separately
   - Composed from tiles only (no static background image)

**Verification needed:** 
- Check if levels have a static MDEC background or just tile-based graphics
- Trace what happens during actual level rendering to VRAM

### Level Background Structure (FINDINGS)

**Conclusion: Skullmonkeys uses tile-based backgrounds, NOT static MDEC images during gameplay.**

Evidence:
1. **Loading screens** (sector table entries) use MDEC frames at 0x80116450
2. **Level backgrounds** use a solid RGB color from Asset 100 or first entity
   - Stored at GameState+0x124, +0x125, +0x126 (R, G, B)
   - Set by FUN_80024778 from first entity entry bytes at +0x2C, +0x2D, +0x2E
3. **Tile-based graphics** fill the playable area on top of the solid color

The MDEC decoder functions (`FUN_800399a8`, `FUN_8003958c`) are only used for:
- Loading screens (before level starts)
- Special screens (credits, game over)
- FMV movies (external .STR files)

No MDEC decompression occurs during normal level gameplay - all graphics are tile/sprite based.

---

*Last updated: January 4, 2026*
