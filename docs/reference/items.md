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
- Init Callback: `EntityType008_InitCallback` @ 0x80081504 (empty stub)
- Storage: `g_pPlayerState[0x17]` bit 0x01 (powerup_flags)
- Visual Effect: `CreateHaloEntity` @ 0x8006de98 creates halo ring entity following player
- Player Entity Reference: Player entity+0x168 stores halo entity pointer
- Duration: Timed via player entity+0x144 (powerup_timer)
- Sprite ID: 0x0c34aa22 (Item type)

**Note:** This is the pickup item, distinct from the visual halo effect entity.

**Verified:** 2025-01-13 via Ghidra @ 0x8006de98, game-loop.md lines 521-522, 540

---

### Swirly Q (Bonus Room Collectible)

**Gameplay:** Collect 3 in a level to open bonus room portal. Cumulative total tracked for secret ending.

**Technical Details:**
- Storage: `g_pPlayerState[0x13]` (u8, current level swirly_q_count)
- Max via cheat: 20 (allows multiple bonus room entries)
- Pickup Only: No button activation
- Cheat: 0x03 @ 0x80082278 sets count to 20
- Portal Spawn: `SpawnSwirlPortalEntity` @ 0x8005ad54 decrements count
- Cumulative Total: `g_pPlayerState[0x1B]` (48+ for secret ending)

**Note:** Cheat 0x03 is correctly labeled "Max Swirly Q's" in cheat lists.

**Verified:** 2026-01-16 via CheckCheatCodeInput @ 0x800820b4 + SpawnSwirlPortalEntity @ 0x8005ad54

---

### Hamsters (Orbiting Shield)

**Gameplay:** Orbiting protective shield. Provides 3 extra hits of protection.

**Technical Details:**
- Storage: `g_pPlayerState[0x1A]` (u8, hamster_count)
- Max Count: 3 (3 hamsters orbit player, each absorbs 1 hit)
- Pickup Only: No button activation (R1 mentioned in manual but may be incorrect)
- Cheat: 0x0A @ 0x80082380 sets count to 3
- Clear Function: `ClearHamsterCount` @ 0x8002615c (clears on level start)
- Time Limit: Hamsters wander off after a duration ("short attention spans")

**Note:** Cheat 0x0A is MISLABELED as "Max Green Bullets" in cheat lists - it actually controls Hamsters!

**Verified:** 2026-01-16 via CheckCheatCodeInput @ 0x800820b4 + ClearHamsterCount @ 0x8002615c

---

### Green Bullets (Projectile Ammo)

**Gameplay:** Ranged projectile weapon. Press Circle to fire at enemies.

**Technical Details:**
- Storage: **Unknown** - likely entity-based ammo system, NOT in PlayerState
- Max Count: 20 (per game manual)
- Button: Circle
- Attack: Player fires green energy ball projectile

**Note:** Despite the name appearing in cheat lists, the actual Green Bullet ammo storage
is NOT at `[0x1A]` (that's Hamsters) or `[0x13]` (that's Swirly Qs). The projectile
weapon may use a different storage mechanism or be unlimited with cooldown.

**Status:** ⚠️ Needs further investigation to find actual ammo storage.

**Verified:** 2026-01-16 - storage location needs confirmation

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
- Init Callback: `EntityType095_InitCallback` @ 0x800814A4
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

All powerup items stored in `g_pPlayerState` with max count of 7. Use buttons L1/L2/R1/R2.

### Fart Clone (Universe Enemas)

**Gameplay:** Creates a gas clone of yourself. Acts like an extra life without checkpoint respawn.

**Technical Details:**
- Storage: `g_pPlayerState[0x16]` (u8, universe enemas count)
- Max Count: 7
- Button: R1 (0x0008 mask)
- Cheat: 0x06 @ 0x800822D0 sets count to 7
- HUD Slot: Powerup icon 3 (pause menu +0x78)
- Effect: Screen-clearing attack

**Verified Functions (Ghidra):**
| Address | Function | Purpose |
|---------|----------|---------|
| 0x8006c0d8 | UniverseEnemaActivate | Activation callback |
| 0x8006c278 | UniverseEnemaKillAllEnemies | Kill all entities with flag 0x04 |

**Activation Flow:**
1. R1 pressed → checks `g_pPlayerState[0x16] > 0`
2. Broadcasts MSG_UNIVERSE_ENEMA (0x1018) to entities
3. Iterates collision list, sends MSG_PROJECTILE_HIT (0x1002) to killable entities
4. Decrements count, clears screen effect flag

**Verified:** 2026-01-16 via cheat table @ 0x800820B4, Ghidra decompilation

---

### Bird (Phoenix Hands)

**Gameplay:** Attacks the closest enemy when released.

**Technical Details:**
- Storage: `g_pPlayerState[0x14]` (u8, phoenix hands count)
- Max Count: 7
- Button: L1 (0x0004 mask)
- Cheat: 0x07 @ 0x800822F8 sets count to 7
- HUD Slot: Powerup icon 1 (pause menu +0x5C)
- Effect: Spawns homing bird projectile entity on use

**Verified:** 2026-01-16 via cheat table @ 0x800820B4, button mapping from game manual

---

### Super Power (Super Willies)

**Gameplay:** Pink ball of energy that auto-collects all items on screen.

**Technical Details:**
- Storage: `g_pPlayerState[0x1C]` (u8, super willies count)
- Max Count: 7
- Button: R2 (0x0002 mask)
- Cheat: 0x08 @ 0x80082320 sets count to 7
- HUD Slot: Powerup icon 4 (pause menu +0x84)
- Effect: Auto-collect all items on screen

**Verified:** 2026-01-16 via cheat table @ 0x800820B4, button mapping from game manual

---

### Head (Phart Heads)

**Gameplay:** Spins on screen as a ghostly scout clone.

**Technical Details:**
- Storage: `g_pPlayerState[0x15]` (u8, phart heads count)
- Max Count: 7
- Button: L2 (0x0001 mask)
- Cheat: 0x09 @ 0x80082348 sets count to 7 + resets flags
- HUD Slot: Powerup icon 2 (pause menu +0x68)
- Effect: Spawns ghostly scout clone entity

**Verified:** 2026-01-16 via cheat table @ 0x800820B4, button mapping from game manual

---

## Special Powerups

### Hamster Shield

**Gameplay (Manual Claims):** Three hamsters spin around player and absorb up to 3 enemy 
hits, then "wander off after a limited duration".

**Technical Reality - ⚠️ POSSIBLY CUT FEATURE:**

Code analysis reveals the hamster protection mechanism described in the manual was either:
1. **Cut/unimplemented** in the PAL version
2. **Works differently** than documented
3. **Regional difference** (NTSC may differ)

**Evidence Against Full Implementation:**
- `PlayerEntityCollisionHandler` (0x8005c400) ONLY checks `g_pPlayerState[0x17] & 1` (Halo) 
  for damage protection - **NO hamster count check exists**
- `PlayerProcessBounceCollision` similarly only checks Halo bit for lethal damage
- No orbiting entity creation found - unlike Halo (`player+0x168`) and Yellow Bird 
  (`player+0x16c`) which have explicit entity spawning
- Cheat 0x0A only sets `g_pPlayerState[0x1A] = 3` with no entity spawn side effects
- Count appears used ONLY for HUD icon visibility (3 icons at `param_1+0x50`)
- **NO PICKUP MECHANISM EXISTS** - exhaustive search found only ClearHamsterCount (sets to 0)
  and cheat code (sets to 3). No entity pickup increments this value.

**What IS Verified:**
| Component | Details |
|-----------|---------|
| Storage | `g_pPlayerState[0x1A]` (u8, max 3) |
| Cheat | 0x0A @ 0x80082370 sets count to 3 |
| Clear Function | `ClearHamsterCount` @ 0x8002615c |
| HUD Sprite | `0x80e85ea0` (3 icons in HUD) |
| Callback | `HamsterSpriteCallback` @ 0x8006d910 |

**Verified Functions (Ghidra):**
| Address | Function | Purpose |
|---------|----------|---------|
| 0x8002615c | ClearHamsterCount | Clear hamster count on level start |
| 0x8006d910 | HamsterSpriteCallback | Sprite state callback (HUD display only?) |

**IMPORTANT:** The cheat system label "Max Green Bullets" is WRONG. Field [0x1A] is
the Hamster count, NOT Green Bullet ammo. However, the count may only affect HUD display.

**Final Verdict:** The Hamster system in PAL SLES-01090 is **NOT FULLY IMPLEMENTED**:
1. No pickup handler exists in the code
2. No damage absorption logic in PlayerEntityCollisionHandler
3. No orbiting entity spawning like Halo/YellowBird
4. Only the cheat code can set the value

The game manual's description of "orbiting hamsters that absorb hits" appears to be
either a cut feature, regional difference (NTSC may vary), or embellished documentation.

**Verified:** 2026-01-16 via exhaustive Ghidra code search. Feature NOT implemented.

---

### Yellow Bird (Glide Ability)

**Gameplay:** Follows player and allows gliding by holding 'X' button.

**Technical Details:**
- Storage: `g_pPlayerState[0x17]` bit 0x02 (powerup_flags, "Trail" bit)
- Creator: `CreateYellowBirdEntity` @ 0x8006e1d8
- Visual Effect: Trail entity following player
- Player Entity Reference: Player entity+0x16c stores trail entity pointer
- Input: Hold 'X' (jump button) while falling to activate glide
- Physics: Reduces fall velocity when active

**Verified:** 2025-01-15 via player-system.md lines 30-33, Ghidra @ 0x8006e1d8

---

## Key Function References

| Address | Name | Purpose |
|---------|------|--------|
| 0x80056518 | ClayballTickCallback | Clayball entity tick - collection logic |
| 0x800561D4 | ClayballInitCallback | Clayball entity initialization |
| 0x800226f8 | CheckEntityCollision | Main collision detection (type mask routing) |
| 0x8001b47c | CollisionCheckWrapper | Wraps CheckEntityCollision with entity bbox |
| 0x8006de98 | CreateHaloEntity | Create halo powerup visual effect |
| 0x8006e1d8 | CreateYellowBirdEntity | Create yellow bird / glide trail entity |
| 0x8005ad54 | SpawnSwirlPortalEntity | Spawn bonus room portal (uses swirl count) |
| 0x8003cfd8 | InitSpecialPickupEntity | Init special pickup (entity type 118/24) |
| 0x800418a4 | InitCollectibleEntity | Init collectible entity (sprite 0x88210498) |
| 0x80081504 | EntityType008_InitCallback | Item pickup init (empty stub) |
| 0x800814A4 | EntityType095_InitCallback | "1970" icon init handler |
| 0x800820B4 | CheckCheatCodeInput | Cheat code entry point (22 handlers) |
| 0x800262ac | DecrementPlayerLives | Decrement lives counter |
| 0x8002639c | AddPlayerLives | Add lives (max 99), notify HUD |
| 0x8002646c | AddPlayerOrbs | Add orbs, 100→1up conversion |
| 0x8002615c | ClearHamsterCount | Clear hamster count on level start |
| 0x80026164 | ResetPlayerCollectibles | Reset collectibles on death/level start |
| 0x8007cfc0 | RespawnAfterDeath | Respawn player after death |

**Weapon Button Mappings:**
| Button | Weapon | Storage | Notes |
|--------|--------|---------|-------|
| L1 | Phoenix Hand | [0x14] | Homing bird attack |
| L2 | Phart Head | [0x15] | Ghostly scout/clone |
| R1 | Universe Enema | [0x16] | Screen-clear attack |
| R2 | Super Willie | [0x1C] | Auto-collect items on screen |
| Circle | Green Bullet | [0x13]? | Projectile (storage TBD) |

**Note:** Collection handling occurs via GameState callback mechanism (message 3) invoked from ClayballTickCallback. The actual score/counter increment logic is dispatched through GameState+0xC callback table.

---

## Player State Structure Reference

`g_pPlayerState` @ 0x8009DC20:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x05 | u8 | total_1ups | Total 1-ups collected (returned on reset) |
| 0x11 | u8 | lives | Current lives (default: 5) |
| 0x12 | u8 | orb_count | Clay/orb count (100 → 1up) |
| 0x13 | u8 | swirly_q_count | **Swirly Qs** (bonus room collectible, 3 for portal, max 20) |
| 0x14 | u8 | phoenix_hands | Phoenix Hand count (max 7, L1) |
| 0x15 | u8 | phart_heads | Phart Head count (max 7, L2) |
| 0x16 | u8 | universe_enemas | Universe Enema count (max 7, R1) |
| 0x17 | u8 | powerup_flags | Bit 0x01=Halo, 0x02=Yellow Bird |
| 0x18 | u8 | shrink_mode | Player is shrunk (mini mode) |
| 0x19 | u8 | icon_1970_count | "1970" icon count (max 3) |
| 0x1A | u8 | hamster_count | **Hamsters** (orbiting shield, max 3 hits) |
| 0x1b | u8 | total_swirly_qs | **Total Swirly Qs** (secret ending, need 48+) |
| 0x1C | u8 | super_willies | Super Willie count (max 7, R2) |

---

## Documentation Status

- ✅ **Verified:** Clay, Halo, Energy Ball, Swirl, 1up, 1970 Icons, Fart Clone, Bird, Super Power, Head, Yellow Bird
- ⚠️ **Partial:** Hamster Shield (gameplay confirmed, entity type/callbacks need tracing)

**Last Updated:** 2025-01-15
**Verification Method:** Ghidra decompilation + runtime trace analysis + cheat code reverse engineering
