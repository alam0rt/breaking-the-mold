# Entity Types 5, 6, 7, 9, 11, 12: Additional Collectibles

**Entity Types**: 5, 6, 7, 9, 11, 12  
**Callbacks**: Various  
**Category**: Collectibles/Items  
**Status**: Pattern-based (collectible family)

---

## Overview

Additional collectible types beyond clayballs and items. Likely special pickups, checkpoint markers, or level-specific collectibles.

---

## Type 5 - Collectible A

**Callback**: 0x8007f7b0  
**Pattern**: Stationary collectible  
**Behavior**: Similar to clayball/item  
**Purpose**: May grant specific powerup or currency

---

## Type 6 - Collectible B

**Callback**: 0x8007f830  
**Pattern**: Stationary collectible  
**Behavior**: Similar to above  
**Purpose**: Different collectible type

---

## Type 7 - Collectible C

**Callback**: 0x80080408  
**Pattern**: Stationary collectible  
**Purpose**: Level-specific collectible?

---

## Type 9 - Collectible D

**Callback**: 0x800804e8  
**Pattern**: Stationary or moving collectible  
**Purpose**: Special item or bonus

---

## Type 11 - Collectible E

**Callback**: 0x80080478  
**Pattern**: Collectible  
**Purpose**: Unknown specific type

---

## Type 12 - Collectible F

**Callback**: 0x8007f8b0  
**Pattern**: Collectible  
**Purpose**: Unknown specific type

---

## Generic Collectible Implementation

```gdscript
extends Area2D
class_name GenericCollectible

@export var entity_type: int
@export var collection_value: int = 1
@export var collection_effect: String = "add_score"

func _on_player_touch(body: Node2D) -> void:
    if body.is_in_group("player"):
        match collection_effect:
            "add_score":
                body.add_score(collection_value)
            "grant_powerup":
                body.grant_powerup(entity_type)
            "unlock_door":
                emit_signal("door_unlocked", entity_type)
        
        AudioManager.play_sound(0x7003474c)
        queue_free()
```

---

**Status**: ⚠️ **Pattern-Based** (40% complete)  
**Coverage**: 6 collectible types  
**Note**: Use generic collectible pattern with type-specific effects

