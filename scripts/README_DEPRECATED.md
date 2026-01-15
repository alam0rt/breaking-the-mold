# ⚠️ DEPRECATED - Use New Workflow

**These scripts have been replaced by the simplified `decompile.py` tool.**

See **[../docs/DECOMPILATION_WORKFLOW.md](../docs/DECOMPILATION_WORKFLOW.md)** for the new process.

## Why the change?

The old scripts had several issues:
1. `update_splat_config.py` corrupted YAML formatting
2. Multiple scripts made the workflow confusing
3. Poor error handling and validation
4. Hard to debug when things went wrong

## Quick comparison

### Old way (complex):
```bash
# Step 1: Sync symbols
python3 scripts/ghidra_sync_symbols.py --export

# Step 2: Update splat config
python3 scripts/update_splat_config.py --filter FunctionName

# Step 3: Run splat manually
python3 -m splat split config/splat.pal.yaml

# Step 4: Decompile
python3 scripts/auto_decompile_function.py FunctionName --verify
```

### New way (simple):
```bash
python3 scripts/decompile.py FunctionName --full
```

## Migration guide

If you were using the old scripts, switch to:
- `decompile.py --dry-run` - See what will happen
- `decompile.py --prepare` - Update YAML and generate ASM
- `decompile.py --decompile` - Run m2c
- `decompile.py --full` - Do everything
- `decompile.py --ghidra` - Show Ghidra's decompilation

The new tool handles all the tedious parts automatically!

---

For historical reference, the old documentation is below...
