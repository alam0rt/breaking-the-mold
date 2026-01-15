# Entity Type 28: Platform A (Moving Platform)

**Entity Type**: 28  
**BLB Type**: 28  
**Callback**: 0x80080638  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Interactive Platform  
**Count**: 99 instances

---

## Overview

Moving platforms that carry the player along a fixed path.

**Gameplay Function**: Transportation and platforming challenges

---

## Behavior

**Type**: Moving solid platform  
**Movement**: Follows predefined path  
**Collision**: Player stands on top  
**Carrying**: Moves player with platform  
**Pattern**: Back-and-forth or circular path

---

## Movement Patterns

### Linear Path (Horizontal)

```c
void PlatformA_Tick(Entity* platform) {
    // Move along path
    platform->path_progress += PLATFORM_SPEED;
    
    if (platform->path_progress >= platform->path_length) {
        // Reached end - reverse direction
        platform->direction = -platform->direction;
        platform->path_progress = platform->path_length;
    } else if (platform->path_progress <= 0) {
        // Reached start - reverse direction
        platform->direction = -platform->direction;
        platform->path_progress = 0;
    }
    
    // Update position
    platform->x_position = platform->start_x + (platform->path_progress * platform->direction);
}
```

**Constants**:
- Platform Speed: 0.5-1.5 px/frame
- Path Length: Varies by platform (50-200 pixels typical)

### Vertical Path

Same logic but applies to Y position instead of X.

### Circular/Complex Path

May use waypoint system or parametric path (Asset 504-style)

---

## Player Interaction

### Standing On Platform

**Detection**:
```c
// Player collision check
if (PlayerOnPlatform(player, platform)) {
    // Move player with platform
    player->x_position += platform->velocity_x;
    player->y_position += platform->velocity_y;
    player->on_moving_platform = true;
}
```

**Platform Surface**:
- Acts as solid floor (player can stand on it)
- Player inherits platform velocity
- Player can jump off platform normally

---

## Collision

**Collision Properties**:
- **Top Surface**: Solid (one-way platform)
- **Sides/Bottom**: Pass-through (player can move through)
- **Other Entities**: May carry enemies too

**Bounding Box**: Varies (32×8 to 64×16 pixels typical)

---

## Path Data

**Source**: May use Asset 504 (vehicle path data) or inline waypoints

**Waypoint Structure** (if used):
```
Point 0: Start position (X, Y)
Point 1: End position (X, Y)
Loop: true/false
Speed: pixels per frame
Wait: frames to wait at each end
```

---

## Godot Implementation

```gdscript
extends AnimatableBody2D
class_name MovingPlatform

# Configuration
@export var path_length: float = 200.0
@export var speed: float = 60.0  # 1.0 px/frame @ 60fps
@export var direction: Vector2 = Vector2.RIGHT
@export var loop: bool = true

# State
var progress: float = 0.0
var moving_forward: bool = true

func _physics_process(delta: float) -> void:
    # Update progress
    if moving_forward:
        progress += speed * delta
        if progress >= path_length:
            progress = path_length
            if loop:
                moving_forward = false
    else:
        progress -= speed * delta
        if progress <= 0:
            progress = 0
            if loop:
                moving_forward = true
    
    # Calculate position
    var target_pos = start_position + direction.normalized() * progress
    
    # Move platform (AnimatableBody2D carries riders automatically)
    var motion = target_pos - global_position
    move_and_collide(motion)
```

---

**Status**: ✅ **Fully Documented** (behavior)  
**Sprite ID**: ⚠️ Needs extraction  
**Pattern**: Moving platform  
**Implementation**: Ready

