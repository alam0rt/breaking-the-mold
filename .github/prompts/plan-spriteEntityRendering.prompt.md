# Plan: Implement Sprite/Entity Rendering with RLE Decompression

**TL;DR**: Add sprite rendering to the BLB viewer by parsing Asset 600 containers (Tertiary segment), implementing RLE decompression, and rendering entity sprites at their tilemap positions. Start with first frame only, then expand to animations.

## Steps

### 1. Add sprite parsing functions in `tools/blb_viewer.html`
- `parseSpriteContainer()` to extract TOC with sprite IDs, sizes, offsets
- `parseSpriteHeader()` for 12-byte header (animation_count, frame_meta_offset, rle_offset, palette_offset)
- `parseAnimationEntry()` for 12-byte animation entries
- `parseFrameMetadata()` for 36-byte frame metadata

### 2. Implement RLE decompression with `decodeRLE()` function
- Parse command count (u16) and command array
- Process bitfield: bit 15 = new_line, bits 14-8 = skip_count, bits 7-0 = copy_count
- Decode to width×height pixel array (8bpp indices)

### 3. Add embedded palette parsing to `parsePalette256()` 
- 256 × u16 PSX BGR5551 colors (512 bytes)
- Color 0 = transparent

### 4. Integrate sprite loading into `loadStageData()`
- Parse Tertiary Asset 600 (SPRITES) container
- Build sprite ID → sprite data lookup map
- Cache parsed sprites with decoded first frame

### 5. Render entities in `renderStage()`
- For each entity marker (tileIdx > totalTiles), look up sprite by ID
- Draw sprite frame at entity position using `anchor_x`, `anchor_y` offsets
- Use sprite's embedded palette for coloring

## Further Considerations

1. **Entity ID mapping**: The tilemap stores entity IDs (tileIdx > totalTiles), but we need to map these to sprite IDs in Asset 600 - is there a lookup table, or are they direct matches?

2. **Which Asset 600 to use?** Tertiary contains character sprites, but Primary Asset 600 has level geometry sprites - should we parse both and merge?

3. **Frame selection for multi-frame sprites**: Initially render frame 0, but should we add a UI control to cycle animation frames?

---

## Reference: Sprite Format Details

### Sprite Container Structure (Asset 600 / 0x258)

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    4     u32     Sprite count (N)
0x04+   N×12  array   Sprite TOC entries
```

#### Sprite TOC Entry (12 bytes each)
```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    4     u32     Sprite ID (hash-like lookup key)
0x04    4     u32     Data size in bytes
0x08    4     u32     Data offset from container start
```

### Sprite Header Structure (12 bytes)

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    2     u16     animation_count
0x02    2     u16     frame_meta_offset (from sprite start)
0x04    4     u32     rle_offset (from sprite start)
0x08    4     u32     palette_offset (from sprite start)
```

### Animation Entry Format (12 bytes each)

Located at `spriteStart + 12`:

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    4     u32     animation_id
0x04    2     u16     frame_count
0x06    2     u16     frame_offset (index into frame metadata table)
0x08    2     u16     flags
0x0A    2     u16     extra
```

### Frame Metadata Format (36 bytes = 0x24 per frame)

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    2     u16     reserved_00 (always 0)
0x02    2     u16     reserved_02 (always 0)
0x04    2     u16     flags (1 or 2)
0x06    2     s16     render_x (SIGNED)
0x08    2     s16     render_y (SIGNED)
0x0A    2     u16     render_width
0x0C    2     u16     render_height
0x0E    2     u16     unknown_0e
0x10    2     u16     reserved_10 (always 0)
0x12    2     s16     anchor_x (SIGNED)
0x14    2     s16     anchor_y (SIGNED)
0x16    2     u16     clip_width
0x18    2     u16     clip_height
0x1A    6     bytes   padding (always 0)
0x20    4     u32     rle_offset (from sprite's RLE base!)
```

### RLE Compression Format

#### RLE Data Header
```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    2     u16     command_count
0x02+   N×2   u16[]   RLE commands
var+    var   u8[]    Pixel data (8bpp palette indices)
```

#### RLE Command Format (u16 bitfield)
```
Bit 15:     new_line flag (1 = advance to next row, reset X to 0)
Bits 14-8:  skip_count (0-127 transparent pixels to skip)
Bits 7-0:   copy_count (0-255 literal pixels to copy)
```

### Key Offset Formulas

```javascript
const animationStart = spriteStart + 12;
const frameMetaStart = spriteStart + header.frame_meta_offset;
const rleStart = spriteStart + header.rle_offset;
const paletteStart = spriteStart + header.palette_offset;

// For a specific frame:
const frameRLEAbsolute = spriteStart + header.rle_offset + frame.rle_offset;
```
