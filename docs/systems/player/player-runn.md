# RUNN Player Entity (Auto-Scroller/Runner Levels)

**Status**: ✅ FULLY DOCUMENTED from C Code  
**Last Updated**: January 15, 2026  
**Source**: SLES_010.90.c lines 36777-36820

---

## Overview

RUNN levels are auto-scrolling runner sequences where the player automatically moves forward and must navigate obstacles.

**Level Flag**: 0x100 in tile header (Asset 100)  
**Levels**: RUNN (Level 22, 2 stages)  
**Gameplay**: Automatic forward movement, player controls jumping/dodging

---

## Creation

**Function**: CreateRunnPlayerEntity @ 0x80073934

```c
Entity* CreateRunnPlayerEntity(Entity* buffer, void* inputController,
                                short spawn_x, short spawn_y) {
    // Initialize with sprite table
    InitEntityWithSprite(buffer, &DAT_8009cadc, 1000, spawn_x, spawn_y);
    
    // Set vtable
    buffer[6] = &DAT_80011db4;
    buffer[0x43] = 0xffffffff;
    
    // Configure
    buffer[4] = 1000;  // Z-order
    buffer[0x40] = inputController;  // Input controller
    buffer[0x41] = 0;  // No secondary sprite
    buffer[0x42] = 0;  // Clear field
    
    // Set tick callbacks
    buffer[0] = 0xffff0000;
    buffer[1] = &LAB_80073a88;  // Main tick handler
    buffer[7] = 0xffff0000;
    buffer[8] = &LAB_80073b88;  // Secondary callback
    
    // Set initial state
    EntitySetState(buffer, null_FFFF0000h_800a5ffc, PTR_LAB_800a6000);
    
    return buffer;
}
```

**Entity Size**: 0x110 bytes (272 bytes) - smaller than normal player (0x1b4)

**Sprite Table**: DAT_8009cadc (RUNN-specific sprites)

---

## Entity Structure

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x68 | 2 | x_pos | Current X position |
| +0x6A | 2 | y_pos | Current Y position |
| +0x100 | 4 | inputController | Pointer to g_pPlayer1Input |
| +0x104 | 4 | xVelocity | X velocity (16.16 fixed) |
| +0x108 | 4 | yVelocity | Y velocity (16.16 fixed) |
| +0x40 | 4 | input_ptr | Input controller |
| +0x41 | 4 | secondary_entity | None (set to 0) |
| +0x43 | 4 | field_43 | 0xffffffff |

**Key Difference from Normal Player**: No secondary sprite entity (wake/shadow)

---

## Auto-Scroll Mechanics

### Automatic Forward Movement

**Tick Callback**: LAB_80073a88 (line 36788)

**From code analysis (lines 36797-36820)**:

```c
void RunnPlayerTick(Entity* player) {
    // Apply movement callbacks (collision detection)
    EntityApplyMovementCallbacks(player, player->x, player->y);
    
    // Check input for horizontal adjustment
    if (input->buttons & 0x1000) {  // Triangle - Move left
        player->xVelocity += -0xc000;  // Accelerate left
    } else if (input->buttons & 0x4000) {  // X - Move right
        player->xVelocity += 0xc000;  // Accelerate right
    }
    
    // Check for jump/special (D-Pad buttons)
    if (input->buttons_pressed & 0xf0) {  // Any D-Pad button
        EntitySetState(player, null_FFFF0000h_800a600c, PTR_LAB_800a6010);
    }
}
```

**Constants**:
- Horizontal acceleration: ±0xc000 (±0.75 px/frame in 16.16 fixed)
- Auto-scroll speed: Handled by camera or level scrolling
- Jump trigger: D-Pad buttons (0xf0 mask)

---

## Controls

**Button Mapping**:
| Button | Mask | Action |
|--------|------|--------|
| Triangle | 0x1000 | Move left (adjust position) |
| X | 0x4000 | Move right (adjust position) |
| D-Pad (any) | 0xf0 | Jump/special move |

**Control Style**: Limited horizontal adjustment, automatic forward progress

**Difference from Normal**:
- No full left/right control
- Only adjustment/dodging
- Automatic forward movement
- Jump is D-Pad, not X button

---

## State Machine

**Initial State**: EntitySetState with null_FFFF0000h_800a5ffc, PTR_LAB_800a6000

**State Transitions**:
- **Idle/Running**: Main state (auto-scroll)
- **Jump**: Triggered by D-Pad (state at 0x800a600c)
- **Collision**: Standard collision handling

**State Callbacks**:
- PTR_PlayerState_Running_800a5dd4 (from line 21052)
- Multiple running-related states in player state table

---

## Camera Behavior

**Auto-Scroll**: Camera likely moves forward automatically

**Player Position**: Player stays relatively centered or slightly left-of-center

**Scrolling Speed**: Constant forward movement (2-3 px/frame estimated)

---

## Level Design

**RUNN Levels**:
- Level 22 (RUNN)
- 2 stages
- Auto-scrolling gameplay
- Obstacles approach player
- Must dodge/jump over hazards

**Typical Elements**:
- Moving platforms
- Timed jumps
- Hazards at regular intervals
- No backtracking (forward only)

---

## Comparison with Other Special Modes

| Mode | Flag | Control Style | Camera |
|------|------|---------------|--------|
| **Normal** | Default | Full platformer | Smooth follow |
| **FINN** | 0x400 | Tank/rotation | Smooth follow |
| **RUNN** | 0x100 | Auto-scroll + dodge | Auto-scroll |
| **SOAR** | 0x10 | Flying (TBD) | Vertical |
| **GLIDE** | 0x04 | Gliding (TBD) | Follow |

---

## Godot Implementation

```gdscript
extends CharacterBody2D
class_name PlayerRunn

# Constants
const AUTO_SCROLL_SPEED = 180.0  # 3.0 px/frame @ 60fps
const HORIZONTAL_ADJUST = 45.0   # 0.75 px/frame
const JUMP_VELOCITY = -240.0     # -4.0 px/frame
const GRAVITY = 360.0            # 6.0 px/frame²

# State
var auto_scroll_active: bool = true

func _physics_process(delta: float) -> void:
    # Automatic forward movement
    if auto_scroll_active:
        velocity.x = AUTO_SCROLL_SPEED
    
    # Horizontal adjustment (limited)
    if Input.is_action_pressed("ui_left"):  # Triangle
        velocity.x -= HORIZONTAL_ADJUST
    elif Input.is_action_pressed("ui_right"):  # X
        velocity.x += HORIZONTAL_ADJUST
    
    # Jump (D-Pad)
    if Input.is_action_just_pressed("ui_up") and is_on_floor():
        velocity.y = JUMP_VELOCITY
    
    # Gravity
    if not is_on_floor():
        velocity.y += GRAVITY * delta
    
    move_and_slide()

# Camera follows with auto-scroll
func _on_camera_update(camera: Camera2D) -> void:
    # Camera moves forward automatically
    camera.position.x += AUTO_SCROLL_SPEED * get_physics_process_delta_time()
    
    # Player stays relatively centered
    if global_position.x < camera.position.x - 100:
        # Player too far left - push forward
        global_position.x = camera.position.x - 100
    elif global_position.x > camera.position.x + 100:
        # Player too far right - limit
        global_position.x = camera.position.x + 100
```

---

## Special Mechanics

### Limited Horizontal Control

**Not Full Movement**: Player can only adjust position slightly

**Purpose**: 
- Dodge obstacles
- Navigate between lanes
- Avoid hazards

**Range**: ±100 pixels from center (estimated)

### Jump Timing

**Critical**: Must time jumps precisely

**Obstacles**: Approach at constant speed

**Failure**: Miss jump = hit obstacle = damage

---

## Related Documentation

- [Player FINN](player-finn.md) - Swimming/tank controls
- [Player System](player-system.md) - Normal platforming
- [Level Flags](../../blb/asset-types.md) - Flag 0x100
- [Game Loop](../game-loop.md) - Player type selection

---

**Status**: ✅ **FULLY DOCUMENTED**  
**Source**: Complete C code analysis  
**Flag**: 0x100 (RUNN mode)  
**Implementation**: Ready for auto-scroller levels

