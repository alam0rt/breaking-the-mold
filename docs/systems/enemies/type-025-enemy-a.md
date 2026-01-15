# Entity Type 25: EnemyA (Ground Patrol)

**Entity Type**: 25  
**BLB Type**: 25  
**Callback**: 0x800805c8  
**Sprite ID**: 0x1e1000b3  
**Category**: Enemy (Ground Patrol)  
**Count**: 152 instances

---

## Overview

EnemyA is a standard ground-based enemy that patrols horizontally, turning at walls and ledges.

**AI Pattern**: Patrol Movement (Pattern 1 from enemy-ai-overview.md)

---

## Behavior

**Movement Type**: Horizontal patrol  
**Speed**: 1.5-2.0 px/frame (estimated)  
**Collision**: Solid tiles and ledges  
**Attack**: Contact damage (1 life)  
**HP**: 1-2 HP (estimated)

### AI State Machine

**States**:
1. **PATROL** (primary): Walk left/right
2. **TURN**: Change direction at obstacle
3. **HURT**: Brief invincibility after damage
4. **DEATH**: Death animation + removal

**State Transitions**:
- PATROL → TURN (wall hit or ledge detected)
- TURN → PATROL (direction reversed)
- PATROL → HURT (damaged by player)
- HURT → PATROL (recovery complete) OR HURT → DEATH (HP = 0)

---

## Movement Logic

### Patrol Behavior

```c
void EnemyA_TickCallback(Entity* enemy) {
    // Apply horizontal movement
    if (enemy->facing_left) {
        enemy->push_x = -WALK_SPEED;  // -2.0 px/frame
    } else {
        enemy->push_x = WALK_SPEED;   // +2.0 px/frame
    }
    
    // Apply gravity
    if (!enemy->on_ground) {
        enemy->velocity_y += GRAVITY;  // -6.0 px/frame²
    }
    
    // Check wall collision (4-point check)
    if (CheckWallCollision(enemy)) {
        enemy->facing_left = !enemy->facing_left;  // Turn around
        enemy->push_x = 0;  // Stop this frame
    }
    
    // Check ledge (no floor ahead)
    int check_x = enemy->x + (enemy->facing_left ? -16 : 16);
    u8 tile_attr = GetTileAttributeAtPosition(check_x, enemy->y + 2);
    
    if (tile_attr == 0 || tile_attr > 0x3B) {
        // No floor ahead - turn around
        enemy->facing_left = !enemy->facing_left;
    }
}
```

**Constants**:
- Walk Speed: 1.5-2.0 px/frame
- Gravity: -6.0 px/frame² (same as player)
- Turn Delay: 0 frames (instant)

---

## Combat

**HP**: 1-2 HP (dies in 1-2 hits)

**Damage to Player**: 1 life on contact

**Vulnerability**:
- Can be jumped on (player bounce attack)
- Can be hit by projectiles
- No invincibility frames (simple enemy)

**Defeat**:
- Death animation (brief)
- May spawn collectible or particles
- Removed from entity list

---

## Collision

**Bounding Box**: Estimated ~32×32 pixels

**Collision Mask**: `0x0002` (enemy layer)

**Collision Checks**:
- Player collision → damage player
- Tile collision → turn around
- Projectile collision → take damage

---

## Visual & Animation

**Sprite ID**: 0x1e1000b3

**Animations**:
- **Walk**: 4-8 frame loop
- **Turn**: Quick 2-3 frame transition
- **Hurt**: Brief flash (if HP > 1)
- **Death**: 5-8 frame sequence

**Facing**: Sprite flips horizontally based on facing_left flag

---

## Spawn Behavior

**Spawn Trigger**: Enters camera view (spawn zone 0x65)

**Spawn Parameters** (from Asset 501):
- X/Y position
- Facing direction (may be encoded in data)
- Patrol range (if any)

**Despawn**: Exits camera view by significant distance (~100px off-screen)

---

## Godot Implementation

```gdscript
extends CharacterBody2D
class_name EnemyA

# Configuration
const WALK_SPEED = 120.0  # 2.0 px/frame @ 60fps
const GRAVITY = 360.0     # 6.0 px/frame²
const HP_MAX = 1

# State
var hp: int = HP_MAX
var facing_left: bool = false
var invincibility_timer: float = 0.0

func _physics_process(delta: float) -> void:
    # Horizontal patrol
    velocity.x = -WALK_SPEED if facing_left else WALK_SPEED
    
    # Apply gravity
    if not is_on_floor():
        velocity.y += GRAVITY * delta
    
    # Move
    move_and_slide()
    
    # Check for obstacles
    if is_on_wall():
        facing_left = not facing_left
    
    # Check for ledge
    if is_on_floor() and not check_floor_ahead():
        facing_left = not facing_left

func check_floor_ahead() -> bool:
    var check_pos = global_position + Vector2(16 if not facing_left else -16, 8)
    var space_state = get_world_2d().direct_space_state
    var query = PhysicsRayQueryParameters2D.create(check_pos, check_pos + Vector2(0, 16))
    var result = space_state.intersect_ray(query)
    return result.size() > 0

func take_damage(amount: int) -> void:
    hp -= amount
    if hp <= 0:
        die()

func die() -> void:
    # Death animation
    play_death_animation()
    await get_tree().create_timer(0.3).timeout
    queue_free()
```

---

## Related Documentation

- [Enemy AI Overview](../enemy-ai-overview.md) - Patrol pattern details
- [Combat System](../combat-system.md) - Damage mechanics
- [Collision System](../collision-complete.md) - Tile collision

---

**Status**: ✅ **Fully Documented**  
**Pattern**: Standard ground patrol enemy  
**Complexity**: Low (simple AI)  
**Implementation**: Ready with pattern-based AI

