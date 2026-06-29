---
title: "Binary Layout Analysis Methodology"
category: analysis
tags: [analysis, binary, layout, methodology]
---

# Binary Layout Analysis Methodology

This document describes the process used to analyze the Skullmonkeys (SLES-01090) binary layout for creating accurate splat segment configurations.

## Overview

The goal is to identify natural segment boundaries in the compiled binary to:
1. Create accurate splat configurations that match the original linker layout
2. Identify code vs data regions for proper disassembly
3. Detect PSY-Q library boundaries for proper attribution
4. Find anomalies that indicate Ghidra misidentification

## Tools Used

### 1. Ghidra MCP Server
The Ghidra instance runs an MCP server on port 8192, providing HTTP API access to:
- Function listing with addresses and names
- Data item listing with types and labels
- Disassembly for individual functions
- Cross-references

### 2. Symbol Export Script
`scripts/export_ghidra_symbols.py` exports all Ghidra symbols via HTTP API:

```bash
# Fast export with inferred sizes (no extra API calls)
python3 scripts/export_ghidra_symbols.py --exclude-external --infer-sizes --format=csv > build/symbols_export.csv
```

Key options:
- `--exclude-external`: Skip GTE external functions (addresses < 0x80000000)
- `--infer-sizes`: Calculate sizes from next symbol address (fast, no individual API calls)
- `--format=csv`: Proper CSV with quoted fields for parsing

### 3. Analysis Scripts
Python one-liners for analyzing the exported CSV:
- Gap analysis: Find unmapped regions between symbols
- Type transitions: Find function↔data boundaries
- Block analysis: Identify dominant type per 4KB block
- Library detection: Pattern-match PSY-Q function names

## Analysis Process

### Step 1: Export All Symbols
```bash
python3 scripts/export_ghidra_symbols.py --exclude-external --infer-sizes --format=csv 2>/dev/null > build/symbols_export.csv
```

This produces a CSV with columns: `address, rom_offset, type, name, size, data_type`

### Step 2: Calculate Statistics
```python
# Count functions vs data
funcs = [s for s in symbols if s['type'] == 'function']
data = [s for s in symbols if s['type'] == 'data']

# Size distribution
sizes = [int(s['size']) for s in funcs if s['size']]
```

### Step 3: Find Gaps
Gaps between symbols might indicate:
- Unmapped regions (missed functions/data)
- Padding between sections
- Jump tables or other structures

```python
for i, s in enumerate(symbols):
    if i > 0:
        prev_end = int(prev['address'], 16) + int(prev['size'] or 0)
        gap = int(s['address'], 16) - prev_end
        if gap > 256:
            print(f"Gap: {gap} bytes after {prev['name']}")
```

### Step 4: Identify Type Transitions
Function↔data transitions often indicate segment boundaries:

```python
for i, s in enumerate(symbols):
    if i > 0 and prev['type'] != s['type']:
        print(f"Transition: {prev['type']} -> {s['type']} at {s['address']}")
```

### Step 5: Detect PSY-Q Libraries
Pattern-match function names to identify library origins:

| Library   | Patterns |
|-----------|----------|
| libcd     | `Cd*`, `CdInit`, `CdRead`, `CDREAD*` |
| libspu    | `Spu*`, `SpuInit`, `_spu*`, `SPU_*` |
| libgpu    | `Gpu*`, `DrawSync`, `VSync`, `LoadImage`, `GPU_*` |
| libgte    | `gte_*`, `RotMatrix`, `TransMatrix`, `Set*Matrix` |
| libetc    | `PadInit`, `PadRead`, `ResetCallback`, `SENDPAD*` |
| libapi    | `_*` (underscore prefix), `SYS_OBJ*`, `INTR_*` |
| libpress  | `Dec*`, `MDEC*`, `Vlc*`, `VLC_*` |
| libc      | `mem*`, `str*`, `printf`, `malloc`, `free` |

### Step 6: Analyze 4KB Blocks
Group symbols by 4KB blocks to find dominant type:

```python
blocks = {}
for s in symbols:
    block = (rom_offset // 0x1000) * 0x1000
    blocks[block]['func'] += 1 if s['type'] == 'function' else 0
    blocks[block]['data'] += 1 if s['type'] == 'data' else 0
```

### Step 7: Identify Anomalies

#### Tiny Functions (≤8 bytes)
Functions smaller than 8 bytes are suspicious:
- Might be jump trampolines (valid but should be noted)
- Might be data misidentified as code
- Common in PSY-Q: `j target; nop` = 8 bytes

#### Adjacent Tiny Functions
Multiple 8-byte functions in sequence often indicate:
- Jump table trampolines (common in PSY-Q)
- Should potentially be merged or marked as jumptable data

#### Functions in Data Sections
Functions appearing after the expected code section end (ROM > 0x80FD0) are suspicious.

## Known Patterns

### GCC 2.7.2 Layout
The PSX toolchain (GCC 2.7.2 + PSY-Q) produces interleaved text/rodata:
- Functions and their associated rodata are often adjacent
- Jump tables immediately follow switch statements
- String literals appear near their using functions

This is why `ld_legacy_generation: true` is required in splat.

### PSY-Q Jump Trampolines
PSY-Q libraries often use 8-byte jump trampolines:
```asm
j target_function
ori a1, zero, 0x8000  ; delay slot with parameter
```

These appear as separate 8-byte "functions" in Ghidra but are really part of a dispatch table.

### Section Boundaries
Typical PSX binary layout:
1. `.text` - Code (ROM 0x800 onwards)
2. `.rodata` - Read-only data (interleaved with text)
3. `.data` - Initialized data
4. `.sdata` - Small data (accessed via $gp)
5. `.sbss` - Small BSS
6. `.bss` - Uninitialized data (not in ROM)

## Verification

Always verify findings by:
1. Checking disassembly in Ghidra for suspicious items
2. Cross-referencing with known PSY-Q source/docs
3. Testing splat configuration produces matching build

## Output

The analysis produces:
1. `build/symbols_export.csv` - Full symbol listing
2. Proposed segment boundaries for `config/splat.pal.yaml`
3. List of anomalies for manual investigation

## References

- [splat documentation](https://github.com/ethteck/splat)
- [PSY-Q SDK documentation](https://psx.arthus.net/sdk/Psy-Q/)
- [MIPS calling conventions](https://courses.cs.washington.edu/courses/cse410/09sp/examples/MIPSCallingConventionsSummary.pdf)
