---
title: "Entity Init Architecture Analysis"
category: systems
tags: [systems, entity, init, architecture]
---

# Entity Init Architecture Analysis

**Date**: January 25, 2026  
**Status**: Verified via Ghidra decompilation

## Overview

The entity system uses a **three-tier dispatch architecture**:

```
BLB Entity Type (0x00-0xFF in Asset 501)
        │
        ▼
[Entity Callback Table] @ 0x8009d5f8 (121 entries × 8 bytes)
        │
        ▼
EntityTypeXXX_Init (82 unique functions)
        │
        ▼
Init Helper Function (reads entity_def+0x12 for sub-dispatch)
```

## Key Pattern: Type-Based Variant Selection

Many init functions share the same callback but use `entity_def+0x12` (the BLB entity type) to select different behaviors:

```c
// Pattern seen in CreateCollectibleWithFlags, InitSuperWillieCollectible, etc.
switch(*(short *)(entity_def + 0x12)) {
    case 0x00: case 0x56: case 0x70: case 0x73:  // Animation sequence A
        state = AnimSequence4A;
        break;
    case 0x03: case 0x57: case 0x71: case 0x74:  // Animation sequence B
        state = AnimSequence4B;
        break;
    case 0x04: case 0x58: case 0x72: case 0x75:  // Animation sequence C
        state = AnimSequence4C;
        break;
}
```

## Shared Callback Groups (14 total)

| Address | Types | Helper Function | Purpose |
|---------|-------|-----------------|---------|
| 0x8007EFD0 | 0, 3, 4 | `CreateCollectibleWithFlags` | Default pickups (ammo variants) |
| 0x8007F050 | 86, 87, 88 | `CreateCollectibleWithFlags` | Invisible hazards |
| 0x8007F0D0 | 106, 107, 108 | `InitCollectibleVariant` | Falling platforms |
| 0x8007F140 | 112, 113, 114 | `CreateCollectibleWithFlags` | Animated decor (flag=1) |
| 0x8007F1C0 | 115, 116, 117 | `CreateCollectibleWithFlags` | Parallax decor (flag=0,1) |
| 0x8007F460 | 24, 118 | `InitSpecialPickupEntity` | Special ammo pickups |
| 0x80080AF8 | 31, 32, 33 | `InitScaledPlatformEntity` | Moving platforms (scaled) |
| 0x80080B60 | 34, 35, 36 | `InitScaledPlatformEntity` | Trigger platforms (0x22-0x24) |
| 0x80080BC8 | 37, 38 | `InitDirectionalScaledEntity` | Directional scaled objects |
| 0x80080C8C | 39, 52 | `InitFloatingPlatformEntity` | Type 39: Floating platform, Type 52: Klaydog (harmless "enemy") |
| 0x80080DDC | 42-44, 53-55, 60 | `InitSuperWillieCollectible` | Super Willie heads (7 variants!) |
| 0x80080E4C | 47, 48 | `InitSoundEmittingEnemy` | Sound-emitting platforms |
| 0x800812EC | 85, 104, 105 | `InitTriggerZoneEntity` | Collision triggers |
| 0x8008134C | 89, 97, 98, 110, 111 | `InitGridLineEntity` | Grid/level event helpers |

## Init Helper Function Categories

### Category A: Sprite-Based Init (`CreateCollectibleWithFlags`)
- Takes sprite list pointer as parameter
- Calls `InitEntityWithSprite()` → `InitCollectibleEntity()`
- Uses switch on entity type for animation selection
- Entity size: 0x120 bytes

```c
EntityType000_004_Pickup_Init(param_1, param_2) {
    entity = AllocateFromHeap(0x120);
    CreateCollectibleWithFlags(entity, param_2, g_SpriteList_Type004_Type114, flag1, flag2);
    AddEntityToSortedRenderList(param_1, entity);
    AddToUpdateQueue(param_1, entity);
}
```

### Category B: Scaled Platform Init (`InitScaledPlatformEntity`)
- Uses game state scaling factor at `GameState+0x11C`
- Checks entity types 0x22, 0x23, 0x24 for special scaling behavior
- Sets up scale callbacks: `ScaleXByEntityScale`, `WorldToScreenYWithParallax`
- Entity size: 0x110 bytes

### Category C: Collectible Variants (`InitSuperWillieCollectible`)
- Complex switch statement on 7+ entity type values
- Each type maps to different animation sequence
- Entity size: 0x120 bytes

### Category D: Grid/Helper Init (`InitGridLineEntity`)
- Minimal visual representation (invisible helpers)
- Entity size: 0x80 bytes (smallest)
- Used for triggers, waypoints, level events

### Category E: Trigger Zone Init (`InitTriggerZoneEntity`)
- Checks collision types 0x2C, 0x2E
- Creates child entities based on entity type (0x68 → sprite A, 0x69 → sprite B)
- Entity size: 0x110 bytes

## Entity Size Allocation Patterns

| Size | Use Case | Examples |
|------|----------|----------|
| 0x80 | Grid/helper entities | Types 89, 97, 98, 110, 111 |
| 0x100 | Child entities | Spawned by trigger zones |
| 0x108 | Directional entities | Types 37, 38 |
| 0x110 | Platforms, triggers | Types 31-36, 45, 85, 104, 105 |
| 0x114 | Moving platforms | Types 39, 47, 48, 52, 106-108, 118 |
| 0x120 | Collectibles, pickups | Types 0-12, 42-44, 53-55, 86-88, 112-117 |

## Address Range Organization

The init functions are organized by address range:

### 0x8007EFxx-0x8007FFxx (45 types) - Core Entity Init
- Basic pickups, platforms, enemies
- Boss-related entities (49-51)
- Decor variants (112-117)

### 0x80080xxx (50 types) - Main Init Region  
- Collectibles (2, 7, 11, 23, 25)
- Scaled platforms (31-36)
- Super Willie collectibles (42-44, 53-55, 60)
- Projectiles (90-93)

### 0x80081xxx (17 types) - Extended Init
- Boss entities (100-103: Glenn Yntis, Shriney Guard, Joe-Head-Joe, Klogg)
- Level events/triggers (89, 97, 98, 104, 105, 110, 111)
- Special entities (8 Klogg ball, 79 spawner, 82 BOIL object)

## Why Collectibles Span Multiple Entity Types

**Design Pattern: Variant Selection via Type Number**

The BLB entity type serves as a **variant selector** parameter:

1. **Multiple visual variants** - Same collectible with different animations
   - Super Willie: 7 types → 7 different animation sequences
   - Ammo: 3 types (0, 3, 4) → 3 animation variants

2. **Layer/Z-order variants** - Same object at different depths
   - Grid helpers: z=900, 0x4b0, 0x578, 0x640 based on type

3. **Behavior variants** - Same base logic with tweaked parameters
   - Scaled platforms: Types 34-36 use special scaling for 0x22-0x24

4. **Color/tint variants** - Same sprite with different coloring
   - Type 0x76 gets special RGB values (0x40, 200, 100)

## Entity Definition Structure (24 bytes @ entity_def)

```c
struct EntityDef {
    u16 x1;           // +0x00 - left bound
    u16 y1;           // +0x02 - top bound  
    u16 x2;           // +0x04 - right bound
    u16 y2;           // +0x06 - bottom bound
    u16 x_center;     // +0x08 - center X (used for spawn position)
    u16 y_center;     // +0x0A - center Y (used for spawn position)
    u16 variant;      // +0x0C - variant parameter (used by some helpers)
    u8  padding[4];   // +0x0E - zeros
    u16 entity_type;  // +0x12 - THE KEY FIELD for sub-dispatch
    u16 layer;        // +0x14 - z-order layer
    u8  padding2[2];  // +0x16 - zeros
};
```

The `entity_type` at offset +0x12 is preserved from BLB and read by init helpers to select specific behaviors.

## Common Init Flow

```
1. AllocateFromHeap(size, 1, '\0')
2. InitEntityWithSprite(entity, sprite_list_ptr) or InitEntityStruct(entity)
3. entity[6] = vtable_address (sets entity vtable)
4. InitCollectibleEntity(entity, entity_def, z_order) [for collectibles]
5. Read entity_def+0x12 for type-specific setup
6. EntitySetState(entity, state_id, state_callback)
7. EntityApplyMovementCallbacks(entity, x, y+2)
8. CalcEntityYFromTileHeight(entity, ...) [for ground collision]
9. AddEntityToSortedRenderList(game_state, entity)
10. AddToUpdateQueue(game_state, entity) [for entities that tick]
```

## Sprite List Tables

Key sprite list addresses used by init functions:

| Address | Name | Used By |
|---------|------|---------|
| 0x8009D9FC | g_SpriteList_Type004_Type114 | Types 0, 3, 4, 112-114 |
| 0x8009DA18 | g_SpriteList_Type088_ForegroundDecor | Types 86, 87, 88 |
| 0x8009DA50 | g_SpriteList_Type010_InteractiveObject | Type 10 (Egg-Beater) |
| 0x8009DA5C | g_SpriteList_Type026_Enemy | Type 26 (Flying Egg-Beater) |
| 0x8009DA68 | g_SpriteList_Type027_PathEnemy | Type 27 (Flapper) |
| 0x8009DA78 | g_SpriteList_Type061_ScaledPlatform | Types 34-36 |
| 0x8009DA98 | g_SpriteList_Type063_MovingPlatform | Types 31-33 |
| 0x8009DAA4 | g_SpriteList_Type064_Trigger | Types 34-36 |
| 0x8009B508 | g_SuperWillieSpriteTable | Types 42-44, 53-55, 60 |
| 0x8009B844 | g_FloatingPlatformSprites | Types 39, 52 |

## Function Address Quick Reference

| Init Helper | Address | Purpose |
|-------------|---------|---------|
| CreateCollectibleWithFlags | 0x8003B634 | Main collectible init with animation selection |
| InitCollectibleVariant | 0x8003FE90 | Variant collectible (falling platforms) |
| InitSpecialPickupEntity | 0x8003CFD8 | Special ammo (types 24, 118) |
| InitScaledPlatformEntity | 0x80041BD8 | Scaled/trigger platforms |
| InitFloatingPlatformEntity | 0x80045EE4 | Floating platforms (types 39, 52) |
| InitSuperWillieCollectible | 0x8003E0FC | Super Willie heads |
| InitScaleResetCollectible | 0x8002F1C8 | Yellow chevron (type 29) |
| InitTriggerZoneEntity | 0x800441AC | Trigger zones (85, 104, 105) |
| InitGridLineEntity | 0x80035EBC | Grid helpers (89, 97, 98, 110, 111) |
| InitDirectionalScaledEntity | ??? | Types 37, 38 |
| InitSoundEmittingEnemy | ??? | Types 47, 48 |
