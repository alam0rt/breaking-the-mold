# Boss: Monkey Mage (WIZZ Level)

**Level**: WIZZ (Level 21)  
**Boss Flag**: 0x0040  
**Position**: Late-game boss  
**Status**: ⚠️ Estimated behavior (needs verification)

---

## Overview

Monkey Mage is a late-game boss featuring magic-themed attacks and higher complexity.

**Difficulty**: High (late-game)  
**HP**: 5 (likely standard)  
**Multi-Entity**: Yes (standard 9-entity structure)

---

## Expected Behavior (Late-Game Boss)

### Design Philosophy

Late-game boss should:
- Have complex attack patterns
- Use 3-4 different attack types
- Have unpredictable elements
- Require mastery of game mechanics
- Take 5-7 minutes to defeat

---

## Likely Attack Patterns

### Pattern 1: Magic Projectiles

**Type**: Multiple homing or tracking projectiles  
**Speed**: Medium-Fast (2.5-3.0 px/frame)  
**Frequency**: High (every 1-2 seconds)  
**Special**: May home in on player

**Magic Theme Variations**:
- Lightning bolts (fast, straight)
- Fireballs (medium, arcing)
- Energy spheres (slow, tracking)

### Pattern 2: Teleportation

**Behavior**: Boss disappears and reappears  
**Frequency**: Every 10-15 seconds  
**Telegraph**: Visual effect (sparkle, fade)  
**New Position**: Random or pattern-based  
**Invincibility**: During teleport (3-5 seconds)

**Purpose**: Forces player to reposition, increases difficulty

### Pattern 3: Summon Minions

**Type**: Spawns 2-4 enemy entities  
**Enemy Types**: Common enemies (Type 25, 27)  
**Behavior**: Minions use standard AI  
**Boss During**: May be invincible while minions active  
**Clear Condition**: Defeat all minions to resume boss fight

### Pattern 4: Area Hazard

**Type**: Ground hazards or falling objects  
**Coverage**: Large area (150-200 pixels)  
**Duration**: 5-10 seconds  
**Avoidance**: Player must find safe zones  
**Damage**: Contact = 1 life lost

---

## Phase Structure (Estimated)

**Phase 1** (HP: 5-4):
- **Attacks**: Pattern 1 (projectiles)
- **Speed**: Moderate
- **Teleports**: Occasional
- **Difficulty**: Medium

**Phase 2** (HP: 3-2):
- **Attacks**: Patterns 1 + 2 (projectiles + teleport)
- **Speed**: Fast
- **Teleports**: Frequent
- **Minions**: 1-2 spawned
- **Difficulty**: High

**Phase 3** (HP: 1):
- **Attacks**: All patterns
- **Speed**: Very fast
- **Teleports**: Very frequent
- **Minions**: 2-4 spawned
- **Hazards**: Active
- **Difficulty**: Very High

---

## Combat Strategy (Estimated)

### Damage Method

**Likely**: Projectile-based combat
- Hit boss with player projectiles
- Must destroy minions first (if active)
- Vulnerable between teleports
- May need to hit specific weak points

**Window**: Brief vulnerability periods between attacks

### Difficulty Elements

**Challenges**:
- Teleportation makes boss hard to hit
- Minions distract and threaten player
- Area hazards limit movement options
- Multiple simultaneous threats

**Strategy**:
- Stay mobile
- Clear minions quickly
- Attack during teleport recovery
- Memorize teleport locations if patterned

---

## Magic Attacks (Thematic)

**Visual Style**: Magical/mystical effects

**Possible Effects**:
- Sparkles and glows
- Color shifts (purple, blue magical auras)
- Particle trails on projectiles
- Dramatic teleport effects

**Sound Design**:
- Magical "whoosh" sounds
- Teleport sound effect
- Impact sounds for spells

---

## Godot Implementation (Template)

```gdscript
extends Node2D
class_name BossMonkeyMage

const MAX_HP = 5
const TELEPORT_INTERVAL = 12.0  # seconds
const MINION_SPAWN_HP = 3  # HP threshold

var hp: int = MAX_HP
var current_phase: int = 1
var attack_timer: float = 0.0
var teleport_timer: float = TELEPORT_INTERVAL
var is_teleporting: bool = false
var minions_active: int = 0

func _process(delta: float) -> void:
    if is_teleporting:
        return
    
    update_phase()
    update_timers(delta)
    
    # Teleport check
    if teleport_timer <= 0 and current_phase >= 2:
        perform_teleport()
        return
    
    # Attack
    if attack_timer <= 0:
        perform_attack()

func perform_teleport() -> void:
    is_teleporting = true
    invincible = true
    
    # Fade out
    var tween = create_tween()
    tween.tween_property(self, "modulate:a", 0.0, 0.3)
    await tween.finished
    
    # Move to new position
    global_position = get_random_teleport_position()
    
    # Fade in
    tween = create_tween()
    tween.tween_property(self, "modulate:a", 1.0, 0.3)
    await tween.finished
    
    is_teleporting = false
    invincible = false
    teleport_timer = TELEPORT_INTERVAL

func perform_attack() -> void:
    match current_phase:
        1:
            spawn_simple_projectiles(1)
            attack_timer = 2.0
        2:
            spawn_tracking_projectiles(2)
            if hp == 3:
                spawn_minions(2)
            attack_timer = 1.5
        3:
            spawn_tracking_projectiles(3)
            spawn_area_hazard()
            if minions_active < 2:
                spawn_minions(2)
            attack_timer = 1.0

func spawn_minions(count: int) -> void:
    for i in range(count):
        var minion = ENEMY_SCENE.instantiate()
        minion.global_position = global_position + Vector2(randf_range(-100, 100), 0)
        get_parent().add_child(minion)
        minion.tree_exited.connect(_on_minion_defeated)
        minions_active += 1

func _on_minion_defeated() -> void:
    minions_active -= 1
```

---

## Verification Needed

**To Complete**:
- ❌ Actual attack patterns
- ❌ Teleport mechanics
- ❌ Minion spawn system
- ❌ Damage method
- ❌ Phase transitions
- ❌ Visual design

**Method**: Play WIZZ level or analyze callback functions

---

## Related Documentation

- [Boss Behaviors](boss-behaviors.md) - Boss system
- [Enemy AI](../enemy-ai-overview.md) - Minion behaviors
- [Entity Types](../../reference/entity-types.md) - Type 49 callback

---

**Status**: ⚠️ **Speculative** (25% complete)  
**Implementation**: Can create complex placeholder  
**Priority**: Medium (late-game content)

