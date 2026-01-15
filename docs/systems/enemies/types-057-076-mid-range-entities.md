# Entity Types 57-76: Mid-Range Entities

**Entity Types**: 57-59, 62-76 (17 types)  
**Callbacks**: Various unique (0x8007fdxx - 0x8007ffxx range)  
**Category**: Mid-range entity types  
**Status**: ⚠️ Pattern-based analysis

---

## Overview

17 entity types with unique callbacks in the mid-address range (0x8007fdxx - 0x80080xxx).

**Address Range**: 0x8007fd18 - 0x80080868  
**Pattern**: Sequential addresses suggest related functionality

---

## Types 57-59: Early Mid-Range

**Callbacks**:
- Type 57: 0x8007fd18
- Type 58: 0x8007fd94  
- Type 59: 0x8007fe10

**Address Pattern**: ~100 bytes apart (0x7c, 0x7c increments)

**Likely Purpose**:
- Enemy variants
- Collectible variants
- Level objects

**Behavior Pattern**: Probably simple (collectible, decoration, or basic enemy)

---

## Types 62-66: Core Mid-Range

**Callbacks**:
- Type 62: 0x8007fe8c
- Type 63: 0x8007fefc
- Type 64: 0x8007ff6c
- Type 65: 0x80080f8c
- Type 66: 0x8007ffdc

**Address Pattern**: Clustered in 0x8007fexx-ffxx range

**Likely Purpose**:
- Enemy types
- Interactive objects
- Level-specific entities

**Behavior Pattern**: May include AI (movement, attacks)

---

## Types 67-72: Late Mid-Range

**Callbacks**:
- Type 67: 0x80080050
- Type 68: 0x800800c4
- Type 69: 0x80080788
- Type 70: 0x800807f8
- Type 71: 0x80080fec
- Type 72: 0x80080868

**Address Pattern**: Scattered in 0x80080xxx range

**Likely Purpose**:
- Enemy types (address range matches Types 25-30 enemies)
- Combat entities
- Hazard objects

**Behavior Pattern**: Likely active gameplay entities

---

## Types 75-76: Additional Mid-Range

**Callbacks**:
- Type 75: 0x800808d8
- Type 76: 0x8007f3dc

**Scattered**: Non-sequential

**Likely Purpose**: Special entities

---

## Estimated Behaviors by Address Range

### 0x8007fdxx-fexx Range (Types 57-59, 62-64)

**Proximity**: Near boss callbacks (0x8007fbxx) and Type 61 Sparkle (0x80080718)

**Estimated Behaviors**:
- Visual effects
- Boss-related entities
- Special objects
- Collectibles

**Pattern**: Likely passive or simple behavior

### 0x8007ffxx-80080xxx Range (Types 65-72, 75)

**Proximity**: Enemy callback range (Types 22-30)

**Estimated Behaviors**:
- Enemy variants
- Active gameplay entities
- Hazards
- Combat objects

**Pattern**: Likely include AI or physics

---

## Implementation Strategy

```gdscript
# Generic mid-range entity
extends CharacterBody2D
class_name MidRangeEntity

@export var entity_type: int  # 57-76 range
@export var behavior_pattern: BehaviorType

enum BehaviorType {
    COLLECTIBLE,
    ENEMY_SIMPLE,
    DECORATION,
    INTERACTIVE,
    HAZARD
}

func _physics_process(delta: float) -> void:
    match behavior_pattern:
        BehaviorType.COLLECTIBLE:
            # Stationary, collision detect
            check_player_collision()
        BehaviorType.ENEMY_SIMPLE:
            # Patrol or fly pattern
            update_enemy_ai(delta)
        BehaviorType.DECORATION:
            # Render only
            pass
        BehaviorType.INTERACTIVE:
            # Trigger-based
            check_interaction()
        BehaviorType.HAZARD:
            # Damage on contact
            check_hazard_collision()
```

---

## Prioritization

**Highest Value** (likely enemies):
- Types 67-72 (enemy address range)
- Types 62-65 (possible enemies)

**Medium Value** (likely collectibles/objects):
- Types 57-59, 66, 75-76

**Lower Value** (likely decorations):
- Remaining types

---

**Status**: ⚠️ **Pattern-Based** (50% complete)  
**Count**: 17 types  
**Estimation**: Address-range based  
**Implementation**: Ready with generic patterns  
**Full Documentation**: Requires individual callback analysis (10-15 hours)

