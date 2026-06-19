# Animation frame-event hashes — breakpoint plan (best remaining name-crack vector)

**2026-06-19.** Builds on gist `docs/gist/asset-system/` Session 31 (`anim_frame_events.json`).
This adds the concrete **compare-site addresses** (from the disassembly) and the **decomp handler
names**, which already constrain many event semantics without the emulator.

## Mechanism (verified in asm)

ToolX tags an animation frame with a calcHash **event id**. On playback reaching that frame the
engine sends a message: `event_id = 1` (NM_ANIMATION_START), with the hash in **`param_3` (`$a2`)**.
The entity handler does `if (param_3 == 0x<hash>) { fire sound / projectile / hitbox / state }`.

Confirmed example — `GliderEventHandler @0x80047768` (`asm/.../bosses/GliderEventHandler.s`):

```
a1 (event) == 1  ->  compare a2 (param_3):
    a2 == 0x10D86282  -> entity[0x10D] = 1   (glider ON)   ; bp 0x800477D8
    a2 == 0xB0C10420  -> entity[0x10D] = 0   (glider OFF)  ; bp 0x800477E8
```

These two were previously mislabeled "glider discriminator tokens" (see
`docs/reference/asset-hash-ids.md`); they are calcHash frame-event ids.

Cross-validation: `0x146CE002` == verified `FX_KLAY_DIE_EXPLODE` (a tagged frame on Klaymen's
death animation fires the explosion). Proves frame-events share the calcHash namespace **and** the
`FX_*` functional vocabulary.

## Why this is the best vector left

Unlike asset names (string discarded at build → dead end offline), a frame-event fires at a
**known, reproducible gameplay moment**. Breakpoint the compare → trigger the moment → observe the
on-screen effect → you know the semantic → collapse to a short functional vocab
(FOOTSTEP/FIRE/HIT/STEP/SHOOT/LAND/SPAWN…) → brute calcHash (short words crack, like
`FX_KLAY_DIE_EXPLODE`). Offline brute alone yielded 0 new (the names are long).

**Bonus from the decomp:** the handlers are already named, so many semantics are known *now*:
- `PlayerCallback_AnimFrameParticleHandler` events → spawn-particle-on-frame
- `PlayerCallback_DeathAnimSetStateHandler` events → death-animation state transitions
- `PlayerCallback_TeleporterAnimHandler` / `BounceEventHandler`, `WalkToRunTransition`,
  `CheckpointSaveAndSpawnHUD` (`0x182D840C`), `CollisionDamageSetup` (`0x50900004`)

## Breakpoint targets (compare-site addresses, param_3 in `$a2`)

37/39 events located as literals in the disassembly. Set an exec breakpoint on the listed address
(or the following instruction) and read `$a2`.

| hash | bp addr | handler |
|---|---|---|
| `0x10022814` | `0x80040E80` | EntityEventHandlerSpawnMultipleProjectiles (also `0x8002FDD0` DecorEntityCollisionHandlerExt) |
| `0x1002A434` | `0x8005CD70` | PlayerCallback_AnimFrameParticleHandler |
| `0x1020AC7D` | `0x8005CD7C` | PlayerCallback_AnimFrameParticleHandler |
| `0x11022025` | `0x8005CD58` | PlayerCallback_AnimFrameParticleHandler |
| `0x20833814` | `0x8005CD6C` | PlayerCallback_AnimFrameParticleHandler |
| `0x70032814` | `0x8005CD9C` | PlayerCallback_AnimFrameParticleHandler |
| `0x10062824` | `0x8005E6E4` | PlayerCallback_DeathAnimSetStateHandler |
| `0x1023281C` | `0x8005E714` | PlayerCallback_DeathAnimSetStateHandler |
| `0x11232C94` | `0x8005E6B8` | PlayerCallback_DeathAnimSetStateHandler |
| `0x12820D34` | `0x8005E740` | PlayerCallback_DeathAnimSetStateHandler |
| `0x30160814` | `0x8005E6CC` | PlayerCallback_DeathAnimSetStateHandler |
| `0x384D0812` | `0x8005E73C` | PlayerCallback_DeathAnimSetStateHandler |
| `0x404082C4` | `0x8005E76C` | PlayerCallback_DeathAnimSetStateHandler |
| `0x80062824` | `0x8005E6A0` | PlayerCallback_DeathAnimSetStateHandler |
| `0x89038298` | `0x8005E7B0` | PlayerCallback_DeathAnimSetStateHandler |
| `0x8C5E41E4` | `0x8005E7BC` | PlayerCallback_DeathAnimSetStateHandler |
| `0x92062824` | `0x8005E798` | PlayerCallback_DeathAnimSetStateHandler |
| `0x920E2062` | `0x8005E7AC` | PlayerCallback_DeathAnimSetStateHandler |
| `0x92120854` | `0x8005E7DC` | PlayerCallback_DeathAnimSetStateHandler |
| `0x94062824` | `0x8005E6B4` | PlayerCallback_DeathAnimSetStateHandler |
| `0x94222814` | `0x8005E808` | PlayerCallback_DeathAnimSetStateHandler |
| `0x94422814` | `0x8005E814` | PlayerCallback_DeathAnimSetStateHandler (also `0x8005EE9C` func_8005ECEC) |
| `0x98062824` | `0x8005E794` | PlayerCallback_DeathAnimSetStateHandler |
| `0xB0062824` | `0x8005E804` | PlayerCallback_DeathAnimSetStateHandler |
| `0x1C0C66F2` | `0x8005E74C` | PlayerCallback_DeathAnimSetStateHandler |
| `0x221017CA` | `0x8005D130` | PlayerCallback_TeleporterAnimHandler (also `0x8005D2B0` BounceEventHandler) |
| `0x382C92C1` | `0x8005D12C` | PlayerCallback_TeleporterAnimHandler (also BounceEventHandler) |
| `0x2809A510` | `0x8005D118` | PlayerCallback_TeleporterAnimHandler (also `0x80074568` RunnLand_HazardDamageTransition) |
| `0x1210CD03` | `0x8005F590` | PlayerCallback_WalkToRunTransition |
| `0x50900004` | `0x8005F47C` | PlayerCallback_CollisionDamageSetup |
| `0x182D840C` | `0x8005DEFC` | PlayerCallback_CheckpointSaveAndSpawnHUD (also `0x8006DE60` ScaleBoundsAndCollision) |
| `0x1A800898` | `0x8005CF7C` | PlayerBounceCallback (also `0x8006BAFC` ScaleBoundsAndCollision) |
| `0x146CE002` | `0x80073540` | RunnLand_HazardDamageTransition  — **= FX_KLAY_DIE_EXPLODE** |
| `0x4A212247` | `0x800454F0` | TeleporterPortalEventHandler |
| `0x8CAA1108` | `0x8004892C` | EntityMessageHandler |
| `0x10D86282` | `0x800477D8` | GliderEventHandler (glider ON; also EntityMessageHandler `0x80048944`) |
| `0xB0C10420` | `0x800477E8` | GliderEventHandler (glider OFF; also EntityMessageHandler `0x80048940`) |

**Not located as plain literals** (likely split `lui`/`ori` or loaded from a data table — find via
the owning handler from `anim_frame_events.json`): `0x0105080E`, `0x080CA433`.

## Tracer script (implemented)

`scripts/frame_event_tracer.lua` arms an Exec breakpoint at all 37 compare sites and, on a
*match* (`$a2 == site_hash` — the event actually fired), logs the firing entity's
type/sprite_id/anim_frame/position + player state to `game_watcher/logs/frame_events.jsonl` and a
live console line. Console helpers: `fe_summary()` (which hashes have fired), `fe_stop()`.

```bash
make lua SCRIPT=scripts/frame_event_tracer.lua      # boots game + arms tracer
```

Restored alongside it from git (`68e6f69^`): `scripts/game_watcher.lua` + `scripts/structs/*` so
`make record LEVEL=.. STAGE=..` (boot directly into a level) works again.

## Emulator procedure (PCSX-Redux)

1. `make emu` (enable Web Server), or `make record LEVEL=… STAGE=…` to boot into the relevant level.
2. Add exec breakpoints at the addresses above (or hook them in `scripts/game_watcher.lua`,
   logging `$a2` + the entity's sprite id, like the existing dispatch hooks).
3. Trigger the moment: e.g. let Klaymen die → catches the `DeathAnimSetStateHandler` cluster +
   `0x146CE002`; specific Klaymen actions → the `AnimFrameParticleHandler` particle events.
4. Note the on-screen effect → assign the semantic → brute calcHash over the short functional vocab.
   The `..062824` cluster (`0x10062824 / 0x80062824 / 0x92062824 / 0x94062824 / 0x98062824 /
   0xB0062824`) shares the tail `..062824` (one base event name, varying top byte) — cracking one
   member names the whole family.
