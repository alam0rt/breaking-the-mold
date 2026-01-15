# Boss AI Behaviors

**Status**: 🔬 In Progress (30% Complete)  
**Last Updated**: January 15, 2026  
**Source**: Boss system analysis + level flag analysis + gameplay observation

---

## Overview

Skullmonkeys features **5 boss encounters** across 26 levels. Each boss uses a multi-entity structure with complex state machines and multi-phase attack patterns.

**Boss Levels**:
1. **MEGA** (Level 5) - Shriney Guard
2. **HEAD** (Level 9) - Joe-Head-Joe ✅ Most documented
3. **GLEN** (Level 15) - Glenn Yntis
4. **WIZZ** (Level 21) - Monkey Mage
5. **KLOG** (Level 24) - Final Boss (Klogg)

---

## Boss System Architecture

### Multi-Entity Boss Structure

**All bosses use 9 entities** (confirmed from InitBossEntity @ 0x80047fb8):

```
Boss Entity Hierarchy:
├─ Main Controller Entity (logic)
├─ Main Sprite (visual body) - 0x181c3854
├─ 6 Boss Parts (attachments) - 0x8818a018 each
├─ Auxiliary Entity (custom sprite)
└─ Final Sprite (additional element) - 0x244655d
```

**Entity Roles**:
- **Main Controller**: AI logic, no visual
- **Main Sprite**: Boss body graphics
- **6 Parts**: Limbs, weapons, or destructible components
- **Auxiliary**: Special attacks or effects
- **Final Sprite**: Additional visual elements

---

## Boss HP System

**Storage**: `g_pPlayerState[0x1D]`

**Boss HP Values**:
- Default: `5 HP` (set in InitBossEntity)
- Decrements on damage
- When HP reaches 0 → boss defeated
- **Joe-Head-Joe**: Requires 5+ hits (bouncing on head)

**HP Display**: Likely shown in HUD during boss fights

**Code Reference** (line 15299):
```c
g_pPlayerState[0x1D] = 5;  // Boss HP initialization
```

**Damage Requirements**:
- Joe-Head-Joe: ~5 hits bouncing on head
- Other bosses: Likely similar (5 HP standard)
- Boss parts: 1-2 HP each (if destructible)

---

## Common Boss Patterns

### Pattern 1: Multi-Phase Combat

**Phases Based on HP**:

```c
if (boss->hp == 5) {
    // Phase 1: Slow attacks, simple patterns
    boss->attack_interval = 120;  // 2 seconds
    boss->movement_speed = 1.0;
} else if (boss->hp >= 3) {
    // Phase 2: Medium difficulty
    boss->attack_interval = 90;   // 1.5 seconds
    boss->movement_speed = 1.5;
} else {
    // Phase 3: Fast & aggressive
    boss->attack_interval = 60;   // 1 second
    boss->movement_speed = 2.0;
    EnableSpecialAttacks(boss);
}
```

### Pattern 2: Destructible Parts

**Boss Parts** (6 entities):
- Each part has own HP (1-2 HP)
- Must destroy parts before damaging main body
- Parts positioned using offset tables

**Part Position Offsets** (from code):
```c
// From line 15339-15340
s16 offset_x = (&null_FF60h_8009b860)[part_index * 2];
s16 offset_y = (&null_FFE0h_8009b862)[part_index * 2];
```

**Part Position Table** @ 0x8009b860 (12 entries × 2 bytes):
- 6 X offsets (typically ±20 to ±80 pixels)
- 6 Y offsets (typically ±10 to ±50 pixels)

### Pattern 3: Attack Cycles

**Typical Boss Attack Cycle**:
1. **Idle** (30-60 frames): Position/prepare
2. **Telegraph** (15-30 frames): Visual warning
3. **Attack** (30-60 frames): Execute attack
4. **Recovery** (15-45 frames): Cooldown
5. **Repeat**

**Attack Types**:
- Projectile spray (spawn multiple projectiles)
- Charge attack (move toward player)
- Summon minions (spawn enemy entities)
- Area attack (hazard zones)
- Special moves (boss-specific)

### Pattern 4: Movement Patterns

**Boss Movement Types**:

**Type A: Fixed Position**
- Boss stays in one location
- Only moves for special attacks
- Player must approach

**Type B: Patrol Movement**
- Boss moves along horizontal path
- Changes direction at boundaries
- Player dodges and attacks

**Type C: Tracking**
- Boss follows player position
- Maintains distance
- Circles or pursues

**Type D: Teleport**
- Boss disappears
- Reappears at new location
- Telegraph with visual effect

---

## Boss Encounter: Joe-Head-Joe (HEAD Level)

**Level**: HEAD (Level 9)  
**Boss Flag**: 0x2048  
**Status**: ✅ **FULLY DOCUMENTED** (3 projectile types, 3 phases, damage strategy)

### Visual Structure

**Main Body**: Large head sprite
**6 Parts**: Facial features or surrounding elements
**Auxiliary**: Attack effects
**Positioning**: Center of arena

### Projectile Types

Joe-Head-Joe uses **3 distinct projectile types**:

**1. Flame Projectiles**
- **Visual**: Fire/flame sprite
- **Behavior**: Aimed shot toward player's current position
- **Movement**: Straight-line trajectory, constant velocity
- **Physics**: No gravity (flies through air)
- **Speed**: 2.0-2.5 px/frame (estimated)
- **Damage**: 1 life lost on contact
- **Counter**: Dodge by moving when fired

**2. Eyeball Projectiles**
- **Visual**: Boss's eyeballs detach and roll
- **Behavior**: Falls from boss, then rolls on ground
- **Movement**: 
  - Phase 1: Falls with gravity until hitting ground
  - Phase 2: Rolls horizontally toward player
- **Physics**: 
  - Initial: Gravity-affected (falls)
  - Ground: Constant roll speed, follows ground contour
- **Speed**: 1.5-2.0 px/frame roll speed (estimated)
- **Damage**: 1 life lost on contact
- **Counter**: Jump over when rolling, or avoid landing zone

**3. Blue Bounce Ball** ⭐ (Critical Mechanic)
- **Visual**: Blue ball sprite
- **Behavior**: Bounces along ground like a basketball
- **Movement**:
  - Falls with gravity: -6.0 px/frame²
  - Bounces on landing: -2.25 px/frame (same as player)
  - Horizontal movement (may move toward or away)
- **Physics**: Full platformer physics (gravity + bounce)
- **Interaction**: **Player can stand on it like a platform**
- **Purpose**: **Primary damage delivery method**
- **Strategy**: 
  1. Wait for boss to spawn blue ball
  2. Jump onto blue ball (acts as moving platform)
  3. Use bounce to reach boss's head height
  4. Land on head to deal 1 damage
  5. Repeat 5 times to defeat boss
- **Dual Nature**: Serves as both obstacle and opportunity

### Combat Phases

**Phase 1** (HP: 5):
- **Behavior**: Stationary, single projectile type
- **Attack Interval**: 120 frames (2 seconds)
- **Projectiles**: Flame shots toward player
- **Blue Ball**: Spawns occasionally for player to bounce
- **Movement**: Minimal or none

**Phase 2** (HP: 3-4):
- **Behavior**: More aggressive, adds eyeball attack
- **Attack Interval**: 90 frames (1.5 seconds)
- **Projectiles**: 
  - Flames (aimed)
  - Eyeballs (roll on ground)
- **Blue Ball**: Spawns more frequently
- **Movement**: May bob or move slightly

**Phase 3** (HP: 1-2):
- **Behavior**: Desperate, all attack types
- **Attack Interval**: 60 frames (1 second)
- **Projectiles**:
  - Flames (rapid fire)
  - Eyeballs (multiple at once)
  - Blue balls (multiple simultaneously)
- **Movement**: More erratic, harder to predict
- **Difficulty**: Must dodge multiple threats while waiting for blue ball

### Attack Patterns

**Pattern 1: Flame Projectile**
- **Projectile**: Flame that travels toward Klaymen (player)
- **Behavior**: Aimed shot, travels in straight line
- **Speed**: 2.0-2.5 px/frame (estimated)
- **Uses**: SpawnProjectileEntity with angle toward player
- **Damage**: 1 life lost on contact
- **Frequency**: Primary attack, used in all phases

**Pattern 2: Eyeball Roll**
- **Projectile**: Shoots out eyeballs that roll on the ground
- **Behavior**: Falls with gravity, then rolls horizontally toward player
- **Physics**: 
  - Initial velocity downward (gravity-affected)
  - Lands on ground
  - Rolls at constant speed toward player
- **Speed**: 1.5-2.0 px/frame roll speed (estimated)
- **Damage**: 1 life lost on contact
- **Frequency**: Mid-to-late phases

**Pattern 3: Blue Bounce Ball**
- **Projectile**: Blue ball that serves as platform
- **Behavior**: Bounces along ground toward player
- **Physics**:
  - Gravity-affected (bounces on landing)
  - Bounce velocity: Similar to player bounce (-2.25 px/frame)
  - Horizontal movement toward/away from player
- **Interaction**: **Player can jump on it to bounce onto boss's head**
- **Damage Opportunity**: Bouncing onto head is primary damage method
- **Frequency**: Spawned regularly, provides damage window

**Pattern 4: Combined Attacks** (Phase 3 likely)
- May fire multiple projectile types simultaneously
- Creates complex dodging scenarios
- Blue ball provides both threat and opportunity

### Vulnerability & Damage Strategy

**Primary Damage Method**: **Bounce on blue ball to reach head**

**Damage Strategy**:
1. Boss spawns blue bounce ball
2. Player jumps onto blue ball (bounces upward)
3. Player lands on boss's head while airborne
4. Deals 1 damage to boss
5. Repeat 5 times to defeat (5 HP total)

**Alternative Damage** (if exists):
- May be able to hit boss directly with projectiles
- Likely less effective than bounce strategy
- Blue ball provides intended damage method

**Invincibility**:
- During attack telegraphs
- During special move animations
- ~30-60 frames after taking damage
- Boss may lower head between attack cycles

**Damage Windows**:
- When blue ball is spawned (use it to bounce)
- Brief windows between attacks
- After eyeball attack completes

---

## Boss Encounter: Shriney Guard (MEGA Level)

**Level**: MEGA (Level 5)  
**Boss Flag**: 0x2000  
**Status**: ⚠️ Partial documentation

### Known Information

**Position**: Early game (first boss)
**Difficulty**: Tutorial boss (easier patterns)
**Attack Complexity**: Simple, predictable
**HP**: 5 (standard)

### Likely Patterns

Based on early-game positioning:
- **Stationary or slow movement**
- **Simple attack patterns**
- **Clear telegraphing**
- **Fewer attacks per cycle**
- **Longer recovery windows**

### To Document

- Exact attack patterns
- Movement behavior
- Phase transitions
- Special attacks (if any)

---

## Boss Encounter: Glenn Yntis (GLEN Level)

**Level**: GLEN (Level 15)  
**Boss Flag**: 0x0140 (⚠️ unusual flag)  
**Status**: ⚠️ Minimal documentation

### Known Information

**Position**: Mid-game boss
**Flag Note**: 0x0140 instead of 0x2000 (may use different spawn system)
**HP**: Likely 5 (standard)

### To Document

- Boss visual appearance
- Attack patterns
- Movement behavior
- Special mechanics
- Phase changes

---

## Boss Encounter: Monkey Mage (WIZZ Level)

**Level**: WIZZ (Level 21)  
**Boss Flag**: 0x0040  
**Status**: ⚠️ Minimal documentation

### Known Information

**Position**: Late-game boss
**Difficulty**: High complexity expected
**HP**: Likely 5 (standard)

### To Document

- Magic attack patterns
- Teleportation behavior
- Summon mechanics
- Phase transitions
- Special abilities

---

## Boss Encounter: Klogg (KLOG Level)

**Level**: KLOG (Level 24)  
**Boss Flag**: 0x0400  
**Status**: ⚠️ Minimal documentation

### Known Information

**Position**: Final boss
**Difficulty**: Highest complexity
**HP**: Possibly higher than 5
**Multi-Phase**: Likely 3+ distinct phases

### Expected Patterns

Based on final boss conventions:
- **Multiple attack types**
- **Complex movement patterns**
- **Minion spawning**
- **Environmental hazards**
- **Dramatic phase transitions**
- **Longer battle duration**

### To Document

- All attack patterns
- Phase transition triggers
- Special mechanics
- Difficulty curve
- Victory conditions

---

## Boss Sprite IDs

**Main Boss Sprite**: 0x181c3854  
**Boss Parts**: 0x8818a018 (all 6 parts use same sprite)  
**Final Sprite**: 0x244655d

**Note**: All bosses appear to use the same sprite IDs. Visual differences may come from:
- Animation sequences
- Palette swaps
- Part positioning
- Auxiliary sprite customization

---

## Boss Collision System

### Collision Masks

**Main Controller**: No collision (logic only)
**Main Sprite**: `mask = 0x0002` (enemy layer)
**Parts**: `mask = 0x0002` (each part separately)

### Damage Handling

```c
// When player attacks boss part
if (CheckCollision(player_projectile, boss_part)) {
    boss_part->hp--;
    
    if (boss_part->hp == 0) {
        RemoveEntity(boss_part);  // Destroy part
        boss->parts_destroyed++;
        
        if (boss->parts_destroyed >= 6) {
            boss->vulnerable = true;  // Can now damage main body
        }
    }
}

// When player attacks main body (after parts destroyed)
if (boss->vulnerable && CheckCollision(player_projectile, boss_main)) {
    g_pPlayerState[0x1D]--;  // Decrement boss HP
    
    if (g_pPlayerState[0x1D] == 0) {
        BossDefeat(boss);
    } else {
        CheckPhaseTransition(boss);  // May enter new phase
    }
}
```

---

## Boss Arena Design

### Common Arena Features

**Boundaries**:
- Fixed horizontal bounds
- No scrolling during fight
- Player contained in arena

**Platform Layout**:
- Central platform (player starting position)
- Side platforms (dodge areas)
- May have hazards (lava, spikes)

**Visual Elements**:
- Background layers (dramatic atmosphere)
- Special lighting
- Environmental animations

---

## Boss Defeat Sequence

**When boss HP reaches 0**:

```c
void BossDefeat(Entity* boss) {
    // 1. Stop all boss attacks
    boss->attack_enabled = false;
    
    // 2. Play defeat animation
    SetAnimation(boss->main_sprite, ANIM_DEATH);
    
    // 3. Destroy all parts
    for (int i = 0; i < 6; i++) {
        if (boss->parts[i] != NULL) {
            SpawnDeathParticles(boss->parts[i]);
            RemoveEntity(boss->parts[i]);
        }
    }
    
    // 4. Spawn victory items
    SpawnVictoryPortal(boss->x_position, boss->y_position);
    
    // 5. Play victory music
    PlayMusic(MUSIC_VICTORY);
    
    // 6. Set level complete flag
    SetLevelFlag(LEVEL_COMPLETE);
    
    // 7. Remove boss entity
    RemoveEntity(boss);
}
```

---

## Implementation Notes

### Joe-Head-Joe Projectile Implementation

```gdscript
# Projectile scenes
const FLAME_PROJECTILE = preload("res://bosses/joe_head_joe/flame.tscn")
const EYEBALL_PROJECTILE = preload("res://bosses/joe_head_joe/eyeball.tscn")
const BLUE_BALL = preload("res://bosses/joe_head_joe/blue_ball.tscn")

func spawn_flame_projectile(target_pos: Vector2) -> void:
    var flame = FLAME_PROJECTILE.instantiate()
    flame.global_position = global_position
    
    # Calculate angle to player
    var direction = (target_pos - global_position).normalized()
    flame.velocity = direction * 150.0  # 2.5 px/frame @ 60fps
    
    add_to_projectile_layer(flame)

func spawn_eyeball_projectile() -> void:
    var eyeball = EYEBALL_PROJECTILE.instantiate()
    eyeball.global_position = global_position
    
    # Eyeball falls first, then rolls
    eyeball.velocity = Vector2(0, 100)  # Initial fall
    eyeball.roll_speed = 120.0  # 2.0 px/frame @ 60fps
    eyeball.roll_toward_player = true
    
    add_to_projectile_layer(eyeball)

func spawn_blue_ball() -> void:
    var ball = BLUE_BALL.instantiate()
    ball.global_position = global_position
    
    # Blue ball is platform + projectile
    ball.set_collision_layer_value(3, true)  # Platform layer
    ball.set_collision_mask_value(1, true)   # Collides with player
    ball.gravity_enabled = true
    ball.bounce_velocity = -135.0  # -2.25 px/frame @ 60fps
    ball.horizontal_speed = 60.0   # Moves slowly
    
    add_to_projectile_layer(ball)

# Eyeball projectile behavior
class Eyeball extends CharacterBody2D:
    var roll_speed: float = 120.0
    var roll_toward_player: bool = true
    var on_ground: bool = false
    
    func _physics_process(delta: float) -> void:
        if not on_ground:
            # Falling phase
            velocity.y += 360.0 * delta  # Gravity
            
            if is_on_floor():
                on_ground = true
                velocity.y = 0
        else:
            # Rolling phase
            var player = get_tree().get_first_node_in_group("player")
            if player and roll_toward_player:
                var direction = sign(player.global_position.x - global_position.x)
                velocity.x = direction * roll_speed
            
            velocity.y += 360.0 * delta  # Still affected by gravity
        
        move_and_slide()

# Blue ball behavior
class BlueBall extends CharacterBody2D:
    var bounce_velocity: float = -135.0  # -2.25 px/frame @ 60fps
    var horizontal_speed: float = 60.0
    var gravity_enabled: bool = true
    
    func _physics_process(delta: float) -> void:
        # Apply gravity
        if gravity_enabled:
            velocity.y += 360.0 * delta  # 6.0 px/frame² @ 60fps
        
        # Bounce on ground
        if is_on_floor() and velocity.y > 0:
            velocity.y = bounce_velocity  # Bounce up
        
        # Horizontal movement
        velocity.x = horizontal_speed
        
        move_and_slide()
    
    # Player can stand on this
    func _on_player_collision(player: Node2D) -> void:
        if player.velocity.y > 0:  # Player landing from above
            # Player bounces on ball
            player.velocity.y = bounce_velocity
```

### For Godot Recreation

```gdscript
extends Node2D
class_name BossJoeHeadJoe

# Boss configuration
@export var max_hp: int = 5
@export var head_height: float = 100.0  # Height of head above origin

# Boss state
var hp: int = max_hp
var current_phase: int = 1
var attack_timer: float = 0.0
var invincibility_timer: float = 0.0

func _process(delta: float) -> void:
    update_phase()
    update_invincibility(delta)
    update_attacks(delta)

func update_phase() -> void:
    if hp == 5:
        current_phase = 1
    elif hp >= 3:
        current_phase = 2
    else:
        current_phase = 3

func update_attacks(delta: float) -> void:
    if invincibility_timer > 0:
        return  # Cannot attack while invincible
    
    attack_timer -= delta
    if attack_timer <= 0:
        perform_attack()
        
        # Set next attack interval based on phase
        match current_phase:
            1: attack_timer = 2.0  # 120 frames
            2: attack_timer = 1.5  # 90 frames
            3: attack_timer = 1.0  # 60 frames

func perform_attack() -> void:
    var player = get_tree().get_first_node_in_group("player")
    
    match current_phase:
        1:
            # Phase 1: Flames + occasional blue ball
            spawn_flame_projectile(player.global_position)
            if randf() < 0.3:  # 30% chance
                spawn_blue_ball()
        
        2:
            # Phase 2: Flames + eyeballs + blue ball
            if randf() < 0.5:
                spawn_flame_projectile(player.global_position)
            else:
                spawn_eyeball_projectile()
            
            if randf() < 0.4:  # 40% chance
                spawn_blue_ball()
        
        3:
            # Phase 3: All attacks, multiple projectiles
            spawn_flame_projectile(player.global_position)
            spawn_eyeball_projectile()
            spawn_eyeball_projectile()  # Double eyeballs
            
            if randf() < 0.6:  # 60% chance for blue ball
                spawn_blue_ball()

func take_damage_from_bounce() -> void:
    if invincibility_timer > 0:
        return  # Still invincible
    
    hp -= 1
    invincibility_timer = 1.0  # 60 frames @ 60fps
    
    # Visual feedback
    flash_sprite()
    
    if hp <= 0:
        defeat()
    else:
        # Check for phase change
        var old_phase = current_phase
        update_phase()
        if current_phase != old_phase:
            on_phase_transition()

func _on_head_area_entered(area: Area2D) -> void:
    # Check if player bounced onto head
    if area.is_in_group("player"):
        var player = area.get_parent()
        if player.velocity.y > 0:  # Player falling/bouncing
            take_damage_from_bounce()
            player.velocity.y = -135.0  # Bounce player back up
```

### Combat Phases

**Phase 1** (HP: 5):

---

## Analysis Status

### Completed (30%)
- ✅ Boss system architecture
- ✅ Multi-entity structure
- ✅ HP system (5 HP standard)
- ✅ Common patterns
- ✅ **Joe-Head-Joe FULLY documented**:
  - ✅ 3 projectile types (flame, eyeball, blue ball)
  - ✅ 3 combat phases
  - ✅ Damage strategy (bounce on blue ball → land on head)
  - ✅ Attack patterns per phase
  - ✅ Godot implementation examples

### In Progress (30%)
- 🔬 Remaining 4 boss behaviors
- 🔬 Boss-specific attack patterns
- 🔬 Movement pattern variations
- 🔬 Boss-specific mechanics

### Not Started (40%)
- ❌ Complete documentation for Shriney Guard, Glenn Yntis, Monkey Mage, Klogg
- ❌ Boss sprite animations
- ❌ Victory condition variations
- ❌ Boss arena mechanics

---

## Next Steps

1. **Document Remaining Bosses** (20-30 hours):
   - Analyze each boss callback function
   - Document attack patterns
   - Extract movement behaviors

2. **Extract Boss Data** (5-10 hours):
   - Part position tables
   - Attack timing values
   - Phase transition triggers

3. **Runtime Analysis** (5-10 hours):
   - Play each boss fight
   - Record attack sequences
   - Document vulnerabilities

**Time to 80% Complete**: ~30-50 hours

---

## Related Documentation

- [Boss System Analysis](boss-system-analysis.md) - Technical implementation details
- [Enemy AI Overview](../enemy-ai-overview.md) - General AI patterns
- [Combat System](../combat-system.md) - Damage mechanics
- [Entity Types](../../reference/entity-types.md) - Boss entity callback table

---

**Status**: ✅ **30% Complete** - Architecture + 1 boss fully documented  
**Joe-Head-Joe**: ✅ **100% documented** (3 projectiles, 3 phases, damage strategy)  
**Blocking Issues**: None - sufficient for implementing Joe-Head-Joe accurately  
**Time to 80%**: ~25-40 hours for remaining 4 bosses

