# Items Reference

This document provides technical details for all collectible items in Skullmonkeys (SLES-01090).

All item data verified via Ghidra decompilation and runtime tracing.

## Collectible Items

### Clay (Clayball)

**Gameplay:** Collect 100 for an extra life (1up).

**Technical Details:**
- Entity Type: 2 (BLB type 2, no remapping)
- Callback: `ClayballTickCallback` @ 0x80056518
- Init: `ClayballInitCallback` @ 0x800561D4
- Storage: `g_pPlayerState[0x12]` (u8, orb/clay count)
- Collection Logic: Type mask = 2, special fast path via `CheckEntityCollision` @ 0x800226f8
- Count: 5,727 total across all levels
- Sprite ID: 0x09406d8a
- Collision Flow:
  1. Clayball tick checks entity+0x110 flag
  2. Calls `CollisionCheckWrapper` @ 0x8001b47c with type_mask=2
  3. `CheckEntityCollision` checks player at GameState+0x2c directly (optimization)
  4. On collision: Calls GameState callback with message 3 (COLLECTED)
  5. Clayball disappears, score increments

**Verified:** 2025-01-13 via Ghidra decompilation of 0x80056518

---

### Halo (Invincibility Powerup)

**Gameplay:** Allows taking an extra hit from enemies. Collecting 2 halos gives you some clay.

**Technical Details:**
- Entity Type: 8 (Item pickup with variant)
- Callback: `EntityType008_TickCallback` @ 0x80081504 (empty stub)
- Storage: `g_pPlayerState[0x17]` bit 0x01 (powerup_flags)
- Visual Effect: `CreateHaloEntity` @ 0x8006de98 creates halo ring entity following player
- Player Entity Reference: Player entity+0x168 stores halo entity pointer
- Duration: Timed via player entity+0x144 (powerup_timer)
- Sprite ID: 0x0c34aa22 (Item type)

**Note:** This is the pickup item, distinct from the visual halo effect entity.

**Verified:** 2025-01-13 via Ghidra @ 0x8006de98, game-loop.md lines 521-522, 540

---

### Energy Ball (Green Bullets)

**Gameplay:** Projectile weapon fired at enemies after picking up [9 MAX].

**Technical Details:**
- Storage: `g_pPlayerState[0x1A]` (u8, green orb count)
- Max Count: 3 (displayed on HUD as "Green orbs × 3")
- Cheat: 0x0A @ 0x80082380 sets count to 3
- HUD Display: game-loop.md line 1114
- Attack: Player fires energy ball projectile on button press

**Verified:** 2025-01-13 via cheat table @ 0x800820B4, HUD rendering

---

### Swirl (Checkpoint/Bonus Room Unlock)

**Gameplay:** Collect 3 to unlock bonus room access. Take the exit at the end of the stage with the swirl on it.

**Technical Details:**
- Storage: `g_pPlayerState[0x13]` (u8, checkpoint/swirl count)
- Unlock Condition: When count reaches 3, bonus room portal becomes active
- Cheat: 0x03 @ 0x80082278 sets count to 20 ("Get all Swirly Q's immediately")
- HUD Display: game-loop.md line 1112

**Verified:** 2025-01-13 via cheat table and player state documentation

---

### 1up (Extra Life)

**Gameplay:** Grants an extra life. Looks like Klaymen's head.

**Technical Details:**
- Storage: `g_pPlayerState[0x11]` (u8, lives count)
- Conversion: 100 clayballs → 1up automatic conversion
- Total 1ups: `g_pPlayerState[0x05]` (u8, total_1ups, returned on reset)
- Cheat: 0x05 @ 0x800822A8 sets lives to 99
- Death Handler: `DecrementPlayerLives` @ 0x800262ac
- Respawn: `RespawnAfterDeath` @ 0x8007cfc0

**Verified:** 2025-01-13 via player-system.md line 16, death handling functions

---

### 1970 Icons

**Gameplay:** Only 3 exist in the entire game. Collecting all 3 unlocks the special SEVN bonus level (1970s-themed psychedelic area with extra lives reward).

**Technical Details:**
- Entity Type: 95 (0x5F)
- Callback: `EntityType095_TickCallback` @ 0x800814A4
- BLB Types: 213, 214, 221 remapped to internal type 95 (variant at entity+0xC)
- Storage: `g_pPlayerState[0x19]` (u8, "1970" icon count)
- Max Count: 3
- Unlock: When count == 3, portal in level 11 bonus room opens
- SEVN Level: special_level_id = 99, accessible from level 11
- Cheat: 0x0B @ 0x800823B0 sets "1970" icons to 3
- HUD Display: game-loop.md line 1113 ("1970" icons × 3)

**Verified:** 2025-01-13 via EntityType095 decompilation, cheat 0x0B trace, SEVN level documentation

---

## Powerup Items (7 MAX each)

All powerup items stored in `g_pPlayerState` with max count of 7.

### Fart Clone (Universe Enemas)

**Gameplay:** Creates a gas clone of yourself. Acts like an extra life without checkpoint respawn.

**Technical Details:**
- Storage: `g_pPlayerState[0x16]` (u8, universe enemas count)
- Max Count: 7
- Cheat: 0x06 @ 0x800822D0 sets count to 7
- HUD Slot: Powerup icon 3 (pause menu +0x78)
- Effect: Spawns temporary player clone entity on use

**Verified:** 2025-01-13 via cheat table @ 0x800820B4, game-loop.md line 1212

---

### Bird (Phoenix Hands)

**Gameplay:** Attacks the closest enemy when released.

**Technical Details:**
- Storage: `g_pPlayerState[0x14]` (u8, phoenix hands count)
- Max Count: 7
- Cheat: 0x07 @ 0x800822F8 sets count to 7
- HUD Slot: Powerup icon 1 (pause menu +0x5C)
- Effect: Spawns homing bird projectile entity on use

**Verified:** 2025-01-13 via cheat table @ 0x800820B4, game-loop.md line 1213

---

### Super Power (Super Willies)

**Gameplay:** Pink ball of energy that kills all enemies on screen.

**Technical Details:**
- Storage: `g_pPlayerState[0x1C]` (u8, super willies count)
- Max Count: 7
- Cheat: 0x08 @ 0x80082320 sets count to 7
- HUD Slot: Powerup icon 4 (pause menu +0x84)
- Effect: Screen-clearing attack on use

**Verified:** 2025-01-13 via cheat table @ 0x800820B4, game-loop.md line 1208

---

### Head (Phart Heads)

**Gameplay:** Spins on screen and collects all other items automatically (item magnet).

**Technical Details:**
- Storage: `g_pPlayerState[0x15]` (u8, phart heads count)
- Max Count: 7
- Cheat: 0x09 @ 0x80082348 sets count to 7 + resets flags
- HUD Slot: Powerup icon 2 (pause menu +0x68)
- Effect: Spawns item magnet entity on use

**Verified:** 2025-01-13 via cheat table @ 0x800820B4, game-loop.md line 1209

---

## Special Powerups

### Hamster Shield

**Gameplay:** Three hamsters spin around player and kill three enemies on contact.

**Technical Details:**
- Entity System: Creates 3 rotating shield entities
- Duration: Until 3 enemy collisions occur
- Collision: Each hamster disappears after killing one enemy
- Entity Type: Unknown (requires trace analysis)

**Verified:** Partial - gameplay description confirmed, technical details need tracing

---

### Yellow Bird (Glide Ability)

**Gameplay:** Follows player and allows gliding by holding 'X' button.

**Technical Details:**
- Storage: `g_pPlayerState[0x17]` bit 0x02 (powerup_flags, "Trail" bit)
- Visual Effect: Trail entity following player
- Player Entity Reference: Player entity+0x16c stores trail entity pointer
- Input: Hold 'X' (jump button) while falling to activate glide
- Physics: Reduces fall velocity when active

**Verified:** 2025-01-13 via player-system.md lines 30-33, game-loop.md line 541, 879

---

## Key Function References

| Address | Name | Purpose |
|---------|------|---------|
| 0x80056518 | ClayballTickCallback | Clayball entity tick - collection logic |
| 0x800561D4 | ClayballInitCallback | Clayball entity initialization |
| 0x800226f8 | CheckEntityCollision | Main collision detection (type mask routing) |
| 0x8001b47c | CollisionCheckWrapper | Wraps CheckEntityCollision with entity bbox |
| 0x8006de98 | CreateHaloEntity | Create halo powerup visual effect |
| 0x800814A4 | EntityType095_TickCallback | "1970" icon collection handler |
| 0x800820B4 | CheckCheatCodeInput | Cheat code entry point (22 handlers) |
| 0x800262ac | DecrementPlayerLives | Decrement lives counter |
| 0x8007cfc0 | RespawnAfterDeath | Respawn player after death |

**Note:** Collection handling occurs via GameState callback mechanism (message 3) invoked from ClayballTickCallback. The actual score/counter increment logic is dispatched through GameState+0xC callback table.

---

## Player State Structure Reference

`g_pPlayerState` @ 0x8009DC20:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x05 | u8 | total_1ups | Total 1-ups collected (returned on reset) |
| 0x11 | u8 | lives | Current lives (default: 5) |
| 0x12 | u8 | orb_count | Clay/orb count (100 → 1up) |
| 0x13 | u8 | checkpoint_count | Swirl/checkpoint count (3 → bonus room) |
| 0x14 | u8 | phoenix_hands | Bird powerup count (max 7) |
| 0x15 | u8 | phart_heads | Head powerup count (max 7) |
| 0x16 | u8 | universe_enemas | Fart Clone powerup count (max 7) |
| 0x17 | u8 | powerup_flags | Bit 0x01=Halo, 0x02=Yellow Bird |
| 0x19 | u8 | icon_1970_count | "1970" icon count (max 3) |
| 0x1A | u8 | green_bullets | Energy Ball count (max 3) |
| 0x1C | u8 | super_willies | Super Power count (max 7) |

---

## Documentation Status

- ✅ **Verified:** Clay, Halo, Energy Ball, Swirl, 1up, 1970 Icons, Fart Clone, Bird, Super Power, Head, Yellow Bird
- ⚠️ **Partial:** Hamster Shield (gameplay confirmed, entity type/callbacks need tracing)

**Last Updated:** 2025-01-13
**Verification Method:** Ghidra decompilation + runtime trace analysis + cheat code reverse engineering
