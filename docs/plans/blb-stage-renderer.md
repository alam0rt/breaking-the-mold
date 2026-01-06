# BLB World→Stage HTML Renderer

## Overview

A single static HTML file that opens Skullmonkeys BLB files and renders stages as close to the original game engine as possible, featuring a Windows 2000-style UI with a tree navigator on the left and a canvas renderer on the right.

## Goals

1. **Open BLB files** - Load GAME.BLB or extracted level BLBs via file picker
2. **Navigate levels/stages/layers** - Tree view showing World → Stage → Layer hierarchy
3. **Render stages accurately** - Display background tileset, layers with parallax, and entity markers
4. **Clean Windows 2000 aesthetic** - Classic 3D beveled borders, system fonts, gray backgrounds

## Architecture

### UI Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│ File: [Open BLB...] │ Camera X: [====●====] │ Zoom: [1x ▼]          │
├──────────────────────┬──────────────────────────────────────────────┤
│ 📁 GAME.BLB          │                                              │
│ ├─ 📂 PHRO (1-1)     │                                              │
│ │  ├─ Stage 0        │         [Canvas Render Area]                 │
│ │  │  ├─ Layer 0     │                                              │
│ │  │  ├─ Layer 1     │                                              │
│ │  │  └─ Layer 2     │                                              │
│ │  └─ Stage 1        │                                              │
│ ├─ 📂 HERA (1-2)     │                                              │
│ └─ ...               │                                              │
└──────────────────────┴──────────────────────────────────────────────┘
```

### File Structure

Single `blb_viewer.html` containing:
- Inline CSS for Windows 2000 styling
- JavaScript BLB parser (ported from ImHex template)
- Canvas-based tile/layer renderer
- Tree view component

## Implementation Steps

### Step 1: Document Implementation Plan ✓

This document.

### Step 2: Create HTML Structure with Windows 2000 UI

Build a `<div>`-based layout with classic Windows 2000 styling:
- 3D beveled borders using `border-style: outset/inset`
- System fonts: `Tahoma, "MS Sans Serif", sans-serif`
- Gray backgrounds (`#c0c0c0`, `#d4d0c8`)
- Button styling with raised/pressed states

Components:
- File picker button and status bar
- Split pane: tree view (left, ~250px), canvas (right, flex)
- Camera X slider for parallax preview
- Zoom dropdown

### Step 3: Implement BLB Header Parsing

Port header parsing from `scripts/blb.hexpat` to JavaScript using `DataView` with little-endian reads.

**Level Entry (112 bytes at offset 0x00, 26 entries):**
```javascript
// Offsets from level entry start
const LEVEL_ENTRY = {
  sectorOffset: 0x00,      // u16 - Primary sector offset
  sectorCount: 0x02,       // u16 - Primary sector count
  bufferSize: 0x04,        // u32 - Primary buffer size
  entry1Offset: 0x08,      // u32 - Collision data offset
  levelIndex: 0x0C,        // u8  - Level asset index
  selectable: 0x0D,        // u8  - Password-selectable flag
  stageCount: 0x0E,        // u16 - Number of stages (1-6)
  tertDataOff: 0x10,       // u16[6] - Tertiary data offsets
  secSectorOff: 0x1E,      // u16[6] - Secondary sector offsets
  secSectorCnt: 0x2C,      // u16[6] - Secondary sector counts
  tertSectorOff: 0x3A,     // u16[6] - Tertiary sector offsets
  tertSectorCnt: 0x48,     // u16[6] - Tertiary sector counts
  levelId: 0x56,           // char[5] - Level ID (e.g., "PHRO")
  levelName: 0x5B,         // char[21] - Level display name
};
```

### Step 4: Build Tree View Navigation

Populate left panel from parsed level metadata:

```
World (level_id + name)
└─ Stage N (0 to stageCount-1)
   └─ Layer M (from Asset 201 in tertiary segment)
```

Each node is clickable to trigger rendering:
- Clicking World loads base secondary tiles
- Clicking Stage loads tertiary segment and renders all layers
- Clicking Layer highlights/solos that layer

### Step 5: Implement Tile System Parsing

Parse secondary segment TOC to extract tile data:

| Asset | ID | Description |
|-------|-----|-------------|
| 100 | 0x064 | Tile Header (36 bytes) - dimensions, tile counts |
| 300 | 0x12C | Tile Pixels (8bpp indexed, 16×16 or 8×8) |
| 301 | 0x12D | Palette Indices (1 byte per tile) |
| 302 | 0x12E | Tile Flags (bit 0=semi-transparent, bit 1=8×8, bit 2=skip) |
| 400 | 0x190 | Palette Container (sub-TOC with 256-color CLUTs) |

**PSX Color Conversion (15-bit to RGBA):**
```javascript
function psxColorToRGBA(color16) {
  if (color16 === 0) return [0, 0, 0, 0]; // Transparent
  const r = (color16 & 0x1F) * 8;
  const g = ((color16 >> 5) & 0x1F) * 8;
  const b = ((color16 >> 10) & 0x1F) * 8;
  return [r, g, b, 255];
}
```

**Tile Layout:**
- 16×16 tiles: 256 bytes each (16 rows × 16 bytes)
- 8×8 tiles: 128 bytes each (8 rows × 16-byte stride)
- Layout: All 16×16 tiles first, then 8×8 tiles

### Step 6: Implement Layer Rendering with Parallax

Parse tertiary segment for layer data:

| Asset | ID | Description |
|-------|-----|-------------|
| 200 | 0x0C8 | Tilemap Container (sub-TOC, one entry per layer) |
| 201 | 0x0C9 | Layer Entries (92 bytes each) |

**Layer Entry Structure (92 bytes):**
```javascript
const LAYER_ENTRY = {
  xOffset: 0x00,        // u16 - X position in tiles
  yOffset: 0x02,        // u16 - Y position in tiles
  width: 0x04,          // u16 - Layer width in tiles
  height: 0x06,         // u16 - Layer height in tiles
  levelWidth: 0x08,     // u16 - Level width in tiles
  levelHeight: 0x0A,    // u16 - Level height in tiles
  renderParam: 0x0C,    // u32 - Render parameter
  scrollX: 0x10,        // u32 - X parallax (16.16 fixed-point)
  scrollY: 0x14,        // u32 - Y parallax (16.16 fixed-point)
  // ... more fields ...
  layerType: 0x26,      // u8 - Type (0=normal, 3=skip)
  skipRender: 0x28,     // u16 - Skip render flag
  bgR: 0x2C,            // u8 - Background red
  bgG: 0x2D,            // u8 - Background green
  bgB: 0x2E,            // u8 - Background blue
};
```

**Tilemap Format:**
- Array of u16 values, one per tile position
- Bits 0-10: Tile index (1-based, 0=transparent)
- Bits 11-15: Unused (no flip flags)

**Parallax Calculation:**
```javascript
function getParallaxOffset(cameraPos, scrollFactor) {
  // scrollFactor is 16.16 fixed-point
  // 0x10000 = 1.0 (moves with camera)
  // 0x8000 = 0.5 (half-speed parallax)
  const factor = scrollFactor / 65536.0;
  return cameraPos * (1.0 - factor);
}
```

**Rendering Order:**
1. Clear canvas to background color from Asset 100 or Layer 0
2. Render layers back-to-front (layer 0 first)
3. Apply parallax offset per layer based on camera position

### Step 7: Add Entity Marker Visualization

Detect entity markers in tilemaps:

```javascript
function isEntityMarker(tileValue, totalTileCount) {
  const tileIdx = tileValue & 0x7FF;
  return tileIdx > totalTileCount;
}
```

Render entity markers as:
- Colored rectangles at tile positions
- Optional: Show entity ID as text overlay

## Critical Implementation Details

### Stage/Secondary Pairing

**This is critical for correct rendering!**

Each stage's tertiary segment uses the secondary that PRECEDES it in sector layout:

```
Sector order:  BASE_SEC → TERT[0] → SEC_SUB[0] → TERT[1] → SEC_SUB[1] → ...

Stage 0 tertiary → uses BASE secondary
Stage 1 tertiary → uses Stage 0 secondary (SEC_SUB[0])
Stage 2 tertiary → uses Stage 1 secondary (SEC_SUB[1])
Stage N tertiary → uses Stage N-1 secondary
```

### TOC Parsing

All segments begin with a TOC structure:
```javascript
function parseTOC(data, offset = 0) {
  const view = new DataView(data.buffer, data.byteOffset + offset);
  const count = view.getUint32(0, true);
  const entries = [];
  for (let i = 0; i < count; i++) {
    entries.push({
      assetType: view.getUint32(4 + i * 12, true),
      size: view.getUint32(4 + i * 12 + 4, true),
      offset: view.getUint32(4 + i * 12 + 8, true),
    });
  }
  return entries;
}
```

### Performance Considerations

- BLB files are ~72MB - load entire file into `ArrayBuffer`
- Pre-render tiles to `ImageData` objects for fast canvas blitting
- Cache parsed segments to avoid re-parsing on navigation
- Use `requestAnimationFrame` for smooth parallax preview

## Future Enhancements (v2+)

1. **Sprite Rendering** - Decode RLE sprites from Asset 600
2. **Animated Tiles** - Support Asset 303 animated tile lookup
3. **Palette Animation** - Implement Asset 401 color cycling
4. **Collision Visualization** - Overlay collision data from Asset 601
5. **Export Options** - Save rendered layers/stages as PNG

## Source of Truth

- **ImHex Template**: `scripts/blb.hexpat` - Canonical file format documentation
- **Python Library**: `scripts/blb.py` - Reference parsing implementation
- **Extraction Script**: `scripts/extract_all_graphics.py` - Reference rendering logic
- **Format Documentation**: `docs/blb-data-format.md` - Additional format details
