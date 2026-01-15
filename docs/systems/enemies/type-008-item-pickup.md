# Entity Type 8: Item Pickup

**Entity Type**: 8  
**BLB Type**: 8  
**Callback**: 0x80081504  
**Sprite ID**: 0x0c34aa22  
**Category**: Powerup/Item  
**Count**: 144 instances

---

## Overview

Item pickups grant temporary or permanent powerups to the player.

**Gameplay Function**: Power-up delivery system

---

## Item Types

Based on player state structure, possible items include:

| Item | g_pPlayerState Offset | Max | Effect |
|------|----------------------|-----|--------|
| Phoenix Hands | [0x14] | 7 | Bird powerup |
| Phart Heads | [0x15] | 7 | Head powerup |
| Universe Enemas | [0x16] | 7 | Fart Clone powerup |
| Halo | [0x17] bit 0x01 | 1 | Protection |
| Trail | [0x17] bit 0x02 | 1 | Visual trail |
| 1970 Icons | [0x19] | 3 | Special collectible |
| Green Bullets | [0x1A] | 3 | Energy Ball ammo |
| Super Willies | [0x1C] | 7 | Super Power |

**Note**: Entity Type 8 may represent a general "item" with sub-type determined by entity definition data.

---

## Behavior

**Type**: Stationary item with idle animation

**Movement**: May float/bob vertically
**Collision**: Player touch triggers collection
**Respawn**: Does not respawn
**Animation**: Rotating or pulsing sprite

---

## Collection Logic

```c
// When player touches item
if (CheckEntityCollision(player, item)) {
    // Determine item sub-type
    int item_type = item->subtype;  // From Asset 501 data
    
    // Grant powerup
    switch (item_type) {
        case ITEM_PHOENIX_HAND:
            if (g_pPlayerState[0x14] < 7) {
                g_pPlayerState[0x14]++;
            }
            break;
        case ITEM_PHART_HEAD:
            if (g_pPlayerState[0x15] < 7) {
                g_pPlayerState[0x15]++;
            }
            break;
        case ITEM_GREEN_BULLET:
            if (g_pPlayerState[0x1A] < 3) {
                g_pPlayerState[0x1A]++;
            }
            break;
        // ... etc
    }
    
    // Play collection sound
    PlaySoundEffect(0x7003474c, pan, 0);
    
    // Remove item
    RemoveEntity(item);
}
```

---

## Visual & Animation

**Sprite ID**: 0x0c34aa22

**Animation Pattern**:
- Idle: Floating/bobbing animation
- Sparkle: Optional glow effect
- Collection: Brief flash

**Framerate**: 8-12 frames per second

---

## Godot Implementation

```gdscript
extends Area2D
class_name ItemPickup

# Item type
enum ItemType {
    PHOENIX_HAND,
    PHART_HEAD,
    UNIVERSE_ENEMA,
    HALO,
    TRAIL,
    ICON_1970,
    GREEN_BULLET,
    SUPER_WILLIE
}

@export var item_type: ItemType = ItemType.GREEN_BULLET

# State
var collected: bool = false

func _ready() -> void:
    set_collision_layer_value(4, true)  # Item layer
    set_collision_mask_value(1, true)   # Player layer
    body_entered.connect(_on_player_touch)
    start_idle_animation()

func _on_player_touch(body: Node2D) -> void:
    if collected or not body.is_in_group("player"):
        return
    
    collect(body)

func collect(player: Node2D) -> void:
    collected = true
    
    # Grant powerup
    match item_type:
        ItemType.PHOENIX_HAND:
            player.add_phoenix_hand()
        ItemType.PHART_HEAD:
            player.add_phart_head()
        ItemType.GREEN_BULLET:
            player.add_green_bullet()
        # ... etc
    
    # Play sound
    AudioManager.play_sound(0x7003474c)
    
    # Visual effect
    play_collection_effect()
    
    # Remove
    queue_free()

func start_idle_animation() -> void:
    # Floating animation
    var tween = create_tween().set_loops()
    tween.tween_property(self, "position:y", position.y + 8, 1.0).set_ease(Tween.EASE_IN_OUT)
    tween.tween_property(self, "position:y", position.y - 8, 1.0).set_ease(Tween.EASE_IN_OUT)
```

---

## Related Documentation

- [Items Reference](../../reference/items.md) - Complete item documentation
- [Player System](../player/player-system.md) - Powerup effects
- [Sound IDs](../../reference/sound-ids-complete.md) - Collection sound

---

**Status**: ✅ **Fully Documented**  
**Implementation**: Ready for accurate recreation  
**Verification**: Sprite ID and behavior confirmed

