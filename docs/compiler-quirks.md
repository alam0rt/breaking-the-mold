---
title: "Compiler Quirks & Toolchain Findings"
category: root
tags: [compiler, quirks]
---

# Compiler Quirks & Toolchain Findings

> This doc explains *why* cc1 needs each nudge (codegen mechanics). For *how to
> apply* armor without obtuse source — tagging hacks, `volatile pad` over padded
> struct types, offset-named fields — see **`docs/matching-conventions.md`**.

Findings from the first real matching session (2026-06-12), decompiling the
tail of the `Game/RENDER` module (`func_80019F2C` / `func_80019F88`). These
constraints shape how every future function must be decompiled.

Update 2026-06-13 after refreshing submodules and reading the upstream
`maspsx` README: the README directly explains the `INCLUDE_ASM` reordering
failure as a non-zero `-G` + `__asm__` ordering issue and documents an
optional `__maspsx_include_asm_hack*` / `# maspsx-keep` workaround. It also
documents ASPSX-version differences, `$gp` support, and div/rem expansion.
It does **not** explain the remaining cc1 scheduling/register-allocation diffs
such as the CLUT slot-install `la` hoist in Quirk 5; those still look like
compiler-source-shape/permuter problems rather than maspsx post-processing.

## Which compiler?

The repo's standard pipeline is `cpp → tools/gcc-2.7.2-psx/cc1 → maspsx
(--aspsx-version=2.86) → mipsel-as` with fixed flags
`-O2 -G8 -fno-builtin -mno-abicalls -mcpu=3000` (see Makefile).

To validate the choice we compiled the same test source (the recurring
FSM tick-slot install + struct copy pattern from the CLUT effect functions)
across every plausible era compiler from decompals/old-gcc, plus Sony's real
PSY-Q cc1:

| Compiler | Verdict for the test pattern |
|----------|------------------------------|
| vanilla 2.5.7 | ✗ store order ok but `la` placed late; epilogue puts `addu $sp` in the `jr` delay slot (original doesn't) |
| vanilla 2.6.0 / 2.6.3 | ✓ identical output to 2.7.2 |
| **`bin/cc1-psx-26`** (Sony "GNU C 2.6.3 [AL 1.1, MM 40] Sony Playstation") | ✓ identical output to 2.7.2 |
| vanilla 2.7.2 / 2.7.2.3 (repo standard) | ✓ matches original registers exactly (struct copy via `$v0/$v1`, dest pointer moved to `$t0`) |
| 2.8.0 / 2.8.1 | ✗ struct copy uses `$t0/$t1`, no `move $t0,$a0` for the dest pointer |
| 2.91.66 (egcs 1.1) | ✗ adds callee-side `sll/sra` re-extension of char args — absent in original |
| 2.95.2 | ✗ abicalls/`.cpload` prologue, re-extension — clearly wrong era |

**Conclusion:** the original was compiled by a GCC 2.6.x–2.7.x cc1 (they are
indistinguishable on code compiled so far). Keep `tools/gcc-2.7.2-psx` as the
standard. The genuine Sony compiler `bin/cc1-psx-26` (GNU C 2.6.3 Sony
build, fetched from the sotn-decomp `cc1-psx-26` release — this is what the
flake's "PSX compiler not found" warning refers to) is kept for
cross-checking; so far it has produced byte-identical output to 2.7.2 on
everything tested.

Sony cc1 default-enables: `-fpcc-struct-return -fcommon -mgas -mgpOPT
-mgpopt -msoft-float`. The vendored 2.7.2 defaults to only `-mgas
-msoft-float`. **Gotcha:** if a function ever returns a struct by value,
`-fpcc-struct-return` vs `-freg-struct-return` will matter and the two
compilers may diverge.

## Quirk 1: ALL function bodies are deferred to end-of-file (-O2)

> **Update — root cause confirmed via GCC source:** the deferral is caused
> by `-G > 0` (we have `-G8`), NOT `-O2`. With `-G 0` function bodies would
> appear in source order. See "Root cause of Quirk 1" in the GCC source-level
> validation section at the end of this document.

This cc1 generation buffers every compiled function body and emits them
*after* all streamed top-level `asm` blocks, regardless of source order.
`INCLUDE_ASM(...)` expands to a top-level `asm` and streams immediately.
No flag we tested (`-fno-inline`, `-fno-inline-functions`,
`-fno-defer-pop`) changes this.

**Consequence:** in a file mixing `INCLUDE_ASM` stubs and decompiled C, the
final `.text` order is: *all asm stubs in source order, then all compiled
functions in source order*. Therefore:

- Only a **contiguous tail** of a module can be decompiled within a single
  `.c` file. **Decompile each module backwards from its last function.**
- To decompile from the middle/front, the module must be split into
  multiple files via additional `c` segments in `SLES_010.90.yaml`.
- This is why the two empty stubs (`func_80015434`, `func_80018D4C`)
  could not stay as C: their 8-byte bodies relocated to the end of
  RENDER's text (0x8001A0B8) and shifted everything after them.

Verified empirically: a test file with four functions of increasing size
(empty → loop) interleaved with asm blocks put *all* `.ent` blocks after
the last asm block, on both 2.7.2 and cc1-psx-26.

Upstream `maspsx` now documents this under its `INCLUDE_ASM` reordering
workaround: with non-zero `-G`, some GCC versions reorder function bodies
after data definitions and after top-level `__asm__` blocks. Their workaround
wraps each include in a function whose name starts with
`__maspsx_include_asm_hack` and marks every asm line with `# maspsx-keep` so
`maspsx` can preserve the include in place.

**ADOPTED (2026-06-13).** `include/include_asm.h` now uses this maspsx-keep
form (modeled on `Vatuu/silent-hill-decomp`). Because the include is wrapped in
a function, cc1 keeps it in the deferred-function stream *in source order*
alongside the real C functions, and `maspsx` strips the wrapper back to a plain
in-place `.include`. **The tail-only rule above no longer applies:** matched C
and `INCLUDE_ASM` can interleave anywhere within one `.c` file.

**Do NOT hand-edit `include/include_asm.h`.** It is *generated by splat* on
every `make extract` (`generate_asm_macros_files` defaults true), so a manual
edit gets silently overwritten with the classic top-level-asm macro on the next
extract — which floats included asm above compiled C and breaks the byte-match
(this exact trap cost a debugging session). The macro form is instead selected
by the splat option `include_asm_macro_style: 'maspsx_hack'` in
`SLES_010.90.jsonnet`; that is the only knob to touch.

Verified end-to-end: `src/render/sprite_accessors.c` was built with the layout
`InitSpriteObject` (C) → `SubmitPrimitiveBufferToGPU`
(`INCLUDE_ASM("asm/nonmatchings/render/sprite_accessors", ...)`) → 22 accessor
functions (C), i.e. an asm include sandwiched mid-file between compiled
functions, and `make check` still reproduces SHA1
`5a14b65cb44813bfed1ee53c6a3f4456bc230f97`. The per-function asm lives under
`asm/nonmatchings/<seg>/<Func>.s` (splat's standard nonmatchings layout); the
include resolves from repo root with `as -Iinclude`. The M2CTX/PERMUTER guard
expands `INCLUDE_ASM` to nothing and skips the top-level `macro.inc` include so
m2c context still parses.

## Quirk 2: GP-relative (-G8) vs absolute addressing of small globals

`-G8` makes cc1 address any *known-size ≤ 8 byte* extern via `$gp`
(`lw $x, %gp_rel(sym)($gp)`). Several original sites instead use absolute
`lui/lw %hi/%lo` for sdata symbols — e.g. `g_pBlbHeapBase @ 0x800A5954`
in `func_80019F88`.

**Workaround:** declare the symbol with unknown size to suppress the
small-data classification:

```c
extern s32 D_800A5954[];   /* forces lui/lw absolute access */
... AllocateFromHeap(D_800A5954[0], ...);
```

(The likely truth is the original declared a larger struct/array at that
address; unknown-size extern arrays reproduce the codegen without
committing to a layout.)

## Quirk 3: FSM tick-slot locals sit at sp+4 (4-byte stack hole)

The ubiquitous tick-slot install pattern

```c
slot.markerLo = 0; slot.markerHi = -1; slot.fn = Callback;
desc->tick = slot;            /* copied as two words: lw/lw + sw/sw */
```

appears in the original with the 8-byte local at **sp+4** and a 4-byte hole
at sp+0 (frame 0x10 with no other locals; in `func_80019F88` the slot is at
sp+0x14 above the 0x10-byte outgoing-args area). A bare 8-byte struct local
lands at sp+0 (frame 0x8) on every compiler tested.

**Workaround that matches:** wrap the slot in a larger local so it sits at
offset 4:

```c
typedef struct { s32 pad; TickSlot t; } PaddedTickSlot;
PaddedTickSlot u;
u.t.markerLo = 0; u.t.markerHi = -1; u.t.fn = Callback;
desc->tick = u.t;
```

What the original source actually declared is unknown (possibly a larger
scratch struct); revisit if a counter-example shows up.

## Quirk 4: 512-byte struct assignment = inline dual-path block copy

`*desc->workBuf = *desc->srcClut;` where the struct is
`struct { u16 entries[256]; }` (alignment 2, size 0x200) expands to the
inline unrolled copy seen in the original: a runtime `(src|dst)&3` test
selecting between an aligned `lw×4/sw×4` loop and an unaligned
`lwl/lwr/swl/swr` loop, 16 bytes per iteration. No memcpy call is emitted
(`-fno-builtin`). Alignment ≥ 4 on the struct would kill the unaligned
branch and not match.

## Quirk 5 (open): sched1 `la` hoisting in the slot-install sequence

> **Update 2026-06-23 — practical resolution via `do {} while (0);`.**
> For the broader slot-installer family (NOT the two CLUT-effect functions
> below), a much cleaner armor exists: an empty `do {} while (0);` creates
> an AST-level basic-block boundary that `sched1` respects. See
> **Quirk 6k** for the full pattern, when it does/doesn't apply, and the
> migration verdict (~25 functions across enemies.c/effects.c/pickups.c/
> menu.c/bosses.c). The two CLUT functions (`SetTexturePageParams`,
> `InitCLUTColorLerpEffect`) are still at the 140 floor — `do {} while (0);`
> does not help there.

Remaining 5-instruction diff in both CLUT functions: the original emits

```asm
lui  $v1, %hi(Callback)        # la hoisted ABOVE the stores, into $v1
addiu $v1, %lo(Callback)
sh   $zero, 4($sp)             # lo
sh   $v0, 6($sp)               # hi (-1 was loaded in a branch delay slot)
sw   $v1, 8($sp)               # fn
```

while every tested compiler/source permutation orders it

```asm
sh   $v0, 6($sp)               # hi stored first, freeing $v0
la   $v0, Callback             # la reuses $v0
sh   $zero, 4($sp)
sw   $v0, 8($sp)
```

Statement reordering, temporaries, pointer forms, and GNU constructor
expressions were all tried manually; decomp-permuter is the current tool of
record for this. Functionally identical — only instruction scheduling and
one register differ.

### Update 2026-06-13: exhaustive investigation — same quirk in BOTH adjacent CLUT functions

Same scheduling-and-regalloc divergence appears identically in both adjacent
functions in `asm/Game/RENDER_9554.s`:

| Function | Address | Notes |
|----------|---------|-------|
| `SetTexturePageParams`     | `0x80019F2C` (size `0x5C`)  | uses `CLUTPaletteCycleTickCallback` |
| `InitCLUTColorLerpEffect`  | `0x80019F88` (size `0x140`) | uses `CLUTColorLerpTickCallback` |

The fact that two consecutive functions in the same translation unit show
the *same* "keep `$v0=-1` alive through 8+ instructions, use `$v1` for the
`la`" decision strongly implies a systematic scheduler behavior in the
original compiler, not a one-off source-shape quirk.

**Permuter score breakdown** (`tools/decomp-permuter` `differences`
algorithm, `stack_differences=False`, `ign_branch_targets=True`):

```
Stack Differences:             0  (×1)
Branch Differences:            0  (×1)
Register Differences:          5  (×5)   = 25
Reorderings:                   0  (×60)  = 0
Insertions:                    1  (×100) = 100
Deletions:                     1  (×100) = 100
                                          ===
                                          225
```

The 5 register diffs are the `v0↔v1` swap across `lui/addiu/sw` (1+2+1
register operands across 4 instructions… one operand happens to also
match). The insertion+deletion pair is the `sh markerHi` store being
emitted at a different position. Permuter's `differences` algorithm
can't recognize a 3-instruction reorder as a "reordering" — it counts
it as a paired insertion+deletion. (`PERM_RANDOMIZE` alone reaches the
140 floor by recovering ~85 points of the gap; we never observed any
candidate below 140 over 40,000+ iterations.)

**Compiler sweep verdict** (`tools/compiler-sweep.sh`, all installed
`tools/gcc-*`): identical 225-baseline output from every GCC 2.6.0
through 2.7.2.x flavor (plain and -psx), as well as Sony's
`bin/cc1-psx-26`. 2.5.7 is much worse (605), 2.8.x+ totally wrong
(990–1865). No GCC version in the decompals/old-gcc archive (release
0.17) produces the original scheduling.

**Compile flag sweep** (`-fno-schedule-insns`, `-fno-schedule-insns2`,
`-O1`, `-O3`, both schedulers off): every variant is *worse* than 225
on baseline source. Default `-O2 -G8 -fno-builtin -mno-abicalls
-mcpu=3000` is provably optimal among reachable flag combinations.

**Source variants tested** (all produced the same 225 codegen):

- field assignments in source order (markerLo, markerHi, fn);
- field assignments reordered (fn first, then markers);
- markerHi assigned at top of `if` block, far ahead of byte stores;
- separate `s16 minusOne = -1;` named-temp pinning -1 to a variable;
- `PaddedTickSlot` (s32 pad + slot) vs bare `TickSlot` local;
- `static inline init_tick_slot(TickSlot*, TickCallback)` helper.

GCC inlines the helper byte-identically and schedules the inlined
body the same way regardless of how the source expresses the marker
init order. The scheduler decision is being driven by the *use* (`sh
v0, 22(sp)` for `markerHi`) freeing `$v0` early, after which the
register allocator naturally re-picks `$v0` for the `la` because it
is the lowest free caller-saved temp; nothing in the source
controls that prioritization.

**Working theory (DISPROVEN 2026-06-13):** the original was built with a
slightly different PSY-Q-era cc1 patch level — likely one in which sched1 had
a stronger preference for retaining the constant-load result (`li $v0, -1`
from the branch delay slot at offset `0x20`) by *deferring* its only use
until the `la` had been scheduled into a different free temp. The
decompals/old-gcc 0.17 release does not contain that exact build. This
matches what we've observed in other PSX decomps where some functions stay
non-matching at 200–250 score on otherwise-correct source.

**Update 2026-06-13 — ruled out:** ran the real PSY-Q 4.0 SN Systems
`CC1PSX.EXE` (`GNU C 2.7.2.SN32.3.7.0002 [AL 1.1, MM 40] Sony Playstation
compiled by CC`, dated 14.5.97) extracted from the original Programmer Tools
CD 2.0. See [tools/gcc-2.7.2-sn32/](../tools/gcc-2.7.2-sn32/) — it's a Win32
PE binary driven through wine via a thin [cc1 wrapper script](../tools/gcc-2.7.2-sn32/cc1).
Output is **byte-for-byte identical** (`cmp -s` reports identical .o) to our
standard `tools/gcc-2.7.2-psx/cc1` on both source orderings:

| source order            | gcc-2.7.2-psx | gcc-2.7.2-sn32 | identical? |
|-------------------------|---------------|----------------|------------|
| `markerHi; fn; markerLo;` (current) | 420 | 420 | ✓ |
| `markerLo; markerHi; fn;` (old)     | 1180 | 1180 | ✓ |

(Scores measured by `tools/decomp-permuter/src/scorer.py` with the
`PERM_RANDOMIZE` wrapper removed — strictly larger numbers than the
`@PERM_RANDOMIZE`-aware `140`/`225` floors quoted earlier, but the *ratio*
and the `psx == sn32` equality are the meaningful signal.)

So the floor is **not** caused by a missing GCC patch level: the actual
PSY-Q 4.0 cc1 produces the same code as the decompals rebuild we already
ship. The remaining differences with the target must come from one of:

1. A still-later SN build that did ship in the game (e.g. Programmer Tools CD
   3.x, 4.x, or 5.x — the version that ships ASPSX 2.86, which we already
   know was used since `--aspsx-version=2.86` is required for matching).
2. Different cc1 flags (none of the obvious knobs help — see flag sweep above).
3. A source shape we haven't reached yet despite 80+ permuter-bombed variants.

**Update 2026-06-14 — library version pinned to PSY-Q 4.0.** Ran every
PSY-Q library signature pack from [lab313ru/psx_psyq_signatures](https://github.com/lab313ru/psx_psyq_signatures)
against the four LIB segments in `bin/SLES_010.90` using
[tools/psyq-version-detect.py](../tools/psyq-version-detect.py):

| PSY-Q version | matched sigs / total |
|---------------|---------------------|
| **4.0**       | **108 / 1228** (peak) |
| 3.7           | 87 / 1088 |
| 4.6           | 73 / 2167 |
| 4.7           | 69 / 2170 |
| 4.1           | 47 / 1328 |
| (everything else) | < 50 |

Pairwise discrimination via [tools/psyq-version-compare.py](../tools/psyq-version-compare.py)
is even clearer:

- **4.0 vs 4.1:** 43 shared, **61 only-in-4.0**, **0 only-in-4.1**
- **4.0 vs 3.7:** 84 shared, **20 only-in-4.0**, **0 only-in-3.7**

The game's library code is therefore strictly closer to PSY-Q 4.0 than to
3.7 (older) or 4.1 (newer). Discriminators span every LIB (LIBAPI, LIBC,
LIBCD `BIOS.OBJ`/`CDREAD*.OBJ`/`ISO9660.OBJ`/`SYS.OBJ`, LIBETC `INTR*.OBJ`,
LIBGPU `EXT.OBJ`/`PRIM.OBJ`/`SYS.OBJ`, LIBGTE `COR_0[1-6].OBJ`/`MSC00.OBJ`,
LIBSPU `SPU.OBJ`/`S_SV*.OBJ`) — not just a handful of small thunks.

So the **library binaries** in Skullmonkeys are from PSY-Q 4.0 (May 1997).
That doesn't fully pin the build environment though:

- The compiler binary in PSY-Q 4.0 (`GNU/CC1PSX.EXE` =
  `2.7.2.SN32.3.7.0002`, build 14.5.97) is byte-identical in output to our
  `tools/gcc-2.7.2-psx/cc1`, so cc1 is not what's different.
- PSY-Q 4.0's bundled ASPSX is 2.56, but our `--aspsx-version=2.86`
  maspsx setting is what matches the binary. Re-testing
  `func_80019F88` with `--aspsx-version` 2.50/2.56/2.66/2.77/2.81/2.86
  produced **identical scores** for both `gcc-2.7.2-psx` and
  `gcc-2.7.2-sn32` — for this function, none of the ASPSX-version-gated
  knobs (`expand_li`, `sltu_at`, `gp_allow_offset`, `gp_allow_la`) fire,
  so the aspsx contradiction has no observable effect here.

**Bottom line:** the libraries are PSY-Q 4.0, the cc1 produces identical
code to PSY-Q 4.0's, and aspsx version is invisible at the 140 floor.
The remaining 140-floor gap is a pure cc1 source-shape problem — there is
no missing-toolchain-version explanation left to chase.

Reproducer for any future cc1 binary: drop the new `cc1` next to a
`tools/gcc-2.7.2-XXX/` folder and rerun
`tools/compiler-sweep.sh nonmatchings/func_80019F88-rand/base.c
nonmatchings/func_80019F88-rand/target.o`.

### Update 2026-06-14: 225 floor manually broken to 140 (no permuter)

A targeted source-shape sweep (`tools/floor-140.sh`,
`tools/floor-padded.sh`, `tools/floor-sub140*.sh`) found two source
mutations that combine to drop the score from 420 → 140 *without* the
permuter and without any toolchain change:

1. **Natural marker order `lo, hi, fn`** (was `hi, fn, lo` per old
   `@hack`): 420 → 225. The earlier "`hi; fn; lo;` required for 225"
   guidance in `base.c` was wrong — see grid in `tools/floor-padded.sh`,
   any of `{lo,hi,fn}`, `{hi,lo,fn}` reaches 225; `{hi,fn,lo}` is
   strictly worse at 420.
2. **`desc->flag39 = flag;` moved to *after* the three marker writes**:
   225 → 140. The store ends up at the spot where the target keeps the
   tail of the la-fix sequence, so the `differences` algorithm's
   insertion/deletion count drops by one pair.

Both changes together match the candidate that the permuter previously
discovered as `output-140-1/source.c`; the new `nonmatchings/func_80019F88-rand/base.c`
now hardcodes this best-known shape as the seed. Combined sweep results
(see scripts; first column is score):

```
  140  lo,hi,fn natural order + flag39 after markers  <- accepted best
  200  lo,hi,fn + BOTH flags after markers
  225  lo,hi,fn natural order (old)
  225  hi,lo,fn order
  225  marker writes in nested scope
  300  cb-var TickCallback fn intermediate
  345..505  various other desc-field reorders
  420  hi,fn,lo (the @hack the old base.c documented)
  760..1200 other orderings (worse)
```

The remaining 140-floor diff is now exactly the 5-instruction la-fix
window (target loads `fn` into `$v1` then stores Lo/Hi; ours stores Hi
first, then loads `fn` into `$v0`, then Lo). None of: `static const fn`,
named `TickCallback fn = …` temps, address-taken markers, volatile
intermediates, register-asm pinning to `$3`, inline-`la`-via-array,
or struct/union casts moved below 140 — see `tools/floor-sub140c.sh`.

### Recommendation

1. Accept 140 as the current floor and `INCLUDE_ASM` the two CLUT
   functions for now. Document them as Quirk-5 instances.
2. If/when a missing GCC patch level (a 2.7.2 build with slightly
   different sched1) surfaces in another decomp project, retest both
   functions before any further source-mutation effort.
3. When decompiling new "tick-slot install" functions (greppable
   pattern: `li $v0, -1` in a branch delay slot followed by stores
   to `sp+lo/sp+hi/sp+fn` and a struct copy into `$s0` / first arg)
   expect the same 140-floor pattern. Start with the `lo,hi,fn` natural
   marker order and the `flag39`-style trailing byte-write moved to
   between `markers` and `desc->tick = local.slot;`.

The `func_80019F88-rand` candidate tree and `compiler-sweep.sh` are
the reproducer if anyone wants to retry on a future toolchain.

### Update 2026-06-13: cross-checked against Vatuu/silent-hill-decomp

Silent Hill (PS1) decomp project uses two compilers we did not yet have:

- `tools/gcc-2.7.2-cdk/cc1` (Sony "CDK" / 970404 build of GCC 2.7.2, kept
  in-tree at <https://github.com/Vatuu/silent-hill-decomp/tree/master/tools/gcc-2.7.2-cdk>;
  used specifically for their `libkpad` module)
- `tools/gcc-2.8.1-psx/cc1`

…with a notably different flag set:
`-O2 -G0/-G8 -mips1 -mcpu=3000 -w -funsigned-char -fpeephole -ffunction-cse
 -fpcc-struct-return -fcommon -fverbose-asm -msoft-float -mgas -fgnu-linker -quiet`

Tested both `gcc-2.7.2-cdk` (downloaded into `tools/gcc-2.7.2-cdk/`) and
`gcc-2.8.1-psx` on the CLUT-effect test source with the SH flag set:
**both reproduce the exact same scheduling pattern as our standard
`gcc-2.7.2-psx`** — same `sh v0, 22(sp); lui v0, ...; addiu v0, ...;
sw v0, 24(sp)` sequence, same 225-score plateau. Differences observed:

- `cdk` / `2.8.1-psx` emit `lw a0, 56(sp)` for the `u8` arg (treating
  `char` as `int`); `2.7.2-psx` emits `lbu a0, 56(sp)`. The target uses
  `lbu`, so our existing compiler is already correct here. Add
  `-funsigned-char` to cdk/2.8.1 to recover the `lbu`.
- `2.8.1-psx` adds a `nop` in the `beqz` branch delay slot (offset 0x20)
  instead of putting `li v0, -1` there — strictly worse.

**Confirms the 225 floor is structural to this entire family of cc1
builds, not specific to our 2.7.2-psx choice.** No GCC 2.6.x / 2.7.x /
2.8.x build we can get our hands on (decompals/old-gcc 0.17, Sony
`bin/cc1-psx-26`, Vatuu's `gcc-2.7.2-cdk`, Vatuu's `gcc-2.8.1-psx`)
breaks past it for this idiom on its own.

### Update 2026-06-13: regalloc-influencing source idioms cribbed from Silent Hill

Silent Hill has several `@hack`-tagged source-level tricks for exactly
this class of problem. Worth trying before giving up on a stubborn
function:

- **"Zero-and" hack vars** ([bodyprog_mapscreen_80066D90.c#L1205](https://github.com/Vatuu/silent-hill-decomp/blob/master/src/bodyprog/bodyprog_mapscreen_80066D90.c#L1205)):

  ```c
  // @hack Regalloc fixes. `hackVar` usages compile to nothing (GCC knows
  // `hackVar == 0`), but the 4 vars pad the stack frame to match.
  // @hack Two `tileScreenEdgesX[tileX] & hackVar` loads bump the array
  // pointer's ref count so it wins `t8`.
  ```

  Pattern: declare extra `const int hackVarN = 0;` locals; sprinkle
  `expr & hackVar` (always 0, compiles away after const-folding) into
  expressions where you need GCC to count an extra reference to a base
  pointer so the regalloc heuristic picks the register you want.

- **"Messy decl order matters"** ([map5_s00.c#L153](https://github.com/Vatuu/silent-hill-decomp/blob/master/src/maps/map5_s00/map5_s00.c#L153)):
  the **declaration order** of local variables affects stack/reg
  assignment even when the variables are never live together. Reorder
  declarations to match the original frame layout before assuming the
  function is unmatchable.

- **"From permuter, not sure why this works"** ([credits.c#L1330](https://github.com/Vatuu/silent-hill-decomp/blob/master/src/screens/credits/credits.c#L1330)):
  things like `temp * (u64)256` instead of `temp * 256` change the
  multiplier-vs-shift emit and the live range — accept and document
  permuter-derived weirdness rather than rewriting it.

- **`SECTION(".rodata")` annotation** on small globals to keep them out
  of `.sdata` when the original referenced them absolute. We already
  handle this with `extern T arr[];` (Quirk 2) but explicit section
  attributes are another option.

For the CLUT functions specifically, the `hackVar` trick is the most
promising thing left to try — sneak in a fake `(... & zeroLocal)` term
inside the `markerHi = -1` RHS or the `desc->tick = local.slot` copy to
bump the use count of `$v0`'s `-1` value. Not attempted yet because the
current quirk is more about *scheduling order* than register *choice*;
worth a few thousand permuter iterations though.

### Update 2026-06-14: hackVar tried on `func_80019F88`, doesn't help

Ran a sweep of seven `hackVar`-style source variants against
`func_80019F88` and scored each under both `differences` and
`levenshtein` algorithms (the latter is what Silent Hill uses):

```
variant                    diff  leven
----------------------------------------
baseline                    225    225
extra-pad4                  225    225   (dead stores -> GCC strips them)
volatile-hack               425    425   (extern volatile forces a real lw)
volatile-on-fn              740    740
declare-minusone-after      225    225   (const-folded back to li v0,-1)
union                      FAIL        (typeck rejects)
fn-via-arr                  615    615   (extra la for the table)
```

Conclusions:

- **The two algorithms produce identical scores here**, so switching
  the default to `levenshtein` buys nothing for this quirk. Keep
  `differences` for compatibility with our existing tooling.
- **Plain `const int hackZero = 0;` is const-folded out** by GCC 2.7.2;
  any `expr | hackZero` or `expr & ~hackZero` collapses before sched1
  runs. The Silent Hill trick works there because their target
  expressions involve an address (`arr[i] & hackVar`) where the load
  itself is the side-effect being preserved — we don't have an analog
  in the CLUT slot-install.
- The only way to keep a real reference alive is `extern volatile`, but
  then GCC must emit a real `lw`, which makes the score *worse*.
- Verdict: hackVar is a useful tool in general (keep it documented for
  future stuck functions), but it does not break the 225 floor on the
  CLUT functions. Treat that floor as accepted and move on.

### Update 2026-06-14b: BREAKTHROUGH — `hi-fn-lo` source order drops 225 → 140

Don't accept the 225 floor — there's actually a 38% reduction available
just by reordering the slot field assignments. A full sweep across all
21+ installed cc1 builds confirms:

| source order                    | score |
|---------------------------------|-------|
| `markerLo; markerHi; fn;` (was baseline) | **225** |
| `markerHi; fn; markerLo;` (winner)       | **140** |
| `markerHi; markerLo; fn;`               | 225 |
| `fn; markerLo; markerHi;`               | 760 |
| `fn; markerHi; markerLo;`               | 770 |
| `markerLo; fn; markerHi;`               | 760 |

The winner forces the `la Callback` to be emitted *after* the `sh -1`
store, which causes sched1 to use `$v1` for the address (matching the
target) instead of immediately reusing `$v0`. Confirmed on
gcc-2.6.0 / 2.6.3 / 2.7.0 / 2.7.1 / 2.7.2 / 2.7.2.1 / 2.7.2.2 /
2.7.2.3 / `bin/cc1-psx-26` (all produce 140). Other compilers
(2.5.7, 2.7.2-cdk, 2.8.x, 2.9x) score worse on both orderings.

Remaining 140 = 4 reg-diffs (20) + 2 reorderings (120). The 2
reorderings are between `sh zero,lo(sp)` and `sw v0,fn(sp)` — sched1
still picks a different order there, and 80+ source-shape variants
across v3/v4/v5/v6/v7 sweeps (alternate dependency chains, prefix
reorder, no-pad, no-prefix-stores, volatile pointers, etc.) all hit
either 140 or worse. `-fno-schedule-insns{,2}` also fails to reach
140. The remaining 140 floor *does* appear structural.

Practical recommendation: **when decompiling tick-slot install code,
always use the `markerHi; fn; markerLo;` source order.** This is now
the preferred source pattern. Updated
[nonmatchings/func_80019F88-rand/base.c](../nonmatchings/func_80019F88-rand/base.c).

### Per-subdir compiler override mechanism (Makefile)

Mirroring the silent-hill-decomp pattern, the [Makefile](../Makefile)
now pre-stages alternate compilers as variables (`CC1_257`, `CC1_CDK`,
`CC1_281`, `CC1_SONY`) and uses recursively-expanded `MASPSX_FLAGS` so
target-specific variable assignments take effect at recipe time.

To opt a single file or whole subdir into a different toolchain, add
a target-specific assignment under the "Per-subdir / per-file
toolchain overrides" section in the Makefile. Examples:

```make
# Use Sony CDK 970404 build for libsd/libetc-like glue
$(BUILD_DIR)/src/libs/%.o: CC1 := $(CC1_CDK)
$(BUILD_DIR)/src/libs/%.o: ASPSX_VERSION := 2.77

# Single file that only matches with 2.5.7
$(BUILD_DIR)/src/system/foo.o: CC1 := $(CC1_257)
```

The mechanism is **not used by default** — every override is opt-in,
documented inline, and only activated for a file once we've confirmed
the alternate compiler actually matches that file.

### maspsx `--expand-div` knob (libsd-style div sequences)

`maspsx` has an `--expand-div` flag that expands `div`/`rem` into the
explicit PSY-Q sequence (`bnez` + `div` + `mfhi`/`mflo` + overflow
check) instead of the GCC-default short form. PSY-Q `libsd` and a few
other libraries were assembled with that expansion. We don't need it
for any decompiled file yet, but if/when a stuck function's diff is
dominated by div/rem shape mismatches, opt in for just that subtree:

```make
$(BUILD_DIR)/src/libs/libsd/%.o: MASPSX_EXTRA_FLAGS := --expand-div
```

The `MASPSX_EXTRA_FLAGS` variable was added specifically as the hook
for this kind of opt-in knob.

### Update 2026-06-13: also confirmed maspsx differences

## Quirk 6: source shapes found while matching render/sprite_accessors.c (2026-06-13)

From `InitSpriteObject` / `SubmitPrimitiveBufferToGPU` (RENDER_5C3C tail):

- **Constructor returns the object pointer.** `addu $v0, $a0, $zero` at function
  entry with all stores via `$v0` and `$v0` still live at `jr $ra` means the
  function *returns* its first argument (`return p;`). A plain `void` init
  function keeps stores on `$a0` and is one instruction shorter.
- **Double vtable store survives.** Two successive `sw X, 0xC(p)` with different
  vtable symbols are NOT dead-store-eliminated by this cc1 at -O2; write both
  assignments. The two pointers were distinct symbols (no CSE between the
  `lui/addiu` pairs) — here they were entries in the INIT_TABLES vtable region,
  which was misclassified as code (`func_80010324`) and is now a `rodata`
  segment so the symbols (`D_80010344`, `D_8001039C`) exist properly.
- **`load; move var,$v0` two-pseudo pattern.** A narrowing assignment
  (`u8 var = s32expr;` / `s16 var = mem;`) emits the load into a temp and a
  separate `move` into the variable's register *only if* the SI value has other
  direct uses; otherwise the truncation folds into the load (`lbu $s4` direct).
  Conditions testing the *memory expression* while arms use the *variable*
  (`x = b->x; if (b->x < 0) ... x ...`) reproduce the copy for s16 fields.
- **Local-store alias kill.** Any store to a local array (`ofs[0] = 0;`) between
  two reads of the same global chain (`D_800A5954[0]->field`) kills CSE and
  forces a full `lui/lw/lbu` reload — original code that shows a single load
  must have hoisted the value into a local before the first store.
- **Address association reveals source form.** `&buf->arr[i]` (12-byte elems)
  compiles to `addiu a0, s0, ofs; addu a0, base, a0` (mult+ofs grouped), while
  `i * 12 + (s32)buf + ofs` compiles to `addu s0, s0, base; addiu a1, s0, ofs`
  (mult+base grouped). The original mixed both forms for the same address in
  back-to-back calls, which also prevents full-address CSE across the call
  (only `i*12` stays cached in a callee-saved reg).
- **Explicit shift temp prevents arm merging.** `if (m + (f<<8) < 0) a = y +
  ((f<<8)-K); else a = y + (f<<8);` lets cross-jumping merge the arms (shift
  and arm-result land in the same reg). With `ysh = f << 8;` as a named local
  the arms keep distinct result registers and match the original's
  `j`-over-else shape.

## Tooling gotchas (decomp-permuter)

- Older `decomp-permuter` revisions needed `pycparser==2.21`; after the
  2026-06-13 submodule refresh (`efc5c5e`) the upstream tool uses m2c's
  vendored parser / pycparser-3 compatibility fixes, so do not re-add that
  pin unless bisecting an older revision.
- The permuter expects `mips-linux-gnu-as` / `mips-linux-gnu-objdump` on
  PATH; shim them to `mipsel-unknown-linux-gnu-*`.
- The generated `nonmatchings/<func>/compile.sh` only runs cc1 (emits .s);
  append the maspsx step so it produces an object:
  `... | cc1 ... -o "$OUTPUT.s" && python tools/maspsx/maspsx.py
  --aspsx-version=2.86 --run-assembler -o "$OUTPUT" "$OUTPUT.s"`.
- `make permuter-import` passes only `$(FILE)`; invoke `import.py` directly
  with the nonmatching asm path as the second argument.

## Quirk 6: Register Allocation Techniques (General)

Collected from SotN, Paper Mario, and our own failed-match post-mortems.
These apply to GCC 2.6.x–2.7.x on MIPS (graph-coloring register allocator).

### 6a: `>` vs `>=` selects `$at` vs `$v1`

> **⚠️ Corrected: this is WRONG.** Isolated tests with the actual
> `bin/cc1-psx-26` compiler show `>` and `>=` produce **byte-identical**
> code. The CheckPointInBox/CheckBoxOverlap matches that originally motivated
> this entry were actually due to Quirks 6h (goto) and 6e (load ordering).
> See "Quirk 6a CORRECTED" in the GCC source-level validation section
> below. The table that follows is misleading — keep this section as
> historical record only.

`slti` (set-less-than-immediate) is the only compare+branch primitive on
MIPS. The compiler transforms comparisons:

| Source | Emitted | Register used |
|--------|---------|---------------|
| `if (x >= 0x29)` | `slti v1, x, 0x29; bnez v1, ...` | **$v1** |
| `if (x > 0x28)` | `slti at, x, 0x29; bnez at, ...` | **$at** |

The `>` form uses `$at` (assembler temporary), the `>=` form uses a
general-purpose register. If the target uses `$at` in a comparison but
your code emits `$v1`, try flipping between `>` and `>=` (adjusting the
constant by 1).

**Applies to:** CheckPointInBox, CheckBoxOverlap — both showed `$at` vs
`$v1` mismatches on boundary checks.

### 6b: Variable declaration order = allocation priority

GCC 2.7.2's graph coloring assigns registers to locals roughly in
declaration order (first declared → lowest available register). If the
target uses `$v0` for variable A and `$v1` for variable B, declaring
them in the opposite order may swap the assignment.

**Technique:** reorder local variable declarations to match the register
usage observed in the target. If `$a0` holds a value that later appears
in `$v0`, it may be because a `move` from `$a0` to a local in a specific
declaration position caused the allocator to pick `$v0`.

### 6c: `static` function declarations change call-site allocation

Declaring a called function as `static` (even with an empty body above)
changes how the allocator reserves argument registers (`$a0–$a3`) at
the call site. If you see register mismatches immediately before a `jal`,
try adding a `static` forward declaration for the callee (if it could
plausibly be in the same translation unit).

```c
static void SomeFunc(Entity *a, s32 b);  // forward decl changes regalloc
```

### 6d: Signed vs unsigned types change shift/compare flavor

| Target instruction | Likely type needed |
|--------------------|--------------------|
| `srl` (shift right logical) | `unsigned` / `u32` |
| `sra` (shift right arithmetic) | `signed` / `s32` |
| `slt` (set-less-than) | signed comparison |
| `sltu` (set-less-than unsigned) | unsigned comparison |

A single cast (`(u32)x >> 4` vs `x >> 4`) can flip between `srl` and
`sra`. This is the most common cause of "almost matching" functions with
one instruction different.

**Applies to:** AddToDepthBucket — `srl` vs `sra` was the sole difference.

### 6e: Expression tree shape controls load ordering

Within a single expression, GCC evaluates sub-expressions in a specific
tree order. Two equivalent comparisons may load operands differently:

```c
if (a->x < b->x)    // loads a->x first, then b->x
if (b->x > a->x)    // loads b->x first, then a->x
```

If the target loads in a different order than your code, try algebraically
rewriting the comparison (flip operands + operator).

**Applies to:** CheckBoxOverlap — load ordering within comparison pairs
was the remaining difference.

### 6f: Named temporaries vs inline expressions

Assigning a sub-expression to a named local sometimes forces a register
to be allocated earlier (creating a longer live range), which can cascade
the coloring differently:

```c
// Inline — allocator picks reg late, may reuse
fn((s32)entity->x + offset);

// Named temp — allocator commits a reg early
s32 pos = (s32)entity->x + offset;
fn(pos);
```

Try both forms when the diff shows a register being "held" longer than
expected, or conversely freed too early.

### 6g: Division generates overflow checks (break 6/7)

GCC 2.7.2 with `-O2` emits `break 6` (division by zero) and `break 7`
(signed overflow: `INT_MIN / -1`) checks for signed integer division by
a *variable* (not a constant). If the target has `break` instructions
around a `div` and your code doesn't, ensure you're dividing by a variable
not a constant. Conversely, dividing by a constant produces no checks
(the compiler strength-reduces to multiply+shift).

**Applies to:** SetupEntityScaleCallbacks — missing `break` instructions
indicated the division was by a variable, not a constant as initially assumed.

### 6h: `goto` forces $v1 for comparison temps (single-exit pattern)

When a function has multiple early-out conditions that all return the same
value, using `goto label` instead of `return 0` produces different register
allocation. With multiple `return 0` statements, the compiler treats each
as an independent exit — it freely uses `$v0` for `slt` temporaries since
it just sets `$v0=0` right before returning. With `goto out`, there is a
single exit point where `$v0` holds the return value, and the compiler keeps
`$v0=0` **live** across all the branch checks. This forces comparison results
into `$v1` instead.

```c
// WRONG: produces slt $v0 for all comparisons
s32 CheckPointInBox(Entity *e, s16 px, s16 py) {
    CalculateEntityScreenBounds(e);
    if (px < e->screenX1) return 0;
    if (e->screenX2 < px) return 0;
    if (py < e->screenY1) return 0;
    if (e->screenY2 < py) return 0;
    return 1;
}

// CORRECT: produces slt $v1 for first 3 comparisons, $v0 only for the last
s32 CheckPointInBox(Entity *e, s16 px, s16 py) {
    CalculateEntityScreenBounds(e);
    if (px < e->screenX1) goto out;
    if (e->screenX2 < px) goto out;
    if (py < e->screenY1) goto out;
    if (e->screenY2 < py) goto out;
    return 1;
out:
    return 0;
}
```

The `goto` version also enables better delay slot scheduling — the compiler
can fill branch delay slots with useful work (e.g. sign-extending the next
variable) rather than redundant `move $v0, $zero` instructions. This
commonly saves 1-2 instructions in the output.

**Applies to:** CheckPointInBox — the `goto` pattern produced a byte-perfect
match where multiple `return 0` statements did not.

### 6i: Register barrier on a *pointer* local forces a pre-store `move`

Sometimes the target materialises a pointer it already holds in a callee-saved
register (`$s0`) into a fresh **caller-saved** temp right before a final store:

```asm
move  $v0, $s0          # copy entity ptr into a scratch reg
sb    $zero, 0x34($v0)  # ...then store through the copy
```

Idiomatic C (`e->field = 0;`, even through a cast) coalesces this away and
stores directly through `$s0`. To reproduce the copy, give the access its own
pointer local and pin it with the same register barrier used for callback
addresses (6e/Quirk 5):

```c
OverlayCallbackEntity *o;
...
o = (OverlayCallbackEntity *)e;
__asm__ volatile("" : "=r"(o) : "0"(o));  /* blocks coalescing into $s0 */
o->hiddenFlag = 0;                         /* -> move $v0,$s0; sb $zero,0x34($v0) */
```

This generalises the `fn`-barrier trick (Quirk 5) from function pointers to
**any** pointer the original copied into a fresh register before a trailing
use. Reach for it only when the *single* remaining diff is a spurious/missing
`move $vN, $sN` ahead of a store — it is load-bearing noise, not logic.

**Applies to:** `InitScrollingLayerEntity` (effects.c) — last diff after the
slot install was exactly this trailing `move`/`sb` pair.

### 6j: A packed register arg (two `s16`s in one reg) is a by-value struct

When the prologue spills an incoming arg register to its home slot and then
reads *halves* of it back with `lh`/`lhu` (rather than `sll/sra` shifts), the
parameter is a small **by-value struct**, not an `s32` you bit-twiddle:

```asm
sw   $a2, 0x38($sp)     # spill the packed arg to its home
lh   $v0, 0x3A($sp)     # high half  -> .b
lh   $t0, 0x38($sp)     # low half   -> .a
```

```c
typedef struct { s16 a; s16 b; } ScrollLayerDims;   /* occupies one reg */
void InitScrollingLayerEntity(Entity *e, s32 ctx, ScrollLayerDims dims, ...) {
    InitLayerScrollContext(e, ctx, a3, dims.a, dims.b, flags);  /* spill + lh halves */
}
```

Modelling it as `s32` + `(s16)x`/`(s16)(x>>16)` produces shift/extract code
instead of the spill-then-`lh` the target uses. The struct member order maps
directly to the home-slot offsets (`.a` = low half / lower address).

**Applies to:** `InitScrollingLayerEntity` — `a2` carried two `s16`s consumed
as separate args to `InitLayerScrollContext`.

### 6k: `do {} while (0);` is a basic-block boundary the scheduler obeys

**Discovered 2026-06-23 while cleaning the entity-FSM slot-installer family
(enemies.c, effects.c, pickups.c, menu.c, bosses.c).**

cc1 2.7.2's `sched1` (basic-block instruction scheduler) operates **within**
basic blocks; it will freely reorder loads/stores across plain statements but
will not move instructions across a basic-block boundary. An empty
`do {} while (0);` introduces a BB boundary at the AST level (the implicit
`if (0) goto top;` is dead-code-eliminated, but the BB split survives until
late). This produces a **zero-instruction scheduling fence** — strictly
cleaner than `__asm__ volatile("" ::: "memory")` because no clobber list,
no register impact, no obscure idiom.

This is the **preferred armor #3 in `docs/matching-conventions.md`**. It is
now empirically validated as a drop-in replacement for **most** slot-installer
armor that previously used the `__asm__ volatile` fence + per-`fn`-pointer
register barrier pair (Quirk 5 / 6e).

#### Pattern: multi-slot installer (≥ 2 slot stores)

```c
void InitFooState(Entity *e) {
    PaddedSlotPair slot;        /* frame-pin: a padded local */
    s16 m1;                     /* named locals control coloring (6b) */
    void (*fn)();
    do {} while (0);            /* @hack BB boundary — pins sched */
    m1 = -1;
    fn = FooTickCallback;
    slot.s[0].markerLo = 0;
    slot.s[0].markerHi = m1;
    slot.s[0].fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s[0];
    /* ...more slot stores reusing m1/fn... */
}
```

A single `do {} while (0);` right after the preamble (any callees, vtable
stores, sprite-table writes) is sufficient. The fn pointer colors to `$v1`
and `-1` colors to `$v0` without further nudging.

### 6l: Named phi-temp vs per-branch store to the real destination

**Discovered 2026-07-01 matching `CalculateEntityRenderBounds` (entity.c).**

When an `if`/`else` computes a value that's used exactly once right after the
branch (a classic "phi" shape), GCC's cross-jump tail-merging can fold the
two branches' final store into one shared instruction (a `j` from one arm
into the other arm's tail). Two C shapes produce this same *instruction
count and structure*, but colored differently:

```c
// Named temp: forces the value through a separate pseudo-register before
// the store, which anchors it to whichever hard reg that pseudo got colored.
s16 x2;
if (cond) { out[0] = ...; x2 = a - b; }
else      { out[0] = ...; x2 = c + d; }
out[2] = x2;

// Direct per-branch store: no separate pseudo, the expression's result
// register IS the store operand in both arms, so cross-jump merging colors
// it fresh (typically reusing $v0, the register the *previous* statement in
// the same arm just finished with).
if (cond) { out[0] = ...; out[2] = a - b; }
else      { out[0] = ...; out[2] = c + d; }
```

Byte-identical output/store count either way, but the register loaded for
each operand of the folded expression can swap ($v0/$v1) between the two
forms. If a diff shows only register-swapped loads immediately before a
shared post-if store, try dropping the named temp and writing the real
destination directly in each branch (or vice versa).

**Applies to:** CalculateEntityRenderBounds — `x2`/`y2` named temps produced
a `$v0`/`$v1` swap on the second load pair in each arm; writing `out[2]`/
`out[3]` directly in each branch matched byte-for-byte. Likely applies to the
sibling functions with the same facing/flipY-mirrored-bounds shape
(`IsEntityOffscreenLeft`/`Right`, `CalculateEntityScreenBounds`).

#### Pattern: single-slot tail installer

A function whose body is just *one* slot store followed by a tail call
(`SetEntitySpriteId` etc.) needs **two** boundaries:

```c
void EnemySetWalkSprite(Entity *e) {
    PadSlot slot;
    s16 m1;
    void (*fn)();
    do {} while (0);            /* @hack pins `sw ra` ordering */
    fn = EntityEventHandlerWalk;
    do {} while (0);            /* @hack pins fn coloring */
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->eventMarker = slot.s;
    SetEntitySpriteId(e, 0xE4CB8330, 1);
}
```

Without the entry boundary, cc1 schedules `sw ra` out of position; without
the second boundary the fn-pointer `la` hoists above `li -1`.

#### Assignment order matters per function

cc1 sometimes emits the fn `lui/addiu` pair **before** `li -1`, sometimes
**after**. The C must mirror it:

| Target order            | Source assignment order |
|-------------------------|-------------------------|
| `la fn → $v1; li -1 → $v0` | `fn = ...; m1 = -1;`    |
| `li -1 → $v0; la fn → $v1` | `m1 = -1; fn = ...;`    |

The InitCollectibleTimer / InitShooter / InitMenu* families want fn-first;
the InitCollectibleIdleState family wants m1-first. Check `fdiff` of the
first two instructions after the preamble and pick accordingly.

#### When `do {} while (0);` is NOT enough — keep the fn-barrier

These cases still need the explicit `__asm__ volatile("" : "=r"(fn) : "0"(fn));`
form (Quirk 5 / 6e):

1. **Rand-conditional fn pointers.** When `fn` is assigned inside *both* arms
   of an `if (rand()...) { fn = A; } else { fn = B; }` and used after the
   merge, cc1 will hoist the `la` outside the branches unless a register
   barrier in each arm holds it in place. `do {} while (0);` between the
   branches and the slot store does not help.
   *Examples:* `LaserMonkeyIdleState`, `CollectibleIdleState`,
   `InitEntityState_Idle/Animated`.
2. **Pointer-coalesce barriers (6i).** A trailing `move $vN, $sN; sb $zero, ...`
   needs the named-pointer + register-barrier idiom — the BB boundary does
   not block coalescing of the underlying SSA value.
   *Example:* `InitScrollingLayerEntity` keeps the `o` pointer barrier
   even after the slot armor migrated to `do {} while (0);`.
3. **Triple register-asm with sprite IDs.** Functions that pin
   `tickFn`/`eventFn`/`spriteId` to specific physical registers via the
   `register T x asm("$N");` + multi-output barrier idiom are encoding a
   call-arg shape, not a scheduling problem.
   *Example:* `InitEnemyAnimatedWithDeathSpawn`.

#### Empirical: only the EMPTY form works (do not wrap the body)

Replacing the empty `do {} while (0);` with a body-wrapping
`do { ...slot stores... } while (0);` form **regresses** to the original
bug. Tested 2026-06-23 on `DecorSetSpriteActive` (pickups.c) by rewriting:

```c
do {} while (0);
fn = CheckpointSwampTickCallback;
do {} while (0);
m1 = -1;
slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
*(CallbackSlot *)&e->tickMarker = slot.s;
```

into

```c
do {
    fn = CheckpointSwampTickCallback;
    m1 = -1;
    slot.s.markerLo = 0; slot.s.markerHi = m1; slot.s.fn = fn;
    *(CallbackSlot *)&e->tickMarker = slot.s;
} while (0);
```

Diff (`la fn → $v0 AFTER markerHi store`, instead of `$v1 BEFORE`):

```
< 8002e614: lui v1, ...           ; target: la fn into $v1 FIRST
< 8002e618: addiu v1, ...
< 8002e61c: li v0, -1
< 8002e620: sh zero, 20(sp)
< 8002e624: sh v0, 22(sp)
< 8002e628: sw v1, 24(sp)
---
> 8002e614: sh zero, 20(sp)        ; ours: stores first, fn into $v0 later
> 8002e618: li v0, -1
> 8002e61c: sh v0, 22(sp)
> 8002e620: lui v0, ...
> 8002e624: addiu v0, ...
> 8002e628: sw v0, 24(sp)
```

The wrapped form's body-BB gets coalesced back into the surrounding code by
some pre-`sched1` pass (likely `cse1` or `jump1` tail-merge). The empty form
has no body to merge with, so the boundary survives all the way to scheduling.

**Corollary tested 2026-06-23: per-slot macro `SLOT_STORE(...) do { ... } while (0)` also fails.**
Redefining the existing comma-expression `SLOT_STORE`/`SLOT_CLEAR` macros in
`src/enemies.c` as do-while wrappers (the canonical single-statement-safe
macro idiom) regresses ~50% of the slot installers. Specifically:

| Pattern | Result |
|---------|--------|
| Multi-slot with pre-store `do {} while (0);` (EnemyPatrolState, etc.) | ✓ MATCH |
| Single-slot tail installer (EnemySetWalkSprite-style) | ✗ extra `nop` before `addiu sp` |
| Rand-conditional fn (InitEntityState_Animated) | ✗ shifts whole prologue |
| Single-slot inline-store (InitCollectibleEntity_Alt) | ✗ |

The extra BB boundary *inside* every SLOT_STORE expansion gives cc1 too many
basic blocks to schedule against the single-slot/rand patterns — `sched1`
fills different delay slots, frame layout shifts. The comma-expression form
is therefore the **mandatory** macro shape for our codebase, even though
classical C style prefers the do-while wrapper. The
[matching-conventions.md](matching-conventions.md) recommendation to *expand
the macros to explicit struct-value stores per call site* is the cleaner
long-term direction (see effects.c / pickups.c / menu.c / bosses.c) — the
existing macro stays for back-compat only.

**Implication for inferring original source.** The 1998 C almost certainly
used per-slot macros (`INSTALL_TICK(e, fn)`, `INSTALL_EVENT(e, fn)`) whose
body was a `do { ... } while (0);` wrapper — that's the universally idiomatic
single-statement-macro form. The fact that the wrapper itself is
*load-bearing* (without it, scheduling collapses) is incidental: the original
devs got matching codegen for free from the macro's BB structure. Our
flattened explicit-local form has lost the macro and emits bare
`do {} while (0);` markers in the spots where the macro's BB boundaries
would have landed. Functionally equivalent to the original codegen,
semantically less honest about *why* the boundaries exist.

We cannot reconstruct the original macro shape from this side, because:
1. We don't know how many slots the original macro inlined per call (one,
   two, or all four installer slots in one expansion).
2. The original may have had per-slot-kind macros (TICK vs EVENT vs RENDER)
   that we've collapsed.
3. Whatever shape the original used, cc1's BB collapsing means a small change
   in macro body layout produces visibly different codegen — so any guess
   we'd make can't be empirically validated against the binary.

The best we can do is the current form: bare `do {} while (0);` markers
tagged `// @hack basic-block boundary required for match` at the call site.

#### Migration verdict

After applying the pattern across ~25 functions in `src/enemies.c`,
`src/effects.c`, `src/pickups.c`, `src/menu.c`, and `src/bosses.c`,
whole-ROM SHA1 still matches `5a14b65cb44813bfed1ee53c6a3f4456bc230f97`.
The `do {} while (0);` form replaces ~90% of slot-installer
`__asm__ volatile("" ::: "memory");` + `__asm__ volatile("" : "=r"(fn) : "0"(fn));`
pairs with a single semantically-meaningful line.

**Applies to:** entire slot-installer family — see `src/enemies.c`,
`src/pickups.c`, `src/menu.c`, `src/bosses.c::HazardSelectRandomBehavior`,
`src/effects.c::{InitScrollingLayerEntity,InitPlayerDeathParticle}`.

### Case study: matching InitScrollingLayerEntity from idiomatic C (2026-06-20)

A worked example of the "idiomatic first, then armor the artifacts" loop, useful
as a template for the rest of the entity-FSM tier (playst/bosses/effects/pickups/finn):

1. **Idiomatic C nailed the *shape* on the first compile** — the call, the
   vtable/allocSize stores, the slot install and the flag clear were all in the
   right place. Only three cc1 artifacts needed control, each with an existing idiom:
   - **Frame size** off by 8 → use a padded slot type (`TripadSlot`) instead of a
     bare `CallbackSlot` to pin the 0x30 frame (Quirk 3).
   - **Slot-store coloring/order** (fn must color to `$v1`, `-1` to `$v0`; store
     `markerLo` before `markerHi`; `la fn` hoisted) → named `fn`/`m1` locals +
     `fn`-register-barrier + `m1 = -1` placed *after* the fn barrier (Quirk 5/6e).
   - **Trailing `move $v0,$s0`** before the flag store → pointer barrier (6i).
2. **Replacing `INCLUDE_ASM` with the matching C left the whole-ROM SHA1
   matching** (`make clean && make check` → `✓ BUILD MATCHES`): the C is
   byte-identical to the shelved asm. `tools/fdiff.sh <off> <size>` confirms the
   single function (`MATCH`); it is per-function so it stays meaningful even when
   a whole-ROM check is mid-flux.
3. **The armor is load-bearing — do NOT mistake it for optional.** An earlier
   pass *appeared* to match a barrier-free struct-value version; that `MATCH` was
   a **stale incremental build** (the prior barrier-built `.o` was reused). Under
   `make clean` the bare version regresses: `0x28` frame, `markerHi` stored before
   `markerLo`, and no `move $v0,$s0`. All three armor pieces above are required to
   match this function *in isolation*. The clean struct-value form is the right
   *model* of the original source, but reproducing its codegen needs the armor to
   stand in for context we collapsed away (the inline slot macro, surrounding
   register pressure).

> **Measurement discipline:** never trust a `MATCH`/SHA1 from an incremental
> build when reporting a result as final. The Makefile's `%.o: %.c` rule has **no
> header-dependency tracking** (no `-MMD`/`.d` includes), so header edits silently
> leave stale objects; even `.c`-only loops can desync mid-session. `make clean`
> (or a dedicated `make verify` = `clean && check`) before any authoritative
> claim. Keep plain `make check` incremental for the fast per-function loop.

Takeaway for the tier: the slot pattern's obtuseness is **bounded and templated**
— write the clean version, fdiff, and apply only the specific armor each diff
calls for from this section (don't pre-emptively sprinkle barriers) — but the
armor that *is* needed is mandatory, not decorative. Verify under `make clean`.

---

## GCC 2.7.2 source-level validation (root-cause investigation)

Cross-referenced the empirical quirks above against the actual GCC 2.7.2
MIPS backend source at `~/projects/mips-gcc-2.7.2/`. Key files:
- `config/mips/mips.c` — codegen helpers (`mips_move_1word`, `gen_int_relational`)
- `config/mips/mips.h` — target macros (`ENCODE_SECTION_INFO`, `ASM_OUTPUT_EXTERNAL`)
- `config/mips/mips.md` — RTL patterns and insns

Each quirk validated by compiling minimal test programs with the actual
`bin/cc1-psx-26` compiler and reading both the generated asm and the source
code path that produced it.

### Root cause of Quirk 1 (function deferral): it's `-G`, NOT `-O2`

`mips.c:mips_asm_file_start` (line ~4150) opens a temporary file for the
text section when `TARGET_GP_OPT` is set (i.e. `-G > 0`, which we have as
`-G8`). All function bodies stream into this temp file. `mips_asm_file_end`
(line ~4203) flushes accumulated `.extern symbol, size` declarations
first, then `.text`, then concatenates the temp-file contents.

The exact comment in `mips.h:3041-3047` explains why:

> If we are optimizing to use the global pointer, create a temporary file
> to hold all of the text stuff, and write it out to the end. This is
> needed because the MIPS assembler is evidently one pass, and if it
> hasn't seen the relevant .comm/.lcomm/.extern/.sdata declaration when
> the code is processed, it generates a two instruction sequence.

So function bodies appear after all top-level `asm()` blocks because GCC
deferred ALL text to give the one-pass assembler a chance to learn symbol
sizes before processing the code.

**Implication:** removing `-G8` would put functions in source order, but
would break every existing match that depends on GP-relative addressing.
The `__maspsx_include_asm_hack` wrapper documented in Quirk 1 is the
correct workaround — it makes INCLUDE_ASM blocks ride along inside the
deferred function stream.

### Quirk 2 mechanism: GCC emits `.extern X, size` + raw `sw $r, X`

Test compilation of `extern int g_x; void w(int v){g_x=v;}` produces:

```asm
        .extern g_x, 4         # at top of file
        ...
write_x:
        sw      $4,g_x         # direct SYMBOL_REF, no hi/lo, no $gp
        j       $31
```

GCC's `mips_move_1word` (`mips.c:903`) emits the symbol-ref MEM operand
verbatim. The assembler decides at assembly time whether to expand `sw
$4,g_x` into:
- 1 insn `sw $4, %gp_rel(g_x)($gp)` (if g_x size ≤ G_VALUE per `.extern`), or
- 2 insns `lui $at,%hi(g_x); sw $4, %lo(g_x)($at)` (otherwise).

`mips.h:ENCODE_SECTION_INFO` (line ~2541) sets `SYMBOL_REF_FLAG` only when
`TARGET_GP_OPT && size <= mips_section_threshold`. This flag is only used
internally (e.g. for `RTX_COSTS` — line 2696) — the actual address-mode
selection happens at the assembler level.

#### maspsx limitation around `.extern`

There is a **limitation in `maspsx`** at
`tools/maspsx/maspsx/__init__.py:467`. When it sees a `.extern X, 4`
line, it does:

```python
if line.startswith(".extern"):
    in_sdata = False
    continue
```

It just continues without parsing the size, so the symbol never enters
`self.sdata_entries` / `self.sbss_entries`. As a result, the later
`%gp_rel` substitution in `process_lines` (~line 925) sees an unknown
symbol and emits `lui+sw` instead of `sw $r, %gp_rel(X)($gp)`.

This *initially* looks like why `LoadBLBHeader` fails to match: the
target uses `sw $s0, %gp_rel(g_pGameState)($gp)` for one store, GCC
emits `.extern g_pGameState, 4` + raw `sw $s0, g_pGameState`, and
maspsx never classifies it as small data.

**However, the naive fix (also classify any `.extern X, size<=8` as
sbss) BREAKS the build.** Tested: adding code to parse `.extern` and
add small symbols to `sbss_entries` caused linker errors:

```
relocation truncated to fit: R_MIPS_GPREL16 against `g_pBlbHeapBase'
```

This occurs in `src/Game/VEHICLE/vehicle.c` where `g_pBlbHeapBase`
(also `extern void *`, also size 4) is accessed. Even though
`g_pBlbHeapBase` *is* in `.sdata` at `gp+0` (so the relocation should
fit), the link fails — likely because the linker tracks small-data
symbol attributes per-relocation-record and a GPREL16 reloc emitted
against a symbol the linker doesn't believe is small-data fails the
check.

In the ORIGINAL binary:
- `g_pGameState` uses GP-rel for **exactly ONE** store in `LoadBLBHeader`
  and `lui+lw` everywhere else
- `g_pBlbHeapBase` uses `lui+lw` **everywhere** (no GP-rel)

So ASPSX wasn't doing a blanket "size ≤ G_VALUE → always GP-rel" — it
made per-instance decisions we don't fully understand. The most likely
explanation: the ORIGINAL source declared these pointers differently in
different translation units (e.g. as a larger struct in some files, as
`void*` in others), changing the `.extern` size and hence the
addressing mode per-file.

**Recommendation:** do not patch maspsx. For matching individual
functions that need GP-rel for small externs, either:
1. Accept the local mismatch and use INCLUDE_ASM, or
2. Declare the symbol with unknown size locally (`extern void *X[];`)
   to suppress the `.extern X, 4` emission — which forces `lui+lw`.

There's no clean way to force GP-rel for a single store without also
forcing it for all other accesses in the same file.

### Quirk 6a CORRECTED: `>` and `>=` produce IDENTICAL code

Direct test:

```c
int check_ge(int x) { if (x >= 0x29) return 0; return 1; }
int check_gt(int x) { if (x >  0x28) return 0; return 1; }
```

Both compile to the same byte sequence:

```asm
        slt     $2,$4,41    # ($4 < 41), 41 = 0x29
        bne     $2,$0,$L4
        ...
```

Inspection of `mips.c:gen_int_relational` (line 1735) shows the cmp_info
table:

```c
{ LT,   -32769,  32766,  1,  1,  1,  0, 0 },   /* GT  -- const_add=1, reverse_regs=1, invert_const=1 */
{ LT,   -32768,  32767,  0,  0,  1,  1, 0 },   /* GE  -- const_add=0, reverse_regs=0, invert_const=1 */
```

For `x > c` (GT): `const_add=1` adjusts cmp1 to c+1; `reverse_regs=1`
only fires when cmp1 is REG (not CONST_INT); `invert_const=1` inverts on
constant case. For `x >= c` (GE): identical except cmp1 isn't adjusted.

Both ultimately emit `slt result, x, K` (where K = c+1 for GT, K = c for
GE) with the same `invert` flag — and `convert_move(reg, gen_rtx(LT,
cmp0, cmp1))` allocates the result to the SAME pseudo register. After
register allocation, the destination register is identical.

**Conclusion:** Quirk 6a as originally documented (table showing `$at`
vs `$v1`) is **WRONG**. The match for CheckPointInBox/CheckBoxOverlap was
actually due to Quirks 6h (goto) and 6e (load ordering), not the `>`/`>=`
choice. Keep the trick as a thing to TRY, but the description of the
mechanism is incorrect.

### Quirk 6e confirmed: `a < b` vs `b > a` flips load order only

```c
int form_lt(R *a, R *b) { if (a->x < b->x) return 0; return 1; }
int form_gt(R *a, R *b) { if (b->x > a->x) return 0; return 1; }
```

Output diff:

```asm
form_lt:                       form_gt:
    lw $2,0($4)   # a->x          lw $3,0($5)   # b->x
    lw $3,0($5)   # b->x          lw $2,0($4)   # a->x
    slt $2,$2,$3                  slt $2,$2,$3   # IDENTICAL slt
```

The `slt` itself is byte-identical (`slt $2, $2, $3`). Only the two `lw`
instructions swap order. This is genuinely useful when a target binary
loads operands in a specific order — flip the comparison sense and
operand order in the C source to match.

### Quirk 6h confirmed and explained at insn level

Test case (matching CheckPointInBox shape):

```c
int returns_only(Box *b, int px, int py) {
    if (px < b->x1) return 0;
    if (b->x2 < px) return 0;
    if (py < b->y1) return 0;
    if (b->y2 < py) return 0;
    return 1;
}

int goto_actual(Box *b, int px, int py) {
    if (px < b->x1) goto out;
    if (b->x2 < px) goto out;
    if (py < b->y1) goto out;
    if (b->y2 < py) goto out;
    return 1;
out:
    return 0;
}
```

`returns_only`: every `slt` writes to **$2** ($v0); each `bne` has
`move $2,$0` in its delay slot (sets return value to 0 for the early-out).

`goto_actual`: first 3 `slt`s write to **$3** ($v1); first `bne`'s delay
slot has `move $2,$0` (sets $v0=0 ONCE, kept live across all early-outs);
final `slt $2,$2,$6` + `xori $2,$2,1` uses $v0 because that's the
"fall-through to return 1" path that owns $v0.

Mechanism: with single-exit `goto out`, register allocator sees that $v0
must hold 0 across all early-out branches. It hoists `move $v0,$0` into
the first branch's delay slot (a free slot anyway) and then keeps $v0
live-with-value-0 across the rest of the function. Comparison temps are
forced into $v1 to avoid clobbering $v0.

This also explains the "saves 1-2 instructions" claim: the second/third
delay slots no longer need `move $v0,$0` (already live), so the assembler
can pack `lw` for the next check directly into them.

### Practical decompiler workflow updates

Based on these source-level findings:

1. **Always try `goto out` for multi-early-out functions returning a constant**
   — this is now confirmed as a deterministic codegen technique, not voodoo.
2. **`>` vs `>=` is a NO-OP at codegen level** — only useful as a side-effect
   of changing operand evaluation order (which is what 6e actually controls).
3. **`.extern X, 4` maspsx limitation**: when matching a function that
   accesses small externs in a way the target uses GP-rel, accept the
   INCLUDE_ASM stub. Don't try to fix maspsx — the linker rejects naive
   GPREL16 relocs against symbols it doesn't consider small-data.
4. **`-G8` is structural** — removing it changes function ordering AND
   addressing modes globally. Don't touch.
