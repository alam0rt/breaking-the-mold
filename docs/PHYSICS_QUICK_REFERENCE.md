# Physics Quick Reference

**For**: Game developers implementing Skullmonkeys mechanics  
**Source**: Extracted from PAL decompilation (SLES_010.90.c)  
**Updated**: January 14, 2026

---

## Player Movement (Copy-Paste Ready)

### C99 Constants

```c
// Horizontal movement (16.16 fixed-point)
#define WALK_SPEED_NORMAL   0x20000    // 2.0 px/frame
#define WALK_SPEED_FAST     0x30000    // 3.0 px/frame
#define SPEED_MODIFIER      0x8000     // +0.5 px/frame boost

// Vertical movement
#define JUMP_VELOCITY       0xFFFDC000 // -2.25 px/frame (upward)
#define JUMP_APEX_VELOCITY  0xFFD8     // -0.625 px/frame (peak)
#define GRAVITY             0xFFFA0000 // -6.0 px/frame² (downward)
#define LANDING_CUSHION     0xFFFFEE00 // -0.07 px/frame (soft land)

// Timers
#define LANDING_TIMER       5          // 5 frames
#define JUMP_PARAM          0x0C       // 12 (apex detection)
```

### Godot Constants

```gdscript
# Movement (pixels per second)
const WALK_SPEED_NORMAL = 2.0 * 60   # 120 px/sec
const WALK_SPEED_FAST = 3.0 * 60     # 180 px/sec
const SPEED_BOOST = 0.5 * 60         # 30 px/sec

# Vertical
const JUMP_VELOCITY = -2.25 * 60     # -135 px/sec
const GRAVITY = 6.0 * 60 * 60        # 21,600 px/sec²
const MAX_FALL_SPEED = 8.0 * 60      # 480 px/sec

# Timers
const LANDING_FRAMES = 5
const JUMP_APEX_PARAM = 12
```

---

## Camera System (Copy-Paste Ready)

### Camera State Offsets

```c
// GameState offsets
#define CAMERA_X            0x44  // s16
#define CAMERA_Y            0x46  // s16
#define CAMERA_VEL_X        0x4C  // s32 (16.16 fixed)
#define CAMERA_VEL_Y        0x50  // s32 (16.16 fixed)
#define CAMERA_SUBPIXEL_X   0x5C  // u16
#define CAMERA_SUBPIXEL_Y   0x5E  // u16
#define SCROLL_LEFT_FLAG    0x58  // u8
#define SCROLL_RIGHT_FLAG   0x59  // u8
#define SCROLL_UP_FLAG      0x5A  // u8
#define SCROLL_DOWN_FLAG    0x5B  // u8
```

### Camera Constants

```c
#define CAMERA_ACCEL_FULL   0x10000  // 1.0 acceleration
#define CAMERA_ACCEL_HALF   0x8000   // 0.5 acceleration
#define CAMERA_MAX_DISTANCE 0x8F     // 143 (table size)
```

### Lookup Tables (ROM Addresses)

```c
// RAM addresses (subtract 0x80010000 for ROM offset)
#define CAMERA_VERT_TABLE   0x8009b074  // 576 bytes
#define CAMERA_HORIZ_TABLE  0x8009b104  // 576 bytes
#define CAMERA_DIAG_TABLE   0x8009b0bc  // 576 bytes
```

---

## Projectile System (Copy-Paste Ready)

### Constants

```c
// Projectile spawning
#define PROJECTILE_SPRITE_ID  0x168254b5  // Sprite hash
#define PROJECTILE_SIZE       0x114       // 276 bytes
#define ANGLE_BASE            0xC00       // 3072 (upward)
#define TRIG_SHIFT            12          // >> 12 for fixed-point
#define VELOCITY_SCALE        10          // << 10 for final velocity

// Ammo
#define MAX_GREEN_BULLETS     3           // Default ammo capacity
```

### Spawn Function

```c
void SpawnProjectile(Entity* player, uint angle, int speed) {
    // Adjust angle
    int adj_angle = 0xC00 - (angle & 0xFFFF);
    
    // Calculate velocity
    int vel_y = csin(adj_angle) * speed >> 12;
    int vel_x = ccos(adj_angle) * speed >> 12;
    
    // Allocate and initialize
    Entity* proj = AllocateFromHeap(heap, 0x114, 1, 0);
    InitEntity_168254b5(proj, player->source_id,
                        player->x + vel_x,
                        player->y - vel_y,
                        vel_x << 10,
                        -vel_y << 10);
    AddEntityToRenderList(game_state, proj);
}
```

---

## Collision Attributes (Quick Reference)

### Tile Attribute Values

```c
#define TILE_EMPTY          0x00  // Passable
#define TILE_SOLID          0x02  // Standard solid block
#define TILE_SOLID_MIN      0x01  // Start of solid range
#define TILE_SOLID_MAX      0x3B  // End of solid range (59)
#define TILE_CHECKPOINT     0x53  // Save point
#define TILE_SPAWN_ZONE     0x65  // Entity spawn area

// Trigger types (from CheckTriggerZoneCollision)
#define TRIGGER_CHECKPOINT  0x00       // Checkpoint marker
#define TRIGGER_EXIT_START  0x02       // Level exit (6 types: 0x02-0x07)
#define TRIGGER_COLLECT_START 0x32     // Collectible zones (10 types: 0x32-0x3B)
```

### Floor Check Logic

```c
bool is_solid_floor(uint8_t attr) {
    return (attr != 0x00 && attr <= 0x3B);
}

bool is_trigger(uint8_t attr) {
    return (attr > 0x3B);
}
```

---

## Input Masks (Quick Reference)

```c
// PSX controller button masks
#define BTN_UP      0x0010
#define BTN_RIGHT   0x0020
#define BTN_DOWN    0x0040
#define BTN_LEFT    0x0080
#define BTN_CIRCLE  0x2000
#define BTN_X       0x4000  // Jump
#define BTN_SQUARE  0x8000  // Attack

// Read from: entity[0x100] → InputState structure
```

---

## Sound Effects (Quick Reference)

```c
#define SFX_JUMP    0x248E52      // Jump sound
#define SFX_PICKUP  0x7003474C    // Item collection
```

---

## Fixed-Point Conversion (Quick Reference)

### C Macros

```c
// 16.16 fixed-point conversions
#define FIXED_TO_FLOAT(x)   ((float)(x) / 65536.0f)
#define FLOAT_TO_FIXED(x)   ((int32_t)((x) * 65536.0f))

// Pixels per frame to pixels per second (60fps)
#define PX_FRAME_TO_SEC(x)  ((x) * 60.0f)

// Examples:
// WALK_SPEED = FIXED_TO_FLOAT(0x20000) = 2.0 px/frame
// WALK_SPEED_SEC = PX_FRAME_TO_SEC(2.0) = 120.0 px/sec
```

### Godot Helpers

```gdscript
# Convert PSX 16.16 fixed to Godot velocity (px/sec)
static func psx_velocity(psx_fixed: int) -> float:
    return float(psx_fixed) / 65536.0 * 60.0

# Convert PSX acceleration to Godot (px/sec²)
static func psx_accel(psx_fixed: int) -> float:
    return float(psx_fixed) / 65536.0 * 60.0 * 60.0

# Usage:
# const WALK_SPEED = psx_velocity(0x20000)  # 120.0
# const GRAVITY = psx_accel(0xFFFA0000)     # -21600.0
```

---

## Entity Offsets (Quick Reference)

### Position & Movement

```c
#define ENT_X_POSITION      0x68   // s16
#define ENT_Y_POSITION      0x6A   // s16
#define ENT_X_FRACTION      0x6C   // u16
#define ENT_Y_FRACTION      0x6E   // u16
#define ENT_FACING_LEFT     0x74   // u8 (0=right, 1=left)
#define ENT_MOVING_UP       0x75   // u8 (0=down, 1=up)
#define ENT_VELOCITY_X      0x104  // s32 (16.16)
#define ENT_VELOCITY_Y      0x108  // s32 (16.16)
#define ENT_GRAVITY         0x110  // s32 (16.16)
#define ENT_PUSH_X          0x160  // s16 (player only)
#define ENT_PUSH_Y          0x162  // s16 (player only)
```

### State Machine

```c
#define ENT_STATE_HIGH      0x00   // u32
#define ENT_CALLBACK_MAIN   0x04   // ptr
#define ENT_CALLBACK_SEC    0x0C   // ptr
#define ENT_STATE_INDEX     0xA2   // s16
#define ENT_STATE_CALLBACK  0xA4   // ptr
#define ENT_NEXT_STATE      0x98   // ptr
```

---

## Complete Documentation Index

### Physics & Movement
- [Player Physics](systems/player/player-physics.md) - Complete player movement
- [Physics Constants Reference](reference/physics-constants.md) - All constants
- [Camera System](systems/camera.md) - Smooth scrolling algorithm
- [Projectiles](systems/projectiles.md) - Weapon system

### Game Systems
- [Collision System](systems/collision.md) - Tile collision
- [Entities](systems/entities.md) - Entity system overview
- [Player System](systems/player/player-system.md) - Player state machine

### BLB Format
- [BLB Overview](blb/README.md) - File format overview
- [Asset Types](blb/asset-types.md) - Complete asset reference
- [Level Metadata](blb/level-metadata.md) - Level entry format

---

**This document is intended as a quick reference for implementation. See individual system documents for detailed explanations and algorithms.**

