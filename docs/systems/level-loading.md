# Level Loading System

The game uses a state machine to load levels with three phases: header, level, and stage loading.

## Loading Phases

### Phase 1: Header Loading (Boot)

**Function**: `LoadBLBHeader` @ 0x800208b0

```c
void LoadBLBHeader(GameState* state) {
    // Read first 2 sectors (0x1000 bytes)
    CdBLB_ReadSectors(0, 2, state->headerBuffer);
    
    // Initialize LevelDataContext at GameState+0x84
    InitLevelDataContext(state + 0x84, headerBuffer, loaderCallback);
    
    // Set sprite table globals
    SetSpriteTables(0, state + 0x84);
}
```

The header is read **once** and stays resident for the game session.

### Phase 2: Level Loading (On Selection)

**Function**: `InitializeAndLoadLevel` @ 0x8007d1d0

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

**Function**: `LoadAssetContainer` @ 0x8007b074

Parses segment TOC and populates LevelDataContext:

```c
void LoadAssetContainer(LevelDataContext* ctx, int stageIndex, char containerType) {
    // containerType: '\x01' = secondary, '\x00' = tertiary
    
    // Read sector data
    levelEntry = header + (levelIndex * 0x70) + (stageIndex * 2);
    
    if (containerType == '\x00') {  // Tertiary
        sectorOffset = *(u16*)(levelEntry + 0x38);
        sectorCount = *(u16*)(levelEntry + 0x46);
    } else {  // Secondary
        sectorOffset = *(u16*)(levelEntry + 0x1C);
        sectorCount = *(u16*)(levelEntry + 0x2A);
    }
    
    // Load and parse TOC
    CdBLB_ReadSectors(sectorOffset, sectorCount, buffer);
    
    // Populate ctx pointers based on asset type IDs
    for (entry in TOC) {
        switch (entry.type) {
            case 0x064: ctx[1] = entry.data; break;  // Asset 100
            case 0x258: ctx[16] = entry.data; break; // Asset 600
            // ... etc
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

### Mode Values

| Mode | Meaning | Data Source |
|------|---------|-------------|
| 3 | Normal level | Level metadata |
| 6 | Special mode | Mode 6 sector table |
| 0-2, 4-5 | Unknown | |

### LevelDataContext State

| Offset | Field | Description |
|--------|-------|-------------|
| +0x5C | blbHeaderBuffer | Header pointer (0x800AE3E0) |
| +0x60 | slidingWindowIndex | Current state index |
| +0x64 | loaderCallback | CD read function |
| +0x68 | primaryDataBuffer | Primary segment |
| +0x6C | secondaryDataBuffer | Secondary segment |

## Sector Calculation

```c
// Mode 3 (normal level)
levelEntry = header + (levelIndex * 0x70);
sectorOffset = *(u16*)(levelEntry + 0x00);
sectorCount = *(u16*)(levelEntry + 0x02);

// Mode 6 (special)
addr = header + (levelIndex * 4) + 0xECC;
sectorOffset = *(u16*)(addr);
sectorCount = *(u16*)(addr + 2);
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

## Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `LoadBLBHeader` | 0x800208b0 | Load header at boot |
| `InitializeAndLoadLevel` | 0x8007d1d0 | Full level load |
| `LevelDataParser` | 0x8007a62c | Parse primary segment |
| `LoadAssetContainer` | 0x8007b074 | Parse secondary/tertiary |
| `CdBLB_ReadSectors` | 0x80038ba0 | Low-level CD read |
| `InitLevelDataContext` | 0x8007a1bc | Init context struct |

## Related Documentation

- [Game Loop](game-loop.md) - Main loop and player creation
- [BLB Header](../blb/header.md) - Header structure
- [Level Metadata](../blb/level-metadata.md) - Per-level entries
- [LevelDataContext](../reference/level-data-context.md) - Context structure
- [Asset Types](../blb/asset-types.md) - All asset types
