# Entity Types 26, 29, 30: Additional Enemies

**Entity Types**: 26, 29, 30  
**Callbacks**: 0x8007f2cc, 0x800806a8, 0x80080a98  
**Category**: Enemies  
**Status**: Pattern-based documentation

---

## Type 26 - Enemy C

**Callback**: 0x8007f2cc  
**Sprite ID**: Unknown  
**Behavior**: Enemy between Types 25 (EnemyA) and 27 (EnemyB)

**Likely Pattern**: Ground patrol or flying variant  
**HP**: 1-2  
**Damage**: 1 life on contact

**Implementation**: Use Pattern 1 (Patrol) or Pattern 2 (Flying)

---

## Type 29 - Enemy D

**Callback**: 0x800806a8  
**Sprite ID**: Unknown  
**Behavior**: Enemy after platforms

**Likely Pattern**: Ground patrol with variation  
**HP**: 2-3  
**Damage**: 1 life on contact

**Possible Variations**:
- Faster movement
- Jump ability
- Shoots occasionally

**Implementation**: Use Pattern 1 with modifications

---

## Type 30 - Enemy E

**Callback**: 0x80080a98  
**Sprite ID**: Unknown  
**Behavior**: Sequential with Type 29

**Likely Pattern**: Flying or jumping enemy  
**HP**: 1-2  
**Damage**: 1 life on contact

**Possible Variations**:
- Sine wave flight
- Hop movement
- Tracking behavior

**Implementation**: Use Pattern 2 (Flying) or Pattern 5 (Hop)

---

## Godot Implementation (Generic)

```gdscript
extends CharacterBody2D

@export var entity_type: int
@export var hp: int = 2
@export var speed: float = 120.0
@export var pattern: String = "patrol"

func _physics_process(delta: float) -> void:
    match pattern:
        "patrol":
            velocity.x = speed if not facing_left else -speed
            if is_on_wall():
                facing_left = not facing_left
        "flying":
            velocity.x = speed if not facing_left else -speed
            position.y = base_y + sin(phase * 0.1) * 16
            phase += 2
        "hop":
            if on_ground and hop_timer <= 0:
                velocity.y = -240
                hop_timer = 2.0
            elif not on_ground:
                velocity.y += 360 * delta
    
    move_and_slide()
```

---

**Status**: ⚠️ **Pattern-Based** (50% complete)  
**Coverage**: 3 enemy types with placeholder AI  
**Full Documentation**: Needs callback analysis

