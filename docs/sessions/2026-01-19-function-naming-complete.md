---
title: "Session Summary: FUN_ Function Naming Complete"
category: sessions
tags: [sessions, 2026, function, naming]
---

# Session Summary: FUN_ Function Naming Complete

**Date**: January 19, 2026  
**Result**: ✅ All 236 FUN_ functions named in Ghidra

## Overview

This session completed the systematic naming of all unnamed `FUN_` functions in the Skullmonkeys (PAL SLES-01090) Ghidra project. Starting from 236 unnamed functions, all have now been analyzed and given descriptive names based on their decompiled code.

## Major Systems Discovered

### 1. VRAM Slot Entity System (4 functions)
**Address Range**: 0x800318e0 - 0x80031da0

Entities that allocate dedicated VRAM texture slots for dynamic rendering:
- `InitVRAMSlotEntity` - Allocates 8x16 VRAM slot
- `DestroyVRAMSlotEntity` - Frees VRAM slot
- `RenderVRAMSlotOverlay` - Complex SPRT/TILE_1/DR_OFFSET rendering
- `CheckVRAMSlotPixelColor` - StoreImage + magenta marker check (0x3c0f)

### 2. Multi-Part Entity System (3 functions)
**Address Range**: 0x80032124 - 0x80032800

Composite entities with 5 sub-entity array at +0x104:
- `MultiPartEntityTick` - Iterates sub-entities, sends messages
- `MultiPartEntityRenderTick` - Texture upload + VRAM check
- `MultiPartEntityMessageHandler` - Flag operations (0x100f/0x1009/0x1010)

### 3. Path-Following Entity System (33 functions)
**Address Range**: 0x80032e0c - 0x80080xxx

Movement along predefined path data:
- Init functions: `InitPathFollowEntity`, `InitPathFollowEntityAlt`
- Update: `UpdateEntityAlongPath`, `EntityPathMovementUpdate`
- Rendering: `RenderPathEntitySegments`
- Path tables at 0x8009bc08 (200 entries) and 0x8009bf28 (61 entries)

### 4. Projectile System (24 functions)
**Address Range**: 0x8004fxxx - 0x80070xxx

Complete projectile mechanics including homing:
- Core: `SpawnAngledProjectile`, `ProjectileEntityTick`, `ProjectileApplyVelocity`
- Homing: `HomingProjectileTick`, `HomingProjectile_TrackTarget`, `HomingMissileTrackTarget`
- Math: `CalculateSineValue` - 256-entry lookup table at 0x8009c09c
- Collision: `ProjectileCollisionCallback`, `ProjectileTickWithCollision`

### 5. Player Death System (30 functions)
**Address Range**: 0x80036xxx - 0x8006axxx

Death animations and particle effects:
- States: `PlayerState_DeathStart`, `PlayerState_Death`
- Animation: `PlayerDeathGrowingTick`, `DeathAnimationTick`
- Effects: `InitPlayerDeathParticle`, `SpawnPlayerDeathEffect`
- Menu spawn: `PlayerCallback_DeathSpawnMenuEntities`

### 6. Sound Entity System (41 functions)
**Address Range**: 0x8003cxxx - 0x80075xxx

Entities with SPU voice management:
- Init: `InitSoundEmittingEnemy`, `EntityInitSoundEmitterState`
- Update: `SoundEmitterTickCallback`, `SoundEmitterWithPanningTick`
- Cleanup: `DestroySoundEntity`, `SoundEmitterDestroyCallback`

Key pattern: Sound entities store voice index at +0x110, must stop voice on destroy.

### 7. Menu System (47 functions)
**Address Range**: 0x80019xxx - 0x80082xxx

UI elements and navigation:
- Init: `InitMenuEntity`, `InitMenuStage1`-`InitMenuStage4`
- Callbacks: `MenuTickCallback`, `MenuInputHandler`
- Sound: `Menu_PlayConfirmSound`, `Menu_PlaySelectSoundIfEnabled`
- Global data: `g_MenuCursorSprites`, `g_MenuButtonSprites`

### 8. Timer-Based Entities (37 functions)
**Address Range**: 0x80026xxx - 0x80080xxx

Countdown-driven state transitions:
- `TimerEntityTick` - Decrements +0x100, triggers on 0
- `PlatformTimerTickCallback` - Platform activation
- `HazardTimerTickCallback` - Hazard timing
- `EntityTimerDeathWithParticles` - Timed death spawns

### 9. Overlay/Rendering (7 functions)
**Address Range**: 0x80028xxx - 0x80038xxx

Full-screen effects:
- `InitFadeOverlayEntity`, `CreateFadeOverlayEntity`
- `RenderFullScreenTileOverlay` - Full-screen tile rendering
- `OverlayEntityCallback` - State-based overlay updates

### 10. Game State Notification (11 functions)
**Address Range**: 0x80034xxx - 0x8007axxx

State machine coordination:
- `InvokeGameStateCallback` - Call registered callback
- `NotifyGameStateZero`, `NotifyGameStateOne` - State notifications
- `InitGameState`, `ResetGameStateInputAndContext`

## Key Discoveries

### Sine/Cosine Lookup Table
- **Address**: 0x8009c09c
- **Size**: 256 entries
- **Format**: Fixed-point ×0x1000 (4096)
- Used by homing projectiles for angle calculations

### Path Animation Tables
- **Primary**: 0x8009bc08 (200 entries)
- **Secondary**: 0x8009bf28 (61 entries)
- Used by path-following entities for movement interpolation

### Menu Sprite Globals
- `g_MenuCursorSprites` - Cursor sprite table
- `g_MenuButtonSprites` - Button sprite table

### Entity Timer Locations
- +0x100: Primary timer (32-bit or 16-bit)
- +0x104: Secondary timer
- +0x106: Lifetime counter
- +0x110: SPU voice index

## Documentation Updated

1. **docs/systems/entities.md** - Added subsystem overview, VRAM slot, multi-part, path-following sections
2. **docs/systems/projectiles.md** - Complete function table (95% coverage)
3. **docs/systems/menu-system-complete.md** - 47 functions documented
4. **docs/systems/audio.md** (was sound-system.md, since merged) - 41 sound entity functions added
5. **docs/reference/game-functions.md** - Updated with completion status

## Statistics

| Metric | Value |
|--------|-------|
| Starting FUN_ count | 236 |
| Functions named | 236 |
| FUN_ remaining | 0 |
| Total functions in project | 2411 |
| Coverage | 100% FUN_ named |

## Next Steps

1. Cross-reference named functions with existing documentation
2. Verify function signatures in decompilation
3. Add discovered patterns to copilot-instructions.md
4. Update entity type reference with new callback addresses
