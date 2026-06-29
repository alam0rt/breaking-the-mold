---
title: "Entity Type Remapping Correction"
category: reference
tags: [reference, entity, remapping, correction]
---

# Entity Type Remapping Correction

**Date**: January 20, 2026  
**Status**: CRITICAL DOCUMENTATION FIX

## Summary

Previous documentation incorrectly mapped BLB entity types to behaviors. The key error was:

**WRONG**: "BLB 25 = Loud Mouth enemy"  
**CORRECT**: BLB 25 behavior depends on **layer**

## The Layer-Dependent Remapping System

Entity types in Asset 501 get **remapped differently based on layer** via `RemapEntityTypesForLevel` @ 0x8008150c.

### BLB 25 (0x19) Remapping

| Layer | Internal Type | Callback | Actual Behavior |
|-------|---------------|----------|-----------------|
| **Layer 1** | 0 | EntityType004_DefaultPickup_Init | **Pickup/Collectible** |
| **Layer 2** | 79 (0x4F) | EntityType079_RandomizedMenuAlt_Init | **Enemy Spawner** |

### BLB 27 (0x1B) Remapping

| Layer | Internal Type | Callback | Actual Behavior |
|-------|---------------|----------|-----------------|
| **Layer 1** | 4 | EntityType004_DefaultPickup_Init | **Pickup/Collectible** |
| **Layer 2** | 97 (0x61) | EntityType111_GridLine_Init | **Grid/Helper Entity** |

## SCIE Stage 0 Analysis

Our earlier visual identification in SCIE stage 0 was misleading:

- **BLB 25, Layer 1** (8 entities): These are **foreground pickups**, not enemies
- **BLB 27, Layer 1** (8 entities): These are **foreground pickups**, not enemies
- **BLB 13** (0 entities): The actual "EnemyA" type doesn't exist in this level

## What Are the Actual Enemies?

Looking at `RemapEntityTypesForLevel` Layer 2 mappings:

| BLB Type | Internal | Init Function | Actual Behavior |
|----------|----------|---------------|-----------------|
| 13 (0x0D) | 25 | EntityType025_PhartHeadCollectible_Init | **Phart Head Collectible** (NOT enemy!) |
| 14 (0x0E) | 72 | EntityCallback_Type72 | Unknown |
| 21 (0x15) | 28 | EntityType028_PlatformA_Init | Platform |
| 22 (0x16) | 29 | EntityType029_Enemy_Init | **Ground Patrol Enemy** |

**CRITICAL CORRECTION (Jan 20, 2026 update):**
Internal Type 25 is **NOT an enemy** - it's the Phart Head Collectible!
- Init: `EntityType025_PhartHeadCollectible_Init` @ 0x800805c8
- Calls: `InitPhartHeadCollectible` → creates bobbing collectible
- On touch: Calls `AddPhartHeads(+1)`, plays sound 0xc0034658
- Sprite: 0x8c510186 (collectible), has child sprite 0xa9240484

**Actual Ground Enemies** are Internal Types 26, 27, 29:
- Type 26: `EntityType026_Enemy_Init` - Path-following enemy with `g_SpriteList_Type026_Enemy`
- Type 27: `EntityType027_PathEnemy_Init` - Path-following enemy variant
- Type 29: `EntityType029_Enemy_Init` - Another enemy type

## Action Items

1. ✅ Update `docs/reference/official-enemy-names.md` to remove "BLB 25 = Loud Mouth"
2. ✅ Update `docs/systems/entity-identification.md` with correct layer-dependent mappings
3. ✅ Rename Ghidra functions for clarity
4. ✅ Corrected: Internal Type 25 = Phart Head Collectible, NOT enemy

## Internal Type 25 - Phart Head Collectible

Internal type 25 (from BLB 13 on Layer 2) uses callback at 0x800805c8:

```
EntityType025_PhartHeadCollectible_Init @ 0x800805c8
  ↓ calls
InitPhartHeadCollectible @ 0x8002ea3c
  - Parent sprite: 0x8c510186
  - Child sprite: 0xa9240484 (offset -8, -16)
  - Tick callback: CollectiblePhartHeadTickCallback @ 0x8002ec3c
  - On collection: AddPhartHeads(+1)
```

This is **NOT an enemy** - it's a collectible item!

## Verification Method

To correctly identify enemy types:

1. Check entity's **layer** in Asset 501 data
2. Apply correct remapping table (Layer 1, 2, or 3)
3. Look up internal type in callback table
4. Decompile init function to find sprite ID
5. Compare sprite with gameplay footage
