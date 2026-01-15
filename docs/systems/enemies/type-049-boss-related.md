# Entity Type 49: Boss-Related Entity

**Entity Type**: 49  
**BLB Type**: 49  
**Callback**: 0x8007fba4  
**Category**: Boss System  
**Status**: Boss-specific entity

---

## Overview

Type 49 appears in boss levels and is part of the boss entity system.

**Purpose**: Boss-specific entity (minion, hazard, or boss component)

---

## Possible Roles

### Role 1: Boss Minion

**Behavior**: Enemy spawned by boss during fight  
**AI**: Standard enemy AI (patrol or flying)  
**HP**: 1-2  
**Purpose**: Distract player, increase difficulty

### Role 2: Boss Hazard

**Behavior**: Environmental hazard spawned by boss  
**Movement**: Projectile-like or static  
**Damage**: Contact = 1 life  
**Purpose**: Area denial, force movement

### Role 3: Boss Component

**Behavior**: Part of boss that detaches  
**Connection**: Linked to main boss entity  
**HP**: 1-2  
**Purpose**: Destructible boss part

---

## Boss Context Usage

**From boss documentation**:
- Type 50: Boss Main
- Type 51: Boss Part
- **Type 49**: Boss-related (minion/hazard/component)

**Spawning**: Created during boss fight  
**Lifecycle**: Tied to boss battle  
**Removal**: Destroyed when boss defeated or player defeats it

---

## Implementation

```gdscript
extends CharacterBody2D
class_name BossRelatedEntity

enum BossEntityRole { MINION, HAZARD, COMPONENT }

@export var role: BossEntityRole = BossEntityRole.MINION
@export var boss: Node2D = null
@export var hp: int = 1

func _ready() -> void:
    match role:
        BossEntityRole.MINION:
            setup_as_minion()
        BossEntityRole.HAZARD:
            setup_as_hazard()
        BossEntityRole.COMPONENT:
            setup_as_component()

func setup_as_minion() -> void:
    # Use standard enemy AI
    add_to_group("boss_minions")
    # Patrol or fly pattern

func setup_as_hazard() -> void:
    # Projectile-like behavior
    add_to_group("boss_hazards")
    # Move and damage on contact

func setup_as_component() -> void:
    # Follow boss position
    add_to_group("boss_parts")
    # Destructible, linked to boss

func _on_boss_defeated() -> void:
    # Remove when boss dies
    queue_free()
```

---

**Status**: ⚠️ **Context-Based** (60% complete)  
**Role**: Boss fight support entity  
**Implementation**: Ready (depends on boss)

