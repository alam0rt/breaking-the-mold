---
title: "Study notes: Glover PSX real source + Soul Reaver decomp"
category: analysis
tags: [analysis, glover, source, study]
---

# Study notes: Glover PSX real source + Soul Reaver decomp

Source material reviewed (read-only, external to this repo):
`~/Downloads/glover/Source Archives/psx/Whack.zip/` containing:

- **`Whack/`** — the *real*, shipped source code of **Glover (PSX)**, internal
  codename "Whack", by ISL (1998–9). ~52 `.c` / 50 `.h` files in a flat
  `glovsrc/` directory. This is genuine production source: the same studio
  whose conventions Skullmonkeys' flat `src/` layout already mimics ("Glover
  convention"). It is the single most relevant ground-truth reference we have
  for *what the original code probably looked like*.
- **`soul-re/`** — **Soul Reaverse**, an active byte-match decompilation of
  *Legacy of Kain: Soul Reaver* (SLUS-007.08, Gex 2 engine). A mature,
  well-documented project structurally identical to ours (splat + maspsx + m2c
  + asm-differ + decomp-permuter, PSY-Q `gcc-2.8.1-psx`). Useful as a model for
  *project/workflow conventions*.

Why this matters for us: this repo is clean-room — every symbol name and struct
field is a guess (see [[no-authoritative-source]]). Glover gives us a real
example of the *idioms, naming, guard macros, and data-driven architecture* a
late-90s PSX game from a related studio actually used. Where our guesses
diverge from these idioms, the idioms are usually the better bet.

---

## Part 1 — Glover (real source): patterns worth adopting / recognising

### 1.1 Debug / guard macros (`debug.h`) — the big one

A single switch (`GOLDCD` / `RELEASE`) compiles all the debug scaffolding to
nothing for the shipping build. This is the mechanism that explains *why
release PSX binaries have no strings or asserts* even though the source was full
of them — directly relevant to interpreting what we see in the SLES-01090 ROM.

```c
// development build:
#define ASSERT(cond) if(!(cond)) { \
    psxPrintf("\nASSERT FAILED at line %d, file %s, expression: %s\n", \
        __LINE__, __FILE__, #cond); debugAssert(); }

#define VALIDATE(p) if( <pointer-range checks vs HEAPBASE/STACKTOP/DCACHE> ) \
    { DB("\nPOINTER (%p) UP THE DUFF at line %d, file %s\n", p, __LINE__, __FILE__); CRASH; }

#define DB   debugPrintf      // __line=__LINE__, strcpy(__file,__FILE__), debugPrintf2
#define PRINTNUM(V)  DB("VAR "#V"=(%d) ", V);
#define PRINTVECT(V) DB("VECTOR "#V"=(%d,%d,%d) ", (V)->vx,(V)->vy,(V)->vz);

// gold/release build: every one of the above becomes empty
#define ASSERT
#define VALIDATE
#define DB
#define PRINTF  //printf      // commented-out printf = compiles to nothing
```

Takeaways:
- **`ASSERT(0)` / `ASSERT(0); // should never happen`** is heavily used as an
  unreachable-branch marker. In our disassembly, an original `ASSERT` leaves
  *no trace* in the release ROM — so a C source that drops an assert can still
  byte-match. Conversely, a "default: unreachable" branch in the asm may have
  been an `ASSERT(0)` that the release build elided. Good mental model when a
  switch has a mysterious empty default.
- The `__line=__LINE__, strcpy(__file,__FILE__), debugPrintf2` comma-operator
  trick: debug builds smuggle file/line into globals before each printf. Not
  needed for matching, but explains odd global writes near logging in *debug*
  builds (not ours).
- `POLLHOST()` = `asm("break 1024")` on dev, empty on gold. A bare `break`
  in disassembly is often this.

### 1.2 Custom block-pool allocator (`memory.c`) — *not* malloc

Glover does **not** use PSY-Q `malloc`. It carves a single pool and hands out
blocks from the **top of the pool downward**, tracked in a fixed array
(`MAX_ALLOCS = 400`) of `{offset, size, inuse}` records.

```c
#define MALLOC(A,B)   memoryDebugAllocate((A),(B),__FILE__,__LINE__)  // debug
#define MALLOC(A,B)   memoryAllocate(A)                                // release
#define NEW(T)        MALLOC(sizeof(T), #T)
#define FREENULL(A)   { memoryFree(A); (A) = NULL; }
#define DELETE(P)     FREENULL(P)
```

- Debug allocator stores `desc[10]` (the `#T` stringised type), `file[10]`,
  `lineno` per block, then on `memoryDestroy()` prints **peak usage** and walks
  the array reporting **every leaked block with its origin file/line** + a hex
  dump of its first 32 bytes. This is a textbook custom leak tracker.
- There's a `tempused`/`tempsize` split: a *second* allocation arena growing
  from the other end for short-lived (`TEMPMALLOC`) allocations.
- **Relevance:** Skullmonkeys very likely has an equivalent bespoke allocator
  (block array, alloc-from-top, temp split). If we see a fixed-size struct
  array of `{offset,size,flag}` triples and pointer arithmetic of the form
  `base + poolsize - size - offset`, that's this pattern, *not* a generic heap.
  The `(size + 3) & ~3` long-alignment rounding is also present.

### 1.3 Data-driven entity/enemy tables (`enemies.c`) — *very* relevant

Every enemy "type" is a row in a static table of behaviour descriptors holding
**function pointers** for interact/update plus tuning scalars and a collision-
sphere pointer:

```c
typedef struct ENEMYBEHAVIOUR {
    char           *fname;     // model/asset name string
    short           scale;
    int             radius;    // visibility radius
    int             hag;       // height-above-ground correction
    int             visdist;
    signed char     n_paths;   // -1 = variable, >=0 = required count
    short           flags;     // BLASTRING_STUNS | NME_SPELLABLE | DROPS_GARIB | ...
    NME_SPHERE     *spheres;
    NME_INTERACTROU *interact;  // fn ptr, may be NULL
    NME_UPDATEROU   *update;    // fn ptr, may be NULL
} ENEMYBEHAVIOUR;

ENEMYBEHAVIOUR nmeinfo[] = {
  // fname,     scale, r, hag, visdist, n_paths, flags,            spheres,        interact,   update
  { NULL,        256, 50,  0,  1500,    2,  0,                     NULL,           NULL,       NULL },        //0 camera
  { "MALLET",    256, 20, 10,  1000,    2,  BLASTRING_STUNS|NME_SPELLABLE|DROPS_GARIB, &MalletSpheres[0], NULL, Update_Mallet }, //10
  { "GENERALW",  256, 80, 20,  1200,    3,  BLASTRING_STUNS|NME_SPELLABLE, &GenWuSpheres[0], NULL, Update_GeneralWu }, //11
  ...
};
```

- Dispatch is `nmeinfo[nme->type].update(nme)` etc. The *type enum* indexes the
  table; entries 0–6 are reserved/engine pseudo-types (camera, hand, ball,
  debris, bullet) — exactly the kind of "first N slots are special" layout we
  keep finding in Skullmonkeys entity tables.
- The runtime instance struct (`ENEMYPOS`) is separate from the static
  descriptor and begins with `struct tagENEMYPOS *next;` (intrusive linked
  list) followed by init pos / pos*4096 / vel / angles / model ptr / type /
  flags / **an embedded animation sub-struct** / a small bytecode cursor
  (`doing` = index into instruction list, `ticker` = time left) / a
  `void *extras` escape hatch for per-type collision/physics state.
- **This is almost certainly the shape of the Skullmonkeys Entity system**:
  a global behaviour/type table of `{init, update, destroy, render}` callbacks
  indexed by a type id, instances on an intrusive linked list, a `void*`/union
  for type-specific data, and a tiny per-entity "script cursor + timer". When
  documenting our `EntityTypeNNN_*` families, expect this `nmeinfo`-style row
  layout. Corroborates [[asset-hash-ids-not-ascii]] (the `fname` here is an
  ASCII asset name in *source*, but the shipped game looks it up by hash).
- Note the candid comments revealing N64→PSX porting (`"In the N64 source these
  are HAND and BALL but ball conflicted... Luv, Fred"`). Skullmonkeys is PSX-
  only, but the *engine lineage* (Neverhood/DreamWorks) may show similar
  porting scars.

### 1.4 `main()` discipline

```c
int main(void)  // DO NOT USE LOCAL VARIABLES IN MAIN! STACK POINTER CHANGES!!!!
{
    ... // memset DCACHE, SetMem(2), ResetCallback(), memoryInitialise(...),
        // fileInitialise, CdInit, seedRandomInt, Set_Up_Display, sfxInitialise,
        // textureInitVRAM, VSyncCallback(&VblCallback), then asset preloads,
        // then gameMainGame(level) / menuSpinHub();
}
```

- `main` is a flat init script with **no locals** (stack base relocated during
  boot). If our `main`/entrypoint disassembly looks like a long straight-line
  sequence of setup calls with no stack frame for locals, that's by design.
- Order observed: clear scratchpad → `SetMem`/callbacks → memory pool → files →
  CD/audio → RNG seed → display → poly lists → SFX → VRAM → fonts → generic
  asset bank → enter game. A useful template when reconstructing our boot
  sequence ([[callback-install-map]] territory).

### 1.5 Misc idioms catalogued (for recognition in disasm)

- Type vocabulary: `BYTE/UBYTE/SHORT/USHORT/LONG/ULONG/VOID`, `BOOL`/`BOOLEAN`/
  `Boolean` all = `unsigned char`. `YES/NO`, `ON/OFF`, `INT` aliased to
  `short`. Fixed-point: `FRACTIONAL_BITS 12`, `FRACTIONAL_DIV 4096`,
  `ANG2FIX(a) = (a*4096)/360`, `PER2FIX`, `SAMESIGN(a,b)` via XOR sign test.
- PAL/NTSC handled with `#if PALMODE` switching constants (e.g.
  `GLOBALSPEED 7372` PAL vs `6144` NTSC; `SCREENYLIMIT (120+PALMODE*8)`). We are
  PAL (SLES) — expect the *PAL-valued* constants, and watch for `*8` /
  `+PALMODE*8` style adjustments baked into the numbers.
- `SCREENOFF/SCREENON` = `SetDispMask(0/1)`; `HALT` = `VSync(0)`;
  `CRASH` = `while(0==0)POLLHOST();` (deliberate hang).
- Semi-transparency enum `SEMITRANS_NONE/SEMI/ADD/SUB = 0..3` maps to GPU ABR.
- Big hierarchical model structs (`NEWOBJECT`/`NEWMODEL`/`MODELCTRL`) carry a
  matrix stack (`MAXMATRIXSTACK 11`) and quaternion rotation state — the
  animation system is quaternion keyframe based (`SQKEYFRAME` = short quat +
  short time). Relevant when reversing our anim/object draw code.
- Pad bit masks defined as `(1<<n)` literally in `glover.h` — matches the
  raw masks we see rather than PSY-Q's `PAD*` enum.

---

## Part 2 — Soul Reaver decomp: workflow/process patterns to borrow

This project does several things cleanly that we could adopt or compare against.

### 2.1 `include_asm.h` — the shelve macro, generalised

Their `INCLUDE_ASM`/`INCLUDE_RODATA`/`FORCE_RODATA_ALIGNMENT` are gated so the
*same source* compiles three ways via `-D`:
- normal build → emits the `.s` include (our maspsx-keep equivalent),
- `SPLAT` / `__CTX__` / `PERMUTER` / `SKIP_ASM` defined → macros expand to
  nothing (for ctx generation, permuter, m2c).

We already have the `maspsx-keep` INCLUDE_ASM ([[btm-matching-workflow]]); worth
confirming ours is similarly guarded for the ctx/permuter passes so a single
toggle suppresses asm includes.

### 2.2 `NON_MATCHING` convention for "matches on decomp.me, not in-tree"

```c
#ifndef NON_MATCHING
INCLUDE_ASM("asm/nonmatchings/Game/FX", FX_GetHealthColor);
#else
long FX_GetHealthColor(int currentHealth) { ... }
#endif
```

A clean way to **keep the C body in the repo for documentation even when it
doesn't byte-match the surrounding TU**, without breaking the build. Better than
deleting a near-match back to pure asm. Candidate for our shelving workflow:
shelve to asm for the build, but retain the best C under `#else` so the next
person starts from the 95% version instead of scratch.

### 2.3 Debug-symbol-driven decompilation (their killer advantage, our gap)

Soul Reaver is decompiled from a **prototype with full `.SYM` debug info**.
They run `psx_mnd_sym` to emit a `decls.h` giving, per function: signature,
every local + its **register/stack slot**, inner scope boundaries, and even
source line numbers:

```c
// address: 0x800B6840   line start: 1353
void SAVE_SaveEverythingInMemory() {
  // register: $s0  size: 0x29C
  register struct _Instance *instance;
  ...
}
```

We have *no* such symbols (clean-room). But their **"Resolving Stack Variables"
method works without symbols** and is worth codifying in our
decompilation-guide: stack var `spXX` lives at `XX`; `XX = stackSize - declOffset`;
extra `spYY` between named vars are *fields of the struct above them* — read
struct sizes from our `TYPES.h` equivalent to fold `sp42/sp44` into `pos.y/pos.z`.
Cross-check against [[btm-matching-workflow]] decl-order register-coloring.

### 2.4 Their `DECOMPILATION.MD` pattern catalogue (mostly known, a few sharp)

A genuinely good reference; the non-obvious bits worth keeping:
- **Clearing bitflags**: an AND with a high constant like `&= 0xF7FF` should be
  rewritten `&= ~0x800` — the *complement* is what the dev wrote. We should
  normalise these in our C for readability (doesn't affect match).
- **Fixed-point divide signature**: `x += (1<<n)-1; x >>= n;` (the `if(x<0)
  x += 0xFFF; x >>= 12;` shape) is just `x / 4096`. Compiler re-emits the
  add/shift; write the division. Confirms our `/4096` reading of "random
  add-then-shift" sequences.
- **`abs`**: `if(a<0)a=-a;` → `abs(b)`.
- For/while loop canonical de-gotoing (the `if(cond){do{...}while(cond);}`
  shapes) — standard m2c cleanup, same as ours.
- **GTE macros**: stray `mtc2/mfc2/ctc2/swc2` / m2c "unknown instruction" =
  a `gte_*` macro from PSY-Q; reference the rabbitizer `gte_macros.h`. (We have
  `gte_macros.inc` already.)

### 2.5 Style/process conventions

- **Offset-annotated structs** in `TYPES.h`: every field carries
  `// offset: 0x0004` and structs carry `// size: 0x6`. Makes the
  stack-var math trivial and serves as living documentation. Our `common.h`/
  `Game/` headers could adopt per-field offset comments for the structs we've
  reversed.
- **Label non-canonical code**: variables not from debug info get
  `// not from decls.h`; reorder-only ops get `do {} while (0); // garbage code
  for reordering`. We should similarly flag *guessed* names and
  match-only hacks so future readers don't trust them (echoes
  [[no-authoritative-source]] and [[effects-epilogue-gluing]]).
- **One match per commit**, fix all new warnings (implicit decl / unused) before
  contributing. Matches our "every commit keeps `make check` green" rule.
- **Allman braces**, enum-per-line. Cosmetic, but they keep it "canonical to the
  original".
- `_S`-suffixed functions = hand-written PSY-Q assembly, deliberately left in
  asm (not compiler output). We have an equivalent class (PSY-Q lib stubs / the
  `NullStubFunction` epilogue-sharing case in [[effects-epilogue-gluing]]) —
  same principle: don't try to "C-ify" hand asm.

---

## Highest-value actions for *our* project

1. **Treat `nmeinfo[]` as the template for our Entity type table.** When
   documenting `EntityTypeNNN_*` / `EntityDestroyCallback_*` families, look for
   a static descriptor row of `{assetName?, scalars, flags, sphere*, interact*,
   update*}` and an instance struct led by a `next` pointer + pos(*4096) + a
   `doing`/`ticker` script cursor + `void *extras`.
2. **Expect a bespoke block allocator**, not malloc: fixed `{offset,size,inuse}`
   array, alloc-from-top, long-aligned sizes, optional temp arena.
3. **Read empty/unreachable default branches as elided `ASSERT(0)`** and bare
   `break` as `POLLHOST`/debug-only — they vanish in the release ROM, so C that
   omits them can still match.
4. **Adopt the `NON_MATCHING` `#ifndef`/`#else` shelve** so near-matched C stays
   in-tree as documentation instead of reverting to pure asm.
5. **Codify the stack-var = `stackSize - declOffset` method** (and offset-
   annotated structs) in `docs/decompilation-guide.md`.
6. **Assume PAL-valued constants** with `+PALMODE*8`-style baked adjustments.

(External read-only study; nothing in this repo was modified.)
