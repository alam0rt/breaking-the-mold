# Entity Types 106-117: Variant Groups B, C, D

**Entity Types**: 106-108, 112-114, 115-117 (12 types in 3 groups)  
**Shared Callbacks**: 0x8007f0d0, 0x8007f140, 0x8007f1c0  
**Category**: Systematic variant system  
**Status**: ⚠️ Needs callback analysis

---

## Overview

These 12 types form a systematic variant system with Types 86-88:

**Complete System** (16 types total):
- **Group A** (86-88): Callback 0x8007f050
- **Group B** (106-108): Callback 0x8007f0d0  
- **Group C** (112-114): Callback 0x8007f140
- **Group D** (115-117): Callback 0x8007f1c0

**Address Pattern**: 0x80 byte increments (0x8007f050, 0d0, 140, 1c0)

**Implication**: Intentional systematic design - 4 object categories × 3 variants each

---

## Group B: Types 106-108

**Shared Callback**: 0x8007f0d0

### Type 106: Group B Variant 1

**Likely Purpose**: Decorative object category B, variant 1

### Type 107: Group B Variant 2

**Likely Purpose**: Same category, different sprite/size/color

### Type 108: Group B Variant 3

**Likely Purpose**: Same category, third variant

**Pattern**: 3 size variants (small/medium/large) OR 3 color variants

---

## Group C: Types 112-114

**Shared Callback**: 0x8007f140

### Type 112: Group C Variant 1

**Likely Purpose**: Decorative object category C, variant 1

### Type 113: Group C Variant 2

**Likely Purpose**: Same category, second variant

### Type 114: Group C Variant 3

**Likely Purpose**: Same category, third variant

---

## Group D: Types 115-117

**Shared Callback**: 0x8007f1c0

### Type 115: Group D Variant 1

**Likely Purpose**: Decorative object category D, variant 1

### Type 116: Group D Variant 2

**Likely Purpose**: Same category, second variant

### Type 117: Group D Variant 3

**Likely Purpose**: Same category, third variant

---

## Variant System Analysis

### The 16-Variant Pattern

**4 Groups × 3 Variants = 12 types** (86-88, 106-108, 112-114, 115-117)  
**Plus Group A** (documented separately)

**Possible Categories** (4 groups):
1. **Background decorations** (trees, rocks, plants)
2. **Foreground decorations** (flowers, grass, details)
3. **Animated objects** (flags, water, effects)
4. **Level-specific** (themed decorations)

**Variants** (3 per group):
- Size: Small, Medium, Large
- OR Color: Variant A, B, C
- OR Animation: Different cycles

---

## Implementation Pattern

```gdscript
extends Sprite2D
class_name SystematicVariant

# Variant system
enum VariantGroup { GROUP_A, GROUP_B, GROUP_C, GROUP_D }
enum VariantIndex { VARIANT_1, VARIANT_2, VARIANT_3 }

@export var group: VariantGroup = VariantGroup.GROUP_A
@export var variant: VariantIndex = VariantIndex.VARIANT_1

# Entity type mapping
const TYPE_MAP = {
    VariantGroup.GROUP_A: {
        VariantIndex.VARIANT_1: 86,
        VariantIndex.VARIANT_2: 87,
        VariantIndex.VARIANT_3: 88,
    },
    VariantGroup.GROUP_B: {
        VariantIndex.VARIANT_1: 106,
        VariantIndex.VARIANT_2: 107,
        VariantIndex.VARIANT_3: 108,
    },
    VariantGroup.GROUP_C: {
        VariantIndex.VARIANT_1: 112,
        VariantIndex.VARIANT_2: 113,
        VariantIndex.VARIANT_3: 114,
    },
    VariantGroup.GROUP_D: {
        VariantIndex.VARIANT_1: 115,
        VariantIndex.VARIANT_2: 116,
        VariantIndex.VARIANT_3: 117,
    },
}

func _ready() -> void:
    var type = TYPE_MAP[group][variant]
    load_sprite_for_type(type)
```

---

## Differentiation Strategy

**To Determine Exact Purpose**:
1. Extract sprite IDs from callbacks
2. Observe which levels use which types
3. Check visual appearance of sprites
4. Identify pattern (size/color/theme)

**Extraction Method**:
```bash
# Search for InitEntitySprite calls in callbacks
# Extract sprite ID constants
# Map to entity types
```

---

**Status**: ⚠️ **Pattern Identified** (70% complete)  
**Groups**: 3 groups, 9 types total  
**System**: Part of 16-variant decoration system  
**Implementation**: Ready with systematic pattern  
**Full Documentation**: Requires callback analysis and sprite extraction

