---
title: "lint-clarity follow-up plan"
category: plans
tags: [plans, lint, clarity, followup]
---

# lint-clarity follow-up plan

Date: 2026-06-19

Goal: clear the remaining `make lint-clarity` findings in dedicated, byte-match-safe batches.

## Current inventory

Total remaining findings: **1148**

### By rule

- 524 `no-void-pointer-params`
- 246 `no-byte-pointer-arithmetic`
- 207 `no-raw-address-symbols`
- 106 `no-void-pointer-return`
- 65 `no-entity-cast-field-access`

### By file

| Findings | File | Planned pass |
|---:|---|---|
| 390 | `src/entinit.c` | Spawn wrapper/entity allocation pass |
| 126 | `src/effects.c` | Effect entity layout + vtable pass |
| 82 | `src/enemies.c` | Remaining enemy layouts and raw symbols |
| 77 | `src/menu.c` | Menu entity layouts and shared menu globals |
| 72 | `src/libs/libcd.c` | PSY-Q CD API typed wrapper pass |
| 69 | `src/blbacc.c` | BLB access/context structures |
| 64 | `src/pickups.c` | Pickup/destructor entity families |
| 61 | `src/finn.c` | FINN/vehicle entity family pass |
| 56 | `src/bosses.c` | Boss destructor/state symbol pass |
| 51 | `src/ending.c` | Ending entity destructor/layout pass |
| 48 | `src/clayball.c` | Clayball destructor/layout pass |
| 40 | `src/anim.c` | Animation entity/resource layout pass |
| 12 | `src/entity.c` | Core sprite-context callback layout pass |

## Batch order

1. `src/entity.c` — smallest, but includes callback-offset dispatch; only model what is safe.
2. `src/anim.c`, `src/clayball.c`, `src/ending.c` — moderate destructor/resource cleanup.
3. `src/bosses.c`, `src/finn.c`, `src/pickups.c` — known entity-family cleanup.
4. `src/blbacc.c`, `src/libs/libcd.c`, `src/menu.c` — subsystem globals/APIs.
5. `src/enemies.c`, `src/effects.c` — larger entity layouts.
6. `src/entinit.c` — final repetitive spawn wrapper pass, likely with helper macros/typed aliases.

## Commit cadence

For each batch:

1. Run targeted `ast-grep scan --config sgconfig.yml --report-style short <files> | cat`.
2. Make a small cleanup patch.
3. Run targeted diagnostics.
4. Run `make check` and confirm SHA1 byte-match.
5. Commit only the files touched by that verified batch.

Do not include unrelated untracked research scripts/artifacts in cleanup commits.

## Completion criteria

- `make lint-clarity` reports zero actionable findings, or remaining findings are documented/suppressed as intentional false positives.
- `make check` passes with expected SHA1.
- Final commit records the clean state.
