# Unconfirmed Findings

This document contains observations and guesses about the Skullmonkeys data structures
that have not been fully verified through decompilation or runtime tracing.

## BLB Asset File Headers

The extracted level assets from GAME.BLB have consistent header structures.

### Primary.bin (Level Data)

**Header format:** `[type=0x03] [0x258] [data_size] [0x28]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    4     0x00000003  Type identifier (always 3 for primary)
0x04    4     0x00000258  Header end offset (600 bytes)
0x08    4     varies      Data size or related offset
0x0C    4     0x00000028  Entry size (40 bytes)
```

- Contains level geometry, tile maps, collision data (guessed)
- Has embedded TIM textures (magic `0x10000000` found at various offsets)
- Size varies: 65KB (menu) to 1.3MB (complex levels)

### Secondary.bin / Secondary_sub_*.bin (Sprite/Object Data)

**Header format:** `[count] [entry_size=0x64] [header_size=0x24] [table_size]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    4     N           Entry count (typically 6-12)
0x04    4     0x00000064  Entry size (100 bytes per entry)
0x08    4     0x00000024  Header size (36 bytes before entry table)
0x0C    4     N*12+4      Computed table size
```

The `table_size` field follows a formula: `count * 12 + 4`

| Count | Expected [3] | Hex    |
|-------|--------------|--------|
| 6     | 76           | 0x4C   |
| 7     | 88           | 0x58   |
| 8     | 100          | 0x64   |
| 9     | 112          | 0x70   |
| 10    | 124          | 0x7C   |
| 11    | 136          | 0x88   |
| 12    | 148          | 0x94   |

- Contains sprite sheets, animations, object definitions (guessed)
- Contains embedded TIM images
- Entry table starts at offset 0x24 (36 bytes)

### Tertiary_sub_*.bin (Additional Assets)

- Same header format as secondary files
- Count ranges from 7-12
- Contains additional sprite data, backgrounds (guessed)
- Size varies widely: 4KB to 1MB

### Loading Screens

**Header format:** `[compressed_size:u16] [decompressed_size:u16=0x3800] [data...]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    2     varies      Compressed data size
0x02    2     0x3800      Decompressed size (always 14,336 bytes)
0x04    4     varies      Additional header data
0x08+   ...   ...         Compressed image data
```

- Uses PSX compression (possibly LZSS or BizStream variant)
- Decompressed size always 14,336 bytes
- Likely 112×128 pixels at 8bpp, or similar dimensions
- **NOT** standard TIM format - custom compressed format

### Embedded Assets

- **TIM images** (magic `0x10000000`) found embedded in primary and secondary files
  - Multiple TIM images per file (20-40 typical)
  - Various bit depths detected (4-bit, 8-bit, 16-bit)
- **No VAG audio** found in level files
  - Audio likely handled separately (XA streaming or separate files)

## Credits Sequence Table

**Location:** BLB header offset 0xF10  
**Entry size:** 12 bytes (0x0C)  
**Max entries:** 2 complete entries before overlapping count fields

> **NOTE:** "Credits" is a GUESSED name based on the "CRD1"/"CRD2" codes found
> in entries and their relationship to the "CRED" sector entry. The actual
> purpose is not fully confirmed. JP versions don't have valid data here.

```
Offset  Size  Type    Field       Description
------  ----  ----    -----       -----------
0x00    4     str     code        4-char ID (empty for entry 0, "CRD1"/"CRD2" for others)
0x04    4     bytes   padding     Unused (zeros) - used to detect valid entries
0x08    2     u16     param_a     Entry 0: level_count; CRD1/2: unknown counter
0x0A    2     u16     param_b     Base sector offset
```

**Validation:** Valid credits entries have zero padding at bytes +4 to +8.
JP versions have non-zero values here, indicating garbage/unused data.

**Sector calculation (verified):**
```
CRED.sector_offset = param_b + level_count
Example: 171 + 26 = 197 = CRED sector entry offset
```

## JP Version Differences

### Sector Table Entry Layout

JP versions (papx-90053, slps-01501) have a different byte order for the 16-byte
sector table entries compared to PAL/NTSC versions:

**PAL/NTSC Layout:**
```
Offset  Size  Field
------  ----  -----
0x00    1     level_index
0x01    1     entry_flags
0x02    1     unknown_byte
0x03    5     code (5 chars, null-terminated)
0x08    4     short_name (4 chars)
0x0C    2     sector_offset (u16 LE)
0x0E    2     sector_count (u16 LE)
```

**JP Layout:**
```
Offset  Size  Field
------  ----  -----
0x00    2     sector_offset (u16 LE)
0x02    2     sector_count (u16 LE)
0x04    1     level_index
0x05    1     entry_flags
0x06    1     unknown_byte
0x07    5     code (5 chars, null-terminated)
0x0C    4     short_name (4 chars)
```

**Detection method:**
- PAL: byte[3] is uppercase A-Z (0x41-0x5A) - start of code field
- JP: byte[3] is 0x00 (low byte of sector_count), byte[7] is uppercase A-Z

### JP Content Differences

| Version | Levels | Movies | Sector Entries | Credits Table |
|---------|--------|--------|----------------|---------------|
| PAL/NTSC | 26 | 13 | 32 | Valid (2 entries) |
| JP Demo (papx-90053) | 6 | 1 | 4 | Invalid (garbage) |
| JP Full (slps-01501) | 6 | 1 | 4 | Invalid (garbage) |

JP versions appear to be a subset of the full game with only 6 playable levels.

---

## Level Metadata Unknown Fields Analysis

Analysis performed on all 26 levels across PAL, NTSC-US, Beta, and JP versions.

### Field 0x08: `entry1_offset_lo` - NOW CONFIRMED

**Previous understanding:** "Low 16 bits of Entry[1].offset in primary TOC"

**Actual meaning:** This is `TOC[6].offset` from the primary data blob (100% match on all 26 levels).

**Verification method:**
```python
# For each level:
toc6_offset = struct.unpack_from('<H', primary_data, 6*4)[0]
assert toc6_offset == level.entry1_offset_lo  # Always true
```

**Recommendation:** Rename field to `toc6_offset` and update documentation.

**Runtime verification:**
```bash
# Run the verification Lua script in PCSX-Redux:
make lua SCRIPT=scripts/verify_toc_fields.lua

# After level loads, check console output for TOC comparison
```

### Field 0x0A: `unknown_0A` - NOW CONFIRMED

**Actual meaning:** This is `TOC[2].count` from the primary data blob (100% match on all 26 levels).

**Verification method:**
```python
# For each level:
toc2_count = struct.unpack_from('<H', primary_data, 2*4+2)[0]
assert toc2_count == level.unknown_0A  # Always true
```

**Cross-version consistency:** Same values between PAL and JP for identical levels (PHRO, SCIE, TMPL, FINN, MEGA).

**Recommendation:** Rename field to `toc2_count` and investigate what TOC[2] points to.

**Runtime verification:**
```bash
# Run the verification Lua script in PCSX-Redux:
make lua SCRIPT=scripts/verify_toc_fields.lua

# The script will:
# 1. Dump all level metadata fields
# 2. After a level loads, compare field 0x0A with TOC[2].count
# 3. Report PASS/FAIL for each verification
```

### Field 0x06: `unknown_06` - PARTIALLY UNDERSTOOD

**Range:** 7-20

**Observations:**
- Does NOT directly match any single TOC entry count
- Consistent across regions (PAL, NTSC-US, Beta have identical values)
- Same values between PAL and JP for identical levels
- Pattern: Bosses tend to have higher values (18-20), simple/special levels have lower (7-12)

**Grouped by value:**
| Value | Levels |
|-------|--------|
| 7 | FINN |
| 9 | RUNN |
| 10 | KLOG |
| 12 | GLEN |
| 13 | FOOD |
| 14 | PHRO, SCIE, TMPL, GLID, WEED, SEVN, CRYS |
| 15 | SNOW, CAVE, BRG1, EGGS, CLOU, SOAR, CSTL, EVIL |
| 16 | MENU, BOIL, MOSS |
| 18 | MEGA, WIZZ |
| 20 | HEAD |

**Hypothesis:** May represent total asset complexity, memory requirements, or a computed value from multiple TOC entries.

### Field 0x04: `unknown_04` - UNKNOWN

**Range:** 776-65,240 (large u16 values)

**Observations:**
- Values differ slightly between PAL and JP for same levels (build-dependent)
- Points to valid data within primary blob (not garbage)
- Does NOT match any TOC entry offset directly
- Data at this offset varies (no consistent structure detected)

**Hypothesis:** Byte offset to specific asset data within primary blob (palette? collision map? geometry start?). Needs runtime tracing to confirm.

### Field 0x0D: `level_flag` - PARTIALLY UNDERSTOOD

**Values:** 0 or 1 only

**Pattern analysis:**
| Flag | Levels |
|------|--------|
| flag=0, tert=1 | FINN, MEGA, HEAD, GLEN, WIZZ, KLOG (all bosses + special) |
| flag=0, tert=2 | RUNN |
| flag=0, tert=3 | PHRO |
| flag=0, tert=4 | SOAR |
| flag=0, tert=5 | EGGS, SEVN, CSTL, EVIL |
| flag=0, tert=6 | SNOW, CLOU, CRYS, MOSS |
| flag=1, tert=4 | TMPL, FOOD, WEED |
| flag=1, tert=5 | SCIE, BRG1, CAVE |
| flag=1, tert=6 | MENU, BOIL, GLID |

**Key observation:** All bosses have `flag=0` and `tert_block_count=1`.

**Hypothesis:** May indicate level type (boss vs regular) or loading behavior. Needs runtime confirmation.

### Groupings by (0x06, 0x0A) Pairs

Levels with identical `(unknown_06, unknown_0A)` pairs often share characteristics:

| Pair | Levels | Notes |
|------|--------|-------|
| (14, 7) | PHRO, SCIE, TMPL | First 3 gameplay worlds |
| (14, 9) | GLID, WEED, SEVN, CRYS | Ynt worlds + secret level |
| (15, 9) | SNOW, CAVE, CLOU, CSTL | Ice/castle themed |
| (15, 8) | BRG1, EGGS | Bridge + Eggs |
| (16, 9) | BOIL, MOSS | Industrial themed |
| (18, 15) | MEGA | Boss: Shriney Guard |
| (20, 16) | HEAD | Boss: Joe-Head-Joe |

This suggests these fields may relate to shared asset characteristics or tileset groupings.

---

## Level Loading System Analysis

> **NOTE:** Most of this information has been verified and moved to 
> [blb-data-format.md](blb-data-format.md). See that document for:
> - Complete LevelDataContext structure (128 bytes, all fields documented)
> - Asset ID mapping table (LoadAssetContainer)
> - Playback sequence modes and string markers
> - Known functions with addresses

### Unverified: Menu Demo Rotation

When at the menu (level index 0), the game cycles through demo modes:

```c
// g_MenuDemoRotationCounter at 0x800A60A4
if (counter == 4) counter = 0;
else if (counter == 0) mode = 1;  // Normal
else if (counter == 2) mode = 6;  // End sequence?
else mode = 5;                     // Attract mode
counter++;
```

**Verification needed:** Confirm counter behavior and mode meanings.

---

*Last updated: January 4, 2026*
