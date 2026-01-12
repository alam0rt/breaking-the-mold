# Level Metadata Entry Format

Each of the 26 levels has a metadata entry at header offset `index × 0x70`.

**Entry size**: 0x70 (112) bytes

## Structure Overview

```
Offset   Size   Description
------   ----   -----------
# Primary data (0x00-0x0D)
0x00     u16    Primary sector offset
0x02     u16    Primary sector count
0x04     u32    Primary buffer size
0x08     u32    Entry[1] offset (to Asset 601 in primary TOC)
0x0C     u8     Level asset index (0-25)
0x0D     u8     Password-selectable flag

# Stage configuration (0x0E-0x1D)
0x0E     u16    Stage count (1-6)
0x10     u16[6] Tertiary data offsets (×32 = size for stage i)
0x1C     u16    Padding

# Secondary sectors (0x1E-0x39)
0x1E     u16[6] Secondary sector offsets (per-stage)
0x2A     u16    Padding
0x2C     u16[6] Secondary sector counts (per-stage)
0x38     u16    Padding

# Tertiary sectors (0x3A-0x55)
0x3A     u16[6] Tertiary sector offsets (per-stage)
0x46     u16    Padding
0x48     u16[6] Tertiary sector counts (per-stage)
0x54     u16    Padding

# Level identification (0x56-0x6F)
0x56     char[5]   Level ID (4-char + null, e.g., "MENU")
0x5B     char[21]  Level name (null-terminated)
```

## Field Details

### Primary Buffer Size (0x04) - VERIFIED

Returned by `GetPrimaryBufferSize()` @ 0x8007a5cc.

Used for memory allocation:
```c
bufferSize = GetPrimaryBufferSize(ctx);
tertiarySize = GetCurrentTertiaryDataSize(ctx);
remaining = bufferSize - ALIGN16(tertiarySize);
```

- For mode 6: Returns fixed 0x7d000 (512KB)
- Range: ~510KB to ~1.3MB depending on level complexity

### Entry[1] Offset (0x08) - VERIFIED

Offset to Asset 601 (collision/audio) within primary TOC.

Used to calculate: `ctx[0x1b] = primary_buffer + entry1_offset`

### Password-Selectable Flag (0x0D) - VERIFIED

| Value | Meaning |
|-------|---------|
| 1 | Level can be selected via password system |
| 0 | Not directly selectable |

Levels with flag=1: SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED (8 levels)

Read by `GetLevelFlagByIndex` @ 0x8007aa28.

### Stage Count (0x0E) - VERIFIED

Number of stages in this level (1-6). Used to index the sector arrays.

### Tertiary Data Offsets (0x10-0x1B)

Six u16 values. Shifted left by 5 to get tertiary data size:
```c
size = tert_data_off[stage_index] << 5;
```

### Sector Arrays

Parallel arrays for each stage (index 0-5):

| Stage | Secondary | Tertiary |
|-------|-----------|----------|
| Stage 0 | sec_off[0], sec_cnt[0] | tert_off[0], tert_cnt[0] |
| Stage 1 | sec_off[1], sec_cnt[1] | tert_off[1], tert_cnt[1] |
| ... | ... | ... |
| Stage 5 | sec_off[5], sec_cnt[5] | tert_off[5], tert_cnt[5] |

## Example: MENU Level (Index 0)

```
Level ID:    "MENU"
Level Name:  "Options Menu"
Stage Count: 6

Primary:
  Sector: 0x0069-0x00E8 (128 sectors)
  Buffer Size: 61,472 bytes

Stages:
  Stage 0: sec[233, 67 sectors], tert[300, 488 sectors]
  Stage 1: sec[788, 62 sectors], tert[850, 3 sectors]
  ...
```

## Example: SCIE Level (Index 2)

```
Level ID:    "SCIE"
Level Name:  "Science Centre"
Stage Count: 5

Primary:
  Sector: 0x0F2F (3887)
  Count:  0x10A7 (4263 sectors)
  
Primary TOC (3 entries):
  Entry 0: type=0x258, size=524,212 bytes (geometry)
  Entry 1: type=0x259, size=126,256 bytes (audio)
  Entry 2: type=0x25A, size=148 bytes (palette)
```

## Stage Counts by Level

| Level | ID | Name | Stages |
|-------|-----|------|--------|
| 0 | MENU | Options | 6 |
| 1-3 | PHRO/SCIE/TMPL | World 1 | 3-5 |
| 4 | FINN | Swimming | 1 |
| 5 | MEGA | Boss | 1 |
| 6-8 | BOIL/SNOW/FOOD | World 2 | 4-6 |
| 9 | HEAD | Boss | 1 |
| 10-14 | BRG1-EGGS | World 3-4 | 4-6 |
| 15 | GLEN | Boss | 1 |
| 16-20 | CLOU-CSTL | World 5-6 | 5-6 |
| 21 | WIZZ | Boss | 1 |
| 22 | RUNN | Runner | 2 |
| 23 | MOSS | World 7 | 6 |
| 24 | KLOG | Final Boss | 1 |
| 25 | EVIL | Evil Engine | 5 |

## Code Reference

| Function | Address | Purpose |
|----------|---------|---------|
| `GetPrimaryBufferSize` | 0x8007a5cc | Returns buffer size field |
| `GetLevelFlagByIndex` | 0x8007aa28 | Returns password-selectable flag |
| `LevelDataParser` | 0x8007a62c | Parses level metadata |

## Related Documentation

- [Header Format](header.md) - Full header layout
- [Asset Types](asset-types.md) - Asset IDs and structures
- [Level Loading](../systems/level-loading.md) - Loading state machine
