# Plan: `bare-include-asm` ast-grep rule — a guide for full C decompilation

Status: proposed (2026-06-20)
Owner: decomp tooling
Depends on: adopting the `NON_MATCHING` convention (docs/matching-conventions.md)

## Goal

Add an ast-grep rule that flags every `INCLUDE_ASM(...)` stub that has **no C
attempt** — i.e. a bare stub *not* paired with a `NON_MATCHING`-guarded C body.
This turns the asm backlog into a self-maintaining, greppable TODO list for
"functions that still need to be decompiled to C from scratch," distinct from
"functions that have C but don't byte-match yet."

## Why this distinction matters

Borrowed from soul-re / ffvii: an unmatched-but-attempted function keeps its C
behind a guard instead of being deleted:

```c
#ifndef NON_MATCHING
INCLUDE_ASM("asm/nonmatchings/effects", SomeFunc);   // active build path
#else
void SomeFunc(Entity *e) { /* best-effort C, doesn't match yet */ }
#endif
```

So there are three states a function can be in:

| State | Source shape | Meaning |
|---|---|---|
| Matched | plain C, no `INCLUDE_ASM` | done |
| Attempted | `INCLUDE_ASM` inside a `NON_MATCHING` guard | C exists, needs a match nudge |
| **Untouched** | **bare `INCLUDE_ASM`** | no C exists yet — the real "decompile me" list |

`bare-include-asm` flags only the third row.

## Current repo reality (grounds the plan)

- **873 `INCLUDE_ASM` stubs across 35 files** in `src/` (2026-06-20).
- **`NON_MATCHING` is not used at the C level yet.** (`include/labels.inc` /
  `include/macro.inc` define a `.NON_MATCHING` *asm label* — a different
  namespace, splat/asm-differ machinery, not a cpp macro. Confirm no cpp
  `-DNON_MATCHING` leaks into the build before adopting the C convention.)
- Consequence: **until we adopt `NON_MATCHING`, the rule flags all 873 stubs.**
  That's pure noise inside `make lint-clarity`. The rule must ship as a
  **separate, opt-in report**, and its signal-to-noise improves as guards are
  added.

## Prototype (validated)

This already works against tree-sitter-c's preprocessor nodes:

```yaml
id: bare-include-asm
language: C
severity: hint
message: "Bare INCLUDE_ASM with no C attempt; this function still needs full decompilation (or wrap a best-effort attempt in `#ifndef NON_MATCHING`)."
rule:
  kind: call_expression
  has: { field: function, regex: '^INCLUDE_ASM$' }
  not:
    inside:
      kind: preproc_ifdef
      stopBy: end
      has: { field: name, regex: 'NON_MATCHING' }
```

Verified behavior on a fixture:
- flags a bare `INCLUDE_ASM(...)` ✓
- flags an `INCLUDE_ASM` inside an unrelated `#ifdef SOME_OTHER_GUARD` ✓
  (only `NON_MATCHING` guards are exempt, not any ifdef)
- does **not** flag `INCLUDE_ASM` inside `#ifndef NON_MATCHING` ✓
- the ffvii `#ifdef NON_MATCHING <C> #else INCLUDE_ASM #endif` form is also
  exempt, because the `#else` branch is still inside the same `preproc_ifdef`
  node ✓

## Refinements before shipping

1. **`#if !defined(NON_MATCHING)` variant.** `#ifndef`/`#ifdef` are
   `preproc_ifdef` (matched above). `#if !defined(NON_MATCHING)` is a
   `preproc_if` with an expression condition. Add a second `not: inside`
   alternative for `kind: preproc_if` whose condition text matches
   `NON_MATCHING`, or settle on `#ifndef` as the only sanctioned form in the
   convention doc and lint for that.
2. **`INCLUDE_RODATA` / other `INCLUDE_*` macros.** Decide whether they count.
   Default: rule targets `INCLUDE_ASM` only; rodata stubs are not "functions to
   decompile."
3. **Severity + placement.** Use `severity: hint`, but **do not** add it to the
   default `sgconfig.yml` ruleDirs scan (`make lint-clarity`) while it would
   emit ~873 hits. Options:
   - dedicated target `make decomp-todo` → `ast-grep scan -r tools/ast-grep/rules/bare-include-asm.yml src`,
   - or keep the rule file outside `tools/ast-grep/rules/` (e.g.
     `tools/ast-grep/reports/`) so the clarity dir stays clean, and point the
     target at it explicitly.
4. **Counts / progress view.** Wrap a count form for dashboards:
   `ast-grep scan -r ... src --json | jq 'length'`, and optionally a per-file
   breakdown to pick the next file to finish.

## Acceptance criteria

- Rule parses (`ast-grep scan -r <file>` with no parse error).
- On a fixture covering all three states + an unrelated `#ifdef`, it flags
  exactly the bare stubs.
- Running the dedicated target prints a stub count that matches
  `grep -rc INCLUDE_ASM src` minus any `NON_MATCHING`-guarded stubs.
- It is **not** wired into `make lint-clarity` (no new noise there).
- README (`tools/ast-grep/README.md`) documents it under a "reports" heading,
  separate from the always-on clarity/convention checks.

## Rollout order

1. Land the `NON_MATCHING` convention in `docs/matching-conventions.md` (done)
   and define the single sanctioned guard form (`#ifndef NON_MATCHING`).
2. Confirm the build is clean w.r.t. a C-level `NON_MATCHING` macro (no
   accidental `-D`).
3. Add `bare-include-asm.yml` + a `make decomp-todo` target.
4. As functions get attempted, wrap them in the guard; the report shrinks to
   the genuine untouched backlog.

## References

- docs/matching-conventions.md — `NON_MATCHING` preservation + reader-patterns.
- tools/ast-grep/README.md — existing rule conventions and the `message:`
  quoting gotcha.
- soul-re CONTRIBUTING ("Function Matches on Decomp but not When Building"),
  ffvii `src/boot/18B4.c` (`#ifdef NON_MATCHING` example).
