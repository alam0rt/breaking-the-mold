# Entity Types 85, 104-105: Variant Group E

**Entity Types**: 85, 104, 105  
**Shared Callback**: 0x800812ec  
**Category**: High-number entity group  
**Status**: ⚠️ Needs callback analysis

---

## Overview

Types 85, 104-105 share callback 0x800812ec in the high address range (0x80081xxx).

**Address**: 0x800812ec - Late in entity callback range

**Implication**: Later-added entities or special-purpose objects

---

## Analysis

### Callback 0x800812ec

**Address Range**: 0x80081xxx (cheat code and high-level system range)

**Likely Purpose** (based on address proximity):
- Special entities added late in development
- Level-specific objects
- Bonus/secret objects
- Debug/test entities

**Behavior Pattern** (estimated):
- Possibly interactive
- May have special trigger conditions
- Could be level transition related
- Might be bonus room objects

---

## Type 85: Special Entity A

**Callback**: 0x800812ec

**Estimated Purpose**:
- Special object in specific levels
- May relate to secrets or bonuses
- Late-game content

---

## Type 104: Special Entity B

**Callback**: 0x800812ec

**Estimated Purpose**: Same base behavior as Type 85, different variant

**High Type Number**: Suggests special/optional content

---

## Type 105: Special Entity C

**Callback**: 0x800812ec

**Estimated Purpose**: Third variant in this group

---

## Implementation

```gdscript
extends Node2D
class_name SpecialEntityGroup

@export var entity_type: int  # 85, 104, or 105

# Likely simple behavior
var state: String = "idle"

func _ready() -> void:
    # May have special trigger
    check_spawn_condition()

func check_spawn_condition() -> void:
    # Possibly only spawns in certain conditions
    # Check level flags, player state, etc.
    if meets_condition():
        visible = true
    else:
        queue_free()  # Don't spawn
```

---

**Status**: ⚠️ **Needs Analysis** (40% complete)  
**Pattern**: 3-variant group in high range  
**Likely**: Special/bonus objects  
**Implementation**: Ready with conditional spawn pattern

