# Systematic Enemy AI Analysis

**Date**: January 15, 2026  
**Method**: Group entities by shared callbacks  
**Objective**: Reach 70% AI coverage through systematic analysis

---

## Callback Grouping Analysis

### Shared Callback Groups

Many entity types share the same callback function, indicating they're variants of a base behavior:

| Callback | Entity Types | Count | Likely Purpose |
|----------|--------------|-------|----------------|
| 0x8007efd0 | 0, 3, 4 | 3 | Generic entity/decoration |
| 0x8007f050 | 86, 87, 88 | 3 | Variant group A |
| 0x8007f0d0 | 106, 107, 108 | 3 | Variant group B |
| 0x8007f140 | 112, 113, 114 | 3 | Variant group C |
| 0x8007f1c0 | 115, 116, 117 | 3 | Variant group D |
| 0x80080af8 | 31, 32, 33 | 3 | Variant group E |
| 0x80080b60 | 34, 35, 36 | 3 | Variant group F |
| 0x80080bc8 | 37, 38 | 2 | Variant pair A |
| 0x80080ddc | 42, 43, 44, 53, 54, 55, 60 | 7 | **Portal/Particle family** |
| 0x80080e4c | 47, 48 | 2 | **Platform family** |
| 0x8008134c | 89, 97, 98, 110, 111 | 5 | Variant group G |
| 0x800812ec | 85, 104, 105 | 3 | Variant group H |

**Pattern**: Groups of 2-7 types share callbacks, differing by sprite ID or parameters

**Implication**: ~82 entity callback functions, but only ~65 unique behaviors

---

## Entity Type Categories

### Category 1: Collectibles (DONE ✅)

**Documented**:
- Type 2: Clayball
- Type 8: Item pickup
- Type 24: Special ammo

**Remaining**: Type 3 (Ammo - 308 instances!)

**Priority**: HIGH (Type 3 is very common)

### Category 2: Enemies (Partially Done)

**Documented**:
- Type 25: EnemyA (ground patrol)
- Type 27: EnemyB (flying)

**Remaining Active Types**:
- Types 17-23 (7 types)
- Type 26 (1 type)
- Type 29-30 (2 types)
- Type 49 (boss-related)

**Estimated**: ~15-20 actual enemy types

**Priority**: HIGH

### Category 3: Interactive Objects

**Documented**:
- Type 10: Interactive object (generic)
- Type 28: Platform A
- Type 48: Platform B

**Remaining**:
- Types 31-41 (11 types - likely various interactive objects)
- Types 46-47 (2 types)

**Priority**: MEDIUM

### Category 4: Visual Effects (DONE ✅)

**Documented**:
- Type 60: Particle
- Type 61: Sparkle

**Remaining**: Types 42-44, 53-55 share particle callback (0x80080ddc)

**Priority**: LOW (variants of documented)

### Category 5: Special/Unknown

**Types**: 57-59, 62-120 (many types)

**Priority**: LOW to MEDIUM (need analysis)

---

## Priority Analysis Plan

### High Priority (15-20 types, 12-15 hours)

**Type 3 - Ammo Pickup** (308 instances!):
- Most common uncounted type
- Essential collectible
- Quick to document

**Types 17-23 - Enemy Cluster**:
- 7 consecutive enemy types
- Likely common enemies
- Various behaviors

**Type 26 - Enemy**:
- Between documented enemies
- Likely important

**Types 29-30 - Enemies**:
- Near documented types
- Likely ground or flying

**Type 49 - Boss-Related**:
- Boss entity type
- Important for boss system

### Medium Priority (10-15 types, 8-12 hours)

**Types 31-41 - Interactive Objects**:
- Switches, doors, triggers
- Important for level progression
- 11 types total

**Types 62-76 - Various**:
- Mixed category
- Some may be enemies
- Some may be decorations

### Low Priority (Remaining types)

**Types 79-120**:
- Many share callbacks
- Likely variants
- Level-specific objects
- Can defer or use pattern matching

---

## Systematic Documentation Strategy

### Step 1: Group by Callback

For each unique callback address:
1. Identify all entity types using it
2. Analyze callback function once
3. Document variants (sprite ID differences)
4. Create single doc or variant doc

### Step 2: Pattern Recognition

Look for common patterns in callbacks:
- Movement type (patrol, fly, static)
- Collision behavior
- State machine structure
- Attack patterns

### Step 3: Sprite ID Extraction

For each callback:
1. Search C code for InitEntitySprite calls
2. Extract sprite ID(s)
3. Document visual appearance

### Step 4: Behavioral Documentation

Document:
- Movement pattern
- Collision response
- Attack behavior (if enemy)
- HP and damage (if applicable)
- State machine
- Godot implementation

---

## Estimated Coverage

### Current State

**Documented**: 10 enemy types  
**Coverage**: 10 / ~30 actual enemies = 33%

**AI Systems**:
- Enemy AI: 50% (patterns + 10 types)
- Boss AI: 60% (architecture + 5 bosses, 1 complete)
- Combined: ~55% average

### With Systematic Analysis

**If Document 15 More Types** (high priority):
- Total: 25 / 30 = 83% of enemy types
- Enemy AI: 50% → 70%
- Boss AI: 60% (no change)
- **Combined: ~65% average**

**If Document 10 More Types** (prioritized):
- Total: 20 / 30 = 67% of enemy types
- Enemy AI: 50% → 65%
- Boss AI: 60% (no change)
- **Combined: ~62% average**

**Target**: Document 12-15 more types to reach **70% AI coverage**

---

## Next Steps

### Batch 1: Essential Collectibles (2 hours)

- Type 3: Ammo (very common)
- Type 7: Unknown collectible?
- Type 9: Unknown collectible?
- Type 11: Unknown collectible?
- Type 12: Unknown collectible?

### Batch 2: Enemy Cluster (5-6 hours)

- Types 17-23: 7 enemy types (likely ground/flying variants)
- Type 26: Enemy between documented types

### Batch 3: Important Objects (3-4 hours)

- Type 45: Message box
- Type 42: Portal (level exit)
- Types 31-36: Interactive objects (6 types, 3 shared callbacks)

### Batch 4: Remaining Enemies (3-4 hours)

- Type 29-30: Near documented enemies
- Types 62-70: Mid-range types
- Type 49: Boss-related entity

**Total**: ~15 hours to reach 70% AI coverage

---

## Documentation Template

Each new enemy doc will include:
1. Entity type and callback address
2. Sprite ID (extract from C code)
3. Behavior pattern (patrol/fly/static/etc)
4. Movement logic
5. Combat stats
6. State machine
7. Godot implementation
8. Related docs

**Time per type**: 30-60 minutes average

---

## Related Documentation

- [Enemy AI Overview](../enemy-ai-overview.md) - Common patterns
- [Entity Types](../../reference/entity-types.md) - Complete callback table
- [Enemies Directory](README.md) - Individual enemy docs

---

**Status**: 📋 **Plan Complete**  
**Target**: 70% AI coverage (currently 55%)  
**Method**: Document 12-15 more enemy types systematically  
**Time**: 12-15 hours

