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
- Sets g_GameStatePtr[0x60] = 1
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
