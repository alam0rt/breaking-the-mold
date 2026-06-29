---
title: "Code-Structure TODO / Cleanup Backlog"
category: root
tags: [todo, cleanup, structure, naming]
---

# Code-Structure TODO / Cleanup Backlog

**Last Updated**: June 29, 2026
**Purpose**: Track *code-structure* and *naming* debt in `src/` — things we already
understand but haven't named, consolidated, or organized.

This is **distinct from [KNOWLEDGE_GAPS.md](KNOWLEDGE_GAPS.md)**, which tracks what we
*don't understand yet*. Items here are mostly understood; they need cleanup, not research.

> **Clean-room caveat:** every name/offset below is an inferred hypothesis, not an
> original symbol. Re-verify against the disassembly (and runtime traces where possible)
> before acting. Compiler flags and byte-matching constraints mean some "ugly" structure
> (e.g. per-file struct overlays) may be a deliberate matching crutch — confirm codegen is
> preserved before any refactor.

---

## 1. Per-file "entity view" struct overlays (highest structural debt)

Most gameplay files privately re-declare their own `typedef struct { u8 padXX[..];
sX field_YY; }` overlay onto the **same** underlying entity object, instead of using the
canonical `Entity` in `include/Game/entity.h`. The real layout is being rediscovered
file-by-file.

Files declaring local pad+`field_XX` overlays (count of `padXX[]` arrays):

| File | pad-arrays | `field_XX` refs |
|------|-----------:|----------------:|
| `effects.c`  | 38 | 0   |
| `enemies.c`  | 30 | 0   |
| `anim.c`     | 19 | 107 |
| `bosses.c`   | 17 | 0   |
| `entinit.c`  | 14 | 0   |
| `finn.c`     | 6  | 0   |
| `ending.c`   | 6  | 0   |
| (others)     | …  | …   |

Totals across `src/`: **109 `field_XX`** and **38 `unkNN`** un-named struct members.

Clearest case: **`EntityAccessorView`** (`anim.c:528`) — a ~0xF8-byte struct described
almost entirely with `padXX`/`field_XX`. It *is* the `Entity`, viewed through a keyhole
(only the touched offsets are typed).

**Action:** experiment with collapsing these local overlays into the canonical `Entity`
struct, field-by-field. **Must preserve byte-match** — verify codegen per file, not a
blind global swap. Naming the offsets resolves dozens of `field_XX`/`unkNN` at once.

---

## 2. Unnamed accessor "vtable" — 58 `func_` trampolines in `anim.c`

`anim.c` has **58 matched, fully-commented, but unnamed** accessor trampolines
(`func_8002xxxx`). The plate comments themselves say *"First/Second/Third of three
identical accessor sets"* (`src/anim.c:574, 619, 655`, starting at `func_800200DC`
`src/anim.c:580`).

Verified: the same field writes each repeat 3–4× in address order —
`field_1C`/`field_20`/`field_24` (×4), `field_30`/`field_32`/`field_3A`/`field_3B` (×3).

**Three identical accessor sets in address order = a function-pointer table** (variant
vtables sharing one accessor interface, or one indexed dispatch table). It's decompiled
but never *recognized* as a structure, so it reads as 58 noise functions.

Smaller instances of the same shape:
- `spracc.c` — 22 `RenderSprite` accessors (`func_80018xxx`, comments already good)
- `main.c`   — 22 `GameState` accessors (already richly commented: `boss_defeated`, `checkpoint_powerup_state`, …)
- `finn.c`   — 23

### Consumer investigation (2026-06-29)

Searched for what indexes/installs these accessors. Findings:

- **Zero name references** to any cluster `func_8002xxxx` symbol anywhere in `src/` or
  `docs/` outside their own definitions in `anim.c`. So no *matched* C code calls them by
  name — the consumer is in **unmatched asm** (which references by address) or in a
  **data table** (which stores the raw address).
- The cluster funcs are **not in `symbol_addrs.txt`** either, so nothing has wired them up
  as named table entries yet.
- **Verification is blocked in this checkout:** `asm/` is not extracted and the base ROM /
  `bin/SLES_010.90` is gitignored (commit `8dd793c`). The decisive search — scanning the
  binary's `.data` for the little-endian accessor addresses (e.g. `0x800200DC` → bytes
  `DC 00 02 80`) and grepping extracted `asm/**/*.s` for `.word func_8002…` — **could not
  be run here**. Run it in a checkout that has run `make extract` + `./tools/download-toolchain.sh`.

**What the C *does* tell us** (no binary needed): the accessors are FSM/render **callback-slot
targets**, not a classic indexed vtable. Per [fsm-callback-patterns.md](systems/fsm-callback-patterns.md),
the engine dispatches through 8-byte `[marker, fn]` slots (`Entity+0x1C/0x20` render,
`+0x24/0x28` & `+0x2C/0x30` movement-transform X/Y, `SpriteEntity+0x94..0xAC` animated
state). The cluster's own comment (`anim.c:576`) says it plainly: *"callers hold its
address (slot-installer functions wire them into vtables / FSM tables so they can be
invoked through a uniform fn-pointer signature)"*. The `S32Pair` setters
(`func_800201B0`→`field_98`, `func_800205AC`→`field_2C`, `func_80020668`→`field_24`,
`func_80020724`→`field_1C`; `anim.c:706,963,1011,1050`) are exactly the writes that
**install** an 8-byte `[marker, fn]` pair into those slots. So the accessors are the
per-field get/set leaves invoked *via* the slot machinery, installed by sibling
slot-writer functions.

The three identical sets (`+0x1C..+0x3B`, starting `func_800200DC`/`80020124`/`80020160`)
most likely correspond to the **three movement/render slot pairs that share the same field
window** (render `0x1C/0x20`, move-X `0x24/0x28`, move-Y `0x2C/0x30`) — one accessor set
per slot, all touching the same offsets. Confirm against the actual installer once asm is
available.

Vtable tables themselves live in the **`INIT_TABLES` data block** (`D_80010xxx`–`D_80012xxx`,
e.g. `PRIM_OBJECT_VTABLE = D_80010344`, `BOSS_ENTITY_VTABLE = D_800112E8`) — that's the
range to scan first for the accessor addresses.

**Action:**
1. In an extracted checkout, scan `.data`/`INIT_TABLES` (`D_80010xxx`–`D_80012xxx`) and all
   `asm/**/*.s` for the 47 cluster addresses — that pins the exact installer/table.
2. Identify the slot-installer that wires set+get pairs (look for callers of the `S32Pair`
   setters above) — that names the three-set grouping definitively.
3. Name each accessor by role (`GetRenderCallback`, `SetMoveMarkerX`, `GetClut`, …) and
   add to `symbol_addrs.txt`.
4. Reify the table as a named array once its consumer is found.

Note: **206 `func_XXXXXXXX` placeholders** exist in `src/` overall, but most are matched
bodies with good comments — the missing step is *naming*, not decompilation.

---

## 3. Subsystem name prefixes that span files — verify ownership, don't assume misfiling

Cross-file grep shows these subsystem prefixes defined in more than one file. **Address
check shows most are correctly filed by ROM order** — the shared *name prefix* is what's
misleading, not the file assignment. Confirm each before moving anything.

| Prefix | Files | Addresses | Assessment |
|--------|-------|-----------|------------|
| **ShrineyGuard** | `bosses.c`, `clayball.c` | bosses `0x8004Bxxx` (`ShrineyGuardMoveCallback`, `ShrineyGuardDeathState`); clayball `0x80059xxx` (`ShrineyGuardSoundUpdateTick`, `ShrineyGuardEventWithSound`, `ShrineyGuardDestroyWithSoundCleanup`) | **Different ROM regions** → likely two distinct variants, filed correctly. Probably a *naming* clarity issue (boss vs sound-emitting variant), not misfiling. |
| **Finn** | `finn.c`, `enemies.c` | enemies `CollectibleTickFinnMode 0x8003AEF4`, `FinnModeCollectibleTickCallback 0x80046A2C`; finn.c proper `0x8006Exxx` | enemies-range funcs are filed by address correctly; name just carries "Finn". Naming overlap, not misfiling. |
| **Collectible** | `bosses.c` (6), `enemies.c` (14), `pickups.c` (12) | spread | Three-way split — confirm whether each cluster is ROM-contiguous with its file before deciding. |
| **Decor** | `effects.c`, `pickups.c` | — | Verify ROM ranges. |

**Action:** for each, confirm contiguity via `symbol_addrs.txt`. If contiguous with its
file → rename for clarity (disambiguate the variant). Only relocate if a function is
genuinely stranded outside its sibling's ROM range.

---

## 4. Stopgap / hash names to retire

- **`InitEntity_168254b5`** (`effects.c:544`, `0x80034BB8`) — hex-hash name that escaped
  the naming pass. Identify the init's role and rename.
- **`EntityEventHandler0x1001_1002_1008`** + `_V2` (`enemies.c:122-123, 482, 488`,
  `0x8003BB84` / `0x8003BC74`) — encodes 3 event IDs in the name. Fine as a stopgap;
  signals an un-split multi-event dispatcher. Name by behavior once understood.

---

## 5. Low priority — PSY-Q library stubs (expected)

`libs/libcd.c` retains ~8 `*_OBJ_NNN` / `func_` stubs (`EVENT_OBJ_84`, `CDREAD_OBJ_810`,
`ISO9660_OBJ_650`, …). These are PSY-Q library internals — lowest priority, expected to
stay opaque until matched against the SDK.

---

## Suggested order of attack

The two biggest items share a thread: **start with #2 (anim.c accessor table)** — locate
its consumer/table, name the set, then collapse `EntityAccessorView` into the canonical
`Entity` (#1). That single effort touches both top items. #3–#5 are independent and can be
picked up opportunistically.
