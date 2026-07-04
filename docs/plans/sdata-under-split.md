---
title: "Sdata/Data Under-Split Plan: per-TU small-data ownership"
category: plans
tags: [plans, splat, sdata, data, gp-rel, linker, migration]
---

# Sdata / Data Under-Split Plan

**Created:** July 4, 2026
**Status:** Phase 0 + Phase 1 complete (byte-matching). Phase 4 sweep done for the
large single-owner sdata pieces (blb, pickups, player, finn, vehicle, enemies,
bosses, playst — all byte-matching). `.data` blob + `.bss`/`.sbss` and Phase 3
(shared `D_800A595C`) pending — see Progress log.
**Related:** `docs/architecture/compilation-units.md` §5 (split-correctness audit),
`docs/analysis/sdata-ownership.md` (the committed Phase 0 ownership table),
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

### `.data` blob — pilot done, but materially harder than `.sdata`
The `.data` blob (`data('80FEC')`, 0x800907EC..0x800A5953) is NOT a
straightforward extension of the sdata sweep. Ownership analysis
(`python3 tools/map_sdata_ownership.py data`) reports **365 symbols: 269
single-owner, 18 shared, 78 unreferenced**, and — critically — they are heavily
**interleaved by TU** (e.g. blb tables, then an `effects` symbol at 0x8009B144,
then more blb) and mixed in size (2-byte shorts sitting next to KB-scale tables).
New failure modes vs sdata:
- **`-G8` moves tiny initialized globals to `.sdata`.** A ≤8-byte `.data` symbol
  (e.g. the 2-byte `D_8009B14C`) defined as a plain C global lands in `.sdata`,
  shifting its address. Keep such symbols large (arrays >8 bytes) or leave them
  in asm.
- **All-zero `.data` symbols sink to `.bss`/COMMON.** The 42KB `D_800907EC` and
  480B `D_8009AE58` are zero-fills; an uninitialized (or even `= {0}`) C def can
  land in bss and shift everything. Leave zero-fills in asm.
- **78 unreferenced symbols** can't be attributed to a TU (referenced only via
  pointers inside other data) — keep them in shared asm pieces.
- **Interleaving** means a single-owner contiguous run rarely spans a whole TU;
  carve per-run, not per-TU.

**`.data` recipe (validated by the pilot below):**
1. `local dotdata(start, name) = subsegment(start, '.data', name);` (added).
2. Carve one contiguous single-owner run out of the pooled blob: keep
   `data('80FEC', 'data')` (now ends at the run start), insert
   `dotdata('<runoff>', '<tu>')`, then `data('<nextoff>', 'data')` for the
   remainder. splat auto-splits the blob at these offsets; the dot-prefix `.data`
   subsegment links `build/src/<tu>.o(.data)` at the run position and is removed
   from the early auto-link block (same "option b" as sdata).
3. Define the symbol(s) in the TU's `.c` with a type **compatible with existing
   consumers** and **>8 bytes** so `-G8` keeps them in `.data`. Non-zero content
   only.
4. `make config && make extract && make check` — gate on the gold SHA1.

**Pilot (byte-matching `5a14b65c`):** `D_8009C11C` (owner bosses, 88-byte radius-63
cos/sin `(x,y)` table read by `MaskAngleCosineEntry`). bosses.c already had
`extern u8 D_8009C11C[]` used with u8 negative-index arithmetic, so it was defined
as `u8 D_8009C11C[88] = { <little-endian bytes> }` at EOF (u8[] to keep the
consumer's addressing/stride identical; s16[] would change the pointer stride and
break the match). Carved via `dotdata('8C91C','bosses')` between the clayball
(`D_8009C0F4`) and player (`D_8009C174`) neighbours. First-try match.

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

- [x] Phase 0 ownership table committed (`docs/analysis/sdata-ownership.md`);
      two-place-linking strategy decided on a throwaway pilot.
- [x] `.sdata` blob split into per-TU named asm subsegments, `make check`
      green (Phase 1). *(`.data` blob split deferred to Phase 4.)*
- [x] One TU fully migrated to `.sdata` with gp-offsets preserved (Phase 2 —
      `blb`, byte-matching; see Progress log).
- [ ] `FindLayerSlot` (+ initialized-sdata siblings) un-shelved (Phase 3).
- [ ] Tentative-def gp_rel tricks removed from migrated TUs (natural definitions).

## Progress log

### Phase 0 — DONE (evidence + make-or-break pilot)

- **Ownership table:** `docs/analysis/sdata-ownership.md` (regenerate with
  `python3 tools/map_sdata_ownership.py sdata`). 454 sdata symbols: **428
  single-owner, 20 shared (>1 TU), 6 unreferenced**. Blob symbol order == VRAM
  order == link order (verbatim dump); TU boundaries are contiguous runs.
- **Two-place-linking risk RESOLVED — option (b) works.** With
  `ld_legacy_generation`, a **dot-prefix** `.sdata` subsegment declared at its
  late YAML offset links ONLY at the correct late position (the gp region near
  `main_SDATA_END`), and is automatically removed from the early
  `main_SDATA_START` auto-link block. Pilot ld showed `build/src/level.o(.sdata)`
  appearing once, between the two asm blob pieces. **No need to drop `.sdata`
  from `auto_link_sections`.**
- **CRITICAL addressing-mode constraint (refines Problem §2).** A symbol can only
  be provided by a C `.sdata` definition WITHOUT breaking the match if **gold
  accesses it via gp_rel**. Absolute-accessed symbols must STAY in asm. The pilot
  proved this: `level`'s `D_800A6058` is accessed **absolute** (`lui/%hi` +
  `addiu/%lo`, 8 bytes) in gold; defining it as a small C global forced gp_rel
  (4 bytes), shrank `level.o(.text)` by 4 and cascaded the SHA. cc1 emitted the
  correct sdata bytes — the break was purely the caller's addressing width.
- **Tentative-def pattern already handles INITIALIZED gp_rel sdata** (corrects
  Problem §2's "fails outright for initialized small data"). `enemies.c` declares
  e.g. `EntityCallback X asm("D_800A5A54");` (no initializer, no `extern`) and
  matches today, even though the blob initializes `D_800A5A54 = .word
  StartAnimSequence4A`. The 4-byte tentative **common** merges with the blob's
  4-byte strong def (strong wins, no storage added, no shift) while telling cc1
  the symbol is small → gp_rel. The real blocker for `D_800A595C`/`FindLayerSlot`
  is that it must be used as a **20-entry array base**; declaring it at the full
  array size makes it large → absolute + segment shift. Declaring it as a 4-byte
  **pointer** (`LayerRenderSlot *g_LayerRenderSlots asm("D_800A595C");`, matching
  the single `.word` the blob stores) should clear blocker 1 via the same
  merge-clean pattern — leaving only blocker 2 (the `beq`/`bne` branch-sense),
  which is ordinary permuter work, NOT a linking/segmentation problem.

### Phase 1 — DONE (byte-neutral named split)

- `SLES_010.90.jsonnet`: added a `sdata(start, name)` helper and replaced the
  single `data('96154','sdata')` line with an ordered per-TU split (10 named
  pieces: blb, pickups, gamecd, movie, enemies, bosses, player, playst, finn,
  vehicle) plus 3 unnamed pieces (the shared engine-globals head `0x800A5954`,
  the `0x800A5D10` 16-byte gap, and the interleaved `0x800A603C` tail). All still
  asm (`build/asm/data/*.sdata.o(.sdata)`), same bytes.
- `make config && make extract && make check` → **✓ BUILD MATCHES**
  (`5a14b65cb44813bfed1ee53c6a3f4456bc230f97`). Verified the named pieces link in
  order at the late `main_SDATA_END` position in the generated ld.

### Phase 2 — DONE (`blb` migrated to C `.sdata`, byte-matching)

- Chose `blb` (smallest single-owner piece, 0x800A5980..0x800A59A8, 40 B). Its
  only consumer is the **unmatched-asm** `EntityDestructCallback` — so the
  addressing-mode constraint does not apply (verbatim asm references the symbols
  by name); only byte-identity + exact gp-offsets are required.
- `src/blb.c`: added `extern` decls for the three `PlatformRide*` callbacks and
  eight initialized globals (keeping the exact `asm("D_800A59xx")` names so the
  asm consumer still resolves): the 8-byte config header `D_800A5980`, three
  `{marker=0xFFFF0000, callback}` word pairs, and an `"END2"`+pad sentinel
  (`D_800A59A0[2]`). The trailing pad word MUST be an explicit initialized array
  element (`{0x32444E45, 0}`) — a standalone `= 0` global would sink to `.sbss`
  and shift `pickups` by 4.
- `SLES_010.90.jsonnet`: added `dotsdata(start,name)` helper and flipped
  `sdata('96180','blb')` → `dotsdata('96180','blb')` (splat type `.sdata` =
  linked from `build/src/blb.o(.sdata)`). After `make extract`,
  `asm/data/blb.sdata.s` is gone and the ld links `build/src/blb.o(.sdata)` at the
  same late position — auto-removed from the early auto-link block (option b,
  confirmed again).
- **`make check` → ✓ BUILD MATCHES on the first try.** `objdump -t build/src/blb.o`
  shows the eight symbols at `.sdata` offsets 0x00/0x08/0x0C/0x10/0x14/0x18/0x1C/
  0x20, linking at 0x800A5980..0x800A59A0 exactly. **cc1 2.7.2 emits initialized
  `.sdata` in declaration order (no reorder)** — declare in address order and
  sizes/offsets line up. This is the reusable Phase 4 recipe.

### Phase 3 — NEXT (not started)

- Phase 4 has begun: **`pickups` migrated** as the second TU (0x800A59A8..
  0x800A59E8, byte-matching). It validated the **friendly-name + asm-name
  coexistence** pattern the sweep needs: symbols that matched C code references by
  a friendly name (e.g. `DECOR_TRIGGERED_STATE_MARKER`) get `<type> <friendly>
  asm("D_800A59D8") = <init>;` — the `asm()` ties the output symbol to the D_
  address (so unmatched asm consumers resolve) while the C name stays usable.
  Because the initialized block sits at the END of the file (so all `Decor*`
  callbacks are in scope, no forward decls needed for them), the friendly names
  used earlier in the file were converted from tentative-defs to `extern`
  declarations at their original sites. Callbacks with non-`Entity*` signatures are
  cast to `EntityCallback`. `make check` green on the first try.

- Phase 3 `D_800A595C` is a **shared** engine global (entinit, gstate, layer,
  lvlload) sitting mid-block at gp+8 — carving it into a `.c`-provided `.sdata`
  slot without shifting `g_pGameState` (gp+12) is the delicate part, AND its
  consumer (`FindLayerSlot`) is **matched C**, so the addressing-mode constraint
  DOES apply there (unlike blb). Declaring `D_800A595C` as a 4-byte pointer should
  give gp_rel; blocker 2 (the `beq`/`bne` branch-sense) is permuter work with
  uncertain payoff. Recommend confirming approach before investing.
- Phase 4 sweep: the blb recipe generalizes to every single-owner initialized
  piece — declare each label as an initialized `asm("D_...")` global in ADDRESS
  order, keep trailing pad words inside an initialized array (never a bare `= 0`),
  flip `sdata(...)` → `dotsdata(...)`, `make extract && make check`. TUs whose
  sdata is all-zero (gamecd, movie) are `.sbss`/common — already handled by the
  existing tentative-def trick, not a `.sdata` migration.

- **Phase 4 sweep completed for the large single-owner sdata pieces** (all
  byte-matching `5a14b65c…`): `blb`, `pickups`, `player`, `finn`, `vehicle`,
  `enemies`, `bosses`, `playst`. Remaining sdata asm blobs are the shared head
  `data('96154')`, the all-zero `gamecd`/`movie` (`.sbss`/common), the 16-byte
  gap `data('96510')`, and the tail `data('9683C')` — these stay as asm. The
  `.data` blob `data('80FEC')` (~365 syms, absolute-addressed → looser
  constraint) and `.bss`/`.sbss` remain for a later pass.

- **Generator tool** `tools/gen_sdata_block.py` (untracked helper) automates the
  block for a TU: `python3 tools/gen_sdata_block.py asm/data/<tu>.sdata.s
  src/<tu>.c`. It prints `EXTERN:` lines (callbacks referenced but not defined in
  the `.c`) then the address-ordered C block — markers → `u32 … = 0xFFFF0000`,
  END2 sentinels → `u32 X[2] = {0x32444E45, 0x00000000}` (skips the pad word),
  raw code addresses → `(EntityCallback)0x…`, symbols → `(EntityCallback)Sym`,
  and (fix) `.asciz` strings → `char X[] = "…"`. `friendly_map` skips comment
  lines (`*`/`/*`/`//`) and uses `setdefault` so a friendly name is only bound
  from a *real* decl (never a commented-out example), and it now emits a
  `// WARNING: gap …` when consecutive addresses aren't contiguous.

- **Two techniques the sweep relies on** (both byte-verified):
  1. *Raw-address callbacks* — an sdata slot holding a code address with no
     symbol becomes `(EntityCallback)0xADDR`; byte-identical to gold's `.word
     0xADDR`. `EntityCallback` = `void(*)(Entity*)`; casting any fn to it is safe.
  2. *Same-TU tentative + strong merge* — a tentative-def `u32 X asm("D_Y");`
     earlier in a TU and an initialized strong-def `u32 X asm("D_Y") = v;` in the
     appended block, using the **same C name**, merge to one `.sdata` entry (decl
     order, no error). So scattered friendly tentative-defs can be **left in
     place** (as in `playst`) instead of converted to `extern` — provided the
     block reuses the exact same C name for that address. A *different* C name on
     the same `asm()` output symbol → two conflicting output-symbol definitions →
     assembler error.

- **`playst` gotcha (string in the sdata range):** g_FntFmt_PercentS @0x800A5EC8
  is a 4-byte `"%s\0\0"` string sitting *between* two descriptor slots. The
  generator's original `.word`-only parser silently dropped it, leaving a 4-byte
  hole → every later slot shifted and the symbol went undefined. Fixed the
  generator to parse `.asciz`; the manual entry is
  `char g_FntFmt_PercentS[4] asm("g_FntFmt_PercentS") = "%s";`. Lesson for the
  `.data` blob: always verify address contiguity of the emitted block (the
  generator's gap-warning now does this) — string/float data can hide between
  `.word` rows.
