# Ghidra Unknown Function Analysis Workflow

**Created:** 2026-01-15  
**Purpose:** Document the repeatable process for analyzing and decompiling unknown functions from Ghidra

---

## Overview

This document describes the complete workflow for identifying, analyzing, and decompiling unknown functions in the Skullmonkeys (SLES_010.90) PSX binary using Ghidra with MCP integration.

### Current Status
- **Total Functions:** ~1,600
- **Known Functions:** 217 (in symbol_addrs.txt)
- **Unknown Functions:** ~800 (50%)
- **Priority High/Critical:** ~50 functions

---

## Tools & Setup

### Required Tools
1. **Ghidra** with MCP server running on `localhost:8192`
2. **GitHub Copilot** with MCP integration
3. **m2c** MIPS-to-C decompiler (`tools/m2c/`)
4. **splat** disassembly framework
5. **PCSX-Redux** (optional, for runtime tracing)

### Helper Scripts
- `scripts/export_unknown_functions.py` - Export unknown functions to JSON
- `scripts/generate_ghidra_report.py` - Generate analysis templates
- `scripts/verify_ghidra_updates.py` - Verify Ghidra updates were applied
- `scripts/game_watcher.lua` - Runtime behavior capture (PCSX-Redux)

---

## Step-by-Step Workflow

### Phase 1: Identify Unknown Functions

**1. List all functions in Ghidra:**
```
Ask Copilot: "List all unknown functions from Ghidra"
```

This uses `mcp_ghidra_functions_list()` to get all ~1,600 functions.

**2. Export to JSON for analysis:**
```bash
cd /home/sam/projects/btm
python3 scripts/export_unknown_functions.py --output build/unknown_functions.json
```

**3. Review the report:**
```bash
cat docs/analysis/ghidra_unknown_functions_report.md
```

This shows priority rankings by:
- Memory region (core systems vs utilities)
- Function size (larger = more complex/important)
- Cross-reference count (highly called = critical)

### Phase 2: Select Target Function

**Prioritization criteria:**
1. **Critical systems first** (Entity, Player, Graphics)
2. **Large functions** (>1KB likely core logic)
3. **High xref count** (called from many places)
4. **Unknown player states** (capture with game_watcher.lua)

**Example targets:**
- `FUN_80015614` (2568 bytes) - Graphics system, likely tilemap renderer
- `FUN_8002be8c` (7956 bytes) - Menu system, massive function
- `FUN_800313cc` (7840 bytes) - Particle/effects, complex system

### Phase 3: Analyze in Ghidra

**1. Get function details:**
```
Ask Copilot: "Show me the decompiled code for function at 0x80015614"
```

This uses:
- `mcp_ghidra_functions_get(address="0x80015614")`
- `mcp_ghidra_functions_decompile(address="0x80015614")`

**2. Check cross-references:**
```
Ask Copilot: "What calls the function at 0x80015614?"
```

Uses `mcp_ghidra_xrefs_list(to_addr="0x80015614")` to find callers.

**3. Analyze called functions:**
```
Ask Copilot: "What functions does 0x80015614 call?"
```

Uses `mcp_ghidra_xrefs_list(from_addr="0x80015614")` to find callees.

**4. Identify purpose:**
- Look at called functions (known names give context)
- Check for patterns (loops, conditionals, data structures)
- Compare with evil-engine documentation
- Use game_watcher.lua traces if it's a callback

### Phase 4: Decompile with m2c

**1. Ensure ASM exists:**
```bash
cd /home/sam/projects/btm
make  # Runs splat if needed
```

**2. Decompile function:**
```bash
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/<function_name>.s > /tmp/decompiled.c
```

Example:
```bash
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/80015614.s > /tmp/FUN_80015614.c
```

**3. Review decompiled output:**
- Check for obvious purpose (variable names, called functions)
- Identify data structures used
- Note any global variables accessed

### Phase 5: Document Findings

**1. Document in evil-engine:**

Create or update documentation in `~/projects/evil-engine/docs/`:
```
docs/systems/graphics/tilemap-renderer.md
docs/systems/entity/entity-spawning.md
docs/systems/player/player-states.md
```

Format:
```markdown
## Function: RenderTilemapLayer

**Address:** 0x80015614  
**Size:** 2568 bytes  
**Confidence:** 95%  
**Verified:** Via Ghidra decompilation + runtime tracing  

### Purpose
Renders a 16x16 or 8x8 tilemap layer to the screen using GPU primitives.

### Implementation
<details>
<summary>Decompiled C code</summary>

```c
void RenderTilemapLayer(LayerContext* layer, int offsetX, int offsetY) {
    // ... decompiled code from m2c
}
```
</details>

### Called By
- InitTilemapLayerRendering @ 0x8001601c
- EntityTickLoop @ 0x80020e1c

### Calls
- GetTilePixelData @ 0x8007b588
- CalculateVRAMCoordinates @ 0x80014278
```

**2. Update Ghidra:**

Rename and add plate comment:
```
Ask Copilot: "Rename function at 0x80015614 to RenderTilemapLayer and add a comprehensive plate comment explaining what it does"
```

Uses:
- `mcp_ghidra_functions_rename(address="0x80015614", new_name="RenderTilemapLayer")`
- `mcp_ghidra_functions_set_comment(address="0x80015614", comment="...")`

**3. Update symbol_addrs.txt:**
```bash
cd /home/sam/projects/btm
# Add to symbol_addrs.txt:
echo "RenderTilemapLayer = 0x80015614; /* size:0xA08 */" >> symbol_addrs.txt
```

**4. Update splat.pal.yaml:**
```yaml
# Add segment definition:
- [0x15614, c, graphics/tilemap_renderer]
```

**5. Create C source file:**
```bash
# Create src/graphics/tilemap_renderer.c with decompiled code
cp /tmp/FUN_80015614.c src/graphics/tilemap_renderer.c
```

**6. Verify build:**
```bash
make clean
make
make check  # Should match original if decompilation is correct
```

### Phase 6: Verify & Commit

**1. Verify in Ghidra:**
```bash
python3 scripts/verify_ghidra_updates.py
```

**2. Test build:**
```bash
make clean && make && make check
```

**3. Document update:**

Update `docs/ghidra-updates-YYYY-MM-DD.md`:
```markdown
## RenderTilemapLayer @ 0x80015614

**Old Name:** FUN_80015614  
**New Name:** RenderTilemapLayer  
**Confidence:** 95%  
**Source:** Ghidra decompilation analysis  
**Date:** 2026-01-15  

**Plate Comment:**
Renders a complete tilemap layer (16x16 or 8x8 tiles) to the screen.
Handles scrolling, clipping, and VRAM texture lookups.

Called by: InitTilemapLayerRendering, EntityTickLoop
Calls: GetTilePixelData, CalculateVRAMCoordinates

**Implementation Notes:**
- Uses OT z-index for layering
- Supports both tile sizes via flags
- Clips tiles outside camera view
- Optimized for PSX GPU limitations
```

**4. Git commit:**
```bash
git add symbol_addrs.txt config/splat.pal.yaml src/graphics/tilemap_renderer.c
git add docs/ghidra-updates-2026-01-15.md
git commit -m "Identify and decompile RenderTilemapLayer (0x80015614)"
```

---

## Runtime Tracing for Unknown Callbacks

For unknown player state callbacks and entity callbacks, use runtime tracing:

### 1. Start PCSX-Redux with game watcher
```bash
cd /home/sam/projects/btm
make record LEVEL=1 STAGE=0
```

### 2. Perform specific actions
- For player states: jump, walk, attack, die
- For entities: interact, damage, collect
- For triggers: pass through zones, checkpoints

### 3. Analyze trace log
```bash
cat game_watcher/logs/trace_*.jsonl | jq 'select(.type == "PlayerStateChange")'
```

### 4. Map callback addresses to behaviors
```json
{"frame":1234,"type":"PlayerStateChange","data":{"callback":"0x80067e28","state":"Jump"}}
```

→ Now we know `0x80067e28` is `PlayerState_Jump`

### 5. Update Ghidra and documentation
Follow Phase 5 steps above.

---

## Batch Processing

For processing many functions at once (e.g., all entity init callbacks):

### 1. Create batch list
```bash
# Extract all EntityType*_InitCallback addresses
grep "EntityType.*InitCallback" symbol_addrs.txt | \
  sed 's/.*= 0x\([0-9a-f]\+\).*/\1/' > /tmp/entity_addrs.txt
```

### 2. Process each function
```bash
while read addr; do
    echo "Processing 0x$addr..."
    python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
        asm/pal/nonmatchings/$addr.s > src/entities/init_$addr.c
done < /tmp/entity_addrs.txt
```

### 3. Document in batch
Create a single comprehensive document covering all entity types:
`docs/systems/entities/entity-init-callbacks-complete.md`

---

## Tips & Best Practices

### Analysis
1. **Start with xrefs** - What calls this? What does it call?
2. **Look for patterns** - Loops over arrays? State machines? Rendering?
3. **Check global vars** - Accesses to known structures give context
4. **Use evil-engine docs** - Already verified functions help identify unknowns

### Decompilation
1. **Check m2c output** - May need manual cleanup for complex functions
2. **Match types** - Use types from `include/common.h`
3. **Verify patterns** - PSX code has recognizable patterns (GTE, DMA, OT)
4. **Test build** - `make check` should still pass

### Documentation
1. **Be specific** - Include addresses, sizes, confidence levels
2. **Show evidence** - Link to traces, xrefs, known callers
3. **Update multiple places** - Ghidra + evil-engine + symbol_addrs.txt
4. **Version control** - Document when/why each change was made

### Common Pitfalls
- **Don't guess** - Only rename when confident (90%+)
- **Don't skip verification** - Always test build after changes
- **Don't lose context** - Document *why* you think it's X, not just *that* it's X
- **Don't over-batch** - Process functions individually until you have a good workflow

---

## Example: Complete Function Analysis

Let's walk through analyzing `FUN_80015614`:

### 1. Initial Query
```
Copilot: "Show me the function at 0x80015614 and what it does"
```

**Response:**
- Size: 2568 bytes
- Calls: GetTilePixelData, CalculateVRAMCoordinates, AddPrim
- Called by: InitTilemapLayerRendering
- Uses: RECT structs, ordering table, tile data arrays

### 2. Hypothesis
Based on calls and callers, this looks like a tilemap layer renderer.

### 3. Check evil-engine docs
```bash
grep -r "tilemap" ~/projects/evil-engine/docs/
```

Found: `docs/systems/rendering/tilemap-system.md` describes layer rendering.

### 4. Compare decompilation
```bash
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/80015614.s > /tmp/check.c
```

Decompiled code shows:
- Loops over tile X/Y coordinates
- Calculates screen positions with camera offset
- Loads tile pixels from VRAM
- Adds primitives to OT

**Confidence:** 95% - This is definitely the tilemap layer renderer.

### 5. Update all systems
```bash
# Ghidra
Copilot: "Rename 0x80015614 to RenderTilemapLayer with plate comment"

# symbol_addrs.txt
echo "RenderTilemapLayer = 0x80015614; /* size:0xA08 */" >> symbol_addrs.txt

# splat.pal.yaml
# Add: - [0x15614, c, graphics/tilemap_renderer]

# Create source
cp /tmp/check.c src/graphics/tilemap_renderer.c

# Document
# Update docs/systems/rendering/tilemap-system.md in evil-engine
```

### 6. Verify
```bash
make clean && make && make check
# ✓ Build succeeds, no changes to output binary
```

### 7. Commit
```bash
git add symbol_addrs.txt config/splat.pal.yaml src/graphics/tilemap_renderer.c
git commit -m "Decompile RenderTilemapLayer (0x80015614)

- Verified via Ghidra analysis and m2c decompilation
- Handles 16x16 and 8x8 tile rendering with camera scrolling
- Critical graphics system function (2568 bytes)
- Confidence: 95%"
```

**Done!** Function is now identified, decompiled, documented, and integrated.

---

## Reporting & Tracking

### Progress Tracking
Update `docs/DECOMPILATION_STATUS.md`:
```markdown
## Functions Decompiled: 218 / 1600 (13.6%)

### Recently Added (2026-01-15)
- ✅ RenderTilemapLayer @ 0x80015614 (2568 bytes)
- ✅ EntitySpawnSystem @ 0x80024288 (480 bytes)
```

### Weekly Reports
Generate weekly progress reports:
```bash
python3 scripts/generate_progress_report.py --week 2026-W03
```

### Milestone Goals
- **Q1 2026:** 300 functions decompiled (18%)
- **Q2 2026:** 500 functions decompiled (31%)
- **Q3 2026:** 800 functions decompiled (50%)
- **Q4 2026:** 1200 functions decompiled (75%)

---

## Additional Resources

### Documentation
- `docs/decompilation-guide.md` - General decompilation guide
- `docs/ghidra/ghidra-mcp-usage.md` - Ghidra MCP tool reference
- `docs/analysis/ghidra_unknown_functions_report.md` - Current status report
- `evil-engine/docs/` - Verified function documentation

### Tools
- m2c documentation: `tools/m2c/README.md`
- Splat documentation: Online at mips.rehab/splat
- Ghidra API: localhost:8192/docs (when server running)

### Community
- Discord: #decompilation channel
- GitHub: Open issues for difficult functions
- Weekly sync: Mondays 10am (review progress, blockers)
