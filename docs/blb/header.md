# BLB Header Format

The BLB header occupies the first 0x1000 bytes (2 CD sectors) of GAME.BLB.

**RAM Location**: 0x800AE3E0 (PAL)

## Header Layout

```
Offset   Size    Description
------   ----    -----------
0x000    0xB60   Level Metadata Table (26 entries × 0x70 bytes)
0xB60    0x168   Movie Table (13 entries × 0x1C bytes)
0xCC8    0x004   Padding (4 zeros)
0xCD0    0x200   Sector Table (32 entries × 0x10 bytes)
0xECC    0x044   Mode 6 Sector Table (17 entries × 4 bytes)
0xF10    0x021   Credits Sequence Table (2 entries × 0x0C bytes)
0xF31    0x001   Level Count (u8, value=26)
0xF32    0x001   Movie Count (u8, value=13)
0xF33    0x001   Sector Table Entry Count (u8)
0xF34    0x0CC   Playback Sequence Data
```

## Movie Table (0xB60-0xCC7)

13 FMV movie entries, 0x1C (28) bytes each:

```
Offset   Size       Description
------   ----       -----------
0x00     u16        Reserved (always 0)
0x02     u16        Sector count
0x04     char[5]    Movie ID (4-char null-terminated)
0x09     char[3]    Short name (2-char)
0x0C     char[16]   ISO path (e.g., "\\MVLOGO.STR;1")
```

### Movie List

| # | ID | Sectors | Path | Description |
|--:|:---|-------:|:-----|:------------|
| 0 | DREA | 79 | \MVDWI.STR | Dreamworks intro |
| 1 | LOGO | 105 | \MVLOGO.STR | Logo |
| 2 | ELEC | 60 | \MVEA.STR | EA logo |
| 3 | INT1 | 3091 | \MVINTRO1.STR | Intro part 1 |
| 4 | INT2 | 156 | \MVINTRO2.STR | Intro part 2 |
| 5 | GASS | 1545 | \MVGAS.STR | Gas cutscene |
| 6 | YAMM | 1776 | \MVYAM.STR | Yam cutscene |
| 7 | REDD | 2119 | \MVRED.STR | Red cutscene |
| 8 | YNTS | 463 | \MVYNT.STR | YNT world intro |
| 9 | EYES | 918 | \MVEYE.STR | Eye cutscene |
| 10 | EVIL | 1008 | \MVEVIL.STR | Evil Engine intro |
| 11 | END1 | 1044 | \MVEND.STR | Ending part 1 |
| 12 | END2 | 793 | \MVWIN.STR | Ending part 2 |

Note: Movies are external .STR files, not embedded in GAME.BLB.

## Sector Table (0xCD0-0xECF)

Loading screen and special sector entries, 0x10 (16) bytes each.

```
Offset   Size       Description
------   ----       -----------
0x00     u8         Level index (0-25 when entry_flags=0x00)
0x01     u8         Entry flags (0x00=level, 0x03=game over, 0x05=special)
0x02     u8         Display timeout (seconds)
0x03     char[5]    Code (4-char null-terminated)
0x08     char[4]    Short name
0x0C     u16        Sector offset in BLB
0x0E     u16        Sector count
```

### Entry Flags

| Flags | Meaning |
|-------|---------|
| 0x00 | Level loading screen (level_index = 0-25) |
| 0x03 | Game over screen (display_timeout=99) |
| 0x05 | Special loading screen (intro, legal) |

### Display Timeout

- **0x00**: No display (skip immediately)
- **0x0A (10)**: 10 seconds, any button skips
- **0x63 (99)**: 99 seconds + game over button handling (X=retry, Start=menu)

### Loading Screen MDEC Format

Loading screens are **BS v2** (Bitstream version 2) MDEC frames:

```
BS Frame Header (8 bytes):
  0x00  u16   Frame size in 16-bit words
  0x02  u16   Magic (0x3800)
  0x04  u16   Quantization scale (1-63)
  0x06  u16   Version (0x0002)
  0x08+ var   VLC-encoded DCT data
```

Decoding: `DecDCTvlcBuild()` → `DecDCTvlc2()` → `DecDCTin()` → `DecDCTout()`

Display: 320×256 pixels, 15-bit RGB, double-buffered (Y=0/256)

## Mode 6 Sector Table (0xECC-0xF0F)

17 entries for special playback sequences, 4 bytes each:

```
Offset   Size   Description
------   ----   -----------
0x00     u16    Sector offset
0x02     u16    Sector count
```

**Note**: Entry[0] overlaps with `sectors[31].sector_offset/count`.

Access formula: `header + (level_index * 4) + 0xECC`

Used when playback mode = 6 (inter-level transitions, world intros).

## Playback Sequence Data (0xF34-0xFFF)

Controls level loading state machine:

```
0xF36+   u8[]   Mode values (0-6) - indexed by arrayPosition
0xF92+   u8[]   Level indices - indexed by arrayPosition
```

### Mode Values

| Mode | Meaning |
|------|---------|
| 3 | Normal level mode |
| 6 | Special mode (uses Mode 6 sector table) |
| 0-2, 4-5 | Unknown |

### Sliding Window Formula

```c
arrayPosition = headerOffset - 0x0A;
levelIndex = header[0xF92 + arrayPosition];
levelEntry = header + (levelIndex * 0x70);
```

## JP Version Differences (SLPS-01501)

| Feature | PAL | JP |
|---------|-----|-----|
| Sector table offset | 0xCD0 | 0xCB0 (-32 bytes) |
| Movie count | 13 | 12 |
| Credits index | 2 | 28 |

JP spliced intro movies into one file and moved CRED index to fix a credits trigger bug.

## Related Documentation

- [Level Metadata](level-metadata.md) - 0x70-byte level entries
- [BLB Overview](README.md) - File structure overview
