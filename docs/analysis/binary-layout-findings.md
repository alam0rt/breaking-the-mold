---
title: "Binary Layout Analysis Findings"
category: analysis
tags: [analysis, binary, layout, findings]
---

# Binary Layout Analysis Findings

**Status: UNCONFIRMED** - These findings require manual verification in Ghidra before being considered authoritative.

**Date:** January 22, 2026  
**Binary:** SLES_010.90 (PAL)  
**Ghidra Database:** btm project on port 8192

## Summary Statistics

| Metric | Value |
|--------|-------|
| Total symbols | 3,493 |
| Functions | 2,259 |
| Data items | 1,234 |
| Min function size | 4 bytes |
| Max function size | 8,660 bytes |
| Avg function size | 227 bytes |

## Proposed Segment Layout

Based on symbol analysis, here are the natural segment boundaries:

### Code Sections

| ROM Offset | VRAM Address | Type | Proposed Name | Description |
|------------|--------------|------|---------------|-------------|
| 0x800 | 0x80010000 | asm | core/early | Early init (memset_entrypoint, DecodeRLESprite) |
| 0x39F0 | 0x800131F0 | asm | GAME | Main game code block 1 (292 functions) |
| 0x18808 | 0x80028008 | asm | GAME | Main game code block 2 - entities (349 functions) |
| 0x31A08 | 0x80041208 | asm | GAME | Main game code block 3 - core systems (983 functions) |
| 0x70C30 | 0x80080430 | asm | entity/init | Entity initialization functions |
| 0x73800 | 0x80083000 | asm | LIBS | PSY-Q library code (interleaved) |

### Data Sections

| ROM Offset | VRAM Address | Type | Proposed Name | Description |
|------------|--------------|------|---------------|-------------|
| 0xB24 | 0x80010324 | rodata | GAME_tables | Entity callback tables, remap tables |
| 0x2FF0 | 0x800127F0 | rodata | LIBS_strings | PSY-Q debug strings (libcd) |
| 0x80FD0 | 0x800907D0 | data | sdata | Static data section begins |
| 0x8CAC0 | 0x8009C2C0 | data | sdata | Global variables, lookup tables |

## PSY-Q Library Detection

Functions were pattern-matched to identify PSY-Q library origins:

| Library | Count | Address Range | ROM Range |
|---------|-------|---------------|-----------|
| game | 1,811 | 0x800127EC - 0x8009C2A8 | 0x2FEC - 0x8CAA8 |
| libapi | 96 | 0x800831F8 - 0x800905FC | 0x739F8 - 0x80DFC |
| libcd | 65 | 0x80038BA0 - 0x800865FC | 0x293A0 - 0x76DFC |
| libc | 52 | 0x80010000 - 0x8008E7EC | 0x800 - 0x7EFEC |
| libpress | 43 | 0x80010068 - 0x8008E550 | 0x868 - 0x7ED50 |
| libspu | 27 | 0x8008E81C - 0x800907B4 | 0x7F01C - 0x80FB4 |
| libgpu | 18 | 0x800879C4 - 0x8008CE6C | 0x781C4 - 0x7D66C |
| unknown | 141 | 0x80082FF8 - 0x80090638 | 0x737F8 - 0x80E38 |
| libetc | 3 | 0x8008762C - 0x80087E18 | 0x77E2C - 0x78618 |
| libgte | 3 | 0x8008D58C - 0x8008D81C | 0x7DD8C - 0x7E01C |

**Key Finding:** PSY-Q libraries are scattered throughout the binary, not in one contiguous block. This is typical of GCC 2.7.2 linker behavior (links objects in reference order).

## Anomalies Requiring Investigation

### 1. Jump Trampolines Identified as Functions

**Status:** CONFIRMED - Verified via Ghidra disassembly

**Pattern:** 8-byte functions with `j target; ori a1,zero,param` pattern. These are dispatch table entries that pass a parameter to a common handler.

**Statistics:**
- 28 trampolines total
- 4 unique targets (common handler functions)
- 15 additional stubs (`jr ra; nop` - just return)
- 6 real functions (actual code)

**Trampoline Groups:**

| Target | Count | Functions |
|--------|-------|-----------|
| 0x8008FAFC | 7 | S_SCA_OBJ_5C through S_SCA_OBJ_8C |
| 0x8008FBC4 | 7 | S_SCA_OBJ_124 through S_SCA_OBJ_154 |
| 0x80090020 | 7 | SR_SV_OBJ_1EC through SR_SV_OBJ_21C |
| 0x800900F0 | 7 | SR_SV_OBJ_2BC through SR_SV_OBJ_2EC |

**Example disassembly (S_SCA_OBJ_64 @ 0x8008FAC4):**
```asm
8008fac4: j 0x8008fafc      ; Jump to handler
8008fac8: ori a1,zero,0x9000 ; Pass 0x9000 as parameter (delay slot)
```

**Fixing in Ghidra:**

**Option A: Manual (for a few functions)**
1. Navigate to the function address (e.g., `g` then type `8008fabc`)
2. Right-click the function → "Delete Function" 
3. The code remains as disassembly, just no longer a function
4. Add a plate comment: Right-click → "Comments" → "Set Plate Comment"
5. Enter: `TRAMPOLINE: S_SCA_OBJ_5C -> 0x8008fafc`
6. Create a label: Right-click → "Create Label" → `trampoline_S_SCA_OBJ_5C`

**Option B: Script (recommended)**
Run the provided Ghidra script:
```
tools/ghidra_scripts/ConvertTrampolines.py
```

This script will:
1. Preview all trampolines to be converted
2. Ask for confirmation
3. Remove function definitions
4. Add plate comments explaining the trampoline
5. Create labels prefixed with `trampoline_`

**Why this matters:**
- Reduces false function count (28 fewer "functions")
- More accurate analysis of actual code
- Proper attribution to PSY-Q libsnd library patterns

### 2. Tiny Functions (≤8 bytes)

61 functions are 8 bytes or smaller. Most appear to be legitimate trampolines or simple wrappers:

| Address | Size | Name | Notes |
|---------|------|------|-------|
| 0x800127EC | 4 | Callback_800127ec | Single instruction |
| 0x80013244 | 4 | SetVideoModePAL | Single instruction |
| 0x80015434 | 8 | StubVibrateOff | Stub function |
| 0x800195AC | 4 | FindOrderingTableEntryByValue | Tail call |
| 0x8001A304 | 8 | CalculateParallaxXOffset | Simple calc |
| 0x8002615C | 8 | ClearHamsterCount | Simple setter |

**Action Required:** Verify each 4-byte function is not misidentified data.

### 3. Large Functions (>4KB)

Only 2 functions exceed 4KB:

| Address | Size | Name |
|---------|------|------|
| 0x800281A4 | 8,660 | CreateMenuEntities |
| 0x80078200 | 5,180 | CreateBossPlayerEntity |

These are legitimate large functions (menu system and boss logic).

### 4. Memory Gaps

Only one significant gap found:
- **Gap:** 0x800AE3E0 - 0x801FC400 (1,368,096 bytes)
- **After:** g_BLBHeaderBase
- **Before:** Magic1234
- **Explanation:** This is the dynamic heap/stack region - no static symbols expected.

### 5. Type Transitions (Function ↔ Data)

Key boundaries identified at type transitions:

| Address | ROM | Transition | After | Before |
|---------|-----|------------|-------|--------|
| 0x80010324 | 0xB24 | func→data | DecodeRLESprite | DAT_80010324 |
| 0x800131F0 | 0x39F0 | data→func | (unnamed) | GetFrameReadyFlag |
| 0x800907D0 | 0x80FD0 | func→data | SpuSetCommonCDVolume | g_CrtConstructorTable |

These transitions mark natural segment boundaries.

## 4KB Block Analysis

Dominant type per 4KB block (significant blocks only):

| ROM Block | Functions | Data | Dominant | First Symbol |
|-----------|-----------|------|----------|--------------|
| 0x00000 | 2 | 9 | DATA | memset_entrypoint |
| 0x02000 | 1 | 12 | DATA | switchdataD_80012158 |
| 0x03000 | 14 | 179 | DATA | s_CdlDemute_8001280c |
| 0x04000 | 13 | 0 | FUNC | AllocPrimSlot_0x18 |
| ... | | | | |
| 0x80000 | 73 | 9 | FUNC | S_SK_OBJ_1EC |
| 0x81000 | 0 | 3 | DATA | null_00h_80090809 |
| 0x8C000 | 2 | 21 | DATA | null_FF60h_8009b860 |

**Finding:** Code section ends around ROM 0x80FD0 (VRAM 0x800907D0), data section begins there.

## Proposed splat.yaml Segments

```yaml
# Based on binary layout analysis - UNCONFIRMED
segments:
  - name: main
    type: code
    start: 0x80010000
    subsegments:
      # Early init
      - [0x800, asm, core/early]
      
      # Game rodata - callback tables
      - [0xB24, rodata, GAME_tables]
      
      # PSY-Q strings
      - [0x2FF0, rodata, LIBS_strings]
      
      # Main game code
      - [0x39F0, asm]
      
      # PSY-Q libraries (interleaved with game code)
      - [0x73800, asm, LIBS]
      
      # Static data section
      - [0x80FD0, data, sdata]
```

## Verification Checklist

- [ ] Verify jump trampolines pattern in Ghidra (sample: S_SCA_OBJ_5C)
- [ ] Verify 4-byte functions are not misidentified data
- [ ] Confirm PSY-Q library boundaries with SDK documentation
- [ ] Test proposed splat.yaml produces matching build
- [ ] Verify sdata section boundary at 0x80FD0

## Files Generated

- `build/symbols_export.csv` - Full symbol listing (3,493 entries)
- `docs/analysis/binary-layout-analysis-methodology.md` - Process documentation
- `docs/analysis/binary-layout-findings.md` - This file

## Next Steps

1. **Manual Verification:** Review anomalies in Ghidra
2. **Trampoline Cleanup:** Create script to rename/merge jump trampolines
3. **Splat Config:** Update `config/splat.pal.yaml` with verified boundaries
4. **Build Test:** Verify changes produce matching build
