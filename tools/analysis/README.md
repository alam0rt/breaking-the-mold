# Analysis build (advisory)

A second, throwaway compile of the C source with a **modern** compiler, whose
only product is diagnostics. It never produces bytes, is never linked, and
**never gates the matching build**. Think of it as `go vet` for the decomp.

The matching build (cc1-psx 2.7.2, frozen flags) remains the sole source of
truth: `make check` SHA1s the ROM. Nothing here can change that output.

## Why two tracks

| Track | Compiler | Purpose | Gates? |
|-------|----------|---------|--------|
| Matching build | cc1-psx 2.7.2, frozen flags | Produce the ROM; SHA1 is truth | **Yes** (`make check`) |
| Analysis build | modern clang, `-fsyntax-only` | Diagnostics + layout invariants | Layout: yes. Warnings: advisory |

## The ABI pin (critical)

The analysis compiler **must** target `mipsel-unknown-none-elf`:

```
clang -target mipsel-unknown-none-elf -std=gnu11 -fno-short-enums ...
```

The game structs contain 141 pointer fields. On the PSX target a pointer and a
`long` are **4 bytes**; on an arm64/x86-64 host they're 8. Compile with the host
ABI and every `sizeof(struct-with-a-pointer)` assertion fails — not from drift,
but because the host doubled every pointer. `-fno-short-enums` makes
enum-in-struct width (4 bytes) agree with cc1 2.7.2, which matters the moment an
enum lands inside a struct.

No `psyq/` headers are needed: `common.h` gates the PSY-Q includes behind
`__has_include`, and no PSX GPU type (`SPRT`, `POLY_*`, ...) is used *by value*
in any game struct (they're byte-pool arrays / pointers), so `sizeof` and
`offsetof` are computable without them.

## What runs

`make analyze` runs `tools/analysis/run_analyze.sh`, which has two tracks:

### 1. Layout asserts — HARD gate

`gen_layout_asserts.py` reads the `/* 0xNN */` field-offset comments and
`/* Size: 0xNN */` struct-size comments in `include/Game/*.h` and emits one
`*.asserts.c` per header (into `asserts/`, gitignored) full of:

```c
_Static_assert(offsetof(PlayerEntity, worldX) == 0x68, "...");
_Static_assert(sizeof(PlayerEntity)          == 0x1B4, "...");
```

Each TU is compiled `-fsyntax-only`. `_Static_assert` is evaluated during sema,
so this verifies layout with zero codegen. **A failure means a struct's
documented layout no longer matches what the compiler computes** — i.e. a
header edit shifted a field or resized a struct. This track exits non-zero.

One TU per header (not one big TU) because 5 typedef names legitimately collide
across headers (`SpriteContext`, `SpriteRenderContext`, `DecorSpawnData`,
`HazardTimerEntity`, `SpriteContextCallbackTable`).

### 2. Warning sweep — ADVISORY

Every `src/*.c` is compiled `-Wall -Wextra` (+ `-Wshadow`,
`-Wimplicit-fallthrough`) with the decomp-noise classes suppressed (see the
`WARN_FLAGS` comment in `run_analyze.sh`). In a faithful decomp most warnings
are *working as intended* — reproductions of the original's quirks — so this
track **never fails**. The product is the **delta** vs
`warnings.baseline.txt`: a warning that appears after an edit but wasn't there
before.

## Usage

```bash
make analyze            # regenerate asserts, gate on layout, report warning delta
make analyze-baseline   # re-snapshot warnings.baseline.txt (after an intentional change)
```

Workflow: run `make analyze` after any header edit. If the layout track fails,
your edit shifted a struct — fix it before burning a matching-build cycle. If a
new warning appears, decide whether it's a real regression or an intended
reproduction (if intended, re-baseline).

## Files

- `gen_layout_asserts.py` — comment → `_Static_assert` generator (committed)
- `run_analyze.sh` — the two-track runner (committed)
- `warnings.baseline.txt` — advisory warning snapshot (committed)
- `asserts/` — generated per-header assert TUs (gitignored; `make analyze` writes them)
