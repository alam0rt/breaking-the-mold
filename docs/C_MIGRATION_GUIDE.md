---
title: "C Migration Guide for Skullmonkeys (SLES-01090)"
category: root
tags: [migration, guide]
---

# C Migration Guide for Skullmonkeys (SLES-01090)

Based on the [splat General Workflow](https://github.com/ethteck/splat/wiki/General-Workflow) and project-specific configuration.

## Prerequisites

Before migrating to C:
- ✅ Build matches original binary (SHA1 verified)
- ✅ `ld_legacy_generation: true` set (required for interleaved text/rodata)
- ✅ Symbol addresses exported from Ghidra
- ⚠️ `migrate_rodata_to_functions: false` in `SLES_010.90.yaml` (keep disabled until rodata is paired)

## Overview

The migration workflow is:
1. **Pair rodata with text segments** (optional but recommended)
2. **Change `asm` to `c`** in `SLES_010.90.yaml`
3. **Create C file** with `INCLUDE_ASM` macros
4. **Decompile functions** one by one
5. **Migrate data/rodata** to C when fully decompiled
6. **Update segment types** from `data`/`rodata` to `.data`/`.rodata`

## Splat Wiki Cross-Check (2026-06-13)

Relevant points from the upstream workflow for this repo:

- Pair `rodata` with its owning text segment before converting large `asm` segments to `c`. Splat's "Rodata segment may belong to text segment" messages are useful hints, but they are heuristic; verify questionable cases against disassembly.
- If one text segment appears to own multiple `rodata` segments, either the `rodata` was split too aggressively or the text segment is missing a split. If multiple text segments appear to own one `rodata` segment, split the text or verify that splat produced a false positive.
- Keep `data`/`rodata` as extracted assembly while functions are still included with `INCLUDE_ASM`; switch to `.data`/`.rodata` only after the data has actually been migrated into the compiled C object. With `auto_link_sections` enabled, fully migrated data subsegments can sometimes be omitted and splat will link the compiled sections.
- `bss` migration is trickier because it has no ROM bytes to compare directly; migrate only when symbol order, sizes, and section ownership are understood.
- The wiki's macro guidance matches this repo's split between `include/labels.inc`, `include/macro.inc`, and `include/include_asm.h`. If asm-differ ever struggles with jump-table `jlabel`s, the wiki suggests marking `jlabel` as a function, but that should only be tried as a controlled clean-build experiment.
- The page is workflow-focused; it does not explain compiler scheduling/register-allocation quirks like the CLUT slot-install `la` hoist in `docs/compiler-quirks.md`.

---

## Step 1: Choose a Function to Decompile

Start with a simple, well-understood function. Good candidates:
- Small functions (< 50 instructions)
- Functions with clear purpose (from Ghidra analysis)
- Functions with minimal dependencies

**Example candidates from our analysis:**
- `GetAssetCount` - Simple getter
- `GetTileHeaderPtr` - Simple pointer return
- `UpdateEntityRender` - Medium complexity, well-documented

Use the decompile.py helper:
```bash
# See what would happen
python3 scripts/decompile.py GetAssetCount --dry-run

# Full workflow: update YAML, generate ASM, get m2c output
python3 scripts/decompile.py GetAssetCount --full
```

---

## Step 2: Update `SLES_010.90.yaml`

### 2.1 Create a Named Segment

Change from anonymous `asm` to named segment:

**Before:**
```yaml
- [0x39F0, asm]
```

**After:**
```yaml
- [0x39F0, c, game]   # Named "game" - all game code
```

Or for a specific file:
```yaml
- [0x39F0, c, game/main]   # Creates src/game/main.c
```

### 2.2 For Our Binary (Interleaved Layout)

Since we have interleaved text/rodata with `ld_legacy_generation: true`, we keep the current structure but change `asm` to `c`:

```yaml
subsegments:
    # Early code
  - [0x800, asm, core/early]
    # Rodata segments (keep as-is for now)
  - [0xE2C, rodata]
  - [0x1F28, rodata]
  - [0x2044, rodata]
  - [0x2940, rodata, LIBS_1]
  - [0x34F4, rodata, LIBS_2]
  - [0x3970, rodata, LIBS_3]
    # Main game code - CHANGE THIS TO C
  - [0x39F0, c, game]       # <-- Changed from asm
    # PSY-Q libraries (libcd, libgpu, libgte, libspu, libetc)
  - [0x73800, asm, LIBS]
```

---

## Step 3: Run Splat

```bash
make clean
python3 -m splat split SLES_010.90.yaml
```

This generates:
- `src/game.c` - C file with `INCLUDE_ASM` macros for each function
- `asm/nonmatchings/game/*.s` - Individual function assembly files

---

## Step 4: Include Header

This repo already has `include/include_asm.h` with `INCLUDE_ASM` and `INCLUDE_RODATA` macros. Do not replace it unless intentionally testing a toolchain workaround. The current macro emits `.section .text`, sets `noat`/`noreorder`, includes `FOLDER/NAME.s`, then restores assembler settings. It includes `include/labels.inc` by default, or `include/macro.inc` when `INCLUDE_ASM_USE_MACRO_INC` is enabled.

---

## Step 5: Generated C File Structure

Splat generates a C file like this:

```c
#include "common.h"

INCLUDE_ASM("asm/nonmatchings/game", func_800131F0);
INCLUDE_ASM("asm/nonmatchings/game", func_80013240);
// ... more functions
```

---

## Step 6: Decompile a Function

### 6.1 Get Decompiled Code from m2c

```bash
# Using decompile.py
python3 scripts/decompile.py func_800131F0 --decompile

# Or manually with m2c
python3 tools/m2c/m2c.py \
    --context ctx.c \
    --target mipsel-gcc-c \
    asm/nonmatchings/game/func_800131F0.s
```

### 6.2 Replace INCLUDE_ASM with Decompiled Code

**Before:**
```c
INCLUDE_ASM("asm/nonmatchings/game", func_800131F0);
```

**After:**
```c
void func_800131F0(void) {
    // Decompiled code here
}
```

### 6.3 Build and Compare

```bash
make
make check
```

If it doesn't match:
- Use `diff.py` or asm-differ to compare
- Adjust code until it matches
- Common issues: register allocation, instruction ordering

---

## Step 7: Migrate Data (When Function is Done)

Once ALL functions that reference a piece of data are decompiled:

### 7.1 Move Data to C File

**In game.c:**
```c
// Previously in data.s or rodata.s
int g_SomeGlobal = 0x1234;
const char* g_SomeString = "Hello";
```

### 7.2 Update Segment Type

**In `SLES_010.90.yaml`:**
```yaml
# Before: extract as assembly
- [0x42200, data, game]
- [0x42300, rodata, game]

# After: link from compiled C
- [0x42200, .data, game]
- [0x42300, .rodata, game]
```

---

## Project-Specific Notes

### PSY-Q Libraries
- Keep `LIBS` segment as `asm` - don't try to decompile PSY-Q
- The LIBS_1/2/3 rodata serves both game code and PSY-Q

### Interleaved Sections
- `ld_legacy_generation: true` is required
- Splat warnings about rodata pairing are informational
- The build matches, so linking is correct

### Function Naming
- Use Ghidra-derived names when available
- Update `symbol_addrs.txt` with good names
- Run splat again to regenerate with new names

### Context File (ctx.c)
- Keep `ctx.c` updated with struct definitions
- Export structs from Ghidra: `python3 scripts/decompile.py --export-struct PlayerState`

---

## Recommended First Functions

Based on analysis, these are good starting points:

| Function | Address | Size | Reason |
|----------|---------|------|--------|
| `GetTileHeaderPtr` | 0x8007b4b8 | 28 | Simple getter, single return |
| `GetAssetCount` | 0x8007b4d4 | 20 | Very simple |
| `GetPaletteDataPtr` | 0x8007b4f8 | 48 | Small, clear purpose |
| `GetTotalTileCount` | 0x8007b53c | 76 | Medium, well-documented |

---

## Troubleshooting

### Build Doesn't Match
1. Check instruction ordering differences
2. Verify compiler flags match original
3. Check for missing/extra padding
4. Use `diff.py` to see byte-level differences

### Missing Symbols
1. Add to `symbol_addrs.txt`
2. Run splat again
3. Update `undefined_syms_auto.txt` if needed

### Rodata Not Linking
1. Ensure segment names match
2. Check `migrate_rodata_to_functions` setting
3. Verify `.rodata` vs `rodata` segment type

---

## Quick Reference Commands

```bash
# Full build
make clean && make && make check

# Just splat
python3 -m splat split SLES_010.90.yaml

# Decompile helper
python3 scripts/decompile.py <function_name> --full

# Generate context
python3 scripts/decompile.py --export-struct <StructName>

# Compare builds
sha1sum build/pal/game.elf disks/pal/SLES_010.90
```
