---
title: "Unknown Functions Analysis"
category: reference
tags: [reference, unknown, functions]
---

# Unknown Functions Analysis

This document tracks unknown (`FUN_*`) functions discovered via Ghidra analysis and their relationships to documented functions.

## Summary

Based on Ghidra call graph analysis from `main`, many functions have been identified and named.

## Recently Named Functions (Session: 2026-01-17)

### Sound/Audio System
| Address | Name | Purpose |
|---------|------|---------|
| 0x8007bfb8 | `InitSPUDefaults` | SPU init (reverb off, volume setup) |
| 0x8007c2e0 | `PopSPUUploadBlock` | SPU upload block cleanup |
| 0x80081c0c | `GetLevelSoundTableEntry` | Sound lookup table accessor |
| 0x80038cac | `PlayCDAudioTrack` | CD audio playback |
| 0x80038e0c | `StopCDAudio` | Stop CD audio playback |

### Memory/Heap System
| Address | Name | Purpose |
|---------|------|---------|
| 0x800143a4 | `InitHeapConfig` | Set heap base/size |
| 0x80014854 | `InitHeapFreeList` | Initialize free list (100 entries) |
| 0x80014968 | `ClearHeapBlocks` | Iterate/clear heap blocks |

### VRAM Slot Management
| Address | Name | Purpose |
|---------|------|---------|
| 0x800138f0 | `AllocateVRAMSlot` | Allocate slot with size calculation |
| 0x80013ab0 | `FreeVRAMSlot` | Free VRAM slot |
| 0x800149e8 | `InitVRAMSlotArray` | Init 88 VRAM slots |
| 0x80014a9c | `FindVRAMSlotBySize` | Find slot using bitmask |
| 0x80014cf8 | `AllocateVRAMSlotAligned` | Aligned allocation with block split |
| 0x800150c4 | `GetMaxVRAMSlotSize` | Get max slot size in list |
| 0x80015134 | `FreeAndCoalesceVRAMSlot` | Free and merge adjacent blocks |

### Entity System
| Address | Name | Purpose |
|---------|------|---------|
| 0x80081fd0 | `ClearEntityPoolArray` | Clear 256-entry entity pool |
| 0x8002086c | `InitEntityWithTable` | Init entity with vtable |
| 0x8002453c | `IsEntityOffScreen` | AABB bounds check vs camera |
| 0x800255c8 | `GetEntitySpawnData` | Get spawn data from context |
| 0x80025630 | `SetEntitySpawnData` | Set spawn data in context |

### Graphics/Texture System
| Address | Name | Purpose |
|---------|------|---------|
| 0x80019f2c | `SetTexturePageParams` | Texture page setup |
| 0x8001f944 | `InitLayerScrollContext` | Layer scroll context setup |
| 0x80039c4c | `SetupMovieDisplay` | Display env for movie playback |
| 0x8001889c | `InitSpriteObject` | Initialize sprite object |

### Player/Collectible System
| Address | Name | Purpose |
|---------|------|---------|
| 0x8002639c | `AddPlayerLives` | Lives (+0x11, max 99) |
| 0x8002646c | `AddPlayerOrbs` | Orbs/Clay (+0x12, 100→1up) |
| 0x800262cc | `AddSwirlys` | Swirly Qs (+0x13, max 20) |
| 0x80026614 | `AddPhoenixHands` | Phoenix Hands (+0x14, max 7, L1) |
| 0x800266e4 | `AddPhartHeads` | Phart Heads (+0x15, max 7, L2) |
| 0x800267b4 | `AddUniverseEnemas` | Universe Enemas (+0x16, max 7, R1) |
| 0x80026884 | `Add1970Icons` | 1970 Icons (+0x19, max 3) |
| 0x80026a48 | `AddSuperWillies` | Super Willies (+0x1C, max 7, R2) |
| 0x8002615c | `ClearHamsterCount` | Clear hamster count (+0x1A) |

### UI/HUD System
| Address | Name | Purpose |
|---------|------|---------|
| 0x800275e4 | `UpdateDigitDisplay` | Update digit sprite for counter |
| 0x800279c8 | `UpdateHUDItemVisibility` | Set HUD item visibility flag |

### Stubs
| Address | Name | Purpose |
|---------|------|---------|
| 0x80015434 | `StubVibrateOff` | Empty stub (PSX vibration) |

## Progress Summary

- **Total Functions**: 1701
- **Named This Session**: 38 functions (+ 5 renamed after verification)
- **Unknown (FUN_*) Remaining**: ~140

## Verified Player State Offsets (from Ghidra)

All collectible add functions follow the same pattern:
- Add value to `PlayerState + offset`
- Clamp to max value
- Notify HUD entity via message `(0x1013, msg_id, 0)`

| Offset | Max | HUD Msg | Field | Add Function |
|--------|-----|---------|-------|--------------|
| 0x11 | 99 | 2 | Lives | AddPlayerLives |
| 0x12 | 99→0 | 1 | Orbs/Clay | AddPlayerOrbs |
| 0x13 | 20 | 3 | Swirly Qs | AddSwirlys |
| 0x14 | 7 | 6 | Phoenix Hands (L1) | AddPhoenixHands |
| 0x15 | 7 | 7 | Phart Heads (L2) | AddPhartHeads |
| 0x16 | 7 | 8 | Universe Enemas (R1) | AddUniverseEnemas |
| 0x19 | 3 | 4 | 1970 Icons | Add1970Icons |
| 0x1A | 0 | - | Hamsters | ClearHamsterCount |
| 0x1C | 7 | 9 | Super Willies (R2) | AddSuperWillies |

## Player State Machine Functions (0x8005xxxx-0x8006xxxx)

Many `PlayerCallback_*` functions are state machine handlers. See `docs/systems/player/player-normal.md` for the full state machine documentation.

## Unknown FUN_ Functions by Address Range

### 0x80013xxx - System/Graphics
- FUN_80015434 - Constructor init (called from __main)
- FUN_800149e8 - VRAM slot helper

### 0x80019xxx - Texture/Rendering  
- FUN_80019f2c - Texture setup (called from LoadTileDataToVRAM)

### 0x8001fxxx - Entity/Layer
- FUN_8001f944 - Layer render helper

### 0x80020xxx - Entity Management
- FUN_8002086c - Entity initialization

### 0x80024xxx - Level Loading
- FUN_8002453c - Entity cleanup helper

### 0x80025xxx - Input/Player Init
- FUN_800255c8 - Near LoadTileDataToVRAM
- FUN_80025630 - Near spawn offset functions

### 0x80026xxx - Player State
- FUN_80026614, FUN_800266e4, FUN_800267b4, FUN_80026884, FUN_80026a48
  - Player collectible/unlock related (near AddPlayerOrbs, ResetPlayerUnlocksByLevel)

### 0x80038xxx-0x8003axxx - CD/Movie System
- FUN_80038e0c - CD system init
- FUN_80039c4c through FUN_8003a9b4 - CD streaming/movie playback

### 0x8007bxxx-0x8007cxxx - BLB/Level System
- FUN_8007bfb8 - BLB sector calculation (called from LoadBLBHeader)
- FUN_8007c2e0 - Level state cleanup

### 0x80081xxx - Entity Type System
- FUN_80081c0c - Entity spawn helper
- FUN_80081fd0 - Entity init helper

### 0x80086xxx-0x80088xxx - PSX System
- FUN_80086710, FUN_80086720 - Interrupt handlers
- FUN_80088540, FUN_80088958, FUN_80088970 - Graphics/sound init

## Naming Conventions Found

Based on analysis, functions follow these patterns:
- `Init*` - Initialization functions
- `Create*Entity` - Entity factory functions  
- `*Callback` / `*_` followed by address - State machine callbacks
- `Player*` - Player-specific logic
- `Entity*` - Generic entity operations
- `FUN_*` - Not yet analyzed/named

## Priority for Analysis

### ✅ COMPLETED: High Priority (Core Systems)

All high-priority core system functions have been named:

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007bfb8 | `InitSPUDefaults` | SPU init (reverb off, volume, CD mix) |
| 0x8007c2e0 | `PopSPUUploadBlock` | SPU upload block cleanup |
| 0x80081c0c | `GetLevelSoundTableEntry` | Sound lookup: `table[(param_2 & 0xff) * 0x10 + (param_3 & 0xf)]` |
| 0x80081fd0 | `ClearEntityPoolArray` | Clear 256-entry pool at GameState+0x16c |
| 0x8007a594 | `PeekNextPlaybackMode` | Peek next playback sequence mode |
| 0x8007bac8 | `GetDemoDataPtr` | Returns ctx+0x54+0x10 (demo data) |

### ✅ COMPLETED: Medium Priority (Gameplay)

| Address | Name | Purpose |
|---------|------|---------|
| 0x80026614 | `AddPhoenixHands` | Powerup L1 (+0x14, max 7) |
| 0x800266e4 | `AddPhartHeads` | Powerup L2 (+0x15, max 7) |
| 0x800267b4 | `AddUniverseEnemas` | Powerup R1 (+0x16, max 7) |
| 0x80026884 | `Add1970Icons` | Secret item (+0x19, max 3) |
| 0x80026a48 | `AddSuperWillies` | Powerup R2 (+0x1C, max 7) |
| 0x80038e0c | `StopCDAudio` | Stop CD audio playback |

### Low Priority (Internal)
- 0x80086xxx, 0x80088xxx - PSX SDK internals (library code, don't rename)

## Workflow for Naming Functions

1. Check xrefs to understand context
2. Decompile and analyze logic
3. Rename in Ghidra using MCP tools:
   ```
   mcp_ghidra_functions_rename(address="0x80019f2c", new_name="SetupTextureRenderState")
   ```
4. Update this document
5. Add to symbol_addrs.txt if decompiling

## Related Documents

- [Game Functions Reference](game-functions.md)
- [Entity System](../systems/entities.md)  
- [Player State Machine](../systems/player/player-normal.md)
- [BLB Data Format](../blb-data-format.md)
