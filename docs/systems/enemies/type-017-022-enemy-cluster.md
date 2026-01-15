# Entity Types 17-23: Enemy Cluster

**Entity Types**: 17, 18, 19, 20, 21, 22, 23  
**Callbacks**: Various (0x8007f930 through 0x80080558)  
**Category**: Enemies (Various)  
**Status**: Pattern-based documentation

---

## Overview

Types 17-23 represent a cluster of 7 enemy types with individual callbacks. Likely represent different enemy variants seen across multiple levels.

---

## Individual Type Analysis

### Type 17 (0x8007f930)

**Behavior**: Unknown enemy type  
**Pattern**: Likely ground patrol or flying  
**HP**: 1-2 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use Pattern 1 (Patrol) or Pattern 2 (Flying) from enemy-ai-overview.md

### Type 18 (0x8007f9b0)

**Behavior**: Unknown enemy type  
**Pattern**: Likely ground patrol or flying  
**HP**: 1-2 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use Pattern 1 or 2

### Type 19 (0x8007fa30)

**Behavior**: Unknown enemy type  
**Pattern**: Likely ground patrol or stationary shooter  
**HP**: 1-3 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use Pattern 1 or 3

### Type 20 (0x8007faac)

**Behavior**: Unknown enemy type  
**Pattern**: Likely ground or flying enemy  
**HP**: 2-3 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use appropriate pattern

### Type 21 (0x8007fb28)

**Behavior**: Unknown enemy type  
**Pattern**: Ground, flying, or jumping  
**HP**: 1-2 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use appropriate pattern

### Type 22 (0x80080398)

**Behavior**: Unknown enemy type  
**Pattern**: Likely special behavior  
**HP**: 2-3 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use appropriate pattern

### Type 23 (0x80080558)

**Behavior**: Unknown enemy type  
**Pattern**: Likely ground or flying  
**HP**: 1-2 (estimated)  
**Sprite**: Needs extraction

**Implementation**: Use appropriate pattern

---

## Common Characteristics

**All 7 types likely**:
- Have 1-3 HP
- Use standard enemy AI patterns
- Deal contact damage (1 life)
- Drop items on defeat (sometimes)
- Despawn when off-screen

---

## Implementation Strategy

### Pattern-Based Implementation

```gdscript
# Generic enemy that can use any AI pattern
extends CharacterBody2D
class_name GenericEnemy

enum AIPattern { PATROL, FLYING, SHOOTER, CHASE, HOP }

@export var entity_type: int = 17
@export var ai_pattern: AIPattern = AIPattern.PATROL
@export var hp: int = 2
@export var walk_speed: float = 120.0

func _physics_process(delta: float) -> void:
    match ai_pattern:
        AIPattern.PATROL:
            update_patrol_ai(delta)
        AIPattern.FLYING:
            update_flying_ai(delta)
        AIPattern.SHOOTER:
            update_shooter_ai(delta)
        AIPattern.CHASE:
            update_chase_ai(delta)
        AIPattern.HOP:
            update_hop_ai(delta)
```

### Sprite Assignment

Map entity types 17-23 to sprite IDs once extracted:

```gdscript
const ENEMY_SPRITES = {
    17: 0xXXXXXXXX,  # Extract from C code
    18: 0xXXXXXXXX,
    19: 0xXXXXXXXX,
    20: 0xXXXXXXXX,
    21: 0xXXXXXXXX,
    22: 0xXXXXXXXX,
    23: 0xXXXXXXXX,
}
```

---

## Analysis Method

**To Complete Documentation**:

1. **Extract Sprite IDs** (2 hours):
   - Search C code for each callback function
   - Find InitEntitySprite calls
   - Document sprite IDs

2. **Analyze Callbacks** (4-5 hours):
   - Read each callback function in C code
   - Identify movement patterns
   - Identify state machines
   - Extract constants

3. **Document Individually** (2-3 hours):
   - Create individual docs for unique behaviors
   - Or document as variants in this file

**Total**: 8-10 hours for complete documentation

---

## Placeholder Implementation

**For Now**: Use pattern-based AI until specific behaviors extracted

```gdscript
func get_ai_pattern_for_type(type: int) -> AIPattern:
    match type:
        17, 18, 19:
            return AIPattern.PATROL  # Likely ground enemies
        20, 21:
            return AIPattern.FLYING  # Likely flying enemies
        22:
            return AIPattern.SHOOTER  # Likely stationary
        23:
            return AIPattern.CHASE  # Likely aggressive
        _:
            return AIPattern.PATROL  # Default
```

---

**Status**: ⚠️ **Pattern-Based** (60% complete)  
**Coverage**: 7 enemy types with placeholder patterns  
**Full Documentation**: Needs callback analysis (8-10 hours)

