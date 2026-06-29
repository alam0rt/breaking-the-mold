---
title: "Asset Types Reference"
category: blb
tags: [blb, asset, types]
---

# Asset Types Reference

Complete reference for all asset types in the BLB file.

## Asset ID Naming Convention

Asset IDs are decimal numbers stored as hex in the TOC:
- Asset 100 = 0x064
- Asset 600 = 0x258

## Quick Reference Table

| Asset | Hex | Segment | Structure | LevelDataContext | Description |
|-------|-----|---------|-----------|------------------|-------------|
| 100 | 0x064 | Sec/Tert | RAW | ctx[1] +0x04 | Tile header (36 bytes) |
| 101 | 0x065 | Tert | RAW | ctx[2] +0x08 | VRAM slot config (12 bytes) |
| 200 | 0x0C8 | Tert | RAW | ctx[3] +0x0C | Tilemap container header |
| 201 | 0x0C9 | Tert | RAW | ctx[4] +0x10 | Layer entries (92 bytes each) |
| 300 | 0x12C | Sec | RAW | ctx[5] +0x14 | Tile pixel data (8bpp) |
| 301 | 0x12D | Sec | RAW | ctx[6] +0x18 | Palette index per tile |
| 302 | 0x12E | Sec/Tert | RAW | ctx[7] +0x1C | Tile size flags |
| 303 | 0x12F | Sec | RAW | ctx[10] +0x28 | Animated tile lookup |
| 400 | 0x190 | Sec | CONTAINER | ctx[8] +0x20 | Palette container |
| 401 | 0x191 | Sec/Tert | RAW | ctx[9] +0x24 | Palette animation config |
| 500 | 0x1F4 | Tert | RAW | ctx[11] +0x2C | Tile attribute map |
| 501 | 0x1F5 | Tert | RAW | ctx[14] +0x38 | Entity placement data |
| 502 | 0x1F6 | Tert | RAW | ctx[15] +0x3C | VRAM rectangles |
| 503 | 0x1F7 | Tert | RAW | ctx[12] +0x30 | Animation offset table |
| 504 | 0x1F8 | Tert | RAW | ctx[13] +0x34 | Trigger zone data (FINN/RUNN) |
| 600 | 0x258 | Pri/Tert | CONTAINER | ctx[16-17] | Geometry/Sprites |
| 601 | 0x259 | Pri/Sec | CONTAINER | ctx[18-19] | Audio samples |
| 602 | 0x25A | Pri/Sec | RAW | ctx[20] +0x50 | Palette/Audio metadata |
| 700 | 0x2BC | Tert | RAW | ctx[21-22] | Demo replay data |

---

## Secondary Segment Assets (Tiles)

### Asset 100 - Tile Header (36 bytes)

Contains tile counts, level dimensions, and spawn position.

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    3     u8[3]  Background RGB color
0x03    1     u8     Padding
0x04    3     u8[3]  Secondary RGB color
0x07    1     u8     Padding
0x08    2     u16    Level width (tiles)
0x0A    2     u16    Level height (tiles)
0x0C    2     u16    Spawn X (tiles)
0x0E    2     u16    Spawn Y (tiles)
0x10    2     u16    16×16 tile count
0x12    2     u16    8×8 tile count
0x14    2     u16    Additional tile count
0x16    2     u16    Vehicle waypoint count
0x18    2     u16    Level flags bitfield
0x1A    2     u16    Special level ID (99=FINN/SEVN)
0x1C    2     u16    VRAM rect count (matches Asset 502)
0x1E    2     u16    Entity count (matches Asset 501)
0x20    2     u16    World index (values 0-6) ✅ VESTIGIAL - Written to g_pPlayerState[4], never read
0x22    2     u16    Padding

**Field 0x20 (World Index)**: ✅ **CONFIRMED VESTIGIAL** - Accumulated to g_pPlayerState[4] but never read. Originally tracked world/area number but replaced by other systems. Safe to ignore or set to 0.
```

**Accessor**: `GetTotalTileCount` @ 0x8007b53c

### Asset 300 - Tile Pixel Data

8-bit indexed pixels, 16-byte row stride.

**Layout**:
1. 16×16 tiles first: 256 bytes each (16 rows × 16 bytes)
2. 8×8 tiles after: 128 bytes each (8 rows × 16 bytes)

```python
if tile_index < count_16x16:
    offset = tile_index * 256
else:
    offset = count_16x16 * 256 + (tile_index - count_16x16) * 128
```

**Note**: 8×8 tiles are scaled 2× during rendering to fill 16×16 grid cells.

### Asset 301 - Palette Assignment

One byte per tile, indexing into Asset 400 palettes.

- Size: `count_16x16 + count_8x8` bytes
- Value: Palette index (0 to N-1)

### Asset 302 - Tile Size Flags

One byte per tile controlling rendering behavior.

| Bit | Mask | Meaning |
|-----|------|---------|
| 0 | 0x01 | Semi-transparency (enables GPU alpha) |
| 1 | 0x02 | Tile size (0=16×16, 1=8×8) |
| 2 | 0x04 | Skip flag (don't upload/render) |

### Asset 400 - Palette Container

Sub-TOC format with 256-color palettes.

```
0x00    u32     Palette count
0x04+   12×N    Sub-entries: {index, size(512), offset}

Each palette: 256 × u16 PSX 15-bit RGB
  Color 0 = transparent
  Bits 0-4: Red (×8 for 8-bit)
  Bits 5-9: Green
  Bits 10-14: Blue
  Bit 15: STP (semi-transparency)
```

### Asset 401 - Palette Animation Config

4 bytes per palette for color cycling animation.

```
0x00    u8    enabled (0=static, 1=animate)
0x01    u8    start_index (first color)
0x02    u8    end_index (last color)
0x03    u8    speed (animation rate)
```

---

## Tertiary Segment Assets (Sprites, Entities, Audio)

### Asset 200 - Tilemap Container Header

```
0x00    u16    Layer count
0x02+   var    Additional header data
```

**Accessor**: `GetLayerCount` @ 0x8007b6c8

### Asset 201 - Layer Entries (92 bytes each)

See [Tiles and Tilemaps](../systems/tiles-and-tilemaps.md) for full structure.

Key fields:
- 0x00-0x0B: Position, dimensions
- 0x0C: Render priority (low 16 bits = z-order)
- 0x10-0x17: Parallax scroll factors
- 0x26: Layer type (3 = skip)
- 0x2C: Color tints (48 bytes)

### Asset 500 - Tile Attribute Map

Per-tile collision and trigger data.

```
0x00    u16              offset_x ✅ FUNCTIONAL (usually 0) - Collision map X offset
0x02    u16              offset_y ✅ FUNCTIONAL (usually 0) - Collision map Y offset
0x04    u16              Level width (tiles)
0x06    u16              Level height (tiles)
0x08    width×height     Tile data (1 byte per tile)
```

**Header Fields 0x00-0x03**: Copied to GameState+0x6C but no runtime consumer found. Likely unused offset values from development.

**Tile Values**: 0=passable, 2=solid, 0x2A=death, 0x3D-0x41=wind, 0x51-0x7A=spawn zones. See [Collision System](../systems/tile-collision-complete.md) for complete reference.

### Asset 501 - Entity Placement Data (24 bytes each)

See [Entities](../systems/entities.md) for full structure.

```
0x00    u16    x1 (bbox left)
0x02    u16    y1 (bbox top)
0x04    u16    x2 (bbox right)
0x06    u16    y2 (bbox bottom)
0x08    u16    x_center
0x0A    u16    y_center
0x0C    u16    variant
0x0E    4      padding
0x12    u16    entity_type    (BLB type — NOT the runtime callback index)
0x14    u16    layer+flags    (low byte = layer 1/2/3, high byte = render flags)
0x16    u16    padding
```

**Entry count** = `Asset 501 size / 24` = `Asset 100 field_0x1E`.

> ⚠️ **CORRECTNESS-CRITICAL: entity_type is LAYER-DEPENDENT.**
> The `entity_type` field at +0x12 is the **BLB type**, which is remapped to an
> **internal type** at runtime by `RemapEntityTypesForLevel @ 0x8008150c` using the
> layer (low byte of +0x14). The same BLB type on different layers spawns different entities:
>
> | BLB Type | Layer 1 → internal | Layer 2 → internal |
> |----------|--------------------|--------------------|
> | 25 (0x19) | 0 (Pickup) | 79 (Spawner) |
> | 27 (0x1B) | 4 (Pickup) | 97 (Grid/Helper) |
>
> Any tool/decomp that maps BLB type → behavior **must apply this remapping per-entity**.
> Full tables: [`reference/ENTITY_REMAPPING_CORRECTION.md`](../reference/ENTITY_REMAPPING_CORRECTION.md).

### Asset 502 - VRAM Rectangles (16 bytes each)

Texture page boundaries and trigger zones.

```
0x00    u16    x1
0x02    u16    y1
0x04    u16    x2
0x06    u16    y2
0x08    u16    rect_type (2, 4, or 21)
0x0A    6      padding
```

Count stored in Asset 100 offset 0x1C.

### Asset 503 - Animation Offset Table

ToolX animation sequence data.

```
0x00    u32    Animation count
0x04+   12×N   TOC: {index, size, offset}
...     var    Frame data sections
```

### Asset 504 - Trigger Zone Data (FINN/RUNN levels only)

Extended trigger zones for special level modes. Used by `CheckTriggerZoneCollision` to detect when player enters specific areas and trigger events (checkpoints, spawns, pickups, etc.).

**Storage**: Pointer at GameState+0x74, count at GameState+0x78 (from Asset 100 field 0x16).

**Entry structure** (16 bytes minimum, accessed in 8-short increments):
```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    2     s16     x1 (bbox left)
0x02    2     s16     y1 (bbox top)
0x04    2     s16     x2 (bbox right)
0x06    2     s16     y2 (bbox bottom)
0x08    4     u32     trigger_type (0x00, 0x51, 0x52, 0x65, 0x66, 0x79, 0x7a, etc.)
0x0C    4     var     trigger_data (type-specific payload)
```

**Extended entries** (64 bytes observed in extracted data):
The JSON extraction shows additional fields beyond the 16-byte core:
- Center position (x, y)
- Flags (often 0x8000)
- Extended parameters for spawn offsets, state transitions

**Trigger types** (from `FinnCheckTriggerZones`):
| Type | Purpose |
|------|---------|
| 0x00 | State transition |
| 0x51 | Spawn group 1, offset 1 |
| 0x52 | Spawn group 2, offset 1 |
| 0x65 | Spawn group 1, offset 0 |
| 0x66 | Spawn group 2, offset 0 |
| 0x79 | Spawn group 1, offset 2 |
| 0x7a | Spawn group 2, offset 2 |

**Levels**:
- **FINN**: 78 entries (maze arena with tile-based wall collision + trigger zones for checkpoints/events)
- **RUNN**: 1 entry (auto-runner with simple trigger)

---

## Primary/Tertiary Segment Assets

### Asset 600 - Geometry/Sprites (CONTAINER)

**In Primary**: Level geometry, background graphics
**In Tertiary**: RLE-encoded character sprites

See [Sprites](../systems/sprites.md) for sprite format details.

Sub-TOC format:
```
0x00    u32    Sprite count
0x04+   12×N   Entries: {sprite_id, size, offset}
```

### Asset 601 - Audio Sample Bank (CONTAINER)

SPU ADPCM audio samples.

```
0x00    u16    Sample count
0x02    u16    Reserved
0x04    12×N   Entries: {sample_id, spu_size, offset}
...     var    ADPCM data
```

See [Audio](../systems/audio.md) for details.

### Asset 602 - Audio Metadata

**In Primary/Secondary**: Volume/pan table (4 bytes per sample)

```
0x00    u16    Volume (0-0x3FFF)
0x02    u16    Pan (0=center)
```

### Asset 700 - Demo Replay Data ✅ RESOLVED (2026-01-19)

Appears in 9 of 26 levels: MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED.  
**17 levels don't have demos** - Only levels with Asset 700 can be used in attract mode.

```
0x00    u32    Entry count (always 1)
0x04    u32    Entry ID (varies per level)
0x08    u32    Data size (bytes)
0x0C    u32    Data offset (always 16)
0x10+   4×N    Replay entries (buttons u16, duration u16)
```

**Status**: ✅ **RESOLVED** - This is the demo/attract mode input replay data, NOT audio. The "SPU-like" bytes (0x80, 0xC0) are PSX button bitmasks. Accessed via `GetDemoDataPtr` @ 0x8007BAC8 which returns `ctx[0x54] + 0x10`.

**See**: [Demo/Attract Mode System](../systems/demo-attract-mode.md) for complete documentation.

---

## Cross-Asset Relationships

Verified size relationships:

| Relationship | Formula |
|--------------|---------|
| Asset 302 size | = Asset 100 total_tiles |
| Asset 401 size | = Asset 400 palette_count × 4 |
| Asset 602 size | = Asset 601 sample_count × 4 |
| Asset 501 entries | = Asset 100 field_1e |
| Asset 502 entries | = Asset 100 field_1c |
| Asset 504 entries | = Asset 100 field_16 |

## Related Documentation

- [Tiles and Tilemaps](../systems/tiles-and-tilemaps.md) - Tile rendering
- [Sprites](../systems/sprites.md) - Sprite format
- [Entities](../systems/entities.md) - Entity system
- [Audio](../systems/audio.md) - Audio system
