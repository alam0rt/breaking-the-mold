# Boss: Shriney Guard (MEGA Level)

**Level**: MEGA (Level 5)  
**Boss Flag**: 0x2000  
**Position**: First boss (tutorial)  
**Status**: ⚠️ Estimated behavior (needs verification)

---

## Overview

Shriney Guard is the first boss encounter in Skullmonkeys, serving as a tutorial for boss fight mechanics.

**Difficulty**: Tutorial (easiest boss)  
**HP**: 5 (standard)  
**Multi-Entity**: Yes (9 entities like all bosses)

---

## Expected Behavior (Tutorial Boss)

### Design Philosophy

As the first boss, Shriney Guard should:
- Have simple, predictable attack patterns
- Provide clear visual telegraphs
- Have longer recovery windows
- Use fewer simultaneous attacks
- Teach boss fight mechanics to player

---

## Likely Attack Patterns

### Pattern 1: Simple Projectile

**Type**: Single straight-line projectile  
**Speed**: Slow (1.5-2.0 px/frame)  
**Frequency**: Low (every 2-3 seconds)  
**Telegraph**: Clear wind-up animation

**Purpose**: Teaches dodging

### Pattern 2: Stationary Vulnerability Window

**Behavior**: Boss remains in one position between attacks  
**Vulnerability**: Player can attack during idle periods  
**Damage Method**: Likely jump-on-head or projectile hit

**Purpose**: Teaches basic boss damage mechanics

### Pattern 3: Two Phases Only

**Phase 1** (HP: 5-3):
- Slow attack rate
- Single projectile type
- Stationary or minimal movement

**Phase 2** (HP: 2-1):
- Slightly faster attacks
- May add second attack type
- Still relatively easy

**Purpose**: Intro to phase transitions

---

## Combat Strategy (Estimated)

### Damage Method

**Likely Option A**: Jump on head
- Boss lowers head periodically
- Player jumps to reach
- Simple timing challenge

**Likely Option B**: Projectile hits
- Shoot boss with player projectiles
- Boss has vulnerable points
- No complex mechanics needed

**Likely Option C**: Environmental
- Use level elements to damage boss
- Teaches creative combat

### Victory Condition

**HP**: Reduce from 5 → 0  
**Time**: ~2-3 minutes (short for tutorial)  
**Continues**: Player progresses to next level

---

## Visual Design (Speculation)

**Name**: "Shriney Guard" suggests:
- Guardian or sentinel theme
- Possibly armored or mechanical
- Blocking passage through MEGA level

**Appearance**:
- Large sprite (uses standard boss multi-entity)
- 6 parts (arms, armor pieces, or decorative elements)
- Symmetrical design likely

---

## Godot Implementation (Template)

```gdscript
extends Node2D
class_name BossShrineyGuard

const MAX_HP = 5

var hp: int = MAX_HP
var current_phase: int = 1
var attack_timer: float = 0.0
var invincibility_timer: float = 0.0

func _ready() -> void:
    initialize_boss()

func _process(delta: float) -> void:
    update_phase()
    update_invincibility(delta)
    update_attacks(delta)

func update_phase() -> void:
    if hp >= 3:
        current_phase = 1
    else:
        current_phase = 2

func update_attacks(delta: float) -> void:
    if invincibility_timer > 0:
        return
    
    attack_timer -= delta
    if attack_timer <= 0:
        perform_attack()
        
        # Tutorial timing (slow)
        attack_timer = 2.5 if current_phase == 1 else 2.0

func perform_attack() -> void:
    # Simple projectile attack
    var player = get_tree().get_first_node_in_group("player")
    if player:
        spawn_projectile_toward(player.global_position)

func take_damage(amount: int) -> void:
    if invincibility_timer > 0:
        return
    
    hp -= amount
    invincibility_timer = 1.5  # Longer for tutorial
    
    flash_sprite()
    
    if hp <= 0:
        defeat()
```

---

## Verification Needed

**To Complete**:
- ❌ Exact attack patterns (requires gameplay or code analysis)
- ❌ Damage method (head bounce vs projectiles)
- ❌ Movement behavior
- ❌ Visual appearance
- ❌ Attack timings
- ❌ Phase transition details

**Method**: Play MEGA level boss fight or analyze callback @ 0x8007fba4 (Type 49)

---

## Related Documentation

- [Boss Behaviors](boss-behaviors.md) - All boss documentation
- [Joe-Head-Joe](boss-joe-head-joe.md) - Fully documented boss (reference)
- [Entity Types](../../reference/entity-types.md) - Callback table

---

**Status**: ⚠️ **Estimated** (50% complete)  
**Implementation**: Can create placeholder with simple patterns  
**Verification**: Needs gameplay observation or callback analysis

