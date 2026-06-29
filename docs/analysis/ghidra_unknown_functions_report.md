---
title: "Ghidra Unknown Functions Report"
category: analysis
tags: [analysis, ghidra, unknown, functions, report]
---

# Ghidra Unknown Functions Report

**Generated:** 2026-01-15 14:15:00  
**Ghidra Project:** skullmonkeys (SLES_010.90)  
**Total Functions:** 1,600  
**Known Functions:** 217 (from symbol_addrs.txt)  
**Unknown Functions:** ~800  

---

## Executive Summary

This report identifies unknown functions in the Skullmonkeys PSX binary that need to be analyzed, decompiled, and documented. Functions are categorized by memory region and prioritized by their importance to the game's core systems.

### Statistics

| Category | Count | Percentage |
|----------|-------|------------|
| GTE/PSY-Q Library | ~350 | 22% |
| Known Game Functions | 220 | 14% |
| **Unknown Game Functions** | **~800** | **50%** |
| Entity Type Init Callbacks | ~120 | 7.5% |
| Player State Callbacks | ~80 | 5% |
| Generic Callbacks | ~30 | 2% |

---

## Priority 1: Core Game Systems (High Impact)

### Graphics/Rendering System (0x80010000 - 0x80020000)

**Unknown Functions (20)**

| Address | Name | Est. Size | Priority | Notes |
|---------|------|-----------|----------|-------|
| 0x800138f0 | FUN_800138f0 | ~450 | HIGH | Graphics system init |
| 0x80013ab0 | FUN_80013ab0 | ~108 | HIGH | VRAM management |
| 0x80013d10 | FUN_80013d10 | ~576 | HIGH | Tile rendering setup |
| 0x80013f50 | FUN_80013f50 | ~808 | HIGH | Complex rendering |
| 0x800143a4 | FUN_800143a4 | ~76 | MEDIUM | Utility function |
| 0x80014854 | FUN_80014854 | ~276 | HIGH | Heap/memory related |
| 0x80014968 | FUN_80014968 | ~128 | MEDIUM | Unknown purpose |
| 0x800149e8 | FUN_800149e8 | ~180 | MEDIUM | Unknown purpose |
| 0x80014a9c | FUN_80014a9c | ~604 | HIGH | Large unknown |
| 0x80014cf8 | FUN_80014cf8 | ~896 | HIGH | Very large, important |
| 0x80015074 | FUN_80015074 | ~80 | LOW | Small utility |
| 0x800150c4 | FUN_800150c4 | ~112 | LOW | Small utility |
| 0x80015134 | FUN_80015134 | ~752 | HIGH | Large unknown |
| 0x80015424 | FUN_80015424 | ~16 | LOW | Tiny function |
| 0x80015434 | FUN_80015434 | ~8 | LOW | Tiny function |
| 0x80015614 | FUN_80015614 | ~2568 | **CRITICAL** | Massive function |
| 0x8001889c | FUN_8001889c | ~1344 | **CRITICAL** | Very large |
| 0x800195b0 | FUN_800195b0 | ~140 | MEDIUM | Unknown purpose |
| 0x8001963c | FUN_8001963c | ~20 | LOW | Tiny function |
| 0x80019650 | FUN_80019650 | ~136 | MEDIUM | Unknown purpose |

**Decompilation Targets:** Start with FUN_80015614 (2568 bytes) and FUN_8001889c (1344 bytes).

### Entity System (0x80020000 - 0x80030000)

**Unknown Functions (15)**

| Address | Name | Est. Size | Priority | Notes |
|---------|------|-----------|----------|-------|
| 0x8002086c | FUN_8002086c | ~68 | MEDIUM | Entity helper |
| 0x800224d0 | FUN_800224d0 | ~552 | HIGH | Entity list management |
| 0x80023194 | FUN_80023194 | ~556 | HIGH | Unknown system |
| 0x8002453c | FUN_8002453c | ~128 | MEDIUM | Cleanup function |
| 0x800255c8 | FUN_800255c8 | ~104 | MEDIUM | Level loading related |
| 0x80025630 | FUN_80025630 | ~52 | LOW | Small function |
| 0x800275e4 | FUN_800275e4 | ~996 | **CRITICAL** | Large unknown |
| 0x800279c8 | FUN_800279c8 | ~56 | LOW | Small function |
| 0x8002bde8 | FUN_8002bde8 | ~164 | MEDIUM | Menu/HUD related |
| 0x8002be8c | FUN_8002be8c | ~7956 | **CRITICAL** | Massive, unknown |

**Decompilation Targets:** FUN_8002be8c (7956 bytes - likely menu system), FUN_800275e4 (996 bytes).

### Player System (0x80050000 - 0x80070000)

**Player State Callbacks (80+)**

Many player state callbacks are still unknown. Examples:

| Address | Name | Est. Size | State | Priority |
|---------|------|-----------|-------|----------|
| 0x80059e3c | FUN_80059e3c | ~492 | Unknown | HIGH |
| 0x8005a028 | FUN_8005a028 | ~496 | Unknown | HIGH |
| 0x8005a3f8 | FUN_8005a3f8 | ~460 | Unknown | HIGH |
| 0x8005a5c4 | FUN_8005a5c4 | ~108 | Unknown | MEDIUM |
| 0x8005ad54 | FUN_8005ad54 | ~632 | Unknown | HIGH |
| 0x8005afcc | FUN_8005afcc | ~504 | Unknown | HIGH |
| 0x8005b1c4 | FUN_8005b1c4 | ~272 | Unknown | HIGH |
| 0x8005b2d4 | FUN_8005b2d4 | ~252 | Unknown | HIGH |
| 0x8005c1c4 | FUN_8005c1c4 | ~572 | Unknown | HIGH |
| 0x8005ca60 | FUN_8005ca60 | ~548 | Unknown | HIGH |
| 0x8005cf24 | FUN_8005cf24 | ~420 | Unknown | HIGH |
| 0x8005f29c | FUN_8005f29c | ~312 | Unknown | HIGH |
| 0x8005f694 | FUN_8005f694 | ~416 | Unknown | HIGH |
| 0x8006209c | FUN_8006209c | ~552 | Unknown | HIGH |

**Decompilation Strategy:** Use runtime traces from `game_watcher.lua` to identify which states these belong to.

---

## Priority 2: Gameplay Systems (Medium Impact)

### Particle/Effects System (0x80030000 - 0x80040000)

| Address | Name | Est. Size | Priority | Notes |
|---------|------|-----------|----------|-------|
| 0x80030e54 | FUN_80030e54 | ~2936 | HIGH | Large particle system |
| 0x800313cc | FUN_800313cc | ~7840 | **CRITICAL** | Massive effects code |
| 0x8003286c | FUN_8003286c | ~4816 | **CRITICAL** | Large effects |
| 0x800346f4 | FUN_800346f4 | ~416 | MEDIUM | Effect helper |
| 0x80034a54 | FUN_80034a54 | ~356 | MEDIUM | Effect helper |
| 0x800362a4 | FUN_800362a4 | ~1012 | HIGH | Effect rendering |
| 0x80036698 | FUN_80036698 | ~2876 | HIGH | Complex effects |
| 0x800371d4 | FUN_800371d4 | ~1596 | HIGH | Large effect |
| 0x80037810 | FUN_80037810 | ~720 | HIGH | Effect system |
| 0x80037ae0 | FUN_80037ae0 | ~596 | HIGH | Effect system |
| 0x80037d34 | FUN_80037d34 | ~760 | HIGH | Effect system |
| 0x8003802c | FUN_8003802c | ~724 | HIGH | Effect system |
| 0x80038300 | FUN_80038300 | ~1756 | HIGH | Large effect |

### CD/Streaming System (0x80038000 - 0x8003a000)

| Address | Name | Est. Size | Priority | Notes |
|---------|------|-----------|----------|-------|
| 0x80038cac | FUN_80038cac | ~352 | MEDIUM | CD utility |
| 0x80038e0c | FUN_80038e0c | ~68 | LOW | Small CD function |
| 0x80039c4c | FUN_80039c4c | ~148 | MEDIUM | CD helper |
| 0x80039ce0 | FUN_80039ce0 | ~252 | MEDIUM | CD state |
| 0x80039ddc | FUN_80039ddc | ~128 | MEDIUM | CD utility |
| 0x80039e5c | FUN_80039e5c | ~320 | MEDIUM | CD processing |
| 0x80039f9c | FUN_80039f9c | ~8 | LOW | Tiny function |
| 0x80039fa4 | FUN_80039fa4 | ~128 | MEDIUM | CD helper |
| 0x8003a024 | FUN_8003a024 | ~280 | MEDIUM | CD processing |
| 0x8003a13c | FUN_8003a13c | ~144 | MEDIUM | CD utility |
| 0x8003a1cc | FUN_8003a1cc | ~152 | MEDIUM | CD helper |
| 0x8003a264 | FUN_8003a264 | ~116 | MEDIUM | CD utility |
| 0x8003a2d8 | FUN_8003a2d8 | ~800 | HIGH | Large CD function |
| 0x8003a5f8 | FUN_8003a5f8 | ~300 | MEDIUM | CD processing |
| 0x8003a724 | FUN_8003a724 | ~176 | MEDIUM | CD helper |
| 0x8003a7d4 | FUN_8003a7d4 | ~480 | HIGH | CD system |
| 0x8003a9b4 | FUN_8003a9b4 | ~444 | HIGH | CD function |
| 0x8003ab70 | FUN_8003ab70 | ~344 | MEDIUM | CD utility |
| 0x8003acc8 | FUN_8003acc8 | ~556 | HIGH | CD processing |
| 0x8003aef4 | FUN_8003aef4 | ~668 | HIGH | Large CD function |

---

## Priority 3: Entity Initialization Callbacks

All 120 entity type init callbacks need documentation. Current format:

```c
void EntityType000_InitCallback(Entity* entity) { /* ... */ }
void EntityType001_InitCallback(Entity* entity) { /* ... */ }
// ... 118 more
```

**Status:** Addresses are known, but implementations need decompilation and sprite ID mapping.

**See:** `docs/systems/entities/entity-types.md` for partial mapping.

---

## Recommended Workflow

### Phase 1: High-Priority Functions (Week 1)
1. `FUN_80015614` @ 0x80015614 (2568 bytes) - Graphics system
2. `FUN_8002be8c` @ 0x8002be8c (7956 bytes) - Menu system
3. `FUN_800313cc` @ 0x800313cc (7840 bytes) - Effects system
4. `FUN_8003286c` @ 0x8003286c (4816 bytes) - Effects system

### Phase 2: Player State Machine (Week 2)
5. Unknown player callbacks (80 functions)
   - Use `game_watcher.lua` to capture runtime behavior
   - Match callback addresses to player states
   - Decompile with m2c
   - Document in `docs/systems/player/`

### Phase 3: Entity System (Week 3)
6. Entity type init callbacks (120 functions)
   - Already have addresses
   - Need sprite ID mappings
   - Decompile batch with m2c
   - Update `docs/systems/entities/entity-types.md`

### Phase 4: Remaining Systems (Week 4+)
7. CD/Streaming functions
8. Particle/Effects functions
9. Misc. gameplay functions

---

## Tools & Commands

### Query Specific Unknown Function
```bash
# Via MCP in Copilot
mcp_ghidra_functions_get(address="0x80015614")
mcp_ghidra_functions_decompile(address="0x80015614")
mcp_ghidra_xrefs_list(to_addr="0x80015614")
```

### Decompile with m2c
```bash
cd /home/sam/projects/btm
python3 tools/m2c/m2c.py --context ctx.c --target mipsel-gcc-c \
  asm/pal/nonmatchings/80015614.s > src/graphics_unknown_80015614.c
```

### Add to symbol_addrs.txt
```
GraphicsUnknownFunction = 0x80015614; /* size:0xA08 */
```

### Update splat.pal.yaml
```yaml
- [0x15614, c, graphics_unknown_80015614]
```

---

## Appendix: Full Unknown Function List

See `build/unrecognized_functions.json` for machine-readable list of all unknown functions with:
- Address
- Name (FUN_*)
- Estimated size
- Cross-reference count
- Memory region

**Total:** ~800 functions requiring analysis.
