---
title: "Player Trace Analysis - January 14, 2026"
category: systems/player
tags: [systems, player, trace, findings]
---

# Player Trace Analysis - January 14, 2026

## Trace File
`trace_20260114_191737_unknown_stage0_f0.jsonl` (268 events, 47KB)

## Gameplay Session
- Level: SCIE Stage 0 (Science level)
- Duration: Frames 504-1243 (739 frames, ~12.3 seconds @ 60fps)
- Activities: Jump, walk right, run right, collect clayballs, hit monkey (death), respawn, jump onto second monkey (crash)

## Discovered States

### Movement States

**Standing Idle** (0x8006888C)
- Sprite: 0x1c395196 or 0x3838801a
- Frames: 504, 572

**Walking Right** (0x8006736C)  
- Sprite: 0x292e8480
- Frames: 601, 618, 641
- Pattern: Short bursts with respawn transitions between

**Falling** (0x800678D4)
- Sprite: 0x0b2084d0
- Uses special tick callback: PlayerCallback_8005bb80 (not normal PlayerTickCallback!)
- Frames: 653, 1059, 1238

**Jump** (0x80067E28)
- Sprite: 0x092b8480
- Plays sound: 0x248e52
- Sets +0x156 = 0x0C (jump parameter)
- Frames: 731, 1064, 1239

**Death** (0x8006A0B8)
- Sprite: 0x1b301085
- Disables movement callbacks (sets to 0)
- Sets g_GameStatePtr[0x170] = 0
- Frame: 814 (after monkey collision)

**Respawn/Transition** (0x80066CE0)
- Sprite: 0x48204012 (turn animation)
- Used between movement states
- Frames: 604, 621

**Pickup Item** (0x80068B48)
- Sprite: 0x1c3aa013
- Uses special tick: PlayerCallback_8005bbac
- Sets g_GameStatePtr[0x60] = 1 (bounce_active_flag)
- Frames: 521, 1183, 1243 (crashed)

## Velocity Data

From GameStateTick at frame 1200 (player at position 880, 370):
- **vx**: 0 (standing still)
- **vy**: -65536 (0xFFFF0000 in s32)

Converting from 16.16 fixed point:
- vy = -65536 / 65536 = **-1.0 pixels/frame**
- At 60fps: **-60 pixels/second** (downward)

This is likely **gravity being applied while on ground** or a **tiny upward bounce** (negative Y = up on PSX).

### Walk/Run Velocity (from entities list)

Multiple entity entries show various vx values (these appear to be other entities, not the player):
- vx: 1245203 → ~19.0 px/frame (enemy movement?)
- vx: 1179697 → ~18.0 px/frame  
- vx: 1114159 → ~17.0 px/frame
- vx: 917543 → ~14.0 px/frame
- vx: 720926 → ~11.0 px/frame
- vx: 524311 → ~8.0 px/frame
- vx: 393234 → ~6.0 px/frame
- vx: 327694 → ~5.0 px/frame

**Need to filter for actual player entity** to get accurate walk/run speeds.

## Animation System

From PlayerAnimTick events:
- **Walk animation**: 8 frames (0-7)
- **Animation speed**: 5 (timer value)
- **Frame advance rate**: Every ~4 game frames (15fps animation)
- **Loop pattern**: 5→6→7→5→6→7 (after reaching frame 7, loops back to frame 5, not 0)

### Example Animation Sequence
```
Frame 435: anim[1] speed=5 → Start walking
Frame 439: anim[2] (+4 frames)
Frame 443: anim[3]
Frame 447: anim[4]
Frame 451: anim[5]
Frame 455: anim[6]
Frame 459: anim[7] → End of sequence
Frame 463: anim[5] → Loop back to 5!
Frame 467: anim[6]
```

## State Transition Flow

```
Spawn → Idle (504)
  ↓
Pickup (521) → Idle (572)  [Klayman head collection?]
  ↓
Walk Right (601) ⇄ Respawn (604) → Walk Right (618) ⇄ Respawn (621) → Walk Right (641)
  ↓
Falling (653) [sprite change to 0x0b2084d0]
  ↓
Jump (731)
  ↓
Death (814) ← Collision with monkey enemy
  ↓
Respawn (984) [PlayerStateCallback_2]
  ↓
Falling (1059) → Jump (1064)
  ↓
Pickup (1183) [clayball]
  ↓
Falling (1238) → Jump (1239)
  ↓
Pickup (1243) [CRASH - likely second pickup too fast]
```

## Key Findings

### 1. Falling State is Special
The falling state uses **PlayerCallback_8005bb80** instead of the normal **PlayerTickCallback**. This is a unique tick callback only used for falling/airborne states.

### 2. Death Disables Movement
Death state clears the movement callback pointers (+0x104/+0x108) by setting them to 0, preventing any collision or movement processing during death animation.

### 3. Pickup Crash Confirmed
The crash occurred at frame 1243 during a pickup state transition. This aligns with our hypothesis that rapid pickups (frames 1183 → 1243 = 60 frames = 1 second) cause handler overload.

### 4. Jump Sound Constant
Jump sound ID: **0x248e52** (confirmed from decompilation)

### 5. Walk/Run Share Same Sprite
Both walking and running states use sprite 0x292e8480. The actual movement speed difference is controlled by velocity values, not sprite IDs.

## Next Steps

### Critical: Get Actual Player Velocity
The GameStateTick captured entity data but need to **isolate the player entity** specifically. Current vx/vy readings are from mixed entities.

**Recommended approach:**
1. Add player-specific velocity logging to game_watcher.lua
2. Log entity+0xB4 (vx) and entity+0xB8 (vy) every frame during:
   - Standing idle (should be 0, 0)
   - Walking (capture X velocity)
   - Running (capture X velocity, should be higher than walk)
   - Jumping (capture Y velocity sequence: initial → apex → falling)
   - Landing (capture velocity damping)

### Physics Constants to Verify
- **Walk speed**: Estimate ~2-3 px/frame (need actual value)
- **Run speed**: Estimate ~4-5 px/frame (need actual value)
- **Jump initial velocity**: Unknown (need to capture at jump start)
- **Gravity**: Unknown (need frame-by-frame Y velocity during fall)
- **Terminal velocity**: Unknown (max fall speed)
- **Landing damping**: Unknown (velocity change on ground contact)

### Enemy Collision
- **Enemy type 25** (monkey): Caused death at frame 814
- Need to trace:
  - Damage amount
  - Knockback velocity
  - Invincibility timer duration
  - Hit detection radius

### Sprite ID Reference
| Sprite ID | Hex | State | Description |
|-----------|-----|-------|-------------|
| 473518486 | 0x1C395196 | Idle | Standing still |
| 943226906 | 0x3838801A | Idle | Standing variant |
| 690914432 | 0x292E8480 | Walk/Run | Movement right |
| 405373456 | 0x18298210 | Walk | Movement left |
| 186680528 | 0x0B2084D0 | Falling | Descending |
| 153846912 | 0x092B8480 | Jump | Ascending |
| 456134789 | 0x1B301085 | Death | Explosion |
| 473604115 | 0x1C3AA013 | Pickup | Collection anim |
| 1210073106 | 0x48204012 | Turn | Direction change |
| 3703056 | 0x00388110 | Idle | Post-respawn |

---

# Player Trace Analysis — June 21, 2026 (struct_watcher)

## Trace File
`game_watcher/logs/struct_watch_20260621_104346.jsonl` — 33,040 events, ~78 seconds of gameplay across GLEN level + bonus-room (1970s) teleport.

Captured with the dynarec-safe `scripts/struct_watcher.lua` (no breakpoints, GPU::Vsync diff sampling). Loaded into DuckDB via `tools/struct_db.py`; analysis in `.duckdb_findings.md`.

## 1. Player collisionVtable swap is the canonical death signature

`Player.collisionVtable @ +0x18` is one of two values:

| Address | Symbol | State |
|---|---|---|
| `0x80011804` | `g_PlayerCallbackTable` | Alive |
| `0x800104AC` | `g_EntityVtable_Destroyed` | Dead / mid-transition |

Every observed death (frames 663, 4596) and bonus-room transition (frames 3021→3185) is bracketed by a vtable swap. **The vtable swap is a more reliable death signal than `PS.lives--`** because lives can also change at password-load time (frame 238 of this trace, when the engine writes 5 lives on top of the boot defaults).

## 2. Death timeline (verified, T = frame of `PS.lives--`)

| Rel. frame | Event |
|---|---|
| **T-2** | Hitbox shrinks 64×65 → 24×37 (death-sprite swap), `Player.textureDirty 0→1`; `GS.advance_level_flag 0→1` (queue) |
| **T-1** | `Player.textureDirty 1→0` (death texture uploaded to VRAM) |
| **T+0** | `Player.collisionVtable 0x80011804 → 0x800104AC`; `GS.collision_list_head → NULL`; `GS.frame_counter → 0` (level resets); `GS.bg_color_change_flag 0→1` (fade prep); `GS.advance_level_flag 1→0` (consumed); `PS.lives--` |
| **T+1** | `GS.{render,tick}_list_head` swap (lists rebuilding for respawn) |
| **T+2** | `Player.collisionVtable → 0x80011804` (back to alive); `Player.world{X,Y}` warp to `GS.spawn_{x,y}`; `Player.velocity{X,Y} → 0`; `Player.eventCallback → PlayerEvent_ZoneTriggerHandler (0x8005BE6C)`; `GS.level_active 0→1` |

Verified on frames 663 and 4596 — pattern is identical.

## 3. Bonus-room teleport timeline (frames 2862 → 3185, ~5.4 s @ 60 Hz)

| Frame | Event |
|---|---|
| 2862 | Player despawn: `tickCallback → EntityUpdateCallback (0x8001CB88)` (generic fallback), event/render callbacks zeroed, both markers cleared |
| 3021 | Clayball consumed: vtable → Destroyed, `PS.clayball_flag_0 1→0`, `PS.total_1ups 0→1` |
| 3022 | Level data wiped: `entity_spawn_list`, `palette_group_ptrs`, `tile_render_state_*` → NULL |
| 3096 | Second wipe (camera/entity_callback_table/trigger_zones zeroed) and new palette+tile data begins loading; `player_entity_ptr/alt → NULL` |
| 3182 | New level metadata loaded: `spawn_x=168`, `spawn_y=399`, camera limits 6400×640, bg color (126, 106, 49), new trigger/tile_collision pointers |
| 3183 | `scroll_limit_*` enabled (all 4 bytes in same write) |
| 3184 | `level_active 0→1`, `PS.world_index_tally 3→9` (bonus-room internal index), `PS.active 1→2` (transition state) |
| 3185 | **New Player allocated at `0x801ACFE4`** (level-1 was `0x80194A04`/`0x801949D4`); vtable → alive; `eventCallback → PlayerEvent_ZoneTriggerHandler` |

**The Player entity is freed and re-allocated per level transition** — not held in a fixed slot. Frame 4596 death from inside the bonus-room re-allocates again at `0x80194E24`/`0x801B2244`.

## 4. Player callback FSM (cross-referenced against `symbol_addrs.txt`)

| Slot | Address | Symbol |
|---|---|---|
| eventCallback | 0x8005C1C4 | `PlayerEntityEventHandler` (idle dispatcher) |
| eventCallback | 0x8005CC84 | `PlayerCallback_EventHandlerWithQueue` |
| eventCallback | 0x8005CF24 | `PlayerBounceCallback` (airborne) |
| eventCallback | 0x8005CEC8 | `PlayerCallback_CollisionHandlerWithQueue` |
| eventCallback | 0x8005BE6C | `PlayerEvent_ZoneTriggerHandler` (respawn/teleport) |
| eventCallback | 0x8005D404 | `PlayerCallback_DeathDebrisSpawner` (death T+0) |
| eventCallback | 0x8005C400 | **UNKNOWN** — appears 8+ times in trace |
| eventCallback | 0x8005F540 | **UNKNOWN** — appears 4+ times in trace |
| tickCallback | 0x8005B414 | `PlayerTickCallback` (main) |
| tickCallback | 0x8005BAD0 | `PlayerState_IdleRandom` |
| tickCallback | 0x8005BB80 | `PlayerState_FrameCountTick` |
| tickCallback | 0x8005BBAC | `PlayerState_CooldownTick` |
| tickCallback | 0x8001CB88 | `EntityUpdateCallback` (generic, used when despawned) |
| tickCallback | 0x8005BDB4 | **UNKNOWN** — paired with eventCallback 0x8005CCE0 (also unknown) |
| renderCallback | 0x80061180 | **UNKNOWN** |
| renderCallback | 0x8006120C | **UNKNOWN** |
| renderCallback | 0x800638D0 | **UNKNOWN** |
| renderCallback | 0x80064B40 | **UNKNOWN** |
| renderCallback | 0x800650C4 | **UNKNOWN** |

**Eight unlabelled player callbacks** are high-value targets for the next labelling pass (every one is called every few frames).

## 5. `Player.eventMarker` / `Player.renderMarker` are validity sentinels

Both fields flip between `-65536 (0xFFFF0000)` and `0` exactly when their paired callback is installed/cleared. They look like "callback is live" guards consumed by the dispatcher (skip slot when marker == 0). No data carried in the value beyond live/dead.

## 6. `Player.{worldX,worldY}` vs `GS.player_render_offset_{x,y}` lag

Jaccard = 0.999 (1618/1620 same frames). The 2-frame lag is the respawn path: `Player.world{X,Y}` get the new spawn coords before the render cache picks them up. **`GS.player_render_offset_{x,y}` is a per-frame mirror written by the renderer, not authoritative position.**

## 7. Render & screen-rect coupling

| Pair | Jaccard | Interpretation |
|---|---|---|
| `Player.screenX1 ↔ screenX2` | 0.935 | Sprite rect translates as a unit |
| `Player.hitboxHeight ↔ hitboxOffsetY` | 0.952 | Stand/duck swaps both atomically |
| `Player.pFrameTable ↔ pPaletteData` | 0.945 | Animation transition swaps both |

---

# Boss Trace Analysis — June 21, 2026 (Shriney Guard / MEGA)

## Trace
`game_watcher/logs/struct_watch_20260621_110109.jsonl` — 11,009 events over
2233 frames (~44.7 s @ 50 Hz PAL). Boot override loaded MEGA / Stage 0
directly via dynarec-safe vsync poller (no breakpoints, see `make watcher
LEVEL=5 STAGE=0`). Two complete boss attempts captured: died on
intended-killing-blow during attempt 1, defeated on attempt 2.

## 1. Shriney Guard combat loop (verified)

`PS.boss_hp` lives at `PlayerState[+0x1D]` and is initialised to **3** by
`InitShrineyGuardBoss` (confirms `docs/systems/boss-entity-pattern.md`).

| Event | Signature (same frame, unless noted) |
|---|---|
| **Hit boss** | `GS.bounce_active_flag == 1` AND `GS.screen_shake_index` re-set to **19** AND `PS.boss_hp -= 1` AND `Player.eventCallback` flips through `PlayerCallback_CollisionHandlerWithQueue (0x8005CEC8)` AND `Player.{pFrameTable,pPaletteData}` swap to hit-anim assets |
| **Shriney slam** (boss's own attack) | `GS.screen_shake_index` re-set to **19** with **no** `PS.boss_hp` change; bounce_active may or may not be 1 |
| **Player death impact** | `GS.screen_shake_index` set to **28** (single occurrence at f929; bigger than every hit/slam shake) |
| **Boss defeat** | Final HP→0 hit (same signature as normal hit), then `GS.level_active 1→0` ~120 frames later, then `GS.direct_level_load = 99` (magic value) ~80 frames after that |

Shake intensity is a perfect linear decay (`-1` per frame); a "re-trigger"
shows up as `new_num > old_num` mid-decay (observed at f1360 `6→19` and
f1541 `5→19`). **Use `new > old` not `old == 0` to detect shake events.**

### Full boss-fight timeline (one row per logical event)

```
f405   boss_hp 0→3                    Shriney spawned
f592   shake 0→19  +  hp 3→2          HIT #1 (attempt 1)
f758   shake 0→19  (no hp change)     Shriney slam
f805   shake 0→19  +  hp 2→1          HIT #2 (attempt 1)
f929   shake 0→28                     PLAYER DEATH IMPACT
f1049  advance_level_flag 0→1         death/respawn requested (1-frame pulse)
f1051  hp 1→0  +  lives 5→4           PS reset by death + lives decrement
f1053  hp 0→3                         Shriney re-spawned on retry
f1180  shake 0→19  +  hp 3→2          HIT #1 (attempt 2)
f1346  shake 0→19  (no hp change)     Shriney slam
f1360  shake 6→19 +  hp 2→1           HIT #2 (attempt 2) — overlapping shake
f1526  shake 0→19  (no hp change)     Shriney slam
f1541  shake 5→19 +  hp 1→0           KILL  — overlapping shake
f1600  shake 0→19                     Boss defeat shake
f1662  level_active 1→0               MEGA marked inactive
f1740  direct_level_load 0→99         exit-to-next pulse
f1741  direct_level_load 99→0         consumed
```

## 2. `boss_hp` reset on player death (not boss death)

When the player died (f1051), `PS.boss_hp` was forced 1→0 in the *same
frame* as `PS.lives--`, then re-initialised to 3 two frames later (f1053).
The boss had **not** taken a third hit — this is a side-effect of the
death/respawn sequence flushing `InitShrineyGuardBoss` again. Implication:
**`PS.boss_hp == 0` is NOT a reliable "boss defeated" signal on its own**;
must be qualified with `lives` unchanged.

The real "boss defeated" signal is:
`PS.boss_hp` hits 0 **without** `PS.lives` decrementing in the same frame.

## 3. Player FSM during boss bouncing

The bouncing-on-boss loop cycles four `tickCallback` values per bounce:

| tickCallback | Symbol | Phase |
|---|---|---|
| `0x8005B414` | `PlayerTickCallback` | Airborne / falling onto boss |
| `0x8005BBAC` | `PlayerState_CooldownTick` | Mid-bounce contact frame |
| `0x8005BB80` | `PlayerState_FrameCountTick` | Post-hit pause (timer-gated) |
| `0x8005BAD0` | `PlayerState_IdleRandom` | Recovery / landing |

`eventCallback` cycles in parallel:

| eventCallback | Symbol | Notes |
|---|---|---|
| `0x8005C400` | `PlayerEntityCollisionHandler` | Default (between bounces) |
| `0x8005CF24` | `PlayerBounceCallback` | Active while bouncing on enemy (rising edge correlates with `bounce_active_flag 0→1`) |
| `0x8005CEC8` | `PlayerCallback_CollisionHandlerWithQueue` | Fires the exact frame boss HP decrements (queued hit-event) |
| `0x8005CC84` | `PlayerCallback_EventHandlerWithQueue` | Re-arms after queue drains |
| `0x8005C1C4` | `PlayerEntityEventHandler` | Idle / between events |

`renderCallback` toggles among now-labelled handlers:

| renderCallback | Symbol |
|---|---|
| `0x80061180` | `ApplyEntityPositionUpdate` |
| `0x8006120C` | `PlayerCallback_HorizontalWallCollision` |
| `0x800638D0` | `PlayerCallback_HandleMovementAndCollision` |
| `0x80064B40` | `PlayerCallback_WalkingCollisionHandler` |
| `0x800650C4` | `PlayerCallback_FallingPhysicsMain` |

**Update**: this nukes 5 of the "UNKNOWN renderCallback" entries from the
GLEN-trace section above — all five appear in `symbol_addrs.txt`.

## 4. Level-exit trigger uses magic `direct_level_load = 99`

Levels 0-25 are valid; **`GS.direct_level_load = 99`** appears to be a
sentinel meaning "use the level's stored next-level pointer" or "advance to
results screen". On Shriney Guard victory this pulse happens at f1740,
followed by an immediate clear (f1741, → 0). No other field changes between
f1662 (`level_active 1→0`) and f1740, suggesting a timed wait.

Worth verifying against `GetNextLevelInSequence` / `SeekToLevelInSequence
(0x8007A3AC)` in `src/lvlload.c` to see whether 99 is the explicit "next in
campaign" code or whether it just means "consume the queue".

## 5. Player Y trajectory confirms pit-death

The intensity-28 shake at f929 corresponds to the player falling into a pit:

```
f889..f924   worldY rises 141→124 then drops to 145  (jump arc)
f925..f1051  worldY UNCHANGED for 128 frames         (death animation / OOB)
f1052        worldY = 20                              (respawn at top of arena)
```

The 128-frame static-Y window is the death-animation lock-out — confirms
that on death the engine stops Y-physics until respawn warps the entity
to `GS.spawn_{x,y}` (matches the GLEN-trace section 2 T+2 finding).

## 6. New GameState fields exercised (no prior log coverage)

| Offset | Field | Meaning (from this trace) |
|---|---|---|
| `+0x60` | `bounce_active_flag` | 1 while player is in bounce-on-enemy contact (18 transitions over the fight) |
| `+0x130` | `bg_color_change_flag` | Pulses 0→1→0 around respawn (palette fade prep) |
| `+0x146` | `advance_level_flag` | 1-frame pulse before each respawn (death-acknowledged) |
| `+0x148` | `direct_level_load` | u8 destination level; `99` = sentinel for end-of-level transition |
| `+0x170` | `level_active` | 1 while gameplay tickling, 0 during death/transition windows |
| `+0x171` | `password_levels` | Touched (no numeric change captured — likely string/struct) at f404 (load) and f1541 (victory) |
| `+0x12A` | `glide_boss_flag` | Set to **40** at f387 during MEGA load (not a boolean; matches the "level flag = 0x28" pattern from `docs/blb/level-metadata.md` — needs decode) |

## 7. Re-usable detection rules for boss traces

```sql
-- All boss-impact moments (player landed a hit OR Shriney slammed)
SELECT frame, new_num AS shake_intensity
FROM events
WHERE watch='GS' AND field='screen_shake_index'
  AND new_num > old_num
  AND new_num <> 28;       -- 28 = death impact, not a combat shake

-- Just the confirmed player hits (HP went down in same frame)
SELECT s.frame, s.new_num AS shake_intensity
FROM events s
JOIN events h ON h.session = s.session AND h.frame = s.frame
            AND h.watch='PS' AND h.field='boss_hp' AND h.delta = -1
WHERE s.watch='GS' AND s.field='screen_shake_index' AND s.new_num > s.old_num;
```

## 8. Player bounce-on-boss has a deterministic `worldY` signature

Re-examining `Player.worldY` in the 12 frames surrounding each successful
boss hit reveals the same 14-frame pattern at every hit (verified on
f592, f805, f1180):

```
Δ-12..Δ-3   worldY climbs +6..+8 per frame   (falling down onto boss)
            f580..f589:  85→90→96→102→108→115→122→129→137→145
Δ-2         worldY event ABSENT                (Y-physics paused 1 frame)
Δ-1         worldY event ABSENT                (Y-physics paused 1 frame)
Δ +0        worldY snaps back to ~155          (bounce-impact)
Δ+1..Δ+5    worldY decreases ~5/frame          (rebound up off the boss)
            f593..f597:  149→144→139→133→128
```

The two-frame "quiet window" at `Δ-2`/`Δ-1` (where neither `worldY` nor
`velocityY` changed) is the **collision-resolve pause** — Y-physics is
gated by the contact frames in `PlayerCallback_CollisionHandlerWithQueue`
before the bounce impulse is applied. **This Y-trajectory pattern is a
cleaner bounce detector than `velocityY` (which oscillates ±25000 every
frame from sprite-pose churn) and than `screen_shake_index` (which can
re-fire mid-decay).**

```sql
-- Detect a successful bounce-on-boss using ONLY player-Y physics
-- (no boss watch needed). Match on: 10+ frames of monotonic +ΔY,
-- followed by 2 frames of no ΔY, followed by a single negative ΔY
-- snap of >-5. Window: ~14 frames per hit.
```

`velocityY` is unreliable in this range: the 12-frame pre-hit shows
chaotic ±25000..±32000 swings (these are the sprite vertical-bob
animation values, not actual physics velocity). Stick to `worldY` deltas
for hit-detection.

## 9. Player animation pointer catalogue (FSM ↔ pFrameTable correlation)

The same 11k-event trace shows `Player.pFrameTable` cycles through
**13 unique pointers** during the boss fight. Joined against
`Player.tickCallback`, the dominant FSM/anim co-occurrences are:

| tickCallback | symbol | pFrameTable | role | frames |
|---|---|---|---:|---:|
| `0x8005BBAC` | `PlayerState_CooldownTick` | `0x801388E0` | Klayman bounce-cooldown anim (primary post-bounce pose) | 5319 |
| `0x8005BBAC` | `PlayerState_CooldownTick` | `0x8013C610` | Klayman bounce-cooldown anim (secondary frame set) | 2415 |
| `0x8005B414` | `PlayerTickCallback` | `0x800AF868` | Standing idle (main tick + low-addr idle frames) | 821 |
| `0x8005BAD0` | `PlayerState_IdleRandom` | `0x8015677C` | Random idle anim (yawn / blink) | 372 |
| `0x8005B414` | `PlayerTickCallback` | `0x8013211C` | Walk / run anim during main tick | 346 |
| `0x8005BBAC` | `PlayerState_CooldownTick` | `0x800AFE08` | Cooldown into idle (transition frames) | 332 |
| `0x8005BB80` | `PlayerState_FrameCountTick` | `0x801285A8` | Brief post-hit pause anim (timer-gated) | 315 |
| `0x8005B414` | `PlayerTickCallback` | `0x80131F48` | Walk variant (left/right facing) | 245 |
| `0x8005B414` | `PlayerTickCallback` | `0x80152C80` | Jump-ascending anim | 238 |
| `0x8005B414` | `PlayerTickCallback` | `0x801507DC` | Pre-jump windup or falling-onset frames | 65 |

The **`PlayerState_CooldownTick + 0x801388E0`** combo accounts for 50%+
of the entire fight's per-frame samples — i.e., most of "fighting the
boss" is the engine running the bounce-cooldown FSM with the bouncing
sprite. Worth labelling these 13 pointers in `symbol_addrs.txt` next:
they're all in the `0x800AF000..0x80170000` data-section range
(player.c / player_anim.c constant tables, currently `D_8013xxxx`
labels in `asm/data/`).

The two low addresses (`0x800AF868`, `0x800AFD54`, `0x800AFE08`,
`0x800B05D0`, `0x800B0ABC`) cluster in `asm/data/` near
`player_anim`-named files — likely the canonical idle/walk frame tables.
The higher addresses (`0x80128000+`) cluster with combat / hit-reaction
animations.

## 10. Implications for `src/player.c` / `src/playst.c` decompilation

- `PlayerCallback_CollisionHandlerWithQueue (0x8005CEC8)` is **the
  bounce-hit dispatcher** — it gates the 2-frame Y-physics pause +
  applies the bounce impulse + queues `PlayerState_CooldownTick` as the
  next tickCallback. High-value decomp target (still `INCLUDE_ASM`).
- `PlayerState_CooldownTick (0x8005BBAC)` is **already decompiled** in
  [src/playst.c](../../../src/playst.c#L57). The trace clarifies its
  semantics: it decrements `timer13C` then tail-calls `PlayerTickCallback`
  but does **not itself** check `timer13C == 0` — so the state-exit
  transition must happen inside `PlayerTickCallback` (still ASM, size
  `0x6BC`) or in the event handler. Worth annotating with this trace
  evidence as a comment.
- `PlayerState_FrameCountTick (0x8005BB80)` is **already decompiled**
  in [src/playst.c](../../../src/playst.c#L53). Same pattern: increments
  `jumpParam` then tail-calls `PlayerTickCallback`. The per-frame
  counter pattern (315 frames in the trace) confirms it's a brief
  windup state — not a long-running idle.
- `PlayerState_IdleRandom (0x8005BAD0)`, size `0x94` — still
  `INCLUDE_ASM`. Trace shows transitions every ~120 frames (~2.4 s @
  PAL) — picks a random idle anim each cycle. **Easy decomp** given the
  trace-validated cadence.
- `PlayerTickCallback (0x8005B414)`, size `0x6BC` — still `INCLUDE_ASM`.
  The big one. All four `PlayerState_*` ticks tail-call into it, so
  this is the actual FSM dispatcher; the smaller ticks just maintain
  per-state counters on top. **Decompiling this unlocks the entire
  player FSM** (animation transitions, callback installs, state-exit
  conditions).

Trace-validated insight: the four `PlayerState_*` ticks are NOT
self-driving FSM states — they are "main + side-effect" wrappers
around `PlayerTickCallback`. The state-transition logic lives in the
main tick, not the per-state ticks.

---

# FINN (3D Boat Level) Trace Analysis — June 21, 2026

## Traces
- `game_watcher/logs/struct_watch_20260621_111050.jsonl` — 12,824 events, took damage → returned to menu (no respawn)
- `game_watcher/logs/struct_watch_20260621_111517.jsonl` — 27,185 events, **full clear** including level-exit countdown (the trace this section quotes from)

FINN is a 3D top-down boat-driving level (the "Finn-Run" tank-control vehicle
section) — fundamentally different from the platformer levels. The watcher
ran on Level 4 / Stage 0 for ~60 s @ 50 Hz PAL.

## 1. Entire level runs on ONE tick callback (`FinnMainTickHandler`)

`Player.tickCallback` had **only one transition over the entire trace**:

| Frame | old | new | Meaning |
|---|---|---|---|
| 2910 | `FinnMainTickHandler (0x8006EFC8)` | `FinnTick_LevelExitCountdown (0x8006F0A8)` | Player crossed the exit zone — countdown installed |

`Player.eventCallback` and `Player.renderCallback` **never changed**. This is a
massive departure from platformer levels (Shriney trace had 30+ transitions
over a 45 s fight). All FINN logic is encapsulated inside `FinnMainTickHandler`,
which internally dispatches to `FinnHandleInput`, `FinnVehicleMovementUpdate`,
`FinnUpdateRotationSprite`, etc. — see [src/finn.c](src/finn.c).

## 2. Our `Entity` schema is too small for FINN — vehicle state lives at +0x80…+0x120

The struct_watcher captures only the first 0x80 bytes of the player Entity.
But [src/finn.c](src/finn.c) reveals FINN entities use:

| Offset | Field (per `src/finn.c` typedefs) | Source typedef |
|---|---|---|
| `+0x10D` | `stateFlag` | `FinnSubentityStateFlags` |
| `+0x10F` | `stateValue` | `FinnSubentityStateFlags` |
| `+0x104..+0x108` | `pairA`, `pairB` (s32 pair) | `FinnPairEntity` |
| `+0x100` | `stateValue` (u32) | `FinnStateValueEntity` |
| `+0x100` | `spawnCountdown` (u8) | `FinnSpawnCountdownEntity` |
| `+0x114` | `voiceHandle` (s32) | `FinnVoiceEntity` |
| `+0x118` | `exitTimer` (u8) | `FinnLevelExitEntity` |
| `+0x11A` | `exitFlag` (u8) | `FinnLevelExitEntity` |

**Implication**: the boat's real heading/thrust/depth fields are in this
+0x80…+0x120 range and are invisible to our current `Entity` schema.
A targeted FINN schema (or extending `Entity` to ~0x120 bytes) would expose
them. Worth doing before the next FINN/RUNN trace.

## 3. `Player.velocityX/Y (+0x6C/+0x6E)` are NOT control velocity in FINN

Bucketed s16 velocity values by held button:

| Held | n | velocityX range | mean | velocityY range | mean |
|---|---|---|---|---|---|
| X (thrust) | 1189 | -32612 … 32750 | 374 | -32712 … 32726 | 335 |
| (none) | 845 | -32634 … 32614 | 369 | -32596 … 32406 | 428 |
| L (turn) | 54 | -31304 … 32624 | -2087 | -32426 … 32016 | -276 |
| R+O (turn) | 38 | -31770 … 31760 | -644 | -32556 … 25828 | -3467 |
| R+X+O | 71 | -32636 … 32618 | 6400 | -31026 … 32032 | 1559 |
| L+X | 50 | -32594 … 31550 | 1507 | -32708 … 32102 | 4856 |

The ±32750 range with means ≈ 0 (vs. typical ±0.5 px/frame oscillation) is
the **water wave-bob animation**, not the boat's world velocity. The real
control velocity must live in the higher-offset fields (see §2).

Compared to the boat's actual measured world-space speed:

| Window | worldX delta | frames | px/frame |
|---|---|---|---|
| f402 → f502 | 455 → 620 = 165 | 100 | **1.65** |
| f502 → f602 | 620 → 762 = 142 | 100 | **1.42** |
| f1002 → f1102 | 1080 → 1256 = 176 | 100 | **1.76** |

So the boat moves **~1.5 px/frame** in worldX, but the velocityX field only
shows ±0.5 px/frame oscillation. **`Player.velocityX/Y` is sprite-bob, not
boat-velocity, in this level.**

## 4. Tank-control button mapping (decoded from `buttons_held` traces)

PSX button bitmap (verified against [scripts/game_watcher.lua#L880-905](scripts/game_watcher.lua)):
`0x8000=Left 0x4000=Down 0x2000=Right 0x1000=Up 0x0040=X 0x0020=Circle 0x0080=Square 0x0010=Triangle`

Most-held combinations during FINN clear:

| Mask | Buttons | Held frames | Likely role |
|---|---|---|---|
| `0x0040` | X alone | 72 | Forward thrust |
| `0x2060` | Right + Circle + X | 17 | Turn right + secondary + thrust |
| `0x8001` | Left + (Select?) | 15 | Turn left (the 0x0001 bit on Left only is unexplained — possibly an analog-rest sentinel; harmless) |
| `0x2020` | Right + Circle | 13 | Turn right + secondary |
| `0x8041` | Left + X + (Select?) | 10 | Turn left + thrust |

Tank-control confirmed: **X = thrust, L/R = rotate**. Circle's role is
unclear (could be brake/strafe — needs a targeted trace pressing only Circle).
No Up/Down direction key was ever held during the clear, ruling out
forward = D-pad-up in this level.

## 5. Boat trajectory (worldX/worldY at 100-frame intervals)

| Frame | Held | worldX | worldY |
|---|---|---|---|
| 302 | – | 329 | 0 |
| 402 | X | 455 | 610 |
| 902 | X | 920 | 552 |
| 1502 | X | 1703 | 619 |
| 2041 | X | 2250 | 625 |
| 2545 | – | 2812 | 486 |
| 2959 | X | 3154 | 413 |

- worldX is **monotonic increasing** (boat goes left → right along a
  ~3000-unit track)
- worldY stays in a narrow 410..640 band — the level is essentially a
  side-scrolling 3D rail with limited Y wiggle
- f302 worldY=0 → f402 worldY=610 is the spawn-snap (boat appears at
  start of track)

## 6. GameState is barely touched in FINN

| GS field | Changes during gameplay (f>500) |
|---|---|
| `render_list_head` | 23 (sub-entity churn — boat parts, exit gate, etc.) |
| `collision_list_head` | 19 |
| (everything else excluding camera & frame_counter) | 0–2 |

No `bounce_active_flag`, no `screen_shake_index`, no `advance_level_flag`,
no `level_active` toggles. The 3D physics never touches the platformer
GameState fields. **FINN is a self-contained subsystem that ignores most
of the GameState contract.**

## 7. Damage → menu (NOT respawn) — confirmed by the 12k-event trace

In the first FINN session the player took damage and **returned to the
menu** instead of respawning in-place. This is a fundamental difference
from platformer levels (which respawn via `advance_level_flag` + warp to
`spawn_{x,y}`). FINN appears to treat damage as a full level-exit, not a
checkpoint reset. Worth verifying with a 3-watch trace (PS, GS, Player)
of a deliberate FINN death — we didn't capture the exact transition
in this round.

## 8. End-of-level signal: `tickCallback` swap (no `direct_level_load = 99`)

Where Shriney Guard pulsed `GS.direct_level_load = 99` ~200 frames after
victory, **FINN uses an entirely different exit path**:

- f2910: `Player.tickCallback` swaps from `FinnMainTickHandler` to
  `FinnTick_LevelExitCountdown` (player drove into the exit zone)
- Then `FinnTick_LevelExitCountdown` runs an internal frame counter at
  `+0x118` (per `FinnLevelExitEntity` typedef) and sets `+0x11A` when
  the timer hits 0 — that flag is presumably what advances the level
- `GS.direct_level_load` was 0 throughout — the boss-level "direct_level_load=99"
  pattern is **NOT universal**; it's a boss/platformer convention

This means there are **at least two distinct level-exit mechanisms** in
the engine:
1. **Platformer/Boss**: `GS.advance_level_flag` pulse + `GS.direct_level_load=99` sentinel
2. **FINN**: tickCallback swap to a countdown function with internal timer state

## 9. Schema gap & next steps

To get useful FINN traces in the future:

1. **Extend the `Entity` Lua schema** (or create a `FinnEntity` schema) to
   cover +0x80 … +0x120. Reference fields from `src/finn.c` typedefs:
   - `+0x100` u32 stateValue
   - `+0x104` s32 pairA
   - `+0x108` s32 pairB
   - `+0x10D` u8 stateFlag
   - `+0x10F` u8 stateValue (u8 variant)
   - `+0x114` s32 voiceHandle
   - `+0x118` u8 exitTimer
   - `+0x11A` u8 exitFlag
2. Sample the FINN player entity beyond 0x80 to find the real boat
   velocity/heading fields (probably 2 u16 or s16 in the +0x80…+0x100 region).
3. Do a **single-button-isolation trace**: hold only Circle (then only
   Triangle, only Square) to identify the secondary FINN inputs.
4. Capture a **deliberate FINN death** to verify damage-to-menu transition
   and find the canonical "FINN death" signal.

---

# FINN Boat Vehicle Trace — June 21, 2026 (FinnPlayerEntity overlay)

## Setup

The `Makefile`'s `watcher` target was extended to add a `FINN` overlay
watch when `LEVEL=4`, using the existing
[scripts/schemas/FinnPlayerEntity.lua](scripts/schemas/FinnPlayerEntity.lua)
schema (size 0x114), field-filtered to 7 fields:

| Offset | Field | Type | Role |
|---|---|---|---|
| `+0x100` | `pInput` | InputState * | Controller input pointer |
| `+0x104` | `wakeEntity_or_velocityX` | s32 | Forward-thrust accumulator (NOT world-X) |
| `+0x108` | `velocityY_or_stateCallback` | s32 | Lateral/strafe component |
| `+0x10C` | `rotationAngle` | s16 | **Boat heading angle (mod 65536)** |
| `+0x10E` | `rotationSpriteBucket` | u8 | Cached coarse sprite-rotation bucket |
| `+0x10F` | `rotationVelocity` | s8 | **Per-frame angular velocity** |
| `+0x110` | `soundHandle_or_inputFlags` | s32 | (Actually a frame/voice counter in FINN; updates ~every frame) |

Trace `game_watcher/logs/struct_watch_20260621_113008.jsonl` — 24,764
events over frames 130–2247 (~42 s of gameplay ending in death).

## 1. Tank-control angular dynamics are FULLY decoded

`rotationAngle` deltas bucketed by held buttons:

| Held buttons | n | Δ range | mean Δ | median Δ |
|---|---|---|---|---|
| L | 150 | -64 … -16 | **-57.3** | **-64** |
| R+O | 82 | +16 … +64 | **+58.2** | **+64** |
| R+X+O | 110 | +16 … +64 | +50.0 | +64 |
| L+X | 54 | -64 … -16 | -45.6 | -48 |
| X | 192 | -56 … +56 | +3.7 | +8 |
| NONE | 95 | -56 … +56 | -6.7 | -16 |

**Confirmed**:
- **L pressed → -64 angular units/frame** (max left turn)
- **R+O pressed → +64 angular units/frame** (max right turn)
- **Combining turn + X thrust attenuates angular speed** to ~±50 (-22%)
- Turn rate is unit `64 angle/frame` at max; `65536 angle / 64 = 1024 frames ≈ 20.5 s` for a full revolution at PAL 50fps
- The s8 `rotationVelocity` field at +0x10F holds the per-frame Δ directly
  (range -64 … +64 matches the angle deltas perfectly)

## 2. Tank-controls have **momentum/inertia**

Tail of trace shows release-decay over 9 frames after L is released:

```
f1987  NONE  Δ=-56
f1988  NONE  Δ=-48
f1989  NONE  Δ=-40
f1990  NONE  Δ=-32
f1991  NONE  Δ=-24
f1992  NONE  Δ=-16
f1993  NONE  Δ= -8
       (then 0)
```

Symmetric ramp-up when L is pressed again:

```
f2127  L     Δ=-16
f2128  L     Δ=-32
f2129  L     Δ=-48
f2130  L     Δ=-64     ← full rate after 4 frames
```

So the boat has **±8 units/frame angular acceleration**, capping at
±64 units/frame angular velocity. Press for **4 frames to reach max
turn rate**, release for **9 frames** to coast to a stop. Asymmetric —
deceleration is slower than acceleration, which gives the boat its
"floaty" tank feel.

## 3. Circle (O) is held alongside R but has no observable effect on rotation

The `R+O` and `R+X+O` buckets show identical angular behaviour to a
pure-R press would (R alone never appears in the trace). Most likely
the player just always pressed Circle alongside R out of habit — its
real role (firework? horn? cannon?) isn't visible in the watched fields.
**Needs a single-button-isolation trace to pin down.**

## 4. Real velocity fields use 16.16 fixed-point, range ±1.95/frame

`wakeEntity_or_velocityX` (+0x104) and `velocityY_or_stateCallback`
(+0x108) are both s32 16.16 fixed-point. By held button:

| Held | velocityX mean | velocityY mean | Interpretation |
|---|---|---|---|
| X | +0.75 | -0.15 | Forward thrust dominates |
| NONE | +0.89 | +0.09 | Drift forward (boat has inertia!) |
| L | +0.82 | +0.97 | Forward + lateral (turning curves trajectory) |
| R+O | +1.24 | -0.59 | Faster forward + opposite lateral |
| R+X+O | +0.91 | -0.48 | Right curve with thrust |

These match the **~1.5 px/frame world speed** measured from worldX
deltas in the prior trace — confirming these are the *real* control
velocity fields (vs the +0x6C/+0x6E sprite wave-bob).

The schema's name `wakeEntity_or_velocityX` is appropriate: during
init the field holds a `wakeEntity` pointer (the wake child sprite),
and after `FinnVehicleMovementUpdate` overwrites it with the
fixed-point velocity component every frame.

## 5. `rotationSpriteBucket` is the cached 16-way sprite index

Only 119 changes for 683 angle changes → ratio 5.74, matching a coarse
quantization. `65536 / 16 = 4096` angle units per bucket. Sprite swaps
in the previous trace counted **86 pFrameTable swaps** — close to the
~119 bucket-changes here (allowing for some swap merging by the
animation system).

## 6. `soundHandle_or_inputFlags` (+0x110) is the noisy field — drop it from future filters

1861 changes (3× more than any other field). Values range 1..276
incrementing-then-resetting — looks like a **per-frame SPU counter or
sample-timer**, not packed input flags. Suggest filtering it OUT of the
FINN overlay watch by default — it's pure noise for movement analysis.

## 7. Pickup counts dropped this run (death cut it short)

Compared to the full-clear trace:

| Field | Full clear (111517) | This run (113008 — died) |
|---|---|---|
| `orb_count` | 13 collected | 8 collected |
| `lives` | 6 gained | 2 gained |
| `phoenix_hands` | 2 | 1 |
| `phart_heads` | 1 | 1 |

`level_complete` was set once (during init, not as exit signal — same as
prior trace).

## 8. Next steps

1. **Drop `soundHandle_or_inputFlags` from FINN overlay** (noise).
2. Add **per-frame world-velocity from rotation+thrust** to verify the
   physics model: `worldX += vel*cos(θ)`, `worldY += vel*sin(θ)`.
3. **Decode Circle** with an isolated-button trace.
4. Look at the corresponding source: [src/finn.c](src/finn.c) has
   `FinnHandleInput` (input → rotationVelocity/thrust) and
   `FinnVehicleMovementUpdate` (rotationVelocity → rotationAngle →
   wakeX/velY) — both still `INCLUDE_ASM`. With the trace-validated
   physics constants (±8/frame angular accel, ±64 max angular vel,
   ±1.95/frame max linear), these are now feasible to decompile by hand
   or m2c.
