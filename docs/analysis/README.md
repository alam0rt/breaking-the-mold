---
title: "Ghidra Function Analysis Documentation"
category: analysis
tags: [analysis]
---

# Ghidra Function Analysis Documentation

This directory contains comprehensive documentation for analyzing and decompiling unknown functions in the Skullmonkeys (SLES_010.90) PSX binary.

## 📁 Files

### Core Documentation

| File | Purpose | Use When |
|------|---------|----------|
| **QUICK_REFERENCE.md** | One-page cheat sheet | Need quick commands |
| **ANALYSIS_WORKFLOW.md** | Complete step-by-step guide | First time or need details |
| **ghidra_unknown_functions_report.md** | Full analysis report | Picking next target |
| **port-decomp-findings.md** | Struct maps, data tables, and name corrections harvested from the `port/decomp/` functional-C reconstructions | Decompiling a still-`INCLUDE_ASM` function; need a struct offset map |

> **Note**: The per-function decompilation workflow is driven by Makefile
> targets, not standalone scripts. See `docs/decompilation-guide.md` and the
> "Common Commands" section of the repo `CLAUDE.md`
> (`make context` / `make decompile FUNC=` / `make diff FUNC=` /
> `make permuter FUNC=`). The older `export_unknown_functions.py` /
> `verify_ghidra_updates.py` / `generate_ghidra_report.py` helper scripts and
> their `*.json` outputs are no longer in the tree.

---

## 🎯 Quick Start

**Never done this before?**
1. Read `QUICK_REFERENCE.md` (5 min)
2. Follow `ANALYSIS_WORKFLOW.md` with one function (30 min)
3. Use `ghidra_unknown_functions_report.md` to pick targets

**Want to batch process?**
1. Read `ANALYSIS_WORKFLOW.md` → "Batch Processing"
2. Run `export_unknown_functions.py` to get JSON
3. Process category (e.g., all entity callbacks)

**Need to verify changes?**
```bash
python3 scripts/verify_ghidra_updates.py
make clean && make && make check
```

---

## 📊 Current Status

Decompilation is now far past the early "unknown functions" phase this
directory was written for. Ground truth (derive yourself; do not trust cached
percentages):

```bash
# Functions still implemented as raw asm (not yet byte-matched to C)
grep -rc INCLUDE_ASM src/ | awk -F: '{s+=$2} END{print s}'   # ~731

# Named function-range symbols
grep -cE '= 0x800[0-9A-Fa-f]{5};' symbol_addrs.txt           # ~2,270
```

As of this writing **~731** functions remain as `INCLUDE_ASM` stubs; the
remainder are already decompiled to matching C across `src/*.c`. The live gap
tracker is [`../KNOWLEDGE_GAPS.md`](../KNOWLEDGE_GAPS.md).

---

## 🔄 Workflow Summary

```
1. IDENTIFY
   ↓
   Pick from ghidra_unknown_functions_report.md
   
2. ANALYZE
   ↓
   Use Copilot: "Show me function at 0x[ADDR]"
   
3. DECOMPILE
   ↓
   m2c asm → C code
   
4. DOCUMENT
   ↓
   evil-engine docs + Ghidra comments
   
5. UPDATE
   ↓
   symbol_addrs.txt + SLES_010.90.yaml
   
6. VERIFY
   ↓
   make check (should still match)
   
7. COMMIT
   ↓
   git commit with details
```

---

## 🛠️ Tools Required

- **Ghidra** with MCP server (localhost:8192)
- **GitHub Copilot** with MCP integration
- **m2c** MIPS decompiler
- **splat** disassembly framework
- **PCSX-Redux** (optional, for runtime tracing)

---

## 📚 Related Documentation

### In btm project:
- `docs/decompilation-guide.md` - General decompilation
- `docs/systems/` - System documentation
- `symbol_addrs.txt` - Known symbols

### In evil-engine project:
- `docs/` - Verified function documentation
- `docs/systems/` - System implementations
- `docs/blb-data-format.md` - BLB file format

---

## 🎓 Learning Path

**Beginner (Week 1):**
1. Read QUICK_REFERENCE.md
2. Analyze 1 small function (<200 bytes)
3. Follow complete workflow once
4. Document what you learned

**Intermediate (Week 2-3):**
1. Analyze 5 medium functions (200B-1KB)
2. Use runtime tracing for callbacks
3. Batch process entity init callbacks
4. Contribute to evil-engine docs

**Advanced (Week 4+):**
1. Tackle large functions (>1KB)
2. Identify complex systems (menu, effects)
3. Create new analysis patterns
4. Improve tooling/automation

---

## 🤝 Contributing

When you analyze a function:

1. **Document it** in evil-engine
2. **Update Ghidra** with proper name/comment
3. **Add to symbol_addrs.txt**
4. **Create git commit** with evidence
5. **Share findings** in Discord

---

## �� Progress Tracking

Track your progress:

```bash
# Count unknown functions remaining
grep -c "FUN_" symbol_addrs.txt

# Count functions decompiled this week
git log --since="1 week ago" --grep="Decompile" --oneline | wc -l
```

---

## ❓ FAQ

**Q: Where do I start?**  
A: Read QUICK_REFERENCE.md, then pick a small function from the report.

**Q: How long does one function take?**  
A: 15-30 minutes for small functions, 1-2 hours for large ones.

**Q: What if I'm stuck?**  
A: Ask in #decompilation Discord, check similar known functions.

**Q: How do I know I'm right?**  
A: Xrefs, runtime traces, and `make check` all help verify.

**Q: Should I work on PSY-Q library functions?**  
A: No, those are already documented. Focus on game code.

---

## 🏆 Milestones

- [x] **Phase 0:** Setup analysis system (2026-01-15)
- [ ] **Phase 1:** Top 10 priority functions (Week 1)
- [ ] **Phase 2:** All player callbacks (Week 2)
- [ ] **Phase 3:** All entity callbacks (Week 3)
- [ ] **Phase 4:** 50% functions known (Q2 2026)
- [ ] **Phase 5:** 75% functions known (Q3 2026)
- [ ] **Phase 6:** 90% functions known (Q4 2026)

---

## 📞 Support

- **Documentation:** This directory
- **Tools:** `scripts/` directory
- **Community:** #decompilation Discord
- **Issues:** GitHub issues

---

**Good luck analyzing! 🎮🔍**
