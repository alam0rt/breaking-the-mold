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

**IMPORTANT**: Function `GetTileHeaderField08` @ 0x8007b490 is **MISNAMED**!

```c
// Actually reads field 0x20, NOT field 0x08!
char GetTileHeaderField08(LevelDataContext* ctx) {
    return *(char*)(ctx[1] + 0x20);  // ctx[1] is TileHeader pointer
}
```

### Usage

1. **InitGameState** @ 0x8007cd34: Initializes player state:
   ```c
   g_pPlayerState[4] = GetTileHeaderField08(state + 0x21);
   ```

2. **FUN_8007d8a0** (Level Transition): Accumulates across levels:
   ```c
   g_pPlayerState[4] = g_pPlayerState[4] + GetTileHeaderField08();
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
- [ ] Rename `GetTileHeaderField08` to `GetTileHeaderField20` in Ghidra
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
    u32 data_size;     // Size of ADPCM data
    u32 data_offset;   // Offset to data (typically 16)
};
```

### Levels with Asset 700
- MENU: count=1, size=480
- SCIE: count=1, size=284
- TMPL: count=1, size=304  
- BOIL: count=1, size=480 (same as MENU)
- FOOD: count=1, size=316
- BRG1: count=1, size=340
- GLID: count=1, size=192
- CAVE: count=1, size=208
- WEED: count=1, size=344

### Purpose
Additional SPU audio samples beyond Asset 601. Loaded to SPU RAM after main audio.
The entry_id values don't appear to be ASCII - possibly sample identifiers or offsets.

### Needs Further Analysis
- Trace where ctx[21] (Asset 700 pointer) is used at runtime
- Determine entry_id meaning

---

## 5. Playback Sequence unknown_00 - TBD

The playback sequence entries have 2 unknown bytes at offset 0x00.
Need to examine if these are used or padding.

---

## Summary of Findings

| Field | Status | Purpose |
|-------|--------|---------|
| Asset 101 | ✅ UNDERSTOOD | VRAM texture bank slot counts |
| Tile Header 0x20 | ⚠️ PARTIAL | Cumulative world index, needs consumer analysis |
| Layer Entry 0x2A | ✅ CONFIRMED PADDING | All zeros, mark as padding |
| Asset 700 | ⚠️ PARTIAL | SPU audio, structure understood, entry_id meaning unknown |
| Playback unknown_00 | ❓ TODO | Needs investigation |

## Ghidra Fixes Needed

1. **Rename `GetTileHeaderField08` to `GetTileHeaderField20`** at 0x8007b490
2. **Rename `FUN_80013b1c` to `InitVRAMSlotTable`** at 0x80013b1c
3. **Create `GetAsset101Entry` comment** explaining VRAM slot configuration
