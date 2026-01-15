# Entity Types 43-44, 53-55: Portal/Particle Variants

**Entity Types**: 43, 44, 53, 54, 55  
**Shared Callback**: 0x80080ddc  
**Category**: Visual Effects / Interactive Objects  
**Status**: ⚠️ Variant analysis (shares with Types 42, 60)

---

## Overview

Types 43-44 and 53-55 share callback 0x80080ddc with:
- **Type 42**: Portal (fully documented)
- **Type 60**: Particle Effect (fully documented)

**Implication**: These are **variants** of portal or particle systems

---

## Analysis Based on Shared Callback

### Callback 0x80080ddc Behavior

From documented Types 42 and 60, this callback handles:
- **Stationary or physics-based entities**
- **Limited lifetime** (particles) OR **persistent** (portals)
- **No AI** - simple behavior
- **Visual effects** or **trigger objects**

---

## Type 43: Portal Variant A

**Callback**: 0x80080ddc (shares with Type 42 Portal)

**Likely Purpose**:
- **Option A**: Different portal type (color, size, destination)
- **Option B**: Particle effect variant
- **Option C**: Trigger zone (invisible)

**Behavior**: Same base as Type 42 (portal) but different sprite or parameters

**Implementation**: Use portal pattern with variant sprite

---

## Type 44: Portal Variant B

**Callback**: 0x80080ddc

**Likely Purpose**: Another portal/particle/trigger variant

**Pattern**: Shares behavior with Types 42, 43

---

## Type 53: Effect/Object Variant C

**Callback**: 0x80080ddc

**Likely Purpose**: Visual effect or trigger object

**Pattern**: Portal/particle family member

---

## Type 54: Effect/Object Variant D

**Callback**: 0x80080ddc

**Likely Purpose**: Visual effect or trigger object

---

## Type 55: Effect/Object Variant E

**Callback**: 0x80080ddc

**Likely Purpose**: Visual effect or trigger object

---

## Common Characteristics

**All 5 types share**:
- ✅ Same callback 0x80080ddc
- ✅ Same base behavior (portal or particle)
- ⚠️ Different sprite IDs (need extraction)
- ⚠️ Different parameters (size, lifetime, purpose)

---

## Implementation Strategy

```gdscript
extends Node2D
class_name PortalParticleVariant

enum VariantType { PORTAL_ALT, PARTICLE_ALT, TRIGGER_ZONE }

@export var entity_type: int  # 43-44, 53-55
@export var variant_type: VariantType = VariantType.PORTAL_ALT

func _ready() -> void:
    match variant_type:
        VariantType.PORTAL_ALT:
            setup_as_portal()
        VariantType.PARTICLE_ALT:
            setup_as_particle()
        VariantType.TRIGGER_ZONE:
            setup_as_trigger()

func setup_as_portal() -> void:
    # Use Type 42 portal behavior
    # Different sprite/color
    pass

func setup_as_particle() -> void:
    # Use Type 60 particle behavior
    # Different effect type
    pass
```

---

## Differentiation

**To Identify Exact Purpose**:
1. Extract sprite ID for each type from callback
2. Observe in gameplay which levels use which type
3. Check entity placement data (Asset 501)

**Likely Differences**:
- **Color**: Red vs blue vs green portal
- **Size**: Small vs large particle
- **Purpose**: Exit vs checkpoint vs bonus portal
- **Effect**: Explosion vs sparkle vs smoke particle

---

**Status**: ⚠️ **Variant Analysis** (60% complete)  
**Shared Callback**: Documented for Types 42, 60  
**Implementation**: Ready with variant pattern  
**Full Documentation**: Needs sprite ID extraction and gameplay observation

