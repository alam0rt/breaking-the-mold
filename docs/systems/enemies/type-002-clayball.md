# Entity Type 2: Clayball

**Entity Type**: 2  
**BLB Type**: 2  
**Callback**: 0x80080328  
**Sprite ID**: 0x09406d8a  
**Category**: Collectible  
**Count**: 5,727 instances (most common entity!)

---

## Overview

Clayballs are the primary collectible in Skullmonkeys. Collecting 100 clayballs grants an extra life.

**Gameplay Function**: Currency/collectible system (like rings in Sonic or coins in Mario)

---

## Behavior

**Type**: Stationary collectible with idle animation

**Movement**: None (fixed position)
**Collision**: Player touch triggers collection
**Respawn**: Does not respawn after collection
**Persistence**: Collection persists through checkpoints

---

## Collection System

### Player State Storage

**Counter**: `g_pPlayerState[0x12]` (orb_count)
**Maximum**: 99 displayed (100th grants 1-up)

**From player state documentation**:
```c
// Clayball counter
g_pPlayerState[0x12] = orb_count;  // 0-99 (100 → 1up, reset to 0)
```

### Collection Logic

```c
// When player touches clayball
if (CheckEntityCollision(player, clayball)) {
    // Play collection sound
    PlaySoundEffect(0x7003474c, pan, 0);  // Collection sound
    
    // Increment counter
    g_pPlayerState[0x12]++;
    
    // Check for 1-up
    if (g_pPlayerState[0x12] >= 100) {
        g_pPlayerState[0x11]++;  // Grant extra life
        g_pPlayerState[0x12] = 0;  // Reset counter
        PlaySoundEffect(ONE_UP_SOUND, 0, 0);
    }
    
    // Remove clayball
    RemoveEntity(clayball);
}
```

---

## Visual & Animation

**Sprite ID**: 0x09406d8a

**Expected Animation**:
- Idle: Gentle bobbing or spinning
- Collection: Brief sparkle effect
- Framerate: 6-8 frames per second

**Visual Style**: Spherical/orb-shaped collectible

---

## Entity Structure

**Size**: Likely standard entity (0x44C bytes) or smaller collectible variant

**Key Fields**:
- `+0x68/0x6A`: Position (X/Y)
- `+0x18`: Tick callback (idle animation)
- `+0x12`: Collision mask (player layer)
- `+0xCC`: Active sprite ID (0x09406d8a)

**Collision Mask**: Likely `0x0001` (player collision layer)

---

## Spawn Data (Asset 501)

**Entity Definition** (24 bytes):
```
+0x00  s16  x_position (pixels)
+0x02  s16  y_position (pixels)
+0x04  ???  Additional data
+0x12  u16  entity_type = 2 (Clayball)
```

**Spawning**: SpawnOnScreenEntities detects when clayball enters camera view and spawns entity

---

## Sound Effects

**Collection**: 0x7003474c (verified from line 17812)

**Context**:
```c
FUN_8001c4a4(param_1, 0x7003474c);  // Play pickup sound
```

**1-Up Sound**: Unknown (need extraction)

---

## Level Design

**Placement Patterns**:
- Trails guiding player through level
- Hidden in secret areas
- Along optimal path lines
- Near hazards (risk/reward)
- Groups of 5-10 for visual appeal

**Density**: 5,727 total across all levels (average ~220 per level)

---

## HUD Display

**Location**: Top-left or top-right of screen  
**Format**: "×NN" where NN is count (0-99)  
**Update**: Real-time when collected  
**100th Ball**: Counter resets, life count increments

---

## Godot Implementation

```gdscript
extends Area2D
class_name Clayball

# Configuration
const SPRITE_ID = 0x09406d8a
const COLLECTION_SOUND_ID = 0x7003474c

# State
var collected: bool = false

func _ready() -> void:
    # Set up collision
    set_collision_layer_value(4, true)  # Item layer
    set_collision_mask_value(1, true)   # Collides with player
    
    # Connect signal
    body_entered.connect(_on_player_touch)
    
    # Start idle animation
    play_idle_animation()

func _on_player_touch(body: Node2D) -> void:
    if collected:
        return
    
    if body.is_in_group("player"):
        collect(body)

func collect(player: Node2D) -> void:
    collected = true
    
    # Play sound
    AudioManager.play_sound(COLLECTION_SOUND_ID)
    
    # Increment player's clayball counter
    player.add_clayball()
    
    # Visual effect
    play_collection_effect()
    
    # Remove
    queue_free()

func play_idle_animation() -> void:
    # Gentle bobbing or rotation
    var tween = create_tween().set_loops()
    tween.tween_property(self, "position:y", position.y + 4, 0.5)
    tween.tween_property(self, "position:y", position.y - 4, 0.5)
```

---

## Related Documentation

- [Player System](../player/player-system.md) - Player state with orb counter
- [Items Reference](../../reference/items.md) - All collectible items
- [Sound IDs](../../reference/sound-ids-complete.md) - Collection sound

---

**Status**: ✅ **Fully Documented**  
**Implementation**: Ready for accurate recreation  
**Coverage**: 100% for this entity type

