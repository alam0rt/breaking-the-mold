# Unconfirmed Findings

This document contains observations and guesses about the Skullmonkeys data structures
that have not been fully verified through decompilation or runtime tracing.

---

## BLB File Coverage Analysis (2026-01-04)

Run `python3 scripts/blb_asset_coverage.py` for full analysis.

### File-Level Coverage

| Category | Size | % of File | Description |
|----------|------|-----------|-------------|
| Referenced Data | 67.4 MB | 93.0% | Data pointed to by level metadata table |
| Unreferenced Data | 5.1 MB | 7.0% | Gaps between/after levels |

### Unreferenced Regions Breakdown

| Location | Size | Content |
|----------|------|---------|
| Sectors 2-200 | 407 KB | Title screen/menu tile graphics (raw 4bpp) |
| 12 gap containers | 2.9 MB | Complete asset containers (11 entries each) - likely bonus rooms |
| Small gaps | ~90 KB | Level-specific tile data |
| End of file | ~2.4 MB | Additional level containers |

The gap containers contain full asset sets (100, 200, 300, 400, 600, 601, 602) but are
NOT referenced in the level metadata table. These are likely bonus room or secret stage data
that gets loaded via a different code path.

### Asset Type Coverage (within referenced data)

| Status | Bytes | % | Description |
|--------|-------|---|-------------|
| ✓ Parsed | 3.8 MB | 5% | Can extract and view (sprites, palettes, metadata) |
| ◐ Partial | 9.0 MB | 12% | Structure known but not fully verified (tiles, audio) |
| ◐ Primary | 20.8 MB | 27% | Raw level data (partial understanding) |
| ✗ Unknown | 0.2 MB | 0.3% | Format not understood |

### Known Asset Types

| ID | Name | Status | Notes |
|----|------|--------|-------|
| 100 | Level Metadata | Parsed | 36-byte config per level |
| 400 | Sprite Palettes | Parsed | 256-color CLUT container |
| 600 | RLE Sprites | Parsed | RLE-encoded sprite container |
| 601 | Audio Samples | Verified | ADPCM audio uploaded to SPU RAM |
| 602 | Audio Metadata | Verified | Per-sample parameters (volume, channel, etc.) |
| 200 | Background Tiles | Partial | Large graphics data |
| 300 | Tile/Collision Data | Partial | Similar to 200 |
| 500 | Audio Data | Partial | Sound effects or music |

### Unknown Asset Types (priority investigation targets)

| ID | Size | Occurrences | Notes |
|----|------|-------------|-------|
| 501/502/503 | 117 KB | 69 | Possibly audio/animation metadata |
| 201/301/302 | 94 KB | 104 | Possibly tile/layer metadata |
| 700 | 3 KB | 9 | Small entries, purpose unclear |

---

## Ghidra Function Naming Summary (2026-01-05)

The following functions have been renamed in Ghidra based on decompilation analysis:

### Tileset/Layer Functions

| Address | Old Name | New Name | Purpose |
|---------|----------|----------|---------|
| 0x8007b6c8 | FUN_8007b6c8 | GetLayerCount | Layer count from Asset 200 |
| 0x8007b700 | FUN_8007b700 | GetLayerEntry | Layer entry from Asset 201 (92-byte stride) |
| 0x8007b6dc | FUN_8007b6dc | GetTilemapDataPtr | Tilemap data pointer from Asset 200 sub-TOC |
| 0x8007b658 | FUN_8007b658 | GetAnimatedTileData | Animated tile lookup from ctx[11] |
| 0x8007b4f8 | FUN_8007b4f8 | GetPaletteDataPtr | Palette color data from Asset 400 |
| 0x8007b530 | FUN_8007b530 | GetPaletteAnimData | Palette animation flags from ctx[9] |

### Sprite/Animation Functions (2026-01-04)

| Address | Old Name | New Name | Purpose |
|---------|----------|----------|---------|
| 0x80010068 | FUN_80010068 | DecodeRLESprite | RLE decompression with mirror support |
| 0x8007bf7c | FUN_8007bf7c | DecodeRLESpriteChecked | Checks valid flag, then calls DecodeRLESprite |
| 0x8007bde8 | FUN_8007bde8 | RenderSprite | Sprite render orchestration |
| 0x8007bebc | FUN_8007bebc | GetFrameMetadata | Frame accessor: `ctx[0] + frame_idx * 0x24` |
| 0x8007bc3c | FUN_8007bc3c | InitSpriteContext | Initializes 20-byte SpriteContext |
| 0x8007bc18 | FUN_8007bc18 | ClearSpriteContext | Zeros 20-byte SpriteContext |
| 0x8007bb10 | FUN_8007bb10 | LookupSpriteById | Searches DAT_800a6064, then DAT_800a6060 |
| 0x8007bb00 | FUN_8007bb00 | SetSpriteTables | Sets global sprite table pointers |
| 0x8007b968 | FUN_8007b968 | FindSpriteInTOC | Searches ctx+0x70, then ctx+0x40 (12B stride) |

### Functions with comments added (already had names):
- `GetTotalTileCount` (0x8007b53c) - Sum of Asset 100 offsets 0x10+0x12+0x14
- `CopyTilePixelData` (0x8007b588) - Copies tile pixels with 16x16/8x8 handling
- `GetTileSizeFlags` (0x8007b6bc) - Returns Asset 302 pointer (ctx[7])
- `GetPaletteIndices` (0x8007b6b0) - Returns Asset 301 pointer (ctx[6])
- `GetPaletteGroupCount` (0x8007b4d0) - Returns palette count from Asset 400
- `LoadAssetContainer` (0x8007b074) - Full asset loading with ID mapping
- `LoadTileDataToVRAM` (0x80025240) - VRAM upload with size flag processing
- `GetAsset100Field1C` (0x8007b7c8) - Unknown field at Asset 100 offset 0x1C

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
- `GetLayerCount(ctx)` (0x8007b6c8) → returns `**(u16**)(ctx + 0xc)` = count from Asset 200
- `GetLayerEntry(ctx, i)` (0x8007b700) → returns `*(ptr*)(ctx + 0x10) + i * 0x5c` = entry from Asset 201

`LoadAssetContainer` (0x8007b074) sets:
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

### Processing Logic (FUN_80024778 / ProcessLayerEntries)

```c
count = GetLayerCount(ctx);      // From Asset 200
entries = *(ptr*)(ctx + 0x10);   // Asset 201 base

for (int i = 0; i < count; i++) {
    entry = GetLayerEntry(ctx, i);  // entries + i * 0x5c
    
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

**UPDATED 2026-01-05: Layers 8-11 contain ENTITY SPAWN DATA, not just collision zones.**

| Layer | src Size | dst Size | Purpose |
|-------|----------|----------|---------|
| 0-5 | 4-25×6-15 | 25×25 or 600×90 | Parallax background tiles |
| 6 | 9×9 | 600×90 | Foreground decorative element |
| 7 | 11×12 | 600×90 | Same as 6, different parallax |
| 8 | 109×41 | 600×90 | Entity spawn layer (parallax objects) |
| 9 | 454×42 | 456×70 | Entity spawn layer (main level objects) |
| 10 | 439×37 | 456×70 | Entity spawn layer (platforms) |
| 11 | 237×37 | 456×70 | Entity spawn layer (additional objects) |
| 12 | 15×46 | 600×90 | Foreground decoration |

**See "Entity Spawn System" in blb-data-format.md for full details.**

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

**Status:** CODE-VERIFIED via Ghidra decompilation of `CopyTilePixelData` (0x8007b588)

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

From `CopyTilePixelData` (0x8007b588):

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

**UPDATED 2026-01-05: Bit 12 is the ENTITY FLAG, not just a palette bank.**

For all layers, tile indices use this format:

```
Bits 0-12 (0x1FFF): Tile index with entity flag
  - If bit 12 = 0: Regular tile from secondary tileset (indices 0 to tile_count-1)
  - If bit 12 = 1: ENTITY TILE (bits 0-11 index into entity tile atlas)
Bits 13-15:         Flip flags (horizontal, vertical, unknown)
```

**Entity Detection:** If `(tile_index & 0x1FFF) > total_tile_count`, this is an entity tile.
The entity tile index is `tile_index & 0xFFF` (masking off bit 12).

Example from Layer 9: 
- Index `0x1050` (4176 decimal) → bit 12 set → entity tile index = 80
- Index `0x7097` → tile index = `0x097` (151), flip flags = `0x38`

Layers 8-11 contain many tile indices with bit 12 set, forming connected regions
that represent entity spawn positions. See blb-data-format.md for details.

---

## Layer Purpose Analysis (2026-01-04, UPDATED 2026-01-05)

**Status:** ENTITY-VERIFIED via tilemap extraction and bit analysis (PHRO level)

Based on visual inspection of extracted layer images:

| Layer | Dimensions | Visual Content | Verified Purpose |
|-------|------------|----------------|---------------------|
| 0-5 | 25×14, etc | Small sprites/icons | Parallax background patterns |
| 6 | 9×9 | Foreground object | Decorative foreground element |
| 7 | 11×12 | Same as layer 6 | Same object on different parallax plane |
| 8 | 109×41 | Rectangular zones | **Entity spawn layer** - large objects/decorations |
| 9 | 454×42 | Main level tiles | **Entity spawn layer** - main level objects |
| 10 | 439×37 | Additional zones | **Entity spawn layer** - platforms/hazards |
| 11 | 237×37 | Additional zones | **Entity spawn layer** - additional objects |
| 12 | 15×46 | Tall structure | Foreground decoration |

### Entity Spawn Discovery (2026-01-05)

**STATUS: VERIFIED** - Layers 8-11 contain entity spawn data encoded in tilemap indices.

Key findings:
- Tile indices with bit 12 set (value >= 4096) are entity tiles
- Connected regions of entity tiles form single entity instances  
- 28 entity regions found in PHRO across layers 8-11
- 23 unique entity types identified by their tile ID sets
- Entity tile indices range 9-1102 (after masking bit 12)

See **Entity Spawn System** section in `blb-data-format.md` for complete documentation.

### Layer Rendering Order (Hypothesis)

Layers are likely rendered back-to-front based on priority field in Asset 201:
1. Background color (from Entry 0)
2. Far parallax layers (0-5) - tiny repeating patterns
3. Mid parallax layers (6-8) - entity objects + decorations
4. Main level layers (9-11) - entity spawn markers (platforms, objects, hazards)
5. Foreground layer (12)

### Entity vs Collision Layers (UPDATED 2026-01-05)

Previous hypothesis about layers 9-11 being "collision layers" was incorrect.

**New understanding:**
- Layers 8-11 contain **entity spawn data** encoded as tile indices with bit 12 set
- Entity tiles reference graphics from tertiary Asset 600 sprite data
- Each connected region of entity tiles = one entity instance
- True collision data is likely in **Primary Asset 601 (0x259)**, not in tilemaps

The distinction is:
- **Tilemaps (layers)**: Visual rendering + entity spawn positions
- **Asset 601**: Collision geometry and physics boundaries

**Investigation needed:** Decode Asset 601 collision format to understand actual collision detection.

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

## Sprite Animation System - ToolX (2026-01-04)

**Status:** RESEARCHING - Ghidra analysis in progress

### Developer Context

From a 1998 developer interview:

> "We took [the captured frames] and built our sequences using our proprietary
> animation scripting software, ToolX, which was written by Kenton Leach.
> Using this tool was similar to entering the images into an exposure sheet,
> which then wrote out a sequence file which could be read by our game engine."

This confirms:
1. **Exposure sheet model** - Frame metadata follows animation industry conventions
2. **Sequence files** - Animation data is separate from sprite pixel data
3. **ToolX output** - The 36-byte frame metadata structure is ToolX's output format

### Sprite Container Structure (Asset 600)

```
Container:
├── Count (u32) - Number of sprites
├── Sub-TOC (12 bytes × count):
│   ├── ID (u32)
│   ├── Size (u32)
│   └── Offset (u32)
└── Sprite data (for each entry):
    ├── Sprite Header (24 bytes)
    ├── Frame Metadata (36 bytes × frame_count)
    └── RLE Pixel Data
```

### Sprite Header (24 bytes)

```
Offset  Size  Field              Description
------  ----  -----              -----------
0x00    2     magic              Always 1
0x02    2     header_size        Always 24 (0x18)
0x04    2     frame_meta_size    Total bytes for all frame metadata
0x06    2     padding            Usually 0
0x08    4     pixel_size         Total bytes of RLE pixel data
0x0C    4     sprite_id          Same as Sub-TOC ID
0x10    2     frame_count        Number of animation frames
0x12    2     unknown_12         ?
0x14    2     unknown_14         ?
0x16    2     clut_value         CLUT VRAM position encoding (see below)
```

**Bytes per frame:** `frame_meta_size / frame_count` = 36 (0x24) bytes

### CLUT Value Encoding (2026-01-04)

**Status:** CODE-VERIFIED via `GetClut` (0x80089720) and `DumpClut` (0x80089798)

The `clut_value` (offset 0x16 in sprite header) encodes a VRAM position where
the 256-color palette is loaded at runtime:

```c
// GetClut formula (0x80089720):
u_short GetClut(int x, int y) {
    return (y << 6) | ((x >> 4) & 0x3f);
}

// DumpClut decode (0x80089798):
void DumpClut(u_short clut) {
    printf("clut: (%d,%d)\n", (clut & 0x3f) << 4, clut >> 6);
}
```

**Encoding:**
- `CLUT = (vram_y << 6) | ((vram_x >> 4) & 0x3F)`
- X is stored in low 6 bits (in units of 16 pixels)
- Y is stored in upper 10 bits

**Decoding:**
- `vram_x = (clut & 0x3F) << 4`
- `vram_y = clut >> 6`

**Example:** CLUT value 0x815D:
- X = (0x815D & 0x3F) << 4 = 29 << 4 = 464
- Y = 0x815D >> 6 = 517
- VRAM position: (464, 517)

**Palette Index Mapping:**
For this game, palettes are loaded into VRAM starting at Y=512 in the CLUT area.
Each row holds one 256-color palette. The palette index is:
- `palette_idx = vram_y - 512 + 1` (1-indexed)

For 0x815D: Y=517 → palette index = 517 - 512 + 1 = 6

### Asset 400 - Palette Container (2026-01-04)

**Status:** DATA-VERIFIED via BLB analysis

Asset 400 in the SECONDARY segment contains multiple 256-color palettes.

**Structure:**
```
Offset  Size        Field              Description
------  ----        -----              -----------
0x00    4           count              Number of palette entries
0x04    524         entry_0            Lookup table entry
0x?     524 × (N-1) entries_1..N       Raw 256-color palettes
```

**Entry 0 (Lookup Table):**
```
Offset  Size  Field         Description
------  ----  -----         -----------
0x00    12    header        VRAM RECT info (x, y, w, h, ...)
0x0C    12×N  entries       Palette sub-TOC entries
```

**Sub-TOC Entry (12 bytes each):**
```
u16 palette_idx    Index (1-7)
u16 padding
u16 size           Always 512 (256 colors × 2 bytes)
u16 padding
u16 offset         Offset within Asset 400 to palette data
u16 padding
```

**Palette Data:**
Each palette is 512 bytes = 256 colors × 2 bytes (PSX 15-bit RGB).

**PSX 15-bit Color Format:**
```
Bit 15:    STP (semi-transparency bit, usually 0)
Bits 10-14: Blue (0-31)
Bits 5-9:   Green (0-31)
Bits 0-4:   Red (0-31)
```

Conversion to 8-bit RGB:
```c
r = (color & 0x1F) << 3;
g = ((color >> 5) & 0x1F) << 3;
b = ((color >> 10) & 0x1F) << 3;
```

**PHRO Level Example:**
- Asset 400 size: 4196 bytes
- Palette count: 8
- Palette offsets: 612, 1124, 1636, 2148, 2660, 3172, 3684
- Tertiary sprites use palette 6 (CLUT 0x815D → Y=517 → index 6)

### Frame Metadata (36 bytes per frame)

From `FUN_8007bebc` (frame accessor) and `FUN_8007bc3c` (sprite context init):

```
Offset  Size  Field              Description
------  ----  -----              -----------
0x00    ?     unknown_00         
0x0A    2     width              Frame width in pixels
0x0C    2     height             Frame height in pixels
0x0E    ?     unknown_0E         
0x20    4     pixel_offset       Offset to RLE data for this frame
0x24    -     (end of frame)
```

The remaining 28 bytes likely contain **exposure sheet data**:
- X/Y offset (hotspot/origin)
- Duration (frames to display)
- Flip flags
- Palette override
- Sound trigger
- Hit/hurt box offsets

### Sprite Context Structure (20 bytes, runtime)

From `InitSpriteContext` (0x8007bc3c):

```c
struct SpriteContext {
    void* frame_metadata;   // [0] Base pointer to frame metadata
    void* secondary_ptr;    // [1] From sprite_data+4 (pixel offset table?)
    void* pixel_data;       // [2] RLE pixel data base
    u16   max_width;        // [3] +0x0C: Maximum width across all frames
    u16   max_height;       //     +0x0E: Maximum height across all frames
    u16   frame_count;      // [4] +0x10: Number of frames
    u8    unknown_12;       //     +0x12: Unknown byte
    u8    valid_flag;       //     +0x13: 1 = initialized, 0 = invalid
};
```

### Sprite System Functions (renamed in Ghidra)

| Address | Name | Purpose |
|---------|------|---------|
| 0x80010068 | `DecodeRLESprite` | RLE decompression with mirror support |
| 0x8007bf7c | `DecodeRLESpriteChecked` | Checks +0x13 valid flag before decode |
| 0x8007bde8 | `RenderSprite` | Sprite render orchestration |
| 0x8007bebc | `GetFrameMetadata` | Gets frame metadata: `ctx[0] + frame_idx * 0x24` |
| 0x8007bc3c | `InitSpriteContext` | Initializes SpriteContext from sprite data |
| 0x8007bc18 | `ClearSpriteContext` | Zeros 20-byte context |
| 0x8007bb10 | `LookupSpriteById` | Finds sprite by ID in DAT_800a6064/60 |
| 0x8007bb00 | `SetSpriteTables` | Sets global sprite table pointers |
| 0x8007b968 | `FindSpriteInTOC` | Searches ctx+0x70, then ctx+0x40 (12B stride) |

### Global Sprite Tables

From `FUN_8007bb10`:
- `DAT_800a6064` - Primary sprite table (searched first)
- `DAT_800a6060` - Secondary sprite table (fallback)

These are set by `FUN_8007bb00` during level loading.

### Connection to Assets 500-503

The animation sequence data (exposure sheet) may be split across:
- **Asset 401** (SEC/TERT): Palette animation config
- **Asset 500** (TERT): Unknown container - possibly animation sequences
- **Asset 501** (TERT): VRAM texture coordinates
- **Asset 502** (TERT): VRAM rectangles
- **Asset 503** (TERT): Animation frame offsets - **likely ToolX sequence data**

**Hypothesis:** Asset 503's "animation frame offsets" correspond to the sequence
files output by ToolX, providing timing and ordering for sprite playback.

### Sprite Type 2 - Extended Animation Sequences (2026-01-05)

**Status:** DATA-VERIFIED - Frame count formula confirmed across all Type 2 sprites

Type 2 sprites (`type=2` at offset 0x00) have a 36-byte header (vs 24-byte for type 1)
and contain additional frame entries beyond the base `frame_count`.

**VERIFIED FORMULA:**
```
Total 36-byte frame entries = frame_count (offset 0x10) + extra_frames (offset 0x1C)
```

**Extended Header Structure (Type 2 only, 36 bytes):**
```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    24    base_header     Same as Type 1 sprite header
0x18    4     unknown_18      Varies (sprite ID related?)
0x1C    2     extra_frames    Additional frame entries beyond frame_count
0x1E    2     unknown_1E      Varies (360 for most, 2736 for PIRA)
0x20    2     unknown_20      0 or 1 (animation flags?)
0x22    2     clut_copy       Always 0x815D (same garbage as offset 0x16)
```

**Verification (100% match across all Type 2 sprites):**
```
PIRA spr2:  76 + 23 = 99 frames ✓
LEGL spr12: 10 + 10 = 20 frames ✓
SNOW spr8:  25 + 25 = 50 frames ✓
GLID spr2:   3 + 10 = 13 frames ✓
```

**Animation structure hypothesis:**
- `frame_count`: Number of frames in primary animation segment
- `extra_frames`: Number of frames in secondary animation segment
- Both use standard 36-byte frame entry format
- RLE offsets in extra segment often reference same data as primary (frame reuse)

**Example: LEGL sprite 12 (20 total frames = 10 + 10)**
```
Frame  0:  51x55  rle=0x0000  (start unique)
Frame  1:  60x38  rle=0x0484  (start unique)
Frame  2:  40x81  rle=0x08C8  (main sequence)
Frame  3:  51x104 rle=0x0F08  ...
Frame  4:  50x116 rle=0x15B4  ...
Frame  5:  49x116 rle=0x1C58  ...
Frame  6:  49x117 rle=0x2398  ...
Frame  7:  49x108 rle=0x2AD8  ...
Frame  8:  50x96  rle=0x31B0  ...
Frame  9:  48x69  rle=0x37B8  (main sequence end)
--- Extra frames below ---
Frame 10:  28x24  rle=0x3D50  (end unique)
Frame 11:  29x20  rle=0x3F68  (end unique)
Frame 12:  40x81  rle=0x08C8  (REUSES frame 2)
Frame 13:  51x104 rle=0x0F08  (REUSES frame 3)
Frame 14:  50x116 rle=0x15B4  (REUSES frame 4)
...
Frame 19:  48x69  rle=0x37B8  (REUSES frame 9)
```

**Hypothesis:** The extra frames provide a "reverse" or alternate animation path:
- Primary: Full forward animation (intro → main → peak)
- Extra: Reverse back (outro → loop back through main sequence)
- This creates smooth looping or state transitions without duplicating RLE data

### Sprite Unknowns Summary (2026-01-05)

**Status:** UNCONFIRMED - Collected from extraction analysis

#### Sprite Header Unknowns

| Offset | Size | Values Observed | Hypothesis |
|--------|------|-----------------|------------|
| 0x14 | 2 | 0, 1 | Boolean flag - animation loop? single-shot? |
| 0x16 | 2 | Always 0x815D | Garbage/padding - NOT actually CLUT (palette loaded at runtime) |

**Note:** The CLUT value at 0x16 appears to be leftover data from ToolX export.
At runtime, the game loads palettes from Asset 400 to fixed VRAM positions.

#### Frame Entry Unknowns (36-byte structure)

| Offset | Size | Values Observed | Hypothesis |
|--------|------|-----------------|------------|
| 0x00-0x01 | 2 | 0, 32, 79, 576, 648... | Purpose unknown - possibly tool artifact |
| 0x02-0x03 | 2 | 0, 64, 144, 257... | Purpose unknown |
| 0x04-0x05 | 2 | 1, 2, 3 (multi), 900 (single) | Frame index? RLE size for single-frame? |
| 0x0E-0x0F | 2 | 0, 3, 5, 8, 10, 65519 | Animation timing? (0-10 range with outliers) |
| 0x10-0x11 | 2 | Various | Unknown |

#### Confirmed Frame Entry Fields

| Offset | Size | Field | Verified By |
|--------|------|-------|-------------|
| 0x06 | 2 | render_x_offset (signed) | Ghidra + extraction |
| 0x08 | 2 | render_y_offset (signed) | Ghidra + extraction |
| 0x0A | 2 | width | `GetFrameMetadata` (0x8007bebc) |
| 0x0C | 2 | height | `GetFrameMetadata` (0x8007bebc) |
| 0x12 | 2 | hitbox_x (signed) | Data pattern analysis |
| 0x14 | 2 | hitbox_y (signed) | Data pattern analysis |
| 0x16 | 2 | hitbox_width | Data pattern analysis |
| 0x18 | 2 | hitbox_height | Data pattern analysis |
| 0x1A | 6 | padding (zeros) | Consistent across all sprites |
| 0x20 | 4 | rle_offset | `GetFrameMetadata` (0x8007bebc) |

### Next Steps

1. ✅ Rename sprite functions in Ghidra (9 functions renamed)
2. ✅ Decode CLUT value encoding (GetClut/DumpClut analysis)
3. ✅ Parse Asset 400 palette container structure
4. ✅ Create sprite extraction script (`scripts/extract_rle_sprites.py`)
5. ✅ Extract and verify sprites with correct palette colors
6. ☐ Map complete 36-byte frame metadata (bytes 0-5, 0x0E-0x11)
7. ☐ Decode Type 2 second 36-byte block
8. ☐ Decode Asset 503 to find sequence/timing data

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

## LevelDataContext Accessor Functions (2026-01-04, UPDATED 2026-01-05)

**Status:** CODE-VERIFIED via Ghidra (functions renamed)

These accessor functions operate on the LevelDataContext structure (GameState + 0x84).
They provide the link between loaded asset data and game rendering.

### Asset 600/200/201 (Level Layer/Sprite) Accessors

| Function | Address | Returns | Description |
|----------|---------|---------|-------------|
| `GetLayerCount` | 0x8007b6c8 | u16 | Entry count: `**(u16**)(ctx + 0xc)` (from Asset 200) |
| `GetLayerEntry` | 0x8007b700 | ptr | Entry by index: `*(ptr*)(ctx+0x10) + i * 0x5c` (from Asset 201) |
| `GetTilemapDataPtr` | 0x8007b6dc | ptr | Tilemap data from Asset 200 sub-TOC |
| `GetAnimatedTileData` | 0x8007b658 | ptr | Animated tile lookup from ctx[11] |

### Asset 100 (Tile Header) Accessors

| Function | Address | Returns | Field |
|----------|---------|---------|-------|
| `GetLevelDimensions` | 0x8007b434 | void | Writes ctx+4 offsets 0x8, 0xB (width, height) |
| `FUN_8007b458` | 0x8007b458 | void | Writes ctx+4 offsets 0xC, 0xF (alt dims?) |
| `FUN_8007b4b8` | 0x8007b4b8 | ptr | Background RGB (offset 0x00 in Asset 100) |
| `FUN_8007b4c4` | 0x8007b4c4 | ptr | Secondary RGB (offset 0x04 in Asset 100) |
| `GetPaletteGroupCount` | 0x8007b4d0 | u8 | Palette count from Asset 400 |
| `GetTotalTileCount` | 0x8007b53c | u16 | Sum of offsets 0x10+0x12+0x14 in Asset 100 |
| `CopyTilePixelData` | 0x8007b588 | void | Copy tile pixels to buffer (8bpp) |
| `GetPaletteIndices` | 0x8007b6b0 | ptr | Asset 301 pointer (ctx[6]) |
| `GetTileSizeFlags` | 0x8007b6bc | ptr | Asset 302 pointer (ctx[7]) |
| `GetAsset100Field1C` | 0x8007b7c8 | u16 | Unknown field at +0x1C |
| `FUN_8007b7dc` | 0x8007b7dc | ptr | ctx+0x3C (Asset 501?) |

### Asset 400 (Palette) Accessors

| Function | Address | Returns | Description |
|----------|---------|---------|-------------|
| `GetPaletteGroupCount` | 0x8007b4d0 | u8 | Palette count from Asset 400 sub-TOC |
| `GetPaletteDataPtr` | 0x8007b4f8 | ptr | Palette color data pointer (navigates sub-TOC) |
| `GetPaletteAnimData` | 0x8007b530 | ptr | Palette animation flags from ctx[9] (Asset 401) |

### Core Loading Functions

| Function | Address | Description |
|----------|---------|-------------|
| `LoadAssetContainer` | 0x8007b074 | Parses sub-TOC, populates ctx[1-22] based on asset IDs |
| `LoadTileDataToVRAM` | 0x80025240 | Uploads tiles to VRAM (called after LoadAssetContainer) |
| `InitLevelDataContext` | 0x8007a1bc | Initializes context with header pointer and callback |
| `LevelDataParser` | 0x8007a62c | Parses primary TOC, calls LoadAssetContainer |

### Context Field Layout (partial)

From accessor analysis, the LevelDataContext fields at low offsets:

```
Word   Offset  Description                           Source / Accessor
----   ------  -----------                           -----------------
[1]    0x04    Asset 100 pointer (tile header)       GetLevelDimensions, GetTotalTileCount
[3]    0x0C    Asset 200 pointer (tilemap TOC)       GetLayerCount (reads count at *[3])
[4]    0x10    Asset 201 pointer (layer entries)     GetLayerEntry (entries are 92 bytes each)
[5]    0x14    Asset 300 pointer (tile pixels)       CopyTilePixelData
[6]    0x18    Asset 301 pointer (palette indices)   GetPaletteIndices
[7]    0x1C    Asset 302 pointer (size flags)        GetTileSizeFlags
[8]    0x20    Asset 400 pointer (palette container) GetPaletteDataPtr
[9]    0x24    Asset 401 pointer (palette anim)      GetPaletteAnimData
[11]   0x2C    Asset 500 pointer (animated tiles)    GetAnimatedTileData
[15]   0x3C    Asset 501 pointer                     FUN_8007b7dc
```

**Note:** These are separate from the main LevelDataContext offsets documented in
blb-data-format.md. The ctx+0x04 here refers to content WITHIN the loaded asset
buffers, accessed via the base pointer stored in the context.

---

## Secondary Asset 601 - AUDIO SAMPLES (2026-01-04)

**Status:** VERIFIED - Asset 601 contains audio samples uploaded to SPU RAM

### Discovery

By tracing `GetAsset601Ptr` (0x8007ba78) through Ghidra, found that Asset 601 data 
is passed to `UploadAudioToSPU` (0x8007c088) which calls:
- `SpuSetTransferMode(0)`
- `SpuSetTransferStartAddr(addr)`
- `SpuRead(data, size)`
- `SpuIsTransferCompleted(1)`

This confirms Asset 601 contains **audio sample data** (likely ADPCM format for PSX SPU).

### Key Code

From `InitializeAndLoadLevel` (0x8007d1d0):
```c
uVar9 = GetAsset601Ptr(pLevelDataCtx);    // Audio samples
uVar10 = GetAsset602Ptr(pLevelDataCtx);   // Audio metadata (0x3FFF = all channels?)
uVar11 = GetAsset601Size(pLevelDataCtx);  // Size in bytes
UploadAudioToSPU(uVar9, uVar10, uVar11);
```

### Key Findings

1. **Location**: Asset 601 is in **secondary** containers only (not tertiary).
   - This explains why it uses different IDs than Asset 600 sprites.

2. **Distribution**:
   - Levels with 601: MENU(21), LEGL(19), SEVN(19), CLOU(15), CSTL(14), etc.
   - Levels WITHOUT 601: PHRO, SCIE, BOIL, CAVE, SOAR, WIZZ (silent levels?)

3. **Container Structure**: Same TOC format as other assets:
   ```
   u32  entry_count      // Number of audio samples
   [entry_count x 12 bytes]:
       u32  sample_id    // Unique hash ID
       u32  data_size    // Entry size in bytes
       u32  data_offset  // Offset from container start
   ```

4. **Entry Internal Structure** (each audio sample):
   ```
   Offset  Size  Description
   0x00    16    Zero padding (SPU alignment?)
   0x10    4     Frame count? (seen: 72, 19, 57, 20)
   0x14    var   Audio sample data (ADPCM encoded)
   ```

5. **Asset 602**: Contains audio metadata/parameters (2-byte entries per sample).
   Default value 0x3FFF may mean "all channels" or "full volume".

### Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `GetAsset601Ptr` | 0x8007ba78 | Returns pointer to audio samples |
| `GetAsset601Size` | 0x8007ba50 | Returns size of audio data |
| `GetAsset602Ptr` | 0x8007baa0 | Returns pointer to audio metadata |
| `UploadAudioToSPU` | 0x8007c088 | Uploads samples to SPU RAM |

### Updated Asset Naming

Previous documentation incorrectly labeled 601 as "collision data". The correct mapping:
- Asset 600 (0x258): RLE sprite pixel data (in tertiary sub-blocks)
- Asset 601 (0x259): Audio samples for SPU (in secondary container)
- Asset 602 (0x25A): Audio metadata/parameters (in secondary container)

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

## Layer/Entity Data Accessors (2026-01-04, UPDATED 2026-01-05)

**Status:** Decompiled via Ghidra, functions renamed

Functions that access layer/entity data from the tertiary segment:

### GetLayerCount (0x8007B6C8)

```c
u16 GetLayerCount(int ctx) {
    return **(u16**)(ctx + 0x0C);  // ctx[3] → Asset 200 (tilemapContainer)
}
```

Returns the layer count from the first u16 of Asset 200 (0xC8).
- `ctx + 0x0C` = LevelDataContext[3] = tilemapContainer pointer
- Reads count from the beginning of the asset data

### GetLayerEntry (0x8007B700)

```c
int GetLayerEntry(int ctx, uint index) {
    return *(int*)(ctx + 0x10) + (index & 0xFFFF) * 0x5C;
}
```

Returns pointer to layer entry at given index.
- `ctx + 0x10` = LevelDataContext[4] = layerEntries pointer
- Each entry is **0x5C (92) bytes**
- Asset 201 (0xC9) contains the layer definition table

### Layer Entry Structure (0x5C = 92 bytes, PARTIAL)

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

**Layer Type 3:** When `*(short*)(entry + 0x26) == 3`, the layer is skipped in FUN_80024778.

**First Layer Special Handling:** When index == 0, copies RGB from entry+0x2C to GameState+0x124 (background color).

### GetTilemapDataPtr (0x8007B6DC)

Called for each layer in FUN_80024778. Returns tilemap data pointer for the layer by navigating the Asset 200 sub-TOC.

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
