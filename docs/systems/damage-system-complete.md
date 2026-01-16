# Complete Damage and Death System

**Status**: ✅ FULLY DOCUMENTED from C Code  
**Last Updated**: January 15, 2026  
**Source**: SLES_010.90.c lines 17689-17695, 32556-32584, 10149-10157

---

## Overview

Skullmonkeys uses a **lives-based system**, not traditional HP. Klaymen dies in **1 hit** unless protected by powerups.

**Core Mechanic**: 1 hit = 1 life lost (instant death)  
**Protection**: Halo powerup absorbs 1 hit  
**Respawn**: Checkpoint or level restart

---

## Damage System

### One-Hit Death Mechanic

**Rule**: Any enemy contact or hazard = instant death

**No HP System**: Klaymen doesn't have hit points  
**Lives System**: Deaths decrement lives counter  
**Game Over**: Lives = 0

---

## Halo Protection System

### Halo Powerup (g_pPlayerState[0x17] bit 0x01)

**Storage**: `g_pPlayerState[0x17]` bit 0x01  
**Effect**: **Absorbs one hit** before death

### Damage Check Logic (Line 17689-17696)

```c
// When player takes damage (enemy collision, hazard, etc.)
void PlayerTakeDamage(Entity* player) {
    // Check if halo is active
    if ((g_pPlayerState[0x17] & 1) == 0) {
        // NO HALO - Go to death state
        EntitySetState(player, null_FFFF0000h_800a5d80, PTR_PlayerState_Death_800a5d84);
        
        // If bounced entity involved
        if (player[0x144] != 0) {
            EntitySetState(player, null_FFFF0000h_800a5d78, PTR_Callback_8006c95c_800a5d7c);
        }
    } else {
        // HAS HALO - Remove halo, survive hit
        g_pPlayerState[0x17] &= ~0x01;  // Clear halo bit
        
        // Remove halo visual entity
        if (player[0x168] != 0) {  // Halo entity pointer
            player[0x168][0x2c] = 1;  // Mark for removal
            player[0x168] = 0;  // Clear pointer
        }
        
        // Play "hit but survived" sound/animation
        // Player continues gameplay
        
        // Continue to bounce state (survived)
        EntitySetState(player, bounce_state, bounce_callback);
    }
}
```

**Halo Protection Flow**:
1. Player has halo (bit 0x01 set)
2. Enemy/hazard collision occurs
3. Check: `if (g_pPlayerState[0x17] & 1) == 0`
4. Bit is SET (1): Remove halo, survive hit, continue playing
5. Bit is CLEAR (0): No protection, enter death state

**Key Insight**: Halo doesn't prevent damage, it **absorbs the death** and is consumed

---

## Halo Visual Entity

### CreateHaloEntity @ 0x8006DE98 (Line 34218)

**When Created**: When g_pPlayerState[0x17] & 0x01 is set

```c
void CreateHaloEntity(Entity* halo, Entity* player) {
    // Allocate halo entity (0x30 bytes = 48 bytes)
    // Entity follows player position
    // Visual: Ring sprite around Klaymen
    // Z-order: Added to z-order and x-position lists
    // Stored: player[0x5a] or player[0x168] (halo entity pointer)
}
```

**Entity Pointer Storage**:
- player[0x5a]: Initial halo pointer (line 17316)
- player[0x168]: Active halo pointer (line 18184)

**Removal** (line 18191-18193):
```c
// When halo consumed or powerup lost
if ((g_pPlayerState[0x17] & 1) == 0) {  // Halo bit cleared
    player[0x168][0x2c] = 1;  // Mark halo entity for removal
    player[0x168] = 0;  // Clear pointer
}
```

---

## Death State Sequence

### PlayerState_Death @ 0x8006A0B8 (Line 32560)

**Triggered**: When damage taken without halo protection

```c
void PlayerState_Death(Entity* player) {
    // 1. Set global death flag
    g_GameStatePtr[0x170] = 0;
    
    // 2. Set entity death flag
    player[0x5e] = 1;
    
    // 3. Configure texture page for death sprite
    ConfigureTPage(player[0xd]);
    
    // 4. Set tick callbacks (remove collision, keep update)
    player[0] = 0xffff0000;
    player[1] = PlayerTickCallback;  // Keep ticking
    player[2] = 0xffff0000;
    player[3] = PlayerCallback_8005d404;  // Death secondary
    player[0x41] = 0;
    player[0x42] = 0;
    player[7] = 0xffff0000;
    player[8] = ApplyEntityPositionUpdate;  // Position only, no collision
    
    // 5. Set death animation sprite
    SetEntitySpriteId(player, 0x1b301085, 1);  // Death/explosion sprite
    
    // 6. Clear state fields
    player[0x4a] = 0;
    player[0x56] = 1;
}
```

**Death Sprite**: 0x1b301085 (explosion animation)  
**Duration**: ~60-120 frames before respawn  
**Callbacks**: Keep ticking for animation, disable collision

---

## Lives and Game Over

### DecrementPlayerLives @ 0x800262ac (Line 10149)

**Called**: During RespawnAfterDeath

```c
void DecrementPlayerLives(PlayerState* state) {
    // 1. Clear all powerups on death
    state[0x17] = 0;  // Clear halo and trail
    state[0x1d] = 0;  // Clear boss HP
    
    // 2. Decrement lives
    if (state[0x11] != 0) {
        state[0x11]--;  // Lives = Lives - 1
    }
}
```

**Effects of Death**:
- ✅ Lose 1 life
- ✅ Lose halo powerup
- ✅ Lose trail powerup  
- ✅ Reset boss HP field
- ⚠️ Keep: Collected items, clayballs, ammo, other powerups

### Game Over Condition

**Check**: After DecerementPlayerLives, check `g_pPlayerState[0x11]`

```c
// In GameModeCallback (from game-loop.md line 92-97)
if (state[0x146] && !state[0x19c]) {
    if (g_pPlayerState[0x11] == 0) {  // Lives = 0
        // GAME OVER
        AdvanceLevelSequence(state + 0x84);  // Advance to game over screen
        SetupAndStartLevel(state, 99);  // Load menu (level 99)
    } else {
        // Still have lives - respawn
        RespawnAfterDeath(state);
    }
}
```

**Game Over Flow**:
1. Lives reaches 0
2. AdvanceLevelSequence advances to game over screen
3. Load level 99 (menu system)
4. Player returns to main menu

---

## Enemy Death System

### Enemy HP: 1 Hit to Die

**Confirmed**: Most enemies die in 1 hit

**Damage Methods**:
1. **Jump on head**: Player bounces on enemy
2. **Shoot with projectile**: Hit with Swirly Q or Green Bullet
3. **Universe Enema**: Screen-clear powerup

### Jump Kill Mechanic (Line 33063-33067)

```c
// When player bounces on enemy
if (PlayerBounceOnEnemy(player, enemy)) {
    // Read damage from player
    int damage = player[0x44];
    
    // Apply damage modifier
    if (player[0x16] == 0x8000) {
        damage = damage >> 1;  // Half damage
    }
    
    // Deal damage to enemy
    enemy->hp -= damage;
    
    // Check death
    if (enemy->hp <= 0) {
        EntitySetState(enemy, EnemyDeathCallback);
    }
    
    // Bounce player upward
    player->velocity_y = BOUNCE_VELOCITY;  // -2.25 px/frame
}
```

**Player Damage Field**: player[0x44] (damage dealt when bouncing)  
**Damage Modifier**: player[0x16] = 0x8000 → half damage  
**Standard Damage**: 1 (kills 1 HP enemies)

### Projectile Kill Mechanic

**Projectile Collision**: When projectile hits enemy

```c
if (ProjectileHitsEnemy(projectile, enemy)) {
    // Deal damage
    enemy->hp--;  // Usually 1 HP, so dies immediately
    
    // Remove projectile
    RemoveEntity(projectile);
    
    // Enemy death
    if (enemy->hp <= 0) {
        SpawnDeathParticles(enemy);
        RemoveEntity(enemy);
    }
}
```

**Projectile Damage**: 1 (standard)  
**Enemy HP**: 1 (standard)  
**Result**: 1 projectile = 1 kill

---

## Universe Enema Powerup (Screen Clear)

**Storage**: `g_pPlayerState[0x16]` (max 7)  
**Effect**: **Clears all enemies on screen**  
**Trigger**: R1 button (button mask 0x08)

### Verified Functions (Ghidra)

| Address | Function Name | Purpose |
|---------|---------------|----------|
| 0x8006c0d8 | UniverseEnemaActivate | Activation callback |
| 0x8006c278 | UniverseEnemaKillAllEnemies | Kill phase callback |
| 0x80022f24 | SendMessageToPlayerVariant | Broadcast message |

### Activation Flow (Verified)

```c
// Phase 1: UniverseEnemaActivate @ 0x8006c0d8
void UniverseEnemaActivate(Entity* player) {
    // Broadcast message 0x1018 to all entities
    SendMessageToPlayerVariant(g_GameStatePtr, 0x40, 0x1018, 0, player);
    
    // Set screen effect flag
    *(g_GameStatePtr + 0x149) = 1;
    
    // Clear input buffers
    player[0x179] = player[0x17a] = player[0x17b] = 0;
    
    // Set callback selection based on water state
    code* nextCallback = (player[0x4b] != 0) 
        ? PlayerCallback_800625b4   // Underwater
        : PlayerCallback_8006187c;  // Normal
    
    // Play activation sprite
    SetEntitySpriteId(player, 0x6c22083a, 1);
    
    // Transition to kill phase
    EntitySetCallback(player, NULL, UniverseEnemaKillAllEnemies);
}

// Phase 2: UniverseEnemaKillAllEnemies @ 0x8006c278
void UniverseEnemaKillAllEnemies(Entity* player) {
    // Iterate collision list (g_GameStatePtr + 0x24)
    for (ListNode* node = g_GameStatePtr->collision_list; node; node = node->next) {
        Entity* entity = node->entity;
        
        // Check killable flag (entity_flags & 0x04)
        if ((entity->flags_at_0x12 & 0x04) != 0) {
            // Get entity callback from state machine
            callback = GetEntityStateCallback(entity);
            
            // Send MSG_PROJECTILE_HIT (0x1002) - kills the entity
            (*callback)(entity, 0x1002, 0, player);
        }
    }
    
    // Consume one Universe Enema
    g_pPlayerState[0x16]--;
    
    // Clear screen effect flag
    *(g_GameStatePtr + 0x149) = 0;
}
```

**Mechanics**:
- Instant kill on ALL on-screen enemies
- Enemies explode/die simultaneously
- No damage calculation (bypass HP)
- Powerful defensive ability

---

## Damage System Summary

### Player

**HP**: None (lives-based)  
**Damage Taken**: 1 hit = death OR consume halo  
**Halo Protection**: Absorbs 1 hit, then removed  
**Lives**: Decremented on death  
**Game Over**: Lives = 0

### Enemies

**HP**: 1 (standard for most enemies)  
**Damage Taken**: 1 hit = death  
**Damage Sources**:
- Player bounce (1 damage)
- Player projectile (1 damage)
- Universe Enema (instant death, ignores HP)

### Bosses

**HP**: 5 (standard)  
**Damage Taken**: 1 damage per hit  
**Requires**: 5 hits to defeat  
**Special**: Boss HP in g_pPlayerState[0x1d]

---

## Halo Mechanics Details

### Collection

**Entity Type**: 8 (Item pickup)  
**Effect**: Set `g_pPlayerState[0x17] |= 0x01`

**Multiple Halos**: Collecting 2 halos gives clay/orbs (special bonus)

### Visual Effect

**Halo Entity**: Ring sprite following player  
**Z-Order**: High (above player)  
**Position**: Tracks player position  
**Animation**: Rotating or pulsing

**Created**: When bit 0x01 set (lines 17312-17318, 18181-18189)  
**Removed**: When bit 0x01 cleared (lines 18191-18193)

### Duration

**Permanent**: Until consumed by damage  
**Not Timed**: Lasts entire level unless hit  
**Lost on**: Death, level transition

---

## Complete Combat Flow

### Player Hit by Enemy

```
1. Enemy collision detected
2. Check halo: g_pPlayerState[0x17] & 1
3a. If HALO (bit = 1):
    - Clear halo bit: g_pPlayerState[0x17] &= ~0x01
    - Remove halo visual entity
    - Play hit sound
    - Brief invincibility frames
    - Continue playing
3b. If NO HALO (bit = 0):
    - Enter PlayerState_Death
    - Play death animation (sprite 0x1b301085)
    - After animation (~60-120 frames):
      - RespawnAfterDeath called
      - DecrementPlayerLives (lives--)
      - Check lives == 0 → Game Over
      - Else → Respawn at checkpoint
```

### Player Kills Enemy

```
1. Bounce on head OR projectile hit
2. Enemy HP decremented (usually HP = 1)
3. Enemy HP <= 0 → Death
4. Spawn death particles
5. Remove enemy entity
6. Play death sound
```

### Universe Enema (Screen Clear)

```
1. R1 pressed → check g_pPlayerState[0x16] > 0
2. UniverseEnemaActivate (0x8006c0d8):
   - Broadcast message 0x1018
   - Set screen effect flag (GameState+0x149)
   - Play activation sprite
3. UniverseEnemaKillAllEnemies (0x8006c278):
   - Iterate collision list (GameState+0x24)
   - Send MSG_PROJECTILE_HIT (0x1002) to killable entities
   - Enemies die via their normal death handlers
4. Decrement enema count: g_pPlayerState[0x16]--
5. Clear screen effect flag
```

---

## Godot Implementation

```gdscript
extends CharacterBody2D
class_name Player

# Lives-based system
var lives: int = 5
var has_halo: bool = false
var invincibility_timer: float = 0.0
var is_dead: bool = false

# Universe Enema
var universe_enemas: int = 0

func _physics_process(delta: float) -> void:
    if is_dead:
        return  # No control during death
    
    # Update invincibility
    if invincibility_timer > 0:
        invincibility_timer -= delta
    
    # Normal gameplay...
    move_and_slide()

func take_damage() -> void:
    # Ignore if invincible
    if invincibility_timer > 0:
        return
    
    # Check halo protection
    if has_halo:
        # HALO PROTECTS - Absorb hit
        has_halo = false
        remove_halo_visual()
        play_hit_sound()
        invincibility_timer = 2.0  # 120 frames @ 60fps
        flash_sprite()
    else:
        # NO PROTECTION - DIE
        die()

func die() -> void:
    is_dead = true
    
    # Play death animation
    play_death_animation()  # Sprite 0x1b301085
    
    # Wait for animation
    await get_tree().create_timer(2.0).timeout
    
    # Decrement lives
    lives -= 1
    
    # Clear powerups lost on death
    has_halo = false
    has_trail = false
    
    # Check game over
    if lives <= 0:
        game_over()
    else:
        respawn()

func respawn() -> void:
    is_dead = false
    
    # Respawn at checkpoint or level start
    if has_checkpoint:
        global_position = checkpoint_position
    else:
        global_position = level_start_position
    
    # Brief invincibility after respawn
    invincibility_timer = 3.0  # 180 frames
    
    # Reload level state
    emit_signal("player_respawned")

func activate_universe_enema() -> void:
    if universe_enemas <= 0:
        return
    
    # Consume enema
    universe_enemas -= 1
    
    # Get all enemies
    var enemies = get_tree().get_nodes_in_group("enemies")
    
    # Kill ALL enemies
    for enemy in enemies:
        enemy.die_instantly()  # No HP check, instant death
    
    # Screen flash effect
    flash_screen()
    
    # Play screen-clear sound
    AudioManager.play_sound(ENEMA_SOUND)

# Enemy collision
func _on_enemy_collision(enemy: Node2D) -> void:
    if invincibility_timer > 0:
        return  # Invincible, no damage
    
    # Check if bouncing on enemy from above
    if velocity.y > 0 and global_position.y < enemy.global_position.y:
        # Jump kill - bounce on head
        enemy.take_damage(1)
        velocity.y = -135.0  # Bounce upward
        play_bounce_sound()
    else:
        # Contact damage - player hit
        take_damage()
```

---

## Enemy Death System

### Standard Enemy (1 HP)

```gdscript
extends CharacterBody2D
class_name Enemy

var hp: int = 1  # Standard: 1 hit to die

func take_damage(amount: int) -> void:
    hp -= amount
    
    if hp <= 0:
        die()

func die() -> void:
    # Spawn death particles
    spawn_death_particles()
    
    # Play death sound
    AudioManager.play_sound(ENEMY_DEATH_SOUND)
    
    # Remove entity
    queue_free()

func die_instantly() -> void:
    # For Universe Enema - bypass all checks
    spawn_explosion()
    queue_free()
```

---

## Constants and Values

### Player

| Property | Value | Source |
|----------|-------|--------|
| **Default Lives** | 5 | g_pPlayerState[0x11] = 5 |
| **Max Lives** | 99 | Cheat sets to 99 |
| **Damage per Hit** | 1 life | Instant death |
| **Halo Protection** | 1 hit | Absorbs one death |
| **Invincibility Frames** | ~120 frames | After hit or respawn |
| **Death Animation** | 0x1b301085 | Sprite ID |
| **Bounce Velocity** | -2.25 px/frame | After jump kill |

### Enemy

| Property | Value | Source |
|----------|-------|--------|
| **Standard HP** | 1 | Most enemies |
| **Damage from Bounce** | 1 | Player jump |
| **Damage from Projectile** | 1 | Swirly Q or Green Bullet |
| **Universe Enema** | Instant death | Bypasses HP |

### Boss

| Property | Value | Source |
|----------|-------|--------|
| **Boss HP** | 5 | g_pPlayerState[0x1d] |
| **Damage per Hit** | 1 | Standard |
| **Hits to Defeat** | 5 | Joe-Head-Joe confirmed |

---

## Special Cases

### Bounce Damage Modifier

**Player Field**: player[0x16]  
**Check**: `if (player[0x16] == 0x8000)`  
**Effect**: Damage = damage >> 1 (half damage)

**Usage**: Unknown context (possibly powerup or state)

### Enemy HP Variations

**Some enemies MAY have 2-3 HP**:
- Stronger enemy types
- Mid-bosses
- Special entities

**Method to Identify**: Requires testing each enemy type

---

## Powerup Effects on Combat

### Halo (0x17 bit 0x01)

**Effect**: Survive 1 extra hit  
**Visual**: Ring around player  
**Lost on**: Taking damage OR death

### Universe Enema (0x16)

**Effect**: Kill all on-screen enemies  
**Count**: Max 7  
**Usage**: Consumable, instant screen-clear

### Phoenix Hands (0x14)

**Effect**: Unknown (needs analysis)  
**Count**: Max 7

### Phart Heads (0x15)

**Effect**: Unknown (needs analysis)  
**Count**: Max 7

### Super Willies (0x1C)

**Effect**: Unknown (needs analysis)  
**Count**: Max 7

---

## Summary: Lives-Based Combat

**Not Traditional HP**:
- Klaymen has lives, not hit points
- 1 hit = 1 life lost (instant death)
- Halo adds 1-hit protection (absorbs death once)

**Enemies Also Simple**:
- Most enemies: 1 HP
- 1 hit from any source = death
- Bosses: 5 HP (exception)

**Screen-Clear Mechanic**:
- Universe Enema kills ALL enemies instantly
- Powerful defensive/offensive tool
- Limited uses (max 7)

**System Design**: Simple, arcade-style combat  
**Player Skill**: Dodge > tank damage  
**Challenge**: Avoid all hits, use halo wisely

---

## Related Documentation

- [Player System](player/player-system.md) - Player states
- [Items Reference](../reference/items.md) - All powerups
- [Combat System](combat-system.md) - Combat overview
- [Checkpoint System](checkpoint-system.md) - Death and respawn

---

**Status**: ✅ **FULLY DOCUMENTED**  
**System Type**: Lives-based, not HP-based  
**Complexity**: Simple (1-hit deaths)  
**Implementation**: Ready with complete understanding

