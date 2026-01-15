# Entity Types 86-88: Variant Group A

**Entity Types**: 86, 87, 88  
**Shared Callback**: 0x8007f050  
**Category**: Unknown (likely decorations or collectibles)  
**Status**: ⚠️ Needs callback analysis

---

## Overview

Types 86-88 share callback 0x8007f050 and are grouped together sequentially.

**Address Pattern**: 0x8007f050, 0x8007f0d0, 0x8007f140, 0x8007f1c0 (0x80 increments)

**Implication**: Part of a systematic variant system (possibly 12 total variants in 4 groups of 3)

---

## Analysis

### Callback 0x8007f050

**Address Range**: 0x8007f050 in early system functions

**Likely Purpose** (based on address range):
- Decorative objects
- Non-interactive entities
- Background elements
- Collectible variants

**Behavior Pattern** (estimated):
- Stationary or simple animation
- No collision or passive collision
- Render-only or minimal interaction
- Variant distinguished by sprite ID

---

## Type 86: Variant A1

**Callback**: 0x8007f050

**Estimated Purpose**: 
- Decorative object variant 1
- OR collectible type variant 1
- OR background element

**Pattern**: Shares behavior with 87, 88

---

## Type 87: Variant A2

**Callback**: 0x8007f050

**Estimated Purpose**: Same base behavior as Type 86, different sprite

---

## Type 88: Variant A3

**Callback**: 0x8007f050

**Estimated Purpose**: Same base behavior, third variant

---

## Variant Group Pattern

**Types 86-88** (0x8007f050):  
**Types 106-108** (0x8007f0d0):  
**Types 112-114** (0x8007f140):  
**Types 115-117** (0x8007f1c0):

**Pattern**: 4 groups of 3 types = 12 variants total

**Likely System**:
- 4 object categories
- 3 variants per category
- Differ by sprite/size/color

**Possible Categories**:
- Small/Medium/Large sizes
- Red/Blue/Green colors
- Different animation cycles
- Level-specific variants

---

## Implementation

```gdscript
extends Sprite2D
class_name VariantGroupA

@export var entity_type: int  # 86, 87, or 88
@export var variant_index: int = 0  # 0, 1, or 2

# Sprite IDs (need extraction)
const SPRITE_IDS = {
    86: 0xXXXXXXXX,
    87: 0xXXXXXXXX,
    88: 0xXXXXXXXX,
}

func _ready() -> void:
    # Load appropriate sprite
    texture = load_sprite(SPRITE_IDS[entity_type])
    
    # Simple behavior (render only or idle animation)
    if has_animation():
        play("idle")
```

---

**Status**: ⚠️ **Needs Callback Analysis**  
**Pattern**: Part of 12-variant system (4 groups × 3)  
**Implementation**: Ready with generic variant pattern  
**Full Documentation**: Requires C code analysis of callback 0x8007f050

