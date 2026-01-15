# Entity Type 48: Platform B (Alternate Platform)

**Entity Type**: 48  
**BLB Type**: 48  
**Callback**: 0x80080e4c  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Interactive Platform  
**Count**: 297 instances (very common!)

---

## Overview

Alternate moving platform type - possibly different path behavior or visual style from Platform A.

**Gameplay Function**: Transportation and platforming

---

## Behavior

**Type**: Moving platform (variant)  
**Movement**: May differ from Platform A  
**Collision**: Solid platform surface  
**Pattern**: Unknown variation (needs analysis)

---

## Possible Differences from Platform A

**Hypothesis 1**: Circular Path
- Moves in circular or elliptical path
- Continuous motion (no reversal)

**Hypothesis 2**: Timed Appearance
- Appears/disappears on timer
- Creates timing challenge

**Hypothesis 3**: Triggered Movement
- Starts moving when player approaches
- One-way travel

**Hypothesis 4**: Different Speed/Size
- Faster movement than Platform A
- Larger or smaller platform

**Note**: Exact difference needs callback analysis

---

## Godot Implementation (Generic)

```gdscript
extends AnimatableBody2D
class_name MovingPlatformB

# Configuration
@export var platform_variant: PlatformVariant = PlatformVariant.CIRCULAR
@export var speed: float = 90.0

enum PlatformVariant {
    CIRCULAR,
    TIMED,
    TRIGGERED,
    FAST
}

func _physics_process(delta: float) -> void:
    match platform_variant:
        PlatformVariant.CIRCULAR:
            update_circular_motion(delta)
        PlatformVariant.TIMED:
            update_timed_appearance(delta)
        PlatformVariant.TRIGGERED:
            update_triggered_motion(delta)
        PlatformVariant.FAST:
            update_fast_linear(delta)

func update_circular_motion(delta: float) -> void:
    # Circular path
    angle += speed * delta * 0.01
    var offset = Vector2(cos(angle), sin(angle)) * radius
    global_position = center + offset
```

---

**Status**: ⚠️ **Needs Analysis**  
**Sprite ID**: ⚠️ Needs extraction  
**Implementation**: Requires callback analysis for exact behavior

