# Session 2 — Decomp cross-reference (role map) + key correction

## Correction that reframed the model
**There are no seed constants in the binary.** So the per-namespace SEED is NOT a runtime
constant — it is a build-time artifact: ToolX hashed the **full string** `PREFIX + name`
(a path/namespace prefix) at build time and stored only the 32-bit result. Equivalently
`SEED = calcHash(PREFIX)`, `R = sigma(PREFIX)`. This is why grepping the EXE for
`0x08280150` / `0x28C0E011` finds nothing, and why the earlier "breakpoint on a seed load"
idea was wrong. The IDs appear in the decomp as **literal baked hex constants** in
`InitEntitySprite(...)` calls — which gives us *semantic role* per id even with names gone.

## What we extracted from docs/ghidra/SLES_010.90.c (64,362 lines)
- Parsed every `InitEntitySprite / SetEntitySpriteId / InitParticleEntity /
  AllocSpriteRenderContext` call site and mapped id -> spawning function.
- **116 asset ids appear as literal constants; 105 of the 398 uncracked sprites now have a
  decomp role; 84 have a human-named role function.** (See `sprite_roles.json`.)

### Confirmations (validate prior work)
- `FUN_8002be8c` spawns exactly the 6 TEXT-namespace ids (CONTINUE/QUIT/YES/NO/PAUSED/QUIT GAME).
- `Callback_80048f40` co-spawns `0x60DE1050` (our WIZARD_HIT_01) with `0xA0CC18D0` (Mage staff)
  — code structure corroborates the BOSS-seed decode.

### New role leads (uncracked, named role)
- `InitPlayerEntity` / `PlayerState_*` -> the player (Klaymen) sprite set, per-animation.
- `FinnDeathExplosion` -> 0x088C5011, 0x344210B1 (FINN).
- `CreateSoarPlayerEntity` -> 0x04835000, 0xCA1B20CB (SOAR vehicle stage).
- `CreateMenuEntities` -> 6 menu/HUD sprites.
- ~30 `Callback_*` / `EntityInitCallback_*` each spawning a level-localized enemy sprite.

## Hard negative results (honest)
- The 16-entry **Player Sprite Table @ 0x8009c174** does **not** solve under any
  `KLAY/KLAYMEN + action` seed (max 2/16).
- Known-label player pairs disprove the naive scheme: `WalkingRight`/`WalkingLeft` XOR does
  **not** equal a RIGHT/LEFT suffix swap at any rotation; `Idle`/`Idle2` is not base+'2'.
  => Player sprites do NOT use `ENTITY+ACTION` concatenation in any obvious spelling. The
  decomp function names are RE labels, **not** the original asset strings.
- FinnDeathExplosion / Soar pairs are not low-popcount frame-siblings -> distinct base names,
  own (unknown) seeds.

## Where this leaves seed-cracking
- Each new namespace still needs **one correct full base-name guess** OR an external anchor.
  The decomp gives *roles* (what an id is) but not the *strings* (what it was named).
- Highest-value next step remains an **in-game/emulator capture** of the `char*` passed to the
  build-time-equivalent hash, OR finding an asset-name string table in the original ToolX
  project data — the binary alone cannot yield the strings (they were hashed away at build).

## Files
- `sprite_roles.json` — function -> [ids, level] (the role map; 92 functions)
- `id_role_map.json`  — id -> {func,line,call,level,name,status}
