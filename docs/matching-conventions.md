# Matching-hack conventions (how to write byte-matching C without obtuse code)

When idiomatic C doesn't byte-match, we reach for "armor" (padding locals,
register/memory barriers, statement reordering). This doc is about **how to
apply that armor so the source stays readable and honest** — not *why* the
compiler needs it (that's `compiler-quirks.md`).

The guiding principle, validated against the
[silent-hill-decomp](https://github.com/Vatuu/silent-hill-decomp) project
(also PS1-era GCC), is:

> **Write idiomatic, documented C. When a match needs a nudge, use a single
> visible, clearly-tagged, localized hack — never an abstraction that hides the
> armor.**

Their repo is good evidence the cleanest path is *idiomatic C + a few honest,
labeled hacks*, not "a clever macro that happens to match." Note they use
gcc-2.8.1, we use gcc-2.7.2, so their exact incantations won't reproduce our
bytes — only the methodology transfers.

## Anti-pattern: armor hidden in a macro

`src/enemies.c` historically wrapped the FSM callback-slot install in
`SLOT_STORE`/`SLOT_CLEAR` macros. Call sites look clean, but:

- the mechanism (scratch struct, barrier, `m1=-1` placement) is invisible at
  the site, and
- the macro body itself is obtuse comma-operator soup.

Prefer expanding to explicit struct-value writes (the approach validated in
`src/effects.c`: `InitScrollingLayerEntity`, `InitPlayerDeathParticle`) and
tag any remaining armor at the site.

## Tag every hack

Use a consistent, greppable tag with a reason. Borrowed from SH's `// @hack`:

```c
volatile s32 pad;            // @hack frame size (see compiler-quirks.md 6 / frame)
do {} while (0);             // @hack basic-block boundary required for match
__asm__ volatile("" :::"memory"); // @hack pin store order before `la fn`
```

A future reader (or a cleanup pass) must be able to see *that* a line is
load-bearing and *why*, without re-deriving it from the diff.

## Preferred armor, least-surprising first

1. **Source-level shape** — declaration order, named temporaries, `goto out`
   for multi-early-out predicates, signed `s32` temps to force `lh+and` over
   `lhu+andi+sext`. Always try these before any barrier (see compiler-quirks.md
   Quirk 6 series).
2. **`volatile s32 pad;`** for a frame-size bump — one uninitialized volatile
   local, tagged. **Prefer this over inventing a padded struct *type***
   (`PaddedEntSlot`/`TripadSlot`): the wart is visible and local instead of
   masquerading as a real type. (SH uses exactly this, tagged
   `// TODO: better solution?`.)
3. **`do {} while (0); // @hack`** to force a statement/basic-block boundary
   when ordering is the only problem. **Empirically validated** as a drop-in
   replacement for the slot-installer fence + fn-barrier pair across ~25
   functions in enemies.c / effects.c / pickups.c / menu.c / bosses.c
   (2026-06-23). See `compiler-quirks.md` Quirk 6k for the full pattern.
4. **Empty `__asm__ volatile` barriers** (`"":::"memory"` for store ordering;
   `"=r"(x):"0"(x)` for register coloring / la-hoist) — tagged, used only when
   1–3 can't do it.
5. **Real inline asm** (actual instructions) when a region is genuinely
   irreducible. SH writes the real instructions rather than scattering empty
   barriers — if you're already in `__asm__`, emitting the true ops can be
   clearer than nudging the compiler toward them.

## Struct/field naming (stop the raw offset casts)

Casts like `*((u8*)e + 0x84)` and `*(CallbackSlot*)&e->tickMarker` are how we
lose "what the original looked like." Adopt SH's offset-suffixed convention so
structs are self-documenting and layout drift is caught:

- `field_1C` — accessed, purpose unclear; `unk_324` — unknown/padding;
  `flags_328` — power-of-two flag word. Keep the **hex offset suffix** until a
  struct is fully understood, then drop it and add a `/** 0x1C */` comment.
- Use a real typed `CallbackSlot` field for the FSM slots instead of casting
  through `&e->tickMarker`, where the struct allows it.
- Lock known sizes with a `STATIC_ASSERT_SIZEOF`-style assert so a field edit
  that moves offsets fails loudly. (We don't have this macro yet — worth
  adding when we next touch `include/`, which requires explicit instruction.)

## Boolean / comparison conventions

Codegen is sensitive to comparison *shape* (our Quirk 6e). SH bakes this into
their conventions and so should we:

- Prefer `if (!x)` / `if (x)` over `if (x == 0)` / `if (x != 0)` **unless** the
  match requires the explicit form — some comparisons against a non-zero value
  must stay `x == N` to match. When you keep the verbose form for matching,
  tag it: `if (x == 1) // @hack matches; !x regresses`.

## Verification discipline (unchanged, restated)

- Per-function: `tools/fdiff.sh <rom_off_hex> <size_hex>` → MATCH.
- **Always `make clean && make check`** before trusting a result — incremental
  builds can show a stale false MATCH (see compiler-quirks.md "Measurement
  discipline").
- Never edit `asm/`; don't edit `include/` unless instructed.

## See also

- `docs/compiler-quirks.md` — *why* the compiler needs each nudge (cc1 codegen).
- `docs/ghidra/struct-workflow.md` — recovering real types/offsets to replace casts.
- effects.c worked examples; `docs/plans/effects-dispatch-family.md`.
