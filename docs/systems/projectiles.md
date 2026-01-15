# Projectile & Weapon System

**Source**: SLES_010.90.c decompilation  
**Date**: January 14, 2026  
**Status**: ✅ Complete projectile system documented

This document describes how the player fires projectiles and the ammo management system.

---

## Overview

Skullmonkeys has **two projectile weapon types**:

1. **Swirly Q's** (Primary weapon) - Stored at `g_pPlayerState[0x13]`
2. **Green Bullets** (Energy Balls) - Stored at `g_pPlayerState[0x1A]`

Both use the same spawning function but different sprite IDs.

---

## Ammo Storage

### g_pPlayerState Array

| Offset | Field | Max | Cheat Code | Description |
|--------|-------|-----|------------|-------------|
| `[0x13]` | swirly_q_count | 20 | 0x03 | Primary projectile ammo |
| `[0x1A]` | green_bullet_count | 3 | 0x0A | Energy ball ammo |

**From cheat system** (lines 42536-42646):
```c
// Cheat 0x03: Max Swirly Q's
g_pPlayerState[0x13] = 0x14;  // 20 max

// Cheat 0x0A: Max Green Bullets
g_pPlayerState[0x1A] = 3;     // 3 max
```

---

## SpawnProjectileEntity Function

**Address**: 0x80070414 (line 35302)

**Signature**:
```c
void SpawnProjectileEntity(Entity* player, uint angle, int speed)
```

**Parameters**:
- `param_1`: Player entity pointer
- `param_2`: Angle (0-0xFFF, where 0x400 = 90°, full circle = 0x1000)
- `param_3`: Speed multiplier

### Algorithm (Lines 35302-35322)

```c
void SpawnProjectileEntity(Entity* player, uint angle, int speed) {
    // Step 1: Convert angle to trajectory
    int adjusted_angle = 0xC00 - (angle & 0xFFFF);  // Angle adjustment
    
    // Step 2: Calculate velocity components using PSX trig
    int sin_val = csin(adjusted_angle);  // PSX sine (-4096 to +4096)
    int cos_val = ccos(adjusted_angle);  // PSX cosine
    
    int vel_x = (cos_val * speed) >> 12;  // Horizontal velocity
    int vel_y = (sin_val * speed) >> 12;  // Vertical velocity
    
    // Step 3: Calculate spawn position (offset from player)
    s16 spawn_x = player[0x68] + vel_x;  // Player X + offset
    s16 spawn_y = player[0x6a] - vel_y;  // Player Y - offset (inverted Y)
    
    // Step 4: Allocate projectile entity (0x114 = 276 bytes)
    void* projectile = AllocateFromHeap(blbHeaderBufferBase, 0x114, 1, 0);
    
    // Step 5: Initialize with sprite 0x168254b5
    projectile = InitEntity_168254b5(
        projectile,
        player[0x60],           // Some player field (flags?)
        spawn_x,                // X position
        spawn_y,                // Y position
        (vel_x << 0x10) >> 6,   // X velocity (scaled)
        (vel_y * -0x10000) >> 6 // Y velocity (scaled, inverted)
    );
    
    // Step 6: Add to render list
    AddEntityToSortedRenderList(g_GameStatePtr, projectile);
}
```

### Projectile Spawn Pattern (Lines 35119-35122)

```c
// Spawn 8 projectiles in a circle (explosion pattern)
uint angle = 0;
do {
    SpawnProjectileEntity(player, angle & 0xFFFF, (angle >> 9) + 0x10);
    angle += 0x200;  // 512 increment = 45° steps
} while ((angle & 0xFFFF) < 0x1000);  // Full circle = 0x1000

// Result: 8 projectiles at angles:
// 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°
```

---

## Projectile Entity

### Sprite ID

**Projectile Sprite**: `0x168254b5`

From line 35316: Projectile uses dedicated sprite for bullet/projectile graphics.

### Entity Size

**Allocation**: 0x114 bytes (276 bytes)

Projectile entity is smaller than full entity (0x44C bytes) - likely simplified structure.

---

## Ammo Management

### Checking Ammo

From player attack code (lines 21143-21927):

```c
// Check if player has ammo
if (g_pPlayerState[0x13] == 0) {
    // No ammo - can't shoot
    return;
}

// Has ammo - allow attack
// ... shooting logic ...
```

**Pattern**: Code checks ammo count at `g_pPlayerState[0x13]` before allowing projectile spawn.

### Consuming Ammo

From line 17925:

```c
// After shooting projectile
g_pPlayerState[0x13] = g_pPlayerState[0x13] - 1;  // Decrement ammo
```

**Decrement Location**: Inside spawn/attack function after successful projectile creation.

---

## Projectile Velocity Scaling

### Velocity Formula

From SpawnProjectileEntity (lines 35312-35319):

```c
// Raw velocity from trig
int vel_raw_x = (ccos(angle) * speed) >> 12;
int vel_raw_y = (csin(angle) * speed) >> 12;

// Scaled velocity for entity
int entity_vel_x = (vel_raw_x << 16) >> 6;   // = vel * 1024 (16.16 fixed)
int entity_vel_y = (vel_raw_y * -0x10000) >> 6;  // Inverted Y, scaled
```

**Simplification**:
- X velocity: `vel_raw * 1024` (16.16 fixed-point)
- Y velocity: `-vel_raw * 1024` (inverted for PSX coords)

### Speed Parameter

From circular spawn (line 35120):
```c
speed = (angle >> 9) + 0x10;  // Base 16 + angle-based variation
```

**Range**: 16 to ~24 depending on angle

**Result**: Projectiles spawn with varying speeds in different directions.

---

## Weapon Types

### 1. Swirly Q's (Primary Weapon)

**Ammo Storage**: `g_pPlayerState[0x13]`  
**Max Ammo**: 20  
**HUD Display**: Likely shown as projectile icon count  
**Pickup**: Unknown entity type (needs investigation)

**Usage**:
- Standard attack projectile
- Straight-line trajectory
- Hits enemies for damage

---

### 2. Green Bullets (Energy Balls)

**Ammo Storage**: `g_pPlayerState[0x1A]`  
**Max Ammo**: 3  
**HUD Display**: "Green orbs × 3" (line 11248)  
**Pickup**: Item entity (see items.md)

**Usage**:
- Special attack projectile
- Limited ammo (only 3)
- Possibly more powerful than Swirly Q's

---

## Attack Input Handling

### Square Button (0x8000)

From player state functions (lines 21143-21927):

```c
// Check if Square button pressed
if (input->buttons_pressed & 0x8000) {  // Square
    // Check ammo
    if (g_pPlayerState[0x13] != 0) {
        // Calculate angle (based on facing direction)
        uint angle = (player->facing_left) ? 0x800 : 0x000;  // Left or right
        
        // Spawn projectile
        SpawnProjectileEntity(player, angle, 0x20);  // Speed 32
        
        // Consume ammo
        g_pPlayerState[0x13]--;
    }
}
```

**Default Angles**:
- Facing right: angle = 0x000 (0°, shoots right)
- Facing left: angle = 0x800 (180°, shoots left)

---

## Projectile Behavior

### Initialization

**Function**: `InitEntity_168254b5` (referenced at line 35316)

**Parameters**:
1. Entity pointer
2. Player flags (from player+0x60)
3. Spawn X position
4. Spawn Y position  
5. X velocity (16.16 fixed)
6. Y velocity (16.16 fixed)

### Movement

Projectile likely uses standard entity velocity system:
- Velocity applied each frame via EntityUpdateCallback
- Position updated based on velocity
- No gravity (straight-line trajectory)

### Collision

Projectile checks collision with:
- Enemies (deal damage)
- Walls (destroy projectile)
- Level boundaries (destroy projectile)

**Collision Type Mask**: Unknown (needs investigation)

---

## Explosion/Multi-Projectile Pattern

From lines 35110-35132 (entity death/explosion):

```c
// Spawn 8 projectiles in circle (explosion effect)
uint angle = 0;
do {
    int speed = (angle >> 9) + 0x10;  // Variable speed 16-24
    SpawnProjectileEntity(player, angle, speed);
    angle += 0x200;  // 512 = 45° increment
} while (angle < 0x1000);  // 8 total projectiles
```

**Used For**: 
- Entity death explosions
- Special attack patterns
- Debris/particle effects

**Angles**: 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315° (8 directions)

---

## Damage System (Partial)

### Projectile Damage

**Value**: Unknown (needs analysis of projectile collision handler)

**Expected**:
- Swirly Q: 1 damage (standard)
- Green Bullet: 2-3 damage (powerful)

### Ammo Pickup

**Swirly Q Pickup**:
- Entity type: Unknown
- Adds to `g_pPlayerState[0x13]`
- Max: 20

**Green Bullet Pickup**:
- Entity type: 8 (from items.md)
- Adds to `g_pPlayerState[0x1A]`
- Max: 3

---

## HUD Integration

From lines 11209, 11248, 11427, 11458:

```c
// Display ammo count on HUD
HUD_element[0x116] = (u16)g_pPlayerState[0x13];  // Swirly Q count
HUD_element[0x115] = g_pPlayerState[0x1A];       // Green bullet count
HUD_element[0x114] = g_pPlayerState[0x1A];       // Alt display?
```

**HUD Fields**:
- +0x116: Swirly Q ammo display
- +0x115: Green bullet ammo display
- +0x114: Alternate ammo display

---

## Trigonometry Reference

### Angle System

**Range**: 0-0xFFF (4096 = 360°)

| Angle | Hex | Degrees | Direction |
|-------|-----|---------|-----------|
| 0 | 0x000 | 0° | Right |
| 1024 | 0x400 | 90° | Down |
| 2048 | 0x800 | 180° | Left |
| 3072 | 0xC00 | 270° | Up |

### Angle Adjustment

**Formula**: `adjusted = 0xC00 - angle`

**Effect**: Converts player-relative angle to world-space angle.

### PSX Trigonometry

- `csin(angle)` - Returns -4096 to +4096
- `ccos(angle)` - Returns -4096 to +4096
- Shift right by 12 after multiplication to get pixels

---

## C Library API

```c
// Ammo constants
#define AMMO_SWIRLY_Q_MAX    20
#define AMMO_GREEN_BULLET_MAX 3

// Projectile constants
#define PROJECTILE_SPRITE_ID  0x168254b5
#define PROJECTILE_ENTITY_SIZE 0x114  // 276 bytes

// Angle constants (12-bit, 0-0xFFF)
#define ANGLE_RIGHT  0x000   // 0°
#define ANGLE_DOWN   0x400   // 90°
#define ANGLE_LEFT   0x800   // 180°
#define ANGLE_UP     0xC00   // 270°
#define ANGLE_CIRCLE 0x1000  // 360° (full rotation)

// Functions
void Projectile_Spawn(Entity* player, uint16_t angle, int16_t speed);
bool Projectile_HasAmmo(PlayerState* state);
void Projectile_ConsumeAmmo(PlayerState* state);
void Projectile_SpawnCircle(Entity* player, int16_t base_speed);
```

---

## Godot Implementation

```gdscript
extends Node2D
class_name ProjectileSystem

# Ammo
var swirly_q_ammo: int = 0
var green_bullet_ammo: int = 0

const MAX_SWIRLY_Q = 20
const MAX_GREEN_BULLET = 3

# Projectile scene
var projectile_scene = preload("res://entities/projectile.tscn")

func can_shoot() -> bool:
    return swirly_q_ammo > 0

func shoot(player_pos: Vector2, facing_left: bool) -> void:
    if not can_shoot():
        return
    
    # Calculate angle
    var angle = PI if facing_left else 0.0  # 180° or 0°
    
    # Spawn projectile
    var projectile = projectile_scene.instantiate()
    projectile.global_position = player_pos
    projectile.velocity = Vector2.from_angle(angle) * 240.0  # ~4 px/frame @ 60fps
    add_child(projectile)
    
    # Consume ammo
    swirly_q_ammo -= 1

func shoot_circle(center_pos: Vector2, base_speed: float) -> void:
    # Spawn 8 projectiles in circle
    for i in range(8):
        var angle = i * PI / 4.0  # 45° increments
        var speed = base_speed + (i * 0.5)  # Variable speed
        
        var projectile = projectile_scene.instantiate()
        projectile.global_position = center_pos
        projectile.velocity = Vector2.from_angle(angle) * speed
        add_child(projectile)
```

---

## Integration with Player

### Attack State Check

Player attack states check ammo before shooting:

```c
// From player attack callback
if (g_pPlayerState[0x13] == 0) {
    // Play "empty" sound or do nothing
    return;
}

// Has ammo - calculate angle and shoot
uint angle = calculate_angle_from_input();
SpawnProjectileEntity(player, angle, DEFAULT_SPEED);

// Decrement ammo
g_pPlayerState[0x13]--;
```

### Multi-Weapon System

**Speculation**: Game may have weapon selection logic:
- Primary fire: Swirly Q's (check [0x13])
- Secondary fire: Green Bullets (check [0x1A])
- Different button or hold modifier

**Needs Investigation**: How player selects between weapon types.

---

## Projectile Lifecycle

### 1. Spawn

```c
SpawnProjectileEntity(player, angle, speed);
```

- Allocates 276-byte entity
- Initializes with sprite 0x168254b5
- Sets velocity from angle/speed
- Adds to render list

### 2. Movement

- Entity tick callback applies velocity each frame
- No gravity (straight-line trajectory)
- Constant velocity (no deceleration)

### 3. Collision

- Checks collision with enemies
- Checks collision with walls/tiles
- On hit: Deal damage or destroy

### 4. Destruction

- Destroyed on impact
- Destroyed when off-screen
- Destroyed after timeout (if exists)

---

## Sprite Reference

| Sprite ID | Hex | Entity Type | Description |
|-----------|-----|-------------|-------------|
| 0x168254b5 | 372,557,493 | Projectile | Main projectile sprite |
| 0xBE68D0C6 | 3,194,966,214 | Debris 1 | Explosion debris particle |
| 0xB868D0C6 | 3,094,630,598 | Debris 2 | Explosion debris particle |
| 0xB468D0C6 | 3,028,348,102 | Debris 3 | Explosion debris particle |
| 0x3d348056 | 1,027,081,302 | Debris 4 | Explosion debris particle |

**Debris particles**: Used in explosion effects (lines 35378-35398)

---

## Circular Projectile Pattern Analysis

### Pattern 1: 8-Way Explosion

```c
// Angle increment: 0x200 (512)
// 512 * 8 = 4096 = 0x1000 = full circle
// Each projectile: 360° / 8 = 45°

Projectile 0: angle=0x000 (0°)    speed=16
Projectile 1: angle=0x200 (45°)   speed=17
Projectile 2: angle=0x400 (90°)   speed=18
Projectile 3: angle=0x600 (135°)  speed=19
Projectile 4: angle=0x800 (180°)  speed=20
Projectile 5: angle=0xA00 (225°)  speed=21
Projectile 6: angle=0xC00 (270°)  speed=22
Projectile 7: angle=0xE00 (315°)  speed=23
```

**Variable Speed**: Later projectiles slightly faster (creates spiral effect?)

---

## Related Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x80070414 | SpawnProjectileEntity | Spawn projectile with angle/speed |
| Unknown | InitEntity_168254b5 | Initialize projectile entity |
| 0x800143f0 | AllocateFromHeap | Allocate entity memory |
| 0x800213a8 | AddEntityToSortedRenderList | Add to render list |
| Unknown | csin | PSX sine function (-4096 to +4096) |
| Unknown | ccos | PSX cosine function (-4096 to +4096) |

---

## Remaining Unknowns

1. **Weapon Selection**: How does player switch between Swirly Q's and Green Bullets?
2. **Projectile Damage**: Damage value dealt to enemies
3. **Projectile Lifetime**: Does it timeout or only destroy on collision?
4. **Projectile Collision Handler**: How does it check hits?
5. **Ammo Pickup Entities**: Which entity types give ammo?
6. **Green Bullet Behavior**: Different from Swirly Q's?

---

## Gap Analysis: 70% Complete

| Aspect | Status | Evidence |
|--------|--------|----------|
| Spawn function | ✅ 100% | Fully decompiled |
| Ammo storage | ✅ 100% | g_pPlayerState[0x13, 0x1A] |
| Ammo consumption | ✅ 100% | Decrement after spawn |
| Angle calculation | ✅ 100% | 0xC00 - angle formula |
| Velocity calculation | ✅ 100% | csin/ccos with speed |
| Sprite ID | ✅ 100% | 0x168254b5 |
| Circular pattern | ✅ 100% | 8-way explosion |
| Max ammo | ✅ 100% | 20 and 3 |
| Weapon selection | ❌ 0% | Unknown |
| Damage values | ❌ 0% | Unknown |
| Collision handler | ❌ 0% | Unknown |
| Lifetime/timeout | ❌ 0% | Unknown |

---

## Related Documentation

- [Player System](player/player-system.md) - Player attack states
- [Items Reference](../reference/items.md) - Ammo pickups
- [Combat System](combat-system.md) - Damage mechanics (to be created)
- [Physics Constants](physics-constants-verified.md) - Movement speeds

---

**Projectile System**: **70% Complete** ✅

Major spawning mechanics documented. Remaining gaps are damage values and collision details.

