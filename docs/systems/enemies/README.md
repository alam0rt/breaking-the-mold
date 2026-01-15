# Enemy Types Documentation

**Location**: `docs/systems/enemies/`  
**Last Updated**: January 15, 2026  
**Status**: 10 types documented (out of ~30 enemy types)

---

## Overview

This directory contains detailed documentation for individual enemy entity types.

**Total Enemy Types**: ~30 (out of 121 total entity types)  
**Documented**: 10 types  
**Coverage**: ~33% of enemies

---

## Documented Enemy Types

### Collect ibles

| Type | Name | File | Sprite ID | Status |
|------|------|------|-----------|--------|
| 2 | Clayball | [type-002-clayball.md](type-002-clayball.md) | 0x09406d8a | ✅ Complete |
| 8 | Item Pickup | [type-008-item-pickup.md](type-008-item-pickup.md) | 0x0c34aa22 | ✅ Complete |
| 24 | Special Ammo | [type-024-special-ammo.md](type-024-special-ammo.md) | Unknown | ✅ Behavior complete |

### Enemies

| Type | Name | File | Sprite ID | Status |
|------|------|------|-----------|--------|
| 25 | EnemyA (Ground Patrol) | [type-025-enemy-a.md](type-025-enemy-a.md) | 0x1e1000b3 | ✅ Complete |
| 27 | EnemyB (Flying) | [type-027-enemy-b.md](type-027-enemy-b.md) | 0x182d840c | ✅ Complete |

### Interactive Objects

| Type | Name | File | Sprite ID | Status |
|------|------|------|-----------|--------|
| 10 | Interactive Object | [type-010-interactive-object.md](type-010-interactive-object.md) | Unknown | ✅ Patterns complete |
| 28 | Platform A | [type-028-platform-a.md](type-028-platform-a.md) | Unknown | ✅ Behavior complete |
| 48 | Platform B | [type-048-platform-b.md](type-048-platform-b.md) | Unknown | ⚠️ Needs analysis |

### Visual Effects

| Type | Name | File | Sprite ID | Status |
|------|------|------|-----------|--------|
| 60 | Particle Effect | [type-060-particle.md](type-060-particle.md) | 0x168254b5 | ✅ Complete |
| 61 | Sparkle Effect | [type-061-sparkle.md](type-061-sparkle.md) | 0x6a351094 | ✅ Complete |

---

## Enemy Types by Behavior Pattern

### Pattern 1: Patrol Movement
- **Type 25**: EnemyA - Ground patrol enemy

### Pattern 2: Flying Movement
- **Type 27**: EnemyB - Flying enemy with sine wave

### Pattern 3: Collectibles
- **Type 2**: Clayball - Primary collectible
- **Type 8**: Item Pickup - Powerups
- **Type 24**: Special Ammo - Weapon ammo

### Pattern 4: Interactive
- **Type 10**: Interactive Object - Switches, bounce pads
- **Type 28**: Platform A - Moving platforms
- **Type 48**: Platform B - Alternate platforms

### Pattern 5: Visual Effects
- **Type 60**: Particle - Generic particles
- **Type 61**: Sparkle - Collection feedback

---

## Undocumented Enemy Types

**Remaining**: ~20 enemy types need documentation

**Priority Types** (high frequency):
- Type 3: Ammo (308 instances)
- Type 9, 11, 12: Unknown
- Type 17-23: Various
- Type 29-44: Various objects
- Type 45: Message Box

**Boss Types**:
- Type 49: Boss-related
- Type 50: Boss Main (documented in boss-ai/)
- Type 51: Boss Part (documented in boss-ai/)

---

## Documentation Template

Each enemy type document includes:
- Entity type and callback info
- Sprite ID
- Behavior description
- AI state machine
- Movement/collision logic
- Combat stats (HP, damage)
- Godot implementation example
- Related documentation links

---

## Implementation Coverage

**Ready for Implementation**:
- ✅ All 10 documented types
- ✅ Common AI patterns established
- ✅ Godot code examples provided

**Placeholders Needed**:
- ⚠️ Remaining 20 enemy types
- ⚠️ Enemy-specific variations
- ⚠️ Special abilities

---

## Related Documentation

- [Enemy AI Overview](../enemy-ai-overview.md) - Common patterns
- [Entity Types](../../reference/entity-types.md) - Complete callback table
- [Entity Sprite Mapping](../../reference/entity-sprite-id-mapping.md) - Sprite IDs
- [Combat System](../combat-system.md) - Damage mechanics

---

**Status**: ✅ **10 Types Documented**  
**Coverage**: 33% of enemy types  
**Implementation**: Ready for top 10 types + patterns for rest

