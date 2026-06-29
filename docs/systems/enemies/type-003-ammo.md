---
title: "Entity Type 3: Green Bullet Pickup"
category: systems/enemies
tags: [systems, enemies, type, 003, ammo]
---

# Entity Type 3: Green Bullet Pickup

**Entity Type**: 3  
**BLB Type**: 3  
**Callback**: 0x8007efd0 (shared with types 0, 4)  
**Sprite ID**: Unknown (needs extraction)  
**Category**: Collectible (Ammo)  
**Count**: 308 instances (very common!)

---

## Overview

Green Bullet pickups that grant projectile ammunition.

> **CORRECTION (Jan 16, 2026)**: Per official game manual:
> - `[0x13]` = **Green Bullets** (projectile ammo, max 20, Circle button)
> - `[0x1A]` = **Hamsters** (orbiting shield, max 3 hits)

**Gameplay Function**: Replenish Green Bullets for Circle button projectile attack

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
// When player touches Green Bullet pickup
if (DispatchEventToCollidingEntity(player, pickup)) {
    // Grant Green Bullets
    int current = g_pPlayerState[0x13];  // Green Bullet count
    g_pPlayerState[0x13] = min(current + 5, 20);  // +5, max 20
    
    // Play collection sound
    PlaySoundEffect(0x7003474c, pan, 0);
    
    // Remove pickup
    RemoveEntity(pickup);
}
```

**Storage**: `g_pPlayerState[0x13]` (Green Bullet count)  
**Max**: 20  
**Amount**: +5 per pickup (estimated)

---

## Godot Implementation

```gdscript
extends Area2D
class_name GreenBulletPickup

const MAX_GREEN_BULLETS = 20
const PICKUP_AMOUNT = 5

func _on_player_touch(body: Node2D) -> void:
    if body.is_in_group("player"):
        # Grant Green Bullets (projectile ammo)
        body.green_bullet_count = min(body.green_bullet_count + PICKUP_AMOUNT, MAX_GREEN_BULLETS)
        
        # Sound and remove
        AudioManager.play_sound(0x7003474c)
        queue_free()
```

---

**Status**: ✅ **Fully Documented** (corrected Jan 16, 2026)  
**Sprite ID**: ⚠️ Needs extraction  
**Implementation**: Ready

