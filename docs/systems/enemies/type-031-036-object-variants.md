# Entity Types 31-36: Interactive Object Variants

**Entity Types**: 31, 32, 33, 34, 35, 36  
**Callbacks**: 0x80080af8 (31-33), 0x80080b60 (34-36)  
**Category**: Interactive Objects  
**Count**: 6 types in 2 callback groups

---

## Overview

Types 31-36 represent interactive object variants, split into two groups of 3 types each.

**Group A** (31-33): Callback 0x80080af8  
**Group B** (34-36): Callback 0x80080b60

---

## Group A: Types 31, 32, 33

**Shared Callback**: 0x80080af8

**Likely Behaviors**:
- Switch variants (color, size, or timing)
- Destructible block variants
- Bounce pad variants

**Differences**: Sprite ID and/or parameters

### Possible Variants

**Type 31**: Standard version  
**Type 32**: Alternate appearance  
**Type 33**: Special variation

**Implementation**: Same base behavior, different visuals

---

## Group B: Types 34, 35, 36

**Shared Callback**: 0x80080b60

**Likely Behaviors**:
- Different object family than Group A
- Could be doors, gates, or barriers
- Could be collectible containers

**Differences**: Sprite ID and timing/behavior parameters

### Possible Variants

**Type 34**: Small version  
**Type 35**: Medium version  
**Type 36**: Large version

**Or**:

**Type 34**: Red variant  
**Type 35**: Blue variant  
**Type 36**: Green variant

---

## Generic Interactive Object Pattern

```c
void InteractiveObject_Tick(Entity* object) {
    // Check player interaction
    if (PlayerNearby(player, object)) {
        if (PlayerPressesButton(BUTTON_ACTION)) {
            // Trigger activation
            object->activated = true;
            
            // Visual feedback
            SetAnimation(object, ANIM_ACTIVATED);
            
            // Sound feedback
            PlaySoundEffect(ACTIVATE_SOUND, pan, 0);
            
            // Trigger effect
            TriggerLevelEvent(object->event_id);
        }
    }
    
    // Update animation
    if (object->activated) {
        // Stay in activated state
        // Or reset after timer
        if (object->reset_timer > 0) {
            object->reset_timer--;
            if (object->reset_timer == 0) {
                object->activated = false;
                SetAnimation(object, ANIM_IDLE);
            }
        }
    }
}
```

---

## Godot Implementation

```gdscript
extends StaticBody2D
class_name InteractiveObjectVariant

enum VariantType { SWITCH, BLOCK, BOUNCE, DOOR, CONTAINER }

@export var entity_type: int  # 31-36
@export var variant_type: VariantType = VariantType.SWITCH
@export var activated: bool = false
@export var reset_time: float = 0.0  # 0 = permanent
@export var event_id: int = 0

signal activated_signal(event_id: int)

func _on_player_interaction() -> void:
    if activated and reset_time == 0:
        return  # Already activated, permanent
    
    activated = true
    emit_signal("activated_signal", event_id)
    
    # Visual feedback
    play_activation_animation()
    
    # Sound
    AudioManager.play_sound(ACTIVATE_SOUND)
    
    # Reset timer
    if reset_time > 0:
        await get_tree().create_timer(reset_time).timeout
        activated = false
        play_idle_animation()
```

---

**Status**: ⚠️ **Pattern-Based** (50% complete)  
**Coverage**: 6 interactive object types  
**Note**: Variants share behaviors, differ by appearance

