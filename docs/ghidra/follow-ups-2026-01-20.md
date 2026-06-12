# Documentation Push Session Follow-ups (2026-01-20)

This session documented 21 high-priority functions (entity-list helpers, platform-ride
helpers, entity event handlers, 12 player physics callbacks, GetWorldPositionX/Y).
The following items were identified during the work and need follow-up.

## Mis-named globals: `g_DefaultBGColorR/G/B`

During physics-callback documentation, the decomp of
`PlayerCallback_FallInputHandler` (0x80060890) and `PlayerCallback_WalkInputHandler`
(0x8005FB18) showed:

```c
if ((_g_DefaultBGColorG & param_1->pInput->wButtonsPressed) == 0) ...
if ((_g_DefaultBGColorR & uVar1) == 0) ...
```

The `_` prefix means the decompiler is reading a wider (u16) value that overlaps the
u8 symbol we typed earlier. These are **button bitmasks**, not RGB background colour
channels. Likely roles:

| Current name           | Probable role                                 |
|------------------------|-----------------------------------------------|
| `g_DefaultBGColorR`    | `g_wBtnMaskFallDown` (attack-down activation) |
| `g_DefaultBGColorG`    | `g_wBtnMaskTeleport` (swirly-Q activation)    |
| `g_DefaultBGColorB`    | likely another button mask                    |

**Action**: re-investigate by tracing more callers, then rename + retype as `u16`.
Drop the "BGColor" name everywhere (including `symbol_addrs_new.txt` if it was
emitted there).

The plate comments written this session reference fields by offset, not by the
incorrect symbol names, so no plate rework is needed when the rename happens.
Each affected plate already includes a "NOTE: g_DefaultBGColorR/G are MISNAMED
button masks" caveat.

## ~~Deferred: include/Game/entity.h sync~~ — DONE 2026-06-12

`SpriteEntity`, `PlayerEntity`, `FinnPlayerEntity`, `RunnPlayerEntity`,
`SoarPlayerEntity`, and `PathEnemyEntity` were added to
`include/Game/entity.h` from `get_struct_layout` dumps, with every size and
key offset verified by compile-time `sizeof`/`offsetof` asserts under the
MIPS cross-gcc. `GlidePlayerEntity` remains TBD (next section still open) —
the header documents that the Glide creator reuses the Soar layout with a
0x11C allocation.

## Deferred: GlidePlayerEntity struct definition

`CreateGlidePlayerEntity` @ 0x8006EDB8 is currently typed in Ghidra with its
first param as `SoarPlayerEntity *`. They share most fields (Glide reuses
`FinnMainTickHandler` as its tick callback). Known Glide-specific fields from
the init function:

| Offset | Type | Init value | Notes                                    |
|--------|------|------------|------------------------------------------|
| +0x104 | s32  | 0x20000    | `nFlightSpeed` (2.0 in 16.16)            |
| +0x108 | s32  | 0          | unknown                                  |
| +0x10C | u32  | 0x400      | `handle10C` — angle?                     |
| +0x110 | u8   | 0          | flag                                     |
| +0x111 | u8   | 0          | flag                                     |
| +0x112 | s16  | 0x14       | `stateTimer`                             |
| +0x113 | u8   | 0          | flag                                     |
| +0x114 | s32  | 0xFFFFFFFF | `field_0x114`                            |
| +0x119 | u8   | 0          | Same offset as SOAR `forcedGravityTimer` under current shared typing; verify before cloning. |

Allocation is 0x11C bytes (Soar is 0x128, so Glide is 12 bytes smaller — some
Soar tail fields are missing or repurposed in Glide).

**Pragmatic path**: `clone_data_type SoarPlayerEntity → GlidePlayerEntity`,
then truncate to 0x11C bytes. Defer until per-field verification confirms which
tail fields go away.

## Tools / env notes

- `mcp_ghidra-new_apply_function_documentation` requires `target_address`
  parameter (NOT `address`). First-time gotcha.
- `mcp_ghidra-new_apply_function_documentation` reports `changes_applied: 2` —
  this counts plate-comment + signature. It does NOT apply name renames or tags;
  use explicit `rename_function` / `batch_add_function_tags` follow-ups.
- `mcp_ghidra_structs_get` and `mcp_ghidra_functions_decompile` (older MCP
  server family) return "MCP server could not be started: Process exited with
  code 2" in this env. Use `mcp_ghidra-new_*` equivalents.
