---
title: "Plan: de-shelve the GameState event-dispatch family in effects.c"
category: plans
tags: [plans, effects, dispatch, family]
---

# Plan: de-shelve the GameState event-dispatch family in effects.c

Status: proposed (2026-06-20)
Scope: `src/effects.c` — replace INCLUDE_ASM for a cluster of functions that all
inline the same "invoke a GameState event slot" dispatch, plus two adjacent
easy wins.

## Background

`effects.c` still has ~60 INCLUDE_ASM functions. An exploratory Ghidra-vs-asm
pass (REST decompiler on the headless server, `:8089`) found that several of them
are not independent — they share one inlined code shape, the **consumer side of
the entity-FSM slot convention** `{s16 markerLo, s16 markerHi, void(*fn)()}` that
the already-matched `Init*` functions install.

### The shared dispatch (read side of the slot convention)

```
hi = gs->eventMarker.markerHi            // *(s16*)((u8*)gs + 0x0A)
if (hi == 0)  return;                     // no handler
if (hi <  0)  fn = gs->eventCallback;     // direct-call sentinel (0xFFFF0000 path)
else {                                     // hi > 0: table lookup
    base = *(int*)((u8*)gs + gs->markerLo);   // markerLo @ +0x0C selects table
    entryLo = (s16) base[hi].lo;          // *(base + hi*8 - 8)
    fn      =       base[hi].fn;          // *(base + hi*8 - 4)
}
arg0 = (u8*)gs + (s16)gs->eventMarker;    // markerLo @ +0x08
if (hi > 0) arg0 += entryLo;
fn(arg0, 3, payload);
```

(Field offsets are from the asm: lh 0xA = hi, lh 0xC = table-base selector,
lh 0x8 = arg0 base, lw 0xC for the direct fn pointer. Confirm against the
`GameState` struct in `include/` before finalizing names.)

### Members of the family (event id is always 3)

| function | addr | size | guard | payload (3rd arg) |
|---|---|---|---|---|
| `NotifyGameStateZero`     | 0x80036214 | 0x90 | — | `0` |
| `NotifyGameStateOne`      | 0x80037434 | 0x80 | — | `1` |
| `InvokeGameStateCallback` | 0x80034B2C | 0x8C | — | `*(a3_entity + 0x20)` |
| `EntityDespawnIfFlagSet`  | 0x800361F8 | 0x1C | `entity->+0x7C != 0` | `0` |

`InvokeGameStateCallback` takes the source entity in **a3** (Ghidra's `void`
signature is wrong — register convention). `EntityDespawnIfFlagSet` is the same
body wrapped in a single `if`.

## CRITICAL constraint: layout is load-bearing

`NotifyGameStateOne` has **no stack frame** and ends with `jalr $t2` then falls
through into `NullStubFunction` (0x800374B4), reusing its `jr $ra` as the
epilogue. Its early-out is literally `beqz $a1, NullStubFunction`. So:

- `NullStubFunction` is **NOT** a PSY-Q SDK stub (a checked-in Ghidra plate
  comment claims this — it is wrong). It is game code whose `jr $ra` is shared.
- **Source/link order is part of the byte match.** `NullStubFunction` must be
  emitted immediately after `NotifyGameStateOne`. Do not reorder these or insert
  functions between them. Verify the others for similar epilogue-sharing before
  moving any definition.

## Approach

1. **Model the dispatch once.** Express the read-side dispatch as a single
   shape — preferably a `static inline` helper or macro in effects.c (mirroring
   the slot-install armor). The compiler must *inline* it into each of the four
   functions (they have no `jal` to a shared dispatcher in the asm), so this is
   an inline/macro, not a real function.
2. **Iterate per function with `tools/fdiff.sh`** (per-function objdump diff;
   valid even if whole-ROM check is dirty). Apply the same armor playbook as the
   `Init*` work where register coloring / signed-shift / la-hoist diffs appear.
   Expect the `(x*scale)>>16`-style signed temps and the `in_t0` ghost (the
   `entryLo` value carried in t0) to need a named local.
3. **Respect ordering.** Keep `NullStubFunction` adjacent to `NotifyGameStateOne`
   and confirm no other family member is epilogue-glued before defining it.
4. **Whole-ROM verify.** `make clean && make check` after each match — never
   trust an incremental MATCH (stale-object hazard; see compiler-quirks.md).

## Order of work

1. **Easy wins first (no slot armor, build confidence):**
   - `CheckVRAMSlotPixelColor` (0x80031DA0, 0x70) — libgpu `StoreImage` +
     `DrawSync(0)`, compare `0x3C0F`.
   - `IsEntityOutsideSpawnBounds` (0x800346F4, 0xD4) — parallax cull predicate;
     watch the signed `(x*scale)>>16`.
2. **The dispatch family** (find the shape on the simplest, then reuse):
   - `EntityDespawnIfFlagSet` (smallest, single-`if` wrapper) or
     `NotifyGameStateZero` first to lock the dispatch shape.
   - Then `NotifyGameStateOne` (+ keep `NullStubFunction` adjacent).
   - Then `InvokeGameStateCallback` (adds the a3 entity payload).
3. **Adjacent hash-named init** (separate effort, documented for later):
   - `InitEntity_168254b5` (0x80034BB8, 0xF0) — has hidden stack args
     (`stack[0x10]`, `stack[0x14]`) and an `a1` packed lo/hi payload; treat like
     `InitPlayerDeathParticle` (asm hides arity — trust Ghidra's signature).

## Verification checklist (per function)

- [ ] `tools/fdiff.sh <rom_off_hex> <size_hex>` prints MATCH
- [ ] no `asm/` edited; no `include/` edited unless instructed
- [ ] `make clean && make check` → `✓ BUILD MATCHES`
- [ ] layout/order of `NotifyGameStateOne` ↔ `NullStubFunction` preserved
- [ ] commit only on SHA match

## Risks / open questions

- Whether the dispatch inlines identically across all four under -O2 (register
  allocation may differ enough that one needs extra armor or a slightly
  different source shape). If a macro can't cover all four, fall back to writing
  each body explicitly with shared named locals.
- Exact `GameState` field names/offsets — verify against `include/` rather than
  trusting Ghidra's `mode_base_offset` / `eventMarker` guesses.
- Other effects.c functions may also be epilogue-glued; audit branch targets
  (`beqz ... <OtherFunc>`) before reordering anything.

## References

- Slot-install (write side) worked examples: `InitScrollingLayerEntity`,
  `InitPlayerDeathParticle` (already matched) — docs/compiler-quirks.md case study.
- Armor + measurement discipline: docs/compiler-quirks.md Quirk 6 series.
- Ghidra REST workflow: headless GhidraMCP server on `:8089`,
  `curl "http://127.0.0.1:8089/decompile_function?address=0x..."`.
