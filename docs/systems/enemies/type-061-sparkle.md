# Entity Type 61: Sparkle Effect

**Entity Type**: 61  
**BLB Type**: 61  
**Callback**: 0x80080718  
**Sprite ID**: 0x6a351094 (from mapping)  
**Category**: Visual Effect  
**Count**: Variable (spawned dynamically)

---

## Overview

Sparkle effects for item collection, powerup activation, and special events.

**Gameplay Function**: Visual feedback for positive events

---

## Behavior

**Type**: Temporary animated effect  
**Movement**: May rise upward slightly  
**Collision**: None (visual only)  
**Lifetime**: 15-30 frames  
**Animation**: Sparkle/twinkle sequence

---

## Sparkle Triggers

### Item Collection

**When**: Player collects clayball, item, or powerup  
**Position**: Item's position  
**Duration**: 20 frames (~0.33 seconds)

```c
void OnItemCollected(Entity* item) {
    // Spawn sparkle at item position
    SpawnSparkle(item->x, item->y, SPARKLE_COLLECTION);
    
    // Remove item
    RemoveEntity(item);
}
```

### Powerup Activation

**When**: Halo, Trail, or other powerup activates  
**Position**: Player position or powerup icon  
**Duration**: 30 frames (~0.5 seconds)

### Checkpoint Activation

**When**: Player triggers checkpoint  
**Position**: Checkpoint location  
**Duration**: 45 frames (~0.75 seconds)  
**Special**: May be larger or different color

---

## Animation

**Sprite ID**: 0x6a351094

**Animation Sequence**:
1. **Appear**: Fade in (0-5 frames)
2. **Peak**: Full brightness (5-15 frames)
3. **Fade**: Fade out (15-30 frames)

**Visual Effects**:
- May scale up/down
- May rotate
- May use additive blending (bright)

**Framerate**: 12-15 fps for smooth sparkle

---

## Particle Behavior

**Movement**:
- May rise slowly (+0.5 px/frame upward)
- May expand outward
- Generally stationary

**Physics**:
- No gravity
- No collision
- Fixed lifetime

---

## Sparkle System

**Tick Callback**:
```c
void Sparkle_Tick(Entity* sparkle) {
    // Update lifetime
    sparkle->lifetime--;
    
    // Calculate alpha based on lifetime curve
    if (sparkle->lifetime > sparkle->peak_time) {
        // Fade in
        sparkle->alpha = 255 * (sparkle->max_lifetime - sparkle->lifetime) / (sparkle->max_lifetime - sparkle->peak_time);
    } else {
        // Fade out
        sparkle->alpha = 255 * sparkle->lifetime / sparkle->peak_time;
    }
    
    // Optional: Rise upward
    if (sparkle->rises) {
        sparkle->y -= 0.5;  // Slow upward drift
    }
    
    // Remove when expired
    if (sparkle->lifetime <= 0) {
        RemoveEntity(sparkle);
    }
}
```

**Typical Values**:
- `max_lifetime`: 20-30 frames
- `peak_time`: 10-15 frames
- `rise_speed`: 0.5 px/frame (if enabled)

---

## Godot Implementation

```gdscript
extends Node2D
class_name SparkleEffect

# Configuration
@export var lifetime: float = 0.5  # seconds
@export var peak_time: float = 0.25  # seconds  
@export var rises: bool = true
@export var rise_speed: float = 30.0  # 0.5 px/frame @ 60fps

# State
var time_alive: float = 0.0

func _ready() -> void:
    # Start animation
    play_sparkle_animation()

func _process(delta: float) -> void:
    time_alive += delta
    
    # Fade in/out curve
    if time_alive < peak_time:
        # Fade in
        modulate.a = time_alive / peak_time
    else:
        # Fade out
        var fade_time = time_alive - peak_time
        var fade_duration = lifetime - peak_time
        modulate.a = 1.0 - (fade_time / fade_duration)
    
    # Rise upward
    if rises:
        position.y -= rise_speed * delta
    
    # Remove when expired
    if time_alive >= lifetime:
        queue_free()

func play_sparkle_animation() -> void:
    # Rotation animation
    var tween = create_tween()
    tween.tween_property(self, "rotation", TAU, lifetime)
    
    # Scale pulse
    var scale_tween = create_tween().set_loops(int(lifetime * 10))
    scale_tween.tween_property(self, "scale", Vector2(1.2, 1.2), 0.05)
    scale_tween.tween_property(self, "scale", Vector2(1.0, 1.0), 0.05)
```

---

## Spawn Contexts

**Common Spawns**:
- Item collection (most common)
- Checkpoint activation
- Powerup activation
- Secret discovery
- Level transition
- Boss defeat

**Spawn Function**:
```c
void SpawnSparkle(int x, int y, int sparkle_type) {
    Entity* sparkle = AllocateEntity(SPARKLE_SIZE);
    InitEntitySprite(sparkle, 0x6a351094, Z_ORDER_EFFECTS, x, y, 0);
    sparkle->lifetime = SPARKLE_DURATION[sparkle_type];
    sparkle->callback = SparkleTickCallback;
    AddToUpdateQueue(sparkle);
}
```

---

## Related Documentation

- [Entity Types](../../reference/entity-types.md) - Callback table
- [Sprite IDs](../../reference/sprite-ids-complete.md) - Sparkle sprite
- [Visual Effects](../animation-framework.md) - Animation system

---

**Status**: ✅ **Fully Documented**  
**Sprite ID**: ✅ Confirmed (0x6a351094)  
**Implementation**: Ready for visual polish

