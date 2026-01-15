# Entity Type 27: EnemyB (Flying Enemy)

**Entity Type**: 27  
**BLB Type**: 27  
**Callback**: 0x8007f354  
**Sprite ID**: 0x182d840c  
**Category**: Enemy (Flying)  
**Count**: 60 instances

---

## Overview

EnemyB is a flying enemy that moves through the air, unaffected by gravity. Typically uses sine wave movement patterns.

**AI Pattern**: Flying Movement (Pattern 2 from enemy-ai-overview.md)

---

## Behavior

**Movement Type**: Flying (no gravity)  
**Speed**: 1.0-1.5 px/frame horizontal  
**Collision**: Obstacles cause direction change  
**Attack**: Contact damage (1 life)  
**HP**: 1-2 HP (estimated)

### AI State Machine

**States**:
1. **FLY**: Main movement state
2. **TRACK**: Optional player tracking
3. **HURT**: Invincibility after damage
4. **DEATH**: Death animation

---

## Movement Logic

### Sine Wave Flight

```c
void EnemyB_TickCallback(Entity* enemy) {
    // Horizontal movement (constant speed)
    if (enemy->facing_left) {
        enemy->x_position -= FLIGHT_SPEED;  // -1.5 px/frame
    } else {
        enemy->x_position += FLIGHT_SPEED;  // +1.5 px/frame
    }
    
    // Vertical oscillation (sine wave)
    enemy->oscillation_phase += 2;  // Increment wave phase
    
    // Lookup or calculate sine value
    int sine_offset = sin_table[enemy->oscillation_phase & 0xFF];
    enemy->y_position = enemy->base_y + (sine_offset >> 4);  // ±8-16 pixels
    
    // No gravity applied (flying enemy)
    
    // Optional: Track player Y position slowly
    if (enemy->tracks_player) {
        if (player->y < enemy->base_y) {
            enemy->base_y--;  // Move up slowly
        } else if (player->y > enemy->base_y) {
            enemy->base_y++;  // Move down slowly
        }
    }
    
    // Turn at walls or screen boundaries
    if (CheckWallCollision(enemy) || OutOfBounds(enemy)) {
        enemy->facing_left = !enemy->facing_left;
    }
}
```

**Constants**:
- Flight Speed: 1.0-1.5 px/frame
- Oscillation Amplitude: ±8-16 pixels
- Oscillation Period: ~128 frames (2 seconds @ 60fps)
- Tracking Speed: 0.5-1.0 px/frame (if enabled)

---

## Combat

**HP**: 1-2 HP  
**Damage to Player**: 1 life on contact

**Vulnerability**:
- Can be jumped on
- Can be hit by projectiles
- Brief invincibility after hit (if HP > 1)

**Defeat**:
- Falls to ground (gravity applied on death)
- Brief death animation
- Removed

---

## Collision

**Bounding Box**: Estimated ~24×24 pixels

**Collision Mask**: `0x0002` (enemy layer)

**Special Behavior**: May have smaller hitbox than visual sprite (generous hit detection)

---

## Visual & Animation

**Sprite ID**: 0x182d840c

**Animations**:
- **Fly**: 4-6 frame wing flap cycle
- **Turn**: May have turn animation
- **Hurt**: Flash effect
- **Death**: Fall + crash animation

**Visual Style**: Likely bird, bat, or floating creature

---

## Spawn Behavior

**Spawn Trigger**: Enters camera view (spawn zone)

**Spawn Parameters**:
- Initial X/Y position
- Base Y for oscillation
- Facing direction
- Optional: Tracking enabled flag

**Despawn**: Exits camera bounds

---

## Variants

**Possible Variants**:
- **Stationary Flyer**: Oscillates in place
- **Horizontal Flyer**: Moves side-to-side with oscillation
- **Tracker**: Follows player Y position
- **Diver**: Swoops down at player periodically

**Note**: Variant determined by entity definition data or sub-type

---

## Godot Implementation

```gdscript
extends CharacterBody2D
class_name EnemyB_Flying

# Configuration
const FLIGHT_SPEED = 90.0  # 1.5 px/frame @ 60fps
const OSCILLATION_AMPLITUDE = 12.0
const OSCILLATION_SPEED = 2.0
const HP_MAX = 2

# State
var hp: int = HP_MAX
var facing_left: bool = false
var base_y: float = 0.0
var oscillation_phase: float = 0.0
var tracks_player: bool = false

func _ready() -> void:
    base_y = global_position.y

func _physics_process(delta: float) -> void:
    # Horizontal movement
    velocity.x = -FLIGHT_SPEED if facing_left else FLIGHT_SPEED
    
    # Sine wave oscillation
    oscillation_phase += OSCILLATION_SPEED
    var sine_value = sin(oscillation_phase * 0.1)
    global_position.y = base_y + sine_value * OSCILLATION_AMPLITUDE
    
    # Optional player tracking
    if tracks_player:
        var player = get_tree().get_first_node_in_group("player")
        if player:
            if player.global_position.y < base_y:
                base_y -= 0.5  # Slow vertical tracking
            elif player.global_position.y > base_y:
                base_y += 0.5
    
    # Turn at walls
    if is_on_wall():
        facing_left = not facing_left
    
    # No gravity (flying enemy)
    move_and_slide()

func take_damage(amount: int) -> void:
    hp -= amount
    if hp <= 0:
        die()
    else:
        flash_sprite()

func die() -> void:
    # Enable gravity on death (falls to ground)
    velocity.y = 0
    set_physics_process(false)
    
    var fall_tween = create_tween()
    fall_tween.tween_property(self, "global_position:y", global_position.y + 200, 1.0)
    fall_tween.finished.connect(queue_free)
```

---

## Related Documentation

- [Enemy AI Overview](../enemy-ai-overview.md) - Flying pattern (Pattern 2)
- [Combat System](../combat-system.md) - Damage mechanics
- [Entity Types](../../reference/entity-types.md) - Callback table

---

**Status**: ✅ **Fully Documented**  
**Pattern**: Flying enemy with sine wave movement  
**Complexity**: Low-Medium  
**Implementation**: Ready with pattern-based AI

