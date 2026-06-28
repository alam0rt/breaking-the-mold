# Function name/purpose audit

Clean-room name review (CLAUDE.md: every name is an inferred guess and may be
wrong). Two kinds of debt: **placeholder `func_NNNN` names** that need renaming,
and **missing/wrong purpose comments**. This doc maps both for deliberate
follow-up passes. Comments are added in-source as the sweep proceeds; renames are
batched separately (they ripple through `symbol_addrs.txt` + every caller).

## Broad sweep results (all src/*.c, decompiled C only)

**177 decompiled C functions still carry placeholder `func_NNNNNNNN` names.**
~84 are single-statement field accessors; most of the rest are 2–4-statement
compound accessors (read/write a few fields). Their *purpose* is self-evident
from the body — they don't need comments, they need **renaming** once the field
semantics are confirmed. They cluster by the struct they accessorize:

| File | `func_` count | Nature | Rename-batch handle |
|---|---|---|---|
| anim.c | 58 | `EntityAccessorView` field get/set vtable (+0x20..+0x3B, sprite/anim state) at sequential addrs `func_8002xxxx` | `AnimView_Get/Set<Field>` |
| spracc.c | 22 | `RenderSprite` accessors (color, tpage, vram coords, clut) `func_80018xxx` | `Sprite_Get/Set<Field>` |
| main.c | 22 | `GameState` field get/set `func_8008xxxx` | `GameState_Get/Set<Field>` |
| finn.c | 22 | mostly FINN entity accessors + a few real predicates (`func_8006EF48/64/84/A4`) | `Finn_*` |
| effects.c | 13 | mixed accessors + small init/destroy | per-function |
| ending.c | 9 | ending/credits sequence helpers | per-function |
| blbacc.c | 5 | `LevelDataContext` BLB-field readers | `LevelData_*` |
| bosses.c | 4 | boss state helpers | per-function |
| (sound/edtor/menu/entity/pickups/clayball/vehicle/vram/gamecd/emptycb/nullfn) | 1–3 each | one-offs | per-function |

**Recommendation:** the accessor clusters (anim/spracc/main/blbacc) are the bulk
of the debt and are best handled as **struct-by-struct rename batches** (derive
`Get/Set<FieldName>` from the touched offset + the struct's field names), not as
comment work. The non-accessor `func_` one-offs and the genuinely-misnamed
descriptive functions are the comment/flag targets below.

## Suspect descriptive names (not `func_`) — flagged for rename

| Function | File | Severity | Finding |
|---|---|---|---|
| `EntityEventHandlerWalk` | enemies.c | NIT | named for installing state; body is attack-token arbitration on +0x106/+0x108. |
| `EntityEventHandlerIdle` | enemies.c | NIT | same, idle-state token handler (+ EVT_TICK queue pump). |
| `EntityEventHandler0x1001_1002_1008(_V2)` | enemies.c | PLACEHOLDER | hash-of-event-ids names; `_V2` is a behavioral duplicate of `EntityEventHandlerIdle`. |

## Comment pass progress (purpose comments added in-source)

- enemies.c pass A: ~17 funcs documented (token handlers, walk/attack/sparkle
  state setters, timed tick + anim-sequence helpers, stunned/passthrough ticks).
  Remaining enemies.c C funcs: mostly well-named state setters/init helpers
  following the same FSM-slot idiom — terse comments to be filled as needed.
