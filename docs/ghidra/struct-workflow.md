# Ghidra Struct Workflow

## Overview

We now have a **bidirectional workflow** between Ghidra and C source code for struct definitions:

1. **Create structs in Ghidra** using MCP tools or GUI
2. **Export structs to C headers** using `decompile.py --export-struct`
3. **Use structs in decompilation** for better type information

## Creating Structs in Ghidra (MCP)

You can use the Ghidra MCP tools directly in Copilot Chat:

```
# List existing structs
Use mcp_ghidra_structs_list

# Create a new struct
Use mcp_ghidra_structs_create with:
  - name: "PlayerState"
  - category: "/Skullmonkeys"
  - description: "Player state @ 0x8001F3B4"

# Add fields
Use mcp_ghidra_structs_add_field with:
  - struct_name: "PlayerState"
  - field_name: "lives"
  - field_type: "byte"  (or "word", "dword", etc.)
  - offset: 0
  - comment: "Number of lives"

# Update existing field
Use mcp_ghidra_structs_update_field with:
  - struct_name: "PlayerState"
  - field_offset: 4  (or field_name: "lives")
  - new_name: "levelUnlock0"
  - new_comment: "Updated comment"

# Get struct details
Use mcp_ghidra_structs_get with name: "PlayerState"

# Delete struct
Use mcp_ghidra_structs_delete with name: "PlayerState"
```

## Exporting Structs to C Headers

Once you've defined a struct in Ghidra, export it:

```bash
# Export single struct
python3 scripts/decompile.py --export-struct PlayerState

# Save to header file
python3 scripts/decompile.py --export-struct PlayerState > include/player.h
```

**Output format:**
```c
/* -----------------------------------------------------------------------------
 * PlayerState
 * Player state structure initialized by initPlayerState @ 0x8001F3B4
 * Size: 18 bytes (0x12)
 * Exported from Ghidra
 * ----------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u8       lives;  // Number of lives (initialized to 1)
    /* 0x01 */ u8       continues;  // Number of continues (initialized to 1)
    /* 0x02 */ u16      results_total_tally;  // Cumulative results-screen value
    /* 0x04 */ u8       world_index_tally;    // World/progression tally
    /* 0x11 */ u8       health;  // Health counter (initialized to 5)
} PlayerState;  // Size: 0x12 (18 bytes)
```

The exporter:
- Maps Ghidra types (`byte`, `word`, `dword`) to PSX types (`u8`, `u16`, `u32`)
- Preserves field offsets and comments
- Generates padding fields for gaps
- Includes size information

## Complete Example: PlayerState

### 1. Created in Ghidra (via MCP)

```
Created struct "PlayerState" in category "/Skullmonkeys"
Added fields:
  0x00: u8 lives         (comment: "Number of lives (initialized to 1)")
  0x01: u8 continues     (comment: "Number of continues (initialized to 1)")
  0x02: u16 results_total_tally  (comment: "Cumulative results-screen value")
  0x04: u8 world_index_tally     (comment: "World/progression tally")
  0x11: u8 health        (comment: "Health counter (initialized to 5)")
```

### 2. Exported to C

```bash
python3 scripts/decompile.py --export-struct PlayerState > /tmp/player_exported.h
```

### 3. Used in Decompilation

In Ghidra:
- Right-click function parameter → Retype Variable → Select `PlayerState *`
- Decompilation now shows `state->lives` instead of `state[0x00]`

In C source (for binary matching):
```c
void initPlayerState(PlayerState *state) {
    u8 *ptr = (u8 *)state;  // Keep pointer arithmetic for matching
    ptr[0x00] = 1;  // lives
    ptr[0x01] = 1;  // continues
    // ... etc
}
```

## Recommended Workflow

### For Known Structures

1. **Research in Ghidra** - Analyze functions, find data structures
2. **Create struct in Ghidra** - Use MCP tools to define fields
3. **Export to header** - `--export-struct` to C file
4. **Use in analysis** - Retype pointers in Ghidra for better decompilation
5. **Document in code** - Add to `include/game.h` or similar

### For Partial Structures

1. **Start with known fields** - Only add verified fields
2. **Leave gaps as undefined** - Better than wrong assumptions
3. **Update incrementally** - Add fields as you discover them
4. **Re-export** - Update header file with new information

## Type Mapping

| Ghidra Type | C Type (PSX) | Size |
|-------------|--------------|------|
| `byte` | `u8` | 1 byte |
| `word` | `u16` | 2 bytes |
| `dword` | `u32` | 4 bytes |
| `qword` | `u64` | 8 bytes |
| `undefined` | `u8` | 1 byte (unknown) |
| Arrays work: `byte[10]` may fail, use individual fields |

## Benefits

✅ **Single source of truth** - Ghidra database is authoritative  
✅ **Better decompilation** - Typed pointers in Ghidra show field names  
✅ **Automatic export** - Generate C headers on demand  
✅ **Version control** - Track struct definitions in Ghidra project  
✅ **Incremental discovery** - Add fields as you learn  
✅ **Comments preserved** - Documentation travels with definitions  

## Next Steps

Recommended structs to create:
1. ✅ `PlayerState` (done, 18 bytes)
2. `GameState` (large, ~0x14A+ bytes)
3. `LevelDataContext` (128 bytes, well-documented)
4. `EntityDefinition` (24 bytes, Asset 501 format)
5. `BLBHeader` (4096 bytes, file header)
6. `TileHeader` (36 bytes, Asset 100)

Each struct should be created incrementally as fields are verified through code analysis.

## Headless REST server: write-side validation (gotchas)

The headless GhidraMCP server (`make ghidra-mcp`, HTTP on `127.0.0.1:8089`) is the
usable interface for batch edits — the `mcp__ghidra__` bridge could not discover the
headless instance, so drive it directly with `curl`. Read is GET with a query param;
**writes are JSON POST** (form-encoded POST is silently rejected — it reports the
arg as missing).

```bash
# READ (GET, query param):
curl "http://127.0.0.1:8089/decompile_function?address=0x80037434"

# WRITE (POST, JSON body) — form/-d key=val does NOT work, must be JSON:
curl -X POST http://127.0.0.1:8089/set_plate_comment \
  -H 'Content-Type: application/json' \
  -d '{"address":"0x800374B4","comment":"..."}'
```

Confirmed mutation endpoints: `set_plate_comment`, `set_decompiler_comment`,
`set_disassembly_comment`, `rename_function`, `rename_data`, `set_function_prototype`.

Two validators will warn (and the warnings are noise for this project):

1. **Plate-comment section check.** `set_plate_comment` expects the comment to
   contain `Algorithm:`, `Parameters:`, and `Returns:` sections (Purpose/Notes are
   also conventional). Omitting them still succeeds but returns
   `"warnings":["Plate comment missing Algorithm section", ...]`. Keep the
   structured layout to silence it.

2. **Function-name linter (conflicts with our convention).** `rename_function`
   lints names to verb + PascalCase, no underscores (it suggests verbs like
   `Initialize`/`Get`/`Set`). Our project deliberately uses `Init*` and keeps a
   `_<sprite-hash>` suffix for traceability (e.g. `InitFlyingProjectile_168254B5`).
   The rename **still applies** — treat the PascalCase/underscore/verb warnings as
   expected and ignore them; follow the existing repo naming convention, not the
   linter.

`rename_function` params are **camelCase** (`oldName`/`newName`), not snake_case —
the snake_case form errors with "Old function name is required". The `list_functions`
`filter` param is currently ignored (returns the unfiltered list); verify a rename by
re-decompiling the address instead.

> Note: this is GhidraMCP **tooling** behavior, not a cc1 codegen quirk — kept here
> rather than in `docs/compiler-quirks.md`, which is strictly about the compiler.
