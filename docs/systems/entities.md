# Entity System

Entities are game objects combining sprite graphics, behavior callbacks, and position/state data.

## Overview

The entity system has three key aspects:
1. **Entity data** - BLB Asset 501 stores placement (24-byte structures)
2. **Entity behavior** - Hardcoded init functions with sprite IDs
3. **Entity struct** - Runtime 0x44C byte structure

**Critical**: Entity type → sprite ID mapping is **HARDCODED** in game code, not in BLB data.

> **See Also**: [Entity Types Reference](../reference/entity-types.md) for full callback table (121 entries) and type mappings.

## Game Loop Integration

Entities are managed through a frame-based game loop executed in `main` @ 0x800828b0.
a
### Mode System Architecture

**Important**: There is **only ONE mode callback** (`GameModeCallback @ 0x8007e654`), not multiple mode-specific callbacks. The "mode" concept refers to three different systems:

1. **BLB Content Mode** (header+0xF36): Determines which data structures load via `GetCurrentModeReservedData @ 0x8007ae9c`
   - Mode 1: Movie entries (28 bytes @ header+0xB64)
   - Mode 2: Credits entries (12 bytes @ header+0xF1C)
   - Mode 3: Level entries (112 bytes @ header+0x56)
   - Mode 4/5: Demo entries (16 bytes @ header+0xCD3)
   - Mode 0, 6: No reserved data

2. **Level Loading Mode** (param_2): Controls level execution behavior passed to `SetupAndStartLevel`
   - param_2=1: Normal gameplay (live controller input)
   - param_2=5: Demo Mode 1 (input replay from buffer)
   - param_2=6: Demo Mode 2 (alternate demo replay)
   - param_2=99: Menu trigger mode

3. **Audio Mode** (0x800A6082): Used by `PlaySoundEffect @ 0x8007c388` to adjust sound behavior
   - Set via `SetGameMode @ 0x8007c36c` (validates 0-6)
   - Cleared to 0 by `UploadAudioToSPU @ 0x8007c088`

**All modes execute through the same callback** - they differ in which data loads and whether input is live or replayed, but the execution flow is identical.

> **See Also**: [Demo/Attract Mode System](demo-attract-mode.md) for input replay details.

### Frame Processing Flow

```
1. TickCDStreamBuffer()          - Stream CD data (every 4 frames)
2. PadRead(1)                    - Read controller input
3. UpdateInputState()            - Process P1/P2 input
4. Mode Callback                 - Level-specific logic
   └─> GameModeCallback @ 0x8007e654
       ├─ Pause/menu handling
       ├─ Level loading/respawn
       ├─ Checkpoint save/restore
       ├─ FUN_80081d0c()              - Spawn entities (alternate system)
       ├─ SpawnOnScreenEntities()     - Spawn from Asset 501
       └─ EntityTickLoop()            - Update entity callbacks
5. WaitForVBlankIfNeeded()       - VSync if needed
6. RenderEntities()              - Draw all entities
7. DrawSync(0)                   - Wait for GPU
8. Layer Render Callback         - Draw tile layers
9. DrawSync(0)                   - Wait for GPU
10. VSync(2) if needed           - Frame timing
11. ProcessDebugMenuInput()      - Debug menu
12. FlushDebugFontAndEndFrame()  - Present frame
```

### Game Mode Callback @ 0x8007e654

The mode callback coordinates level state, spawning, and entity processing:

```c
void GameModeCallback(GameState* state) {
    // Pause counter handling
    if (state[0x160] != 0) {
        state[0x160]--;
        // Fade out if countdown hits 1
        return;
    }
    
    // Menu/pause input handling
    // START button detection, fade callbacks
    
    // Level transition logic
    if (state[0x146] || state[0x147] || state[0x148]) {
        SetupAndStartLevel(...);
    }
    
    // Respawn handling
    if (state[0x146] && !state[0x19c]) {
        if (g_pPlayerState[0x11] == 0) {
            // Level complete - advance
            FUN_8007a578(state + 0x84);
            SetupAndStartLevel(state, 99);
        } else {
            RespawnAfterDeath(state);
        }
    }
    
    // Checkpoint system
    if (state[0x149] && !state[0x14a]) {
        SaveCheckpointState(state);      // Save entities to +0x134
    }
    if (state[0x14a]) {
        RestoreCheckpointEntities(state); // Restore from +0x134
    }
    
    // Entity spawning (only if not paused/loading)
    if (!state[0x150] && !state[0x14a]) {
        FUN_80081d0c(state);             // Spawn from alternate system
        SpawnOnScreenEntities(state);    // Spawn from Asset 501
    }
    
    // Entity processing (only if not paused)
    if (!state[0x190]) {
        EntityTickLoop(state);           // Update all entities
        UpdateCameraPosition(state);     // Camera scroll
    }
}
```

### Entity Processing Loop @ 0x80020b34

Iterates entity tick list and calls callbacks:

```c
void EntityTickLoop(GameState* state) {
    Entity* entity = *(state + 0x1C);  // Tick list head
    
    while (entity != NULL) {
        // Render layer management at z_order threshold
        if (entity->z_order > 1999 && !rendered) {
            FUN_800233c0(state);  // Camera scroll update
            rendered = true;
        }
        
        // Call entity update callback
        if (entity->callback_main != NULL) {
            (*entity->callback_main)(entity);
        }
        
        // Deferred entity removal
        FUN_80020c74(state);
        
        entity = entity->next;
    }
    
    state[0x10C]++;  // Increment frame counter
}
```

### Entity Removal @ 0x80020c74

Handles deferred entity destruction:

```c
void DeferredEntityRemoval(GameState* state) {
    if (state[0x34] != 0) {  // Entity marked for removal
        if (state[0x38] == 0) {
            RemoveEntityFromAllLists(state[0x34]);
            state[0x34] = 0;
        } else {
            RemoveFromUpdateQueue(state);
            if (state[0x38] != 1) {
                RemoveFromRenderList(state);
            }
            RemoveFromTickList(state, state[0x34]);
            
            // Call entity destructor
            Entity* entity = state[0x34];
            if (entity != NULL) {
                void* vtable = entity[0x18];
                code* destructor = *(vtable + 0xC);
                short offset = *(vtable + 8);
                (*destructor)(entity + offset, 3);  // Cleanup mode
            }
            state[0x34] = 0;
        }
        state[0x38] = 0;
    }
}
```

### Camera Update @ 0x800233c0

Complex camera scroll logic called during entity processing:

```c
void UpdateCameraPosition(GameState* state) {
    Entity* player = state[0x30];
    if (player == NULL || state[99] != 0) return;
    
    // Calculate target camera position from player
    // Applies screen offsets, level bounds clamping
    // Uses lookup tables at 0x8009b074, 0x8009b104, 0x8009b0bc
    // for smooth scrolling acceleration curves
    
    // Update camera velocity (state+0x4c, state+0x50)
    // Apply velocity with sub-pixel precision
    // Clamp to level bounds from tile header
    
    state[0x44] = clamped_x;  // Camera X
    state[0x46] = clamped_y;  // Camera Y
}
```

### Rendering Pipeline @ 0x80020e80

Renders all entities and handles background color updates:

```c
void RenderEntities(GameState* state) {
    // Update background color if requested
    if (state[0x130] != 0) {
        u8 r = state[0x131];
        u8 g = state[0x132];
        u8 b = state[0x133];
        
        // Double-buffered write
        blbHeaderBufferBase[0x1d] = r;    // FB1 red
        blbHeaderBufferBase[0x1e] = g;    // FB1 green
        blbHeaderBufferBase[0x1f] = b;    // FB1 blue
        blbHeaderBufferBase[0x505d] = r;  // FB2 red
        blbHeaderBufferBase[0x505e] = g;  // FB2 green
        blbHeaderBufferBase[0x505f] = b;  // FB2 blue
        
        state[0x130] = 0;  // Clear flag
    }
    
    // Empty iteration over tick list (+0x1C) - optimization artifact?
    
    // Render loop over z-sorted render list (+0x20)
    for (node = state[0x20]; node != NULL; node = node->next) {
        Entity* entity = node[1];
        void* vtable = entity[0xC];
        code* render_func = *(vtable + 0xC);
        short offset = *(vtable + 8);
        
        (*render_func)(entity + offset);
    }
}
```

**Background Color Format:**
- Offset 0x1d/5e/5f: RGB bytes (0-255)
- Two copies for double-buffering (0x5040 byte stride)
- Updated when state[0x130] flag set (collision events, triggers)

**Render List:**
- Z-sorted during insertion (AddEntityToSortedRenderList @ 0x800213a8)
- Each entity has method table (vtable) at entity+0xC
- Render callback at vtable+0xC with adjusted pointer (entity+offset)

## Complete Game Loop Reference

Full main loop with all discovered functions:

| Order | Function | Address | Purpose |
|-------|----------|---------|---------|
| 1 | TickCDStreamBuffer | 0x8007ccb8 | CD streaming (every 4 frames) |
| 2 | PadRead | PSY-Q | Read controller ports |
| 3 | UpdateInputState | 0x800259d4 | Process P1/P2 input |
| 4 | GameModeCallback | 0x8007e654 | Level coordinator |
| 4a | └─ SpawnEntitiesAlternateSystem | 0x80081d0c | Spawn from 128-byte array |
| 4b | └─ SpawnOnScreenEntities | 0x80024288 | Spawn from Asset 501 |
| 4c | └─ EntityTickLoop | 0x80020b34 | Update entity callbacks |
| 5 | WaitForVBlankIfNeeded | 0x8001352c | Conditional VSync |
| 6 | RenderEntities | 0x80020e80 | Entity rendering |
| 7 | DrawSync | PSY-Q | Wait for GPU |
| 8 | [Layer Render Callback] | via GameState+0xC | Tile layer rendering |
| 9 | DrawSync | PSY-Q | Wait for GPU again |
| 10 | VSync timing | PSY-Q | Optional 2-frame wait |
| 11 | ProcessDebugMenuInput | 0x80082c10 | Debug level select |
| 12 | FlushDebugFontAndEndFrame | 0x80013500 | Finalize frame |

**Frame Timing Modes:**
- Normal: VSync locks to 60 FPS (NTSC) or 50 FPS (PAL)
- Flag 0x06 set: Wait 2 vblanks (30 FPS mode)
- g_SkipVSync: Run unlocked (benchmarking/loading)

**CD Streaming:**
- ProcessCDStreamState @ 0x80038ef0 handles async CD reads
- State machine with retry logic (max 6 retries)
- Sector table at 0x8009b3d8, CdlLOC params at 0x8009b43c
- Enables audio/FMV streaming without blocking gameplay

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

Loads 24-byte entity definitions from Asset 501 into a linked list at `GameState+0x28`.

**This does NOT spawn entities** - it only loads the placement data. Actual entity spawning
happens later via `SpawnOnScreenEntities` when entities come into view.

```c
entity_count = GetEntityCount(ctx);  // From Asset 100 +0x1E
entity_data = ctx[14];  // Asset 501 pointer

// Copy each 24-byte entity def into a linked list node
for (i = 0; i < entity_count; i++) {
    EntityDef* def = AllocateFromHeap(blbHeaderBufferBase, 24, 1, 0);
    memcpy(def, entity_data + i * 24, 24);
    
    // Add to linked list at GameState+0x28
    ListNode* node = AllocateFromHeap(blbHeaderBufferBase, 8, 1, 0);
    node->next = GameState->entityDefList;
    node->data = def;
    GameState->entityDefList = node;
}
```

## Entity Spawn Dispatcher (SpawnOnScreenEntities @ 0x80024288)

**THIS IS THE SINGLE HOOK POINT FOR ALL ENTITY SPAWNING!**

Called every frame from the game mode callback (`FUN_8007e654`). Iterates the entity definition
list at `GameState+0x28` and spawns any entities that come on screen.

### Algorithm

```c
void SpawnOnScreenEntities(GameState* state) {
    // Iterate entity definition list
    for (ListNode* node = state->entityDefList; node != NULL; node = node->next) {
        EntityDef* def = node->data;
        
        // Check if entity is on screen (+/- 16px margin)
        if (!IsEntityOnScreen(state, def)) continue;
        
        // Check if already spawned (bit 0 of def->flags at +0x16)
        if (def->flags & 0x01) continue;
        
        // Lookup callback from table
        EntityTypeEntry* entry = state->callbackTable + (def->entity_type * 8);
        EntityCallback callback = entry->callback;
        
        // CALL THE CALLBACK - this allocates and initializes the entity
        callback(state, def);
        
        // Mark as spawned
        def->flags |= 0x01;
    }
}
```

### EntityDef Structure (24 bytes)

```c
struct EntityDef {                    // Offset  Type   Description
    u16 x1;                           // +0x00   u16    Bounding box left
    u16 y1;                           // +0x02   u16    Bounding box top
    u16 x2;                           // +0x04   u16    Bounding box right
    u16 y2;                           // +0x06   u16    Bounding box bottom
    u16 x_center;                     // +0x08   u16    Spawn X position
    u16 y_center;                     // +0x0A   u16    Spawn Y position
    u16 variant;                      // +0x0C   u16    Animation/subtype
    u32 padding;                      // +0x0E   u32    Always 0
    u16 entity_type;                  // +0x12   u16    Type (indexes callback table)
    u16 layer;                        // +0x14   u16    Render layer + flags
    u16 flags;                        // +0x16   u16    Bit 0: spawned flag
};
```

### Callback Table Lookup

The callback table is at `GameState+0x7C` (points to `0x8009D5F8` with 121 entries):

```c
struct EntityTypeEntry {              // 8 bytes per entry
    u32 flags;                        // +0x00   Usually 0xFFFF0000
    void (*callback)(GameState*, EntityDef*);  // +0x04   Function pointer
};

// Lookup:
EntityTypeEntry* entry = *(GameState + 0x7C) + (entity_type * 8);
callback = entry->callback;
```

### Callback Signature

```c
void EntityTypeXXX_Callback(GameState* state, EntityDef* def) {
    // Allocate entity structure (size varies by type)
    Entity* entity = AllocateFromHeap(blbHeaderBufferBase, SIZE, 1, 0);
    
    // Initialize entity with sprite ID (hardcoded per type)
    InitEntitySprite(entity, SPRITE_ID, z_order, def->x_center, def->y_center, flags);
    
    // Set entity-specific callbacks
    entity->callback_main = EntityUpdateCallback;
    
    // Add to game lists
    AddEntityToSortedRenderList(state, entity);
    AddToUpdateQueue(state, entity);
}
```

### Hook Point for Runtime Tracing

To capture entity spawning in `game_watcher.lua`, add a breakpoint at **0x80024288**
or hook the callback invocation to record:

- `entity_type` (entityDef+0x12)
- `x_center` (entityDef+0x08)
- `y_center` (entityDef+0x0A)
- `variant` (entityDef+0x0C)
- `layer` (entityDef+0x14)
- `callback` address being invoked
- Current frame number

This captures ALL entity spawning with complete type information, allowing cross-reference
with player interactions (pickups, collisions, etc.).

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
| `LoadEntitiesFromAsset501` | 0x80024dc4 | Load entity defs to linked list |
| `SpawnOnScreenEntities` | 0x80024288 | **SPAWN DISPATCHER** - calls callbacks from table |
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
| `CollisionCheckWrapper` | 0x8001b47c | Collision check wrapper |
| `CheckBBoxOverlap` | 0x8001b3f0 | Bounding box overlap test |

### Collision Flow

```
1. Entity tick calls CollisionCheckWrapper(entity, type_mask, message, data)
2. CollisionCheckWrapper wraps CheckEntityCollision with entity's bbox
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
