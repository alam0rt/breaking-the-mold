# Entity System

Entities are game objects combining sprite graphics, behavior callbacks, and position/state data.

## Overview

The entity system has three key aspects:
1. **Entity data** - BLB Asset 501 stores placement (24-byte structures)
2. **Entity behavior** - Hardcoded init functions with sprite IDs
3. **Entity struct** - Runtime 0x44C byte structure

**Critical**: Entity type → sprite ID mapping is **HARDCODED** in game code, not in BLB data.

> **See Also**: [Entity Types Reference](../reference/entity-types.md) for full callback table (121 entries) and type mappings.

## Asset 501 - Entity Placement Data

24-byte structures loaded from tertiary segment.

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    x1 (bbox left, pixels)
0x02    2     u16    y1 (bbox top, pixels)
0x04    2     u16    x2 (bbox right, pixels)
0x06    2     u16    y2 (bbox bottom, pixels)
0x08    2     u16    x_center (spawn position)
0x0A    2     u16    y_center (spawn position)
0x0C    2     u16    variant (animation/subtype)
0x0E    4     u32    padding (always 0)
0x12    2     u16    entity_type
0x14    2     u16    layer (with flags)
0x16    2     u16    padding
```

### Entity Count

Stored in Asset 100 (Tile Header) at offset 0x1E.

Accessor: `GetEntityCount` @ 0x8007b7a8

### Layer Field (offset 0x14)

```
Bits 0-7:  Render layer (1, 2, or 3)
Bits 8-15: Render flags (purpose unverified)
```

Most entities use simple values (1, 2, 3). Some use extended values like 0xF301.

**IMPORTANT**: This field does NOT determine z_order! Entity z_order is hardcoded per entity type.

## Known Entity Types

Common entity types observed in BLB data:

| Type | Name | Description |
|------|------|-------------|
| 2 | Clayball | Collectible coins (5,727 total) |
| 3 | Ammo | Standard bullet pickup |
| 8 | Item | Generic item pickup |
| 24 | SpecialAmmo | Special ammunition |
| 25, 27 | EnemyA/B | Enemy entities |
| 28, 48 | PlatformA/B | Moving platforms |
| 42 | Portal | Warp point |
| 45 | Message | Save/message box |
| 50, 51 | Boss/BossPart | Boss entities |
| 60, 61 | Particle/Sparkle | Visual effects |

For the complete list of 121 entity types with callback addresses, see [Entity Types Reference](../reference/entity-types.md).

## Runtime Entity Structure (0x44C bytes)

Based on Ghidra decompilation:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | state_high | State machine upper word |
| 0x04 | 4 | callback_main | Main update callback (EntityUpdateCallback) |
| 0x08 | 2 | x_position | X position (for spatial sorting) |
| 0x0A | 2 | y_position | Y position |
| 0x0C | 4 | callback2 | Secondary callback |
| 0x10 | 2 | z_order | Render depth (for z-sorting) |
| 0x18 | 4 | vtable | Method pointer table |
| 0x1C | 4 | z_list_head | Z-order sorted list head |
| 0x20 | 4 | x_list_head | X-position sorted list head |
| 0x34 | 4 | poly_ptr | Pointer to GPU primitive (POLY_*) |
| 0x38-0x47 | 16 | bbox | Bounding box (copied from frame metadata) |
| 0x68 | 2 | x_pos | X position (render) |
| 0x6A | 2 | y_pos | Y position (render) |
| 0x76 | 1 | sprite_dirty | Frame data needs update |
| 0x77 | 1 | state_dirty | State needs processing |
| 0x78 | 4 | sprite_data_ptr | Pointer to sprite frame data |
| 0xA0 | 4 | state_param1 | State machine parameter 1 |
| 0xA2 | 2 | state_index | Current state index |
| 0xA4 | 4 | state_param2 | State machine parameter 2 |
| 0xA8-0xAC | 8 | pending_state | Pending state transition |
| 0xB4 | 4 | frame_x_scale | X scale (fixed-point) |
| 0xB8 | 4 | frame_y_scale | Y scale (fixed-point) |
| 0xDA | 2 | current_frame | Current animation frame index |
| 0xDE | 2 | target_frame | Target animation frame index |
| 0xE0 | 2 | pending_flags | Pending state change flags (bitmask) |
| 0xE6 | 2 | frame_width | Current frame width |
| 0xE8 | 2 | frame_height | Current frame height |
| 0xEC | 2 | frame_countdown | Animation frame timer |
| 0xF0-0xF2 | 3 | rgb_current | Current RGB modulation |
| 0xF3-0xF5 | 3 | rgb_pending | Pending RGB values |
| 0xF6 | 1 | visibility | Rendering flag |
| 0xF7 | 1 | sprite_type | Sprite lookup type flag |
| 0xF8 | 1 | frame_loaded | Frame data loaded flag |
| 0xFE | 1 | scale_mode | Double-size flag |
| 0x100+ | ... | Extended | Entity-specific data |

## Entity Lifecycle

```
1. ALLOCATION
   AllocateFromHeap(blbHeaderBuffer, size, 1, 0) @ 0x800143f0
   └── Allocates from BLB buffer (16-byte aligned blocks)

2. INITIALIZATION
   InitEntityWithSprite(entity, sprite_id, z_order, x, y, flags) @ 0x8001c868
   ├── InitEntityStruct(entity, 0x44C) @ 0x8001a0c8 - Zero/init struct
   ├── FUN_8007bbc0() - GPU/render state setup
   ├── FUN_8001954c() - Animation state init
   ├── FUN_8001c980() - Entity core setup
   ├── FUN_8001cea4() - Load sprite data
   └── FUN_8001d080() - Finalize setup

3. CALLBACK ASSIGNMENT
   entity[1] = EntityUpdateCallback;  // Default @ 0x8001cb88
   entity[3] = secondary_callback;
   entity[10] = collision_callback?
   entity[12] = destroy_callback?

4. REGISTRATION
   AddEntityToSortedRenderList(GameState, entity) @ 0x800213a8 - Z-order list
   AddToZOrderList(GameState, entity) @ 0x80020f68 - Z-sorted at +0x1C
   AddToXPositionList(GameState, entity) @ 0x8002107c - X-sorted at +0x20

5. TICK LOOP
   EntityTickLoop (0x80020e1c)
   └── For each entity:
       └── EntityUpdateCallback (entity[1])
           ├── TickEntityAnimation() @ 0x8001d290
           ├── ApplyPendingSpriteState() @ 0x8001d554
           └── UpdateSpriteFrameData() @ 0x8001d748

6. STATE TRANSITIONS
   EntitySetState(entity, param1, param2) @ 0x8001eaac
   └── Dispatches to state-specific callbacks

7. DESTRUCTION
   FreeFromHeap(blbHeaderBuffer, ptr, size, flags) @ 0x800145a4
```

## Entity Init Functions

91 functions call `InitEntitySprite` with hardcoded parameters:

```c
// Example signatures:
InitEntitySprite(entity, 0x21842018, 10000, x, y, 0);  // Player
InitEntitySprite(entity, 0x168254b5, 959, x, y, 1);    // Particles
InitEntitySprite(entity, 0xa89d0ad0, 1001, x, y, 0);   // Entity
```

### Known z_order Values

| Entity | z_order | Purpose |
|--------|---------|---------|
| Player | 10000 | Front of most layers |
| UI/HUD | 10000 | Always visible |
| Particles | 959 | Effects behind gameplay |
| General | ~1000 | Gameplay layer |

## Entity Loader (LoadEntitiesFromAsset501 @ 0x80024dc4)

Loads 24-byte entity definitions from Asset 501:

```c
entity_count = GetEntityCount(ctx);  // From Asset 100 +0x1E
entity_data = ctx[14];  // Asset 501 pointer

for (i = 0; i < entity_count; i++) {
    EntityDef* def = entity_data + i * 24;
    // Dispatch to type-specific init function...
}
```

## Entity Tick Loop (EntityTickLoop @ 0x80020e1c)

Called each frame from main loop:

```c
void EntityTickLoop(GameState* state) {
    Entity* entity = state->entity_list;  // +0x1C
    
    while (entity != NULL) {
        if (entity->callback_main != NULL) {
            entity->callback_main(entity);
        }
        entity = entity->next;
    }
}
```

## Key Functions

### Core Entity Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `InitEntityStruct` | 0x8001a0c8 | Zero and init 0x44C byte structure |
| `InitEntityWithSprite` | 0x8001c868 | Full entity+sprite initialization |
| `InitEntitySprite` | 0x8001c720 | Core entity sprite init |
| `EntityTickLoop` | 0x80020e1c | Main update loop |
| `EntityUpdateCallback` | 0x8001cb88 | Default entity tick callback |
| `EntitySetState` | 0x8001eaac | State machine transitions |

### Animation System

| Function | Address | Purpose |
|----------|---------|---------|
| `TickEntityAnimation` | 0x8001d290 | Animation frame tick handler |
| `ApplyPendingSpriteState` | 0x8001d554 | Apply pending sprite changes |
| `UpdateSpriteFrameData` | 0x8001d748 | Update frame dimensions/offsets |

### Entity Lists

| Function | Address | Purpose |
|----------|---------|---------|
| `AddEntityToSortedRenderList` | 0x800213a8 | Sorted render list insertion |
| `AddToZOrderList` | 0x80020f68 | Z-order sorted list (+0x1C) |
| `AddToXPositionList` | 0x8002107c | X-position sorted list (+0x20) |
| `AddPreInitEntitiesToList` | 0x800250c8 | Pre-init entities from palette data |

### Memory Management

| Function | Address | Purpose |
|----------|---------|---------|
| `AllocateFromHeap` | 0x800143f0 | Block-based heap allocator |
| `FreeFromHeap` | 0x800145a4 | Free allocated memory |

### Entity Loading

| Function | Address | Purpose |
|----------|---------|---------|
| `LoadEntitiesFromAsset501` | 0x80024dc4 | Load entity defs |
| `GetEntityCount` | 0x8007b7a8 | Entity count accessor |
| `GetEntityDataPtr` | 0x8007b7bc | Asset 501 data pointer |

### Player/Boss Creation

| Function | Address | Purpose |
|----------|---------|---------|
| `SpawnPlayerAndEntities` | 0x8007df38 | Player creation dispatcher |
| `CreatePlayerEntity` | 0x800596a4 | Default player creation |
| `CreateCameraEntity` | 0x80044f7c | Camera entity creation |
| `InitPlayerEntity` | 0x8001fcf0 | Player setup |
| `InitBossEntity` | 0x80047fb8 | Boss setup |
| `InitPlayerSpriteAvailability` | 0x80059a70 | Check 7 player sprites |

## Entity Data Locations

### LevelDataContext (GameState+0x84)

| Offset | Field | Description |
|--------|-------|-------------|
| +0x38 (ctx[14]) | entityData | Asset 501 pointer (24-byte entity definitions) |

### GameState Entity Lists

| Offset | Field | Description |
|--------|-------|-------------|
| +0x1C | tickListHead | Entity tick list (z-sorted, iterated by EntityTickLoop) |
| +0x20 | renderListHead | Entity render list (z-sorted, iterated by RenderEntities) |
| +0x24 | updateQueueHead | Collision/update queue list |
| +0x28 | entityDefListHead | Entity definition pool (raw defs from Asset 501) |
| +0x2C | playerEntityAlt | Player entity (alternate reference) |
| +0x30 | playerEntity | Main player entity pointer |

## Entity Type Callback Table

The game uses a static callback table at `0x8009d5f8` to dispatch entity initialization/behavior
by type. This table is populated during `RemapEntityTypesForLevel` and stored at `GameState+0x7c`.

**Table Structure**: 121 entries (types 0-0x78), 8 bytes each:
```c
struct EntityTypeEntry {
    u32 flags;         // State flags (often 0xFFFF0000)
    void* callback;    // Init/tick callback function pointer
};
```

**Address**: `g_EntityTypeCallbackTable` @ `0x8009d5f8`

**Example entries** (from ROM):
| Type | Flags | Callback | Description |
|------|-------|----------|-------------|
| 0 | 0xFFFF0000 | 0x8007efd0 | Default/unused |
| 1 | 0xFFFF0000 | 0x8007f730 | Type 1 handler |
| 2 | 0xFFFF0000 | 0x80080328 | Type 2 handler |
| ... | ... | ... | ... |

**Loading flow**:
1. `RemapEntityTypesForLevel` @ 0x8008150c converts BLB entity types → internal types (0-0x78)
2. Internal type indexes into the callback table
3. Callback initializes entity behavior

## Entity Collision System

Collision detection is handled via the entity update queue at `GameState+0x24`.

### Key Functions

| Function | Address | Purpose |
|----------|---------|---------|
| `CheckEntityCollision` | 0x800226f8 | Main collision check |
| `FUN_8001b47c` | 0x8001b47c | Collision check wrapper |
| `FUN_8001b3f0` | 0x8001b3f0 | Bounding box overlap test |

### Collision Flow

```
1. Entity tick calls FUN_8001b47c(entity, type_mask, message, data)
2. FUN_8001b47c wraps CheckEntityCollision with entity's bbox
3. CheckEntityCollision:
   - type_mask == 2: Fast path - check player at GameState+0x2c directly
   - Other: Iterate GameState+0x24 queue for matching entities
4. If collision: Invoke target entity's state callback with message
5. Caller can check return value to determine if collision occurred
```

### Special Case: Clayballs (type_mask = 2)

Clayballs use an optimized collision path:
- Instead of iterating the collision queue, directly check the player entity
- Player entity stored at `GameState+0x2c`
- On collision, GameState callback receives message `3` (COLLECTED)

> **See Also**: [Entity Types Reference - Clayball Collision System](../reference/entity-types.md#clayball-collision-system) for detailed flow.

## Sprite ID Lookup

Entity type → sprite ID is hardcoded in init functions. The BLB does NOT contain this mapping.

To add a new entity type → sprite mapping, you must:
1. Find the init function in Ghidra
2. Extract the sprite ID constant
3. Verify the sprite exists in the level's tertiary container

## Related Documentation

- [Entity Types Reference](../reference/entity-types.md) - Full callback table (121 entries)
- [Game Functions Reference](../reference/game-functions.md) - Function addresses
- [Game Loop](game-loop.md) - Main loop and player creation
- [Player Animation](player-animation.md) - Player sprite system details
- [Sprites](sprites.md) - Sprite data format
- [Rendering Order](rendering-order.md) - Entity z_order
- [BLB Asset Handling](../blb-asset-handling.md) - Asset 501 details
- [Level Loading](level-loading.md) - Entity loading flow
