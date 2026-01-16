# Level Loading System

**Status: VERIFIED via Ghidra analysis (2026-01-16)**

The game uses a state machine to load levels with three phases: header, level, and stage loading.

## Loading Phases

### Phase 1: Header Loading (Boot)

**Function**: `LoadBLBHeader` @ 0x800208b0 (VERIFIED)

```c
// From Ghidra decompilation
void LoadBLBHeader(int state) {
    // Initialize state structure
    g_GameStatePtr = state;
    
    // Get header buffer from blbHeaderBufferBase+0xa650
    int headerBuffer = *(int *)(blbHeaderBufferBase + 0xa650);
    *(int *)(state + 0x40) = headerBuffer;
    *(int *)(state + 0x3c) = headerBuffer + 0x1000;
    
    // Read first 2 sectors (0x1000 bytes = 4KB)
    CdBLB_ReadSectors(0, 2, headerBuffer);
    
    // Initialize LevelDataContext at GameState+0x84
    InitLevelDataContext(state + 0x84, headerBuffer, &LAB_80020848);
    
    // Set sprite table globals
    SetSpriteTables(0, state + 0x84);
}
```

The header is read **once** and stays resident for the game session.

### Phase 2: Level Loading (On Selection)

**Function**: `InitializeAndLoadLevel` @ 0x8007d1d0 (VERIFIED)

Loading sequence:
1. Reset state buffers
2. Advance playback sequence
3. Display loading screen (`DisplayLoadingScreen`)
4. Parse level data (`LevelDataParser`)
5. Upload audio to SPU (`UploadAudioToSPU`)
6. Load secondary container (`LoadAssetContainer`)
7. Calculate buffer sizes
8. Load tiles to VRAM (`LoadTileDataToVRAM`)
9. Load tertiary container
10. Initialize player spawn (`InitPlayerSpawnPosition`)
11. Initialize layers (`InitLayersAndTileState`)

### Phase 3: Asset Container Loading

**Function**: `LoadAssetContainer` @ 0x8007b074 (VERIFIED)

Parses segment TOC (12-byte entries: {count, type, size, offset}) and populates LevelDataContext:

```c
// From Ghidra decompilation (simplified)
void LoadAssetContainer(int *pLevelDataCtx, int subBlockIndex, char containerType) {
    // containerType: '\x01' = secondary, '\x00' = tertiary
    
    // Get current level from playback sequence
    int headerPtr = pLevelDataCtx[0x17];  // +0x5C
    int stateOffset = *(byte *)(pLevelDataCtx + 0x18);  // +0x60
    int levelIndex = *(byte *)(headerPtr + stateOffset + 0xF92);
    
    // Calculate level entry
    int levelEntry = headerPtr + (levelIndex * 0x70) + (subBlockIndex * 2);
    
    if (containerType == '\x00') {  // Tertiary
        sectorOffset = *(u16 *)(levelEntry + 0x38);
        sectorCount = *(u16 *)(levelEntry + 0x46);
    } else {  // Secondary
        sectorOffset = *(u16 *)(levelEntry + 0x1C);
        sectorCount = *(u16 *)(levelEntry + 0x2A);
    }
    
    // Load via callback and parse TOC
    loaderCallback(sectorOffset, sectorCount, buffer);
    
    // Populate ctx pointers based on asset type IDs
    for (entry in TOC) {
        switch (entry.type) {
            case 0x064: ctx[1] = data; break;  // Asset 100: TileHeader
            case 0x065: ctx[2] = data; break;  // Asset 101: VRAMSlotConfig
            case 0x0C8: ctx[3] = data; break;  // Asset 200: TilemapContainer
            case 0x0C9: ctx[4] = data; break;  // Asset 201: LayerEntries
            case 0x12C: ctx[5] = data; break;  // Asset 300: TilePixels
            case 0x12D: ctx[6] = data; break;  // Asset 301: PaletteIndices
            case 0x12E: ctx[7] = data; break;  // Asset 302: TileFlags
            case 0x12F: ctx[10] = data; break; // Asset 303: AnimatedTiles
            case 0x190: ctx[8] = data; break;  // Asset 400: PaletteContainer
            case 0x191: ctx[9] = data; break;  // Asset 401: PaletteAnim
            case 0x1F4: ctx[11] = data; break; // Asset 500: TileAttributes
            case 0x1F5: ctx[14] = data; break; // Asset 501: Entities
            case 0x1F6: ctx[15] = data; break; // Asset 502: VRAMRects
            case 0x1F7: ctx[12] = data; break; // Asset 503: AnimOffsets
            case 0x1F8: ctx[13] = data; break; // Asset 504: VehicleData
            case 0x258: ctx[16-17] = data+size; break; // Asset 600: Geometry
            case 0x259: ctx[18-19] = data+size; break; // Asset 601: Audio
            case 0x25A: ctx[20] = data; break; // Asset 602: Palette
            case 0x2BC: ctx[21-22] = data+size; break; // Asset 700: SPUAudio
        }
    }
}
```

## State Machine

The game tracks load state with a sliding window in the header:

| Address | Field | Description |
|---------|-------|-------------|
| header+0xF36 | mode[] | Mode values (0-6) |
| header+0xF92 | index[] | Level indices |

### Sliding Window Formula

```c
arrayPosition = headerOffset - 0x0A;
levelIndex = header[0xF92 + arrayPosition];
mode = header[0xF36 + arrayPosition];
levelEntry = header + (levelIndex * 0x70);
```

### Mode Values (VERIFIED)

| Mode | Meaning | Data Source |
|------|---------|-------------|
| 1 | Movie playback | Movie table |
| 2 | Credits sequence | Credits entries |
| 3 | Normal level | Level metadata (0x70 bytes) |
| 4/5 | Special loading | Special sector loading |
| 6 | Special mode | Mode 6 sector table (0xECC) |

### LevelDataContext State (VERIFIED)

| Offset | Field | Description |
|--------|-------|-------------|
| +0x5C | blbHeaderBuffer | Header pointer (0x800AE3E0) |
| +0x60 | slidingWindowIndex | Current state index (init 0xFF) |
| +0x64 | loaderCallback | CD read function (0x80020848) |
| +0x68 | primaryDataBuffer | Primary TOC buffer |
| +0x6C | secondaryDataBuffer | Secondary TOC buffer |
| +0x70 | primaryLevel600 | Primary Asset 600 pointer |
| +0x74 | primaryAudio601 | Primary Asset 601 audio |
| +0x78 | primaryAudio601Size | Primary Asset 601 size |
| +0x7C | primaryAudioMeta602 | Primary Asset 602 metadata |

## Sector Calculation (VERIFIED)

```c
// Mode 3 (normal level) - from LevelDataParser
levelEntry = header + (levelIndex * 0x70);
sectorOffset = *(u16 *)(levelEntry + 0x00);
sectorCount = *(u16 *)(levelEntry + 0x02);

// Mode 6 (special) - from LevelDataParser
addr = header + (levelIndex * 4) + 0xECC;
sectorOffset = *(u16 *)(addr + 0x00);
sectorCount = *(u16 *)(addr + 0x02);  // Actually at 0xECE
```

## Loader Callback Chain

```
LoadAssetContainer / LevelDataParser
    └─► loaderCallback(sectorOffset, sectorCount, buffer)
            │ (function pointer at ctx+0x64)
            └─► 0x80020848 (thin wrapper)
                    └─► CdBLB_ReadSectors(g_GameBLBSector + offset, count, buffer)
                            └─► PSY-Q: CdIntToPos → CdControl → CdRead
```

## Menu Idle Demo Behavior

When idle at menu, the game auto-loads TMPL (Monkey Shrines) as demo:

```
MENU (headerOffset=0x0A)
    ↓ (idle timeout)
Demo: TMPL (headerOffset=0x12)
    ↓ (demo ends or button)
MENU (headerOffset=0x0A)
```

## Complete Loading Flow

```
BLB File
  │
  ├─► LoadBLBHeader (0x800208B0)
  │     └─► Read header → InitLevelDataContext
  │
  └─► InitializeAndLoadLevel (0x8007D1D0)
        │
        ├─► LevelDataParser (0x8007A62C)
        │     └─► Primary segment → ctx+0x68 to +0x7C
        │
        ├─► LoadAssetContainer [Secondary]
        │     └─► Assets 100-401 → ctx[1]-ctx[9]
        │
        ├─► LoadAssetContainer [Tertiary]
        │     └─► Assets 500-700 → ctx[11]-ctx[22]
        │
        ├─► UploadAudioToSPU (0x8007C088)
        │
        ├─► LoadTileDataToVRAM (0x80025240)
        │
        └─► InitLayersAndTileState (0x80024778)
```

## Key Functions (VERIFIED)

| Function | Address | Purpose |
|----------|---------|--------|
| `LoadBLBHeader` | 0x800208b0 | Load header at boot |
| `InitializeAndLoadLevel` | 0x8007d1d0 | Full level load |
| `LevelDataParser` | 0x8007a62c | Parse primary segment |
| `LoadAssetContainer` | 0x8007b074 | Parse secondary/tertiary |
| `CdBLB_ReadSectors` | 0x80038ba0 | Low-level CD read |
| `InitLevelDataContext` | 0x8007a1bc | Init context struct |
| `SetSpriteTables` | (called) | Set sprite globals |

## Related Documentation

- [Game Loop](game-loop.md) - Main loop and player creation
- [BLB Header](../blb/header.md) - Header structure
- [Level Metadata](../blb/level-metadata.md) - Per-level entries
- [LevelDataContext](../reference/level-data-context.md) - Context structure
- [Asset Types](../blb/asset-types.md) - All asset types
