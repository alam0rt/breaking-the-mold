# Asset Handling Analysis

This document describes how assets are loaded and processed at runtime.

## Overview

Assets are loaded from the BLB file via `LoadAssetContainer`, which parses the sub-TOC and stores pointers in `LevelDataContext`. These pointers are then accessed by various subsystems (graphics, sprites, audio) through accessor functions.

## Asset 200/201 (Sprites)

### Loading Flow

1. **LoadAssetContainer** parses sub-TOC and stores:
   - Asset 200 pointer → `LevelDataContext[3]` (offset 0x0C) - sprite header
   - Asset 201 pointer → `LevelDataContext[4]` (offset 0x10) - sprite entries

2. **Data Format**:
   - Asset 200: Header with u16 sprite count at offset 0
   - Asset 201: Array of 92-byte (0x5C) sprite entry structures

### Accessor Functions

| Function | Address | Returns |
|----------|---------|---------|
| `GetSpriteCount` | 0x8007B6C8 | `**ctx[3]` - first u16 from asset 200 |
| `GetSpriteEntry` | 0x8007B700 | `ctx[4] + index * 0x5C` - 92-byte entry |
| `GetSpriteMetadata` | 0x8007B6DC | Sprite metadata by index |

### Consumer: FUN_80024778 (Sprite Object Creation)

Called from `InitializeAndLoadLevel`, this function:

1. Iterates over all sprites using `GetSpriteCount`
2. For each sprite entry (92 bytes from asset 201):
   - Reads position, dimensions, flags
   - Determines sprite type based on size/flags
   - Creates sprite object via one of:
     - `FUN_8001ecc0` - standard sprites
     - `FUN_8001f150` - medium sprites (≤128×128)
     - `FUN_8001f534` - large/special sprites (≤64×64 with flags)
   - Adds to render list via corresponding function:
     - `FUN_80021590`, `FUN_80021778`, or `FUN_80021960`

### Sprite Entry Structure (0x5C = 92 bytes)

```
Offset  Size  Description
------  ----  -----------
0x00    4     Position X (u32)
0x04    4     Position Y (u32)
0x08    4     Width (low 16 bits), Height (low 16 bits at +0x0A)
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
0x26    1     Sprite type (≠3 to process)
0x28-0x2B     Unknown  
0x2C    4     Animation data pointer offset
...
```

## Asset 100/300 (Textures/Tiles)

### Loading Flow

1. **LoadAssetContainer** stores:
   - Asset 100 pointer → `LevelDataContext[1]` (offset 0x04) - geometry header
   - Asset 300 pointer → `LevelDataContext[5]` (offset 0x14) - tile graphics

### Accessor Functions

| Function | Address | Returns |
|----------|---------|---------|
| `GetTileCount` | 0x8007B53C | Sum of u16s at header+0x10, +0x12, +0x14 |
| `GetTileData` | 0x8007B588 | Tile pixels (16×16 or 8×8) from asset 300 |

### Consumer: FUN_80025240 (GPU Texture Upload)

Called from `InitializeAndLoadLevel`, this function:

1. Gets tile count from asset 100 header
2. Allocates GPU texture memory
3. For each tile:
   - Reads 16×16 or 8×8 pixel data from asset 300
   - Calls `LoadImage()` to upload to VRAM
   - Calls `DrawSync(0)` to wait for completion
   - Calls `GetTPage()` to get texture page ID
   - Stores TPage and CLUT info for rendering

### Tile Data Layout

```
If tile index < count_at_0x10:
  - 16×16 tiles, 256 bytes each (0x100)
  - Located at: asset300 + index * 0x100

Else:
  - 8×8 tiles, 64 bytes each (0x80)  
  - Located at: asset300 + count_at_0x10 * 0x80 + index * 0x80
```

## Asset 400/401 (Object Data)

### Loading Flow

1. **LoadAssetContainer** stores:
   - Asset 400 pointer → `LevelDataContext[8]` (offset 0x20) - object container
   - Asset 401 pointer → `LevelDataContext[9]` (offset 0x24) - object config

### Accessor Functions

| Function | Address | Returns |
|----------|---------|---------|
| `GetObjectCount` | 0x8007B4D0 | First byte of asset 400 |
| `GetObjectEntry` | 0x8007B4F8 | Sub-entry from asset 400 TOC |

### Object Container Format

Asset 400 uses the standard sub-TOC format:
```
0x00    u32    Entry count
0x04+   12×N   TOC entries: {flags, size, offset}
```

## Complete Asset Accessor Table

| Function | Address | Ctx Offset | Asset ID | Returns |
|----------|---------|------------|----------|---------|
| - | - | +0x04 [1] | 100 | Geometry header |
| - | - | +0x08 [2] | 101 | Unknown geometry |
| `GetSpriteCount` | 0x8007B6C8 | +0x0C [3] | 200 | Sprite count (u16) |
| `GetSpriteEntry` | 0x8007B700 | +0x10 [4] | 201 | Sprite entry (92 bytes) |
| `GetTileData` | 0x8007B588 | +0x14 [5] | 300 | Tile pixels |
| `GetTilePaletteIndices` | 0x8007B6B0 | +0x18 [6] | 301 | Palette index per tile |
| `GetTileFlags` | 0x8007B6BC | +0x1C [7] | 302 | Flags per tile |
| `GetObjectCount` | 0x8007B4D0 | +0x20 [8] | 400 | Object count |
| `GetObjectEntry` | 0x8007B4F8 | +0x20 [8] | 400 | Object sub-entry |
| - | - | +0x24 [9] | 401 | Object config |

## Asset 301/302 (Tile Metadata)

### Overview

Assets 301 and 302 provide per-tile metadata used during GPU texture upload
by `FUN_80025240`. Each asset contains one byte per tile, indexed the same
as asset 300.

### Asset 301: Palette Index

- **Size**: 1 byte per tile (equals tile count)
- **Purpose**: Specifies which palette from asset 400 to use for each tile
- **Value range**: 0 to (palette_count - 1)

```
For tile[i]:
    palette_to_use = asset400.palettes[asset301[i]]
```

### Asset 302: Tile Flags

- **Size**: 1 byte per tile (equals tile count)
- **Purpose**: Per-tile rendering flags

| Bit | Mask | Description |
|-----|------|-------------|
| 0 | 0x01 | Render attribute (stored at output+6, possibly flip/mirror) |
| 1 | 0x02 | Depth mode: 0=8bpp (256 colors), 1=4bpp (16 colors) |
| 2 | 0x04 | Skip flag: if set, don't upload tile to GPU |

### Usage in GPU Upload (FUN_80025240)

```c
// Get metadata arrays
palette_indices = GetTilePaletteIndices(ctx);  // Asset 301
tile_flags = GetTileFlags(ctx);                 // Asset 302

for each tile i:
    flags = tile_flags[i]
    
    if (flags & 4):
        continue;  // Skip this tile
    
    depth = (flags & 2) ? 4bpp : 8bpp
    palette = palettes[palette_indices[i]]
    
    // Upload tile with correct depth and palette
    LoadImage(...)
    output[i].render_attr = flags & 1
```

### Extraction Example (blb.py)

```python
from scripts.blb import BLBFile, extract_level_tiles_with_metadata

with BLBFile('game.blb') as blb:
    tiles = extract_level_tiles_with_metadata(blb, level_index=5)
    
    for img, meta in tiles:
        depth = "4bpp" if meta.is_4bpp else "8bpp"
        print(f"Tile {meta.tile_index}: palette={meta.palette_index}, {depth}")
        img.save(f"tile_{meta.tile_index:04d}.png")
```

## Complete Loading Flow

```
BLB File (GAME.BLB)
  │
  ├─→ LoadBLBHeader (0x800208B0)
  │     └─→ Reads header to 0x800AE3E0
  │     └─→ InitLevelDataContext (sets callback)
  │
  └─→ InitializeAndLoadLevel (0x8007D1D0)
        │
        ├─→ LevelDataParser (0x8007A62C)
        │     └─→ Reads primary TOC
        │     └─→ Sets primaryDataBuffer [0x1A]
        │
        ├─→ LoadAssetContainer (0x8007B074)
        │     └─→ Calls loaderCallback → CdBLB_ReadSectors
        │     └─→ Parses sub-TOC
        │     └─→ Populates asset pointers [0]-[22]
        │
        ├─→ FUN_8007C088 (Audio loading)
        │     └─→ Uses assets 700 (audio data)
        │     └─→ SpuSetTransferMode, etc.
        │
        ├─→ FUN_80025240 (GPU texture upload)
        │     └─→ Uses assets 100, 300, 400
        │     └─→ LoadImage → VRAM
        │
        └─→ FUN_80024778 (Sprite creation)
              └─→ Uses assets 200, 201
              └─→ Creates sprite objects
              └─→ Adds to render lists
```

## Related Documentation

- [BLB Data Format](../blb-data-format.md) - File format and LevelDataContext structure
- [Runtime Behavior](../runtime-behavior.md) - Game loop and state machine
