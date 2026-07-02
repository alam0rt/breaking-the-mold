---
title: "C Migration Plan: ASM → C Decompilation Roadmap"
category: plans
tags: [plans, migration, plan]
---

# C Migration Plan: ASM → C Decompilation Roadmap

**Created:** January 31, 2026  
**Status:** Active  
**Reference:** `docs/C_MIGRATION_GUIDE.md`

## Overview

This document provides a detailed, phased plan for migrating functions from assembly (ASM) to C. Based on Ghidra analysis of 2,412 functions, we've identified logical groupings ordered by:

1. **Dependency** - Functions with fewer dependencies first
2. **Complexity** - Simple functions before complex ones
3. **Cohesion** - Related functions grouped together
4. **Documentation** - Well-documented systems prioritized

**Current State:**
- Total functions: ~2,412 (named in Ghidra)
- Already started: 2 files (`bootstrap.c`, `GetFrameReadyFlag.c`)
- Matching binary: ✅ SHA1 verified

---

## Phase 1: Foundation Functions (Week 1-2)

**Goal:** Build confidence with simple, dependency-free functions.

### 1.1 Simple Stubs and Getters

These functions are trivial (< 10 instructions) with no dependencies:

| Function | Address | Size | Module | Notes |
|----------|---------|------|--------|-------|
| `GetFrameReadyFlag` | 0x800131F0 | 16 | RENDER | ✅ **Already started** |
| `TriggerBufferSwapIfReady` | 0x80013200 | 68 | RENDER | Simple flag check + call |
| `SetVideoModePAL` | 0x80013244 | 4 | RENDER | Empty stub (returns) |
| `StubEmpty` | 0x80015424 | 16 | RENDER | Returns without action |
| `StubVibrateOff` | 0x80015434 | 8 | RENDER | Empty vibration stub |
| `StubReturnZero` | 0x80020288 | 8 | ENTITY | `return 0;` |
| `NullStubFunction` | 0x800374B4 | 16 | OBJECT | Empty stub |
| `PassThroughFunction` | 0x8007A150 | 8 | GAMELOOP | Returns input unchanged |
| `ReturnZero` | 0x8007ADB8 | 16 | GAMELOOP | `return 0;` |
| `ReturnZero2` | 0x8007BAF0 | 16 | GAMELOOP | `return 0;` |

**File:** `src/Game/RENDER/stubs.c` + `src/Game/common_stubs.c`

### 1.2 Simple Field Accessors

Single-line field reads/writes:

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `ZeroEntityField` | 0x8001954C | 12 | Sets entity field to 0 |
| `GetSpriteFrameCount` | 0x8001963C | 20 | Read frame count |
| `ClearEntityStateFlag` | 0x8006E8DC | 8 | Clear flag |
| `SetEntityStateFlagWithValue` | 0x8006E8E4 | 16 | Set flag |
| `EntitySetReadyFlag` | 0x80046FE4 | 32 | Set ready state |

**Expected effort:** 2-4 hours for all Phase 1 functions

---

## Phase 2: VRAM & Primitive Allocation (Week 2-3)

**Goal:** Decompile the graphics primitive subsystem.

### 2.1 Primitive Slot Allocators

These follow a repetitive pattern, ideal for batch decompilation:

| Function | Address | Size | Prim Size | Notes |
|----------|---------|------|-----------|-------|
| `AllocPrimSlot_0x08` | 0x80013718 | 72 | 8 bytes | SPRT_8 |
| `AllocPrimSlot_0x14` | 0x800136C8 | 80 | 20 bytes | POLY_F3 |
| `AllocPrimSlot_0x14_Alt` | 0x800137B0 | 80 | 20 bytes | Alternate |
| `AllocPrimSlot_0x18` | 0x80013800 | 80 | 24 bytes | POLY_G3 |
| `AllocPrimSlot_0x1C` | 0x80013850 | 80 | 28 bytes | POLY_F4 |
| `AllocPrimSlot_0x24` | 0x800138A0 | 88 | 36 bytes | POLY_GT3 |
| `AllocPrimSlot_0x28` | 0x80013760 | 80 | 40 bytes | POLY_GT4 |

**File:** `src/Game/RENDER/prim_alloc.c`

### 2.2 VRAM Slot Management

VRAM texture page allocation:

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `InitVRAMSlotTable` | 0x80013B1C | 500 | Initialize slot array |
| `AllocateVRAMSlot` | 0x800138F0 | 448 | Core allocator |
| `FreeVRAMSlot` | 0x80013AB0 | 108 | Free slot |
| `ProcessPendingVRAMSlotFrees` | 0x80014134 | 324 | Deferred cleanup |
| `InitVRAMSlotArray` | 0x800149E8 | 180 | Array init |
| `FindVRAMSlotBySize` | 0x80014A9C | 604 | Slot lookup |
| `AllocateVRAMSlotAligned` | 0x80014CF8 | 892 | Aligned allocation |
| `FindFreeVRAMSlotEntry` | 0x80015074 | 80 | Find free entry |
| `GetMaxVRAMSlotSize` | 0x800150C4 | 112 | Max size query |
| `FreeAndCoalesceVRAMSlot` | 0x80015134 | 752 | Free with merge |
| `CalculateVRAMCoordinates` | 0x80014278 | 300 | X/Y from slot |

**File:** `src/Game/RENDER/vram.c`

### 2.3 CLUT Slot Management

Color lookup table allocation (follows VRAM pattern):

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `AllocateCLUTSlot` | 0x80013D10 | 576 | CLUT allocator |
| `FreeCLUTSlot` | 0x80013F50 | 484 | Free CLUT |

**File:** `src/Game/RENDER/clut.c`

**Dependencies:** Requires `common.h` structs for VRAM/CLUT state.

---

## Phase 3: Heap Management (Week 3)

**Goal:** Decompile memory allocation subsystem.

### 3.1 Heap Functions

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `InitHeapConfig` | 0x800143A4 | 76 | Setup heap base/size |
| `AllocateFromHeap` | 0x800143F0 | 436 | malloc equivalent |
| `FreeFromHeap` | 0x800145A4 | 688 | free equivalent |
| `InitHeapFreeList` | 0x80014854 | 276 | Initialize free list |
| `ClearHeapBlocks` | 0x80014968 | 128 | Clear all blocks |

**File:** `src/Game/RENDER/heap.c`

**Note:** This is a simple first-fit allocator. The decompiled code will be useful for understanding memory layout.

---

## Phase 4: Coordinate & Position Helpers (Week 4)

**Goal:** Decompile world/screen transformation functions.

### 4.1 Position Getters

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `GetWorldPositionX` | 0x8001A2CC | 28 | Entity X position |
| `GetWorldPositionY` | 0x8001A2E8 | 28 | Entity Y position |
| `GetEntityXPosition` | 0x800204EC | 224 | Complex X getter |
| `GetEntityYPosition` | 0x80020450 | 156 | Complex Y getter |

### 4.2 Coordinate Transforms

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `WorldToScreenX` | 0x8001A33C | 28 | World → screen |
| `WorldToScreenYWithParallax` | 0x8001A358 | 84 | With parallax offset |
| `ScaleXByEntityScale` | 0x8001A26C | 48 | X scaling |
| `ScaleYByEntityScale` | 0x8001A29C | 48 | Y scaling |
| `TransformXCoord` | 0x80020688 | 188 | Full transform |
| `TransformYCoord` | 0x800205CC | 188 | Full transform |
| `CalculateParallaxXOffset` | 0x8001A304 | 8 | Parallax calc |
| `CalculateParallaxXOffsetAlt` | 0x8001A30C | 48 | Alternate |

**File:** `src/Game/ENTITY/coordinates.c`

---

## Phase 5: Collision Detection (Week 5)

**Goal:** Decompile collision detection core.

### 5.1 Primitive Collision

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `CheckPointInBox` | 0x8001B360 | 144 | Point-box test |
| `CheckBoxOverlap` | 0x8001B3F0 | 140 | Box-box test |
| `CollisionCheckWrapper` | 0x8001B47C | 140 | Generic wrapper |
| `LineSegmentIntersectsRect` | 0x8003A394 | 496 | Line-rect test |

### 5.2 Entity Collision

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `CheckEntityCollision` | 0x800226F8 | 580 | Main collision check |
| `CheckEntityBoxCollision` | 0x8001B8C4 | 104 | Box collision |
| `EntityBroadcastBoxCollision` | 0x8001B508 | 140 | Broadcast box |
| `EntityBroadcastPointCollision` | 0x8001B72C | 408 | Broadcast point |
| `BroadcastPointCollision` | 0x8002293C | 544 | Point collision |
| `BroadcastBoxCollision` | 0x80022B5C | 568 | Box collision |
| `CheckPointCollisionAndNotify` | 0x800224D0 | 552 | With notification |
| `TestPointAgainstEntities` | 0x800230B4 | 224 | Point vs entity list |
| `CheckBoxCollision` | 0x80023194 | 552 | Box vs entities |

### 5.3 Offscreen Checks

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `IsEntityOffScreen` | 0x8001B92C | 1156 | Main offscreen |
| `IsEntityOffscreenLeft` | 0x8001BDB0 | 248 | Left check |
| `IsEntityOffscreenRight` | 0x8001BF8C | 248 | Right check |
| `IsEntityOffscreenLeftSimple` | 0x8001BEA8 | 24 | Simple left |
| `IsEntityOffscreenRightSimple` | 0x8001C084 | 24 | Simple right |
| `IsPositionOffscreenLeft` | 0x8001BEC0 | 204 | Position left |
| `IsPositionOffscreenRight` | 0x8001C09C | 220 | Position right |
| `IsEntityOffScreenY` | 0x8001C178 | 492 | Vertical check |

**File:** `src/Game/ENTITY/collision.c`

---

## Phase 6: Entity Core System (Week 6-7)

**Goal:** Decompile the entity system foundation.

### 6.1 Entity Initialization

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `InitEntityStruct` | 0x8001A0C8 | 280 | Core entity init |
| `InitBasicEntityWithVtable` | 0x8001543C | 48 | With vtable |
| `InitFullEntityWithAnimation` | 0x8001C6C8 | 88 | With anim |
| `InitEntityWithSprite` | 0x8001C868 | 280 | With sprite |
| `InitEntityAnimationState` | 0x8001C980 | 224 | Animation init |
| `InitEntitySprite` | 0x8001C720 | 328 | Sprite init |
| `InitEntityWithTable` | 0x8002086C | 68 | With callback table |

### 6.2 Entity State Machine

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `EntitySetState` | 0x8001EAAC | 364 | **Core state setter** |
| `EntitySetCallback` | 0x8001EC18 | 168 | Callback assignment |
| `EntityProcessCallbackQueue` | 0x8001E928 | 388 | Callback dispatch |
| `InvokeEntityCallback` | 0x80020744 | 160 | Callback invoker |

### 6.3 Entity Lists

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `AddToZOrderList` | 0x80020F68 | 276 | Z-sorted insert |
| `AddToUpdateQueue` | 0x80021190 | 276 | Tick queue insert |
| `AddToXPositionList` | 0x8002107C | 276 | X-sorted insert |
| `AddToEntityList` | 0x800212A4 | 172 | Generic list add |
| `AddToEntityDefList` | 0x80021350 | 88 | Definition list |
| `AddEntityToSortedRenderList` | 0x800213A8 | 488 | Render list |
| `AddEntityToBothLists` | 0x80021B48 | 68 | Tick + render |
| `InsertIntoZSortedRenderList` | 0x80021B8C | 420 | Z-sorted render |
| `RemoveFromTickList` | 0x80021D30 | 144 | Remove from tick |
| `RemoveFromRenderList` | 0x80021DC0 | 144 | Remove from render |
| `RemoveFromUpdateQueue` | 0x80020A1C | 88 | Remove from update |
| `RemoveFromZOrderList` | 0x80020A74 | 192 | Remove from z-list |
| `RemoveEntityFromUpdateQueue` | 0x80021E50 | 140 | Wrapper |
| `RemoveEntityFromAllLists` | 0x80022074 | 420 | Full cleanup |
| `ChangeRenderZOrder` | 0x80021EDC | 408 | Change z-order |

### 6.4 Entity Destruction

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `DestroyEntity` | 0x80020974 | 168 | Main destructor |
| `DestroyEntityAndFreeMemory` | 0x8001CA60 | 204 | With memory free |
| `EntityDestructCallback` | 0x80020F48 | 32 | Destruct callback |
| `DeferredEntityRemoval` | 0x80020C74 | 180 | Deferred removal |
| `EntityRemoval` | 0x80020D28 | 244 | Removal handler |
| `FreeWithCallback` | 0x8001A1E0 | 140 | Free with callback |

### 6.5 Entity Tick/Render

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `EntityTickLoop` | 0x80020E1C | 100 | **Main tick loop** |
| `RenderEntities` | 0x80020E80 | 200 | **Main render loop** |
| `EntityTickLoopWithCamera` | 0x80020B34 | 320 | With camera update |

**File:** `src/Game/ENTITY/core.c` + `src/Game/ENTITY/lists.c`

---

## Phase 7: Animation System (Week 8)

**Goal:** Decompile sprite animation framework.

### 7.1 Animation Setters

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `SetEntitySpriteId` | 0x8001D080 | 48 | Sprite ID setter |
| `SetAnimationFrameIndex` | 0x8001D0C0 | 48 | Frame index |
| `SetAnimationFrameCallback` | 0x8001D0F0 | 40 | Frame callback |
| `SetAnimationLoopFrameIndex` | 0x8001D118 | 88 | Loop frame (index) |
| `SetAnimationLoopFrame` | 0x8001D170 | 80 | Loop start frame |
| `SetAnimationTargetFrameIndex` | 0x8001D1C0 | 48 | Target frame (index) |
| `SetAnimationSpriteCallback` | 0x8001D1F0 | 40 | Sprite callback |
| `SetAnimationActive` | 0x8001D218 | 40 | Active flag |
| `SetAnimationSpriteFlags` | 0x8001D0B0 | 16 | Sprite flags |
| `EntitySetRenderFlags` | 0x8001D240 | 80 | Render flags |

### 7.2 Animation Updates

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `TickEntityAnimation` | 0x8001D290 | 556 | Main animation tick |
| `AdvanceAnimationFrame` | 0x8001D4BC | 152 | Frame advance |
| `ApplyPendingSpriteState` | 0x8001D554 | 500 | Apply pending |
| `UpdateSpriteFrameData` | 0x8001D748 | 576 | Frame data update |
| `UpdateEntityRender` | 0x8001D988 | 3120 | **Main render update** |
| `UploadEntityTextureIfDirty` | 0x8001E5B8 | 244 | Texture upload |
| `FindFrameIndexByValue` | 0x8001E6AC | 228 | Frame lookup |
| `StartAnimationSequence` | 0x8001E790 | 40 | Start sequence |
| `StepAnimationSequence` | 0x8001E7B8 | 368 | Step sequence |
| `ApplyAnimationPositionOffsets` | 0x8001CC6C | 116 | Position offsets |

**File:** `src/Game/ENTITY/animation.c`

---

## Phase 8: Player State Helpers (Week 9)

**Goal:** Decompile player collectible management.

### 8.1 Collectible Adders

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `AddSwirlys` | 0x800262CC | 208 | Add swirlys |
| `AddPlayerLives` | 0x8002639C | 208 | Add lives |
| `AddPlayerOrbs` | 0x8002646C | 424 | Add orbs |
| `AddPhoenixHands` | 0x80026614 | 208 | Add Phoenix Hand |
| `AddPhartHeads` | 0x800266E4 | 208 | Add Phart Head |
| `AddUniverseEnemas` | 0x800267B4 | 208 | Add Universe Enema |
| `Add1970Icons` | 0x80026884 | 208 | Add 1970 Icons |
| `AddSuperWillies` | 0x80026A48 | 208 | Add Super Willies |
| `AdjustPlayerStats` | 0x80026954 | 244 | Generic stat adjust |

### 8.2 State Reset

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `initPlayerState` | 0x800260D0 | 140 | Init player state |
| `DecrementPlayerLives` | 0x800262AC | 32 | Decrement lives |
| `ClearHamsterCount` | 0x8002615C | 8 | Clear hamster count |
| `ResetPlayerCollectibles` | 0x80026164 | 112 | Reset collectibles |
| `InitializePlayerState` | 0x800261D4 | 140 | Full init |
| `AdvanceLevelAndClearCollectibles` | 0x80026260 | 76 | Level advance |
| `ResetPlayerUnlocksByLevel` | 0x80026B18 | 96 | Reset unlocks |

**File:** `src/Game/OBJECT/player_state.c`

---

## Phase 9: BLB Accessors (Week 10)

**Goal:** Decompile BLB file format accessors.

### 9.1 Level/Movie Accessors

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `GetLevelCount` | 0x8007A9B0 | 20 | Level count |
| `GetLevelAssetIndex` | 0x8007A9C4 | 36 | Level → asset |
| `getLevelName` | 0x8007AA08 | 32 | Level name |
| `GetLevelFlagByIndex` | 0x8007AA28 | 36 | Level flags |
| `GetCurrentLevelAssetIndex` | 0x8007AA4C | 68 | Current level |
| `GetCurrentLevelDisplayName` | 0x8007AA90 | 180 | Display name |
| `GetAssetCount` | 0x8007ACDC | 20 | Asset count |
| `GetMovieEntryByIndex` | 0x8007ACF0 | 64 | Movie entry |
| `GetCurrentMovieReserved` | 0x8007AD30 | 76 | Movie reserved |
| `GetCurrentMovieFilename` | 0x8007ADC8 | 76 | Movie filename |
| `GetMovieUnknown00` | 0x8007AE14 | 68 | Movie field |
| `GetMovieSectorCount` | 0x8007AE58 | 68 | Sector count |

### 9.2 Tile/Layer Accessors

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `GetTileHeaderPtr` | 0x8007B4B8 | 12 | Tile header |
| `GetSecondaryColorPtr` | 0x8007B4C4 | 12 | Secondary color |
| `GetPaletteGroupCount` | 0x8007B4D0 | 40 | Palette count |
| `GetPaletteDataPtr` | 0x8007B4F8 | 56 | Palette data |
| `GetPaletteAnimData` | 0x8007B530 | 12 | Palette anim |
| `GetTotalTileCount` | 0x8007B53C | 76 | Tile count |
| `GetLayerCount` | 0x8007B6C8 | 20 | Layer count |
| `GetTilemapDataPtr` | 0x8007B6DC | 36 | Tilemap data |
| `GetLayerEntry` | 0x8007B700 | 76 | Layer entry |
| `GetTileAttributeData` | 0x8007B79C | 12 | Tile attributes |
| `GetEntityCount` | 0x8007B7A8 | 20 | Entity count |
| `GetEntityDataPtr` | 0x8007B7BC | 12 | Entity data |

**File:** `src/Game/GAMELOOP/blb_accessor.c`

---

## Phase 10: Audio System (Week 11)

**Goal:** Decompile audio playback system.

### 10.1 Sound Effects

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `InitSPUDefaults` | 0x8007BFB8 | 208 | SPU init |
| `UploadAudioToSPU` | 0x8007C088 | 600 | Upload samples |
| `PopSPUUploadBlock` | 0x8007C2E0 | 84 | Pop upload |
| `ShutdownSPUAndResetSoundState` | 0x8007C334 | 56 | Shutdown |
| `PlaySoundEffect` | 0x8007C388 | 988 | Play SFX |
| `PlayGameSoundById` | 0x8007C764 | 84 | Play by ID |
| `StopSPUVoice` | 0x8007C7B8 | 52 | Stop voice |
| `StopAllSPUVoices` | 0x8007C7EC | 44 | Stop all |
| `CalculateStereoVolume` | 0x8007C818 | 528 | Stereo calc |
| `SetVoicePanning` | 0x8007CA28 | 116 | Panning |

### 10.2 CD Audio

| Function | Address | Size | Notes |
|----------|---------|------|-------|
| `StartCDAudioForLevel` | 0x8007CA9C | 132 | Start CD |
| `StopCDStreaming` | 0x8007CB20 | 36 | Stop CD |
| `SaveAndMuteAllVoicePitches` | 0x8007CB44 | 124 | Mute all |
| `ResumeAllVoicePitches` | 0x8007CBC0 | 104 | Resume all |
| `SetStereoMode` | 0x8007CC28 | 36 | Stereo mode |
| `SetReverbLevel` | 0x8007CC4C | 28 | Reverb level |
| `SetAudioVolume` | 0x8007CC68 | 80 | Volume |
| `TickCDStreamBuffer` | 0x8007CCB8 | 68 | Stream tick |

**File:** `src/Game/GAMELOOP/audio.c`

---

## Summary & Timeline

| Phase | Functions | Est. Hours | Week |
|-------|-----------|------------|------|
| 1. Foundation | ~15 | 4 | 1-2 |
| 2. VRAM/Prim | ~20 | 16 | 2-3 |
| 3. Heap | 5 | 8 | 3 |
| 4. Coordinates | ~12 | 10 | 4 |
| 5. Collision | ~18 | 16 | 5 |
| 6. Entity Core | ~35 | 24 | 6-7 |
| 7. Animation | ~20 | 16 | 8 |
| 8. Player State | ~15 | 12 | 9 |
| 9. BLB Accessors | ~25 | 16 | 10 |
| 10. Audio | ~20 | 12 | 11 |
| **Total** | **~185** | **~134** | **11 weeks** |

---

## Splat YAML Changes Required

For each phase, update `SLES_010.90.yaml`:

```yaml
# Phase 2: VRAM example
- [0x39F0, c, Game/RENDER/vram]      # Was: asm, Game/RENDER

# Phase 6: Entity core example  
- [0xA8C8, c, Game/ENTITY/core]      # Was: asm, Game/ENTITY
```

---

## Verification Checklist

For each function:
- [ ] Ghidra decompilation reviewed
- [ ] m2c output generated
- [ ] C code written
- [ ] `make clean && make check` passes
- [ ] SHA1 matches original binary

---

## Next Steps

1. **Complete Phase 1** - Finish `GetFrameReadyFlag.c` and stubs
2. **Document structs** - Export VRAM/Entity structs to `include/`
3. **Create test scripts** - Validate function behavior with PCSX-Redux
4. **Begin Phase 2** - Start primitive allocators

---

## Appendix: Function Counts by Module

Based on Ghidra analysis:

| Module | Total | Named | % Named |
|--------|-------|-------|---------|
| RENDER | 88 | 85 | 97% |
| ENTITY | 208 | 195 | 94% |
| OBJECT | 709 | 650 | 92% |
| PLAYER | 308 | 290 | 94% |
| GAMELOOP | 382 | 360 | 94% |
| LIBS | 559 | 400 | 72% |
| **Total** | **2,254** | **1,980** | **88%** |

*Note: LIBS functions are PSY-Q SDK and won't be decompiled.*
