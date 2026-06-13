# Compiler Quirks & Toolchain Findings

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

**Working theory:** the original was built with a slightly different
PSY-Q-era cc1 patch level — likely one in which sched1 had a stronger
preference for retaining the constant-load result (`li $v0, -1` from
the branch delay slot at offset `0x20`) by *deferring* its only use
until the `la` had been scheduled into a different free temp. The
decompals/old-gcc 0.17 release does not contain that exact build.
This matches what we've observed in other PSX decomps where some
functions stay non-matching at 200–250 score on otherwise-correct
source.

**Recommendation:**

1. Accept 225 as a known floor and `INCLUDE_ASM` the two CLUT
   functions for now. Document them as Quirk-5 instances.
2. If/when a missing GCC patch level (a 2.7.2 build with slightly
   different sched1) surfaces in another decomp project, retest both
   functions before any further source-mutation effort.
3. When decompiling new "tick-slot install" functions (greppable
   pattern: `li $v0, -1` in a branch delay slot followed by stores
   to `sp+lo/sp+hi/sp+fn` and a struct copy into `$s0` / first arg)
   expect the same 225 floor and don't permuter-bomb them past a
   few thousand iterations.

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
