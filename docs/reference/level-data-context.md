# LevelDataContext Structure

Central data structure for level loading, located at GameState+0x84.

**PAL Address**: 0x8009DCC4  
**Size**: 128 bytes (0x80)

## Complete Structure

```c
struct LevelDataContext {
    // Asset Pointers (populated by LoadAssetContainer)
    /* 0x00 */ int      subBlockFlag;         // Set to stageIndex or 1
    /* 0x04 */ void*    tileHeader;           // Asset 100 (36 bytes)
    /* 0x08 */ void*    vramSlotConfig;       // Asset 101 (12 bytes: bank_a_count, bank_b_count, reserved)
    /* 0x0C */ void*    tilemapContainer;     // Asset 200
    /* 0x10 */ void*    layerEntries;         // Asset 201 (92 bytes each)
    /* 0x14 */ void*    tilePixels;           // Asset 300 (8bpp)
    /* 0x18 */ void*    paletteIndices;       // Asset 301
    /* 0x1C */ void*    tileSizeFlags;        // Asset 302
    /* 0x20 */ void*    paletteContainer;     // Asset 400
    /* 0x24 */ void*    paletteAnimData;      // Asset 401
    /* 0x28 */ void*    animatedTileData;     // Asset 303
    /* 0x2C */ void*    tileAttributes;       // Asset 500
    /* 0x30 */ void*    animOffsets;          // Asset 503
    /* 0x34 */ void*    vehicleData;          // Asset 504 (FINN/RUNN)
    /* 0x38 */ void*    entityData;           // Asset 501 (24-byte structs)
    /* 0x3C */ void*    vramRects;            // Asset 502
    /* 0x40 */ void*    levelGeometry;        // Asset 600
    /* 0x44 */ u32      levelGeometrySize;    // Asset 600 size
    /* 0x48 */ void*    audioSamples;         // Asset 601
    /* 0x4C */ u32      audioSamplesSize;     // Asset 601 size
    /* 0x50 */ void*    paletteData;          // Asset 602
    /* 0x54 */ void*    spuAudioData;         // Asset 700
    /* 0x58 */ u32      spuAudioDataSize;     // Asset 700 size
    
    // Context State
    /* 0x5C */ void*    blbHeaderBuffer;      // → 0x800AE3E0
    /* 0x60 */ u8       slidingWindowIndex;   // Playback index
    /* 0x61 */ u8[3]    padding61;
    /* 0x64 */ void*    loaderCallback;       // → 0x80020848
    /* 0x68 */ void*    primaryDataBuffer;    // Primary TOC buffer
    /* 0x6C */ void*    secondaryDataBuffer;  // Secondary buffer
    
    // Primary TOC Assets
    /* 0x70 */ void*    primaryLevel600;      // Primary Asset 600
    /* 0x74 */ void*    primaryAudio601;      // Primary Asset 601
    /* 0x78 */ u32      primaryAudio601Size;  // Size
    /* 0x7C */ void*    primaryAudioMeta602;  // Primary Asset 602
};
```

## Word Index Reference

For code using `ctx[N]` notation:

| Word | Offset | Asset | Description |
|------|--------|-------|-------------|
| [0] | 0x00 | - | Sub-block flag |
| [1] | 0x04 | 100 | Tile header |
| [2] | 0x08 | 101 | Unknown (optional) |
| [3] | 0x0C | 200 | Tilemap container |
| [4] | 0x10 | 201 | Layer entries |
| [5] | 0x14 | 300 | Tile pixels |
| [6] | 0x18 | 301 | Palette indices |
| [7] | 0x1C | 302 | Tile flags |
| [8] | 0x20 | 400 | Palette container |
| [9] | 0x24 | 401 | Palette animation |
| [10] | 0x28 | 303 | Animated tiles |
| [11] | 0x2C | 500 | Tile attributes |
| [12] | 0x30 | 503 | Animation offsets |
| [13] | 0x34 | 504 | Vehicle data |
| [14] | 0x38 | 501 | Entity data |
| [15] | 0x3C | 502 | VRAM rects |
| [16] | 0x40 | 600 | Geometry/sprites |
| [17] | 0x44 | - | Geometry size |
| [18] | 0x48 | 601 | Audio samples |
| [19] | 0x4C | - | Audio size |
| [20] | 0x50 | 602 | Palette data |
| [21] | 0x54 | 700 | SPU audio |
| [22] | 0x58 | - | SPU audio size |
| [23] | 0x5C | - | Header buffer |
| [24] | 0x60 | - | Playback index |
| [25] | 0x64 | - | Loader callback |
| [26] | 0x68 | - | Primary buffer |
| [27] | 0x6C | - | Secondary buffer |
| [28] | 0x70 | 600 | Primary geometry |
| [29] | 0x74 | 601 | Primary audio |
| [30] | 0x78 | - | Primary audio size |
| [31] | 0x7C | 602 | Primary audio meta |

## Initialization

`InitLevelDataContext` @ 0x8007A1BC:
- Sets `blbHeaderBuffer` [0x17]
- Sets `loaderCallback` [0x19]
- Sets `slidingWindowIndex` to 0xFF [0x18]

## Population Flow

1. **LevelDataParser** @ 0x8007A62C:
   - Clears all fields
   - Parses primary TOC
   - Populates [0x1A-0x1F] from primary segment

2. **LoadAssetContainer** @ 0x8007B074:
   - Parses secondary/tertiary TOC
   - Populates [0x00-0x16] based on asset IDs

## Runtime Example (SCIE Level)

```
ctx base:      0x8009DCC4
header:        0x800AE3E0
headerOffset:  0x0E (14)
loadCallback:  0x80020848
tocPtr:        0x800AF3E0 (3 entries)
asset258:      0x800AF408 (524,212 bytes geometry)
asset259:      0x8012F3BC (126,256 bytes audio)
asset25A:      0x8014E0EC (148 bytes palette)
```

## Key Accessor Functions

| Function | Address | Returns |
|----------|---------|---------|
| `GetLayerCount` | 0x8007B6C8 | u16 from ctx[3] |
| `GetLayerEntry` | 0x8007B700 | ctx[4] + index*92 |
| `GetTotalTileCount` | 0x8007B53C | Sum from ctx[1] |
| `GetTileSizeFlags` | 0x8007B6BC | ctx[7] |
| `GetPaletteIndices` | 0x8007B6B0 | ctx[6] |
| `GetPaletteDataPtr` | 0x8007B4F8 | From ctx[8] sub-TOC |
| `GetEntityCount` | 0x8007B7A8 | From ctx[1]+0x1E |
| `GetAsset601Ptr` | 0x8007BA78 | ctx[18] or ctx[29] |
| `GetAsset602Ptr` | 0x8007BAA0 | ctx[20] or ctx[31] |

## Related Documentation

- [Asset Types](../blb/asset-types.md) - All asset IDs
- [Level Loading](../systems/level-loading.md) - How context is populated
- [Game Functions](game-functions.md) - Accessor function list
