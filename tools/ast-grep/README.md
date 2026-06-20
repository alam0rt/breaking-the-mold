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

Matching-convention checks (`warning`) — encode `docs/matching-conventions.md`:

- `no-callback-slot-macro`: flags `SLOT_STORE`/`SLOT_CLEAR` usage. Real PSX source (Glover) used simple one-line punning macros, never multi-statement armor blocks hiding barriers — expand these to explicit struct-value writes.
- `untagged-frame-pad`: flags a `volatile` **local** not preceded by an `// @hack`/match comment. `volatile s32 pad;` is the preferred frame-size armor, but it must be tagged so the load-bearing line is obvious. Module-level `volatile` (ISR/MMIO) is not flagged.
- `untagged-asm-barrier`: flags an inline-asm fence/register-barrier statement not preceded by an `// @hack`/match comment. Companion to the broader `no-inline-asm` hint: armor is acceptable scaffolding only if tagged.

The "untagged-*" rules use a `not: { follows: { kind: comment, regex: 'hack|HACK|@|match' } }` clause, so a barrier/pad with a `// @hack: <why>` (or any `@`/`match`/`hack`) comment on the line directly above is treated as compliant and not flagged.

> YAML gotcha when authoring rules: a `message:`/`note:` value containing `: ` (colon-space, e.g. `// @hack: <why>`) **must be quoted**, or ast-grep fails with "mapping values are not allowed in this context."
