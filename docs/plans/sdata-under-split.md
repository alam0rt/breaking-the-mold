---
title: "Sdata/Data Under-Split Plan: per-TU small-data ownership"
category: plans
tags: [plans, splat, sdata, data, gp-rel, linker, migration]
---

# Sdata / Data Under-Split Plan

**Created:** July 4, 2026
**Status:** Draft / not started
**Related:** `docs/architecture/compilation-units.md` §5 (split-correctness audit),
`docs/plans/rodata-sdata-migration.md` (the rodata sibling — SOLVED),
memories `gp-rel-small-global-d800a595c`, `rodata-migration-mechanism`.

---

## Problem

`.sdata` and `.data` are each a **single pooled asm blob**, not split per owning
translation unit:

```
SLES_010.90.jsonnet:
  data('80FEC', 'data'),    # -> build/asm/data/80FEC.data.o(.data)   (~365 syms)
  data('96154', 'sdata'),   # -> build/asm/data/96154.sdata.o(.sdata) (~450 syms)
```

Every game TU that touches a small global reaches into the blob via the
**tentative-def + `--use-comm-section` gp_rel trick** (10 TUs today: finn, gamecd,
menu, bosses, pickups, enemies, …; 376 distinct `%gp_rel` symbols referenced). The
c-segments' own `.sdata` are all empty (`objdump`: `playst/enemies/layer .sdata =
0x0`) — their small globals are declared as uninitialized tentative-def commons
that the linker merges into the blob's strong defs.

This produces **correct bytes**, but:

1. It is the one place the split forces unnatural C (tentative defs everywhere)
   instead of natural per-TU definitions.
2. It **fails outright for initialized small data.** `D_800A595C = .word
   D_8009AE58` cannot be a tentative def (that adds storage and shifts the whole
   sdata segment by 8), and a plain `extern` emits absolute `lui/%lo` instead of
   gp_rel. So `FindLayerSlot` and ~8 siblings stay shelved. This is the concrete
   payoff of fixing the under-split.

## How the canonical splat workflow does it (Soul Reaver reference)

The mature Soul Reaver PSX decomp (`fmil95/soul-re`, SLUS-007.08) and the splat
General-Workflow wiki both split data per file, using the **dot-prefix convention**:

- **non-dotted** `data` / `sdata` / `rodata` / `bss` → disassembled, kept as `.s`.
- **dot-prefixed** `.data` / `.sdata` / `.rodata` / `.sbss` → linked from the
  compiled `.c` object (`build/src/<TU>.o(.sdata)`).
- **named** subsegment (`[0x0C0BA0, .sdata, Game/CAMERA]`) attributes the block to
  a source file; the name matches the `c()` segment. Unnamed = an unowned gap kept
  as asm (`[0x0C1524, sdata] # Unused`).

So the split is expressed **natively in splat config** — no custom post-splat tool.

### Why sdata is *cleaner* than the rodata migration was, but *harder*

- **Cleaner:** no dead-label problem. The rodata jump tables referenced code
  labels (`.L8005xxxx`) that vanish when a function becomes C, which is why that
  migration needed `tools/migrate_rodata.py`. Sdata symbols are referenced **by
  name** (`D_800A595C`, function pointers), so splitting the blob into named
  subsegments is pure config — splat's native mechanism suffices.
- **Harder:** sdata is **gp-relative**. Every symbol's offset from
  `_gp = 0x800A5954` must be preserved exactly. Rodata was absolute (`lui/%lo`), so
  a table could move as long as its bytes were right; an sdata symbol that shifts
  by 4 bytes breaks every `%gp_rel(sym)` that references it.

## The central technical risk (must be resolved in Phase 0/2)

Today the `.ld` links sdata in **two places**:

- **early**, auto-linked per c-segment (`build/src/*.o(.sdata)`, `.ld:79-128`) —
  currently all empty; and
- **late**, the blob (`build/asm/data/96154.sdata.o(.sdata)`, `.ld:339`), which
  holds all the real bytes at their correct gp-offsets.

Because `auto_link_sections` includes `.sdata`, the moment a TU defines a real
small global its `build/src/<TU>.o(.sdata)` becomes non-empty and links at the
**early** position — a *different* gp-offset than the blob piece it replaces. That
shifts the symbol and breaks the match.

**So a naive per-TU `.sdata` flip does NOT preserve offsets.** The migration must
make all sdata (asm pieces + migrated C pieces) link in **one contiguous region in
original link order**. Options to evaluate in Phase 0:

- (a) Drop `.sdata` from `auto_link_sections` and place every TU's sdata explicitly
  as ordered subsegments in the blob region (Soul-Reaver-style), so migrated and
  non-migrated sdata interleave at the correct offsets; or
- (b) Keep auto-link but verify (empirically) that `ld_legacy_generation` places
  each `build/src/<TU>.o(.sdata)` at the TU's declared subsegment offset, not the
  early auto-link block.

This is the make-or-break question; resolve it on a throwaway pilot before any
sweep.

## Phase 0 — Map the blob (evidence, no changes)

Analogous to the rodata xref-anchoring that confirmed the text boundaries.

1. For every symbol in `96154.sdata.s` and `80FEC.data.s`, record: VRAM /
   gp-offset, size, **initialized vs uninitialized**, and **owning TU** — derived
   from which text segment's functions reference it (Ghidra xrefs, or
   `grep %gp_rel`/`%hi/%lo` across `asm/nonmatchings/<tu>/`). Output an ownership
   table (like `docs/architecture/compilation-units.md` §1).
2. Flag **shared** globals (referenced from >1 TU) — these cannot be cleanly
   attributed; they stay in a shared/unnamed segment (e.g. the true engine
   globals: `g_pGameState`, `g_pBlbHeapBase`, gp-8 `D_800A595C`, …).
3. Confirm the blob's symbol order == original link order == TU order (it should,
   being a verbatim dump). Record the exact offset boundaries between TUs.
4. Decide the layout strategy (a) vs (b) above with a **throwaway** experiment on
   one TU: flip its sdata to `.sdata`, define one global in C, `make check`, and
   observe whether the symbol keeps its gp-offset. Revert.

## Phase 1 — Split the blob into per-TU NAMED asm subsegments (byte-neutral)

Pure relabel; still asm, still `build/asm/...`; bytes unchanged.

1. Replace `data('96154','sdata')` with an ordered list of `sdata('<off>','<tu>')`
   subsegments at the Phase-0 boundaries, named to match the `c()` segments; leave
   shared/unowned regions as unnamed `sdata('<off>')`. Same for the `.data` blob.
2. `make config && make extract && make check` → must stay green (this only splits
   one `.s` into several, concatenated in the same order — same bytes). If splat
   emits a jumptable-alignment or ordering warning, fix the boundary.
3. Commit. This alone removes the "one giant blob" and gives every future data
   migration a clean per-TU target — the same relabel Soul Reaver ships.

## Phase 2 — Pilot: migrate one TU's sdata to C (`.sdata`)

Pick a TU with **few** sdata globals and at least one blocked function.

1. Define that TU's small globals in its `.c` (initialized where the blob shows an
   initializer, uninitialized otherwise), **in the exact order and with the exact
   sizes** the blob has, so cc1 emits them at the same gp-offsets. Remove the
   corresponding tentative-defs.
2. Flip its subsegment `sdata('<off>','<tu>')` → `.sdata` (dot) and excise those
   symbols from the asm blob piece.
3. Resolve the two-place-linking risk per Phase 0's decision. `make check` must
   reproduce the gold SHA1; verify each migrated symbol's gp-offset is unchanged
   (`objdump -t build/src/<tu>.o` vs the blob's addresses).
4. Also validate that OTHER TUs' tentative-defs still resolve against the now-C
   strong def (they reference it by name — should link unchanged).

## Phase 3 — Unblock the initialized-data class

The concrete payoff. `D_800A595C` (`.word D_8009AE58`, gp+8) is the exemplar:

1. Define it in its owning TU as an initialized pointer (`void *D_800A595C =
   &D_8009AE58;` or the properly-typed global), with `D_8009AE58` extern (it lives
   in the `.data` blob until that too is split).
2. Migrate that TU's sdata to `.sdata` (Phase 2 recipe). cc1 now emits `lw
   a3,8(gp)` (gp_rel) instead of absolute — the first of `FindLayerSlot`'s two
   blockers clears. (The second, a `beq`/`bne` branch-sense diff, is ordinary
   decomp — hand to the permuter.)
3. Sweep the ~8 other functions shelved on the same initialized-sdata wall.

## Phase 4 — Sweep + the `.data` blob

Apply Phase 1-2 across the remaining TUs, then repeat for the `.data` blob
(`80FEC`, ~365 syms) using the same dot-prefix mechanism. `.data` is absolute-
addressed (not gp-relative), so its offset constraint is looser — closer to the
rodata case. `.bss`/`.sbss` last (splat notes bss is trickier — often discarded
from the final binary).

## Risks

- **gp-offset preservation (the big one).** Sdata is gp-relative; any reordering
  or size change shifts symbols and cascades. Gate every step on `make check`, not
  fdiff. Never migrate a TU whose sdata order/sizes you have not mapped.
- **Two-place sdata linking** (early auto-link vs late blob) — Phase 0 blocker;
  may require dropping `.sdata` from `auto_link_sections` and explicit ordering.
- **Shared globals** can't be cleanly attributed — keep them in a shared unnamed
  segment; do not force a TU owner.
- **init vs uninit placement:** initialized → `.sdata`; uninitialized → `.sbss`/
  COMMON. Mixing them up shifts the section.
- **`-G8` threshold + `--use-comm-section`** must stay consistent; a global that
  slips above 8 bytes stops being small data and loses gp_rel.
- Not a byte-changing effort — every phase must reproduce the gold SHA1
  (`5a14b65c`). Relabels (Phase 1) are byte-neutral; migrations (Phase 2+) move a
  symbol's *provider* but not its address.

## Relationship to the rodata migration

The rodata migration (`tools/migrate_rodata.py`) needed a **custom post-splat
patch** because splat re-emits pooled jump tables with dead code-label references.
Sdata has no such problem — use splat's **native** named `sdata`→`.sdata`
subsegments instead. If Phase 0 confirms the two-place-linking fix, the entire
sdata/data split is config-only (jsonnet), no new tooling.

## Success criteria

- [ ] Phase 0 ownership table committed; two-place-linking strategy decided on a
      throwaway pilot.
- [ ] `.sdata`/`.data` blobs split into per-TU named asm subsegments, `make check`
      green (Phase 1).
- [ ] One TU fully migrated to `.sdata` with gp-offsets preserved (Phase 2).
- [ ] `FindLayerSlot` (+ initialized-sdata siblings) un-shelved (Phase 3).
- [ ] Tentative-def gp_rel tricks removed from migrated TUs (natural definitions).
