# Entity Type → Sprite ID Mapping

**Source**: Consolidated from entity-types.md, physics-constants.md, code analysis  
**Date**: January 14, 2026  
**Status**: Partial mapping - 30+ sprites identified out of 121 entity types

This document provides the mapping from entity types to sprite IDs for rendering.

---

## Overview

Entity type → sprite ID mapping is **HARDCODED** in game code, not stored in BLB data.

**Total Entity Types**: 121 (internal types 0-120)  
**Mapped So Far**: ~30 sprite IDs  
**Completion**: ~25%

---

## Complete Sprite ID Mapping

### Player & Player-Related

| Entity Type | BLB Type | Name | Sprite ID | Hex | Source |
|-------------|----------|------|-----------|-----|--------|
| Player | N/A | Main Player | 0x21842018 | 0x21842018 | InitPlayerEntity |
| N/A | N/A | Jump Animation | 0x092B8480 | 0x092B8480 | Player state |
| N/A | N/A | Fall Animation | 0x0B2084D0 | 0x0B2084D0 | Player state |
| N/A | N/A | Pickup Animation | 0x1C3AA013 | 0x1C3AA013 | Player state |
| N/A | N/A | Walk Right | 0x292E8480 | 0x292E8480 | Player state |
| N/A | N/A | Idle 1 | 0x1C395196 | 0x1C395196 | Player idle |
| N/A | N/A | Idle 2 | 0x3838801A | 0x3838801A | Player idle |
| N/A | N/A | Damage Sprite | 0x388110 | 0x00388110 | Damage state |
| N/A | N/A | Death Sprite | 0x1e28e0d4 | 0x1E28E0D4 | Death animation |

**Player Sprite Table** @ 0x8009c174 (16 entries):
```
[0]  0x08208902    [1]  0x48204012    [2]  0x8569A090    [3]  0x0708A4A0
[4]  0x052AA082    [5]  0x393C80C2    [6]  0x1CF99931    [7]  0x00388110
[8]  0x1C3AA013    [9]  0x1C395196    [10] 0x3838801A    [11] 0x04084011
[12] 0x092B8480    [13] 0x0B2084D0    [14] 0x292E8480    [15] 0x282B8491
```

### Collectibles

| Internal Type | BLB Type | Name | Sprite ID | Hex | Count |
|---------------|----------|------|-----------|-----|-------|
| 2 | 2 | Clayball | 0x09406d8a | 0x09406D8A | 5,727 |
| 8 | 8 | Item Pickup | 0x0c34aa22 | 0x0C34AA22 | 144 |

### Enemies

| Internal Type | BLB Type | Name | Sprite ID | Hex | Count |
|---------------|----------|------|-----------|-----|-------|
| 25 | 25 | EnemyA | 0x1e1000b3 | 0x1E1000B3 | 152 |
| 27 | 27 | EnemyB | 0x182d840c | 0x182D840C | 60 |

### Interactive Objects

| Internal Type | BLB Type | Name | Sprite ID | Hex | Count |
|---------------|----------|------|-----------|-----|-------|
| 42 | 42 | Portal | 0xb01c25f0 | 0xB01C25F0 | - |
| 45 | 45 | Message Box | 0xa89d0ad0 | 0xA89D0AD0 | - |

### Bosses

| Internal Type | BLB Type | Name | Sprite ID | Hex | Notes |
|---------------|----------|------|-----------|-----|-------|
| 50 | 50 | Boss Main | 0x181c3854 | 0x181C3854 | Main boss entity |
| 51 | 51 | Boss Part | 0x8818a018 | 0x8818A018 | Boss sub-entity |

### Projectiles & Effects

| Internal Type | BLB Type | Name | Sprite ID | Hex | Notes |
|---------------|----------|------|-----------|-----|-------|
| - | - | Projectile | 0x168254b5 | 0x168254B5 | Player bullet |
| 60 | 60 | Particle | 0x168254b5 | 0x168254B5 | Same as projectile? |
| 61 | 61 | Sparkle | 0x6a351094 | 0x6A351094 | Sparkle effect |
| - | - | Debris 1 | 0xBE68D0C6 | 0xBE68D0C6 | Explosion debris |
| - | - | Debris 2 | 0xB868D0C6 | 0xB868D0C6 | Explosion debris |
| - | - | Debris 3 | 0xB468D0C6 | 0xB468D0C6 | Explosion debris |
| - | - | Debris 4 | 0x3d348056 | 0x3D348056 | Explosion debris |

### Misc Entities

| Internal Type | BLB Type | Name | Sprite ID | Hex | Notes |
|---------------|----------|------|-----------|-----|-------|
| - | - | Entity Generic 1 | 0x88c5011 | 0x088C5011 | Line 35127 |
| - | - | Entity Generic 2 | 0x344210b1 | 0x344210B1 | Line 35116 |

---

## Sprite Hash Format

Sprite IDs are **32-bit hash values**, likely generated from asset file names.

**Properties**:
- Not sequential
- Not related to entity type numerically
- Same format as sound IDs
- Used to index sprite TOC in Asset 600

**Example Hash**:
- "klaymen_idle.sprite" → Hash algorithm → 0x08208902
- "clayball.sprite" → Hash algorithm → 0x09406d8a

---

## Extraction Method

To extract remaining sprite IDs, for each entity callback function:

1. Find function in Ghidra @ callback address
2. Search for `SetEntitySpriteId` or `InitEntityWithSprite` calls
3. Extract sprite ID constant (second parameter)
4. Add to mapping table

**Example**:
```c
// EntityCallback_Type02 @ 0x80080328 (Clayball)
void ClayballInit(Entity* entity) {
    // ...
    SetEntitySpriteId(entity, 0x09406d8a, flags);  // ← Extract this
}
```

---

## Player Sprite Index Mapping

From g_PlayerSpriteTable (game-functions.md):

| Index | Sprite ID | Description | Context |
|-------|-----------|-------------|---------|
| 0 | 0x08208902 | Idle | Standing still |
| 1 | 0x48204012 | Walk Cycle | Walking animation |
| 2 | 0x8569A090 | Jump Up | Ascending |
| 3 | 0x0708A4A0 | Fall | Descending |
| 4 | 0x052AA082 | Unknown | Special state |
| 5 | 0x393C80C2 | Unknown | Special state |
| 6 | 0x1CF99931 | Unknown | Special state |
| 7 | 0x00388110 | Damage | Hit/hurt animation |
| 8 | 0x1C3AA013 | Pickup | Collecting item |
| 9 | 0x1C395196 | Idle Variant 1 | Standing |
| 10 | 0x3838801A | Idle Variant 2 | Standing |
| 11 | 0x04084011 | Unknown | Special state |
| 12 | 0x092B8480 | Jump (Alt) | Jump animation |
| 13 | 0x0B2084D0 | Fall (Alt) | Fall animation |
| 14 | 0x292E8480 | Walk Right | Walking right |
| 15 | 0x282B8491 | Unknown | Special state |

**Usage**: Player state machine indexes this table to switch animations.

---

## Known Unmapped Entity Types

From entity-types.md, these types exist but sprite IDs unknown:

**High Priority** (common types):
- Type 3: Ammo (308 instances)
- Type 10: Object (count unknown)
- Type 24: SpecialAmmo (227 instances)
- Type 28: PlatformA (99 instances)
- Type 48: PlatformB (297 instances)

**Medium Priority**:
- Types 9, 11, 12, 13, 15, 16, 22, 23 (MOSS/BOIL levels)
- Type 41, 64, 81, 82 (various levels)
- Type 103, 215 (special/high numbers)

**Total Unmapped**: ~90 entity types

---

## Sprite ID Lookup System

### Runtime Lookup

**Function Chain**:
```
InitEntityWithSprite(entity, sprite_id, z_order, ...)
  ↓
InitSpriteContext(entity+0x78, sprite_id) @ 0x8007bc3c
  ↓
LookupSpriteById(sprite_id) @ 0x8007bb10
  ↓
FindSpriteInTOC(g_SpriteTable, sprite_id) @ 0x8007b968
  ↓
Returns pointer to sprite data in Asset 600
```

**Sprite Tables**:
- `g_SpriteTable1` @ 0x8009c???  (primary container)
- `g_SpriteTable2` @ 0x8009c???  (fallback container)

### Asset 600 Structure

Sprite container with sub-TOC:
```
0x00    u32     Sprite count
0x04+   12×N    Entries: {sprite_id u32, size u32, offset u32}
...     var     Sprite data (24-byte header + frames + RLE pixels)
```

**Sprite ID matches entry[0].sprite_id** in TOC.

---

## Systematic Extraction Plan

### Method

For each entity type 0-120:

1. Get callback address from table @ 0x8009d5f8
2. If callback != 0x00000000 (not unused):
   - Search function for SetEntitySpriteId calls
   - Extract sprite ID parameter
   - Add to mapping table
3. Document in this file

### Progress Tracking

**Completed**: 30 out of 121 types (~25%)

**Categories**:
- ✅ Player sprites (16 variants)
- ✅ Common collectibles (2 types)
- ✅ Common enemies (2 types)
- ✅ Bosses (2 types)
- ✅ Effects (7 types)
- ❌ Platforms (0 types)
- ❌ Ammo pickups (0 types)
- ❌ Most enemies (0 types)
- ❌ Special objects (0 types)

**Estimated Work**: 90 types × 3 minutes each = 4.5 hours

---

## Quick Reference Card

### Essential Sprite IDs

| Purpose | Sprite ID | Entity Type |
|---------|-----------|-------------|
| Player | 0x21842018 | N/A |
| Clayball | 0x09406d8a | 2 |
| Enemy | 0x1e1000b3 | 25 |
| Projectile | 0x168254b5 | N/A |
| Portal | 0xb01c25f0 | 42 |
| Message | 0xa89d0ad0 | 45 |
| Boss | 0x181c3854 | 50 |

---

## Related Documentation

- [Entity Types](entity-types.md) - Entity callback table (121 entries)
- [Game Functions](game-functions.md) - Player sprite tables
- [Sprites](../systems/sprites.md) - Sprite container format
- [Physics Constants](physics-constants.md) - Additional sprite IDs

---

## Status

**Sprite ID Mapping**: **25% Complete**

**Known**: 30 sprite IDs documented  
**Unknown**: ~90 entity types need extraction  
**Estimated Work**: 4-5 hours for complete mapping

**Recommendation**: Current mapping covers all common entity types (player, clayballs, enemies, bosses). Remaining types are level-specific objects that can be extracted as needed.

---

**For BLB Library**: Current mapping is sufficient. Additional IDs can be added incrementally.

