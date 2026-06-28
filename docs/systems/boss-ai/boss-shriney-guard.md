# Boss: Shriney Guard (MEGA level 5)

**Entity type**: `ENTITY_TYPE_101_BOSS_SHRINEY_GUARD` (0x65)
**Vtable**: `SHRINEY_GUARD_VTABLE` @ `D_800112A8`
**Init function**: `InitShrineyGuardBoss` @ `0x8004AF64` (called from
`EntityType101_Boss_ShrineyGuard_Init` in `src/entinit.c`)
**Source**: [src/bosses.c](../../../src/bosses.c) – the dedicated
"SHRINEY GUARD" section block-commented in the file
**Status**: ✅ Verified from ASM + runtime trace
(`game_watcher/logs/struct_watch_20260621_110109.jsonl`)

---

## TL;DR

The first boss in Skullmonkeys. A 162×240 px clay sprite that idles, then
runs one of two attack patterns (single windup-slam or a 3-slam loop), each
attack ending in a horizontal slam that drives the boss across the arena
on a fixed 25-frame accel + 53-frame decel ramp. Player damages it by
bouncing on its head during the windup frame. **HP = 3.** On death, the
boss writes a cached destination-level number into `GS.direct_level_load`,
which the engine consumes next frame to transition to the next world
(in vanilla MEGA → `direct_level_load = 99`, the menu/credits sentinel).

---

## Verified runtime trace

Captured at PAL 50 fps with `scripts/game_watcher.lua` hooked to
`PlayerTickCallback`, `EntitySetState`, `SetEntitySpriteId`,
`DispatchEventToCollidingEntity` and the
`PlayerState` + `GameState` struct watchers.

| Event | Frame | Field change | Cause |
|------:|:-----:|:-------------|:------|
| Spawn | 0405 | `PS.boss_hp 0 → 3` | `InitShrineyGuardBoss` writes 3 |
| Hit 1 | 0592 | `GS.screen_shake_index 0 → 19`, `PS.boss_hp 3 → 2` | bounce damage |
| Slam tell | 0758 | `GS.screen_shake_index 0 → 19` (no HP change) | boss slam impact |
| Hit 2 | 0805 | `GS.screen_shake_index 0 → 19`, `PS.boss_hp 2 → 1` | bounce damage |
| Player death | 0929 | `GS.screen_shake_index 0 → 28` | player fell off arena |
| Death cleanup | 1051 | `PS.boss_hp 1 → 0` | tick-after-death cleanup |
| Respawn | 1053 | `PS.boss_hp 0 → 3` | re-`InitShrineyGuardBoss` |
| Hit 1 (retry) | 1180 | `PS.boss_hp 3 → 2` | |
| Slam tell  | 1346 | shake = 19 | |
| Hit 2 (retry) | 1360 | shake re-set 6→19, `PS.boss_hp 2 → 1` | hit landed mid-shake |
| Hit 3 → kill | 1541 | shake re-set 5→19, `PS.boss_hp 1 → 0` | victory |
| Post-death shake | 1600 | shake = 19 | cosmetic / level fade |

Notes:
- The shake amplitude is **19 for every hit** the player lands, and
  **28 only for the player's own death** (falling off). The shake-only
  spikes at f0758/f1346 are the boss's own slam impacts; they happen on
  the same frame as `ShrineyGuardMoveCallback` reaches velocity 0.
- Shake re-sets mid-ramp (f1360, f1541) show the engine simply latches
  `shake = max(shake, 19)` on each hit; the second hit happened while the
  previous ramp was still decaying.
- Three hits kill the boss every time. No phase transition exists — HP is
  raw decrement, dispatched purely via `BossEventHandler` event `0x1002`.

---

## Asset roster

All sprite ids are 32-bit `calcHash`-based asset hashes. They are now
exported as constants in [`include/Game/asset_ids.h`](../../../include/Game/asset_ids.h)
under the `SPR_SHRINEY_GUARD_*` and `ANIM_SHRINEY_GUARD_*` group.

| Symbol | Hash | Role |
|---|---|---|
| `SPR_SHRINEY_GUARD_BASE` | `0xA8482860` | Initial `InitEntitySprite` hash (162×240 px, `DOWN_MOUNTLET`) |
| `SPR_SHRINEY_GUARD_IDLE` | `0x09382152` | Passive idle pose (`ShrineyGuardIdleState`) |
| `SPR_SHRINEY_GUARD_WINDUP` | `0x4C106054` | Attack-windup (single attack path; player bounce-damage frame) |
| `SPR_SHRINEY_GUARD_LOOP_LINK` | `0x40106054` | First frame of looping-attack chain (`ShrineyGuardSetLoopingAttackState`) |
| `SPR_SHRINEY_GUARD_LOOP_START` | `0x2C182010` | Looping-attack stun-window pose (`ShrineyGuardStartLoopAttackState`) |
| `SPR_SHRINEY_GUARD_READY` | `0x08192250` | Re-aim pose between slams (`ShrineyGuardReadyAttackState`) |
| `SPR_SHRINEY_GUARD_SLAM` | `0x085860D4` | Slam animation – `MoveCallback` runs (`ShrineyGuardAttackAnimState`) |
| `SPR_SHRINEY_GUARD_DEATH` | `0x0A1820D4` | Death pose (`ShrineyGuardDeathState`) |
| `ANIM_LOOP_DEFAULT` | `0x01084280` | Shared engine loop-frame hash; Shriney's `AttackEventHandler` compares `arg2` against it to trigger `MoveCallback` install (not Shriney-exclusive — also used across `src/enemies.c`) |
| `ANIM_SHRINEY_GUARD_LOOP_KEYFRAME` | `0xC00200C9` | Looping-attack second-loop frame |
| `ANIM_FINISHED_CB` | `0x02421405` | Shared engine anim-finished callback hash (not Shriney-exclusive) |

The "9-sprite multi-entity" listed in
[`docs/systems/boss-entity-pattern.md`](../boss-entity-pattern.md) is
`g_ShrineyGuardBossSprites[9]` at `0x8009BA88` (declared in
[`include/globals.h`](../../../include/globals.h)). These are the LBA
table for the sprite payload; the boss only ever shows one of the above
sprite-id slots at a time via `SetEntitySpriteId`.

### Sound assets

The boss itself does **not** stream voice audio. Its only audio output
is the engine-level "ground slam" — driven by `GS.screen_shake_index`
which the renderer also uses as an envelope for the bass-thud SFX —
plus whatever ambient layer the MEGA level loops.

A sound-emitting clayball that **shares the "Shriney Guard" name
prefix** (`ShrineyGuardSoundUpdateTick`, `ShrineyGuardEventWithSound`,
`ShrineyGuardDestroyWithSoundCleanup`, `InitBonusClayballEntity`) lives
in [`src/clayball.c`](../../../src/clayball.c). That entity is
`ENTITY_TYPE_093_BONUS_CLAYBALL` (the audio-driven rolling clayball the
boss summons in cinematics), **not** the boss entity. It manages its
own two SPU voices at `+0x128` (rolling loop) and `+0x12C` (impact /
short loop).

The companion Glenn Yntis mini-boss (GLEN level) uses a different audio
path: `HazardActivateWithSound` plays sound `0x7A318245` and
`HazardIdleWithSound` plays `0x62318245` — both belong to the
GLEN-level audio cluster
(`docs/gist/asset-system/sister_clusters.json`). Shriney Guard does
not reach those.

---

## Per-entity scratch layout

Overlay on `SpriteEntity`. See `ShrineyGuardEntity` typedef in
[src/bosses.c](../../../src/bosses.c).

| Off | Field | Meaning |
|---:|---|---|
| `0x10C` | `readyFlag` | 0 = idle, 1 = slam-armed, 2 = stun/death lockout |
| `0x110` | `destLevel` | level number for `GS.direct_level_load` on victory (cached from `EntitySpawnData[+0xC]` in `Init`) |
| `0x111` | `activeTimer` | `ShrineyGuardActiveEventHandler` countdown target |
| `0x112` | `idleTimeout` | frames until next attack; init = `0xB4` (180), reset = `0x5A` (90) |
| `0x113` | `stunTimer` | `StartLoopAttackState` arms 5; `StunTickCallback` decays it |
| `0x114` | `attackCounter` | number of consecutive slams (resets at 3) |
| `0x115` | `stunActive` | 1 while `ReadyAttack` windup is playing |
| `0x118` | `slamVelocity` | s32, 16.16 fixed; see physics below |
| `0x11C` | `slamFrameCounter` | u8 frame counter; see physics below |

---

## FSM marker / callback table

Pairs at `D_800A5B**` / `D_800A5C**` in sdata. Each pair is
`(marker u32 = 0xFFFF0000, callback ptr)` so `EntitySetState` matches
on the tagged marker.

| GP-rel marker / fn | Source | Used by | Result |
|---|---|---|---|
| `D_800A5BE8 / EC` | `BossRandomAttackChoice` | `InitShrineyGuardBoss` | enter combat |
| `D_800A5BF0 / F4` | `ShrineyGuardAttackCounterState` | `IdleTickCallback` (idleTimeout=0) | decide loop-or-finish |
| `D_800A5BF8 / FC` | `ShrineyGuardDeathState` | `BossEventHandler` (hp == 0) | terminal sequence |
| `D_800A5C00 / 04` | `ShrineyGuardStartLoopAttackState` | `BossEventHandler` (hp > 0 after hit) | 5-frame stun |
| `D_800A5C08 / 0C` | `ShrineyGuardReadyAttackState` | `MoveCallback` (vel == 0) | re-aim |
| `D_800A5C10 / 14` | `ShrineyGuardAttackAnimState` | `AttackCounterState` (count < 3) | next slam |
| `D_800A5C18 / 1C` | `ShrineyGuardIdleState` | `AttackCounterState` (count == 3) | back to idle |

---

## Combat flow (verified)

```
                    Init (HP := 3, idleTimeout := 180)
                              │
                              ▼
                    BossRandomAttackChoice (RNG 50/50)
                       │                    │
                       ▼                    ▼
       SetAttackState (single)   SetLoopingAttackState (loop)
                       │                    │
                       └─────────┬──────────┘
                                 ▼
                       AttackAnimState (sprite 0x085860D4)
                                 │
                                 │  ← anim keyframe 0x01084280
                                 │    installs ShrineyGuardMoveCallback
                                 ▼
                   MoveCallback ramp:
                   • +0x8000/frame for 25 frames (accel)
                   • -0x1800/frame after  (~53 frames decel)
                   • max velocity 0x50000 (~5.0 px/frame, 16.16)
                   • direction sign from facing byte +0x74
                                 │
                                 │  vel == 0
                                 ▼
                       ReadyAttackState (sprite 0x08192250)
                                 │ (queued: BossRandomAttackChoice)
                                 ▼
                                ...

   PLAYER BOUNCE on head → event 0x1001 → BossEventHandler
       └→ if !stunActive: readyFlag := 1, dispatch queued state

   PLAYER DAMAGE → event 0x1002 → BossEventHandler
       └→ PS.boss_hp--
          if hp > 0 → EntitySetState(D_800A5C00,
                                      ShrineyGuardStartLoopAttackState)
                       (5-frame stun, sprite 0x2C182010)
          if hp == 0 → EntitySetState(D_800A5BF8,
                                       ShrineyGuardDeathState)
                       (sprite 0x0A1820D4, queue DeathCallback)
```

`ShrineyGuardDeathCallback` then copies the cached destination-level
byte (`+0x110`) into `GS.direct_level_load` and sets the entity-dead
flag `+0x106 = 1`. The dispatcher tick callback running game-state sees
`direct_level_load != 0` and performs the cross-level transition.

In vanilla MEGA the destination is `99`, which is the engine's
"menu / credits" sentinel.

---

## Magic event arg constants

Consumed by `BossEventHandler` on `event == 0x0001` (script triggers):

| `arg2` | Behaviour |
|---|---|
| `0x400B43A0` | Spawn directional projectile via `InitDirectionalPositionEntity` (X offset ±0x38 based on facing) |
| `0x18443181` | Write `GS+0x11A = 0x14` (intro / cinematic flag) |
| `0x46384180` | Clear `GS.level_active` + spawn `CreateFadeOverlayEntity` → level-end fade |

Consumed by `ShrineyGuardAttackEventHandler` on `event == 1`:

| `arg2` | Behaviour |
|---|---|
| `0x01084280` | Slam keyframe fired from the slam animation. Installs `ShrineyGuardMoveCallback` on the movement slot `+0x1C/0x20`, sets `readyFlag = 1`, clears `stunActive = 0`. This is the moment the boss starts moving on each slam. |

---

## Slam physics (verified)

In `ShrineyGuardMoveCallback`:

- read `worldX` hi/lo from `+0x68 / +0x6C` and facing byte from `+0x74`
- increment `slamFrameCounter` (`+0x11C`)
- **if** `slamFrameCounter < 0x19` (25 frames):
    - `slamVelocity += 0x8000` (`~0.5/frame`), clamped at `0x50000` (`~5.0/frame`)
- **else**:
    - `slamVelocity -= 0x1800` (`~0.094/frame`)
    - if `slamVelocity == 0`:
      `EntitySetState(D_800A5C08, ShrineyGuardReadyAttackState)`
- apply `±slamVelocity` to the `+0x68 / +0x6C` worldX pair according to
  facing (`0 = right`, `1 = left`)

Total slam ≈ **78 frames** (~1.56 s at 50 fps PAL):

- 25 frame accel from 0 to 5.0/frame  
- 53 frame decel from 5.0/frame to 0

Distance covered ≈ `0.5 × 5 × 25 + 5 × 53 × 0.5` ≈ **~195 world units**
per slam (~3 player widths).

---

## Init constants (`InitShrineyGuardBoss`)

| Write | Value | Notes |
|---|---|---|
| `PLAYER_STATE_DATA[+0x1D]` | `3` | boss HP |
| aux entity alloc size | `0x2710` (10000) | from BLB heap |
| `e[+0x18]` | `SHRINEY_GUARD_VTABLE` | shared boss vtable |
| `e[+0x110]` | `EntitySpawnData[+0xC]` | destination-level byte |
| `e[+0x112]` | `0xB4` (180) | first idle timeout |
| `e[+0x115]` | `1` | stunActive |
| `EntitySetState` | `(D_800A5BE8, BossRandomAttackChoice)` | enter combat |

---

## Related code

- [src/bosses.c](../../../src/bosses.c) – full Shriney Guard section
  with per-function block comments
- [src/clayball.c](../../../src/clayball.c) – the misnamed
  "ShrineyGuardSoundUpdateTick" family (actually entity type 093)
- [src/entinit.c](../../../src/entinit.c) –
  `EntityType101_Boss_ShrineyGuard_Init` dispatch
- [include/Game/asset_ids.h](../../../include/Game/asset_ids.h) –
  sprite-id and anim-hash constants

## Related docs

- [Boss entity pattern](../boss-entity-pattern.md)
- [FSM callback patterns](../fsm-callback-patterns.md)
- [Trace findings](../player/trace-findings.md)

---

**Status**: ✅ **Verified from ASM + trace** (struct_watch_20260621_110109)
