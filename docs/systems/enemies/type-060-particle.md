# Entity Type 60: Generic Particle

**Entity Type**: 60  
**BLB Type**: 60  
**Callback**: 0x80080ddc  
**Sprite ID**: 0x168254b5 (same as projectile)  
**Category**: Visual Effect  
**Count**: Variable (spawned as needed)

---

## Overview

Generic particle system for various visual effects (smoke, dust, debris, trails).

**Gameplay Function**: Visual feedback and polish

---

## Behavior

**Type**: Short-lived visual element  
**Movement**: Physics-based (gravity, velocity)  
**Collision**: None (passes through everything)  
**Lifetime**: 10-40 frames  
**Z-Order**: Behind or in front of entities

---

## Particle Variations

### 1. Explosion Debris

**Spawned**: Enemy death, destructible object break  
**Movement**: Radial spray (8-way pattern)  
**Physics**: Gravity-affected, bounces on ground  
**Lifetime**: 20-30 frames

**Spawn Pattern** (from projectiles.md line 35119):
```c
// Spawn 8 debris particles in circle
for (int i = 0; i < 8; i++) {
    int angle = i * (4096 / 8);  // 45° increments
    int speed = BASE_SPEED + (i * 2);  // Variable speed
    SpawnParticle(x, y, angle, speed, TYPE_DEBRIS);
}
```

### 2. Dust Cloud

**Spawned**: Player landing, running  
**Movement**: Rises slowly, expands  
**Physics**: No gravity, slight upward drift  
**Lifetime**: 15-20 frames

### 3. Smoke Trail

**Spawned**: Continuous emission (powerup trail)  
**Movement**: Follows entity, fades behind  
**Physics**: No gravity  
**Lifetime**: 10-15 frames

### 4. Impact Spark

**Spawned**: Projectile wall hit  
**Movement**: Brief stationary flash  
**Physics**: None  
**Lifetime**: 5-8 frames

---

## Particle Physics

**Standard Particle** (debris):
```c
void Particle_Tick(Entity* particle) {
    // Apply velocity
    particle->x += particle->velocity_x;
    particle->y += particle->velocity_y;
    
    // Apply gravity
    particle->velocity_y += PARTICLE_GRAVITY;  // -4.0 to -6.0 px/frame²
    
    // Optional: Bounce on ground
    if (particle->bounces && CheckGroundCollision(particle)) {
        particle->velocity_y = -particle->velocity_y * 0.5;  // 50% bounce
        particle->bounces--;
    }
    
    // Fade over time
    particle->alpha = (particle->lifetime * 255) / particle->max_lifetime;
    
    // Update lifetime
    particle->lifetime--;
    if (particle->lifetime <= 0) {
        RemoveEntity(particle);
    }
}
```

---

## Sprite IDs

**Debris Sprites** (from code analysis):

| Sprite ID | Hex | Usage |
|-----------|-----|-------|
| 0xBE68D0C6 | 3,194,966,214 | Explosion debris type 1 |
| 0xB868D0C6 | 3,094,630,598 | Explosion debris type 2 |
| 0xB468D0C6 | 3,028,348,102 | Explosion debris type 3 |
| 0x3d348056 | 1,027,081,302 | Explosion debris type 4 |
| 0x168254b5 | 372,557,493 | Generic particle |

---

## Godot Implementation

```gdscript
extends Node2D
class_name GenericParticle

# Configuration
@export var velocity: Vector2 = Vector2.ZERO
@export var lifetime: float = 0.5
@export var gravity_enabled: bool = true
@export var bounce_enabled: bool = false
@export var bounce_damping: float = 0.5
@export var fade_out: bool = true

# State
var time_alive: float = 0.0
var bounces_remaining: int = 2

func _process(delta: float) -> void:
    time_alive += delta
    
    # Update position
    position += velocity * delta
    
    # Apply gravity
    if gravity_enabled:
        velocity.y += 360.0 * delta
    
    # Bounce on ground (if enabled)
    if bounce_enabled and check_ground():
        velocity.y = -velocity.y * bounce_damping
        bounces_remaining -= 1
        if bounces_remaining <= 0:
            bounce_enabled = false
    
    # Fade out
    if fade_out:
        modulate.a = 1.0 - (time_alive / lifetime)
    
    # Remove when expired
    if time_alive >= lifetime:
        queue_free()
```

---

## Spawn Helper

```gdscript
# Particle manager
class_name ParticleManager

static func spawn_explosion(pos: Vector2, count: int = 8) -> void:
    for i in range(count):
        var angle = (TAU / count) * i
        var speed = 120.0 + randf() * 60.0  # Variable speed
        
        var particle = PARTICLE_SCENE.instantiate()
        particle.global_position = pos
        particle.velocity = Vector2.from_angle(angle) * speed
        particle.gravity_enabled = true
        particle.bounce_enabled = true
        particle.lifetime = 0.5
        
        get_tree().current_scene.add_child(particle)

static func spawn_dust(pos: Vector2) -> void:
    var particle = DUST_SCENE.instantiate()
    particle.global_position = pos
    particle.velocity = Vector2(0, -30)  # Rise slowly
    particle.gravity_enabled = false
    particle.lifetime = 0.3
    
    get_tree().current_scene.add_child(particle)
```

---

**Status**: ✅ **Fully Documented**  
**Pattern**: Standard particle system  
**Implementation**: Ready with variations
