# BLB Format Unknown Fields Analysis

This document summarizes the investigation into unknown/undocumented BLB format fields.

## Investigation Date: 2026-01-13

---

## 1. Asset 101 (0x65) - VRAM/Texture Page Configuration

### Structure
```c
struct Asset101 {
    u32 field_0;  // Values 1-4, "texture bank A count"
    u32 field_1;  // Values 0-1, "texture bank B flag"  
    u32 field_2;  // Always 0 (padding)
};
```

### Discovery

Asset 101 is read by `GetAsset101Entry()` @ 0x8007b3fc:
```c
// Returns field at index 0 or 1 from Asset 101 data
u32 GetAsset101Entry(LevelDataContext* ctx, u32 index) {
    if (ctx[2] == 0 || index > 1) return 0;
    return *(u32*)(ctx[2] + index * 4);
}
```

It's used in `InitializeAndLoadLevel()` @ 0x8007d1d0:
```c
u32 field0 = GetAsset101Entry(ctx, 0);
u32 field1 = GetAsset101Entry(ctx, 1);
if (field0 == 0 && field1 == 0) {
    field1 = 2;  // Default to 2 if both are 0
}
FUN_80013b1c(blbHeaderBufferBase, field0 & 0xff, field1 & 0xff);
```

### Purpose: VRAM Slot Allocation

`FUN_80013b1c()` @ 0x80013b1c configures VRAM texture page slots:
- Calculates `(field0 + field1) * 0x20` for one buffer size
- Calculates `(field0 + field1) * 0x10` for another
- Iterates `field0` times for "bank A" entries (y starts at 0xF0)
- Iterates `field1` times for "bank B" entries (y starts at 0x1F0)
- Each iteration creates 3 entries (total: (field0 + field1) * 3 slots)
- Entries stored at blbHeaderBufferBase + 0xa2a0 (8 bytes each)

This controls how many VRAM texture page slots are allocated for the level's sprites/tiles.

### Values Observed

| Level | Stages with Asset 101 | Values [field0, field1, field2] |
|-------|----------------------|----------------------------------|
| SCIE (2) | secondary1, stage1 | [2, 0, 0] |
| FINN (4) | secondary, stage0 | [2, 0, 0] |
| MEGA (5) | secondary, stage0 | [4, 0, 0] |
| BOIL (6) | 0,1,2,5 | [4,0,0], [4,0,0], [2,0,0], [4,0,0] |
| SNOW (7) | stage5 | [2, 0, 0] |
| FOOD (8) | 0, 1 | [2, 1, 0] - Both have field1=1! |
| HEAD (9) | secondary, stage0 | [1, 1, 0] - Both fields set! |
| BRG1 (10) | secondary2, stage2 | [2, 0, 0] |
| GLID (11) | secondary, stage0 | [2, 0, 0] |
| WEED (13) | 0, 1, 2 | [2, 0, 0] all |
| EGGS (14) | stage4 | [4, 0, 0] |
| CLOU (16) | All 6 stages | [2, 0, 0] all |
| MOSS (23) | stage5 | [2, 0, 0] |
| EVIL (25) | 1-4 | [3,0,0], [2,0,0], [2,0,0], [2,0,0] |

### Notable Patterns
- FOOD and HEAD are the only levels with `field1 = 1`
- FOOD has player transformation mechanics
- HEAD is the Joe-Head-Joe boss fight
- Higher field0 values (4) appear in boss levels (MEGA) and complex stages (BOIL, EGGS)

### Suggested Name
- Rename from "unknown101" to "vram_slot_config" or "texture_bank_counts"
- field_0: `bank_a_count`
- field_1: `bank_b_count`

---

## 2. Tile Header Field 0x20 - World Index Counter

### Discovery

**NOTE**: Function was previously misnamed `GetTileHeaderField08` - renamed to `GetTileHeaderWorldIndex` (2026-01-13).

```c
// Reads field 0x20 from TileHeader (Asset 100)
char GetTileHeaderWorldIndex(LevelDataContext* ctx) {
    return *(char*)(ctx[1] + 0x20);  // ctx[1] is TileHeader pointer
}
```

### Usage

1. **InitGameState** @ 0x8007cd34: Initializes player state:
   ```c
   g_pPlayerState[4] = GetTileHeaderWorldIndex(state + 0x21);
   ```

2. **FUN_8007d8a0** (Level Transition): Accumulates across levels:
   ```c
   g_pPlayerState[4] = g_pPlayerState[4] + GetTileHeaderWorldIndex();
   ```

### Values Observed

| Level | Stages | field_20 values |
|-------|--------|-----------------|
| MENU | All | 0 |
| PHRO | 0,1,2 | 6, 3, 1 |
| SCIE | 0-4 | 3, 6, 1, 1, 0 |
| TMPL | 0-3 | 2, 2, 5, 2 |
| FINN | 0 | 0 |
| MEGA | 0 | 0 |
| BOIL | 0-5 | 2, 1, 1, 0, 0, 0 |
| ... | ... | (values 0-6) |

### Purpose Hypothesis
- Values 1-6 observed, accumulated across level transitions
- Stored in `g_pPlayerState[4]` as running total
- Likely represents **world/area index** for audio or visual theming
- Could select background music themes or ambient sound sets

### Action Items
- [x] Rename `GetTileHeaderField08` to `GetTileHeaderWorldIndex` in Ghidra (DONE 2026-01-13)
- [ ] Find xrefs to `g_pPlayerState[4]` to determine what consumes this value
- [ ] Update blb.hexpat field comment

---

## 3. Layer Entry Field 0x2A - CONFIRMED PADDING

### Finding
Searched all 26 levels, all stages, all layers for `unknown_2a` values.
**Result: All values are 0.**

### Action
- Mark as padding in blb.hexpat
- Remove from "unknown" list

---

## 4. Asset 700 (0x2BC) - SPU Audio Header

### Structure (from hexpat)
```c
struct Asset700Header {
    u32 entry_count;   // Always 1
    u32 reserved;      // 0
    u32 entry_id;      // Varies - NOT ASCII
    u32 data_size;     // Size of data after header (NOT ADPCM)
    u32 data_offset;   // Offset to data (always 16)
};
```

### Levels with Asset 700

| Level | entry_id | size | data entries |
|-------|----------|------|--------------|
| MENU (0) | 0x50412804 | 480 | 116 |
| SCIE (2) | 0x1847C001 | 284 | 67 |
| TMPL (3) | 0x5024100C | 304 | 72 |
| BOIL (6) | 0x50412804 | 480 | 116 (same as MENU!) |
| FOOD (8) | 0x10031015 | 316 | 75 |
| BRG1 (10) | 0x72000210 | 340 | 81 |
| GLID (11) | 0x0101820A | 192 | 44 |
| CAVE (12) | 0x10050221 | 208 | 48 |
| WEED (13) | 0x31190002 | 344 | 82 |

### Data Structure Analysis (2026-01-13)

Data after header is organized as **4-byte entries**:
```
+00: u8 command_byte    (128=0x80, 192=0xC0 common)
+01: u8 flags           (0 or 32=0x20 bit)
+02: u8 param           (various values 2-118)
+03: u8 reserved        (usually 0, sometimes 255)
```

Example from GLID (level_11):
```
Entry 0: [47, 0, 66, 255] - command=47, flags=0, param=66, reserved=255
Entry 1: [128, 0, 118, 0] - command=128 (0x80), param=118
Entry 2: [128, 32, 28, 0] - command=128, flags=32 (0x20 bit), param=28
Entry 3: [192, 32, 27, 0] - command=192 (0xC0), flags=32, param=27
```

The command bytes (0x80, 0xC0) resemble PSX SPU control codes.

### Purpose Hypothesis
NOT ADPCM samples (unlike Asset 601). Likely:
1. Sound event trigger sequences
2. SPU channel configuration commands
3. Music/SFX playback schedules

### Needs Further Analysis
- Trace where ctx[21] (Asset 700 pointer) is used at runtime
- Compare command bytes with PSX SPU documentation
- Check if 0x80/0xC0 relate to voice on/off commands

---

## 5. TileAttributeHeader Unknown Field (Asset 500) - TWO U16 VALUES

### Discovery

The first 4 bytes of Asset 500 (TileAttributeMap) were previously documented as:
```c
u32 unknown;  // "Always 0 - padding?"
```

**This is WRONG!** Analysis of all 98 stages with Asset 500 reveals:
- The field contains **two u16 values**, not one u32
- 69 out of 98 stages have non-zero values
- Values range: lo = 0-21, hi = 0-21

### Data Analysis

```
Level/Stage            (lo, hi)   Dimensions
--------------------------------------------
level_01/stage0        ( 7,  0)   428 x  55
level_01/stage1        (21, 13)   328 x  52
level_04/stage0        (18,  5)   180 x  45
level_07/stage3        ( 0,  4)   394 x 113
level_11/stage0        ( 3,  5)   495 x   9   (RUNN - horizontal auto-scroller)
level_16/stage3        ( 0, 21)   200 x  61   (SNOW)
level_23/stage5        ( 7, 20)    87 x   1
```

### Correlations Attempted
- **Level/stage index**: No correlation
- **Level dimensions**: No clear pattern
- **Spawn position**: No correlation
- **Tile page counts**: No clear relationship

### Levels WITHOUT Asset 500
- level_00 (MENU) - all stages lack Asset 500
- This makes sense - menu doesn't need collision data

### Updated Structure
```c
struct TileAttributeHeader {
    u16 unknown_lo;    // Range 0-21, purpose unverified
    u16 unknown_hi;    // Range 0-21, purpose unverified
    u16 level_width;   // Width in tiles
    u16 level_height;  // Height in tiles
};
```

### Ghidra Function Discovery (2026-01-13)

Found the accessor functions for Asset 500:

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007b74c | `HasTileAttributes` | Returns true if Asset 500 exists |
| 0x8007b758 | `GetTileAttributeUnknown` | Reads bytes 0-3 (the two u16 values) |
| 0x8007b778 | `GetTileAttributeDimensions` | Reads bytes 4-7 (width/height) |
| 0x8007b79c | `GetTileAttributeData` | Returns pointer to collision data (header+8) |
| 0x80024cf4 | `InitTileAttributeState` | Copies header to GameState (called during level load) |

**Data Flow:**
1. `InitializeAndLoadLevel` calls `InitTileAttributeState`
2. `InitTileAttributeState` calls `GetTileAttributeUnknown` and `GetTileAttributeDimensions`
3. Values copied to:
   - `GameState + 0x68`: Pointer to collision data
   - `GameState + 0x6c`: Unknown u32 (the two u16 values)
   - `GameState + 0x70`: Width/height u32

**No consumers found!** After tracing the code, the unknown u16 values are copied to GameState+0x6c but I found **no code that reads them back**. This suggests:
- Possibly **unused/vestigial** values from development
- May only be used for validation during loading (not runtime)
- Could require runtime tracing with breakpoints to confirm

### Action Items
- [x] Find accessor functions in Ghidra (DONE - see table above)
- [ ] Set breakpoint on GameState+0x6c to see if anything reads it at runtime
- [ ] Check if values are debugging/editor-only metadata

---

## 6. Playback Sequence Header (offset 0xF34)

### Location
- PAL: Header offset 0xF34-0xF35 (2 bytes before mode array at 0xF36)
- Currently marked as `unknown_00[2]` with `[[hidden]]`

### Values Observed
```
Byte[0] = 0x02 (2)
Byte[1] = 0x11 (17)
```

### Hypothesis
- Byte[1] = 17 matches the count of password screens (17 entries in table at 0xECC)
- Byte[0] = 2 could be a version/revision number

### Action Needed
- Search for code that reads 0xF34 or 0xF35
- Verify if byte[1] is used as password screen count
- May be unused padding

---

## Summary of Findings

| Field | Status | Purpose |
|-------|--------|---------|
| Asset 101 | ✅ UNDERSTOOD | VRAM texture bank slot counts |
| Tile Header 0x20 | ⚠️ PARTIAL | Cumulative world index, needs consumer analysis |
| Layer Entry 0x2A | ✅ CONFIRMED PADDING | All zeros, mark as padding |
| TileAttributeHeader +0x00/+0x02 | ⚠️ PARTIAL | Two u16 values (0-21 range), purpose unknown |
| Asset 700 data | ⚠️ PARTIAL | 4-byte entries, possibly SPU commands |
| Playback 0xF34 | ❓ LOW PRIORITY | Bytes [2, 17], possibly version + password count |

## Ghidra Fixes Completed (2026-01-13)

1. ✅ **Renamed `GetTileHeaderField08` to `GetTileHeaderWorldIndex`** at 0x8007b490
2. ✅ **Renamed `FUN_80013b1c` to `InitVRAMSlotTable`** at 0x80013b1c
3. ✅ **Renamed `FUN_8007b3fc` to `GetAsset101Entry`** with comment explaining VRAM slot configuration
