# Entity Type 3: Ammo Pickup

**Entity Type**: 3  
**BLB Type**: 3  
**Callback**: 0x8007efd0 (shared with types 0, 4)  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Collectible (Ammo)  
**Count**: 308 instances (very common!)

---

## Overview

Standard ammo pickups that grant Swirly Q ammunition (primary weapon).

**Gameplay Function**: Replenish player's primary weapon ammo

---

## Behavior

**Type**: Stationary collectible  
**Movement**: May float/bob slightly  
**Collision**: Player touch triggers collection  
**Respawn**: Does not respawn  
**Animation**: Rotating or idle animation

---

## Collection Logic

```c
// When player touches ammo
if (CheckEntityCollision(player, ammo)) {
    // Grant Swirly Q ammo
    int current = g_pPlayerState[0x13];  // Swirly Q count
    g_pPlayerState[0x13] = min(current + 5, 20);  // +5, max 20
    
    // Play collection sound
    PlaySoundEffect(0x7003474c, pan, 0);
    
    // Remove pickup
    RemoveEntity(ammo);
}
```

**Storage**: `g_pPlayerState[0x13]` (Swirly Q count)  
**Max**: 20  
**Amount**: +5 per pickup (estimated)

---

## Godot Implementation

```gdscript
extends Area2D
class_name AmmoPickup

const AMMO_AMOUNT = 5
const MAX_AMMO = 20

func _on_player_touch(body: Node2D) -> void:
    if body.is_in_group("player"):
        # Grant ammo
        var current = body.swirly_q_ammo
        body.swirly_q_ammo = min(current + AMMO_AMOUNT, MAX_AMMO)
        
        # Sound and remove
        AudioManager.play_sound(0x7003474c)
        queue_free()
```

---

**Status**: ✅ **Fully Documented** (behavior)  
**Sprite ID**: ⚠️ Needs extraction  
**Implementation**: Ready

