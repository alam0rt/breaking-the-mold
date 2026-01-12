# Asset Handling Analysis

> ⚠️ **DEPRECATED**: This document has been reorganized.
> See the new documentation:
> - [Asset Types Reference](../blb/asset-types.md)
> - [Level Loading](../systems/level-loading.md)
> - [LevelDataContext](../reference/level-data-context.md)
>
> This file is kept for reference but will not be updated.

---

This document describes how assets are loaded and processed at runtime.

## Overview

Assets are loaded from the BLB file via `LoadAssetContainer`, which parses the sub-TOC and stores pointers in `LevelDataContext`. These pointers are then accessed by various subsystems (graphics, tilemaps, audio) through accessor functions.

### Data Segment Types

The BLB file contains three types of data segments per level:

| Segment | Contents | Asset Types |
|---------|----------|-------------|
| **Primary** | Level geometry, collision, palette | 600 (geometry), 601, 602 |
| **Secondary** | Tiles, tile metadata, palettes | 100, 300, 301, 302, 400, 401, 601, 602 |
| **Tertiary** | Sprites, entities, layers, audio | 100, 200, 201, 302, 401, 500, **501 (entities)**, 502-503, 600 (sprites), 700 |

**Note:** Asset 600 appears in BOTH primary (level geometry) and tertiary (RLE sprites).
Same asset ID, different purposes. Verified via runtime analysis of SCIE level.

**Note:** Asset 501 contains entity placement data (24-byte structures per entity).
Entity types: 2=clayball, 3=ammo, 8=item, 24=special ammo, 25/27=enemies, 45=message box, etc.

### Container vs Raw Assets

| Type | Format | Examples |
|------|--------|----------|
| **Container** | Has sub-TOC with multiple entries | 0x258, 0x259, 0x190 |
| **Raw** | Data starts immediately, no sub-TOC | 0x25A, 0x064, 0x12C, 0x12D, 0x12E, 0x191 |

---

## Primary Segment Assets (Level Data)

### Asset 600 (0x258) - Level Geometry

**Structure:** CONTAINER with sub-TOC

Contains the main level graphics and world layout data.

```
Offset  Size    Description
------  ----    -----------
0x00    u32     Sub-entry count
0x04+   12×N    Sub-TOC entries: {flags, size, offset}
...     var     Graphics/geometry data
```

- **Typical size:** 500KB - 1MB
- **Consumer:** Level rendering subsystem
- **LevelDataContext offset:** 0x40 [16] (pointer), 0x44 [17] (size)

### Asset 601 (0x259) - Audio Sample Bank (CODE-VERIFIED)

**Structure:** CONTAINER with sample entries

Contains SPU ADPCM audio samples for sound effects. Verified via `UploadAudioToSPU` @ 0x8007c088.

```
Offset  Size    Description
------  ----    -----------
0x00    u16     Sample count
0x02    u16     Reserved (always 0)
0x04    12×N    Sample entries (AudioSampleEntry)
        var     ADPCM audio data

AudioSampleEntry (12 bytes each):
  0x00  u32     Sample ID (hash identifier)
  0x04  u32     SPU size (bytes in SPU RAM)
  0x08  u32     Data offset (within audio data block)
```

- **Typical size:** 30KB - 150KB (varies by stage audio needs)
- **Consumer:** `UploadAudioToSPU` - transfers to SPU RAM at 0x1010+
- **LevelDataContext offset:** 0x48 [18] (pointer), 0x4C [19] (size)

### Asset 602 (0x25A) - Primary Palette

**Structure:** RAW (no sub-TOC)

Small palette data for primary segment rendering.

```
Format: Array of PSX 15-bit RGB values (u16 each)
  - Bits 0-4:   Red (0-31, ×8 for 8-bit)
  - Bits 5-9:   Green (0-31, ×8 for 8-bit)
  - Bits 10-14: Blue (0-31, ×8 for 8-bit)
  - Bit 15:     STP (semi-transparency)
```

- **Typical size:** 24-200 bytes
- **LevelDataContext offset:** 0x50 [20]

---

## Secondary Segment Assets (Tiles & Tilemaps)

### Asset 100 (0x064) - Tile Header (VERIFIED)

**Structure:** RAW, 36 bytes

Header containing tile counts and background colors.

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    3     u8[3]   Background RGB color
0x03    1     u8      Padding
0x04    3     u8[3]   Secondary RGB color  
0x07    1     u8      Padding
0x08    2     u16     Unknown count
0x0A    2     u16     Unknown count
0x0C    4     u8[4]   Unknown
0x10    2     u16     16×16 tile count (VERIFIED)
0x12    2     u16     8×8 tile count (VERIFIED)
0x14    2     u16     (unused, always 0)
0x16    14    var     Remaining header data
```

- **LevelDataContext offset:** 0x04 [1]
- **Accessor:** `GetTotalTileCount` (0x8007B53C) - returns sum of counts at +0x10, +0x12, +0x14

### Asset 101 (0x065) - Secondary Header

**Structure:** RAW (optional)

Additional geometry header, purpose not fully understood.

- **LevelDataContext offset:** 0x08 [2]

### Asset 200 (0x0C8) - Tilemap Container Header

**Structure:** RAW

Header with layer count.

```
Offset  Size  Description
------  ----  -----------
0x00    u16   Layer count
0x02+   var   Additional header data
```

- **LevelDataContext offset:** 0x0C [3]
- **Accessor:** `GetLayerCount` (0x8007B6C8) - returns u16 at offset 0

### Asset 201 (0x0C9) - Layer Entries (VERIFIED)

**Structure:** RAW, array of 92-byte entries

Each layer entry defines position, dimensions, and behavior.

```
Offset  Size  Description
------  ----  -----------
0x00    4     Position X (u32)
0x04    4     Position Y (u32)
0x08    2     Width (u16)
0x0A    2     Height (u16)
0x0C    2     Unknown
0x0E    2     Priority/depth value
0x10    4     Velocity X
0x14    4     Velocity Y
0x18    4     Unknown
0x1C    2     Unknown
0x1E    1     Flag: enables param_1+0x59
0x1F    1     Flag: enables param_1+0x5B
0x20    1     Flag: high byte of word[8]
0x21    1     Flag: enables param_1+0x5A
0x22-0x25     Unknown
0x26    1     Layer type (≠3 to process)
0x28-0x2B     Unknown  
0x2C    4     Animation data pointer offset
0x30-0x5B     Additional data (60 bytes)
```

- **LevelDataContext offset:** 0x10 [4]
- **Accessor:** `GetLayerEntry` (0x8007B700) - returns `ctx[4] + index * 0x5C`

### Asset 300 (0x12C) - Tile Pixel Data (VERIFIED)

**Structure:** RAW, 8-bit indexed pixels

Contains all tile graphics as 8bpp indexed pixel data.

**Layout:**
1. **16×16 tiles first:** 256 bytes each (16 rows × 16 pixels)
2. **8×8 tiles after:** 128 bytes each (8 rows × 16-byte stride, only first 8 used)

```python
# Tile data location calculation
if tile_index < count_16x16:
    offset = tile_index * 256
    size = 256  # 16×16
else:
    adjusted = tile_index - count_16x16
    offset = count_16x16 * 256 + adjusted * 128
    size = 128  # 8×8
```

- **LevelDataContext offset:** 0x14 [5]
- **Accessor:** `CopyTilePixelData` (0x8007B588)

### Asset 301 (0x12D) - Palette Assignment (VERIFIED)

**Structure:** RAW, 1 byte per tile

Specifies which palette from Asset 400 to use for each tile.

```
Size: count_16x16 + count_8x8 bytes
Value: Palette index (0 to palette_count-1)

For tile[i]: palette = asset400.palettes[asset301[i]]
```

- **LevelDataContext offset:** 0x18 [6]
- **Accessor:** `GetPaletteIndices` (0x8007B6B0)

### Asset 302 (0x12E) - Tile Flags (VERIFIED)

**Structure:** RAW, 1 byte per tile

Per-tile rendering flags controlling GPU upload behavior.

| Bit | Mask | Description |
|-----|------|-------------|
| 0 | 0x01 | Semi-transparency: enables alpha blending (SetSemiTrans) |
| 1 | 0x02 | Tile size: 0=16×16 pixels, 1=8×8 pixels |
| 2 | 0x04 | Skip flag: if set, don't upload tile to GPU |

- **LevelDataContext offset:** 0x1C [7]
- **Accessor:** `GetTileSizeFlags` (0x8007B6BC)

### Asset 400 (0x190) - Palette Container (VERIFIED)

**Structure:** CONTAINER with sub-TOC

Contains multiple 256-color palettes for tiles.

```
Offset  Size   Description
------  ----   -----------
0x00    u32    Palette count
0x04+   12×N   Sub-entries: {palette_index, size, offset}

Each palette: 512 bytes = 256 × u16 (PSX 15-bit RGB)
  - Color 0 is typically transparent
  - Size always 512 bytes per palette
```

- **LevelDataContext offset:** 0x20 [8]
- **Accessor:** `GetPaletteGroupCount` (0x8007B4D0), `GetPaletteDataPtr` (0x8007B4F8)

### Asset 401 (0x191) - Animation/Palette Config

**Structure:** RAW

Configuration data for palette animations or cycling.

- **LevelDataContext offset:** 0x24 [9]

---

## Tertiary Segment Assets (Entities, Sprites, Audio)

### Asset 500 (0x1F4) - Sprite Metadata

**Structure:** RAW

Sprite metadata referenced by entity system.

- **LevelDataContext offset:** 0x2C [11]

### Asset 501 (0x1F5) - Entity Placement Data (VERIFIED)

**Structure:** RAW, array of 24-byte entity structures

Entity placement data for collectibles, ammo, enemies, and other placed objects.
Verified via Ghidra analysis of entity loader (FUN_80024dc4) and entity count accessor (FUN_8007b7a8).

```
Entity Structure (24 bytes):
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    x1 - Bounding box min X (pixels)
0x02    2     u16    y1 - Bounding box min Y (pixels)
0x04    2     u16    x2 - Bounding box max X (pixels)
0x06    2     u16    y2 - Bounding box max Y (pixels)
0x08    2     u16    x_center - Entity center X (pixels)
0x0A    2     u16    y_center - Entity center Y (pixels)
0x0C    2     u16    variant - Animation frame or subtype selector
0x0E    4     u32    padding1 - Always 0
0x12    2     u16    entity_type - Type ID (2=clayball, 3=ammo, etc)
0x14    2     u16    layer - Render layer (1, 2, or 3)
0x16    2     u16    padding2 - Always 0
```

**Known entity types (verified 2026-01-10):**
- Type 2 = Clayballs (collectible coins) - 5727 total
- Type 3 = Ammo pickup (bullets for player weapon) - 308 total  
- Type 8 = Item pickup - 144 total
- Type 24 = Special ammo pickup - 227 total
- Type 25, 27 = Enemies
- Type 28, 48 = Moving platforms
- Type 45 = Message box

**IMPORTANT:** Entity type → sprite ID mapping is HARDCODED in game code, not in BLB data.

- **LevelDataContext offset:** 0x38 [14]

### Asset 502-503 (0x1F6-0x1F7) - Audio Configuration

**Structure:** RAW

Audio configuration and index data.

| Asset | Hex | LevelDataContext Offset |
|-------|-----|------------------------|
| 502 | 0x1F6 | 0x3C [15] |
| 503 | 0x1F7 | 0x30 [12] |

### Asset 700 (0x2BC) - Audio/Music Data

**Structure:** Unknown (likely VAB/VH+VB)

Main audio data uploaded to SPU.

- **LevelDataContext offset:** 0x54 [21] (pointer), 0x58 [22] (size)
- **Consumer:** `UploadAudioToSPU` (0x8007C088) - uses SpuSetTransferMode, etc.

---

## Complete Asset ID Mapping

All asset IDs and their LevelDataContext mappings:

| Asset ID | Hex | Word Idx | Ctx Offset | Structure | Description |
|----------|-----|----------|------------|-----------|-------------|
| 100 | 0x064 | [1] | 0x04 | RAW | Tile header (36 bytes) |
| 101 | 0x065 | [2] | 0x08 | RAW | Secondary geometry header |
| 200 | 0x0C8 | [3] | 0x0C | RAW | Tilemap container header |
| 201 | 0x0C9 | [4] | 0x10 | RAW | Layer entries (92 bytes each) |
| 300 | 0x12C | [5] | 0x14 | RAW | Tile pixel data (8bpp) |
| 301 | 0x12D | [6] | 0x18 | RAW | Palette index per tile |
| 302 | 0x12E | [7] | 0x1C | RAW | Tile flags |
| 303 | 0x12F | [10] | 0x28 | RAW | Unknown |
| 400 | 0x190 | [8] | 0x20 | CONTAINER | Palette container |
| 401 | 0x191 | [9] | 0x24 | RAW | Animation/palette config |
| 500 | 0x1F4 | [11] | 0x2C | RAW | Sprite metadata |
| 501 | 0x1F5 | [14] | 0x38 | RAW | **Entity placement (24-byte structs)** |
| 502 | 0x1F6 | [15] | 0x3C | RAW | Audio config |
| 503 | 0x1F7 | [12] | 0x30 | RAW | Audio config |
| 504 | 0x1F8 | [13] | 0x34 | RAW | Audio config |
| 600 | 0x258 | [16-17] | 0x40 | CONTAINER | Level geometry + size |
| 601 | 0x259 | [18-19] | 0x48 | CONTAINER | Collision data + size |
| 602 | 0x25A | [20] | 0x50 | RAW | Palette data |
| 700 | 0x2BC | [21-22] | 0x54 | RAW | Audio/music + size |

---

## Accessor Function Reference

| Function | Address | Asset | Returns |
|----------|---------|-------|---------|
| `GetTotalTileCount` | 0x8007B53C | 100 | Sum of u16s at +0x10, +0x12, +0x14 |
| `CopyTilePixelData` | 0x8007B588 | 300 | Pointer to tile pixels |
| `GetPaletteIndices` | 0x8007B6B0 | 301 | Pointer to palette index array |
| `GetTileSizeFlags` | 0x8007B6BC | 302 | Pointer to tile flags array |
| `GetLayerCount` | 0x8007B6C8 | 200 | u16 layer count |
| `GetLayerEntry` | 0x8007B700 | 201 | Pointer to 92-byte layer entry |
| `GetTilemapDataPtr` | 0x8007B6DC | 200 | Tilemap data pointer |
| `GetPaletteGroupCount` | 0x8007B4D0 | 400 | Palette group count |
| `GetPaletteDataPtr` | 0x8007B4F8 | 400 | Pointer to palette sub-TOC entry |

---

## GPU Upload Process (LoadTileDataToVRAM)

Called from `InitializeAndLoadLevel`, uploads tiles to VRAM:

```c
// Get data pointers
header = ctx[1];                                // Asset 100
tile_pixels = CopyTilePixelData(ctx);           // Asset 300  
palette_indices = GetPaletteIndices(ctx);       // Asset 301
tile_flags = GetTileSizeFlags(ctx);             // Asset 302
palettes = ctx[8];                              // Asset 400

// Get tile counts from header
count_16x16 = *(u16*)(header + 0x10);
count_8x8 = *(u16*)(header + 0x12);
total_tiles = count_16x16 + count_8x8;

for (int i = 0; i < total_tiles; i++) {
    u8 flags = tile_flags[i];
    
    // Skip if bit 2 set
    if (flags & 0x04) continue;
    
    // Bit 1 indicates tile size (0=16×16, 1=8×8)
    bool is_8x8 = (flags & 0x02) != 0;
    
    // Get palette for this tile
    int pal_idx = palette_indices[i];
    u16* palette = palettes[pal_idx];
    
    // Calculate tile data offset
    u8* pixels;
    RECT rect;
    if (i < count_16x16) {
        pixels = tile_pixels + i * 256;
        rect.w = 16; rect.h = 16;
    } else {
        pixels = tile_pixels + count_16x16 * 256 + (i - count_16x16) * 128;
        rect.w = 8; rect.h = 8;
    }
    
    // Upload to VRAM
    LoadImage(&rect, pixels);
    DrawSync(0);
    
    // Store render attributes
    output[i].tpage = GetTPage(depth, ...);
    output[i].clut = GetClut(...);
    output[i].render_attr = flags & 0x01;
}
```

---

## Layer Creation Process (CreateTilemapLayers)

Called from `InitializeAndLoadLevel`, creates tilemap layer objects:

```c
int count = GetLayerCount(ctx);

for (int i = 0; i < count; i++) {
    LayerEntry* entry = GetLayerEntry(ctx, i);
    
    // Skip type 3 layers
    if (entry->type == 3) continue;
    
    int w = entry->width;
    int h = entry->height;
    
    // Choose creation function based on size/flags
    if (w <= 64 && h <= 64 && entry->flags) {
        layer = InitLayerObjectLarge(entry);
        AddLayerToRenderListA(layer);
    } else if (w <= 128 && h <= 128) {
        layer = InitLayerObjectMedium(entry);
        AddLayerToRenderListB(layer);
    } else {
        layer = InitLayerObjectStandard(entry);
        AddLayerToRenderListC(layer);
    }
}
```

---

## Complete Loading Flow

```
BLB File (GAME.BLB)
  │
  ├─→ LoadBLBHeader (0x800208B0)
  │     └─→ Reads header (0x1000 bytes) to 0x800AE3E0
  │     └─→ InitLevelDataContext (0x8007A1BC)
  │           └─→ Sets blbHeaderBuffer, loaderCallback
  │
  └─→ InitializeAndLoadLevel (0x8007D1D0)
        │
        ├─→ LevelDataParser (0x8007A62C)
        │     └─→ Reads primary segment from BLB
        │     └─→ Parses primary TOC (assets 600, 601, 602)
        │     └─→ Stores pointers at ctx+0x68 through ctx+0x7C
        │
        ├─→ LoadAssetContainer (0x8007B074) [Secondary]
        │     └─→ Reads secondary segment from BLB
        │     └─→ Parses sub-TOC (assets 100-401)
        │     └─→ Populates asset pointers ctx[1]-ctx[9]
        │
        ├─→ LoadAssetContainer (0x8007B074) [Tertiary]
        │     └─→ Reads tertiary segment from BLB
        │     └─→ Parses sub-TOC (assets 500-700)
        │     └─→ Populates asset pointers ctx[11]-ctx[22]
        │
        ├─→ UploadAudioToSPU (0x8007C088)
        │     └─→ Uses asset 700 (audio data)
        │     └─→ SpuSetTransferMode, SpuWrite, etc.
        │
        ├─→ LoadTileDataToVRAM (0x80025240)
        │     └─→ Uses assets 100, 300, 301, 302, 400
        │     └─→ LoadImage → uploads tiles to VRAM
        │
        └─→ CreateTilemapLayers (0x80024778)
              └─→ Uses assets 200, 201
              └─→ Creates tilemap layer objects from entry data
              └─→ Adds layers to render lists
```

---

## Related Documentation

- [BLB Data Format](blb-data-format.md) - File format and LevelDataContext structure
- [Runtime Behavior](runtime-behavior.md) - Game loop and state machine
