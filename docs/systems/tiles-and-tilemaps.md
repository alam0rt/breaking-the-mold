# Tiles and Tilemaps

**Status: VERIFIED via Ghidra analysis (2026-01-16)**

The tile system renders level backgrounds using indexed tile graphics and layer-based tilemaps.

## Tile Data (Secondary Segment)

### Asset 100 - Tile Header (36 bytes)

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    3     u8[3]  Background RGB color
0x08    2     u16    Level width (tiles)
0x0A    2     u16    Level height (tiles)
0x0C    2     u16    Spawn X (tiles)
0x0E    2     u16    Spawn Y (tiles)
0x10    2     u16    16×16 tile count
0x12    2     u16    8×8 tile count
0x14    2     u16    Additional tile count
```

**Total tiles** = `count_16x16 + count_8x8 + count_extra`

Accessor: `GetTotalTileCount` @ 0x8007b53c

### Asset 300 - Tile Pixel Data

8-bit indexed pixels.

**Storage Layout**:
1. **16×16 tiles**: 256 bytes each (16 rows × 16 bytes)
2. **8×8 tiles**: 128 bytes each (8 rows × 16 bytes)
   - Only first 8 columns contain pixel data
   - Remaining 8 bytes per row are padding/unused

```python
def get_tile_offset(tile_index, count_16x16):
    """Get byte offset for tile. tile_index is 0-based."""
    if tile_index < count_16x16:
        return tile_index * 256
    else:
        adjusted = tile_index - count_16x16
        return count_16x16 * 256 + adjusted * 128
```

**VRAM Upload** (verified via `CopyTilePixelData` @ 0x8007b588):
- Tile indices are **1-based** (0 = transparent, no data)
- 8×8 tiles: copied as 8 bytes/row for 16 rows into VRAM buffer
- The code reads 128 bytes with 8-byte stride, treating storage as interleaved

**Rendering Note**: 8×8 tiles are scaled 2× to 16×16 during GPU rendering.

Verified in `RenderTilemapSprites16x16` @ 0x8001713c:
- Always uses `SetSprt16()` for 16×16 sprite primitives
- Position advances by 16 pixels per tile

### Asset 301 - Palette Assignment

One byte per tile, indexing into Asset 400 palettes.

Size: `count_16x16 + count_8x8` bytes

### Asset 302 - Tile Flags

One byte per tile:

| Bit | Mask | Meaning |
|-----|------|---------|
| 0 | 0x01 | Semi-transparency (GPU alpha blend) |
| 1 | 0x02 | Tile size (0=16×16, 1=8×8) |
| 2 | 0x04 | Skip flag (don't render) |

### Asset 400 - Palette Container

Sub-TOC with 256-color palettes (512 bytes each).

Format: PSX 15-bit RGB (u16 per color)
- Color 0 = transparent
- Bits 0-4: Red, 5-9: Green, 10-14: Blue
- Bit 15: STP (semi-transparency processing)

---

## Tilemap Data (Tertiary Segment)

### Asset 200 - Tilemap Container

```
0x00    u16    Layer count
0x02+   var    Header data
```

Contains sub-TOC with offsets to each layer's tilemap.

Accessor: `GetLayerCount` @ 0x8007b6c8

### Asset 201 - Layer Entries (92 bytes each)

```
Offset  Size  Type    Field           Description
------  ----  ------  --------------- -----------
0x00    2     u16     x_offset        Layer X position (tiles)
0x02    2     u16     y_offset        Layer Y position (tiles)
0x04    2     u16     width           Layer width (tiles)
0x06    2     u16     height          Layer height (tiles)
0x08    2     u16     level_width     From Asset 100
0x0A    2     u16     level_height    From Asset 100
0x0C    4     u32     render_param    Priority in low 16 bits
0x10    4     u32     scroll_x        Parallax (0x10000 = 1.0)
0x14    4     u32     scroll_y        Parallax factor Y
0x26    1     u8      layer_type      0=normal, 3=skip
0x28    2     u16     skip_render     !=0 means skip
0x2C    48    u8[48]  color_tints     16 RGB entries
```

Accessor: `GetLayerEntry` @ 0x8007b700 - returns `ctx[4] + index * 0x5C`

### Tilemap Entry Format (u16)

```
Bits 0-11:  Tile index (12 bits, 0xFFF mask)
  - 0 = transparent/empty
  - 1+ = tile index (1-based)
  - Values > tile_count = entity spawn markers

Bits 12-15: Color tint selector (4 bits)
  - Indexes into layer's color_tints[16] table
  - Entry 0 = white (no tinting)
```

Verified via `InitTilemapLayer16x16` @ 0x80017540:
```c
color_table_base = layer + 0x2C;
tint_index = (tilemap_entry >> 12) & 0xF;
rgb = color_table_base + tint_index * 3;
```

---

## Rendering System

### Layer Initialization (InitLayersAndTileState @ 0x80024778)

1. Iterate all layers from 0 to `GetLayerCount()`
2. For each layer:
   - Skip if `layer_type == 3` OR `skip_render != 0`
   - Create render context based on dimensions:
     - ≤64×64: Small render list
     - ≤128×128: Medium render list
     - Otherwise: Standard render list
   - Priority = `(short)(render_param & 0xFFFF)`

### Tile Rendering

`RenderTilemapSprites16x16` @ 0x8001713c:

```c
for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
        tile_entry = tilemap[y * width + x];
        tile_idx = tile_entry & 0xFFF;
        color_idx = (tile_entry >> 12) & 0xF;
        
        if (tile_idx == 0) continue;  // Transparent
        
        // Set SPRT_16 primitive
        SetSprt16(sprite);
        sprite->x = layer_x + x * 16;
        sprite->y = layer_y + y * 16;
        
        // Apply color tint
        rgb = color_tints + color_idx * 3;
        SetRGB0(sprite, rgb[0], rgb[1], rgb[2]);
        
        // Get tile texture coordinates
        // ...
    }
}
```

### VRAM Upload (LoadTileDataToVRAM @ 0x80025240)

```c
for (i = 0; i < total_tiles; i++) {
    flags = tile_flags[i];
    
    if (flags & 0x04) continue;  // Skip flag
    
    palette = palettes[palette_indices[i]];
    
    if (i < count_16x16) {
        pixels = tile_pixels + i * 256;
        rect = {x, y, 16, 16};
    } else {
        pixels = tile_pixels + count_16x16*256 + (i-count_16x16)*128;
        rect = {x, y, 8, 8};
    }
    
    LoadImage(&rect, pixels);
    DrawSync(0);
}
```

---

## Priority System

Layer priority from `render_param` determines z-order:

| Priority Range | Content |
|----------------|---------|
| 150-800 | Background/parallax |
| 900-1100 | Main gameplay layers |
| 1200-1500 | Foreground layers |

Lower values render behind higher values.

See [Rendering Order](rendering-order.md) for complete priority documentation.

---

## Key Functions

| Function | Address | Purpose |
|----------|---------|--------|
| `GetTotalTileCount` | 0x8007b53c | Sum tile counts |
| `CopyTilePixelData` | 0x8007b588 | Get tile pixels |
| `GetPaletteIndices` | 0x8007b6b0 | Get palette per tile |
| `GetPaletteGroupCount` | 0x8007b4d0 | Palette count |
| `GetPaletteDataPtr` | 0x8007b4f8 | Get palette data |
| `GetPaletteAnimData` | 0x8007b530 | Palette animation |
| `GetTileSizeFlags` | 0x8007b6bc | Get tile flags |
| `GetLayerCount` | 0x8007b6c8 | Layer count |
| `GetLayerEntry` | 0x8007b700 | Get 92-byte entry |
| `GetTilemapDataPtr` | 0x8007b6dc | Tilemap sub-TOC |
| `LoadTileDataToVRAM` | 0x80025240 | Upload tiles |
| `InitLayersAndTileState` | 0x80024778 | Init layers |
| `InitTilemapLayer16x16` | 0x80017540 | Init layer with sprites |
| `RenderTilemapSprites16x16` | 0x8001713c | Render tiles |

---

## Related Documentation

- [Asset Types](../blb/asset-types.md) - Assets 100, 200, 201, 300-302, 400-401
- [Rendering Order](rendering-order.md) - Layer priority system
- [LevelDataContext](../reference/level-data-context.md) - Asset pointer storage
