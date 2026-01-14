# Player System Overview

This document describes how the player (Klaymen) works in Skullmonkeys, including powerups, death handling, and key mechanics.

## Player State Structure

The global player state is at `g_pPlayerState` (0x8009DC20). This structure persists across levels and tracks:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x00 | u8 | initialized | State initialized (1) |
| 0x01 | u8 | active | Player is active (1) |
| 0x05 | u8 | total_1ups | Total 1-ups collected (returned on reset) |
| 0x06-0x0F | u8[10] | clayball_flags | Per-stage clayball collection flags |
| 0x10 | u8 | level_complete | Level complete flag |
| 0x11 | u8 | lives | Current lives (default: 5) |
| 0x12 | u8 | unknown_12 | |
| 0x13 | u8 | unknown_13 | |
| 0x14-0x16 | u8[3] | unknown | |
| 0x17 | u8 | **powerup_flags** | Active powerup bitmask |
| 0x18 | u8 | **shrink_mode** | Player is shrunk (mini mode) |
| 0x19-0x1C | u8[4] | unknown | |
| 0x1D | u8 | unknown_1d | Cleared on death |

### Powerup Flags (offset 0x17)

| Bit | Name | Effect |
|-----|------|--------|
| 0x01 | **Halo** | Grants invincibility, creates halo entity following player |
| 0x02 | **Trail** | Creates trail/glide entity following player |
| 0x04+ | (unknown) | Reserved for additional powerups |

## Player Entity Structure

The player entity is 0x1B4 (436) bytes. Created by `CreatePlayerEntity` @ 0x800596a4.

### Key Entity Offsets

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x00-0x04 | ptr[2] | state_callback | Primary state machine |
| 0x04 | ptr | tickCallback | `PlayerTickCallback` (0x8005b414) |
| 0x34 | ptr | spriteData | Pointer to GPU primitive |
| 0x40 | ptr | inputController | Controller input state |
| 0x50-0x58 | s32[3] | scale | X/Y scale (16.16 fixed point) |
| 0x68 | s16 | x_pos | X position (pixels) |
| 0x6A | s16 | y_pos | Y position (pixels) |
| 0x74 | u8 | facing_left | Direction (0=right, 1=left) |
| **0x128** | u8 | invincibility_timer | Damage invincibility countdown |
| **0x144** | u16 | powerup_timer | Powerup effect countdown |
| 0x14C | ptr | linkedEntity | Related entity (HUD, etc) |
| 0x15A-0x15C | u8[3] | rgb_current | Current RGB modulation |
| 0x15D-0x15F | u8[3] | rgb_base | Base RGB (from spawn) |
| 0x160 | s16 | x_velocity | Horizontal velocity |
| 0x162 | s16 | y_velocity | Vertical velocity |
| **0x168** | ptr | halo_entity | Halo powerup child entity |
| **0x16C** | ptr | trail_entity | Trail powerup child entity |
| 0x17D | u8 | rgb_cooldown | RGB update cooldown |
| 0x1AE | u8 | damage_flag | Currently taking damage |
| 0x1AF | u8 | particle_flag | Spawn particles |
| **0x1B0** | u8 | shrink_flag | Shrink mode active |

## State Callbacks

The player uses 4 main state callbacks that dispatch behavior:

| Index | Address | Name | Purpose |
|-------|---------|------|---------|
| 0 | 0x80066ce0 | `PlayerStateCallback_0` | Normal gameplay (walking, jumping) |
| 1 | 0x8006a310 | `PlayerStateCallback_1` | Facing left idle |
| 2 | 0x8006864c | `PlayerStateCallback_2` | Hit/damage state |
| 3 | 0x8006ae0c | `PlayerStateCallback_3` | Unknown state |

### PlayerStateCallback_0 (Normal Gameplay)

Sets up movement handlers:
- `PlayerCallback_8005bad0` - Primary update
- `FUN_8005c1c4` - Secondary update  
- `PlayerCallback_8005f834` - Input handler
- `PlayerCallback_8006120c` or `PlayerCallback_80061934` - Horizontal movement

### PlayerStateCallback_2 (Hit/Damage)

Triggered when player takes damage:
- Sets sprite to damage animation (0x388110)
- Applies knockback velocity
- Sets up `PlayerCallback_800650c4` for recovery

## Death & Respawn

### Death Trigger

Death is triggered by `Callback_80069ef4` when:
- Falling off screen
- Hit by hazard/enemy (when no invincibility)
- Tile collision with instant-death attribute

### Death State

```c
void Callback_80069ef4(Entity* player) {
    // Clear level control flags
    g_GameStatePtr[0x170] = 0;
    player[0x5e] = 1;  // Mark as dead
    
    // Play death sound
    PlaySoundEffect(0x4810c2c4, 0xa0, 0);
    
    // Set death animation sprite
    SetEntitySpriteId(player, 0x1e28e0d4, 1);
    
    // Set scale to 3x (death explosion effect)
    player[0x14] = 0x30000;
    player[0x15] = 0x30000;
    player[0x16] = 0x10000;
    player[0x17] = 0x10000;
    
    // Set GameState death pending flag
    g_GameStatePtr[0x144] = 1;
    
    // Disable all callbacks
    player[0x41] = 0;  // No input handler
    player[0x42] = 0;
    player[7] = 0;     // No movement
    player[8] = 0;
}
```

### Respawn Process

`RespawnAfterDeath` @ 0x8007cfc0:

1. Stop all audio (`StopAllSPUVoices`)
2. Clear rendering (`ClearOrderingTables`, `DrawSync`)
3. Fade to black
4. Restore checkpoint entities if checkpoint was reached
5. Decrement lives (`DecrementPlayerLives`)
6. Reload level state
7. Respawn player at checkpoint or level start

```c
void DecrementPlayerLives(PlayerState* state) {
    state[0x17] = 0;  // Clear powerup flags
    state[0x1D] = 0;  // Clear unknown flag
    if (state[0x11] > 0) {
        state[0x11]--;  // Decrement lives
    }
}
```

## Powerups

### Halo Powerup (Invincibility)

**Activation**: `g_pPlayerState[0x17] |= 1`

Created by `FUN_8006de98` (CreateHaloEntity):
- Allocates 0x30 bytes
- Follows player position
- Plays halo sound effect (0xe0880448)
- Stored at player+0x168

**Behavior in PlayerTickCallback**:
```c
if (g_pPlayerState[0x17] & 1) {
    if (player->halo_entity == NULL) {
        player->halo_entity = CreateHaloEntity();
    }
} else if (player->halo_entity != NULL) {
    DestroyEntity(player->halo_entity);
    player->halo_entity = NULL;
}
```

### Trail Powerup

**Activation**: `g_pPlayerState[0x17] |= 2`

Created by `FUN_8006e1d8` (CreateTrailEntity):
- Allocates 0x110 bytes
- Follows player with offset
- Stored at player+0x16C

### Shrink Mode

**Activation**: `g_pPlayerState[0x18] = 1`

Effects:
- Scale reduced to 0x8000 (half size)
- Bounding box shrinks to {-5, -10, 10, 10}
- Can access smaller passages

**Scale transition in tick**:
```c
if (player->shrink_flag) {
    if (player->scale > 0x4000) {
        player->scale -= 0x1000;  // Shrink gradually
    }
} else {
    if (player->scale < 0x10000) {
        player->scale += 0x1000;  // Grow back
    }
}
```

## Collision System

### Entity Collision (`CheckEntityCollision` @ 0x800226f8)

Collision uses **type masks** for filtering:

| Mask | Purpose |
|------|---------|
| 2 | **Clayball fast path** - checks player directly at GameState+0x2c |
| Other | Iterates collision queue at GameState+0x24 |

### Collision Callbacks

When collision detected:
1. Check bounding box overlap (`FUN_8001b3f0`)
2. Invoke target entity's state callback
3. Pass message type (e.g., 0x1000 = COLLECTED)

### Clayball Collection

When player touches clayball (type 2):
1. Clayball tick calls `FUN_8001b47c(clayball, 2, 0x1000, 1)`
2. `CheckEntityCollision` special case: check player at GameState+0x2c
3. On hit: Clear collision flag, notify GameState (message 3)
4. Clayball disappears, score increments

### Enemy Damage

When enemy contacts player (if no invincibility):
1. Check `player->invincibility_timer == 0`
2. Set damage state via `EntitySetState`
3. Apply knockback
4. Start invincibility countdown
5. Flash RGB (damage flash effect)

## Input Handling

Input controller at entity+0x100:

```c
struct InputState {
    u16 buttons_held;     // Currently pressed
    u16 buttons_pressed;  // Just pressed this frame
    u16 buttons_released; // Just released
    u8  demo_mode;        // Playback/recording flags
    // ...
};
```

### PSX Button Masks

| Mask | Button | Action |
|------|--------|--------|
| 0x0010 | Up | Look up / climb |
| 0x0020 | Right | Move right |
| 0x0040 | Down | Duck / climb down |
| 0x0080 | Left | Move left |
| 0x2000 | Circle | (unused?) |
| 0x4000 | X | Jump |
| 0x8000 | Square | Shoot |

## Level Types

Player entity varies by level type:

| Type | Creator Function | Entity Size |
|------|------------------|-------------|
| Normal | `CreatePlayerEntity` (0x800596a4) | 0x1B4 |
| Glide | `CreateGlidePlayerEntity` (0x8006edb8) | - |
| Soar | `CreateSoarPlayerEntity` (0x80070d68) | 0x128 |
| Runn | `CreateRunnPlayerEntity` (0x80073934) | - |
| Finn | `CreateFinnPlayerEntity` (0x80074100) | - |
| Boss | `CreateBossPlayerEntity` (0x80078200) | - |

## Key Functions Summary

| Address | Name | Purpose |
|---------|------|---------|
| 0x800596a4 | CreatePlayerEntity | Create normal player |
| 0x8005b414 | PlayerTickCallback | Main per-frame update |
| 0x8005a914 | PlayerProcessTileCollision | Tile attribute handling |
| 0x800226f8 | CheckEntityCollision | Entity collision detection |
| 0x80069ef4 | (death state) | Enter death state |
| 0x8006d910 | (debug death?) | Debug/cheat death handler |
| 0x8007cfc0 | RespawnAfterDeath | Respawn after death |
| 0x800262ac | DecrementPlayerLives | Decrement lives counter |
| 0x800260d0 | initPlayerState | Initialize player state (new game) |
| 0x80026164 | ResetPlayerCollectibles | Reset collectible tracking |
| 0x8006de98 | (create halo) | Create halo powerup entity |
| 0x8006e1d8 | (create trail) | Create trail powerup entity |
| 0x80066ce0 | PlayerStateCallback_0 | Normal gameplay state |
| 0x8006864c | PlayerStateCallback_2 | Damage/hit state |
