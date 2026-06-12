# Compiler Quirks & Toolchain Findings

Findings from the first real matching session (2026-06-12), decompiling the
tail of the `Game/RENDER` module (`func_80019F2C` / `func_80019F88`). These
constraints shape how every future function must be decompiled.

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

## Tooling gotchas (decomp-permuter)

- `pycparser` must be **2.21** (`pip install pycparser==2.21`); newer
  versions removed `pycparser.plyparser` which the vendored permuter imports.
- The permuter expects `mips-linux-gnu-as` / `mips-linux-gnu-objdump` on
  PATH; shim them to `mipsel-unknown-linux-gnu-*`.
- The generated `nonmatchings/<func>/compile.sh` only runs cc1 (emits .s);
  append the maspsx step so it produces an object:
  `... | cc1 ... -o "$OUTPUT.s" && python tools/maspsx/maspsx.py
  --aspsx-version=2.86 --run-assembler -o "$OUTPUT" "$OUTPUT.s"`.
- `make permuter-import` passes only `$(FILE)`; invoke `import.py` directly
  with the nonmatching asm path as the second argument.
