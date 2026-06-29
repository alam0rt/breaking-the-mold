---
title: "GameState Structure Field Analysis"
category: systems
tags: [systems, gamestate, field, analysis]
---

# GameState Structure Field Analysis

Address: `0x8009DC40` (g_GameState)  
Size: `0x1A0` (416 bytes)  
Script: `scripts/ghidra_fix_gamestate.py`

## Analysis Summary

This document records the decompilation analysis of GameState fields. Each field
was traced through Ghidra decompilation to determine its purpose.

## Verified Fields by Region

### Core Mode System (0x00-0x07)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x00 | mode_base_offset | int | Base offset for current mode (always 0xFFFF0000) |
| 0x04 | mode_callback_ptr | ptr | Current mode callback function |

**Source:** `InitGameState`, `SetupAndStartLevel`

### Event FSM / Reserved / Post-Render Context (0x08-0x1B)

**UPDATED 2026-06-12**: The old layer-list-head interpretation for `0x08-0x18`
was stale. `GameState+0x08/+0x0C` mirrors the entity event FSM slot,
`+0x10/+0x14` currently have no confirmed readers, and `+0x18` is a
post-render callback context. Layer contexts are inserted into the shared
entity tick/render lists at `+0x1C/+0x20`, not separate list heads here.

| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x08 | event_marker | s32 | GameState event FSM marker; same tagged-union encoding as `Entity` |
| 0x0C | event_callback | ptr | GameState event handler callback |
| 0x10 | reserved10 | ptr | Reserved/no confirmed decompiler reader; not a layer-list head |
| 0x14 | reserved14 | ptr | Reserved/no confirmed decompiler reader; not a layer-list head |
| 0x18 | postRenderCallbackContext | ptr | Context whose callback at `ctx+0x1C` is invoked after rendering |

**Source:** `TriggerCollectible100CTickCallback` dispatches through `+0x08/+0x0C`.
`main @ 0x800828B0` calls through `postRenderCallbackContext+0x1C` after
`RenderEntities` and `DrawSync(0)`.

**Disproved:** The January layer-list-head names were based on stale Ghidra
datatypes. `AddLayerToRenderList_Medium @ 0x80021778` and related layer setup
insert layer render contexts into the shared entity lists at `+0x1C/+0x20`.

### Entity Lists (0x1C-0x33)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x1C | tick_list_head | ptr | Z-sorted entity tick list (linked list) |
| 0x20 | render_list_head | ptr | Z-sorted entity render list |
| 0x24 | collision_list_head | ptr | Entity collision/update queue |
| 0x28 | entity_spawn_list | ptr | Raw entity defs from Asset 501 |
| 0x2C | player_entity_alt | ptr | Player entity alternate reference |
| 0x30 | player_entity_ptr | ptr | Main player entity pointer |

**Source:** `AddLayerToRenderList_*`, `FreeEntityLists`, `EntityTickLoopWithCamera`

### Deferred Removal (0x34-0x3B)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x34 | entity_pending_removal | ptr | Entity marked for deferred removal |
| 0x38 | removal_mode | int | 0=all lists, 1=keep render |

**Source:** `DeferredEntityRemoval`

### Camera System (0x44-0x67)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x44 | camera_x | s16 | Camera X position (pixels) |
| 0x46 | camera_y | s16 | Camera Y position (pixels) |
| 0x48 | level_width_limit | s16 | Level width for X clamping |
| 0x4A | level_height_limit | s16 | Level height for Y clamping |
| 0x4C | camera_velocity_x | int | X velocity (16.16 fixed-point) |
| 0x50 | camera_velocity_y | int | Y velocity (16.16 fixed-point) |
| 0x54 | player_render_offset_x | s16 | Player X offset for rendering |
| 0x56 | player_render_offset_y | s16 | Player Y offset for rendering |
| 0x58 | scroll_limit_left | u8 | Left scroll limit flag |
| 0x59 | scroll_limit_top | u8 | Top scroll limit flag |
| 0x5A | scroll_limit_right | u8 | Right scroll limit flag |
| 0x5B | scroll_limit_bottom | u8 | Bottom scroll limit flag |
| 0x5C | camera_subpixel_x | s16 | X sub-pixel accumulator |
| 0x5E | camera_subpixel_y | s16 | Y sub-pixel accumulator |
| 0x60 | bounce_active_flag | u8 | Bounce/pickup active flag (set during bounce animations) |
| 0x61 | camera_mode_flags | u8 | Camera mode flags |
| 0x62 | camera_follow_direction | u8 | Player facing for camera tracking (-1=left, 1=right) |
| 0x63 | pause_freeze_flag | u8 | Pause/freeze flag (used by checkpoint) |
| 0x64 | player_hitbox_width | s16 | Player hitbox width (0x28 normal) |
| 0x66 | player_hitbox_y_offset | s16 | Player Y offset (0xFFD0 = -48) |

**Source:** `UpdateCameraPositionSmooth` @ 0x800233c0

### Tile Collision State (0x68-0x73)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x68 | tile_collision_data_ptr | ptr | Tile collision attribute array (from Asset 500) |
| 0x6C | tile_collision_offset_x | s16 | Collision map X offset (tiles) |
| 0x6E | tile_collision_offset_y | s16 | Collision map Y offset (tiles) |
| 0x70 | tile_collision_width | s16 | Collision map width (tiles) |
| 0x72 | tile_collision_height | s16 | Collision map height (tiles) |

**Source:** `InitTileAttributeState` @ 0x80024cf4, `GetTileAttributeAtPosition` @ 0x800241f4

### Trigger Zones / Entity Callbacks (0x74-0x7F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x74 | trigger_zone_data_ptr | ptr | TriggerZone[] array (16-byte AABB+attr records, from LevelDataContext+0x3C) |
| 0x78 | trigger_zone_count | u16 | Trigger zone count (from TileHeader+0x1C, 0 for bosses) |
| 0x7A | (padding) | s16 | Alignment padding (4-byte boundary) |
| 0x7C | entity_callback_table | ptr | Entity type callbacks (121 × 8 bytes) |
| 0x80 | entity_type_count | int | 0x79 = 121 entity types |

**Source:** `InitLayersAndTileState` @ 0x80024778 (writes), `CheckTriggerZoneCollision`
@ 0x800245BC (reads +0x74 as zone array bounded by +0x78). See
`include/Game/trigger_zone.h` for the zone record layout.

### LevelDataContext (0x84-0x103)
128-byte embedded structure containing level asset pointers.
See `docs/systems/level-data-context.md` for details.

### Render/Frame State (0x104-0x11F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x104 | tile_render_state_count | u16 | Tile render state count (total tiles + 1) |
| 0x106 | (padding) | u16 | Alignment padding after tile_render_state_count |
| 0x108 | tile_render_state_ptr | ptr | Tile render buffer (8 bytes per tile, allocated) |
| 0x10C | frame_counter | int | Frame counter (incremented each tick, saved to 0x138 on checkpoint) |
| 0x110 | palette_group_ptrs | ptr | Array of palette render context pointers |
| 0x114 | palette_group_count | u8 | Count of palette groups (byte) |
| 0x115 | field_115 | u8 | Padding |
| 0x116 | spawn_x | s16 | Player spawn X position |
| 0x118 | spawn_y | s16 | Player spawn Y position |
| 0x11A | screen_shake_index | u8 | Screen shake countdown |
| 0x11B | screen_shake_active | u8 | Screen shake active flag (set with screen_shake_index) |
| 0x11C | camera_scroll_speed | int | 0x8000/0xC000/0x10000 based on level flags |

**Source:** `EntityTickLoopWithCamera` (0x10C, 0x11A), `SpawnPlayerAndEntities`, `LoadTileDataToVRAM` (0x104, 0x108, 0x110, 0x114)

### Special Mode State (0x120-0x12F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x120 | special_state_1 | s16 | Glide/boss state 1 |
| 0x122 | special_state_2 | s16 | Glide/boss state 2 |
| 0x124 | layer0_tint_r | u8 | Layer 0 R tint |
| 0x125 | layer0_tint_g | u8 | Layer 0 G tint |
| 0x126 | layer0_tint_b | u8 | Layer 0 B tint |
| 0x12A | special_mode_flag | u8 | Glide/boss screen offset |

**Source:** `InitLayersAndTileState` (layer tints), `SpawnPlayerAndEntities`

### Checkpoint System (0x134-0x14F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x134 | checkpoint_entity_list | ptr | Saved entity list backup |
| 0x138 | checkpoint_saved_frame_counter | int | Saved frame counter |
| 0x13C | pending_player_entity | ptr | Pending player to restore |
| 0x140 | input_state_ptr | ptr | Input state pointer |
| 0x149 | checkpoint_restore_pending | u8 | Restore pending flag |
| 0x14A | checkpoint_active | u8 | Checkpoint active flag |
| 0x14C | hud_entity_ptr | ptr | HUD entity pointer |

**Source:** `SaveCheckpointState` @ 0x8007eaac, `RestoreCheckpointEntities` @ 0x8007eaec

### Vehicle/Entity System (0x164-0x16F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x164 | vehicle_entity_array_ptr | ptr | Vehicle data / 0x40-byte entity array |
| 0x168 | vehicle_entity_count | u16 | Entity array count |
| 0x16C | entity_pool_ptr | ptr | 256-entry entity pool array |

**Source:** `SetupAndStartLevel` (GetVehicleDataPtr), `ClearAlternateEntitySpawnFlags`

**Note:** 0x164 stores result of `GetVehicleDataPtr()` which returns `LevelDataContext+0x34`
(ctx[13]). `ClearAlternateEntitySpawnFlags` iterates it as 0x40-byte entity entries.

### Password System (0x170-0x17B)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x170 | level_active | u8 | Level active flag |
| 0x171-0x17A | password_level_list | u8[10] | Password-selectable levels |
| 0x17B | password_level_count | u8 | Count (max 10) |

**Source:** `InitGameState` - builds list from levels with flag=1 AND index≠0

### Cheat Code Input Buffer (0x17C-0x18C)

**CORRECTED**: This is a cheat code circular buffer, NOT score display.

| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x17C-0x18A | cheat_input_buffer | u16[8] | Last 8 button presses for cheat detection |
| 0x18C | cheat_input_index | u8 | Circular buffer write index (0-7) |
| 0x18D | player_readd_flag | u8 | Player re-add to render list (cheat 0x13) |
| 0x18E | boss_player_type | u8 | Boss mode player type (0=KLOGG,1=BIRDHEAD,2=JOE_HEAD) |
| 0x18F | debug_pause_enable | u8 | Debug frame-step enable (cheat 0x0F) |
| 0x190 | debug_pause_active | u8 | Debug frame-step active state |

**Source:** `CheckCheatCodeInput` @ 0x800820b4  
**Verified in Ghidra**: Confirmed via decompilation 2026-01-19

The buffer stores the last 8 button inputs. When the pause menu is open, each button press
is written to `cheat_input_buffer[cheat_input_index++]`. The function then compares
the circular buffer contents against 22 predefined cheat code sequences in `g_CheatCodeTable`.

### Background Colors (0x199-0x19B)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x199 | bg_color_r | u8 | Background R (default 0x40) |
| 0x19A | bg_color_g | u8 | Background G (default 0x40) |
| 0x19B | bg_color_b | u8 | Background B (default 0x40) |

### Boss State (0x19C-0x19D)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x19C | boss_defeated | u8 | Set to 1 when boss HP reaches 0, triggers death sequence |
| 0x19D | boss_facing | s8 | Boss facing direction for cutscene (-1=left, 1=right) |

**Source:** Boss AI callbacks (e.g., `BossKloggCallback_BeamAttack` @ 0x80067F98)  
**Verified:** Ghidra MCP tracing analysis 2026-01-20

## Remaining Unknown Fields

**UPDATED 2026-01-20**: All major unknowns resolved via Ghidra analysis.

| Offset | Previous | Resolved |
|--------|----------|----------|
| 0x7A | Unknown | Padding (4-byte alignment) |
| 0x106 | Unknown | Padding after tile_render_state_count |
| 0x11B | Unknown | screen_shake_active flag |
| 0x3C-0x40 | Unknown | previous_spawn_list + blb_header_ptr |

## Key Discoveries

### Entity/Layer Shared Lists (0x1C, 0x20)
Both `AddEntityToBothLists` and `AddLayerToRenderList_*` use the same lists:
- 0x1C: Z-sorted tick list (sorted by priority at entity+0x10)
- 0x20: Z-sorted render list (sorted by layer at entity+0x08)

### Mode Callback System (0x00-0x04)
```
*param_1 = 0xFFFF0000;       // mode_base_offset
param_1[1] = GameModeCallback; // mode_callback_ptr
```
Called via complex pointer indirection in main() game loop.

### Vehicle/Entity Array Dual Purpose (0x164)
- `GetVehicleDataPtr()` returns `LevelDataContext+0x34` (ctx[13])
- `ClearAlternateEntitySpawnFlags` iterates as 0x40-byte entity array
- Count stored at 0x168 from `GetTileHeaderField16()`

### Checkpoint Frame Counter (0x10C, 0x138)
- 0x10C: Active frame counter, incremented in EntityTickLoopWithCamera
- 0x138: Saved frame counter backup for checkpoint restore

## Functions Analyzed

| Function | Address | Fields Used |
|----------|---------|-------------|
| main | 0x800828b0 | 0x00, 0x04, 0x18 (layer render context) |
| InitGameState | 0x8007cd34 | 0x4F, 0x50, 0x51, 0x52, 0x54, 0x58, 0x59, 0x5A, 0x5B, 0x66, 0x67, 0x7C, 0x80, 0x171-0x17B, 0x199-0x19B |
| SetupAndStartLevel | 0x8007d8a0 | 0x28 (via 0x4F), 0x43, 0x4F, 0x5B, 0x59, 0x5A, 0x145-0x14B, 0x161, 0x199-0x19B |
| SpawnPlayerAndEntities | 0x8007df38 | 0x19C, 0x19D, 0x170, 0x11C, 0x16C, 0x64, 0x66, 0x120, 0x122, 0x12A, 0x2C, 0x30, 0x14C |
| SaveCheckpointState | 0x8007eaac | 0x1C, 0x2C, 0x63, 0x10C, 0x134, 0x138, 0x14A |
| RestoreCheckpointEntities | 0x8007eaec | 0x1C, 0x2C, 0x63, 0x10C, 0x134, 0x138, 0x14A |
| EntityTickLoopWithCamera | 0x80020b34 | 0x1C, 0x10C, 0x11A |
| UpdateCameraPositionSmooth | 0x800233c0 | 0x30, 0x44-0x67, 0x11A, 0x120, 0x122, 0x12A |
| SetCameraPositionDirect | 0x80023dbc | 0x44-0x5E, 0x11A, 0x120, 0x122, 0x12A |
| InitLayersAndTileState | 0x80024778 | 0x48, 0x4A, 0x58-0x5B, 0x74, 0x78, 0x108, 0x124-0x126 |
| InitTileAttributeState | 0x80024cf4 | 0x68-0x72 (tile collision) |
| GetTileAttributeAtPosition | 0x800241f4 | 0x68, 0x6C, 0x6E, 0x70, 0x72 (tile collision lookup) |
| AddLayerToRenderList_* | 0x80021590+ | 0x1C, 0x20 |
| AddEntityToBothLists | 0x80021b48 | 0x1C, 0x20 |
| AddEntityToSortedRenderList | 0x800213a8 | 0x1C, 0x20 |
| ClearTickList | 0x80022218 | 0x1C |
| ClearEntityDefList | 0x80022338 | 0x28 |
| FreeEntityLists | 0x800223bc | 0x1C, 0x20, 0x24 |
| DispatchEventToCollidingEntity | 0x800226f8 | 0x24, 0x2C |
| SpawnOnScreenEntities | 0x80024288 | 0x28, 0x44, 0x46, 0x7C, 0x80, 0x10C |
| ClearAlternateEntitySpawnFlags | 0x80081e84 | 0x164, 0x168 |
| GameModeCallback | 0x8007e654 | 0x50-0x5C, 0x58, 0x67, 0x145-0x152, 0x100 (0x190), 0x19D |
| RenderEntities | 0x80020e80 | 0x1C, 0x20, 0x130-0x133 |
| InitPlayerEntity | 0x8001fcf0 | 0x124-0x126 (layer RGB) |
| InitPlayerSpawnPosition | 0x80024720 | 0x116, 0x118 (spawn position) |
| LoadTileDataToVRAM | 0x80025240 | 0x104, 0x108, 0x110, 0x114 (tile and palette render state) |
| AddPreInitEntitiesToList | 0x800250c8 | 0x110, 0x114, 0x1C (adds palette anim entities to tick list) |
| CheckCheatCodeInput | 0x800820b4 | 0x140, 0x17C-0x18C, 0x18F, 0x18D, 0x148, 0x84 |

## Deep Analysis: GameState Usage Patterns (2026-01-20)

This section documents patterns discovered through comprehensive decompilation analysis
of all functions that reference `g_GameState`.

### 1. Mode Callback Dispatch System

From `main()` and `InitGameState()`:

```c
gameState->mode_base_offset = 0xFFFF0000;  // -0x10000
gameState->mode_callback_ptr = GameModeCallback;
```

**Dispatch pattern in main():**
- When `mode > 0`: Uses vtable lookup via `mode_base_offset`
- When `mode <= 0`: Uses direct `mode_callback_ptr` call

This allows switching between a table-driven mode system and direct callbacks.

### 2. Dual Entity List System

| Offset | Field | Purpose |
|--------|-------|---------|
| 0x1C | `tick_list_head` | Active entities for gameplay tick |
| 0x20 | `render_list_head` | Active entities for rendering |
| 0x134 | `checkpoint_entity_list` | Backup for checkpoint respawn |

**Checkpoint algorithm (SaveCheckpointState → RestoreCheckpointEntities):**
1. On Ma-Bird activation: `tick_list → checkpoint_entity_list`, clear `tick_list`
2. Entities spawned after checkpoint go to fresh `tick_list`
3. On death: Swap lists back, destroy post-checkpoint entities
4. Player is re-added to list after restore

### 3. Double-Buffered Background Color System

**Discovered in `RenderEntities` @ 0x80020e80:**

| Offset | Field | Purpose |
|--------|-------|---------|
| 0x130 | `bg_color_change_flag` | Triggers update when non-zero |
| 0x131 | `bg_color_r_pending` | Pending R value |
| 0x132 | `bg_color_g_pending` | Pending G value |
| 0x133 | `bg_color_b_pending` | Pending B value |
| 0x199 | `bg_color_r` | Current/default R (0x40) |
| 0x19A | `bg_color_g` | Current/default G (0x40) |
| 0x19B | `bg_color_b` | Current/default B (0x40) |

When `bg_color_change_flag` is set, RGB is copied to **two locations** in the BLB header buffer:
- `blbHeaderBufferBase + 0x1D-0x1F` (primary buffer)
- `blbHeaderBufferBase + 0x505D-0x505F` (secondary buffer)

This implements double-buffering for the PSX's alternating frame buffers.

### 4. Cheat Code System

**From `CheckCheatCodeInput` @ 0x800820b4:**

- 8-entry circular buffer at offset 0x17C-0x18B (16 bytes total)
- Index wraps: `cheat_input_index` (0x18C) cycles 0-7
- 22 cheats total (indices 0x00-0x15)
- Uses `g_CheatCodeTable` for pattern matching

**Key cheats discovered:**
| Index | Effect | GameState Field |
|-------|--------|-----------------|
| 0x0C | Next stage | `direct_level_load = stage + 1` |
| 0x0D | Skip to menu | `direct_level_load = 99` |
| 0x0F | Frame-step toggle | `debug_pause_enable ^= 1` |
| 0x13 | Re-add player | `player_readd_flag = 1` |

### 5. Demo Mode System

**From `SetupAndStartLevel` @ 0x8007d8a0:**

When `demo_return_flag` (0x152) is set:
1. Sets deterministic RNG: `srand(1)`
2. Sets custom BG colors: R=0x40, G=0x20, B=0x80 (purple tint)
3. Creates special UI sprite at z=30000
4. Uses `GetWorldPositionX/Y` function pointers for sprite positioning
5. Enables demo playback via `EnableDemoPlaybackMode`

### 6. Special Level Handling

**Level 98 (0x62) - Credits/Ending:**
```c
if (param_2 == 0x62) {
    AdvancePlaybackSequence(...);  // Called twice
    bVar5 = 99;  // Redirect to menu
}
```

**Level 99 (0x63) - Menu:**
- Loaded by `InitGameState` at startup
- Target of level 98 redirect
- Clears hamster count on entry

### 7. Level Flags Bitmask (COMPLETE)

**Updated 2026-01-20**: All flags fully documented via Ghidra decompilation.

Level flags are stored at `TileHeader+0x18` (accessed via `GetLevelFlags` @ 0x8007b47c).
These 16-bit flags control player type, camera behavior, and cheat restrictions.

#### Complete Flag Reference

| Bit | Hex | Name | Purpose | Verified By |
|-----|-----|------|---------|-------------|
| 0 | 0x0001 | MENU_CURSOR | Creates menu cursor entity | `SpawnPlayerAndEntities` |
| 1 | 0x0002 | BOSS_DEFEATED_INIT | Initial boss defeated state | `SpawnPlayerAndEntities` |
| 2 | 0x0004 | **LEVEL_GLIDE** | Glide player mode (no gravity) | `SpawnPlayerAndEntities` |
| 3 | 0x0008 | FAST_CAMERA_SCROLL | Camera speed = 0xC000 (faster) | `SpawnPlayerAndEntities` |
| 4 | 0x0010 | **LEVEL_SOAR** | SOAR player mode (Bird-Man) | `SpawnPlayerAndEntities` |
| 5 | 0x0020 | ENTITY_POOL_ENABLE | Enables 256-entry entity pool | `SpawnPlayerAndEntities` |
| 6 | 0x0040 | Unknown | - | - |
| 7 | 0x0080 | SLOW_CAMERA_SCROLL | Camera speed = 0x8000 (slowest) | `SpawnPlayerAndEntities` |
| 8 | 0x0100 | **LEVEL_RUNN** | RUNN player mode (auto-run) | `SpawnPlayerAndEntities` |
| 9 | 0x0200 | **LEVEL_MENU** | Menu screen (no player entity) | `SpawnPlayerAndEntities` |
| 10 | 0x0400 | **LEVEL_FINN** | FINN player mode (swamp boat) | `SpawnPlayerAndEntities` |
| 11 | 0x0800 | AUTO_SCROLL | Auto-scroll level | `GetLevelAutoScrollFlag` |
| 12 | 0x1000 | NO_CAMERA | Disables camera entity creation | `SpawnPlayerAndEntities` |
| 13 | 0x2000 | **LEVEL_BOSS** | Boss fight mode | `SpawnPlayerAndEntities` |
| 14 | 0x4000 | SHOW_HUD | Show HUD elements | `GetLevelShowHUDFlag` |
| 15 | 0x8000 | DEBUG_FLAG | Debug mode flag | `GetLevelDebugFlag` |

#### IsNormalPlatformLevel Check

The function `IsNormalPlatformLevel` @ 0x8008200c returns TRUE only if **ALL** special
level flags are clear:

```c
bool IsNormalPlatformLevel(GameState* gameState) {
    u16 flags = GetLevelFlags(gameState + 0x84);
    return !(flags & (0x400 | 0x200 | 0x2000 | 0x100 | 0x10 | 0x04));
    // FINN | MENU | BOSS | RUNN | SOAR | GLIDE must all be 0
}
```

This determines if the level uses standard platformer controls vs special vehicle/boss modes.

#### Player Type Selection (SpawnPlayerAndEntities)

The flag check cascade in `SpawnPlayerAndEntities` @ 0x8007df38 determines which
player entity to create:

| Priority | Flag | Player Entity | Allocation Size |
|----------|------|---------------|-----------------|
| 1 | 0x400 | `CreateFinnPlayerEntity` | 0x114 bytes |
| 2 | 0x200 | `InitMenuEntity` | 0x140 bytes |
| 3 | 0x2000 | `CreateResultsScreenEntity` | 0x158 bytes |
| 4 | 0x100 | `CreateRunnPlayerEntity` | 0x110 bytes |
| 5 | 0x10 | `CreateSoarPlayerEntity` | 0x128 bytes |
| 6 | 0x04 | `CreateGlidePlayerEntity` | 0x11C bytes |
| 7 | (none) | `CreatePlayerEntity` | 0x1B4 bytes |

#### Cheat Restrictions

Multiple cheats (0x10, 0x11, 0x14, 0x15) use `IsNormalPlatformLevel` to restrict effects:
- `ApplyRandomRGBEffect` (cheat 0x10) - Only on normal platformer levels
- Entity byte 0x1AF/0x1B0/0x1B1 modifications (cheats 0x11/0x14/0x15) - Same restriction

This prevents RGB/visual cheats from breaking vehicle/boss levels.

### 8. Password Level List Population

**From `InitGameState` @ 0x8007cd34:**

```c
// Build password-selectable level list (max 10 entries)
for (level = 0; level < level_count; level++) {
    if ((flag & 1) != 0 && asset_index != 0) {
        password_levels[count++] = level;
        if (count >= 10) break;
    }
}
```

Criteria for password levels:
- Level flag bit 0 is set (flag & 1)
- Level has non-zero asset index

### 9. Frame Counter vs Checkpoint Score

The `frame_counter` field (0x10C) has dual purpose:
- During gameplay: Incremented each frame by `EntityTickLoopWithCamera`
- At checkpoint: Saved to `checkpoint_saved_score` (0x138)
- On restore: Restored from `checkpoint_saved_score`

This may track elapsed time rather than literal frames for score/ranking purposes.
