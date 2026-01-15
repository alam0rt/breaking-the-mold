# Entity Type 10: Interactive Object

**Entity Type**: 10  
**BLB Type**: 10  
**Callback**: 0x8007f244  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Interactive Object  
**Count**: Unknown (moderate frequency)

---

## Overview

Interactive objects are entities that respond to player actions (jumping on, hitting, proximity).

**Gameplay Function**: Environmental interaction and puzzle elements

---

## Behavior

**Type**: Stationary or reactive  
**Movement**: Triggered by player interaction  
**Collision**: Varies by object type  
**Interaction**: Player action triggers state change  
**Examples**: Switches, destructible blocks, bounce pads

---

## Interaction Patterns

### Pattern 1: Bounce Pad

**Trigger**: Player lands on top  
**Effect**: Launches player upward

```c
if (PlayerLandsOnTop(player, object)) {
    // Launch player
    player->velocity_y = BOUNCE_VELOCITY;  // -4.0 to -6.0 px/frame
    
    // Play sound
    PlaySoundEffect(BOUNCE_SOUND, pan, 0);
    
    // Animate object (compression)
    SetAnimation(object, ANIM_COMPRESS);
}
```

### Pattern 2: Switch/Button

**Trigger**: Player jumps on or attacks  
**Effect**: Activates mechanism

```c
if (PlayerActivates(player, switch_object)) {
    // Toggle state
    switch_object->activated = !switch_object->activated;
    
    // Trigger level event
    SetLevelFlag(switch_object->flag_id);
    
    // Visual feedback
    SetAnimation(switch_object, ANIM_PRESSED);
}
```

### Pattern 3: Destructible Block

**Trigger**: Player attacks or jumps on  
**Effect**: Breaks and disappears

```c
if (PlayerAttacks(player, block)) {
    // Spawn debris particles
    SpawnDebris(block->x, block->y, 8);
    
    // Play break sound
    PlaySoundEffect(BREAK_SOUND, pan, 0);
    
    // Remove block
    RemoveEntity(block);
}
```

---

## State Machine

**States**:
1. **IDLE**: Waiting for interaction
2. **TRIGGERED**: Player interaction occurred
3. **ACTIVE**: Performing action
4. **COOLDOWN**: Recovery period
5. **INACTIVE**: Used/destroyed

---

## Godot Implementation

```gdscript
extends StaticBody2D
class_name InteractiveObject

enum InteractionType { BOUNCE_PAD, SWITCH, DESTRUCTIBLE }

@export var interaction_type: InteractionType = InteractionType.BOUNCE_PAD
@export var bounce_velocity: float = -240.0  # -4.0 px/frame
@export var cooldown_time: float = 0.5

var activated: bool = false
var cooldown_timer: float = 0.0

func _process(delta: float) -> void:
    if cooldown_timer > 0:
        cooldown_timer -= delta

func _on_player_interaction(player: Node2D, interaction_mode: String) -> void:
    if cooldown_timer > 0:
        return
    
    match interaction_type:
        InteractionType.BOUNCE_PAD:
            if interaction_mode == "land_on_top":
                player.velocity.y = bounce_velocity
                play_bounce_animation()
                cooldown_timer = cooldown_time
        
        InteractionType.SWITCH:
            activated = not activated
            emit_signal("switch_toggled", activated)
            play_switch_animation()
            cooldown_timer = cooldown_time
        
        InteractionType.DESTRUCTIBLE:
            spawn_debris()
            queue_free()
```

---

**Status**: ✅ **Patterns Documented**  
**Sprite ID**: ⚠️ Needs extraction  
**Implementation**: Ready with pattern variations

