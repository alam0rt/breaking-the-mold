# Entity Type 24: Special Pickup

**Entity Type**: 24  
**BLB Type**: 24  
**Callback**: 0x8007f460  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Collectible (Ammo/Hamsters)  
**Count**: 227 instances

---

## Overview

Special pickups that grant Green Bullets (ammo) or Hamsters (shield).

> **CORRECTION (Jan 16, 2026)**: Per official game manual:
> - `[0x13]` = **Green Bullets** (projectile ammo, max 20, Circle button)
> - `[0x1A]` = **Hamsters** (orbiting shield, max 3 hits)

**Gameplay Function**: Green Bullet or Hamster replenishment

---

## Behavior

**Type**: Stationary collectible  
**Movement**: May float or bob  
**Collision**: Player touch triggers collection  
**Respawn**: Does not respawn  
**Effect**: Grants ammo or shield

---

## Pickup Types

Based on game systems:

**Green Bullets** (Projectile Ammo):
- Storage: `g_pPlayerState[0x13]`
- Max: 20
- Purpose: Ranged attack (Circle button)

**Hamsters** (Orbiting Shield):
- Storage: `g_pPlayerState[0x1A]`
- Max: 3
- Purpose: Provides 3 extra hits of protection
- Time Limited: Hamsters wander off after a duration

**Entity Type 24** likely grants Green Bullets or Hamsters based on variant

---

## Collection Logic

```c
// When player touches pickup
if (CheckEntityCollision(player, pickup)) {
    // Determine pickup type from variant
    int pickup_type = pickup->subtype;
    
    // Grant item
    if (pickup_type == PICKUP_GREEN_BULLET) {
        int current = g_pPlayerState[0x13];
        g_pPlayerState[0x13] = min(current + 5, 20);  // +5, max 20
    } else if (pickup_type == PICKUP_HAMSTER) {
        int current = g_pPlayerState[0x1A];
        g_pPlayerState[0x1A] = min(current + 1, 3);   // +1, max 3
    }
    
    // Play collection sound
    PlaySoundEffect(0x7003474c, pan, 0);
    
    // Remove pickup
    RemoveEntity(pickup);
}
```

---

## Godot Implementation

```gdscript
extends Area2D
class_name SpecialPickup

enum PickupType { GREEN_BULLET, HAMSTER }

@export var pickup_type: PickupType = PickupType.GREEN_BULLET

func _ready() -> void:
    body_entered.connect(_on_player_touch)

func _on_player_touch(body: Node2D) -> void:
    if body.is_in_group("player"):
        match pickup_type:
            PickupType.GREEN_BULLET:
                body.add_green_bullets(5)  # Projectile ammo
            PickupType.HAMSTER:
                body.add_hamster(1)  # Orbiting shield
        
        AudioManager.play_sound(0x7003474c)
        queue_free()
```

---

**Status**: ✅ **Fully Documented** (corrected Jan 16, 2026)  
**Sprite ID**: ⚠️ Needs extraction  
**Implementation**: Ready

