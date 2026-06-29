---
title: "Decompilation Guide"
category: root
tags: [decompilation, guide]
---

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

> The splat config is generated from `SLES_010.90.jsonnet` into
> `SLES_010.90.yaml` by `make config`. Edit the `.jsonnet` source, not the
> generated `.yaml`. RAM↔offset conversion uses the `ram2off` / `off2ram`
> helpers provided in the `nix develop` shell (offset = `(VRAM − 0x80010000) + 0x800`).

### 1. Add the segment to `SLES_010.90.jsonnet`

```jsonnet
// MyFunction: 0x80012345 - 0x800123FF  ->  offset off2ram/ram2off
[0x12B45, 'c', 'MyFunction'],
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
make extract          # re-render config + re-split asm/ from the base ROM
```

## Decompiling with Types

### Generate context file
```bash
make context          # regenerates ctx.c (preprocessed common.h) for m2c
```

### Decompile
```bash
make decompile FUNC=MyFunction   # runs m2c on asm/nonmatchings/**/MyFunction.s
make diff FUNC=MyFunction        # asm-differ side-by-side against the target
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
# Convert VRAM address to file offset (ram2off is provided by `nix develop`)
ram2off 0x8007ABCC
# Output: 0x6B3CC

# Dump bytes to find jr $ra instructions
od -A x -t x1 -j 0x6B3CC -N 0x100 bin/SLES_010.90
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

### Step 3: Update `SLES_010.90.jsonnet`

Add segment entries for your functions. Group related functions into a single C file:

```jsonnet
// Calculate: file_offset = (VRAM - 0x80010000) + 0x800
// func_8007ABCC: (0x8007ABCC - 0x80010000) + 0x800 = 0x6B3CC
[0x6B3CC, 'c', 'creditsacc'],
```

### Step 4: Run splat

```bash
make extract        # re-render config + re-split asm/ from the base ROM
```

This generates:
- `src/creditsacc.c` - Stub file with INCLUDE_ASM macros
- `asm/nonmatchings/creditsacc/*.s` - Assembly for each function

### Step 5: Decompile with m2c

```bash
make decompile FUNC=func_8007ABCC
```

### Step 6: Replace INCLUDE_ASM with Decompiled C

Edit `src/creditsacc.c`:
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

## Decompilation priority / dependency order

`src/` is a flat layout (no `src/Game/`): ~46 `*.c` files, most already with
hundreds of matched functions. Roughly **731 functions remain stubbed** via
`INCLUDE_ASM` referencing `asm/nonmatchings/<seg>/<func>.s`. The notes below order
which of those stubs to convert to C first. An arrow A → B means B's code reads
state set up (or types defined) by A, so decompile A first to avoid re-stubbing.

Rough remaining-stub hotspots (run
`for f in src/*.c src/libs/*.c; do echo "$(grep -c INCLUDE_ASM "$f") $f"; done | sort -rn`
to refresh): `src/libs/libcd.c`, `src/enemies.c`, `src/playst.c`, `src/effects.c`,
`src/bosses.c`, then `finn.c`, `pickups.c`, `entity.c`, `anim.c`, `sprite.c`.

Suggested order (highest leverage first):

1. **Infrastructure / game loop / heap** — `main.c`, `gstate.c`/`gstctor.c`
   (GameState init), and the heap allocator (`AllocateFromHeap`). These set up the
   pointers every downstream subsystem reads. PSY-Q library code (`src/libs/`) is
   already linked from `tools/` and generally does not need hand-decomp.
2. **Data layer** — level/asset loading (`lvlload.c`, `blb.c`/`blbacc.c`/`blbmem.c`),
   tilemap/VRAM setup (`vram.c`, `layer.c`), sprite/RLE rendering
   (`sprite.c`/`sprset.c`/`spracc.c`, `prim.c`, `gfx.c`), and the animation
   framework (`anim.c`). Gameplay can't be verified until levels parse and draw.
   See `docs/blb/` (BLB format) and the relevant `docs/systems/` pages.
3. **Entity core** — `entity.c`/`entinit.c` (spawn, the entity callback dispatch
   table, `EntitySetState`, `SetEntitySpriteId`) and the entity-vs-entity collision
   path (`DispatchEventToCollidingEntity`). Most gameplay code dispatches through here.
4. **Player** — `player.c`/`playst.c`/`playdtor.c`. This is a large block of state
   callbacks; use [`reference/player-callback-inventory.md`](reference/player-callback-inventory.md)
   as the worklist. Within a cluster, start with the callback that has the most
   incoming call-sites — easier to verify against the original once decomp'd.
5. **Enemies / bosses** — `enemies.c`, `bosses.c`, `vehicle.c`, `clayball.c`. These
   depend on the entity core (3) and often the player (4).
6. **Polish / standalone** — `hud.c`, `menu.c`, `passwd.c`, `results.c`, `ending.c`,
   `movie.c`, `sound.c`, `effects.c`, `decor.c`, `pickups.c`. Mostly off the per-frame
   hot path; can land in any order.

Verification gate: run `make check` (SHA1 byte-match) after each function lands,
before moving on. If a function refuses to match under the default compiler, keep
its `INCLUDE_ASM` stub and track it for a later permuter / alternate-compiler pass
rather than tweaking the global flags (see `CLAUDE.md` § Build Pipeline).

> Symbol names and addresses in the older planning notes drift quickly — this is a
> clean-room project, so always confirm a name against `symbol_addrs.txt` and the
> live disassembly before relying on it.

## Related Documentation

- [`../CLAUDE.md`](../CLAUDE.md) — build pipeline, conventions
- [`reference/player-callback-inventory.md`](reference/player-callback-inventory.md) — player worklist
- [`KNOWLEDGE_GAPS.md`](KNOWLEDGE_GAPS.md) — what's still unknown
- [`analysis/unconfirmed-findings.md`](analysis/unconfirmed-findings.md) — risk-tagged research

