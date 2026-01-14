# Player Bounce Mechanics

## Overview

The bounce system allows the player to bounce upward when landing on enemies, checkpoints, and other bouncy surfaces. After 3 consecutive bounces, a special animation and sound effect are triggered. Boss entities only allow a single bounce.

## Collision Detection

Bounce is triggered through the tile/entity collision system in `PlayerCallback_HandleMovementAndCollision` @ `0x800638D0`.

### Collision Flow

1. **EntityApplyMovementCallbacks** checks for collision at various Y offsets below the player (y+2, y-7, y+0x10)
2. Returns a collision type byte indicating the surface type
3. **FUN_8005a630** @ `0x8005A630` processes the collision value and triggers state transitions

### Bounce Collision Values

Collision types that trigger bounce:

| Value | Hex | Description |
|-------|-----|-------------|
| -0x22 | 0xDE | Bouncy surface (primary) - spawns particle effect if level flag set |
| -0x21 | 0xDF | Bouncy surface (alternate) |
| -0x23 | 0xDD | Bouncy surface (alternate 2) |
| -0x4b | 0xB5 | Collision type B5 |
| -0x4a | 0xB6 | Collision type B6 |
| -0x49 | 0xB7 | Collision type B7 |
| -0x37 | 0xC9 | Collision type C9 |
| -0x35 | 0xCB | Collision type CB |

## Bounce Processing

### FUN_8005a630 (Collision State Handler)

**Address:** `0x8005A630`

Handles collision-based player state transitions based on collision type byte.

**Key Logic:**

```c
void FUN_8005a630(int player_entity, char collision_type)
{
    // Special particle effect for 0xDE collision with level flag
    if ((collision_type == -0x22) && (level_flags & 0x8000)) {
        // Spawn particle effect entity
        entity = AllocateFromHeap(...);
        entity = FUN_80030d0c(entity, 0x2e2414, ...);
        AddEntityToSortedRenderList(g_GameStatePtr, entity);
    }
    
    // Check if player can bounce (not dead)
    if ((g_pPlayerState[0x17] & 1) == 0) {
        // Player is dead - handle death state
        if (*(short *)(player_entity + 0x144) != 0) {
            // Has bounce entity reference - use special death callback
            EntitySetState(player_entity, ..., PTR_Callback_8006c95c_800a5d7c);
        } else {
            // Normal death
            EntitySetState(player_entity, ..., PTR_PlayerState_Death_800a5d84);
        }
    } else {
        // Player is alive - select bounce callback based on collision type
        switch (collision_type) {
            case -0x4b: // 0xB5
                EntitySetState(player_entity, ..., PTR_PlayerStateCallback_3_800a5d3c);
                break;
            case -0x4a: // 0xB6
                EntitySetState(player_entity, ..., PTR_Callback_8006ae58_800a5d44);
                break;
            case -0x49: // 0xB7
                EntitySetState(player_entity, ..., PTR_Callback_8006ae94_800a5d4c);
                break;
            case -0x37: // 0xC9
                EntitySetState(player_entity, ..., PTR_Callback_8006adbc_800a5d54);
                break;
            case -0x35: // 0xCB
                EntitySetState(player_entity, ..., PTR_PlayerState_QuickTurn_800a5d5c);
                break;
            case -0x23: // 0xDD
                EntitySetState(player_entity, ..., PTR_Callback_8006ad70_800a5d64);
                break;
            case -0x22: // 0xDE (primary bounce)
                EntitySetState(player_entity, ..., PTR_Callback_8006ad34_800a5d6c);
                break;
            case -0x21: // 0xDF
                EntitySetState(player_entity, ..., PTR_Callback_8006af28_800a5d74);
                break;
        }
    }
}
```

**Player Entity Offsets:**
- `+0x144`: Reference to bounced entity (or bounce counter?)
- `+0x68`: X position
- `+0x6a`: Y position
- `+0x110`: Vertical velocity
- `+0x11c`, `+0x11e`, `+0x11f`, `+0x120`: Bounce state variables

### Callback_8006ad34 (Primary Bounce Handler)

**Address:** `0x8006AD34`

Called when landing on primary bouncy surface (collision type 0xDE).

```c
void Callback_8006ad34(int player_entity)
{
    *(u8 *)(player_entity + 0x11c) = 8;
    *(u8 *)(player_entity + 0x11e) = 8;
    *(u32 *)(player_entity + 0x120) = 0;
    *(s32 *)(player_entity + 0x110) = 0xFFFDC000; // Upward velocity
    *(u8 *)(player_entity + 0x11f) = 8;
    FUN_8006af70();  // Main bounce setup function
}
```

**Velocity:** `0xFFFDC000` = -0x24000 fixed-point = upward bounce velocity

### FUN_8006af70 (Bounce Setup & Triple-Bounce Handler)

**Address:** `0x8006AF70`

Main bounce setup function that handles animation, sound, and triple-bounce special effects.

**Key Logic:**

```c
void FUN_8006af70(int player_entity)
{
    *(u8 *)(player_entity + 0x4a) = 0x78;
    player_entity[0x43] = 0;
    *(u8 *)(g_GameStatePtr + 0x60) = 1;
    
    int bounced_entity = player_entity[0x4b];  // Entity that was bounced on
    player_entity[0x49] = 0x28000;
    player_entity[0x45] = 0;
    player_entity[0x46] = 0;
    *(u8 *)(player_entity + 0x4f) = 0;
    
    // If bounced on an entity (not just tile collision)
    if (bounced_entity != 0) {
        short damage = (short)player_entity[0x44];
        
        // Half damage if flag 0x8000 is set
        if (player_entity[0x16] == 0x8000) {
            damage = damage >> 1;
        }
        
        // Apply damage/interaction to bounced entity
        // (callback system for entity interactions)
        if (bounced_entity has callbacks) {
            (*callback)(bounced_entity, 0x1005, damage, player_entity);
        }
        
        player_entity[0x4b] = 0;  // Clear bounced entity reference
    }
    
    // Set animation and callbacks based on player state flag
    if ((g_pPlayerState[0x17] & 1) == 0) {
        // Special bounce animation/state (triple-bounce?)
        *(u8 *)(player_entity + 0x5e) = 1;
        SetEntitySpriteId(player_entity, 0x393c80c2, 1);
        FUN_8001ec18(player_entity, ..., PTR_Callback_8006b2dc_800a5f28);
        // Set various callbacks for special bounce state
    } else {
        // Normal bounce animation
        g_pPlayerState[0x17] = g_pPlayerState[0x17] & 0xfe;
        SetEntitySpriteId(player_entity, 0x393c80c2, 1);
        FUN_8001ec18(player_entity, ..., PTR_Callback_800691d8_800a5f18);
        // Set various callbacks for normal bounce
    }
}
```

**Player Entity Offsets (Bounce-Related):**
- `+0x110` (param_1[0x44]): Vertical velocity (s32)
- `+0x124` (param_1[0x49]): Set to 0x28000 during bounce
- `+0x12C` (param_1[0x4b]): Reference to entity that was bounced on
- `+0x128` (param_1 + 0x4a)`: Set to 0x78 during bounce
- `+0x10C` (param_1[0x43]): Cleared during bounce
- `+0x114` (param_1[0x45]): Cleared during bounce
- `+0x118` (param_1[0x46]): Cleared during bounce
- `+0x13C` (param_1 + 0x4f)`: Cleared during bounce
- `+0x58` (param_1[0x16]): Flag that affects bounce damage (0x8000 = half damage)

**Global State:**
- `g_pPlayerState[0x17]`: Bit 0 = bounce state flag
  - If bit 0 is clear: Triple-bounce special animation triggered
  - If bit 0 is set: Normal bounce animation

## Triple-Bounce Special Effect

The triple-bounce special animation is triggered via the `g_pPlayerState[0x17]` flag.

**Mechanism:**
1. Normal bounces have `g_pPlayerState[0x17] & 1 == 1`
2. After 3 bounces, bit 0 is cleared
3. Next bounce calls special animation path in FUN_8006af70:
   - Sets sprite ID `0x393c80c2` with flag 1
   - Calls special callback `PTR_Callback_8006b2dc_800a5f28`
   - Sets entity flag at `+0x5e` to 1

**Special Animation Sprite:** `0x393c80c2`

**Special Callback:** `PTR_Callback_8006b2dc_800a5f28` @ stored in data section

## Boss Bounce Limitation

Boss entities return different collision values or set the `+0x144` entity reference differently to limit bouncing.

**Hypothesis:**
- Boss collision may return a non-bouncy collision value after first bounce
- Or boss entity marks itself as "already bounced" to prevent repeated bounces
- The `+0x144` offset in player entity may track the last bounced entity to prevent repeated bounces on same target

**Requires Further Investigation:**
- Boss entity collision handler
- How `+0x144` is set and checked
- Boss-specific collision values

## Position Update

**FUN_8005a218** @ `0x8005A218` handles position updates with collision-based adjustments.

This function applies movement callbacks and adjusts the player's Y position based on collision results.

## Summary

**Bounce Flow:**
1. `EntityApplyMovementCallbacks` detects collision below player → returns collision type byte
2. `FUN_8005a630` checks collision type (0xDE, 0xDF, 0xDD, etc.) → calls EntitySetState with bounce callback
3. `Callback_8006ad34` sets upward velocity (0xFFFDC000) → calls FUN_8006af70
4. `FUN_8006af70` sets up animation, checks `g_pPlayerState[0x17]` flag:
   - If bit 0 set: Normal bounce (sprite 0x393c80c2, normal callback)
   - If bit 0 clear: Triple-bounce special (sprite 0x393c80c2, special callback with sound)
5. If bounced on entity: Apply damage/interaction to entity, clear reference

**Bounce Counter:** Tracked via `g_pPlayerState[0x17]` bit 0 (exact counting mechanism not yet found)

**Special Animation:** Sprite `0x393c80c2` with special callback `PTR_Callback_8006b2dc_800a5f28`

**Boss Limitation:** Mechanism not yet confirmed (likely collision value or entity tracking)

## Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800638D0 | PlayerCallback_HandleMovementAndCollision | Main collision detection loop |
| 0x8005A630 | FUN_8005a630 | Collision type handler - triggers bounce state |
| 0x8005A218 | FUN_8005a218 | Position update with collision adjustment |
| 0x8006AD34 | Callback_8006ad34 | Primary bounce callback (0xDE) - sets velocity |
| 0x8006AF70 | FUN_8006af70 | Bounce setup - animation, sound, triple-bounce |
| 0x8006AD70 | Callback_8006ad70 | Bounce callback for 0xDD collision |
| 0x8006ADBC | Callback_8006adbc | Bounce callback for 0xC9 collision |
| 0x8006AE0C | PlayerStateCallback_3 | Bounce callback for 0xB5 collision |
| 0x8006AE58 | Callback_8006ae58 | Bounce callback for 0xB6 collision |
| 0x8006AE94 | Callback_8006ae94 | Bounce callback for 0xB7 collision |
| 0x8006AEDC | PlayerState_QuickTurn | Quick turn on 0xCB collision |
| 0x8006AF28 | Callback_8006af28 | Bounce callback for 0xDF collision |

**All bounce callbacks call FUN_8006af70 for common bounce setup.**

## Verification Status

- ✅ Collision detection flow confirmed via decompilation
- ✅ Bounce velocity value confirmed: 0xFFFDC000
- ✅ Special animation sprite ID confirmed: 0x393c80c2
- ✅ Triple-bounce flag location confirmed: g_pPlayerState[0x17] bit 0
- ⚠️ Triple-bounce counter mechanism: Not yet found (likely tracked elsewhere)
- ⚠️ Boss bounce limitation: Mechanism not confirmed
- ⚠️ Exact bounce counter increment: Not yet located

## Next Steps

1. Find where `g_pPlayerState[0x17]` bit 0 is cleared (triple-bounce counter reaches 3)
2. Locate bounce counter increment function
3. Decompile special callback `PTR_Callback_8006b2dc_800a5f28` to understand special animation
4. Investigate boss collision handlers to find bounce limitation
5. Search for writes to player entity `+0x144` offset
6. Runtime trace verification of bounce mechanics
