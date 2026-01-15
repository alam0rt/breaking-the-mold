# Physics Constants Reference

**Status**: ✅ VERIFIED from decompiled source  
**Source**: SLES_010.90.c (PAL version, SLES-01090)  
**Extraction Date**: January 14, 2026

This document provides a complete reference of all physics constants extracted from the decompiled game code.

---

## Player Movement Constants

### Horizontal Movement

| Constant | Hex Value | Decimal (16.16 Fixed) | Pixels/Frame | Pixels/Second (60fps) | Source Line(s) |
|----------|-----------|----------------------|--------------|----------------------|----------------|
| **Walk Speed (Normal)** | `0x20000` | 131,072 | **2.0** | 120.0 | 31761, 31941, 32013, 32077 |
| **Walk Speed (Fast)** | `0x30000` | 196,608 | **3.0** | 180.0 | 31759, 31939, 32011, 32075 |
| **Speed Modifier** | `0x8000` | 32,768 | **+0.5** | +30.0 | OR'd with base (line 31943) |
| **Max Horizontal Clamp** | `±0x20000` | ±131,072 | **±2.0** | ±120.0 | Line 34625 |

**Speed Selection Logic**:
```c
// From multiple PlayerState functions
uint max_velocity;
if (g_DefaultBGColorB & entity_input_flags) {
    max_velocity = 0x30000;  // Fast mode
} else {
    max_velocity = 0x20000;  // Normal mode
}

// Often combined with modifier flag
entity[0x124] = max_velocity | 0x8000;  // +0.5 px/frame boost
```

### Vertical Movement (Jump/Fall)

| Constant | Hex Value | Decimal (16.16 Fixed) | Pixels/Frame | Pixels/Second (60fps) | Source Line(s) |
|----------|-----------|----------------------|--------------|----------------------|----------------|
| **Initial Jump Velocity** | `0xFFFDC000` | -147,456 | **-2.25** | -135.0 | 32904, 32919, 32934, 33011, 33026 |
| **Jump Apex Velocity** | `0xFFD8` | -40 (s16) | **-0.625** | -37.5 | 31426 |
| **Gravity/Acceleration** | `0xFFFA0000` | -393,216 | **-6.0** | -360.0 | 32023, 32219, 32271, 33301 |
| **Landing Cushion** | `0xFFFFEE00` | -4,608 | **-0.07** | -4.2 | 32018 |
| **Landing Timer** | `5` | 5 frames | - | 83ms @ 60fps | 32017 |

### Jump Parameters

| Constant | Value | Offset | Description | Source |
|----------|-------|--------|-------------|--------|
| **Jump Param (Active)** | `0x0C` (12) | entity+0x156 | Set during jump for apex detection | Line 31478 |
| **Jump Param (Inactive)** | `0x00` (0) | entity+0x156 | Cleared when not jumping | Line 31481 |

---

## Camera System Constants

### Camera Acceleration

| Constant | Hex Value | Decimal (16.16 Fixed) | Purpose | Source |
|----------|-----------|----------------------|---------|--------|
| **Full Acceleration** | `0x10000` | 65,536 | Velocity increment (1.0) | Line 8436 |
| **Half Acceleration** | `0x8000` | 32,768 | Velocity increment (0.5) | Line 8436 |
| **Max Distance Index** | `0x8F` | 143 | Lookup table size limit | Line 8632 |

### Camera Lookup Tables (ROM Addresses)

| Table | Address | Size | Entries | Purpose |
|-------|---------|------|---------|---------|
| **Vertical Acceleration** | `DAT_8009b074` | 576 bytes | 144 × s32 | Y velocity by distance |
| **Horizontal Acceleration** | `DAT_8009b104` | 576 bytes | 144 × s32 | X velocity by distance |
| **Diagonal Acceleration** | `DAT_8009b0bc` | 576 bytes | 144 × s32 | Combined velocity |
| **Camera Y Offset** | `DAT_8009b038` | Unknown | Variable × s16 | Additional Y offset |

**Table Access Pattern**:
```c
// Index calculation: (distance >> 1) & 0x7C
// This gives: (distance / 2) * 4 (4 bytes per s32)
index = (distance >> 1) & 0x7C;
target_velocity = lookup_table[index];
```

**Extraction Commands**:
```bash
# From SLES_010.90 executable (subtract 0x80010000 from RAM address)
dd if=SLES_010.90 bs=1 skip=$((0x9b074 - 0x80010000)) count=576 of=camera_vert_accel.bin
dd if=SLES_010.90 bs=1 skip=$((0x9b104 - 0x80010000)) count=576 of=camera_horiz_accel.bin
dd if=SLES_010.90 bs=1 skip=$((0x9b0bc - 0x80010000)) count=576 of=camera_diag_accel.bin
```

---

## Projectile System Constants

### Projectile Spawning

| Constant | Hex Value | Decimal | Purpose | Source |
|----------|-----------|---------|---------|--------|
| **Angle Base** | `0xC00` | 3,072 | Base angle for upward direction | Line 35310 |
| **Trig Result Shift** | `>> 0xC` | Right shift 12 | Fixed-point trig scaling | Line 35312 |
| **Velocity Scale** | `<< 10` | Left shift 10 | Final velocity multiplier (×1024) | Line 35318 |
| **Entity Allocation Size** | `0x114` | 276 bytes | Projectile entity size | Line 35315 |

### Projectile Sprite

| Field | Value | Description |
|-------|-------|-------------|
| **Sprite Hash** | `0x168254b5` | Projectile graphics sprite ID |
| **Init Function** | `InitEntity_168254b5` | Projectile initialization |

### Ammo System

| Field | Location | Default Value | Description | Source |
|-------|----------|---------------|-------------|--------|
| **Max Green Bullets** | `g_pPlayerState[0x1A]` | `3` | Maximum bullet count | Line 42542 |

### Damage System

| Field | Offset | Description | Source |
|-------|--------|-------------|--------|
| **Base Damage** | entity+0x44 | Damage value | Line 33063 |
| **Damage Modifier** | entity+0x16 | If `0x8000`, damage is halved | Line 33064 |

**Damage Calculation**:
```c
damage = player_entity[0x44];
if (player_entity[0x16] == 0x8000) {
    damage = damage >> 1;  // Half damage
}
```

---

## Entity Velocity Offsets

### Primary Velocity Fields

| Offset | Size | Field | Description | Usage |
|--------|------|-------|-------------|-------|
| `+0x104` | 4 | `velocity_x` | X velocity (16.16 fixed) | Projectiles, moving entities |
| `+0x108` | 4 | `velocity_y` | Y velocity (16.16 fixed) | Projectiles, moving entities |
| `+0x110` | 4 | `gravity_accel` | Vertical acceleration | Gravity for falling entities |
| `+0x118` | 4 | `cushion_vel` | Landing deceleration | Soft landing effect |
| `+0x11C` | 1 | `landing_timer` | Landing countdown | 5 frames |
| `+0x124` | 4 | `max_velocity` | Velocity limit | Speed clamp |
| `+0x136` | 2 | `apex_velocity` | Jump peak velocity | -0.625 px/frame |
| `+0x156` | 2 | `jump_param` | Apex detection | 12 during jump |

### Player-Specific Push Forces

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| `+0x160` | 2 | `push_x` | X movement force (cleared after apply) |
| `+0x162` | 2 | `push_y` | Y movement force (cleared after apply) |
| `+0x74` | 1 | `facing_left` | Direction flag (0=right, 1=left) |
| `+0x75` | 1 | `moving_up` | Direction flag (0=down, 1=up) |

---

## Fixed-Point Math Reference

### 16.16 Fixed-Point Format

**Structure**: 32-bit value
- Upper 16 bits = whole pixels
- Lower 16 bits = fractional pixels (1/65536 precision)

### Conversion Table

| Hex Value | Decimal | Pixels/Frame | Pixels/Second (60fps) | Usage |
|-----------|---------|--------------|----------------------|-------|
| `0x10000` | 65,536 | 1.0 | 60.0 | Unit value |
| `0x20000` | 131,072 | 2.0 | 120.0 | Normal walk |
| `0x30000` | 196,608 | 3.0 | 180.0 | Fast walk |
| `0x8000` | 32,768 | 0.5 | 30.0 | Half speed |
| `0xFFD8` | -40 | -0.000610 | -0.037 | Jump apex (as s16) |
| `0xFFFA0000` | -393,216 | -6.0 | -360.0 | Gravity |
| `0xFFFDC000` | -147,456 | -2.25 | -135.0 | Jump velocity |
| `0xFFFFEE00` | -4,608 | -0.07 | -4.2 | Landing cushion |

### Conversion Functions

**To Pixels Per Frame**:
```c
float pixels_per_frame = (float)fixed_point_value / 65536.0f;
```

**From Pixels Per Frame**:
```c
int32_t fixed_point = (int32_t)(pixels_per_frame * 65536.0f);
```

**For Godot (Pixels Per Second)**:
```gdscript
# Multiply by 60 fps
const WALK_SPEED = 2.0 * 60  # 120 px/sec
const GRAVITY = 6.0 * 60 * 60  # 21,600 px/sec²
```

---

## Sound Effect Constants

| Sound ID | Hex Value | Context | Source |
|----------|-----------|---------|--------|
| **Jump Sound** | `0x248E52` | Played on jump | Line 31589 |
| **Pickup Sound** | `0x7003474C` | Item collection | Line 17812 |

---

## Angle System (Projectiles)

### Angle Format

**Range**: 0-4095 (12-bit value)  
**Units**: 4096 = full circle (360°)

| Angle | Hex | Degrees | Direction |
|-------|-----|---------|-----------|
| 0 | `0x000` | 0° | Right |
| 1024 | `0x400` | 90° | Down |
| 2048 | `0x800` | 180° | Left |
| 3072 | `0xC00` | 270° | Up |

**Angle Inversion**: `adjusted_angle = 0xC00 - input_angle`

**Trigonometry**:
- PSX `csin()` and `ccos()` return 12-bit fixed-point values (range: -4096 to +4096)
- Result must be shifted right by 12 bits after multiplication

---

## Sprite IDs Reference

### Player & Projectile Sprites

| Entity Type | Sprite Hash | Context | Source |
|-------------|-------------|---------|--------|
| **Player** | `0x21842018` | Main player sprite | InitPlayerEntity |
| **Projectile** | `0x168254b5` | Bullet/projectile | Line 35316 |
| **Jump Animation** | `0x092B8480` | Player jumping | Line 31586 |
| **Falling Animation** | `0x0B2084D0` | Player falling | Line 31496 |
| **Pickup Animation** | `0x1C3AA013` | Item collection | Line 31963 |
| **Debris Particle 1** | `0xBE68D0C6` | Explosion debris | Line 35379 |
| **Debris Particle 2** | `0xB868D0C6` | Explosion debris | Line 35384 |
| **Debris Particle 3** | `0xB468D0C6` | Explosion debris | Line 35388 |

---

## Global State Arrays

### g_pPlayerState Array

**Location**: Global pointer at `0x800A????`

| Index | Hex | Field | Description | Source |
|-------|-----|-------|-------------|--------|
| `[0x04]` | - | `world_index` | Cumulative world counter | TileHeader+0x20 |
| `[0x06-0x0F]` | - | `collectible_flags` | Zone collection flags (10 zones) | Line 17810 |
| `[0x11]` | - | `powerup_flag_1` | Unknown boolean | Lines 10524, 10556 |
| `[0x12]` | - | `powerup_flag_2` | Unknown boolean | Lines 10439, 10471 |
| `[0x13]` | - | `powerup_flag_3` | Unknown boolean | Lines 10614, 10646 |
| `[0x14]` | - | `special_ability_1` | Reset on level load | Lines 10166, 31111 |
| `[0x15]` | - | `special_ability_2` | Reset on level load | Lines 10167, 31104 |
| `[0x16]` | - | `special_ability_3` | Reset on level load, affects damage | Lines 10170, 31097, 33064 |
| `[0x18]` | - | `unknown_flag` | Reset on init | Line 10081 |
| `[0x1A]` | - | `max_green_bullets` | Ammo capacity (default: 3) | Line 42542 |
| `[0x1C]` | - | `special_ability_4` | Reset on level load | Lines 10173, 31092 |

---

## Collision Constants

### Tile Attribute Ranges

| Range | Hex | Meaning | Source |
|-------|-----|---------|--------|
| `0x00` | - | Empty/passable | Floor check logic |
| `0x01-0x3B` | - | Solid range (floor collision) | Line 17759-17803 |
| `0x02` | - | Standard solid block | Common value |
| `0x3C+` | - | Trigger zones | CheckTriggerZoneCollision |

### Trigger Types (from CheckTriggerZoneCollision)

| Value | Hex | Type | Action | Source |
|-------|-----|------|--------|--------|
| `0x00` | - | Checkpoint | Sets GameState+0x148 | Line 17805 |
| `0x02-0x07` | - | Level Exit | 6 exit types, calls SetGameMode() | Lines 17819-17838 |
| `0x32-0x3B` | - | Collectible Zone | 10 zone types, sets collection flags | Lines 17808-17814 |

---

## Entity Structure Offsets (Physics-Related)

### Position & Movement

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| `+0x68` | 2 | s16 | `x_position` | X position (whole pixels) |
| `+0x6A` | 2 | s16 | `y_position` | Y position (whole pixels) |
| `+0x6C` | 2 | u16 | `x_fraction` | X fractional position |
| `+0x6E` | 2 | u16 | `y_fraction` | Y fractional position |
| `+0x74` | 1 | u8 | `facing_left` | 0=right, 1=left |
| `+0x75` | 1 | u8 | `moving_up` | 0=down, 1=up |

### Velocity & Acceleration

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| `+0x104` | 4 | s32 | `velocity_x` | X velocity (16.16 fixed) |
| `+0x108` | 4 | s32 | `velocity_y` | Y velocity (16.16 fixed) |
| `+0x110` | 4 | s32 | `gravity_accel` | Vertical acceleration |
| `+0x118` | 4 | s32 | `cushion_vel` | Landing deceleration |
| `+0x11C` | 1 | u8 | `landing_timer` | Landing state countdown |
| `+0x124` | 4 | u32 | `max_velocity` | Velocity clamp value |
| `+0x136` | 2 | s16 | `apex_velocity` | Jump apex velocity |
| `+0x156` | 2 | s16 | `jump_param` | Jump detection parameter |
| `+0x160` | 2 | s16 | `push_x` | X push force (player) |
| `+0x162` | 2 | s16 | `push_y` | Y push force (player) |

### State Machine

| Offset | Size | Type | Field | Description |
|--------|------|------|-------|-------------|
| `+0x00` | 4 | u32 | `state_high` | State flags (upper word) |
| `+0x04` | 4 | ptr | `callback_main` | Main update callback |
| `+0x0C` | 4 | ptr | `callback_secondary` | Secondary callback |
| `+0xA2` | 2 | s16 | `state_index` | Current state index (-1 = transitioning) |
| `+0xA4` | 4 | ptr | `state_callback` | Current state handler |
| `+0x98` | 4 | ptr | `next_state_callback` | Queued state |

---

## Conversion Utilities

### PSX to Godot Physics

```gdscript
# Convert 16.16 fixed-point to Godot velocity (px/sec)
func psx_to_godot_velocity(psx_fixed: int) -> float:
    var px_per_frame = float(psx_fixed) / 65536.0
    return px_per_frame * 60.0  # 60 fps

# Convert 16.16 fixed-point acceleration to Godot (px/sec²)
func psx_to_godot_accel(psx_fixed: int) -> float:
    var px_per_frame_sq = float(psx_fixed) / 65536.0
    return px_per_frame_sq * 60.0 * 60.0

# Examples:
# WALK_SPEED = psx_to_godot_velocity(0x20000) = 120.0 px/sec
# GRAVITY = psx_to_godot_accel(0xFFFA0000) = -21600.0 px/sec²
```

### Angle Conversion

```gdscript
# PSX angle (0-4095) to radians
func psx_angle_to_radians(psx_angle: int) -> float:
    return float(psx_angle) * TAU / 4096.0

# PSX angle to degrees
func psx_angle_to_degrees(psx_angle: int) -> float:
    return float(psx_angle) * 360.0 / 4096.0
```

---

## Related Documentation

- [Player Physics](../systems/player/player-physics.md) - Complete player movement
- [Camera System](../systems/camera.md) - Camera smooth scrolling
- [Projectiles](../systems/projectiles.md) - Weapon system details
- [Entities](../systems/entities.md) - Entity structure reference
- [Collision System](../systems/collision.md) - Tile collision attributes

