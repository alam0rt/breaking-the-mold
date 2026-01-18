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

### Layer Render System (0x08-0x1B)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x08-0x14 | field_08-14 | int | Unknown - possibly layer-related |
| 0x18 | layer_render_context_ptr | ptr | Layer render context with callback at +0x1C |

**Source:** `main()` game loop uses `layer_render_context_ptr` to call render callbacks

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
| 0x60 | camera_vertical_lock | u8 | Vertical lock flag |
| 0x61 | camera_mode_flags | u8 | Camera mode flags |
| 0x62 | camera_invert_flag | u8 | Camera invert flag |
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

### Level/Layer State (0x74-0x7F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x74 | level_context_field_3C | ptr | LevelDataContext+0x3C ptr |
| 0x78 | tile_header_field_1C | s16 | TileHeader field 0x1C |
| 0x7A | field_7A | s16 | Unknown |
| 0x7C | entity_callback_table | ptr | Entity type callbacks (121 × 8 bytes) |
| 0x80 | entity_type_count | int | 0x79 = 121 entity types |

**Source:** `InitLayersAndTileState` @ 0x80024778

### LevelDataContext (0x84-0x103)
128-byte embedded structure containing level asset pointers.
See `docs/systems/level-data-context.md` for details.

### Render/Frame State (0x104-0x11F)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x104 | tile_render_state_count | u16 | Tile render state count (total tiles + 1) |
| 0x106 | field_106 | u16 | Unknown |
| 0x108 | tile_render_state_ptr | ptr | Tile render buffer (8 bytes per tile, allocated) |
| 0x10C | frame_counter | int | Frame counter (incremented each tick, saved to 0x138 on checkpoint) |
| 0x110 | palette_group_ptrs | ptr | Array of palette render context pointers |
| 0x114 | palette_group_count | u8 | Count of palette groups (byte) |
| 0x115 | field_115 | u8 | Padding |
| 0x116 | spawn_x | s16 | Player spawn X position |
| 0x118 | spawn_y | s16 | Player spawn Y position |
| 0x11A | screen_shake_index | u8 | Screen shake countdown |
| 0x11B | field_11B | u8 | Unknown |
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

### Score Display (0x17C-0x18B)
8 × 2-byte array for score digit display.

### Background Colors (0x199-0x19B)
| Offset | Name | Type | Purpose |
|--------|------|------|---------|
| 0x199 | bg_color_r | u8 | Background R (default 0x40) |
| 0x19A | bg_color_g | u8 | Background G (default 0x40) |
| 0x19B | bg_color_b | u8 | Background B (default 0x40) |

## Remaining Unknown Fields

Fields requiring further analysis:
- 0x08-0x14: Possibly layer scroll accumulators or unused padding (not accessed in analyzed functions)
- 0x3C-0x40: Unknown - need to find accessor (possibly layer list heads)

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
| CheckEntityCollision | 0x800226f8 | 0x24, 0x2C |
| SpawnOnScreenEntities | 0x80024288 | 0x28, 0x44, 0x46, 0x7C, 0x80, 0x10C |
| ClearAlternateEntitySpawnFlags | 0x80081e84 | 0x164, 0x168 |
| GameModeCallback | 0x8007e654 | 0x50-0x5C, 0x58, 0x67, 0x145-0x152, 0x100 (0x190), 0x19D |
| RenderEntities | 0x80020e80 | 0x1C, 0x20, 0x130-0x133 |
| InitPlayerEntity | 0x8001fcf0 | 0x124-0x126 (layer RGB) |
| InitPlayerSpawnPosition | 0x80024720 | 0x116, 0x118 (spawn position) |
| LoadTileDataToVRAM | 0x80025240 | 0x104, 0x108, 0x110, 0x114 (tile and palette render state) |
| AddPreInitEntitiesToList | 0x800250c8 | 0x110, 0x114, 0x1C (adds palette anim entities to tick list) |
