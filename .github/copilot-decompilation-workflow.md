# Decompilation Workflow: Analyzing Function Clusters

This document describes the workflow for analyzing and naming clusters of related functions in the Skullmonkeys decompilation project. Use this process when asked to examine a group of related functions (e.g., "analyze the allocation functions", "name the render primitives", etc.).

## Overview

The goal is to:
1. Identify a cluster of related functions
2. Understand what each function does through decompilation and cross-reference analysis
3. Propose consistent, accurate names based on actual behavior
4. Fix any naming discrepancies in Ghidra
5. Add verified functions to `symbol_addrs.txt`

## Step 1: Identify the Function Cluster

Start by listing functions that match a pattern:

```
mcp_ghidra_functions_list(name_contains="<pattern>")
```

Example patterns:
- `Alloc` - allocation functions
- `Free` - deallocation functions  
- `Init` - initialization functions
- `Render` - rendering functions
- `Entity` - entity system functions

Record the addresses and current names of all functions in the cluster.

## Step 2: Decompile Each Function

For each function in the cluster, get the decompiled code:

```
mcp_ghidra_functions_decompile(address="0x800XXXXX")
```

When analyzing, look for:
- **Pool offsets**: Different pools at different struct offsets suggest different purposes
- **Size constants**: Magic numbers like `0x14`, `0x28` often correspond to struct sizes
- **Max counts**: Limits like `200`, `100` indicate pool capacities
- **Called functions**: What PSY-Q or game functions are called (e.g., `SetSprt`, `SetPolyFT4`)
- **Return types**: What's being returned - pointer, index, boolean?

## Step 3: Cross-Reference Analysis

For each function, find what calls it:

```
mcp_ghidra_xrefs_list(to_addr="0x800XXXXX", limit=30)
```

This reveals:
- **Context of use**: A function called only from `RenderSpriteOrScaledQuad` is clearly for sprite rendering
- **Multiple callers**: Functions with many callers are more "generic"
- **Call chains**: Understanding the call graph helps identify subsystems

## Step 4: Correlate with Known Data Structures

For PSX games, correlate sizes with PSY-Q primitive structures:

| Size | PSY-Q Primitive |
|------|-----------------|
| 0x08 | DR_TPAGE |
| 0x0C | DR_AREA, DR_OFFSET |
| 0x10 | TILE, SPRT_8, SPRT_16 |
| 0x14 | POLY_F3, LINE_G2, SPRT |
| 0x18 | POLY_F4 |
| 0x1C | POLY_FT3, POLY_G3 |
| 0x24 | POLY_FT4 (partial), POLY_G4 |
| 0x28 | POLY_FT4, POLY_GT3 |
| 0x34 | POLY_GT4 |

If unsure of sizes, compile a test program with the struct definitions and `sizeof()`.

## Step 5: Propose Naming Convention

Choose a naming convention that:
1. **Reflects actual behavior** - not guessed purpose
2. **Is consistent across the cluster** - same prefix/suffix pattern
3. **Includes key parameters** - size, type, variant

Good naming patterns:
- `AllocPrimSlot_0xXX` - allocation by size (when size is the key differentiator)
- `AllocPrimSlot_SPRT` - allocation by primitive type (when type is known)
- `InitEntity_<Type>` - entity initialization by type
- `Render<Primitive><Variant>` - rendering by what's rendered

## Step 6: Verify and Fix Discrepancies

Compare the proposed name against the actual code:

**Example discrepancy found:**
```
Function: AllocPrimSlot_0x20 @ 0x800138A0
Code shows: element_size = 0x24, not 0x20!
Action: Rename to AllocPrimSlot_0x24
```

Fix in Ghidra:
```
mcp_ghidra_functions_rename(address="0x800138A0", new_name="AllocPrimSlot_0x24")
```

Add comments explaining the function:
```
mcp_ghidra_functions_set_comment(address="0x800138A0", 
    comment="Allocates 0x24-byte primitive slot (POLY_G4). Pool at +0x422c, max 100 entries.")
```

## Step 7: Document Findings

Create a summary table:

| Address | Name | Size | Max | Pool Offset | Used For |
|---------|------|------|-----|-------------|----------|
| 0x800136C8 | AllocPrimSlot_0x14 | 0x14 | 200 | +0x74 | SPRT, POLY_F3 |
| 0x80013718 | AllocPrimSlot_0x08 | 0x08 | 200 | +0x1018 | DR_TPAGE |
| ... | ... | ... | ... | ... | ... |

## Step 8: Add to symbol_addrs.txt

Only add functions that are:
1. **Verified** - name matches actual behavior
2. **Well-understood** - you can explain what it does
3. **Properly named** - follows project conventions

Format:
```
// =============================================================================
// <Subsystem> - <Description>
// =============================================================================
FunctionName = 0x800XXXXX; // size:0xYY // Brief description
```

## Example: Primitive Allocation Cluster

### Input
"Analyze the primitive allocation functions after GetFrameReadyFlag"

### Process

1. **List functions**: Found 7 `AllocPrimSlot_*` functions
2. **Decompile each**: All follow same pattern - allocate from pool, return pointer
3. **Cross-reference**: Called from `RenderSpriteOrScaledQuad`, `SubmitPrimitiveBufferToGPU`
4. **Correlate sizes**: Matched to PSY-Q primitives
5. **Found discrepancy**: `AllocPrimSlot_0x20` actually allocates 0x24 bytes
6. **Fixed in Ghidra**: Renamed to `AllocPrimSlot_0x24`

### Output

| Function | Size | Primitive Type |
|----------|------|----------------|
| AllocPrimSlot_0x14 | 0x14 | SPRT (used in sprite path) |
| AllocPrimSlot_0x08 | 0x08 | DR_TPAGE |
| AllocPrimSlot_0x28 | 0x28 | POLY_FT4 |
| AllocPrimSlot_0x18 | 0x18 | POLY_F4 |
| AllocPrimSlot_0x1C | 0x1C | POLY_G3 |
| AllocPrimSlot_0x24 | 0x24 | POLY_G4 (was misnamed 0x20) |

## Tips

- **Batch decompile**: Call multiple `mcp_ghidra_functions_decompile` in parallel for efficiency
- **Check both callers and callees**: `xrefs_list(to_addr=...)` and `xrefs_list(from_addr=...)`
- **Trust the code, not the name**: Original names may be wrong or auto-generated
- **Document uncertainties**: If unsure, note it in comments rather than guessing
- **Small clusters first**: Start with 5-10 functions, expand as patterns emerge

## When to Stop

Stop analyzing when:
1. All functions in the cluster are understood and named
2. You've documented findings in a summary table
3. Ghidra has been updated with correct names and comments
4. The user has reviewed and approved additions to `symbol_addrs.txt`

Always verify the build still matches after any changes:
```bash
make clean && make check
```
