---
title: "Decompilation Process Improvements Summary"
category: root
tags: [decompilation, improvements, summary]
---

# Decompilation Process Improvements Summary

**Date**: January 15, 2026  
**Context**: After attempting directory reorganization and discovering issues

## What Was Improved

### 1. Enhanced Validation in decompile.py

**Added**:
- ✅ Segment overlap detection - prevents creating segments that overlap with existing ones
- ✅ ASM file verification after splat runs - catches missing files immediately
- ✅ Clear error messages explaining WHY things failed, not just THAT they failed
- ✅ Warnings about function size issues and control flow problems

**Code Changes**:
```python
# Before: Silent failure, user discovers later during build
run_splat()

# After: Immediate verification with helpful error messages
run_splat()
verify_asm_generated(name)  # Fails fast if ASM missing
```

### 2. Critical Bugs Fixed

**Bug 1: Hex Strings vs Integers**
```yaml
# ❌ Wrong - causes "ValueError: invalid literal for int()"
- ["0x161D4", "c", "UpdateInputState"]

# ✅ Correct - splat requires integers
- [90580, "c", "UpdateInputState"]
- [0x161D4, c, UpdateInputState]  # YAML hex literal (also works)
```

**Solution**: Script now uses integers directly, not string formatting

**Bug 2: No validation of segment boundaries**
- Script would create overlapping segments silently
- Build would fail much later with cryptic errors
- Now detected immediately with clear explanation

### 3. Documentation Improvements

**New Troubleshooting Section** (`docs/DECOMPILATION_WORKFLOW.md`):
- Common error messages explained
- Root causes identified
- Step-by-step fixes provided
- Examples of what works vs what doesn't

**Source Structure Plan** (`docs/SOURCE_STRUCTURE_PLAN.md`):
- Documents WHY directory reorganization is risky
- Explains binary matching requirements
- Provides safe migration strategy (incremental, post-100%)
- Lists all discovered issues with reorganization attempt

### 4. Process Robustness

**Before**:
1. Run command
2. Wait for splat
3. Wait for build
4. Discover error 5 minutes later
5. No clue what went wrong

**After**:
1. Run command
2. Validate immediately after each step
3. Clear error with explanation if something fails
4. User knows exactly what to fix
5. Time to feedback: seconds instead of minutes

## Key Lessons Learned

### 1. Binary Matching is Paramount
- ANY file reorganization breaks SHA1 matching
- Linker order depends on file paths
- Cannot reorganize until 100% decompiled OR willing to abandon matching

### 2. Splat Has Specific Requirements
- Integer offsets only (not hex strings)
- INCLUDE_ASM paths must match segment names exactly
- Subdirectories ARE supported but require path updates everywhere

### 3. Function Size Detection is Unreliable
- Ghidra may report wrong sizes
- Control flow can extend beyond boundaries
- Always verify before splitting segments

### 4. Fail Fast Philosophy
- Validate immediately after each operation
- Don't wait for build to discover configuration errors
- Provide actionable error messages, not just "failed"

### 5. Documentation is Critical
- Future maintainers need to understand WHY decisions were made
- Common errors should be documented with solutions
- Process improvements should be captured as they're discovered

## Recommendations

### For Current Decompilation Work

1. **Keep flat directory structure** until 100% decompiled
2. **Use decompile.py** for all new functions (handles validation)
3. **Document issues** as you discover them
4. **Test incrementally** - one function at a time
5. **Commit after each matching function** to avoid losing progress

### For Future (Post-100% Decompilation)

1. **Consider modern build system** that doesn't depend on link order
2. **Reorganize freely** once binary matching is no longer required
3. **Automated testing** to catch regressions
4. **Module system** to enforce clean boundaries

## Files Updated

- `scripts/decompile.py` - Added validation, better error messages
- `docs/DECOMPILATION_WORKFLOW.md` - Added troubleshooting section
- `docs/SOURCE_STRUCTURE_PLAN.md` - Documented migration risks
- `docs/DECOMPILATION_IMPROVEMENTS_SUMMARY.md` - This file

## Testing Status

- ✅ Validation logic tested with UpdateInputState case
- ✅ Error messages verified for clarity
- ✅ Integer vs string fix confirmed
- ⚠️ Full workflow not tested end-to-end (waiting for safe test case)

## Next Steps

1. Test improved decompile.py with a simple function (GetMovieUnknown00)
2. Verify all error paths work as expected
3. Update QUICK_REFERENCE.md if needed
4. Consider adding `--verify` flag to test splat config without changing files
