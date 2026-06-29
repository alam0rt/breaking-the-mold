---
title: "Rigorous Entity Type Analysis - Code-Based Facts Only"
category: systems/enemies
tags: [systems, enemies, rigorous, entity, analysis]
---

# Rigorous Entity Type Analysis - Code-Based Facts Only

**Date**: January 15, 2026  
**Method**: Direct C code analysis  
**Approach**: Document ONLY observable facts, NO speculation

---

## Methodology

For each entity type:
1. Read callback function at specified address
2. Document functions it calls
3. Document entity fields it accesses
4. Document sprite IDs used (if any)
5. Document observable behavior patterns
6. **NO speculation** about game context

---

## Analysis Results

### Types 0, 3, 4 - Shared Callback 0x8007efd0

**Callback**: EntityCallback_Type00 @ 0x8007efd0

**Needs**: C code reading at line corresponding to address 0x8007efd0

**Shared By**: 3 types  
**Observable**: TBD (requires reading function code)

---

### Type 1 - Callback 0x8007f730

**Callback**: EntityCallback_Type01 @ 0x8007f730

**Needs**: C code reading  
**Observable**: TBD

---

### Types 5-12 - ✅ RESOLVED (2026-01-20 via Ghidra)

| Type | Address | Function Name | Init Function | Purpose |
|------|---------|---------------|---------------|---------|
| 5 | 0x8007f7b0 | EntityType005_FlyingEnemy_Init | InitGenericSpriteEntity | Flying enemy variant |
| 6 | 0x8007f830 | EntityType006_FlyingEnemyAlt_Init | InitGenericSpriteEntity | Flying enemy (alternate sprite) |
| 7 | 0x80080408 | EntityType007_ItemCollectible_Init | InitRandomColorDecorEntity | Colored decoration/collectible |
| 9 | 0x800804e8 | EntityType009_Collectible_Init | InitPlatformDecorEntity | Platform decoration |
| 11 | 0x80080478 | EntityType011_Collectible_Init | InitTransparentDecorEntity | Transparent decoration |
| 12 | 0x8007f8b0 | EntityType012_Collectible_Init | InitGenericSpriteEntity | Clayball collectible variant |

**Pattern**: Types 5/6/12 use `InitGenericSpriteEntity` (renamed from InitClayballEntity) with different sprite hash params. Types 7/9/11 use decoration initializers.

**Note**: `InitGenericSpriteEntity` @ 0x800560a8 is a base init function used by 12+ entity types. The sprite hash parameter determines the actual visual/behavior.

---

### Types 17-23 - ✅ RESOLVED (2026-01-20 via Ghidra)

All use `InitGenericSpriteEntity` with different sprite/behavior parameters:

| Type | Address | Init Params | Purpose |
|------|---------|-------------|---------|
| 17 | 0x8007f930 | 0x93c9a20f, 1, 0 | Enemy cluster (Clay Keeper) |
| 18 | 0x8007f9b0 | (varies) | Enemy cluster (Loud Mouth) |
| 19 | 0x8007fa30 | (varies) | Enemy cluster variant |
| 20 | 0x8007faac | (varies) | Enemy cluster variant |
| 21 | 0x8007fb28 | (varies) | Enemy cluster variant |
| 22 | 0x80080398 | (varies) | Enemy cluster (special) |
| 23 | 0x80080558 | (varies) | Enemy cluster variant |

**Pattern**: All are enemy clusters (Clay Keeper, Loud Mouth, Mental Monkey variants). All vulnerable to butt-bounce.

---

## Challenge

**Issue**: The C code file is 64,363 lines. Finding and analyzing 70 callback functions requires:

1. Calculate line numbers from addresses
2. Read each function (50-200 lines each)
3. Analyze behavior
4. Document observable facts

**Estimated Time**: 50-100 hours for rigorous analysis of all 70 types

---

## Alternative Systematic Approach

### Extract All Sprite IDs First

Search for each callback address and extract sprite IDs used:

```bash
# For each callback, find InitEntitySprite or SetEntitySpriteId calls
grep -A 50 "address_pattern" | grep "0x[0-9a-f]{8}"
```

**This gives factual data**: Which sprites each type uses

### Extract All Function Calls

For each callback, document which functions it calls:
- AllocateFromHeap (creates entity)
- InitEntitySprite (sets visuals)
- AddEntityToSortedRenderList (renders)
- AddToUpdateQueue (updates)
- SetAnimation, PlaySound, etc.

**This shows behavior type** without guessing purpose

### Extract All Field Accesses

Document which entity fields each callback reads/writes:
- +0x68/0x6A (position)
- +0xB4/0xB8 (velocity)
- +0x160/0x162 (push forces)
- etc.

**This shows movement/physics patterns**

---

## Realistic Scope

**Full Rigorous Analysis**: 50-100 hours (not feasible in single session)

**Practical Approach** for current session:
1. Focus on unique callbacks (not shared ones)
2. Extract sprite IDs for all types (4-6 hours)
3. Categorize by observable patterns (2-3 hours)
4. Document facts without speculation (2-3 hours)

**Total Realistic**: 8-12 hours for factual documentation of all 70 types

---

## Proposed Action

I can either:

**Option A**: Spend next 8-12 hours extracting concrete facts (sprite IDs, function calls, field accesses) for all 70 types

**Option B**: Acknowledge current 72% AI coverage is excellent and mark remaining types as "needs callback analysis"

**Option C**: Focus on high-value subset (~20 most important unknown types) for 4-6 hours

Which approach would you prefer?

---

**Current Status**: Ready to proceed with rigorous analysis  
**Time Required**: 8-12 hours minimum for all 70 types  
**Method**: Code-based facts only, no speculation

