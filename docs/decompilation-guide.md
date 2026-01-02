# Decompilation Guide

Quick reference for decompiling functions with proper type information.

## Setup (One-time)

1. **PSY-Q Headers**: Place headers in `psyq/` directory (gitignored for legal reasons)
2. **Create lowercase symlinks** (PSY-Q uses mixed case internally):
   ```bash
   cd psyq && for f in *.H; do ln -sf "$f" "$(echo "$f" | tr 'A-Z' 'a-z')"; done
   cd SYS && for f in *.H; do ln -sf "$f" "$(echo "$f" | tr 'A-Z' 'a-z')"; done
   ```

## Adding a New Function

### 1. Add to splat config (`config/splat.pal.yaml`)

`./scripts/addr2offset.sh 0x0`

Or...

Calculate file offset: `(VRAM - 0x80010000) + 0x800`

```yaml
- [0x39F0, asm]
# MyFunction: 0x80012345 - 0x800123FF
- [0x12B45, c, MyFunction]
- [0x12C00, asm]
```

### 2. Add to symbol_addrs.txt

```
MyFunction = 0x80012345;

# For data symbols, add type info:
g_MyGlobal = 0x8009ABCD; // type:s32
g_MyStruct = 0x8009B000; // type:CdlFILE
```

### 3. Run splat

```bash
python3 -m splat split config/splat.pal.yaml
```

## Decompiling with Types

### Generate context file
```bash
cpp -E -P -I include -I psyq -D_LANGUAGE_C include/common.h > ctx.c
```

### Decompile
```bash
python3 tools/m2c/m2c.py -t mipsel-gcc-c --context ctx.c \
    asm/pal/nonmatchings/MyFunction/MyFunction.s
```

## Adding New Types

### PSY-Q types
Already available via `include/common.h` which includes:
- `LIBAPI.H` - BIOS functions
- `LIBGTE.H` - GTE (geometry)
- `LIBGPU.H` - GPU/graphics
- `LIBCD.H` - CD-ROM (`CdlFILE`, `CdSearchFile`, etc.)
- `LIBSPU.H` - Sound
- `LIBETC.H` - Misc utilities
- `LIBPAD.H` - Controller

## Symbol Types in symbol_addrs.txt

```
# Functions
MyFunc = 0x80012345; // type:func

# Basic types
myInt = 0x8009ABCD; // type:s32
myByte = 0x8009ABCE; // type:u8

# Structs (must be defined in headers)
myFile = 0x8009B000; // type:CdlFILE

# Arrays
myArray = 0x8009C000; // type:s32[]
```

## Troubleshooting

### m2c shows `?` for types
- Add the symbol to `symbol_addrs.txt` with `// type:TypeName`
- Ensure the type is defined in a header included by `common.h`
- Regenerate `ctx.c`

### "No such file" for PSY-Q headers
- Check lowercase symlinks exist in `psyq/`
- Headers use lowercase includes internally (e.g., `#include "kernel.h"`)

### Types not found by m2c
- Ensure `ctx.c` was regenerated after adding types
- Check for preprocessor errors: `cpp -E -I include -I psyq include/common.h 2>&1 | head`

## Complete Workflow: Adding New Functions

This is the full process for discovering and adding new functions to the decompilation.

### Step 1: Identify Function Boundaries

Use the original binary to find where functions start and end. Functions end with `jr $ra` (`08 00 e0 03` in little-endian hex).

```bash
# Convert VRAM address to file offset
./scripts/addr2offset.sh 0x8007ABCC
# Output: Offset: 0x6B3CC

# Dump bytes to find jr $ra instructions
od -A x -t x1 -j 0x6B3CC -N 0x100 disks/pal/SLES_010.90
# Look for: 08 00 e0 03 (jr $ra)
# The function ends after the delay slot (4 bytes after jr $ra)
```

**Function size calculation:**
- Find `jr $ra` at offset X
- Function ends at X + 8 (includes delay slot nop)
- Size = (end_offset - start_offset)

### Step 2: Add Symbols to symbol_addrs.txt

```
func_8007ABCC = 0x8007ABCC; // size:0x88
```

**Important:** Do NOT use colons (`:`) in comments after the semicolon, as splat parses them as attributes.

### Step 3: Update splat.pal.yaml

Add segment entries for your functions. Group related functions into a single C file:

```yaml
# Calculate: file_offset = (VRAM - 0x80010000) + 0x800
# func_8007ABCC: (0x8007ABCC - 0x80010000) + 0x800 = 0x6B3CC
- [0x6B3CC, c, CreditsAccessors]
```

### Step 4: Run splat

```bash
make clean  # Remove old asm files
python3 -m splat split config/splat.pal.yaml
```

This generates:
- `src/CreditsAccessors.c` - Stub file with INCLUDE_ASM macros
- `asm/pal/nonmatchings/CreditsAccessors/*.s` - Assembly for each function

### Step 5: Decompile with m2c

```bash
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
    asm/pal/nonmatchings/CreditsAccessors/func_8007ABCC.s
```

### Step 6: Replace INCLUDE_ASM with Decompiled C

Edit `src/CreditsAccessors.c`:
- Replace `INCLUDE_ASM(...)` with the decompiled function
- Add appropriate types and documentation

### Step 7: Build and Verify

```bash
make              # Build
make check        # Verify byte-match with original
```

If the build fails or doesn't match:
- Check function sizes in symbol_addrs.txt
- Use `INCLUDE_ASM` for functions that don't match yet
- Try different compiler flags or manual tweaks

## Example: Decompiling a Simple Accessor

**Original assembly** (`func_8007A9B0`):
```asm
lw    $v0, 0x5C($a0)    ; Load header pointer
nop
lbu   $v0, 0xF31($v0)   ; Load byte at header+0xF31
jr    $ra               ; Return
nop
```

**Decompiled C:**
```c
u8 GetLevelCount(GameState *state) {
    return state->headerBuffer[0xF31];
}
```

