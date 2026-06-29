---
title: "Complete Collectibles System Reference"
category: systems/collectibles
tags: [systems, collectibles]
---

# Complete Collectibles System Reference

**Date**: January 20, 2026  
**Status**: VERIFIED - All collectibles traced to PlayerState

---

## Summary

All collectibles modify fields in `PlayerState` (30 bytes, g_pPlayerState @ PAL runtime).
Collection triggers a HUD message via `GameState+0x14C` (hud_entity_ptr).

---

## PlayerState Field Map (Collectible-Related)

| Offset | Field | Max | Collectible | Add Function | HUD Msg |
|--------|-------|-----|-------------|--------------|---------|
| 0x11 | `lives` | 99 | Extra Life / 100 Orbs | `AddPlayerLives` | 0x1013,2 |
| 0x12 | `orb_count` | 99→0 | Clay Orbs | `AddPlayerOrbs` | 0x1013,1 |
| 0x13 | `swirly_q_count` | 20 | Swirly Q | `AddSwirlys` | 0x1013,3 |
| 0x14 | `phoenix_hands` | 7 | Phoenix Hand | `AddPhoenixHands` | 0x1013,6 |
| 0x15 | `phart_heads` | 7 | Phart Head | `AddPhartHeads` | 0x1013,7 |
| 0x16 | `universe_enemas` | 7 | Universe Enema | `AddUniverseEnemas` | 0x1013,8 |
| 0x17 | `powerup_flags` | - | Shield/Halo (0x01 via first orb), Yellow Bird/Glide (0x02 via Type 75) | direct set | 0x1013,3 |
| 0x18 | `shrink_mode` | 1 | Shrink Pickup | direct set | - |
| 0x19 | `icon_1970_count` | 3 | 1970 Icon | `Add1970Icons` | 0x1013,4 |
| 0x1a | `hamster_count` | 3 | Hamster Shields | `AdjustPlayerStats` | 0x1013,5 |
| 0x1b | `total_swirly_qs` | 48 | Swirly Q (total) | `AdjustPlayerStats` | 0x1013,5 |
| 0x1c | `super_willies` | 7 | Super Willie | `AddSuperWillies` | 0x1013,9 |

---

## Complete Collectible List

### 1. Clay Orbs (Orb Count)
- **Callback**: `CollectibleOrbTickCallback` @ 0x8002d768
- **Effect**: `AddPlayerOrbs(+5)` OR grants Halo if none (first orb special)
- **PlayerState**: `orb_count` (+0x12)
- **Max**: 99 (wraps to 0, grants +1 life)
- **Sound**: 0x4a806042
- **Special**: First orb collected sets `powerup_flags |= 0x01` (Halo)

### 2. Single Clay
- **Callback**: `CollectibleClaySingleTickCallback` @ 0x8002de40
- **Effect**: `AddPlayerOrbs(+1)`
- **PlayerState**: `orb_count` (+0x12)
- **Sound**: 0x4a806042

### 3. Yellow Bird / Glidey Bird (Glide Ability)
- **Callback**: `CollectibleYellowBirdTickCallback` @ 0x8002dac8
- **Effect**: `powerup_flags |= 0x02` (Yellow Bird / Glide bit)
- **PlayerState**: `powerup_flags` (+0x17), bit 0x02
- **Sound**: 0x6082c120
- **Notes**: Grants glide ability (hold X while falling)
- **IMPORTANT**: This is NOT the shield "Halo". The shield is bit 0x01, granted by first Clay Orb.

### 4. Extra Life
- **Callback**: `CollectibleExtraLifeTickCallback` @ 0x8002e870
- **Effect**: `AddPlayerLives(+1)`
- **PlayerState**: `lives` (+0x11)
- **Max**: 99
- **Sound**: 0x428254e2
- **Marks entity type 7** (prevents re-collection)

### 5. Phoenix Hand (L1 Weapon)
- **Callback**: `CollectiblePhoenixHandTickCallback` @ 0x8002e9b0
- **Effect**: `AddPhoenixHands(+1)`
- **PlayerState**: `phoenix_hands` (+0x14)
- **Max**: 7
- **Sound**: 0x44c26454
- **Use**: Homing bird attack

### 6. Phart Head (L2 Weapon)
- **Callback**: `CollectiblePhartHeadTickCallback` @ 0x8002ec3c  
- **Init**: `EntityType025_PhartHeadCollectible_Init` @ 0x800805c8
- **Effect**: `AddPhartHeads(+1)`
- **PlayerState**: `phart_heads` (+0x15)
- **Max**: 7
- **Sound**: 0xc0034658
- **Sprites**: Parent 0x8c510186, Child 0xa9240484
- **Animation**: Bobbing via sine wave on frame counter
- **Use**: Ghostly scout clone

### 7. Universe Enema (R1 Weapon)
- **Callback**: `CollectibleUniverseEnemaTickCallback` @ 0x8002eea0
- **Effect**: `AddUniverseEnemas(+1)`
- **PlayerState**: `universe_enemas` (+0x16)
- **Max**: 7
- **Sound**: 0xc88a346a
- **Animation**: Pulsing scale effect via frame counter
- **Use**: Screen-clear bomb (R1 button)

#### Universe Enema Activation (R1)
When player presses R1 with `g_pPlayerState[0x16] > 0`:
1. `UniverseEnemaActivate` @ 0x8006c0d8 broadcasts message 0x1018 (freeze entities)
2. Sets screen effect flag at GameState+0x149
3. Plays activation sprite 0x6c22083a
4. `UniverseEnemaKillAllEnemies` @ 0x8006c278 iterates collision list:
   - Sends message 0x1002 to entities with killable flag (entity+0x12 & 0x04)
   - Decrements `g_pPlayerState[0x16]`
5. Clears screen effect flag

### 8. Super Willie (R2 Weapon)
- **Callback**: `CollectibleSuperWillieTickCallback` @ 0x8002f7e4
- **Effect**: `AddSuperWillies(+1)`
- **PlayerState**: `super_willies` (+0x1c)
- **Max**: 7
- **Sound**: 0xe48744c4
- **Use**: Auto-collect all items on screen

### 9. 1970 Icon
- **Callback**: `Collectible1970IconTickCallback` @ 0x8002f4b4
- **Effect**: `Add1970Icons(+1)`
- **PlayerState**: `icon_1970_count` (+0x19)
- **Max**: 3 (collect 3 for bonus)
- **Sound**: 0x408a6461 (normal), 0x428a6465 (when 3rd collected)
- **Spawns**: Sparkle particles every 8 frames

### 10. Hamster Shield (Type 70)
- **Callback**: `CollectibleHamsterShieldTickCallback` @ 0x8002f690
- **Effect**: `AdjustPlayerStats(+1)` - increments BOTH fields:
  - `hamster_count` (+0x1a) - orbiting hamster shields, max 3 (kill enemies on contact)
  - `total count` (+0x1b) - global count, max 48 for secret ending tracking
- **PlayerState**: Both +0x1a and +0x1b
- **Sound**: 0x42906465 (normal), 0xc2906565 (when 3rd hamster - shields full)
- **HUD**: Message 0x1013, param 5
- **Entity Type**: 70 (Layer 1, BLB type 5)
- **Sprite**: 0x80e85ea0
- **Manual**: "Three furry hamsters circle Klaymen, protecting him by killing the first three enemies they touch"

### 11. Swirly Q (Type 2) - Bonus Portal Counter
- **Callback**: (uses `AddSwirlys`)
- **Effect**: `AddSwirlys(+1)`
- **PlayerState**: `swirly_q_count` (+0x13)
- **Max**: 20
- **Special**: 3 opens bonus portal via `SpawnSwirlPortalEntity`
- **Note**: Different from Type 70 - this increments the portal counter (+0x13), not hamster shields

---

## Trigger Collectibles (No PlayerState Change)

These collectibles send messages to other entities instead of modifying PlayerState:

### TriggerCollectible100C
- **Callback**: `TriggerCollectible100CTickCallback` @ 0x8002f03c
- **Effect**: Sends message 0x100C to player entity (GameState+0x2C)
- **Sound**: 0x62000441
- **Use**: Triggers events/switches

### TriggerCollectible100D
- **Callback**: `TriggerCollectible100DTickCallback` @ 0x8002f274
- **Effect**: Sends message 0x100D to player entity (GameState+0x2C)
- **Sound**: 0x40864668
- **Use**: Triggers events/switches

---

## Special Behaviors

### Orb First-Collection Behavior
`CollectibleOrbTickCallback` has special logic:
```c
if ((g_pPlayerState[0x17] & 1) == 0) {
    // First orb: grant Halo instead of +5 orbs
    g_pPlayerState[0x17] |= 0x01;
} else {
    AddPlayerOrbs(+5);
}
```

### 100-Orb Life Bonus
`AddPlayerOrbs` auto-grants life at 100:
```c
if (orb_count > 99) {
    orb_count = orb_count - 100 + 0x9D;  // Wraps to remainder
    AddPlayerLives(+1);
    PlaySoundEffect(0x428254e2);  // 1up sound
}
```

### Swirly Q Portal Trigger
When `swirly_q_count >= 3`, game calls `SpawnSwirlPortalEntity` @ 0x8005ad54 to create bonus portal.

### Secret Ending
`total_swirly_qs` at +0x1b tracks ALL Swirly Qs across the entire game. At 48 (0x30), secret ending unlocks.

---

## Collectible Entity Common Pattern

All collectibles follow this pattern:
```c
void CollectibleXTickCallback(entity, param2, param3) {
    DecorEntityTickWithOffscreenCheck(entity, ...);  // Update position
    
    if (entity->collected_flag || CheckEntityBoxCollision(entity, 2, param3)) {
        // Mark entity type 7 to prevent re-collection
        *(entity->spawn_data + 0x12) = 7;
        
        // Modify PlayerState
        AddXXX(g_pPlayerState, +1);
        
        // Visual feedback
        InitDecorEntityWithScreenOffset(entity, ...);
        
        // Audio feedback
        PlayEntityPositionSound(entity, sound_id);
    }
}
```

---

## HUD Message System

All Add functions notify HUD via callback at `GameState+0x14C`:
```c
(*callback)(hud_entity, 0x1013, hud_param, 0);
```

| HUD Param | Collectible Type |
|-----------|------------------|
| 1 | Orb count updated |
| 2 | Lives updated |
| 3 | Swirly Q / Halo |
| 4 | 1970 Icon |
| 5 | Health/Hamster |
| 6 | Phoenix Hand |
| 7 | Phart Head |
| 8 | Universe Enema |
| 9 | Super Willie |

---

## Add Function Addresses

| Function | Address | Field | Max |
|----------|---------|-------|-----|
| `AddPlayerLives` | 0x8002639c | +0x11 | 99 |
| `AddPlayerOrbs` | 0x8002646c | +0x12 | 99→0 |
| `AddSwirlys` | 0x800262cc | +0x13 | 20 |
| `AddPhoenixHands` | 0x80026614 | +0x14 | 7 |
| `AddPhartHeads` | 0x800266e4 | +0x15 | 7 |
| `AddUniverseEnemas` | 0x800267b4 | +0x16 | 7 |
| `Add1970Icons` | 0x80026884 | +0x19 | 3 |
| `AdjustPlayerStats` | 0x80026954 | +0x1a, +0x1b | 3, 48 |
| `AddSuperWillies` | 0x80026a48 | +0x1c | 7 |

---

## Entity Type → Collectible Mapping (Verified January 2026)

| Type | Name | Sprite ID | Storage | Max | Init Function |
|------|------|-----------|---------|-----|---------------|
| **2** | Green Bullets (Swirly Q) | `0xe8628689` | +0x13 | 20 | `InitGreenBulletsCollectible` |
| **7** | Clayball (Orb) | `0xb8700ca1` | +0x12 | 100→life | `InitClayballWithRandomColor` |
| **11** | Extra Life (Klaymen's Head) | `0xa9240484` | +0x11 | 99 | `InitTransparentDecorEntity` |
| **23** | Phoenix Hand (The Bird) | `0x9158a0f6` | +0x14 | 7 | `InitPhoenixHandCollectible` |
| **25** | Phart Head | `0x8c510186` | +0x15 | 7 | `InitPhartHeadCollectible` |
| **61** | Universe Enema (Sparkle) | `0x6a351094` | +0x16 | 7 | `InitUniverseEnemaCollectible` |
| **69** | 1970 Icon | `0x88a28194` | +0x19 | 3 | `Init1970IconEntity` |
| **70** | Hamster Shield | `0x80e85ea0` | +0x1A, +0x1B | 3, 48 | `InitHamsterShieldCollectible` |
| **72** | Super Willie (The Head) | `0x902c0002` | +0x1c | 7 | `InitSuperWillieCollectible` |
| **75** | Yellow Bird (Glidey Bird) | `0xc87ca082` | +0x17 (bit 2) | - | `InitYellowBirdCollectible` |

**Note**: Type 75 grants GLIDE ability (bit 0x02), NOT the shield. The shield Halo (bit 0x01) is granted by the FIRST Clay Orb collected.

---

## Reset Behavior

`ResetPlayerCollectibles` @ 0x80026164 is called on death/level start:
- Clears `shrink_mode` (+0x18)
- Clears Yellow Bird flag from `powerup_flags` (+0x17 &= ~0x02)
- Iterates clayball_flags (offsets 0x06-0x0F) and tallies to `total_1ups` (+0x05)

---

## Clayball System (Bonus Level Tracking)

PlayerState offsets 0x06-0x0F are per-stage clayball collection flags:
```c
// On level complete, ResetPlayerCollectibles counts them:
for (i = 0; i < 10; i++) {
    if (playerState[6 + i] != 0) {
        playerState[5]++;  // total_1ups
        playerState[6 + i] = 0;  // clear flag
    }
}
```

Clayball entities (`EntityType057_WaypointClayball*`) set these flags when collected.

### Runtime confirmation (struct_watcher, 20260621_104346)

Observed in `game_watcher/logs/struct_watch_20260621_104346.jsonl`:

| Frame | Event |
|---|---|
| 238 | Password load: `PS.clayball_flag_0 0→1` (carried in password payload) |
| 3021 | Bonus-room entry: `PS.clayball_flag_0 1→0` AND `PS.total_1ups 0→1` in same frame |

Confirms `clayball_flag_N` is a **redeemable token** (not a permanent collected-flag): the password preserves uncollected redemptions across sessions, and they get consumed into `total_1ups` only when the player actually reaches the bonus-room trigger. The vtable swap to `g_EntityVtable_Destroyed` on the same frame as the flag clear indicates the clayball entity is destroyed as part of the redemption.

### 100-orb → 1UP wrap (verified)

Frame 4312 in the same trace:
```
orb_count: 98 → 1   (delta -97)
lives:     3 → 4
```
The −97 delta is two orbs being picked up in the same tick (98+3 = 101 → wraps to 1, +1 life). Confirms the implementation is `(orb_count += n) >= 100` with subtraction-of-100 wrap rather than per-orb modulo.
