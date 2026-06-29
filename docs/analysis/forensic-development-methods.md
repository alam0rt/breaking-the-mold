---
title: "Forensic Analysis: Skullmonkeys Development Methods"
category: analysis
tags: [analysis, forensic, development, methods]
---

# Forensic Analysis: Skullmonkeys Development Methods

Inferred purely from the decompiled C (`docs/ghidra/SLES_010.90.c`, ~64k lines) plus
secondary sources (`src/Game/RENDER.c`, `include/common.h`, `symbol_addrs.txt`).
Anything marked *inferred* is a best-guess reconstruction, not a citation from
published documentation.

---

## 1. Compiler & Toolchain

**Compiler: GCC 2.7.2 PSX (confirmed).**

Evidence:
- Register allocation patterns (preference for `$v0`/`$v1`, aggressive `$s0-$s7`
  reuse, conservative stack frames) match GCC 2.7.2 output, not 2.5.7.
- Struct padding follows 2.7.2's alignment rules (4-byte for 32-bit members,
  no 8-byte alignment for doubles since there are no doubles).
- Prologue/epilogue shape (`addiu $sp, -N` / `sw $ra, K($sp)` with
  post-increment register save order) is 2.7.2-characteristic.
- Matches the byte-match invariant in `CLAUDE.md` — `tools/gcc-2.7.2-psx/cc1`
  is the toolchain that reproduces the ROM.

**Assembler: ASPSX 2.86** (Sony's PSY-Q assembler, emulated by `maspsx`).

Evidence:
- GP-relative addressing with `$gp = 0x800A5954`, `-G8` threshold (8-byte
  symbols become GP-relative).
- Hazard NOP insertion after loads, div-by-zero guards around `div`/`divu`,
  `rem` expansion — all ASPSX quirks replicated in `tools/maspsx/maspsx.py`.

**Language: pure C.** No C++ name mangling anywhere; no vtables in the C++
sense. What looks like polymorphism is hand-rolled function-pointer dispatch
(entity+0x18).

---

## 2. SDK & Standard Library

**Sony PSY-Q SDK** (1997–1998 vintage):

| Library | Usage |
|---------|-------|
| `libcd` | CD streaming, BLB loading, TOC traversal |
| `libgte` | Fixed-point math cop2 access — **lighting only**, not geometry |
| `libgpu` | Ordering Tables, DMA transfers, framebuffer swap |
| `libspu` | SPU voice allocation, ADPCM sample bank upload |
| `libpad` | Controller polling, digital/analog input |
| `libetc` | VSync, hsync, interrupt hooks |
| `libpress` | Pad-sequence recognition (used for cheat codes) |

**No libc**: `memset` is hand-written inline MIPS (see lines ~1890-1920 of the
Ghidra export). No `malloc`/`free` — all heap goes through the custom arena
allocator (see §3).

**No floating-point.** Every physics/camera/animation value is S16.15 or S32.16
fixed-point (0x10000 scale). The PS1 has no FPU; using floats would have been a
performance trap.

---

## 3. Engine Architecture

**Entity system** (the heart of the engine):

- **128-byte base struct** (some variants extend to 256+ bytes)
- **Vtable pointer at entity+0x18** — per-type callback set (init/tick/draw)
- **Global dispatch table at `0x8009d5f8`** — 121 entity types indexed by
  internal ID (after layer-dependent remapping)
- **Component composition via callbacks**, not inheritance. Same tick logic can
  be attached to wildly different entity types.

**Memory management:**

- **Fixed-size arena allocator** (`AllocateFromHeap @ 0x8001XXXX`)
- **No free / no GC** — memory is reclaimed by resetting the arena on level
  transition
- Global heap base at `blbHeaderBufferBase` (see `docs/systems/heap.md`)
- This is typical of PS1-era games: RAM is 2MB, and a bump allocator is fast
  and predictable.

**Rendering:**

- **Ordering Table** (OT) per frame — primitives sorted by Z into a linked list,
  then DMA'd to GPU
- Double-buffered framebuffers at `0x0,0` and `0x0,256` in VRAM
- **Immediate-mode polygon submission** via libgpu macros (not scenegraph)
- Sprite RLE decoder hand-written (see `docs/systems/sprites.md`)

**No scripting layer.** Every AI, cutscene beat, menu transition is a compiled
C state machine. The "behavior table" at `0x8009d5f8` is the closest thing to
a scripting runtime.

---

## 4. Data-Driven vs Code-Driven Split

**Data (in BLB):**
- Level geometry (tile maps, tile collision, spawn tables)
- Entity placements (Asset 501: 24 bytes each, type + position + flags)
- Tile graphics, palettes, palette animation
- Sprites, sprite animations
- Audio samples + SPU settings
- Demo replay data (Asset 700)
- Password tables

**Code (in EXE):**
- All gameplay behavior (121 entity types, ~128 player callbacks)
- State machines for menus, bosses, cutscenes
- Rendering pipeline, collision response, camera logic
- Audio mixing policy

The BLB TOC format is sophisticated (3-level nesting with container/raw
distinction) — the team clearly invested in tooling for content pipeline.
But logic is 100% native code.

---

## 5. Coding Conventions (inferred from symbol names)

**Naming:**
- `CamelCase` function names
- Lifecycle suffixes: `Init*`, `Tick*`, `Draw*`, `*Callback`
- Loader pattern: `Load*FromBLB`, `Parse*Asset`
- Globals: `g_` prefix (e.g. `g_GameFlags`, `g_CurrentLevel`)
- Pointers: `p_` prefix used sparsely (mostly on globals)

**Structure:**
- Per-system callback triplets (Init/Tick/Draw) enforced by dispatcher
- Flat file layout — no deep namespacing, everything lives in a few translation
  units
- Liberal use of `static` for TU-local functions — kept the final symbol table
  small

**Error handling:**
- **Silent fallback everywhere.** Failed BLB parse? Return null, caller uses
  default. Missing asset? Fall through to empty. No asserts in shipped code.
- This is extremely PS1-era — you cannot pop up an error dialog on a console.

---

## 6. Performance Strategies

- **Hand-tuned MIPS assembly** for hot paths:
  - Hand-written `memset`
  - Parts of player physics that the decompiler can't cleanly reconstruct
  - Tile rendering inner loops
- **Lookup tables** for things that would otherwise need runtime math:
  - Palette cycle tables
  - Sprite animation frame tables
  - Trig (sin/cos) as precomputed tables, S16.15 fixed
- **Fixed-point math everywhere** (no FPU on PS1)
- **GTE for lighting**, not geometry — unusual for PS1 but correct for a 2D
  game that still wants lit sprites
- **Static VRAM layout** — no dynamic texture paging, everything planned at
  compile time

---

## 7. Debug Leftovers

- **CD subsystem `printf` calls** shipped in the retail ROM (strings like
  `"CD: …"` in the string table)
- **Hidden debug menu** gated by `g_GameFlags & 0x80` — accessible via cheat
- **22 cheat codes** recognized by libpress pad-sequence matching
- **Hidden credits slideshow** (Asset 700 has the replay data for it)

---

## 8. Team Scale & Timeline (inferred)

- **API consistency** across 100+ files suggests **1–2 core engineers** owned
  the engine + **3–5 content designers** using tooling
- **12–18 month development cycle** — typical of late-PS1 licensed titles
  (1997–1998 window)
- Vendor: **Neversoft / Dreamworks Interactive**, 1998
- Architectural kinship with **The Neverhood (PC, 1996)** — probably shared
  tooling lineage with PS1-specific rewrites for GTE/SPU/OT

---

## 9. Platform-Specific Patterns

- **CD streaming**: `TickCDStreamBuffer` polls a ring buffer filled by libcd
  async reads. Audio and demo data stream this way.
- **Static VRAM allocation** — tile pages, sprite pages, palette CLUTs all
  assigned fixed slots by BLB Asset 502 definitions
- **No MIDI/SEQ audio** — only streamed ADPCM samples (Asset 601/602 via SPU)
- **OT per frame** — standard PS1 rendering idiom
- **VBlank-driven tick** — 50Hz PAL / 60Hz NTSC, one tick per frame

---

## 10. Code Maturity Assessment

| Dimension | Grade | Notes |
|-----------|-------|-------|
| Architecture | A | Callback composition + TOC data pipeline is clean |
| Optimization | A | Hand assembly where it counts, fixed-point throughout |
| Consistency | A | API naming and call patterns hold across 100+ files |
| Error handling | D | Silent fallback everywhere; no asserts in retail |
| Maintainability | C | No docs in code; 121-entity table is tribal knowledge |
| Build system | B (inferred) | PSY-Q makefiles were standard; exact setup unknown |

---

## 11. Summary

Skullmonkeys is a textbook late-PS1 licensed 2D platformer:

- **GCC 2.7.2 + PSY-Q** toolchain (confirmed by byte-match invariant)
- **Monolithic C** with hand-rolled vtables for entity polymorphism
- **Fixed-point math, no floats** (PS1 constraint)
- **BLB asset archive** with strong TOC-based pipeline — significant tooling
  investment
- **No scripting** — pure compiled state machines
- **Arena allocator, silent errors, lookup tables** — all standard era idioms
- **Small core team** vibe from API consistency; large content team from asset
  volume

The code is more sophisticated than it first appears (the entity dispatch table
and BLB format are real engineering), but the simplicity of the runtime loop
(tick → collide → render) is what keeps it fast enough to hit 50/60fps on PS1.

---

## Related Documentation

- [`../DECOMP_STRATEGY.md`](../DECOMP_STRATEGY.md) — dependency-ordered decomp plan
- [`../SYSTEMS_INDEX.md`](../SYSTEMS_INDEX.md) — per-system documentation index
- [`../blb/README.md`](../blb/README.md) — BLB asset archive format
- [`../reference/player-callback-inventory.md`](../reference/player-callback-inventory.md)
  — full player callback worklist
