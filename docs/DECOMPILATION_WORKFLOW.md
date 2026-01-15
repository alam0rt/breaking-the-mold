# Simplified Decompilation Workflow

## Quick Start

The new `scripts/decompile.py` tool handles everything for you.

### 1. Check what a function looks like

```bash
python3 scripts/decompile.py FunctionName --dry-run
```

This shows you:
- VRAM address
- ROM offset  
- Function size
- What would be changed

### 2. Prepare function for decompilation

```bash
python3 scripts/decompile.py FunctionName --prepare
```

This:
- Adds function to `symbol_addrs.txt` if missing
- Updates `config/splat.pal.yaml` (preserves formatting!)
- Runs splat to generate assembly
- Creates `asm/pal/nonmatchings/FunctionName/FunctionName.s`

### 3. Get initial decompilation

```bash
python3 scripts/decompile.py FunctionName --decompile
```

This runs m2c and shows you the initial C code.

### 4. One-step workflow

```bash
python3 scripts/decompile.py FunctionName --full
```

Does both --prepare and --decompile in one command.

### 5. See Ghidra's decompilation

```bash
python3 scripts/decompile.py FunctionName --ghidra
```

Shows Ghidra's decompilation for reference.

## Example: Decompiling GetLevelAssetIndex

```bash
# 1. See what will happen
python3 scripts/decompile.py GetLevelAssetIndex --dry-run

# 2. Prepare and get initial code
python3 scripts/decompile.py GetLevelAssetIndex --full

# 3. Edit src/GetLevelAssetIndex.c with the output

# 4. Build and test
make
make check

# 5. If it doesn't match, iterate on src/GetLevelAssetIndex.c
```

## Requirements

```bash
pip install ruamel.yaml
```

This preserves YAML formatting (unlike PyYAML which destroys it).

## Key Improvements Over Old Scripts

1. **Single tool** - No need to juggle multiple scripts
2. **Preserves YAML formatting** - Uses ruamel.yaml instead of PyYAML
3. **Better validation** - Checks segments, catches errors early
4. **Clear feedback** - Shows exactly what's happening
5. **Safer** - Dry-run mode, validates before changing files

## Troubleshooting

**"Failed to connect to Ghidra"**
- Make sure Ghidra is running with the MCP server enabled
- Check port 8192 is accessible

**"Function not found in Ghidra"**
- Function may not be in your current Ghidra project
- Check the function name spelling

**"Segment already exists"**
- Function is already prepared for decompilation
- Skip --prepare and go straight to --decompile

**"Function is already in a 'c' segment"**
- Function is part of an existing C file
- You need to manually split it out (see GetAssetCount example)

## Advanced: Manual Segment Splitting

If a function is inside an existing C file (like GetAssetCount was in CreditsAccessors.c):

1. Calculate ROM boundaries:
   ```bash
   ./scripts/addr2offset.sh 0x8007ACDC  # Start
   ./scripts/addr2offset.sh 0x8007ACEF  # End (start + size - 1)
   ```

2. Manually edit `config/splat.pal.yaml`:
   ```yaml
   - [0x6B3CC, c, OldFile]        # Keep what comes before
   - [0x6B4DC, c, NewFunction]    # Your extracted function
   - [0x6B4F0, asm]                # What comes after
   ```

3. Remove the function from the old C file

4. Run splat and rebuild:
   ```bash
   python3 -m splat split config/splat.pal.yaml
   make
   ```

## Tips for Matching Functions

- Start with simple accessor functions (like Get*)
- Use `void *` parameters with pointer arithmetic (see GetLevelCount.c)
- Avoid local variables that force register saves (s0, s1)
- Match the exact order of operations from the assembly
- Compare your assembly with the original:
  ```bash
  mipsel-unknown-linux-gnu-objdump -d build/pal/src/YourFunction.o
  ```

## Function Matching Examples

**Easy (20-40 bytes):** GetAssetCount, GetLevelCount  
**Medium (40-80 bytes):** getLevelName, GetLevelAssetIndex  
**Hard (80+ bytes):** SaveCheckpointState, InitializeAndLoadLevel

Start with easy functions to learn the process!
