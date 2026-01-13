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

Position update formula (from `PlayerCallback_80061180`):
```c
// Combine whole + frac into 16.16 fixed
int32 pos_x = (x_whole << 16) | x_frac;
int32 pos_y = (y_whole << 16) | y_frac;

// Apply velocity (respecting direction flags)
if (facing_left) {
    pos_x -= velocity_x;
} else {
    pos_x += velocity_x;
}

if (moving_up) {
    pos_y -= velocity_y;
} else {
    pos_y += velocity_y;
}

// Split back
x_whole = pos_x >> 16;
y_whole = pos_y >> 16;
x_frac = pos_x & 0xFFFF;
y_frac = pos_y & 0xFFFF;
```

## Velocity Storage

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x74 | u8 | facing_left | 0=right, 1=left |
| 0x75 | u8 | moving_up | 0=down, 1=up |
| 0xB4 | s32 | velocity_x | X velocity (16.16 fixed) |
| 0xB8 | s32 | velocity_y | Y velocity (16.16 fixed) |
| 0x160 | s16 | push_x | External X push force |
| 0x162 | s16 | push_y | External Y push force |

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

## Physics Constants (TO BE VERIFIED)

These are **estimated values** based on typical PSX platformers. Actual values need extraction via PCSX-Redux tracing.

### Movement (Estimated)

| Constant | Estimated Value | 16.16 Fixed | Notes |
|----------|----------------|-------------|-------|
| Walk Speed | 2.0 px/frame | 0x20000 | Horizontal ground movement |
| Run Speed | 3.0 px/frame | 0x30000 | If run button held |
| Air Control | 1.5 px/frame | 0x18000 | Horizontal in air |
| Jump Velocity | -8.0 px/frame | -0x80000 | Initial upward |
| Gravity | 0.5 px/frame² | 0x8000 | Downward accel |
| Max Fall Speed | 8.0 px/frame | 0x80000 | Terminal velocity |

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
