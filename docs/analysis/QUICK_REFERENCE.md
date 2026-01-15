# Unknown Function Analysis - Quick Reference

**Last Updated:** 2026-01-15

---

## 🎯 Top Priority Targets

| Address | Size | Category | Why? |
|---------|------|----------|------|
| 0x80015614 | 2.5KB | Graphics | Core tilemap renderer |
| 0x8002be8c | 8KB | Entity | Menu system (huge!) |
| 0x800313cc | 7.8KB | Particle | Effects system |
| 0x8003286c | 4.8KB | Particle | Complex effects |
| 0x8001889c | 1.3KB | Graphics | Large unknown |

---

## 🔍 Analysis Commands

### Query in Copilot
```
"Show me function at 0x[ADDRESS]"
"What calls 0x[ADDRESS]?"
"Decompile 0x[ADDRESS]"
"Rename 0x[ADDRESS] to [NAME]"
```

### Decompile with m2c
```bash
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/[ADDR].s > /tmp/decompiled.c
```

### Update Symbol Table
```bash
echo "[Name] = 0x[ADDR]; /* size:0x[SIZE] */" >> symbol_addrs.txt
```

---

## 📊 Current Status

- **Total Functions:** 1,600
- **Known:** 217 (13.6%)
- **Unknown:** ~800 (50%)
- **Priority:** Top 50

---

## 📚 Documentation

- **Full Workflow:** `docs/analysis/ANALYSIS_WORKFLOW.md`
- **Function Report:** `docs/analysis/ghidra_unknown_functions_report.md`
- **Session Summary:** `docs/analysis/SESSION_SUMMARY_2026_01_15.md`

---

## ✅ Verification Checklist

- [ ] Decompiled with m2c
- [ ] Renamed in Ghidra
- [ ] Updated symbol_addrs.txt
- [ ] Documented in evil-engine
- [ ] `make clean && make && make check` passes
- [ ] Git commit with details

---

## 🚀 Quick Start

```bash
# 1. Pick target from report
cat docs/analysis/ghidra_unknown_functions_report.md

# 2. Analyze in Ghidra (via Copilot)
# Ask: "Show me function at 0x80015614"

# 3. Decompile
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/80015614.s > /tmp/FUN_80015614.c

# 4. Update everything
# - Ghidra (rename via Copilot)
# - symbol_addrs.txt
# - evil-engine docs
# - splat.pal.yaml

# 5. Verify
make clean && make && make check

# 6. Commit
git add -A
git commit -m "Decompile [FunctionName] (0x[ADDR])"
```

---

## 🎮 Runtime Tracing

For unknown callbacks:

```bash
# 1. Start recording
make record LEVEL=1 STAGE=0

# 2. Perform actions (jump, walk, attack, etc.)

# 3. Analyze trace
cat game_watcher/logs/trace_*.jsonl | \
  jq 'select(.type == "PlayerStateChange")'

# 4. Map addresses to states
# {"callback":"0x80067e28","state":"Jump"} → PlayerState_Jump
```

---

## 💡 Tips

- **Start small:** Pick ~500 byte functions first
- **Check xrefs:** What calls/is called tells you a lot
- **Use patterns:** Similar functions often do similar things
- **Runtime verify:** Traces prove your hypothesis
- **Document why:** Not just what, but evidence
- **Build often:** `make check` catches errors early

---

## 📞 Help

- **Stuck?** Ask in #decompilation Discord
- **Tool issues?** Check `ANALYSIS_WORKFLOW.md`
- **Need example?** See "RenderTilemapLayer" walkthrough
