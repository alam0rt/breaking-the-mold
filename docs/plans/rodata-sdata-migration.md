---
title: "Rodata/Sdata Migration Plan: Un-shelving Verified-Correct Functions"
category: plans
tags: [plans, migration, rodata, sdata, linker, splat, playst]
---

# Rodata/Sdata Migration Plan

**Created:** July 4, 2026
**Status:** Phase 0 + Phase 1 (rodata) COMPLETE — pilot landed; Phase 2 (sdata) + Phase 3 (full-TU) open
**Pilot target:** `PlayerEvent_ZoneTriggerHandler` (playst.c) — jump table `jtbl_80011AE4` ✅ migrated
**Mechanism:** `tools/migrate_rodata.py` + `tools/rodata_migrations.json`, run post-splat by the Makefile
(`RODATA_MIGRATE`). Repeatable via the `rodata-migrate` skill (`.claude/commands/rodata-migrate.md`).
**Related:** `docs/plans/segment-reorganization-proposal.md`, memory `gp-rel-small-global-d800a595c`,
`playst-fsm-frame-coinflip`

---

## RESULT (pilot, 2026-07-04)

`PlayerEvent_ZoneTriggerHandler` is now linked from C with `jtbl_80011AE4` served
from `build/src/playst.o(.rodata)`; `make check` reproduces the gold SHA1, and a
full clean `make extract && make check` re-applies the split and stays green.

Two things differed from the original speculation below:

1. **The split is a post-splat patch, not a jsonnet edit.** Expressing the split
   as extra `rodata()` subsegments in the jsonnet does NOT work: splat would
   re-emit an asm `jtbl_80011AE4` that references code labels (`.L8005BEA8…`)
   which vanish once the function is C (assemble/link error), and splat groups all
   `c`-segment rodata *after* the asm pool so it can't land at `0x22E4`. Instead
   `tools/migrate_rodata.py` runs after every splat call: it excises the blob from
   `asm/data/playst.rodata.s` into a `_b` half and rewrites the `.ld` to insert
   `build/src/playst.o(.rodata)` between the halves. Idempotent; survives extract.
2. **The C wasn't quite "verified clean."** The shelve comment claimed a clean
   whole-function diff, but two residuals remained: `src->worldY` needed an `s32`
   temp to force `lh` (not `lhu`) feeding the `+0x20`/`sh`, and the `0x1017` query
   case had to be written `result = 0; if (disableScale == 0) result = (…);` so the
   `result=0` fills the `bnez` delay slot. Both are ordinary codegen fixes; the
   rodata migration itself was byte-exact on the first linked build.

The key enabling fact confirmed empirically: `SUBALIGN(2)` on `.main` overrides the
compiled section's native `2**3` alignment, so the table drops in at `0x22E4` with
no padding.

---

## Motivation

A class of functions in `src/playst.c` (and elsewhere) is **shelved as `INCLUDE_ASM`
despite having C that is already verified byte-correct across the whole function
range.** They are blocked not by codegen but by *where the linker places
compiler-emitted read-only / small data*. Two distinct problems wear this costume:

- **Problem A — rodata migration (jump tables).** `PlayerEvent_ZoneTriggerHandler`
  compiles to bytes that diff clean, but gcc emits its `switch` jump table into
  `build/src/playst.o(.rodata)`, while the linker script only pulls the splat copy
  from `build/asm/data/playst.rodata.o(.rodata)`. The compiled table is never
  linked; the asm table is, so both a duplicate symbol and a wrong layout result
  unless we move the boundary.

- **Problem B — sdata placement (marker/fn global pairs).** Several installer
  functions (e.g. the Bounce twins `BounceJumpAnimation`/`BounceActive`) reference
  consecutive small-data globals such as `D_800A5F14`/`D_800A5F18`
  (`PlayerHiddenIdleMarker`/`Fn`). The C body matches, but the pair lands at the
  wrong gp-relative offset (off by 4). This is **not** a segment-migration problem
  — playst's `.sdata` is *already* fully compiler-emitted (see below) — it is a
  definition-order / anchoring problem inside the C-owned sdata.

Cracking these recovers **already-solved** functions with zero new decompilation.

---

## Current state (verified July 4, 2026)

`src/playst.c` is a `c` segment. Everything it owns is already served from the
compiled object **except rodata**:

```
SLES_010.90.ld:
  31:  build/asm/data/playst.rodata.o(.rodata);   <-- ASM  (the only holdout)
  82:  build/src/playst.o(.sdata);                <-- C
  133: build/src/playst.o(.sbss);                 <-- C
  184: build/src/playst.o(.bss);                  <-- C
  252: build/src/playst.o(.text);                 <-- C
  285: build/src/playst.o(.data);                 <-- C
```

`find asm -iname '*playst*'` returns only `asm/nonmatchings/playst/` (text stubs)
and `asm/data/playst.rodata.s`. There is **no** playst data/sdata asm segment.
Confirms: Problem B is layout-within-C, Problem A is the segment boundary.

### The playst rodata pool (`asm/data/playst.rodata.s`, 421 lines, rodata offset 8260)

The pool is almost entirely **switch jump tables**, emitted in source order:

| Label            | VRAM        | Owner (current state)                          |
|------------------|-------------|------------------------------------------------|
| `D_80011844`     | 0x80011844  | switch table, owning fn still `INCLUDE_ASM`    |
| `jtbl_800118F4`  | 0x800118F4  | large table (~125 entries), owner `INCLUDE_ASM`|
| **`jtbl_80011AE4`** | **0x80011AE4** | **`PlayerEvent_ZoneTriggerHandler` — C ready** |
| `jtbl_80011B14`  | 0x80011B14  | owner `INCLUDE_ASM`                            |
| `jtbl_80011B44` … `jtbl_80011CC4` | …  | run of small tables, owners `INCLUDE_ASM`     |
| `D_80011CF4`, `D_80011D14`, `D_80011D34` | … | data tables, owners `INCLUDE_ASM`     |

**Key structural fact:** rodata for a translation unit is emitted by gcc as one
contiguous pool in source order, and the linker pulls
`build/asm/data/playst.rodata.o(.rodata)` as a *single contiguous unit* at a fixed
rodata offset. You cannot cherry-pick one table out of that unit without splitting
the unit.

---

## Why partial migration is feasible *right now* (the pilot's leverage)

The general case is all-or-nothing: to link `build/src/playst.o(.rodata)` you must
reproduce **every** rodata byte the whole TU emits, in order — which needs every
switch/const-owning function in the TU to be matched C simultaneously. That is the
wall that deferred this.

`jtbl_80011AE4` is the exception because **`PlayerEvent_ZoneTriggerHandler` is
currently the only in-C `switch` function in `playst.c`.** Every other table's
owner is still `INCLUDE_ASM`, so gcc emits *no* rodata for them. Therefore
`build/src/playst.o(.rodata)` should contain **exactly** `jtbl_80011AE4` and
nothing else — making it safe to splice a single compiled table into the middle of
the otherwise-asm pool. This window closes as more switch functions are migrated,
so the pilot doubles as a template we lock in before the pool gets crowded.

---

## Mechanism: splitting the rodata pool at a table boundary

Precedent exists in git history — `b03ca22 "Split rodata at jumptable alignment
boundaries"`, `03fcc49 "Pair rodata with owning text segments"`. The `.ld` is
**regenerated by splat** from `SLES_010.90.yaml` (`ld_legacy_generation: true`
preserves YAML segment order), so the split must be expressed in the **YAML**, not
hand-patched into `.ld` (hand edits are lost on `make extract`).

Target link order for the pilot:

```
build/asm/data/playst.rodata.o(.rodata);   # part A: D_80011844 .. jtbl_800118F4  (ends at 0x80011AE4)
build/src/playst.o(.rodata);               # jtbl_80011AE4 (compiler-emitted)
build/asm/data/playst_b.rodata.o(.rodata); # part B: jtbl_80011B14 .. D_80011D34
```

For this to byte-match:
1. Part A must occupy exactly `0x80011844 .. 0x80011AE4` (unchanged — it already does).
2. Compiled `jtbl_80011AE4` must be exactly 12 words (0x30 bytes), word-aligned,
   with entries resolving to the same `.L8005B…` code labels. (Table content is
   `.word <label>`; the linker resolves them, so as long as the switch has the
   same case→target mapping the resolved words are identical.)
3. Part B must start exactly at `0x80011B14`.

---

## Phase 0 — Investigation & go/no-go (do first, ~half day)

Before touching any config, **empirically verify the pilot's core assumption.**

1. Temporarily un-shelve `PlayerEvent_ZoneTriggerHandler`: replace its
   `INCLUDE_ASM` with the verified C already parked in the source comment
   (playst.c ~L613–641). Keep the current `.ld`/`.yaml` (expect a build/link
   failure — that's fine; we only want the object).
2. Compile just the TU and dump its rodata:
   ```
   nix develop -c bash -c 'make build/src/playst.o'
   mipsel-unknown-linux-gnu-objdump -s -j .rodata build/src/playst.o
   ```
   **Go criterion:** `.rodata` contains exactly 0x30 bytes = the 12-entry jump
   table, and *nothing else* (no strings, no float/double pools, no other table).
   Record the exact bytes.
3. Diff the compiled table's resolved targets against
   `asm/data/playst.rodata.s:189–202` (`jtbl_80011AE4`). Confirm identical
   case→label mapping. (Also confirm gcc's `switch` lowering here even uses a jump
   table and not an if-chain — the verified-clean text diff implies it does, but
   confirm the table symbol is actually referenced.)
4. **No-go fallback:** if `.rodata` carries extra bytes (e.g. an unexpected string
   literal from another matched fn, or gcc emits the table only under different
   `-G`/switch-density heuristics), abandon the *partial* approach and route this
   function through the full-TU rodata migration (Phase 3) instead. Do not force it.

---

## Phase 1 — Pilot: migrate `jtbl_80011AE4`

Only proceed if Phase 0 is green.

1. **Split the asm rodata file.** Split `asm/data/playst.rodata.s` at the
   `jtbl_80011AE4` boundary into part A (through `enddlabel jtbl_800118F4`) and
   part B (from `jtbl_80011B14`). Preferred: let splat regenerate both from the
   YAML rather than hand-editing (keeps `.d`/labels consistent). Delete
   `jtbl_80011AE4` from the asm entirely — it now comes from C.
2. **Express the split in `SLES_010.90.yaml`.** Convert the single `[8260,
   "rodata", "playst"]` subsegment into an ordered triple so splat emits the three
   linker lines above in order:
   - `[8260, "rodata", "playst"]`   → part A (up to 0x80011AE4 = ROM 0x22E4)
   - a rodata slot that resolves to `build/src/playst.o(.rodata)` for the C table
   - `[0x2314-ish, "rodata", "playst_b"]` → part B (from 0x80011B14 = ROM 0x2314)

   The exact splat spelling for "pull the C object's rodata here" needs to be
   nailed down in Phase 0 by inspecting how already-migrated files (e.g.
   `entity`, `pickups`) get their `build/src/*.o(.rodata)` line — those are `c`
   segments whose rodata *is* compiled, so their YAML pattern is the template.
   (Ghidra/splat term: this is the difference between a standalone `rodata`
   subsegment and rodata owned by a `c` subsegment.)
3. **Un-shelve the function** for real: swap `INCLUDE_ASM(...
   PlayerEvent_ZoneTriggerHandler)` for the verified C, add the required file-scope
   externs / gp-rel global pairs (`PlayerZoneEnterMarker`/`Fn` @D_800A5DB8/BC,
   `PlayerZoneWarpMarker`/`Fn` @D_800A5DC0/C4) as documented in the source comment.
4. **Regenerate & verify:**
   ```
   make extract   # regenerates .ld/.yaml-derived splits — CONFIRM the 3 lines appear in order
   make check     # must reproduce the ROM SHA1
   ```
5. If `make check` is green: commit (never `--no-verify`; let the pre-commit
   byte-match hook run — memory `never-no-verify`). If red: `off2ram` the first
   diff, confirm it is a boundary/alignment issue vs a genuine table mismatch, and
   iterate on the split offset only.

**Deliverable:** one recovered function + a proven, documented recipe for
"single-table splice into an asm rodata pool."

---

## Phase 2 — Sdata anchoring (Problem B, independent of Phase 1)

Distinct workstream; unblocks the Bounce-twin installers.

1. **Map the target sdata layout.** From the ROM / Ghidra, list the ordered
   sequence of small-data globals around `D_800A5D50 … D_800A5F58` with their exact
   gp-relative offsets and sizes. This is the ground truth the C `.sdata` must
   reproduce.
2. **Audit the C-emitted order.** `objdump -t build/src/playst.o | sort` the
   `.sdata`/`.sbss` symbols; compare to the target. The 4-byte shift on
   `D_800A5F14`/`D_800A5F18` means a preceding global is missing, mis-sized, or
   mis-ordered relative to the target.
3. **Anchor via definition order + tentative-def idiom.** Memory
   `playst-fsm-frame-coinflip` records that the tentative-def gp-rel idiom
   (`u32 X asm("D_800A5F04"); EntityCallback Y asm("D_800A5F08");` — *not* `extern`)
   already works for the 5F04/5F08 pair but the 5F14/5F18 pair shifts. Likely cause:
   an intervening global that the target defines but our C doesn't (so our pool is
   4 bytes short at that point). Fix candidates, in order of preference:
   - Provide the missing intervening global as a tentative def in the correct
     source position (cleanest — mirrors the original TU).
   - If genuinely unowned, an explicit padding/dummy small global at the right
     offset (documented as a placeholder to remove when its real owner is found).
4. **Verify** each Bounce twin with a full `make check`, not just an fdiff — memory
   `stale-binary false match` and `permuter-prune-false-positive` both warn that
   sdata-shifting changes can report a local clean diff while the whole-binary
   layout moves. Gate on green SHA1.

---

## Phase 3 — Generalize (full-TU rodata migration, later)

Once the pool has *multiple* in-C switch owners, the single-splice trick stops
working (playst.o(.rodata) then holds several tables and must reproduce a
contiguous run). At that point migrate the whole playst rodata pool at once:

1. Precondition: **all** rodata-owning functions in the TU matched in C, in the
   original source order (so gcc's emitted pool order matches the target byte
   layout). This couples rodata migration to finishing the file's switch-bearing
   `INCLUDE_ASM` functions (the big event handlers).
2. Replace `build/asm/data/playst.rodata.o(.rodata)` wholesale with
   `build/src/playst.o(.rodata)` in the YAML; delete `asm/data/playst.rodata.s`.
3. Risk: gcc's rodata *ordering* (jump tables vs string pools vs const arrays) may
   not equal the original's if the original used a different const/switch layout.
   Expect an ordering-reconciliation pass. Keep the Phase-1 split recipe as the
   fallback for any single table that won't fall into line.

---

## Risks & non-goals

- **`.ld` is generated, not authored.** All changes go through `SLES_010.90.yaml`;
  verify by regenerating with `make extract` and re-reading the `.ld`. Never
  hand-patch `.ld` as the fix (memory: splat regenerates it).
- **Boundary alignment.** Jump tables are word-aligned; a mis-set split offset
  shifts everything downstream and cascades the first diff into an unrelated table.
  Always confirm part-B start VRAM == `0x80011B14`.
- **False matches.** Layout changes move the whole binary; a local fdiff can read
  clean while SHA1 fails. Gate every step on `make check` (memories
  `stale-binary-false-match`, `never-no-verify`).
- **Not a codegen effort.** No function's C body changes here beyond un-shelving
  already-verified text. If a body needs new pins/fences, it belongs in the FSM
  workstream, not this plan.
- **Scope.** Pilot is playst-only. `hud`, `clayball`, `vehicle`, `lvlload` have the
  same asm-rodata holdout (see `.ld` lines 28–34) and inherit the Phase-1 recipe
  once proven.

---

## Success criteria

- [ ] Phase 0 go/no-go recorded with the actual `objdump -s -j .rodata` bytes.
- [ ] `PlayerEvent_ZoneTriggerHandler` linked from C, `make check` green, committed
      via the pre-commit hook.
- [ ] Split-splice recipe documented (this file updated with the exact YAML spelling).
- [ ] At least one Bounce twin un-shelved via Phase 2 sdata anchoring, `make check` green.
- [ ] Recipe confirmed transferable to a second file (hud or clayball).
