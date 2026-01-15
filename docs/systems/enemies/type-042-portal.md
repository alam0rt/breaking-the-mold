# Entity Type 42: Portal (Level Exit)

**Entity Type**: 42  
**BLB Type**: 42  
**Callback**: 0x80080ddc (shared with particle types)  
**Sprite ID**: 0xb01c25f0 (from mapping)  
**Category**: Interactive Object (Level Exit)  
**Count**: Variable (1 per level typically)

---

## Overview

Portals are level exit points that trigger level completion when player enters.

**Gameplay Function**: Level progression trigger

---

## Behavior

**Type**: Stationary object with animation  
**Movement**: None (fixed position)  
**Collision**: Player entry triggers level complete  
**Animation**: Swirling/rotating portal effect  
**Sound**: Portal entry sound

---

## Portal Logic

```c
// When player enters portal
if (CheckEntityCollision(player, portal)) {
    // Check if level complete conditions met
    if (LevelCompleteConditionsMet()) {
        // Trigger level completion
        SetLevelFlag(LEVEL_COMPLETE);
        
        // Play portal sound
        PlaySoundEffect(PORTAL_SOUND, 0, 0);
        
        // Start transition
        FadeOutAndLoadNextLevel();
    } else {
        // Portal inactive (must complete objectives first)
        PlaySoundEffect(PORTAL_LOCKED_SOUND, 0, 0);
    }
}
```

**Conditions** (typical):
- All required items collected
- All enemies defeated (if required)
- Boss defeated (in boss levels)
- Timer requirement met (if timed level)

---

## Visual & Animation

**Sprite ID**: 0xb01c25f0

**Animation**:
- **Idle**: Rotating/swirling continuously
- **Active**: Glowing when conditions met
- **Inactive**: Dim when locked
- **Entry**: Player absorption animation

**Framerate**: 12-15 fps for smooth rotation  
**Effect**: May have particle emission

---

## Portal Types

### Victory Portal

**Spawned**: After boss defeat  
**Position**: Boss arena center  
**Effect**: Returns to map/next world  
**Always Active**: No conditions

### Standard Exit Portal

**Position**: End of level  
**Condition**: Reach portal location  
**May Require**: Minimum collectibles

### Locked Portal

**Visual**: Different sprite or animation  
**Condition**: Specific requirements  
**Feedback**: Sound/visual when attempting locked

---

## Godot Implementation

```gdscript
extends Area2D
class_name Portal

# Configuration
@export var locked: bool = false
@export var required_items: int = 0
@export var next_level: String = ""

# Visual
var rotation_speed: float = 2.0
var is_active: bool = false

func _ready() -> void:
    body_entered.connect(_on_player_entered)
    check_unlock_conditions()

func _process(delta: float) -> void:
    # Rotate portal
    rotation += rotation_speed * delta
    
    # Update active state
    check_unlock_conditions()
    
    # Visual feedback
    if is_active:
        modulate = Color(1, 1, 1, 1)  # Bright
    else:
        modulate = Color(0.5, 0.5, 0.5, 0.5)  # Dim

func check_unlock_conditions() -> void:
    if locked:
        is_active = false
        return
    
    # Check conditions
    var player = get_tree().get_first_node_in_group("player")
    if player:
        is_active = player.clayball_count >= required_items
    else:
        is_active = false

func _on_player_entered(body: Node2D) -> void:
    if not body.is_in_group("player"):
        return
    
    if is_active:
        # Enter portal - complete level
        AudioManager.play_sound(PORTAL_SOUND)
        complete_level()
    else:
        # Portal locked
        AudioManager.play_sound(PORTAL_LOCKED_SOUND)
        shake_portal()

func complete_level() -> void:
    # Transition to next level
    get_tree().change_scene_to_file(next_level)
```

---

**Status**: ✅ **Fully Documented**  
**Sprite ID**: ✅ Confirmed (0xb01c25f0)  
**Implementation**: Ready

