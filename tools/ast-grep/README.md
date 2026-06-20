# ast-grep clarity checks

This directory contains lightweight, non-blocking C clarity rules for the decompilation cleanup workflow.

Run:

```sh
make lint-clarity
```

Most rules are intentionally `hint` severity because the current codebase still has many known cleanup candidates. They are meant to point at the next readable refactors without breaking normal builds or existing `make lint` usage. The `make lint-clarity` target ends in `|| true`, so nothing here ever fails a build regardless of severity.

Clarity checks (`hint`):

- `no-void-pointer-params`: prefer concrete pointer parameter types over `void *` when layouts are known.
- `no-void-pointer-return`: prefer concrete factory/initializer return types over `void *`.
- `no-byte-pointer-arithmetic`: flag byte-cast offset arithmetic that likely wants struct fields.
- `no-scalar-offset-deref`: like the above but through `u16`/`s32`/etc casts.
- `no-entity-cast-field-access`: flag `((Entity *)x)->field` style access when the value should be typed directly.
- `no-raw-address-symbols`: flag direct `D_800xxxxx` C identifiers so they can become meaningful asm-labeled names.
- `no-volatile-cast`: `*(volatile T *)` reload hacks.
- `no-inline-asm`: any inline asm (broad cleanup lens — "could this be C?").
Reader-pattern checks (`hint`) — turn raw decompiler output into plausible original C (from the soul-re `DECOMPILATION.MD` catalog; see `docs/matching-conventions.md`). Split by codegen risk:

*Codegen-neutral (the rewrite compiles identically — safe readability wins, still fdiff to be sure):*

- `raw-inverse-bitmask`: `flags &= 0xF7FF` style bit-clears that should be `&= ~0x800`/`~FLAG`. Excludes `2^n-1` keep-low-bits truncation masks (`& 0xFF`, `& 0xFFFF`).
- `prefer-abs-macro`: `if (x < 0) x = -x;` → `x = ABS(x)` (the `ABS` macro in common.h; -fno-builtin means no libc call).
- `fixed-point-divide`: `if (x < 0) x += 0xFFF; x >>= 12;` (bias = `2^n - 1`) → `x / 4096`; gcc re-emits the same bias+shift for signed power-of-two division.

*Codegen-affecting (the current form may be load-bearing — re-verify with `make clean && make check`):*

- `manual-loop-reconstruction`: `if (c) { do { ... } while (c); }` → `while`/`for` (same condition enforced via metavar). The if+do-while form sometimes IS the matching one (e.g. hand-rolled libc), so this is a candidate only.

Matching-convention checks (`warning`) — encode `docs/matching-conventions.md`:

- `no-callback-slot-macro`: flags `SLOT_STORE`/`SLOT_CLEAR` usage. Real PSX source (Glover) used simple one-line punning macros, never multi-statement armor blocks hiding barriers — expand these to explicit struct-value writes.
- `untagged-frame-pad`: flags a `volatile` **local** not preceded by an `// @hack`/match comment. `volatile s32 pad;` is the preferred frame-size armor, but it must be tagged so the load-bearing line is obvious. Module-level `volatile` (ISR/MMIO) is not flagged.
- `untagged-asm-barrier`: flags an inline-asm fence/register-barrier statement not preceded by an `// @hack`/match comment. Companion to the broader `no-inline-asm` hint: armor is acceptable scaffolding only if tagged.

The "untagged-*" rules use a `not: { follows: { kind: comment, regex: 'hack|HACK|@|match' } }` clause, so a barrier/pad with a `// @hack: <why>` (or any `@`/`match`/`hack`) comment on the line directly above is treated as compliant and not flagged.

> YAML gotcha when authoring rules: a `message:`/`note:` value containing `: ` (colon-space, e.g. `// @hack: <why>`) **must be quoted**, or ast-grep fails with "mapping values are not allowed in this context."
