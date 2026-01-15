# Entity Types 37-41: Mechanisms and Objects

**Entity Types**: 37, 38, 39, 40, 41  
**Callbacks**: 0x80080bc8 (37-38), 0x80080c8c (39), 0x80080cfc (40), 0x80080d6c (41)  
**Category**: Interactive Mechanisms  
**Count**: 5 types

---

## Overview

Types 37-41 represent various interactive mechanisms and objects found in levels.

---

## Type 37-38: Paired Mechanism

**Callback**: 0x80080bc8 (shared)  
**Types**: 37, 38

**Likely Behavior**: Related mechanisms
- **Type 37**: Switch/trigger A
- **Type 38**: Switch/trigger B  
- **Relationship**: May need both activated

**Pattern**: Paired puzzle elements

---

## Type 39: Mechanism C

**Callback**: 0x80080c8c  
**Also Used By**: Type 52

**Likely Behavior**: 
- Moving block
- Conveyor belt
- Elevator/lift
- Rotating platform

**Pattern**: Automated movement object

---

## Type 40: Mechanism D

**Callback**: 0x80080cfc  
**Unique**: Only this type uses this callback

**Likely Behavior**:
- Door or gate
- Barrier that opens/closes
- Timed obstacle

**Pattern**: Blocking object with state

---

## Type 41: Mechanism E

**Callback**: 0x80080d6c  
**Unique**: Only this type

**Likely Behavior**:
- Launcher/catapult
- Spring/bounce mechanism
- Teleporter

**Pattern**: Player transport or launch

---

## Generic Mechanism Behaviors

### Moving Block

```c
void MovingBlock_Tick(Entity* block) {
    // Move along path
    block->position += block->velocity;
    
    // Reverse at endpoints
    if (ReachedPathEnd(block)) {
        block->velocity = -block->velocity;
    }
    
    // Carry player if standing on top
    if (PlayerOnTop(player, block)) {
        player->position += block->velocity;
    }
}
```

### Door/Gate

```c
void Door_Tick(Entity* door) {
    // Check trigger condition
    if (DoorShouldOpen(door)) {
        // Open animation
        if (door->state == CLOSED) {
            door->state = OPENING;
            SetAnimation(door, ANIM_OPEN);
            PlaySoundEffect(DOOR_SOUND, pan, 0);
        }
    } else {
        // Close animation
        if (door->state == OPEN) {
            door->state = CLOSING;
            SetAnimation(door, ANIM_CLOSE);
        }
    }
    
    // Update collision
    door->collision_enabled = (door->state == CLOSED);
}
```

### Launcher

```c
void Launcher_Tick(Entity* launcher) {
    // Check player on launcher
    if (PlayerOnLauncher(player, launcher)) {
        if (launcher->charged) {
            // Launch player
            player->velocity_x = launcher->launch_x;
            player->velocity_y = launcher->launch_y;  // -5.0 to -8.0
            
            // Animation and sound
            SetAnimation(launcher, ANIM_LAUNCH);
            PlaySoundEffect(LAUNCH_SOUND, pan, 0);
            
            // Reset charge
            launcher->charged = false;
            launcher->charge_timer = CHARGE_TIME;
        }
    } else {
        // Recharge
        if (launcher->charge_timer > 0) {
            launcher->charge_timer--;
        } else {
            launcher->charged = true;
        }
    }
}
```

---

## Godot Implementation

```gdscript
extends StaticBody2D
class_name Mechanism

enum MechanismType { MOVING_BLOCK, DOOR, LAUNCHER, CONVEYOR }

@export var type: MechanismType = MechanismType.MOVING_BLOCK
@export var entity_type: int  # 37-41

func _physics_process(delta: float) -> void:
    match type:
        MechanismType.MOVING_BLOCK:
            update_moving_block(delta)
        MechanismType.DOOR:
            update_door(delta)
        MechanismType.LAUNCHER:
            update_launcher(delta)
        MechanismType.CONVEYOR:
            update_conveyor(delta)

func update_moving_block(delta: float) -> void:
    # Simple back-and-forth movement
    position += velocity * delta
    if position.distance_to(start_pos) > path_length:
        velocity = -velocity

func update_launcher(delta: float) -> void:
    # Check for player
    var bodies = get_overlapping_bodies()
    for body in bodies:
        if body.is_in_group("player") and charged:
            body.velocity = launch_velocity
            charged = false
            start_recharge_timer()
```

---

**Status**: ⚠️ **Pattern-Based** (50% complete)  
**Coverage**: 5 mechanism types  
**Implementation**: Ready with pattern variations

