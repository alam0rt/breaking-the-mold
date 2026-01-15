# Decompilation System Improvements - Summary

## Problems with Original Approach

### 1. Multiple fragmented scripts
- `ghidra_sync_symbols.py` (470 lines)
- `update_splat_config.py` (335 lines)  
- `auto_decompile_function.py` (365 lines)
- `auto_decompile.py` (400 lines)
- `analyze_candidates.py` (new, for finding functions)

**Problem:** Too many scripts, unclear workflow, hard to maintain

### 2. YAML corruption
`update_splat_config.py` used `yaml.safe_dump()` which destroyed formatting:

**Before:**
```yaml
- [0x6B1B0, c, GetLevelCount]
- [0x6B1C4, c, StateAccessors]
```

**After yaml.dump():**
```yaml
- - 0x-8000F800  # Invalid hex!
  - c
  - GetAssetCount
- - '0x800'
  - rodata
```

### 3. Poor error handling
- Scripts didn't validate ROM offsets
- No checking for segment overlaps
- Cryptic error messages
- Hard to debug failures

### 4. Address parsing bugs
Example from `auto_decompile_function.py`:
```python
# Function lookup returned {'address': '8007acdc'} (string)
func_address = func.get("entryPoint", 0)  # Wrong field!
# Result: func_address = 0, leading to invalid ROM offset
```

### 5. Manual YAML editing required
Even with automation, you still had to:
- Calculate ROM offsets manually
- Edit YAML by hand when scripts failed
- Fix corrupted YAML after script runs
- Split existing segments manually

## New Simplified Approach

### Single unified tool: `decompile.py`

**Features:**
- ✅ One script does everything
- ✅ Preserves YAML formatting (ruamel.yaml)
- ✅ Validates before making changes
- ✅ Clear error messages with solutions
- ✅ Dry-run mode to preview changes
- ✅ Automatic address conversion
- ✅ Detects existing segments
- ✅ Better Ghidra integration

**Usage:**
```bash
# Preview
python3 scripts/decompile.py FunctionName --dry-run

# Prepare for decompilation
python3 scripts/decompile.py FunctionName --prepare

# Get initial C code
python3 scripts/decompile.py FunctionName --decompile

# Do everything
python3 scripts/decompile.py FunctionName --full

# Show Ghidra's version
python3 scripts/decompile.py FunctionName --ghidra
```

## Key Technical Improvements

### 1. YAML preservation with ruamel.yaml
```python
import ruamel.yaml
yaml = ruamel.yaml.YAML()
yaml.preserve_quotes = True
yaml.default_flow_style = False
yaml.width = 4096  # No line wrapping
```

### 2. Proper address handling
```python
def get_function_info(name: str) -> Optional[Dict]:
    # Get list result
    result = ghidra_request(f"/functions?name={name}")
    func = result.get("result", [])[0]
    
    # Parse address correctly (string or int)
    address_str = func.get("address", "0")
    address = int(address_str, 16) if isinstance(address_str, str) else address_str
    
    # Get full details
    detail = ghidra_request(f"/functions/{address:08x}")
    # Extract size from body.minAddress/maxAddress
```

### 3. Segment validation
```python
def find_segment_for_rom(rom_offset: int):
    """Find which segment contains the ROM offset."""
    # Parse YAML
    # Check each subsegment
    # Return (start, end, type)
    
    if seg_type == 'c':
        error("Function already in 'c' segment. Manual split needed.")
```

### 4. Clear workflow separation
```
--dry-run:    Preview only, no changes
--prepare:    Update YAML + generate ASM
--decompile:  Run m2c on existing ASM
--full:       prepare + decompile
--ghidra:     Show Ghidra's decompilation
```

## Lessons Learned

### Function Matching is Hard

**SaveCheckpointState** didn't match because:
- Local variables force register saves (s0, s1)
- Order of operations matters
- GCC optimizations are unpredictable
- Need to match original register usage

**Solutions:**
- Use `void *` with pointer arithmetic
- Minimize local variables
- Match exact operation order from assembly
- Study functions that DO match (GetLevelCount, GetAssetCount)

### Splat Workflow
From https://github.com/ethteck/splat/wiki/General-Workflow:

1. Functions are defined in `symbol_addrs.txt` with VRAM addresses
2. Segments in YAML use ROM offsets
3. Each `[offset, c, name]` creates one C file
4. Assembly goes in `asm/pal/nonmatchings/Name/func.s`
5. When C file exists, splat doesn't generate ASM

### One Function Per Segment (Usually)
To avoid linker ordering issues:
```yaml
- [0x6B3CC, c, CreditsAccessors]   # Functions at start of range
- [0x6B4DC, c, GetAssetCount]      # Extract to own segment
- [0x6B4F0, asm]                    # Continue after
```

Not:
```yaml
- [0x6B3CC, c, CreditsAccessors]   # Contains INCLUDE_ASM for GetAssetCount
                                    # Linker places ASM first, breaks addresses!
```

## Success Metrics

### Before:
- ❌ 4-5 scripts to learn
- ❌ YAML gets corrupted
- ❌ Manual offset calculation needed
- ❌ Unclear when things fail
- ❌ 30+ minute learning curve

### After:
- ✅ 1 script with clear modes
- ✅ YAML formatting preserved
- ✅ Automatic address conversion
- ✅ Helpful error messages
- ✅ 5 minute learning curve

## Successfully Decompiled Functions

1. **GetLevelCount** (20 bytes) - Simple accessor, perfect match ✓
2. **GetAssetCount** (20 bytes) - Had to extract from CreditsAccessors ✓
3. **SaveCheckpointState** (64 bytes) - Still working on match ⚠️

## Next Steps

1. Document function matching patterns (what works, what doesn't)
2. Add more validation to decompile.py (check for common issues)
3. Create matcher helper to compare assembly side-by-side
4. Build library of "matching recipes" for common patterns

## Dependencies

```bash
# Required
pip install ruamel.yaml

# Already installed
- Python 3.13
- splat 0.37.1
- m2c decompiler
- Ghidra with MCP server
```

## Files Changed

**New:**
- `scripts/decompile.py` - Unified decompilation tool
- `docs/DECOMPILATION_WORKFLOW.md` - User guide
- `scripts/README_DEPRECATED.md` - Migration guide
- `docs/DECOMPILATION_IMPROVEMENTS.md` - This file

**Modified:**
- `config/splat.pal.yaml` - Added GetAssetCount, SaveCheckpointState segments
- `symbol_addrs.txt` - Added new functions
- `src/GetAssetCount.c` - Successfully matched! ✓
- `src/SaveCheckpointState.c` - In progress
- `src/CreditsAccessors.c` - Removed extracted functions

**Kept for reference (deprecated):**
- `scripts/ghidra_sync_symbols.py`
- `scripts/update_splat_config.py`
- `scripts/auto_decompile_function.py`
- `scripts/auto_decompile.py`
- `scripts/README_DECOMPILATION.md` (old docs)
