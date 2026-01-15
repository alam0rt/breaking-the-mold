# Entity Types 89, 97-98, 110-111: Variant Group F

**Entity Types**: 89, 97, 98, 110, 111 (5 types)  
**Shared Callback**: 0x8008134c  
**Category**: High-number entity group  
**Status**: ⚠️ Needs callback analysis

---

## Overview

5 types share callback 0x8008134c, scattered across high type numbers.

**Types**: 89, 97, 98, 110, 111  
**Pattern**: Not sequential - scattered placement

**Implication**: Functionally related despite non-sequential numbering

---

## Analysis

### Callback 0x8008134c

**Address Range**: 0x80081xxx (very late callback range)

**Likely Purpose**:
- Level-specific objects
- Special mechanics
- Bonus content
- Theme-specific decorations

**Behavior**: Same base callback applied to 5 different contexts

---

## Type 89: High Group Variant A

**Callback**: 0x8008134c

**Type Number**: 89 (high range)

**Estimated Purpose**: Special object for specific level/theme

---

## Type 97: High Group Variant B

**Callback**: 0x8008134c

**Type Number**: 97 (very high)

**Estimated Purpose**: Same behavior, different context/sprite

---

## Type 98: High Group Variant C

**Callback**: 0x8008134c

**Estimated Purpose**: Third variant of this behavior

---

## Type 110: High Group Variant D

**Callback**: 0x8008134c

**Type Number**: 110 (near max)

**Estimated Purpose**: Fourth variant

---

## Type 111: High Group Variant E

**Callback**: 0x8008134c

**Type Number**: 111 (near max)

**Estimated Purpose**: Fifth variant

---

## Implementation

```gdscript
extends Node2D
class_name HighNumberVariant

@export var entity_type: int  # 89, 97, 98, 110, or 111

# Shared behavior
var behavior_state: String = "default"

func _ready() -> void:
    # Apply variant-specific properties
    match entity_type:
        89: setup_variant_a()
        97: setup_variant_b()
        98: setup_variant_c()
        110: setup_variant_d()
        111: setup_variant_e()

func setup_variant_a() -> void:
    # Type 89 specific
    pass
```

---

**Status**: ⚠️ **Needs Analysis** (40% complete)  
**Pattern**: 5 types sharing callback  
**Scattered**: Non-sequential type numbers  
**Implementation**: Ready with shared behavior pattern

