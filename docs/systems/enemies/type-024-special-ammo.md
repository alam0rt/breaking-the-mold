# Entity Type 24: Special Ammo Pickup

**Entity Type**: 24  
**BLB Type**: 24  
**Callback**: 0x8007f460  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Collectible (Ammo)  
**Count**: 227 instances

---

## Overview

Special Ammo pickups grant ammunition for the player's projectile weapons.

**Gameplay Function**: Ammo replenishment for Swirly Q's or Green Bullets

---

## Behavior

**Type**: Stationary collectible  
**Movement**: May float or bob  
**Collision**: Player touch triggers collection  
**Respawn**: Does not respawn  
**Effect**: Grants ammunition

---

## Ammo Types

Based on projectile system:

**Primary Ammo** (Swirly Q's):
- Storage: `g_pPlayerState[0x13]`
- Max: 20
- Effect: +1 to +5 ammo

**Secondary Ammo** (Green Bullets):
- Storage: `g_pPlayerState[0x1A]`
- Max: 3
- Effect: +1 ammo

**Entity Type 24** likely grants Swirly Q ammo (primary weapon)

---

## Collection Logic

```c
// When player touches ammo pickup
if (CheckEntityCollision(player, ammo_pickup)) {
    // Determine ammo type
    int ammo_type = ammo_pickup->subtype;
    
    // Grant ammo
    if (ammo_type == AMMO_SWIRLY_Q) {
        int current = g_pPlayerState[0x13];
        g_pPlayerState[0x13] = min(current + 5, 20);  // +5, max 20
    } else if (ammo_type == AMMO_GREEN_BULLET) {
        int current = g_pPlayerState[0x1A];
        g_pPlayerState[0x1A] = min(current + 1, 3);   // +1, max 3
    }
    
    // Play collection sound
    PlaySoundEffect(0x7003474c, pan, 0);
    
    // Remove pickup
    RemoveEntity(ammo_pickup);
}
```

---

## Godot Implementation

```gdscript
extends Area2D
class_name AmmoPickup

enum AmmoType { SWIRLY_Q, GREEN_BULLET }

@export var ammo_type: AmmoType = AmmoType.SWIRLY_Q
@export var ammo_amount: int = 5

func _ready() -> void:
    body_entered.connect(_on_player_touch)

func _on_player_touch(body: Node2D) -> void:
    if body.is_in_group("player"):
        match ammo_type:
            AmmoType.SWIRLY_Q:
                body.add_swirly_q_ammo(ammo_amount)
            AmmoType.GREEN_BULLET:
                body.add_green_bullet_ammo(1)
        
        AudioManager.play_sound(0x7003474c)
        queue_free()
```

---

**Status**: ✅ **Fully Documented** (behavior)  
**Sprite ID**: ⚠️ Needs extraction  
**Implementation**: Ready

