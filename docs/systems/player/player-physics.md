# Player Physics System

This document describes the physics constants and movement system for implementing a basic player entity.

## Overview

The player uses 16.16 fixed-point math for smooth sub-pixel movement. Position is stored as whole + fractional parts.

## Position Storage

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x68 | s16 | x_whole | X position (pixels) |
| 0x6A | s16 | y_whole | Y position (pixels) |
| 0x6C | u16 | x_frac | X fractional (0-65535) |
| 0x6E | u16 | y_frac | Y fractional (0-65535) |

Position update formula (from `ApplyEntityPositionUpdate` @ 0x80061180):
```c
// Combine whole + frac into 16.16 fixed
int32 pos_x = (x_whole << 16) | x_frac;
int32 pos_y = (y_whole << 16) | y_frac;

// Apply push forces (respecting direction flags)
int32 force_x = push_x;
int32 force_y = push_y;

if (facing_left) {
    force_x = -force_x;  // Negate for left movement
}
pos_x += force_x;

if (moving_up) {
    force_y = -force_y;  // Negate for upward movement
}
pos_y += force_y;

// Split back to whole + frac
x_whole = pos_x >> 16;
y_whole = pos_y >> 16;
x_frac = pos_x & 0xFFFF;
y_frac = pos_y & 0xFFFF;

// NOTE: push_x and push_y are then cleared by the calling function
```

## Movement System

**CRITICAL: Primary movement uses push forces at +0x160/+0x162, NOT the velocity fields at +0xB4/+0xB8!**

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x74 | u8 | facing_left | Direction flag: 0=right, 1=left (negates push_x) |
| 0x75 | u8 | moving_up | Direction flag: 0=down, 1=up (negates push_y) |
| 0x160 | s16 | push_x | **Primary X movement force** (pixels per frame) |
| 0x162 | s16 | push_y | **Primary Y movement force** (pixels per frame) |
| 0xB4 | s32 | unknown_vx | Physics state (NOT used for position updates) |
| 0xB8 | s32 | unknown_vy | Physics state (NOT used for position updates) |

## Player Dimensions

### Bounding Box

From `PlayerTickCallback` at 0x8005b414:

**Normal Mode** (estimated from collision checks):
```c
// 4 wall collision points at Y offsets:
// Y-15, Y-16, Y-32, Y-48 (head)
// Suggests player is ~48 pixels tall

// Ceiling check at Y-64 (0x40)
// Forward check at X±12 (0x0C)

BoundingBox normal = {
    .left = -8,    // estimated
    .top = -48,    // head height
    .right = 8,    // estimated  
    .bottom = 0    // feet at origin
};
```

**Shrink Mode** (confirmed from code):
```c
// Set when shrinkFlag (0x1B0) is non-zero
BoundingBox shrink = {
    .left = -5,    // 0xFFFB
    .top = -10,    // 0xFFF6
    .right = 10,   // 0x0A
    .bottom = 10   // 0x0A
};
```

### Collision Check Points

Wall collision (`CheckWallCollision` at 0x80059bc8) checks 4 vertical points:
- Y - 15 (0x0F) - lower body
- Y - 16 (0x10) - lower body
- Y - 32 (0x20) - mid body
- Y - 48 (0x30) - head

Floor detection checks at:
- Y - 7 (0x07) - inside player
- Y + 2 - just below feet
- Y + 16 (0x10) - one tile below

## Movement Speeds (Measured from Gameplay Traces)

**Source:** PHRO Stage 1 gameplay analysis - 949 velocity samples over 4235 frames  
**Method:** Position delta / frame delta between samples (4-frame intervals)  
**Trace:** `trace_20260114_214044_unknown_stage0_f0.jsonl`

| Player State | Avg Speed (px/frame) | Max H Speed | Min Vy | Max Vy | Sample Count |
|--------------|----------------------|-------------|--------|--------|--------------|
| IdleBlink | 2.91 | 3.25 | 0.0 | 7.0 | 114 |
| IdleLook | 2.27 | 3.25 | -6.5 | 8.0 | 693 |
| PlayerTickCallback (0x8005B414) | 0.61 | 3.0 | -0.25 | 6.25 | 42 |
| Idle | 0.25 | 1.75 | 0.0 | 4.0 | 16 |
| Cutscene (0x8001CB88) | 0.82 | 65.82 | -3.0 | 69.7 | 82 |

**Key Findings:**
- Idle animation states (IdleBlink, IdleLook) show most movement due to breathing/looking animations
- Vertical speeds range from -6.5 (rising) to +8.0 (falling) px/frame
- Horizontal speeds typically 2-3 px/frame during normal gameplay
- Cutscene state shows extreme velocities (65+ px/frame) for teleportation effects
- Push forces at +0x160/+0x162 are applied then cleared each frame (sample as 0)

## Physics Constants (TO BE VERIFIED via Direct Memory Reading)

These values need extraction via PCSX-Redux memory inspection during active movement states.
The push_x/push_y fields are cleared after application, so constants must be read from the
input handler or state-specific movement code.

### Movement (Estimated)

| Constant | Estimated Value | 16.16 Fixed | Notes |
|----------|----------------|-------------|-------|
| Walk Speed | 2.0 px/frame | 0x20000 | Horizontal ground movement |
| Run Speed | 3.0 px/frame | 0x30000 | If run button held |
| Air Control | 1.5 px/frame | 0x18000 | Horizontal in air |
| Jump Velocity | -8.0 px/frame | -0x80000 | Initial upward (matches max_vy) |
| Gravity | 0.5 px/frame² | 0x8000 | Downward accel |
| Max Fall Speed | 8.0 px/frame | 0x80000 | Terminal velocity (matches observed) |

### Scale Values

| Value | Meaning |
|-------|---------|
| 0x10000 | Full size (1.0) |
| 0xC000 | 75% size |
| 0x8000 | Half size (shrink powerup) |
| 0x4000 | Quarter size |

Scale affects collision box proportionally.

## Input Masks

From PSX controller (entity+0x100 → InputState):

| Mask | Button | Action |
|------|--------|--------|
| 0x0010 | Up | Look up / Climb |
| 0x0020 | Right | Move right |
| 0x0040 | Down | Crouch / Look down |
| 0x0080 | Left | Move left |
| 0x2000 | Circle | Context action |
| 0x4000 | X | Jump |
| 0x8000 | Square | Attack |

## State Machine

Player states are controlled via callbacks set at different entity offsets:

| Offset | Field | Purpose |
|--------|-------|---------|
| 0x00-0x04 | state_high + tickCallback | Main tick dispatch |
| 0x08-0x0C | secondaryCallback | Animation/sound |
| 0x104-0x108 | stateCallback | Current state handler |
| 0x98-0x9C | nextStateCallback | Queued state |

### Key States

| State | Entry Callback | Description |
|-------|----------------|-------------|
| Idle | `PlayerStateCallback_0` (0x80066ce0) | Standing still |
| Walking | Movement in `PlayerCallback_800638d0` | Horizontal movement |
| Jumping | `Callback_80067e28` | Airborne, going up |
| Falling | `PlayerStateCallback_2` | Airborne, going down |

## Collision Response

### Solid Tile (0x02)

When wall collision detects solid (attribute `'e'` = 0x65):
1. Stop horizontal movement
2. Align to tile boundary
3. Clear fractional position

From `PlayerCallback_800638d0`:
```c
if (hit_wall) {
    if (moving_left) {
        x_pos = FUN_8005a028(entity, check_x);  // Align to right of tile
    } else {
        x_pos = ((check_x & 0xFFFFFFF0) / scale) - 1;  // Align to left
    }
    x_frac = 0;
}
```

### Floor Detection

Ground check looks at Y+2 for solid tiles (attribute in range 0x01-0x3B):
- If solid found → on ground
- If empty (0x00) → falling
- If > 0x3B → special trigger

## Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800596a4 | CreatePlayerEntity | Initialize player |
| 0x8005b414 | PlayerTickCallback | Main per-frame update |
| 0x80061180 | PlayerCallback_80061180 | Position update from velocity |
| 0x800638d0 | PlayerCallback_800638d0 | Movement + collision |
| 0x8005a914 | PlayerProcessTileCollision | Tile trigger processing |
| 0x80059bc8 | CheckWallCollision | 4-point wall check |
| 0x80066ce0 | PlayerStateCallback_0 | Idle state entry |
| 0x80067e28 | Callback_80067e28 | Jump state entry |

## Animation System (Verified from Trace)

### Animation Frame Data

Animation tracked at entity offsets:
- `anim_frame` - Current frame in sequence
- `anim_end` - Last frame index (e.g., 7 for 8-frame walk)
- `anim_timer` - Countdown timer (decrements each tick)
- `anim_speed` - Timer reset value (e.g., 5 = 5 frames per anim frame)

### Walk/Run Animation Pattern

From trace at frames 435-467:
```
Frame 435: anim[1], timer=8, speed=5, end=7  (start walking)
Frame 439: anim[2], timer=8, speed=5, end=7  (+4 frames later)
Frame 443: anim[3], timer=8, speed=5, end=7
Frame 447: anim[4], timer=8, speed=5, end=7
Frame 451: anim[5], timer=8, speed=5, end=7
Frame 455: anim[6], timer=8, speed=5, end=7
Frame 459: anim[7], timer=8, speed=5, end=7  (reached end)
Frame 463: anim[5], timer=8, speed=5, end=7  (loops back to 5!)
Frame 467: anim[6], timer=8, speed=5, end=7
```

**Observations**:
- Animation advances every 4 game frames (at 60fps = 15fps animation)
- 8-frame walk cycle (frames 0-7)
- After reaching frame 7, loops back to frame 5 (not frame 0!)
- This creates a smooth 5→6→7→5→6→7 loop for sustained walking

### Animation Speed Values

| Speed | Game Frames | Real Time @ 60fps |
|-------|-------------|-------------------|
| 5 | ~4-5 frames | ~67-83ms |
| 8 | ~7-8 frames | ~117-133ms |
| 3 | ~2-3 frames | ~33-50ms |

Speed value appears to control timer reset, affecting frame advance rate.

## Godot Implementation Notes

### Basic Player Pseudocode

```gdscript
extends CharacterBody2D

# Constants (estimated - verify via tracing)
const WALK_SPEED = 2.0 * 60  # Convert to px/sec for Godot
const JUMP_VELOCITY = -8.0 * 60
const GRAVITY = 0.5 * 60 * 60  # px/sec²

# Collision
const NORMAL_HEIGHT = 48
const SHRINK_HEIGHT = 20

func _physics_process(delta):
    # Gravity
    if not is_on_floor():
        velocity.y += GRAVITY * delta
    
    # Input
    var direction = Input.get_axis("ui_left", "ui_right")
    velocity.x = direction * WALK_SPEED
    
    # Jump
    if Input.is_action_just_pressed("ui_accept") and is_on_floor():
        velocity.y = JUMP_VELOCITY
    
    move_and_slide()
```

### Collision Layer Setup

- Layer 1: Player
- Layer 2: Solid tiles (0x02)
- Layer 3: Platforms (one-way)
- Layer 4: Triggers (checkpoints, etc.)

## Verification Needed

To extract actual physics constants:

1. **Run game in PCSX-Redux** with breakpoints at:
   - 0x80061180 (position update) - read velocity values from 0xB4/0xB8
   - 0x80067e28 (jump start) - capture initial Y velocity

2. **Memory watch** entity+0xB4 and entity+0xB8 during:
   - Walking → capture walk speed
   - Jumping → capture initial jump velocity
   - Falling → capture gravity accumulation

3. **Compare** frame-by-frame position changes to calculate actual pixel speeds.

## Related Documents

- [Player System](player-system.md) - State machine overview
- [Player Normal](player-normal.md) - Normal platforming details
- [Collision System](collision.md) - Tile collision attributes
