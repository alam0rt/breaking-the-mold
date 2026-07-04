---
title: "Matching idioms — a symptom → fix index for residual fdiffs"
category: root
tags: [matching, idioms, index]
---

# Matching idioms — symptom → fix index

This is the **reverse lookup** for the other two matching docs. When you are
staring at a residual `fdiff`/`asm-differ` diff and need to know *what to try*,
scan the symptom tables below, apply the fix, and follow the reference for the
mechanism.

- **`compiler-quirks.md`** explains *why* cc1 needs each nudge (codegen
  mechanics, GCC-source-level root causes). Read it before writing C.
- **`matching-conventions.md`** explains *how to apply* armor honestly
  (tagging, `do {} while (0)` over macros, offset-named fields).
- **This file** is the index: *"my diff looks like X → try Y (see quirk N)."*

> Every entry here is a real, reproduced result on this ROM (gold SHA1
> `5a14b65cb44813bfed1ee53c6a3f4456bc230f97`), not folklore. Where an idiom
> was first proven on a specific function, that function is named so you can
> read the committed source as a worked example.

## The escalation ladder (try in this order)

Cheapest, least-surprising fix first. Do **not** pre-emptively sprinkle armor —
apply only what the *current* diff calls for, and re-verify after each step.

1. **Fix STRUCTURE first** (arg count, call arity, forwarded registers, loop
   shape). Run `m2c` before hand-guessing — the permuter cannot change
   structure, so a structurally-wrong base is a dead end no matter how long it
   runs. See `decompile` skill Step 2.1 and the InvokeEntityRenderCallback
   post-mortem.
2. **Source-level shape** — declaration order, named temporaries, signed/
   unsigned type choice, `goto out` vs multiple `return`, comparison operand
   order. (compiler-quirks.md Quirk 6 series.)
3. **`do {} while (0);`** basic-block boundary when *ordering* is the only
   problem. (Quirk 6k.)
4. **Empty `__asm__ volatile` barriers** — store-order (`:::"memory"`) or
   register-coloring (`"=r"(x):"0"(x)`) — only when 1–3 can't do it.
5. **Hard-register pins** (`register T x asm("$N")`) — encode a call-arg or
   CSE-across-call shape. Start unpinned; add one pin at a time (each pin adds
   anti-dependence edges that can derail scheduling — see "pin minimally").
6. **decomp-permuter** for pure coloring/scheduling residuals once the shape
   and length are exact.
7. **Shelve** (`INCLUDE_ASM`) and record the closest base + repro command. Some
   classes are provably unmatchable as isolated C — see the last section so you
   don't burn cycles.

---

## Symptom index

### A. Structural (wrong instruction count / arg count) — fix before anything else

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| Whole function is longer/shorter; call passes a different number of args than you wrote | Wrong parameter count or a tail call forwards its own `a2`/`a3` | Run `m2c` and trust its param count / `arg2 = …` live-input reads over your reading of the asm | decompile skill Step 2.1 |
| One instruction short/long, and a downstream **data table** (vtable/jumptable) shows the first byte-diff while the function itself `fdiff`s MATCH | Length delta shifts the whole binary (`ld_legacy` flow layout) — the real diff is elsewhere | Get the count exact *first*; ignore the "off-by-4 global" until length matches (it self-corrects) | [goto-loop-unrotated], [hard-reg-pin] length-trap |
| Divide-bearing function comes out ~7 instructions short; target has `break 0x7`/`break 0x6` around the `div` | maspsx only emits ASPSX signed-division trap guards with `--expand-div`, set **per-TU** | Add `$(BUILD_DIR)/src/<file>.o: MASPSX_EXTRA_FLAGS := --expand-div` next to the other Makefile overrides. Only safe if no *already-matched* func in that TU divides without guards (grep `/`,`%` first) | [maspsx --expand-div] |
| Function ends exactly on `jr ra` (no trailing `nop`); your build is **4 bytes longer** and the first diff is in a downstream data table, yet the function's own range MATCHes and the permuter says score 0 | The original let the `jr ra` delay slot bleed into the *next* function's first instruction; cc1 always adds `jr ra; nop` for a standalone leaf | **Unmatchable as isolated C — keep `INCLUDE_ASM`.** Don't permute. | [jr-ra delay-slot bleed] |

### B. Register coloring (same length, wrong register — $v0↔$v1, $a0↔$v0)

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| A stored `(field == K) ? A : B` result lands in `$a0` but target uses `$v0` | Live range of the ternary result | Pull the compared field into its own named temp *before* the compare (`u16 t = *p; dst = (t==K)?A:B;`). Try this **before** `asm("$2")` pinning — pinning cascades | [ternary-result $v0] |
| A constant (`li vN,imm`) is held in a **saved register across a call**; target re-materializes it on both sides | cc1's cse folds two identical constants into one pseudo that survives the call (cse invalidates *hard* regs at CALL_INSNs, but keeps *pseudos*) | Pin the constant to a caller-saved hard reg and re-assign it per use site: `register s32 one asm("$2"); one = 1;` on each side. An empty `__asm__` between the stores does **not** break this fold | [hard-reg-pin cse call barrier] |
| A spurious/missing `move $vN, $sN` right before a store through a pointer you already hold in `$s0` | idiomatic `e->f = 0;` coalesces the copy into `$s0`; target materialized a fresh caller-saved temp | Give the access its own pointer local + register barrier: `o = (T*)e; __asm__ volatile("":"=r"(o):"0"(o)); o->f = 0;` | Quirk 6i |
| A cast/shift chain computes in-place in a pinned source reg (`sll s2,s2` vs target `sll v0,s2`) | result coalesced into the source's register | Pin the result var (`register s32 x asm("$2")`); the plain-int-copy routing gets coalesced away here so the pin is needed | [hard-reg-pin] |
| Two locals are swapped between `$v0`/`$v1` | graph-coloring assigns roughly in declaration order | Reorder the local **declarations** to match target register order (even if never live together) | Quirk 6b |
| Register mismatch immediately before a `jal` | arg-register reservation at the call site | Add a `static` forward decl for the callee (if plausibly same TU) | Quirk 6c |

> **Pin minimally.** Each hard-reg pin adds anti-dependence edges that make
> `sched1` hoist constant loads and can derail call-setup scheduling. Start
> unpinned, add one pin at a time for the registers that actually differ; a pin
> can cost more than it buys. (Proven on the four `*DespawnEvent` notifiers —
> full pinning made no source shape restore the target schedule; unpinning fixed
> it instantly. Quirk "pin minimally".)

### C. Instruction scheduling / delay slots (right instructions, wrong order)

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| FSM slot-install stores (`markerLo/markerHi/fn`) scheduled in the wrong order, or `la fn` hoisted above/below `li -1` | `sched1` reorders freely within a basic block | Insert an empty `do {} while (0);` (a zero-instruction BB boundary the scheduler obeys). **Empty form only — never wrap the body.** Pick marker assignment order to mirror target's first two post-preamble instrs | Quirk 6k, 6-order table |
| First early-out branch's delay slot has `sw ra` (prologue store) instead of target's hoisted setup for the *second* check | separate `if`s leave nothing ready for the delay slot | Combine the two sequential bounds checks into one `&&`: `if (a<X && b<Y){BODY}` — the 2nd operand's setup then fills the 1st branch's delay slot | [&& delay slot] |
| A `move`/`sra` should sit in a `bne` delay slot but yours has a `nop` (with an exposed hazard `nop` before the branch) | `x=a; if(c) x=f(a);` coalesces the copy away | Write `if (c) x=f(a); else x=a;` — the else-arm copy survives and `dbr` hoists it into the delay slot | [hard-reg-pin] recurring shape |
| Independent call-setup constants (`li a1,K`) float too early / bunch at the wrong end of a region | `sched1` dependency-chain-length priority — **not** steerable by source fences | `do{}while(0)` confines them to a region but can't pick *where within* it. Don't over-tune; hand the closest base to the permuter early | [do-while fence limits] |

### D. Load width / sign-extension (lb/lbu, lh/lhu, srl/sra, slt/sltu)

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| Target uses `lh` for a field but you emit `lhu` (result is stored back via `sh`) | cc1's combine folds `sign_extend`→`lhu` when the high bits are provably unused (truncated store) | Materialize the field into its own `s32` local on a separate statement first (`s32 cx = gs->camera_x; … = e->worldX - cx;`) so the `lh` survives | [lh-vs-lhu] |
| `srl` vs `sra` is the sole difference | signedness of the shifted operand | Cast: `(u32)x >> n` → `srl`; `x >> n` (signed) → `sra` | Quirk 6d |
| `slt` vs `sltu`, or a comparison-temp width diff | signed vs unsigned compare; param type | Match the operand types (`u8 scene` gave `sltiu`; `s32 sub` gave an unmasked `a1-1`) | Quirk 6d, [&& delay slot] |
| Target does redundant per-operand truncation *before* a call; your code correctly defers to one truncate-after | your **extern prototype** for the callee declares a wider/narrower param than the real callee — cc1 only sees the call-site decl | Widen/narrow the *declared* param type in your own extern (e.g. declare `s32` where the real callee takes `s16`). Zero runtime cost — o32 passes both in a full register | [permuter prune / extern width] |

### E. Addressing (gp-relative vs absolute for small globals)

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| Target uses absolute `lui/lw %hi/%lo` for an sdata symbol; you emit `lw …%gp_rel(…)($gp)` | `-G8` classifies known-size ≤8-byte externs as small-data | Declare the extern with **unknown size** to suppress it: `extern s32 D_xxx[];` → forces `lui/lw` | Quirk 2 |
| Target uses **gp-rel** (`lw aN, K(gp)`) for an *initialized* small global; you emit absolute | can't be forced per-access; the `asm("D_…")` alias and tentative-def `--use-comm-section` tricks both fail for *initialized* data (the latter adds a 2nd cell and shifts the whole sdata segment) | Define the initialized global **in the owning TU** (`T *g = (void*)&D_…;`) and remove its dlabel from the splat sdata segment so there's exactly one definition → gp-rel. Coordinate aliases | [gp-rel small global] |
| One store uses gp-rel but every other access of the same symbol is absolute (or vice versa) | ASPSX made per-instance decisions we don't fully model; likely different `.extern` sizes per TU in the original | No clean single-store fix. Accept `INCLUDE_ASM` or match the whole file's dominant mode. **Do not patch maspsx** — the linker rejects naive GPREL16 relocs against non-small-data symbols | Quirk 2 mechanism |

### F. Loop shape

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| Target loop is **top-test + unconditional back-jump** (`top: beqz end; nop; body; j top`); your `while`/`for` emits a rotated bottom-test (one instr shorter) | cc1 -O2 rotates loops to bottom-test, absorbing a pre-loop store into the entry branch's delay slot | Write it as an explicit label + `goto`: `loop: if (cond) { body; goto loop; }` | [goto-loop unrotated] |
| Multi-early-out predicate returning a constant colors all `slt` temps into `$v0`; target uses `$v1` for the first N | multiple `return 0` = independent exits; target keeps `$v0=0` live across all checks | Use single-exit `goto out; … out: return 0;` — forces compare temps into `$v1` and fills delay slots with useful work (saves 1–2 instrs) | Quirk 6h |

### G. Frame size / stack layout

| Diff symptom | Likely cause | Fix | Ref |
|---|---|---|---|
| Frame is 8 bytes too small; an 8-byte slot local sits at `sp+0` but target has it at `sp+4` with a hole | bare 8-byte struct local lands at `sp+0` | Wrap in a padded local so the slot sits at offset 4 (`struct { s32 pad; TickSlot t; }`), or a single tagged `volatile s32 pad;` | Quirk 3 |
| Prologue spills an arg register, then reads *halves* back with `lh`/`lhu` (not `sll/sra`) | the parameter is a small **by-value struct**, not an `s32` you bit-twiddle | Model it as `struct { s16 a; s16 b; }` passed by value (member order = home-slot offset order) | Quirk 6j |

---

## Known unmatchable (or hard-floored) as isolated C — shelve, don't grind

These have been investigated to root cause. Recognize them early and keep the
`INCLUDE_ASM` rather than burning permuter time.

- **`jr ra` delay-slot bleed** — function ends exactly on `jr ra` with no
  trailing `nop`; the delay slot is the next function's first instruction. cc1
  always emits the `nop` for standalone leaf C (+4 bytes). *Tell:* build 4 bytes
  long, first diff in a downstream data table, function's own range MATCHes,
  permuter score 0. Hit on `ClearAllLayerRenderSlots @ 0x80018D54`. See
  [jr-ra delay-slot bleed]. (Inverse trick — deliberately reusing another
  function's epilogue via source/link order — is `effects-epilogue-gluing`.)
- **CLUT slot-install 140-floor** — `SetTexturePageParams @ 0x80019F2C` and
  `InitCLUTColorLerpEffect @ 0x80019F88`. Exhaustively swept (every 2.6.x–2.7.x
  cc1 incl. the real PSY-Q 4.0 SN build, 80+ source shapes, all flag combos):
  floor is a 5-instruction `la`-scheduling window that no reachable toolchain or
  source shape breaks. `do {} while (0)` does *not* help these two specifically.
  compiler-quirks.md Quirk 5.
- **Initialized gp+N small global (`D_800A595C`)** — blocks
  `FindLayerSlotByEntityPointer` until the global is defined in-TU (see row E).
  Currently shelved; permuter-floored at 160 on a residual branch-sense diff.

## Verification discipline

- Per-function: `nix develop -c bash -c 'make && tools/fdiff.sh <off> <size>'` → MATCH.
- **Always `make clean && make check`** before trusting a result. The Makefile
  has no header-dependency tracking, so header edits leave stale `.o`s and can
  show a false MATCH from an incremental build. A permuter score-0 is **not**
  authoritative until installed in the real file and confirmed under a clean
  build (`rm -f build/src/<file>.{o,s,d}` first). Pruned permuter imports can
  compile differently than the real TU — re-import `--no-prune` when the
  residual is a cross-statement shape, not just local coloring. See
  [permuter prune false-positive].
- Never edit `asm/`; don't edit `include/` unless instructed.

## See also

- `docs/compiler-quirks.md` — *why* each nudge works (cc1 codegen, GCC source).
- `docs/matching-conventions.md` — *how* to apply armor honestly (tagging,
  `do{}while(0)`, offset-named fields, `NON_MATCHING` guard).
- `docs/ghidra/struct-workflow.md` — recovering real types/offsets.

<!--
Bracketed refs above ([lh-vs-lhu], [hard-reg-pin], etc.) are working session
findings folded into this repo doc. When a new idiom is proven, add a symptom
row here (indexed by what the DIFF looks like) and put the mechanism in
compiler-quirks.md. Keep this file a router: terse rows + links, not prose.
-->
