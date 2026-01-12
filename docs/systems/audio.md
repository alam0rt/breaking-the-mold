# Audio System

The game uses the PSX SPU (Sound Processing Unit) with ADPCM-encoded samples.

## Audio Asset Distribution

| Segment | Asset | Contents |
|---------|-------|----------|
| Secondary | 601 (0x259) | Audio sample bank with TOC |
| Secondary | 602 (0x25A) | Volume/pan table (per-sample) |
| Primary | 601 (0x259) | Alternative audio source |
| Primary | 602 (0x25A) | Alternative volume/pan |
| Tertiary | 700 (0x2BC) | Additional SPU samples |

## Asset 601 - Audio Sample Bank

```
Offset  Size   Description
------  ----   -----------
0x00    u16    Sample count
0x02    u16    Reserved (always 0)
0x04    12×N   Sample entries
...     var    ADPCM audio data
```

### Sample Entry (12 bytes)

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    4     u32    Sample ID (hash identifier)
0x04    4     u32    SPU size (bytes in SPU RAM)
0x08    4     u32    Data offset (within audio block)
```

### Example (SCIE Stage 0)

13 samples:
| Index | Size | SPU Offset |
|------:|-----:|-----------:|
| 0 | 4,432 | 0xA0 |
| 1 | 3,296 | 0x11F0 |
| 2 | 5,296 | 0x1ED0 |
| ... | ... | ... |
| 12 | 6,160 | 0xD1A0 |

## Asset 602 - Volume/Pan Table

4 bytes per sample:

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    u16   Volume (0-0x3FFF, where 0x3FFF = max)
0x02    u16   Pan (0 = center, non-zero = offset)
```

### Common Values

| Volume | Hex | Meaning |
|--------|-----|---------|
| 16383 | 0x3FFF | Maximum |
| 8192 | 0x2000 | 50% |
| 4915 | 0x1333 | ~30% |

If Asset 602 is NULL, defaults are used: volume=0x3FFF, pan=0.

## Asset 700 - Additional Audio

Appears in 9 of 26 levels: MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED.

⚠️ **Note**: Asset 700 may not be standard ADPCM. Observed data has invalid filter values (filter=15, valid is 0-4).

Possible uses:
- Music track selection
- Level-specific audio configuration
- Unused/legacy data

## Audio Loading Flow

From `InitializeAndLoadLevel` @ 0x8007d1d0:

```c
// After loading secondary segment
audioSamples = GetAsset601Ptr(ctx);    // Secondary Asset 601
volumePanTable = GetAsset602Ptr(ctx);  // Secondary Asset 602
audioSize = GetAsset601Size(ctx);
UploadAudioToSPU(audioSamples, volumePanTable, audioSize);
```

## SPU Upload Process

1. SPU transfer starts at base address 0x1010 (after system reserved)
2. Each upload block tracked in table at `DAT_8009cfa8` (12-byte entries)
3. Sample table at `DAT_8009cc60` maps sample IDs to SPU addresses

## Primary vs Secondary Audio

`GetAsset601Ptr` checks `ctx[1]` to select source:

| Condition | Source | Offsets |
|-----------|--------|---------|
| Primary mode | ctx+0x74 | 0x74, 0x78, 0x7C |
| Secondary mode | ctx+0x48 | 0x48, 0x4C, 0x50 |

## LevelDataContext Audio Offsets

| Offset | Field | Description |
|--------|-------|-------------|
| +0x48 | ctx[18] | Asset 601 ptr (secondary) |
| +0x4C | ctx[19] | Asset 601 size |
| +0x50 | ctx[20] | Asset 602 ptr |
| +0x54 | ctx[21] | Asset 700 ptr |
| +0x58 | ctx[22] | Asset 700 size |
| +0x74 | ctx[29] | Asset 601 ptr (primary) |
| +0x78 | ctx[30] | Asset 601 size (primary) |
| +0x7C | ctx[31] | Asset 602 ptr (primary) |

## Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `UploadAudioToSPU` | 0x8007c088 | Upload samples to SPU RAM |
| `GetAsset601Ptr` | 0x8007ba78 | Get audio sample pointer |
| `GetAsset601Size` | 0x8007ba50 | Get audio bank size |
| `GetAsset602Ptr` | 0x8007baa0 | Get volume/pan table |

## Cross-Asset Relationship

**Verified**: `Asset602.size = Asset601.sample_count × 4`

This holds for all 91 secondary segments with audio data.

## PSX SPU API

The game uses PSY-Q SPU functions:

```c
SpuSetTransferMode(SpuTransByDMA);
SpuSetTransferStartAddr(0x1010 + offset);
SpuWrite(data, size);
SpuIsTransferCompleted(SpuTransferWait);
```

## Related Documentation

- [Asset Types](../blb/asset-types.md) - Asset 601, 602, 700 details
- [Level Loading](level-loading.md) - When audio is uploaded
- [LevelDataContext](../reference/level-data-context.md) - Audio pointer storage
