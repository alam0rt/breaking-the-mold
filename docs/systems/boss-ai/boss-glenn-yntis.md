# Boss: Glenn Yntis (GLEN Level)

**Level**: GLEN (Level 15)  
**Boss Flag**: 0x0140 (unusual - not standard 0x2000!)  
**Position**: Mid-game boss  
**Status**: ⚠️ Estimated behavior (needs verification)

---

## Overview

Glenn Yntis is the mid-game boss, appearing halfway through Skullmonkeys.

**Difficulty**: Medium (mid-game challenge)  
**HP**: 5 (likely standard)  
**Multi-Entity**: Likely yes (standard boss structure)

---

## Special Notes

### Unusual Boss Flag

**Flag**: 0x0140 (instead of 0x2000)

**Implications**:
- May use different spawn system
- May not use standard InitBossEntity
- May have unique mechanics
- Could be mini-boss or special encounter

**Needs Investigation**: Why different flag value?

---

## Expected Behavior (Mid-Game Boss)

### Design Philosophy

Mid-game boss should:
- Have moderate complexity
- Use 2-3 attack types
- Require learned skills from earlier levels
- Have 3 distinct phases
- Take 3-5 minutes to defeat

---

## Likely Attack Patterns

### Pattern 1: Multi-Projectile

**Type**: 2-3 projectiles per attack  
**Speed**: Medium (2.0-2.5 px/frame)  
**Frequency**: Medium (every 1.5-2 seconds)  
**Variation**: May alternate between patterns

### Pattern 2: Area Attack

**Type**: Ground pound, shockwave, or hazard zone  
**Range**: 100-150 pixels  
**Telegraph**: Visual warning (1 second)  
**Avoidance**: Jump or move to safe zone

### Pattern 3: Movement Attack

**Type**: Boss charges or moves toward player  
**Speed**: Fast (3.0-4.0 px/frame)  
**Pattern**: Straight line or predictable path  
**Counter**: Dodge and attack during recovery

---

## Phase Structure (Estimated)

**Phase 1** (HP: 5-4):
- **Attacks**: Pattern 1 only (projectiles)
- **Speed**: Moderate
- **Movement**: Minimal
- **Difficulty**: Medium-Low

**Phase 2** (HP: 3-2):
- **Attacks**: Patterns 1 + 2
- **Speed**: Faster
- **Movement**: More active
- **Difficulty**: Medium

**Phase 3** (HP: 1):
- **Attacks**: All patterns, may combine
- **Speed**: Fast
- **Movement**: Aggressive
- **Difficulty**: Medium-High

---

## Damage Strategy (Speculation)

**Likely Method 1**: Platform mechanics
- Boss creates or uses platforms
- Player must navigate to reach vulnerable point
- Similar to Joe-Head-Joe bounce strategy

**Likely Method 2**: Projectile damage
- Boss has vulnerable spots
- Player shoots when exposed
- May need to destroy parts first

**Likely Method 3**: Environmental
- Use level hazards against boss
- Trigger environmental attacks
- Puzzle-combat hybrid

---

## Visual Design (Speculation)

**Name Analysis**: "Glenn Yntis" sounds like:
- Character or creature name
- Possibly humanoid or anthropomorphic
- May have personality/character

**Expected Appearance**:
- Medium-large sprite
- Distinct visual style
- Uses 9-entity boss structure
- 6 destructible or animated parts

---

## Godot Implementation (Template)

```gdscript
extends Node2D
class_name BossGlennYntis

const MAX_HP = 5

var hp: int = MAX_HP
var current_phase: int = 1
var attack_pattern: int = 0
var attack_timer: float = 0.0

func update_phase() -> void:
    if hp >= 4:
        current_phase = 1
    elif hp >= 2:
        current_phase = 2
    else:
        current_phase = 3

func perform_attack() -> void:
    var player = get_tree().get_first_node_in_group("player")
    
    match current_phase:
        1:
            # Phase 1: Simple projectiles
            spawn_multi_projectile(2)
            attack_timer = 2.0
        
        2:
            # Phase 2: Mix attacks
            if attack_pattern % 2 == 0:
                spawn_multi_projectile(3)
            else:
                perform_area_attack()
            attack_timer = 1.5
            attack_pattern += 1
        
        3:
            # Phase 3: All attacks
            spawn_multi_projectile(4)
            if randf() < 0.5:
                perform_area_attack()
            attack_timer = 1.0
```

---

## Verification Needed

**To Complete**:
- ❌ Exact attack patterns
- ❌ Damage method
- ❌ Boss flag significance (0x0140)
- ❌ Movement behavior
- ❌ Visual design
- ❌ Special mechanics
- ❌ Phase details

**Method**: Play GLEN level boss fight or analyze special spawn system for flag 0x0140

---

## Related Documentation

- [Boss Behaviors](boss-behaviors.md) - Boss system overview
- [Joe-Head-Joe](boss-joe-head-joe.md) - Fully documented reference
- [Level Metadata](../../blb/level-metadata.md) - Boss flags

---

**Status**: ⚠️ **Speculative** (30% complete)  
**Implementation**: Can create mid-difficulty placeholder  
**Priority**: Medium (needs gameplay verification)

