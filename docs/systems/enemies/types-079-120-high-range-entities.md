# Entity Types 79-120: High-Range Entities

**Entity Types**: 79-84, 90-96, 99-103, 109, 119-120 (23 types)  
**Callbacks**: Various unique (0x80080xxx - 0x80081xxx range)  
**Category**: High-number entity types  
**Status**: ⚠️ Pattern-based analysis

---

## Overview

23 entity types with high type numbers (79-120) and unique callbacks.

**Address Range**: 0x80080xxx - 0x80081xxx (late callback range)  
**Type Numbers**: 79-120 (approaching maximum)

**Implication**: Later-added entities, special content, level-specific objects

---

## Types 79-84: Early High Range

**Callbacks**:
- Type 79: 0x8008121c
- Type 80: 0x80080ebc
- Type 81: 0x80080948
- Type 82: 0x8008127c
- Type 83: 0x800809b8
- Type 84: 0x8007f5b0

**Address Range**: 0x8007f5xx - 0x80081xxx (scattered)

**Likely Purpose**:
- Level-specific enemies
- Themed decorations
- Special mechanics
- Bonus content

**Behavior**: Mix of active (enemies) and passive (objects)

---

## Types 90-96: Mid-High Range

**Callbacks**:
- Type 90: 0x80080138
- Type 91: 0x800801b4
- Type 92: 0x80080230
- Type 93: 0x800802ac
- Type 94: 0x80081428
- Type 95: 0x800814a4
- Type 96: 0x8007f638

**Address Pattern**: Mostly 0x80080xxx and 0x80081xxx

**Likely Purpose**:
- Enemy types (address matches enemy range)
- Combat entities
- Interactive objects

**Behavior**: Likely include movement and collision

---

## Types 99-103: Very High Range

**Callbacks**:
- Type 99: 0x8007f4d0
- Type 100: 0x8008105c
- Type 101: 0x800810cc
- Type 102: 0x8008113c
- Type 103: 0x800811ac

**Address Pattern**: 0x8007f4xx - 0x80081xxx

**Likely Purpose**:
- Special entities
- Bonus room objects
- Secret content
- Level-specific mechanics

---

## Types 109, 119-120: Extreme High Range

**Callbacks**:
- Type 109: 0x8007f540
- Type 119: 0x80080a28
- Type 120: 0x8007f6c0

**Type Numbers**: Very high (near 120 maximum)

**Likely Purpose**:
- Last-minute additions
- Special/debug entities
- Secret content
- Rarely-used objects

---

## Address Range Analysis

### 0x8007f4xx-f6xx Range (Types 84, 96, 99, 109, 120)

**Proximity**: Near Type 24 (Special Ammo) and collectibles

**Estimated**: Collectible or powerup variants

### 0x80080xxx Range (Types 79-83, 90-93, 119)

**Proximity**: Enemy callback range

**Estimated**: Enemy types or combat entities

### 0x80081xxx Range (Types 94-95, 100-103)

**Proximity**: Late system functions

**Estimated**: Special mechanics, bonus content

---

## Implementation

```gdscript
extends Node2D
class_name HighRangeEntity

@export var entity_type: int  # 79-120

# Behavior categories
enum Category {
    ENEMY,
    COLLECTIBLE,
    DECORATION,
    SPECIAL_MECHANIC,
    BONUS_CONTENT
}

@export var category: Category = Category.DECORATION

func _ready() -> void:
    # Apply category-based behavior
    match category:
        Category.ENEMY:
            setup_as_enemy()
        Category.COLLECTIBLE:
            setup_as_collectible()
        Category.DECORATION:
            setup_as_decoration()
        Category.SPECIAL_MECHANIC:
            setup_special_behavior()
        Category.BONUS_CONTENT:
            setup_bonus_content()

func setup_as_enemy() -> void:
    # Use standard enemy AI pattern
    add_to_group("enemies")
    # Patrol or flying pattern

func setup_as_collectible() -> void:
    # Use collectible pattern
    add_to_group("collectibles")
    # Stationary with collision

func setup_as_decoration() -> void:
    # Render only
    add_to_group("decorations")

func setup_special_behavior() -> void:
    # Level-specific mechanics
    pass

func setup_bonus_content() -> void:
    # Secret/bonus objects
    # May have spawn conditions
    pass
```

---

## Estimated Distribution

**Enemies** (Types 79-83, 90-93): ~10 types  
**Collectibles** (Types 84, 99, 109): ~5 types  
**Decorations** (Types 96, 119-120): ~3 types  
**Special** (Types 94-95, 100-103): ~5 types

**Confidence**: 50-70% - Based on address range patterns

---

## Prioritization for Analysis

**If Limited Time**:
1. Types 79-83, 90-93 (likely enemies) - Highest gameplay impact
2. Types 84, 99, 109 (likely collectibles) - Medium impact
3. Types 94-95, 100-103 (special mechanics) - Medium impact
4. Types 96, 119-120 (likely decorations) - Lowest impact

**Time**: ~12-15 hours for all 23 types (~30-45 min each)

---

**Status**: ⚠️ **Pattern-Based** (50% complete)  
**Count**: 23 types  
**Estimation**: Address-range and proximity based  
**Implementation**: Ready with category-based patterns  
**Full Documentation**: Requires individual callback analysis

