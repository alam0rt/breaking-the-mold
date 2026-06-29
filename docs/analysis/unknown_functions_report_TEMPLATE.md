---
title: "Ghidra Unknown Functions Report"
category: analysis
tags: [analysis, unknown, functions, report, template]
---

# Ghidra Unknown Functions Report

**Generated**: 2026-01-15T14:10:31.946996

**Status**: TEMPLATE - Run with Ghidra MCP server for actual data

## Instructions

1. Start Ghidra with MCP server: `ghidra -run GhidraBridge`
2. Open SLES_010.90 project
3. Re-run this script
4. Review generated unknown_functions.txt
5. Merge verified symbols into symbol_addrs.txt

## Expected Output Structure

### By Category

#### Graphics/Rendering (0x80010000-0x80020000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### Entity System (0x80020000-0x80030000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### MDEC/Movie (0x80030000-0x80040000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### CD/BLB Loading (0x80038000-0x8003A000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### Player System (0x80050000-0x80070000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### Level/Asset Loading (0x80070000-0x80080000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### Audio System (0x8007C000-0x8007D000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### Entity Types (0x80080000-0x80083000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

#### Main/System (0x80082000-0x80090000)

- Functions would be listed here
- With xref counts and sizes
- Prioritized by importance

### Priority Queue

High-priority functions for decompilation:

1. Functions with 10+ callers
2. Functions > 500 bytes
3. Entity System / Player System functions
4. Functions near known symbols

### Decompilation Workflow

```bash
# For each unknown function:
# 1. Use m2c to decompile
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
    asm/pal/nonmatchings/Unknown_Function.s

# 2. Create C file in src/
# 3. Add to symbol_addrs.txt
# 4. Update Ghidra with proper name
# 5. Add comprehensive plate comment
```

