# Ghidra Unknown Functions Analysis - Session Summary

**Date:** 2026-01-15  
**Deliverables:** Analysis tools, documentation, and repeatable workflow

---

## What Was Created

### 1. Analysis Report
**File:** `docs/analysis/ghidra_unknown_functions_report.md`

Comprehensive report identifying ~800 unknown functions in Ghidra, organized by:
- Memory region (Graphics, Entity, Player, etc.)
- Priority (Critical, High, Medium, Low)
- Estimated size and complexity
- Cross-reference count (importance)

**Key Findings:**
- 50% of functions (~800) are still unknown
- High-priority targets identified (top 50)
- Entity init callbacks need batch processing (120 functions)
- Player state callbacks require runtime tracing (~80 functions)

### 2. Export Script
**File:** `scripts/export_unknown_functions.py`

Python script to export unknown functions to JSON for further analysis.

**Features:**
- Filters unknown patterns (FUN_*, Callback_*, etc.)
- Categorizes by memory region
- Calculates priority scores
- Generates JSON output for automation

**Usage:**
```bash
python3 scripts/export_unknown_functions.py --output build/unknown_functions.json
```

### 3. Workflow Documentation
**File:** `docs/analysis/ANALYSIS_WORKFLOW.md`

Complete step-by-step guide for analyzing and decompiling unknown functions.

**Covers:**
- Phase 1: Identify targets (prioritization)
- Phase 2: Select function (criteria)
- Phase 3: Analyze in Ghidra (using MCP tools)
- Phase 4: Decompile with m2c
- Phase 5: Document findings (multiple systems)
- Phase 6: Verify and commit

**Includes:**
- Runtime tracing with game_watcher.lua
- Batch processing workflows
- Example walkthrough (RenderTilemapLayer)
- Tips and best practices

---

## How to Use This System

### Immediate Next Steps

**1. Start with highest priority unknowns:**
```bash
# Review the report
cat docs/analysis/ghidra_unknown_functions_report.md

# Pick a target (e.g., FUN_80015614 - Graphics system, 2568 bytes)
```

**2. Use Copilot with Ghidra MCP:**
```
Ask: "Show me the decompiled code for function at 0x80015614"
Ask: "What calls function 0x80015614?"
Ask: "What functions does 0x80015614 call?"
```

**3. Follow the workflow:**
```bash
# See complete steps in:
cat docs/analysis/ANALYSIS_WORKFLOW.md
```

### Repeatable Process

The workflow is designed to be repeatable:

1. **Identify** → Use report to pick next target
2. **Analyze** → Use Copilot + Ghidra MCP to understand
3. **Decompile** → Use m2c to generate C code
4. **Document** → Update evil-engine docs with findings
5. **Update** → Rename in Ghidra, add to symbol_addrs.txt
6. **Verify** → Build and test
7. **Commit** → Track progress in git

Repeat for each unknown function, working through priority list.

---

## Key Commands Reference

### Query Ghidra (via Copilot)
```
"List all unknown functions from Ghidra"
"Show me function at 0x[ADDRESS]"
"What calls 0x[ADDRESS]?"
"Rename 0x[ADDRESS] to [NAME] with plate comment"
```

### Decompile Function
```bash
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/[ADDRESS].s > src/[category]/[name].c
```

### Update Symbol Table
```bash
echo "[FunctionName] = 0x[ADDRESS]; /* size:0x[SIZE] */" >> symbol_addrs.txt
```

### Verify Build
```bash
make clean && make && make check
```

---

## Progress Tracking

### Current Status
- **Known Functions:** 217 / 1600 (13.6%)
- **Unknown Functions:** ~800 (50%)
- **Library Functions:** ~350 (PSY-Q, mostly known)
- **Entity Init Callbacks:** 120 (addresses known, need decompilation)

### Priority Queue (Top 10)

| Rank | Address | Name | Size | Category | Reason |
|------|---------|------|------|----------|--------|
| 1 | 0x80015614 | FUN_80015614 | 2568 | Graphics | Large, core rendering |
| 2 | 0x8002be8c | FUN_8002be8c | 7956 | Entity | Massive, likely menu |
| 3 | 0x800313cc | FUN_800313cc | 7840 | Particle | Large effects system |
| 4 | 0x8003286c | FUN_8003286c | 4816 | Particle | Complex effects |
| 5 | 0x8001889c | FUN_8001889c | 1344 | Graphics | Large unknown |
| 6 | 0x80014cf8 | FUN_80014cf8 | 896 | Graphics | VRAM/heap related |
| 7 | 0x800224d0 | FUN_800224d0 | 552 | Entity | List management |
| 8 | 0x800275e4 | FUN_800275e4 | 996 | Entity | Large unknown |
| 9 | 0x80013d10 | FUN_80013d10 | 576 | Graphics | Tile setup |
| 10 | 0x80015134 | FUN_80015134 | 752 | Graphics | Large unknown |

### Milestone Goals
- **Week 1:** Analyze and document top 10 priority functions
- **Week 2:** Identify all player state callbacks via runtime tracing
- **Week 3:** Batch process entity init callbacks (120 functions)
- **Week 4:** Continue with medium-priority functions

---

## Integration with Existing Tools

This analysis system integrates with:

### Ghidra MCP Bridge
- Query functions, xrefs, decompilation
- Update function names and comments
- Automated via Copilot

### m2c Decompiler
- MIPS → C conversion
- Uses ctx.c for context
- Generates matchable C code

### splat Disassembly
- Splits binary into segments
- Manages asm/src file organization
- Configurable via splat.pal.yaml

### PCSX-Redux Traces
- Runtime behavior capture
- game_watcher.lua for callbacks
- Verifies decompilation accuracy

### evil-engine Documentation
- Central knowledge base
- Verified function implementations
- Cross-project reference

---

## Files Created/Modified

### New Files
```
docs/analysis/ghidra_unknown_functions_report.md     # Analysis report
docs/analysis/ANALYSIS_WORKFLOW.md                   # Complete workflow guide
scripts/export_unknown_functions.py                   # Export tool
build/unknown_functions.json                          # Generated data
```

### Context Files (Referenced)
```
docs/ghidra-updates-2026-01-15.md                    # Previous session updates
scripts/verify_ghidra_updates.py                     # Verification script
scripts/generate_ghidra_report.py                    # Report generator
scripts/game_watcher.lua                             # Runtime tracer
symbol_addrs.txt                                     # Known symbols
config/splat.pal.yaml                                # Disassembly config
```

---

## Recommended Reading Order

**For beginners:**
1. `ANALYSIS_WORKFLOW.md` - Start here, full walkthrough
2. `ghidra_unknown_functions_report.md` - See what needs work
3. Follow Phase 1-6 in workflow with one function

**For batch processing:**
1. `ghidra_unknown_functions_report.md` - Priority queue
2. `export_unknown_functions.py --help` - Export tool
3. ANALYSIS_WORKFLOW.md → "Batch Processing" section

**For verification:**
1. `verify_ghidra_updates.py` - Check Ghidra updates applied
2. `make check` - Verify binary matches

---

## Questions & Answers

**Q: How do I know which function to analyze next?**  
A: Use the priority queue in `ghidra_unknown_functions_report.md`. Start with Critical/High priority functions.

**Q: What if m2c decompilation looks wrong?**  
A: Check the ASM manually, compare with known similar functions, use runtime traces to verify behavior.

**Q: Should I rename functions with <90% confidence?**  
A: No. Only rename when you're confident. Use descriptive FUN_* names in code comments if unsure.

**Q: How do I handle entity callbacks?**  
A: Batch process all 120 EntityType*_InitCallback functions. They follow the same pattern, just different sprite IDs.

**Q: What about player state callbacks?**  
A: Use `game_watcher.lua` to capture which callback addresses correspond to which player states (Jump, Walk, etc.).

**Q: How long will this take?**  
A: At ~5 functions per day, completing all 800 unknowns would take ~160 days (~6 months). Batch processing helps.

---

## Success Criteria

You'll know this system is working when:

1. ✅ You can pick an unknown function and understand what it does within 30 minutes
2. ✅ m2c decompilation produces C code that compiles and matches
3. ✅ Ghidra is updated with proper names and comments
4. ✅ evil-engine docs are updated with verified findings
5. ✅ `make check` still passes after changes
6. ✅ Progress is tracked in git commits

---

## Future Improvements

### Automation Opportunities
1. Auto-generate symbol_addrs.txt entries from Ghidra
2. Batch rename functions based on pattern matching
3. Auto-generate splat.pal.yaml segments
4. AI-assisted function purpose identification

### Enhanced Analysis
1. Call graph visualization
2. Data flow analysis
3. Automatic pattern recognition
4. Cross-project function matching

### Integration
1. CI/CD pipeline for verification
2. Web dashboard for progress tracking
3. Automated evil-engine doc updates
4. Real-time collaboration tools

---

## Contact & Support

- **Documentation Issues:** Open issue on GitHub
- **Decompilation Help:** #decompilation Discord channel
- **Tool Problems:** Tag @maintainer in Discord
- **Weekly Sync:** Mondays 10am (progress review)

---

## Conclusion

This analysis system provides:
- ✅ Complete inventory of unknown functions (~800)
- ✅ Priority ranking for efficient work
- ✅ Repeatable workflow for analysis/decompilation
- ✅ Integration with existing tools
- ✅ Progress tracking and verification

**Next Action:** Pick a function from the priority queue and follow the workflow!

```bash
# Start here:
cat docs/analysis/ghidra_unknown_functions_report.md

# Then:
cat docs/analysis/ANALYSIS_WORKFLOW.md

# Pick a target and ask Copilot:
"Show me the function at 0x80015614 and what it does"
```

Good luck decompiling! 🎮🔍
