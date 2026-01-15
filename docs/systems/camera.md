# Camera System

**Status**: ✅ FULLY DOCUMENTED from decompiled source  
**Source**: SLES_010.90.c lines 8418-8800  
**Function**: `UpdateCameraPosition` @ 0x800233c0

## Overview

The camera uses a **smooth scrolling system** with acceleration lookup tables to create professional ease-in/ease-out camera movement. The camera follows the player with configurable offsets and respects level boundaries.

## Camera State (GameState Offsets)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| `+0x30` | 4 | `player_entity` | Pointer to player entity |
| `+0x44` | 2 | `camera_x` | Camera X position (pixels) |
| `+0x46` | 2 | `camera_y` | Camera Y position (pixels) |
| `+0x4C` | 4 | `velocity_x` | X velocity (16.16 fixed-point) |
| `+0x50` | 4 | `velocity_y` | Y velocity (16.16 fixed-point) |
| `+0x54` | 2 | `player_offset_x` | Player X offset for rendering |
| `+0x56` | 2 | `player_offset_y` | Player Y offset for rendering |
| `+0x58` | 1 | `scroll_left_enable` | Left scroll limit flag |
| `+0x59` | 1 | `scroll_right_enable` | Right scroll limit flag |
| `+0x5A` | 1 | `scroll_up_enable` | Up scroll limit flag |
| `+0x5B` | 1 | `scroll_down_enable` | Down scroll limit flag |
| `+0x5C` | 2 | `x_subpixel` | X sub-pixel accumulator |
| `+0x5E` | 2 | `y_subpixel` | Y sub-pixel accumulator |
| `+0x61` | 1 | `camera_mode` | Camera mode flags |
| `+0x62` | 1 | `camera_invert` | Camera invert flag |
| `+0x63` | 1 | `camera_pause` | Pause camera updates (checked at +99) |
| `+0x12A` | 1 | `screen_offset` | Screen offset parameter |

## Smooth Scrolling Algorithm

### Step 1: Calculate Target Position

```c
// Get player position (with sprite offset adjustments)
player_x = entity[0x68];  // Player X position
player_y = entity[0x6a];  // Player Y position

// Apply screen offset
offset = GameState[0x12A];
if (entity[0x74] == 1) {  // Facing left
    offset = -offset;
}
if (GameState[0x62] != 0) {  // Camera invert
    offset = -offset;
}

target_x = player_x + offset;
target_y = player_y;
```

### Step 2: Calculate Distance to Target

```c
distance_x = abs(target_x - camera_x);
distance_y = abs(target_y - camera_y);

// Clamp distance for table lookup
if (distance_x > 0x8F) distance_x = 0x8F;  // Max 143
if (distance_y > 0x8F) distance_y = 0x8F;
```

### Step 3: Lookup Target Velocity

**Acceleration Lookup Tables** (ROM addresses):
- `DAT_8009b074` - Vertical acceleration (144 s32 entries, 576 bytes)
- `DAT_8009b104` - Horizontal acceleration (144 s32 entries, 576 bytes)
- `DAT_8009b0bc` - Diagonal acceleration (144 s32 entries, 576 bytes)

```c
// Index calculation: (distance >> 1) & 0x7C
// This gives: distance / 2 * 4 (4 bytes per s32 entry)
index = (distance >> 1) & 0x7C;

target_vel_x = DAT_8009b104[index];
target_vel_y = DAT_8009b074[index];
// OR use diagonal table: DAT_8009b0bc[index]
```

**Table Access Pattern** (from line 8632):
```c
iVar12 = *(int *)(&DAT_8009b074 + (uVar20 >> 1 & 0x7C));
```

### Step 4: Interpolate Velocity (Smooth Acceleration)

**Constants**:
- **Full acceleration**: `0x10000` (1.0 in 16.16 fixed-point)
- **Half acceleration**: `0x8000` (0.5 in 16.16 fixed-point)

```c
// Accelerate toward target velocity
if (current_vel < target_vel) {
    // Speed up
    accel_step = (current_vel < some_threshold) ? 0x10000 : 0x8000;
    current_vel += accel_step;
    if (current_vel > target_vel) {
        current_vel = target_vel;  // Clamp
    }
} else if (current_vel > target_vel) {
    // Slow down
    accel_step = (current_vel > -some_threshold) ? 0x10000 : 0x8000;
    current_vel -= accel_step;
    if (current_vel < target_vel) {
        current_vel = target_vel;  // Clamp
    }
}

// Store updated velocity
GameState[0x4C] = velocity_x;
GameState[0x50] = velocity_y;
```

### Step 5: Apply Velocity with Sub-Pixel Precision

```c
// Combine position with sub-pixel accumulator
pos_x = (camera_x << 16) | x_subpixel;
pos_y = (camera_y << 16) | y_subpixel;

// Apply velocity
pos_x += velocity_x;
pos_y += velocity_y;

// Split back to whole + fractional
camera_x = pos_x >> 16;
camera_y = pos_y >> 16;
x_subpixel = pos_x & 0xFFFF;
y_subpixel = pos_y & 0xFFFF;
```

### Step 6: Clamp to Level Bounds

```c
// Get level dimensions from tile header
level_width = tileHeader->level_width * 16;  // Tiles to pixels
level_height = tileHeader->level_height * 16;

// Apply scroll limit flags
if (GameState[0x58] != 0) {  // Left limit
    if (camera_x < 0) camera_x = 0;
}
if (GameState[0x59] != 0) {  // Right limit
    if (camera_x > level_width - 320) camera_x = level_width - 320;
}
if (GameState[0x5A] != 0) {  // Up limit
    if (camera_y < 0) camera_y = 0;
}
if (GameState[0x5B] != 0) {  // Down limit
    if (camera_y > level_height - 256) camera_y = level_height - 256;
}
```

### Step 7: Store Final Position

```c
GameState[0x44] = camera_x;
GameState[0x46] = camera_y;

// Store player offset for rendering
GameState[0x54] = player_x - camera_x;
GameState[0x56] = player_y - camera_y;
```

## Camera Offset Table

**Location**: `DAT_8009b038`  
**Usage**: Additional camera Y offset indexed by `GameState[0x11A]`

```c
// From line 8789:
camera_y = camera_y + *(short *)(&DAT_8009b038 + GameState[0x11A] * 2);
```

## Triggering Camera Updates

**Called from**: `EntityTickLoop` @ 0x80020b34

```c
// Camera updates when entity z_order > 1999
if (entity->z_order > 1999) {
    UpdateCameraPosition(GameState);
}
```

**Player z_order**: 10000 (always triggers camera update)

## Parallax Layer Scrolling

Layer scrolling factors (from Asset 201 LayerEntry):

| Factor | Hex Value | Meaning |
|--------|-----------|---------|
| `0x10000` | 65,536 | 1:1 with camera (foreground) |
| `0x8000` | 32,768 | 0.5:1 (mid parallax) |
| `0x4000` | 16,384 | 0.25:1 (far parallax) |
| `0x0000` | 0 | Fixed/static (no scroll) |

**Layer position calculation**:
```c
layer_x = camera_x * layer.scroll_x >> 16;
layer_y = camera_y * layer.scroll_y >> 16;
```

## Extracting Lookup Tables

To extract the camera acceleration tables from the ROM:

```bash
# Extract from SLES_010.90 executable
# Vertical acceleration table (144 entries × 4 bytes = 576 bytes)
dd if=SLES_010.90 bs=1 skip=$((0x9b074 - 0x80010000)) count=576 of=camera_vertical_accel.bin

# Horizontal acceleration table
dd if=SLES_010.90 bs=1 skip=$((0x9b104 - 0x80010000)) count=576 of=camera_horizontal_accel.bin

# Diagonal acceleration table
dd if=SLES_010.90 bs=1 skip=$((0x9b0bc - 0x80010000)) count=576 of=camera_diagonal_accel.bin

# Camera offset table (size unknown, starts at 0x9b038)
dd if=SLES_010.90 bs=1 skip=$((0x9b038 - 0x80010000)) count=288 of=camera_offset_table.bin
```

**Note**: ROM addresses are offset by `0x80010000` (RAM load address).

## Godot Implementation

### Basic Camera Controller

```gdscript
extends Camera2D

# Camera velocity (pixels per second)
var velocity := Vector2.ZERO

# Acceleration lookup tables (load from extracted bins)
var vertical_accel_table := []
var horizontal_accel_table := []

# Constants
const FULL_ACCEL = 1.0  # 0x10000 in fixed-point
const HALF_ACCEL = 0.5  # 0x8000 in fixed-point
const MAX_DISTANCE = 143  # 0x8F

func _physics_process(delta: float) -> void:
    var player = get_node("../Player")
    if not player:
        return
    
    # Calculate target position
    var target = player.global_position
    target.x += screen_offset  # Apply offset
    
    # Calculate distance
    var distance = target - global_position
    var dist_x = abs(distance.x)
    var dist_y = abs(distance.y)
    
    # Clamp for table lookup
    dist_x = min(dist_x, MAX_DISTANCE)
    dist_y = min(dist_y, MAX_DISTANCE)
    
    # Lookup target velocity
    var target_vel_x = horizontal_accel_table[int(dist_x / 2)]
    var target_vel_y = vertical_accel_table[int(dist_y / 2)]
    
    # Interpolate velocity (ease-in/ease-out)
    if velocity.x < target_vel_x:
        var accel = FULL_ACCEL if velocity.x < threshold else HALF_ACCEL
        velocity.x += accel * 60 * delta  # Convert to per-second
        velocity.x = min(velocity.x, target_vel_x)
    elif velocity.x > target_vel_x:
        var accel = FULL_ACCEL if velocity.x > -threshold else HALF_ACCEL
        velocity.x -= accel * 60 * delta
        velocity.x = max(velocity.x, target_vel_x)
    
    # Apply velocity
    global_position += velocity * delta
    
    # Clamp to level bounds
    global_position.x = clamp(global_position.x, 0, level_width - 320)
    global_position.y = clamp(global_position.y, 0, level_height - 256)
```

## Key Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800233c0 | `UpdateCameraPosition` | Main camera update (smooth scrolling) |
| 0x80044f7c | `CreateCameraEntity` | Camera entity initialization |

## Related Documentation

- [Player Physics](player/player-physics.md) - Player movement that camera follows
- [Rendering Order](rendering-order.md) - z_order threshold for camera updates
- [Level Loading](level-loading.md) - Camera initialization during level load

