# Enemy AI System Overview

**Status**: ✅ VERIFIED via Ghidra decompilation (2026-01-16)  
**Last Updated**: January 16, 2026  
**Source**: Ghidra decompilation + Entity callback table analysis

---

## Overview

Skullmonkeys enemies use a **factory + state machine pattern** where each enemy type has:
1. **Init Callback** - One-time setup when entity spawns
2. **Tick Callback** - Per-frame behavior (set during init)
3. **State Machine** - Multiple behavior states with transitions
4. **Movement Patterns** - Velocity-based or scripted movement
5. **Collision Response** - Interaction with player, tiles, and other entities

**Total Enemy Types**: ~30 distinct enemy behaviors (out of 121 total entity types)

---

## Enemy Entity Architecture

### Entity Lifecycle

```
1. Spawn Trigger
   └─> SpawnOnScreenEntities detects entity in camera view
   
2. Factory Init
   └─> EntityTypeXX_InitCallback called from callback table
       ├─ Allocate entity memory (heap)
       ├─ Set sprite ID
       ├─ Set initial position from Asset 501
       ├─ Initialize AI state
       └─ Register tick callback (entity+0x18)
   
3. Per-Frame Updates
   └─> TickCallback called every frame
       ├─ Update AI state machine
       ├─ Apply movement/velocity
       ├─ Check collisions
       ├─ Update animation
       └─ Trigger state transitions
   
4. Destruction
   └─> Entity removed when:
       ├─ Off-screen (despawn)
       ├─ HP reaches 0
       ├─ Collected by player
       └─ Level transition
```

### Entity Structure (Relevant AI Fields)

**VERIFIED via Ghidra decompilation (2026-01-16):**

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| `+0x00` | 4 | state_high | State machine upper word (typically 0xFFFF0000) |
| `+0x04` | 4 | callback_main | Main update callback (EntityUpdateCallback) |
| `+0x08` | 2 | x_position | X position (for spatial sorting) |
| `+0x0A` | 2 | y_position | Y position (for spatial sorting) |
| `+0x18` | 4 | tick_callback | Per-frame vtable pointer (see EntityTickLoop) |
| `+0x68` | 2 | world_x_pos | **World X position (gameplay/physics)** |
| `+0x6A` | 2 | world_y_pos | **World Y position (gameplay/physics)** |
| `+0x74` | 1 | facing_left | Direction flag (0=right, 1=left) |
| `+0x94` | 2 | x_pos | X position (render) |
| `+0x96` | 2 | y_pos | Y position (render) |
| `+0x100` | 1 | ai_random_value | Random behavior value (0-7 from rand()&7) |
| `+0x101` | 1 | ai_timer | Timer countdown (decremented each frame) |
| `+0x108` | 4 | collision_target | Entity pointer for collision checks |
| `+0x10C` | 1 | hit_counter | Damage counter (max 4 hits) |
| `+0x116` | 2 | sound_cooldown | Sound cooldown timer (set to 0xB4=180 frames) |

**Note**: Entity has THREE position pairs for different purposes:
1. **+0x08/+0x0A**: Used for sorted list organization (spatial queries)
2. **+0x68/+0x6A**: Used for actual world position (gameplay/physics)
3. **+0x94/+0x96**: Used for screen rendering

### Key Enemy AI Functions (Verified via Ghidra)

| Function | Address | Purpose |
|----------|---------|---------|
| `InitGroundPatrolEnemy` | 0x8002ea3c | Initialize ground-based patrol enemies (Clay Keeper, Loud Mouth) |
| `InitEnemyEntityWithAI` | 0x8004f8dc | Generic enemy init with AI state machine |
| `EnemyAIUpdateWithRandomization` | 0x8004fb58 | Per-frame AI tick with randomized behavior |
| `EnemyHitMessageHandler` | 0x8004ddf0 | Handle enemy damage messages and sounds |
| `InitPathFollowingEnemy` | 0x8003c5b8 | Initialize path-following enemies |
| `InitWalkingEnemyEntity` | 0x800514a0 | Walking enemy initialization |
| `EntityType025_GroundPatrolEnemy_Init` | 0x800805c8 | Type 25 factory callback |

### EnemyAIUpdateWithRandomization (0x8004fb58)

**Verified decompiled behavior:**
```c
void EnemyAIUpdateWithRandomization(Entity* entity) {
    // Decrement AI timer each frame
    entity->ai_timer--;
    
    if (entity->ai_timer == 0) {
        // Randomize behavior when timer expires
        entity->ai_random_value = rand() & 7;           // 0-7
        entity->ai_timer = (rand() & 0x1F) + 0x20;      // 32-63 frames
    }
    
    EntityUpdateCallback(entity);  // Base update
    
    // Check off-screen cleanup when no collision target
    if (entity->collision_target == NULL) {
        if (IsEntityOffScreen(entity)) {
            // Notify game state for cleanup (message 3)
            SendMessage(g_GameState, 3, 0, entity);
        }
    }
    
    // Collision check with mask 0x1002
    Entity* collider = CollisionCheckWrapper(entity, 4, 0x1002, 0);
    if (collider == entity->collision_target) {
        SendMessage(g_GameState, 3, 0, entity);
    }
}
```

### EnemyHitMessageHandler (0x8004ddf0)

**Message types handled:**
- `0x1002`: Player projectile hit - decrements boss HP (g_pPlayerState[0x1D]), spawns particle, max 4 hits
- `0x24825800`, `0x308b4800`: Play random sounds from tables at 0x8009bb84/90/9C
- `0x1019`: Play sound with position
- All sound messages respect sound_cooldown timer (0xB4 = 180 frames = 3 seconds)

---

## Common AI Patterns

### Pattern 1: Patrol Movement (Horizontal)

**Used By**: Ground enemies, walking monsters

**Behavior**:
1. Move in one direction at constant speed
2. Detect wall collision or ledge
3. Turn around (reverse direction)
4. Repeat

**Pseudo-code**:
```c
void PatrolAI_Tick(Entity* enemy) {
    // Apply horizontal movement
    if (enemy->facing_left) {
        enemy->push_x = -WALK_SPEED;
    } else {
        enemy->push_x = WALK_SPEED;
    }
    
    // Apply gravity
    enemy->velocity_y += GRAVITY;
    
    // Check wall collision
    if (CheckWallCollision(enemy)) {
        enemy->facing_left = !enemy->facing_left;  // Turn around
    }
    
    // Check ledge (no floor ahead)
    if (!CheckFloorAhead(enemy)) {
        enemy->facing_left = !enemy->facing_left;  // Turn around
    }
}
```

**Movement Constants**:
- Walk Speed: 1.0-2.0 px/frame (typical)
- Gravity: -6.0 px/frame² (same as player)

---

### Pattern 2: Flying Movement (Sine Wave)

**Used By**: Flying enemies, floating objects

**Behavior**:
1. Move horizontally at constant speed
2. Apply sine wave vertical oscillation
3. No gravity applied
4. May track player Y position

**Pseudo-code**:
```c
void FlyingAI_Tick(Entity* enemy) {
    // Horizontal movement
    enemy->x_position += enemy->horizontal_speed;
    
    // Sine wave oscillation
    enemy->oscillation_counter += 2;  // Increment phase
    int offset = sin_table[enemy->oscillation_counter & 0xFF];
    enemy->y_position = enemy->base_y + (offset >> 4);
    
    // Optional: Track player Y
    if (player_y < enemy->y_position) {
        enemy->base_y--;  // Slowly move up
    } else {
        enemy->base_y++;  // Slowly move down
    }
}
```

---

### Pattern 3: Stationary Shooter

**Used By**: Turrets, stationary enemies

**Behavior**:
1. Stay at fixed position
2. Detect player in range
3. Aim toward player
4. Fire projectile at intervals
5. Play attack animation

**Pseudo-code**:
```c
void ShooterAI_Tick(Entity* enemy) {
    enemy->shoot_timer--;
    
    if (enemy->shoot_timer <= 0) {
        // Check if player in range
        int distance = abs(player->x_position - enemy->x_position);
        if (distance < SHOOT_RANGE) {
            // Calculate angle to player
            int angle = CalculateAngle(enemy, player);
            
            // Spawn projectile
            SpawnProjectile(enemy, angle, PROJECTILE_SPEED);
            
            // Reset timer
            enemy->shoot_timer = SHOOT_INTERVAL;
            
            // Play attack animation
            SetAnimation(enemy, ANIM_ATTACK);
        }
    }
}
```

**Typical Values**:
- Shoot Range: 200-300 pixels
- Shoot Interval: 60-120 frames (1-2 seconds)
- Projectile Speed: 2.0-3.0 px/frame

---

### Pattern 4: Chase Player

**Used By**: Aggressive enemies, boss minions

**Behavior**:
1. Detect player position
2. Move toward player
3. Attack when in range
4. May have activation radius

**Pseudo-code**:
```c
void ChaseAI_Tick(Entity* enemy) {
    // Check activation distance
    int distance = DistanceToPlayer(enemy, player);
    
    if (distance < ACTIVATION_RANGE) {
        enemy->state = STATE_CHASE;
        
        // Move toward player
        if (player->x_position < enemy->x_position) {
            enemy->push_x = -CHASE_SPEED;
            enemy->facing_left = 1;
        } else {
            enemy->push_x = CHASE_SPEED;
            enemy->facing_left = 0;
        }
        
        // Attack when close
        if (distance < ATTACK_RANGE) {
            enemy->state = STATE_ATTACK;
            PerformAttack(enemy);
        }
    } else {
        enemy->state = STATE_IDLE;
    }
}
```

---

### Pattern 5: Jump/Hop Movement

**Used By**: Jumping enemies, bouncing creatures

**Behavior**:
1. Wait on ground for interval
2. Jump with fixed velocity
3. Apply gravity during air
4. Land and repeat

**Pseudo-code**:
```c
void HopAI_Tick(Entity* enemy) {
    if (enemy->on_ground) {
        enemy->hop_timer--;
        
        if (enemy->hop_timer <= 0) {
            // Jump
            enemy->velocity_y = JUMP_VELOCITY;
            enemy->on_ground = false;
            enemy->hop_timer = HOP_INTERVAL;
            
            // Optional: Jump toward player
            if (player->x_position < enemy->x_position) {
                enemy->velocity_x = -HOP_HORIZONTAL;
            } else {
                enemy->velocity_x = HOP_HORIZONTAL;
            }
        }
    } else {
        // Apply gravity while airborne
        enemy->velocity_y += GRAVITY;
        
        // Check landing
        if (CheckGroundCollision(enemy)) {
            enemy->on_ground = true;
            enemy->velocity_y = 0;
        }
    }
}
```

**Typical Values**:
- Jump Velocity: -3.0 to -5.0 px/frame
- Hop Interval: 30-90 frames
- Horizontal Speed: 1.0-2.0 px/frame

---

## Enemy State Machines

### Common States

Most enemies use a subset of these states:

| State ID | Name | Behavior |
|----------|------|----------|
| 0 | IDLE | Stationary, waiting |
| 1 | PATROL | Moving along path |
| 2 | CHASE | Pursuing player |
| 3 | ATTACK | Performing attack |
| 4 | HURT | Taking damage (invincibility) |
| 5 | DEATH | Death animation + removal |
| 6 | STUNNED | Temporarily disabled |
| 7 | SPECIAL | Custom behavior |

### State Transitions

```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> PATROL: Timer expires
    PATROL --> CHASE: Player detected
    PATROL --> IDLE: Wall hit
    CHASE --> ATTACK: In range
    CHASE --> PATROL: Player lost
    ATTACK --> CHASE: Attack complete
    PATROL --> HURT: Damaged
    CHASE --> HURT: Damaged
    ATTACK --> HURT: Damaged
    HURT --> PATROL: Recovery complete
    HURT --> DEATH: HP = 0
    DEATH --> [*]
```

---

## Enemy Combat System

### Health Points (HP)

**HP Storage**:
- Enemies typically use `entity[custom_offset]` for HP
- No standard HP offset (varies by enemy type)
- Common range: 1-5 HP for regular enemies
- Boss HP stored in `g_pPlayerState[0x1D]` (Boss HP = 5)

### Damage Calculation

From projectile collision (reference: combat-system.md):

```c
// When player projectile hits enemy
damage = projectile[0x44];  // Base damage

// Check damage modifier
if (enemy[0x16] == 0x8000) {
    damage = damage >> 1;  // Half damage
}

enemy->hp -= damage;

if (enemy->hp <= 0) {
    EntitySetState(enemy, DeathCallback);
}
```

### Invincibility Frames

After taking damage:
- Enemy enters HURT state
- Invincibility timer set (~60-120 frames)
- Visual feedback (flashing sprite)
- Cannot take damage until timer expires

---

## Collision Detection

### Enemy-Player Collision

**Collision Mask**: `entity[0x12]` (u16)

**Common Masks**:
- `0x0002` = Enemy layer (collides with player)
- `0x0004` = Hazard layer (instant damage)
- `0x0008` = Projectile layer (hits projectiles)

**Collision Handler**:
```c
// Check if enemy touches player
if (CheckEntityCollision(enemy, player)) {
    if (player->state == STATE_JUMP && player->velocity_y > 0) {
        // Player bouncing on enemy
        enemy->hp--;
        player->velocity_y = BOUNCE_VELOCITY;
    } else {
        // Enemy damages player
        DamagePlayer(1);  // 1 life lost
    }
}
```

### Enemy-Tile Collision

Enemies use same collision system as player:
- `GetTileAttributeAtPosition` for tile checks
- Solid range (0x01-0x3B) blocks movement
- Special handling for platforms (0x5B)

---

## Enemy Types Classification

### By Behavior

**Ground Patrollers** (10-15 types):
- Walk left/right
- Turn at walls/ledges
- Simple collision
- Examples: Walking enemies, ground monsters

**Flying Enemies** (5-8 types):
- Sine wave movement
- No gravity
- May track player
- Examples: Birds, floating creatures

**Stationary** (3-5 types):
- Fixed position
- Shoot projectiles
- Rotate to face player
- Examples: Turrets, plants

**Jumpers** (3-5 types):
- Hop movement
- Ground-based
- Timed jumps
- Examples: Bouncing creatures

**Chasers** (2-3 types):
- Active tracking
- Pursue player
- Attack when close
- Examples: Aggressive enemies

### By Threat Level

**Low Threat** (1 HP, slow):
- Types: 25, 27, 10
- Behavior: Simple patrol
- Speed: 1.0-1.5 px/frame

**Medium Threat** (2-3 HP, medium):
- Types: 28, 48, various
- Behavior: Patrol + shoot OR chase
- Speed: 1.5-2.5 px/frame

**High Threat** (3-5 HP, fast/complex):
- Types: Boss-related, special enemies
- Behavior: Multiple attack patterns
- Speed: 2.0-3.0 px/frame

---

## Entity Callback Table Reference

From [entity-types.md](../reference/entity-types.md):

**Confirmed Enemy Types**:
- Type 25 @ 0x800805c8 - EnemyA (ground patrol)
- Type 27 @ 0x8007f354 - EnemyB (flying)
- Type 28 @ 0x80080638 - PlatformA (moving platform with collision)
- Type 48 @ 0x80080e4c - PlatformB (alternate platform)
- Type 50 @ 0x8007fc20 - Boss (multi-entity boss)
- Type 51 @ 0x8007fc9c - BossPart (boss components)

**To Document**: ~20 additional enemy types need behavior analysis

---

## Implementation Notes

### For Godot Recreation

```gdscript
extends CharacterBody2D
class_name Enemy

# Enemy configuration
@export var enemy_type: EnemyType
@export var hp: int = 1
@export var walk_speed: float = 60.0  # px/sec
@export var patrol_range: float = 200.0

# AI state
enum State { IDLE, PATROL, CHASE, ATTACK, HURT, DEATH }
var current_state: State = State.PATROL
var facing_left: bool = false
var invincibility_timer: float = 0.0

func _physics_process(delta: float) -> void:
    match current_state:
        State.PATROL:
            update_patrol(delta)
        State.CHASE:
            update_chase(delta)
        State.ATTACK:
            update_attack(delta)
        State.HURT:
            update_hurt(delta)
        State.DEATH:
            update_death(delta)
    
    move_and_slide()

func update_patrol(delta: float) -> void:
    # Simple horizontal patrol
    velocity.x = walk_speed if not facing_left else -walk_speed
    
    # Check walls
    if is_on_wall():
        facing_left = not facing_left
    
    # Check ledges
    if is_on_floor() and not check_floor_ahead():
        facing_left = not facing_left
    
    # Apply gravity
    if not is_on_floor():
        velocity.y += 360 * delta  # 6.0 px/frame @ 60fps

func take_damage(amount: int) -> void:
    if invincibility_timer > 0:
        return
    
    hp -= amount
    if hp <= 0:
        current_state = State.DEATH
    else:
        current_state = State.HURT
        invincibility_timer = 2.0  # 120 frames @ 60fps
```

---

## Analysis Status

### Completed (70%)
- ✅ Enemy lifecycle documented
- ✅ Common AI patterns identified
- ✅ State machine architecture
- ✅ Combat system integration
- ✅ Collision detection patterns
- ✅ **Entity struct AI offsets verified** (2026-01-16)
- ✅ **EnemyAIUpdateWithRandomization decompiled**
- ✅ **EnemyHitMessageHandler decompiled**
- ✅ **InitEnemyEntityWithAI decompiled**
- ✅ **InitGroundPatrolEnemy decompiled**

### In Progress (15%)
- 🔬 Individual enemy type behaviors (20+ remaining)
- 🔬 Specific sprite ID mappings
- 🔬 Movement parameter values

### Not Started (15%)
- ❌ Boss AI state machines (separate doc)
- ❌ Special enemy abilities

---

## Related Documentation

- [Entity Types](../reference/entity-types.md) - Callback table with all 121 types
- [Boss AI](boss-ai/boss-system-analysis.md) - Boss-specific behaviors
- [Combat System](combat-system.md) - Damage and HP mechanics
- [Entity System](entities.md) - General entity architecture
- [Physics Constants](../reference/physics-constants.md) - Movement speeds

---

**Status**: ✅ **70% Complete** - Core AI functions verified, individual enemy types need analysis  
**Blocking Issues**: None - sufficient for placeholder AI implementation  
**Time to 90%**: ~10-15 hours of callback analysis

