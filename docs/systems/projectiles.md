---
title: "Projectile & Weapon System"
category: systems
tags: [systems, projectiles]
---

# Projectile & Weapon System

**Status**: ✅ 95% Complete (all functions named)  
**Last Updated**: January 19, 2026  
**Source**: SLES_010.90.c decompilation + Ghidra analysis

This document describes projectile entities and the weapon/powerup systems.

---

## Overview

> **CORRECTION (Jan 16, 2026)**: Field mappings have been corrected per official game manual.
>
> - `[0x13]` = **Green Bullets** (projectile ammo, max 20, Circle button)
> - `[0x1A]` = **Hamsters** (orbiting shield, max 3 hits, pickup only)
> - `[0x1b]` = **Swirly Q total** (secret ending counter, need 48+)

### Weapon Systems

| Weapon | Storage | Max | Button | Purpose |
|--------|---------|-----|--------|----------|
| Green Bullets | `[0x13]` | 20 | Circle | Standard ranged projectile |
| Phoenix Hand | `[0x14]` | 7 | L1 | Homing bird attack |
| Phart Head | `[0x15]` | 7 | L2 | Ghost clone scout |
| Universe Enema | `[0x16]` | 7 | R1 | Screen-wide destruction |
| Super Willie | `[0x1C]` | 7 | R2 | Auto-collect all items |

### Defensive Items

| Item | Storage | Max | Purpose |
|------|---------|-----|----------|
| Hamsters | `[0x1A]` | 3 | Orbiting shield (3 extra hits) |
| Halo | `[0x17]` bit 0 | 1 | Single-hit shield |

### Cheat Codes

```c
// Cheat 0x03: "Max Swirly Q's" (MISLABELED - actually Green Bullets!)
g_pPlayerState[0x13] = 0x14;  // 20 Green Bullets

// Cheat 0x0A: "Max Green Bullets" (MISLABELED - actually Hamsters!)
g_pPlayerState[0x1A] = 3;     // 3 Hamsters
```

**Note**: The cheat code labels in the game are backwards/confusing.

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
    AddEntityToSortedRenderList(g_pGameState, projectile);
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
    // No Green Bullets - can't shoot
    return;
}

// Has ammo - allow attack
// ... shooting logic ...
```

**Pattern**: Code checks Green Bullet count at `g_pPlayerState[0x13]` before allowing projectile spawn.

### Consuming Ammo

From line 17925:

```c
// After firing projectile
g_pPlayerState[0x13] = g_pPlayerState[0x13] - 1;  // Decrement Green Bullet count
```

**Decrement Location**: Inside projectile spawn function after successful creation.

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

## Collectibles vs Weapons

### Swirls (Bonus Room Unlock) - NOT A WEAPON

> **CORRECTION**: Swirls are collectibles, NOT weapon ammo!

**Storage**: `g_pPlayerState[0x13]`  
**Max Count**: 20  
**HUD Display**: Swirl icon with count  
**Purpose**: Collect 3 to unlock bonus room portal at stage exit

**Portal Spawn**: `SpawnSwirlPortalEntity` @ 0x8005ad54
- Decrements `[0x13]` when portal is spawned
- Uses sprite 0xca1b20cb or 0x3d348056

**NOT used for**:
- ❌ Attack projectiles
- ❌ Weapon ammo
- ❌ Enemy damage

---

### Green Bullets (Energy Balls) - ACTUAL WEAPON

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

> **Note**: The player's normal attack mechanics need further investigation.
> The code below may be from a cheat-enabled mode ("Fire Klaymen's Heads").

From player state functions (lines 21143-21927):

```c
// Check if Square button pressed
if (input->buttons_pressed & 0x8000) {  // Square
    // For Green Bullets:
    if (g_pPlayerState[0x1A] != 0) {
        // Spawn energy ball projectile
        SpawnProjectileEntity(player, angle, 0x20);
        g_pPlayerState[0x1A]--;  // Decrement green bullet count
    }
}
```
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
// Display counts on HUD
HUD_element[0x116] = (u16)g_pPlayerState[0x13];  // Green Bullet count
HUD_element[0x115] = g_pPlayerState[0x1A];       // Hamster count
HUD_element[0x114] = g_pPlayerState[0x1A];       // Alt display?
```

**HUD Fields**:
- +0x116: Green Bullet count (projectile ammo, max 20)
- +0x115: Hamster count (shield, max 3)
- +0x114: Alternate display

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
// Count constants
#define SWIRL_COUNT_MAX       20  // Bonus room collectible
#define GREEN_BULLET_MAX      3   // Actual projectile ammo

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
bool Projectile_HasGreenBullet(PlayerState* state);
void Projectile_ConsumeGreenBullet(PlayerState* state);
void Projectile_SpawnCircle(Entity* player, int16_t base_speed);
void SwirlPortal_Spawn(Entity* player);  // Uses swirl count
```

---

## Godot Implementation

```gdscript
extends Node2D
class_name ProjectileSystem

# Swirls (bonus room collectible - NOT ammo!)
var swirl_count: int = 0
var total_swirls: int = 0  # For secret ending

# Green Bullets (actual projectile ammo)
var green_bullet_count: int = 0

const MAX_SWIRLS = 20
const MAX_GREEN_BULLETS = 3

# Projectile scene
var projectile_scene = preload("res://entities/projectile.tscn")

func can_shoot() -> bool:
    return green_bullet_count > 0

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
    
    # Consume green bullet (actual ammo)
    green_bullet_count -= 1

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
g_pPlayerState[0x13]--;  // Green Bullets
```

### Weapon System (Per Official Manual)

**Confirmed Weapons:**
- **Circle**: Green Bullets (check [0x13], max 20)
- **L1**: Phoenix Hand (check [0x14], max 7) - homing bird
- **L2**: Phart Head (check [0x15], max 7) - ghost scout
- **R1**: Universe Enema (check [0x16], max 7) - screen clear
- **R2**: Super Willie (check [0x1C], max 7) - auto-collect items

**Defensive Item:**
- **Hamsters** ([0x1A], max 3) - orbiting shield, provides 3 extra hits

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

## Complete Projectile Function Table

**All projectile-related functions named in Ghidra (January 19, 2026)**:

### Core Projectile Functions

| Address | Function | Purpose |
|---------|----------|--------|
| 0x80070414 | `SpawnAngledProjectile` | Spawn projectile with angle/speed |
| 0x80040918 | `SpawnProjectileEntityDef` | Spawn projectile from entity def |
| 0x8004f704 | `ProjectileUpdateWithCleanup` | Update + destroy when offscreen |
| 0x8005120c | `ProjectileDeactivate` | Deactivate projectile |
| 0x8005138c | `ProjectileEntityTick` | Main projectile tick callback |
| 0x80051470 | `ProjectileMoveHorizontal` | Horizontal movement only |
| 0x80052554 | `ProjectileApplyVelocity` | Apply +0x100/+0x104 velocity to position |
| 0x8005294c | `ProjectileTickWithLifetime` | Timer at +0x106, deactivate on timeout |
| 0x80052a68 | `ProjectileZOrderCallback` | Z-order based rendering |
| 0x80052adc | `ProjectileCollisionCallback` | Collision detection |
| 0x80052fd8 | `ProjectileTickWithCollision` | Tick + collision check |

### Homing Projectile System

| Address | Function | Purpose |
|---------|----------|--------|
| 0x80050ce4 | `HomingProjectileTick` | Calculate angle to player, apply velocity |
| 0x8004fdb8 | `HomingProjectile_TrackTarget` | Track target position |
| 0x80051898 | `HomingMissileTrackTarget` | Missile-specific tracking |
| 0x80051f54 | `ProjectileHomingTickState` | Homing state machine |
| 0x80052278 | `InitHomingProjectileEntity` | Initialize homing projectile |
| 0x80050970 | `InitProjectileWithTimer` | Init with lifetime timer |

### Sine/Cosine Lookup

| Address | Function | Purpose |
|---------|----------|--------|
| 0x8004f2a4 | `CalculateSineValue` | Fixed-point sine from 256-entry table at 0x8009c09c |

**Sine Table**: 256 entries (0-255 → 0-360°), values scaled ×0x1000 (4096)

### Bouncing/Special Projectiles

| Address | Function | Purpose |
|---------|----------|--------|
| 0x80053920 | `BouncingProjectileTick` | Projectile that bounces off surfaces |
| 0x80056144 | `InitClayballProjectile` | Clayball-specific projectile |
| 0x80040800 | `ProjectilePathFollowerTick` | Projectile following path data |

### Player Projectile Callbacks

| Address | Function | Purpose |
|---------|----------|--------|
| 0x8005e170 | `PlayerCallback_ProjectileEventHandler` | Handle projectile collision events |
| 0x8005e248 | `PlayerCallback_ProjectileSetStateAndSpawn` | Set state + spawn projectile |
| 0x8003e2b0 | `EntityEventHandlerSpawnProjectile` | Event handler to spawn projectile |
| 0x80040d74 | `EntityEventHandlerSpawnMultipleProjectiles` | Spawn circular projectile burst |

### Boss Projectiles

| Address | Function | Purpose |
|---------|----------|--------|
| 0x8004e680 | `KloggSpawnProjectilesCallback` | Klogg boss projectile attack |

---

## Projectile Entity Fields

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| +0x68 | s16 | worldX | Current X position |
| +0x6a | s16 | worldY | Current Y position |
| +0x100 | s32 | velocityX | X velocity (16.16 fixed-point) |
| +0x104 | s32 | velocityY | Y velocity (16.16 fixed-point) |
| +0x106 | u16 | lifetime | Frames until auto-destroy |
| +0x108 | u8 | flags | State flags |

---

## Related Documentation

- [Player System](player/player-system.md) - Player attack states
- [Items Reference](../reference/items.md) - Ammo pickups
- [Combat System](combat-system.md) - Damage mechanics
- [Physics Constants](../reference/physics-constants.md) - Movement speeds
- [Entity System](entities.md) - Entity lifecycle

---

**Projectile System**: **95% Complete** ✅

All projectile functions named. Remaining gaps are specific damage values and some collision mask details.

