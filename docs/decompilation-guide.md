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

### Game-specific types
Add to `include/common.h` or create a new header:

```c
// include/game.h
typedef struct {
    s32 x, y, z;
} Vec3;
```

Then include it in `common.h`:
```c
#if __has_include("game.h")
#include "game.h"
#endif
```

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
