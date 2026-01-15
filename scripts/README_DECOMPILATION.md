# Automated Decompilation Pipeline

This directory contains scripts for automating the Ghidra-to-source decompilation workflow.

## Overview

The automated decompilation pipeline synchronizes Ghidra analysis with the build system, extracts decompiled C code, maintains symbol consistency, and verifies bit-perfect matching.

## Architecture

```
┌─────────────────┐
│  Ghidra SRE     │  ← Manual analysis and function naming
│  + MCP Bridge   │
└────────┬────────┘
         │
         ├─────────────────────────────────────────┐
         │                                         │
         ▼                                         ▼
┌─────────────────────┐                  ┌─────────────────┐
│ symbol_addrs.txt    │  ◄──────────────►│ splat config    │
│ (Function database) │  Bidirectional   │ (Segmentation)  │
└──────────┬──────────┘     Sync         └────────┬────────┘
           │                                       │
           └─────────────┬─────────────────────────┘
                         │
                         ▼
                   ┌───────────┐
                   │  Splat    │  Extracts ASM files
                   └─────┬─────┘
                         │
                         ▼
                   ┌───────────┐
                   │ m2c / MCP │  Decompiles to C
                   └─────┬─────┘
                         │
                         ▼
                   ┌───────────┐
                   │  src/*.c  │  Source files
                   └─────┬─────┘
                         │
                         ▼
                   ┌───────────┐
                   │ make check│  Verifies match
                   └───────────┘
```

## Scripts

### 1. `ghidra_sync_symbols.py` - Bidirectional Symbol Sync

Synchronizes function names and addresses between Ghidra and `symbol_addrs.txt`.

**Features:**
- Export all functions from Ghidra to `symbol_addrs.txt`
- Import symbols from `symbol_addrs.txt` to Ghidra
- Validate consistency between both
- Preserves manual annotations (size, type hints, comments)
- Calculates function sizes from Ghidra analysis

**Usage:**
```bash
# Export from Ghidra to symbol_addrs.txt
python3 scripts/ghidra_sync_symbols.py --export

# Import from symbol_addrs.txt to Ghidra
python3 scripts/ghidra_sync_symbols.py --import

# Validate both are in sync
python3 scripts/ghidra_sync_symbols.py --validate

# Dry run (preview changes)
python3 scripts/ghidra_sync_symbols.py --export --dry-run
```

**Symbol Format:**
```
FunctionName = 0x80012345; // size:0x88 type:func Optional comment
```

### 2. `update_splat_config.py` - Splat Config Updater

Automatically updates `config/splat.pal.yaml` with functions from Ghidra.

**Features:**
- Reads current splat configuration
- Queries Ghidra for all defined functions
- Inserts/updates C segment entries: `[offset, c, FunctionName]`
- Preserves existing manual segments (rodata, data, asm)
- Validates segment alignment and gaps

**Usage:**
```bash
# Update config with all Ghidra functions
python3 scripts/update_splat_config.py

# Dry run to see changes
python3 scripts/update_splat_config.py --dry-run

# Update only specific functions
python3 scripts/update_splat_config.py --filter "EntityType"

# Force overwrite existing C segments
python3 scripts/update_splat_config.py --force

# Validate configuration only
python3 scripts/update_splat_config.py --validate
```

### 3. `auto_decompile_function.py` - Automated Decompilation

End-to-end decompilation workflow for a single function.

**Features:**
- Generates `ctx.c` context file if needed
- Ensures function is in `symbol_addrs.txt`
- Ensures function is in splat config
- Runs splat to generate assembly
- Invokes m2c or Ghidra decompiler
- Creates/updates source file with decompiled code
- Optionally runs build verification

**Usage:**
```bash
# Decompile function by name
python3 scripts/auto_decompile_function.py GetLevelCount

# Decompile function by address
python3 scripts/auto_decompile_function.py 0x8007A9B0

# Decompile and verify build
python3 scripts/auto_decompile_function.py GetLevelCount --verify

# Use Ghidra's decompiler instead of m2c
python3 scripts/auto_decompile_function.py GetLevelCount --use-ghidra-decompile

# Dry run (show what would happen)
python3 scripts/auto_decompile_function.py GetLevelCount --dry-run
```

**Workflow:**
1. Generate context file (`ctx.c`)
2. Add function to `symbol_addrs.txt` if missing
3. Update splat config with function segment
4. Run splat to extract assembly
5. Run m2c (or Ghidra) to decompile
6. Replace `INCLUDE_ASM` in source file with C code
7. Optionally verify with `make check`

### 4. `auto_decompile.py` - Master Orchestration

Coordinates the entire decompilation pipeline for batch processing.

**Features:**
- Full auto mode: decompile all functions
- Single function mode
- Pattern-based batch mode
- Progress tracking with JSON database
- Build verification loop
- Success/failure statistics

**Usage:**
```bash
# Full auto mode: decompile all functions
python3 scripts/auto_decompile.py --full-auto

# Decompile single function
python3 scripts/auto_decompile.py --function GetLevelCount

# Decompile multiple functions by pattern
python3 scripts/auto_decompile.py --pattern "EntityType0*"

# Batch decompile with verification
python3 scripts/auto_decompile.py --batch --verify

# Skip previously attempted functions
python3 scripts/auto_decompile.py --full-auto --skip-attempted

# Limit number of functions
python3 scripts/auto_decompile.py --pattern "EntityType*" --limit 10

# Show progress report
python3 scripts/auto_decompile.py --report

# Dry run
python3 scripts/auto_decompile.py --full-auto --dry-run
```

**Progress Tracking:**
- Tracks attempted, matched, and failed functions
- Stores results in `build/decompilation_progress.json`
- Generates coverage and success rate statistics

## Makefile Integration

### New Target: `context`

Generates `ctx.c` context file for m2c:

```bash
make context
```

This preprocesses `include/common.h` to create a context file with all type definitions, function signatures, and struct layouts that m2c needs for accurate decompilation.

**Updated Target: `decompile`**

Now automatically generates `ctx.c` and uses it:

```bash
make decompile FUNC=GetLevelCount
```

## Prerequisites

### 1. Ghidra with MCP Bridge

The MCP bridge must be running:

```bash
# Start Ghidra MCP server (in separate terminal)
python3 scripts/ghidra_mcp_server.py
```

Default port: 8192

### 2. Required Tools

- Python 3.8+
- splat (`python3 -m splat`)
- m2c decompiler (`tools/m2c/m2c.py`)
- GCC 2.7.2-psx toolchain
- maspsx assembler

### 3. Environment

Set up the build environment:

```bash
# Download toolchain
./tools/download-toolchain.sh

# Verify tools
which python3
python3 -m splat --version
```

## Workflow Examples

### Example 1: Single Function Decompilation

```bash
# 1. Ensure Ghidra MCP is running
python3 scripts/ghidra_mcp_server.py &

# 2. Decompile function with verification
python3 scripts/auto_decompile_function.py GetLevelCount --verify

# 3. If successful, function is now in src/ and builds match
make check
```

### Example 2: Batch Decompilation

```bash
# 1. Export all symbols from Ghidra
python3 scripts/ghidra_sync_symbols.py --export

# 2. Decompile all EntityType functions
python3 scripts/auto_decompile.py --pattern "EntityType*" --verify --limit 10

# 3. Check progress
python3 scripts/auto_decompile.py --report
```

### Example 3: Full Auto Mode

```bash
# Decompile everything with verification
python3 scripts/auto_decompile.py --full-auto --verify --skip-attempted

# This will:
# - Sync all symbols
# - Update splat config
# - Decompile each function
# - Verify each with make check
# - Track progress in build/decompilation_progress.json
# - Skip functions already attempted
```

### Example 4: Manual Workflow (Study initPlayerState)

The only currently matching function is `initPlayerState`. To understand why it matches:

```bash
# 1. Look at the source
cat src/initPlayerState.c

# 2. Check the splat config
grep -A2 "initPlayerState" config/splat.pal.yaml

# 3. Check symbol_addrs
grep "initPlayerState" symbol_addrs.txt

# 4. Rebuild and verify
make clean && make check

# 5. Compare assembly
make diff FUNC=initPlayerState
```

**Key factors for matching:**
- Function is small and simple (no complex control flow)
- Uses basic types (u8, s16) from common.h
- GCC 2.7.2 with `-O2 -G8` produces expected code
- No struct padding or alignment issues
- No function calls (except implicit memset-like loops)

## Troubleshooting

### "No functions found in Ghidra"

**Solution:** Ensure Ghidra MCP server is running and has a project open:
```bash
python3 scripts/ghidra_mcp_server.py
# Check http://localhost:8192/functions in browser
```

### "Assembly file not found"

**Solution:** Run splat first to extract assembly:
```bash
make extract
# or
python3 -m splat split config/splat.pal.yaml
```

### "Build does not match"

**Possible causes:**
- Decompiled code has different semantics
- Compiler optimization differences
- Missing type casts or qualifiers
- Struct padding issues

**Solutions:**
1. Compare with asm-differ: `make diff FUNC=FunctionName`
2. Try adjusting C code (casts, order of operations)
3. Use decomp-permuter: `make permuter FUNC=FunctionName`
4. Check Ghidra's decompilation: `--use-ghidra-decompile`

### "Symbol already exists in Ghidra with different name"

**Solution:** Use `--force` to overwrite:
```bash
python3 scripts/ghidra_sync_symbols.py --import --force
```

Or rename in Ghidra manually and re-export:
```bash
python3 scripts/ghidra_sync_symbols.py --export
```

## Progress Tracking

The pipeline tracks decompilation progress in `build/decompilation_progress.json`:

```json
{
  "attempted": {
    "GetLevelCount": {
      "address": "0x8007A9B0",
      "timestamp": 1705331234.56
    }
  },
  "matched": {
    "initPlayerState": {
      "address": "0x800260D0",
      "timestamp": 1705331234.56
    }
  },
  "failed": {
    "SomeFunction": {
      "address": "0x80012345",
      "timestamp": 1705331234.56,
      "reason": "Build mismatch"
    }
  },
  "stats": {
    "total_attempted": 100,
    "total_matched": 23,
    "total_failed": 77
  }
}
```

View report:
```bash
python3 scripts/auto_decompile.py --report
```

## Best Practices

### 1. Start with Simple Functions

Target small, self-contained functions first:
- Accessor functions (GetLevelCount, GetLevelFlag)
- Simple calculations
- Data structure initialization

Avoid complex functions initially:
- Large switch statements
- Nested loops
- Heavy pointer arithmetic

### 2. Verify Incrementally

Always use `--verify` for automatic rollback on failure:
```bash
python3 scripts/auto_decompile_function.py FunctionName --verify
```

### 3. Use Patterns for Related Functions

Decompile related functions together:
```bash
# All BLB accessor functions
python3 scripts/auto_decompile.py --pattern "GetLevel*" --verify

# All entity type initializers
python3 scripts/auto_decompile.py --pattern "EntityType*" --verify
```

### 4. Keep Symbols in Sync

Regularly export symbols from Ghidra:
```bash
# After renaming functions in Ghidra
python3 scripts/ghidra_sync_symbols.py --export

# Validate consistency
python3 scripts/ghidra_sync_symbols.py --validate
```

### 5. Preserve Manual Annotations

When using `--export`, manual annotations in `symbol_addrs.txt` are preserved:
- `size:0x88` - Function size
- `type:func` - Type hints for m2c
- Comments explaining purpose

## Future Enhancements

### Type Extraction from Ghidra

Extract function signatures and struct definitions from Ghidra to enhance `ctx.c`:
- Function prototypes with parameter types
- Struct definitions with field names
- Global variable declarations

### Rollback on Failure

Implement git-based rollback when build verification fails:
```bash
python3 scripts/auto_decompile.py --full-auto --verify --rollback-on-fail
```

### Parallel Decompilation

Decompile multiple functions in parallel:
```bash
python3 scripts/auto_decompile.py --full-auto --jobs 4
```

### Smart Function Ordering

Use Ghidra's call graph to decompile in optimal order:
1. Leaf functions first (no dependencies)
2. Functions with few dependencies
3. Complex high-level functions last

## References

- [Decompilation Guide](../docs/decompilation-guide.md)
- [Splat Documentation](https://github.com/ethteck/splat)
- [m2c Decompiler](https://github.com/matt-kempster/m2c)
- [Ghidra SRE](https://ghidra-sre.org/)

## Support

For issues or questions:
1. Check troubleshooting section above
2. Review existing progress: `python3 scripts/auto_decompile.py --report`
3. Examine logs in `build/decompilation_progress.json`
4. Compare with working example: `src/initPlayerState.c`
