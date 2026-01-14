# Game Loop & Player Creation

The main game loop, player entity dispatch system, and pause/cheat systems.

## Newly Discovered Systems

This document was updated after tracing GameModeCallback and discovering:
- **Pause System**: Complete pause/unpause flow with audio muting and entity backup
- **Cheat Code System**: Button sequence detection and 22 cheat codes
- **Pause Menu HUD**: Complex HUD with player stats (lives, orbs, powerups)

## Main Loop (`main` @ 0x800828b0)

The main function initializes all subsystems and runs an infinite game loop:

```c
int main(void) {
    // === INITIALIZATION PHASE ===
    __main();                           // C runtime init
    SsUtReverbOn();                     // Enable audio reverb
    ResetCallback();                    // PSX interrupt setup
    LoadGameAssetLocations();           // Find GAME.BLB on CD
    InitGraphicsSystem(blbHeaderBufferBase);  // Double-buffer GPU (320x256)
    g_GameStatePtr = &g_GameStateBase;
    PadInit(0);                         // Controller init
    InitGeom();                         // GTE init
    SetDispMask(1);                     // Enable display
    
    // Debug font at VRAM (0x3c0, 0x100)
    FntLoad(0x3c0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 200, 0, 0x200));
    
    InitPlayerControllerState(g_pPlayerState);
    InitGameState(&g_GameStateBase, g_pPlayer1Input);
    
    // Populate level/movie name tables
    for (i = 0; i < GetLevelCount(); i++) {
        g_LevelNameTable[i] = getLevelName(i);
        g_TotalMenuItems++;
        g_LevelCount++;
    }
    for (i = 0; i < GetAssetCount(); i++) {
        g_MenuItemNames[g_TotalMenuItems++] = GetMovieReservedByIndex();
        g_MovieCount++;
    }
    
    // Default background color
    g_DefaultBGColorR = 0x40;
    g_DefaultBGColorG = 0x20;
    g_DefaultBGColorB = 0x80;
    g_pCurrentInputState = g_pPlayer1Input;
    
    // === MAIN GAME LOOP (infinite) ===
    while (true) {
        TickCDStreamBuffer();           // Stream CD data every 4 frames
        u_long padData = PadRead(1);    // Read controller ports
        UpdateInputState(g_pPlayer1Input, padData & 0xFFFF);       // P1
        UpdateInputState(g_pPlayer2Input, padData >> 16);          // P2
        
        // Mode callback dispatch (level-specific logic)
        // Uses callback table at GameState[1]/GameState[2]
        if (g_GameStatePtr[1] != 0) {
            pcVar12 = *(code **)(g_GameStatePtr + 2);  // Get callback
            (*pcVar12)((int)g_GameStatePtr + offset);  // Execute mode handler
        }
        
        EntityTickLoop(g_GameStatePtr); // Update all entities
        WaitForVBlankIfNeeded(blbHeaderBufferBase);  // Conditional VSync
        RenderEntities(g_GameStatePtr); // Draw entities
        DrawSync(0);                    // Wait for GPU
        
        // Layer render callback (render tile layers)
        (**(code **)(*(int *)(g_GameStatePtr + 0xc) + 0x1c))(...)
        DrawSync(0);
        
        // Frame timing (wait 2 frames if flag set)
        if ((g_GameFlags & 6) != 0) {
            VSync(2);
        }
        
        ProcessDebugMenuInput();        // Handle debug level select
        FlushDebugFontAndEndFrame(blbHeaderBufferBase);
    }
}
```

## Mode Callback System

The game uses a mode callback system stored in GameState offsets 0-8 for dispatching
level-specific logic each frame.

### GameState Mode Fields

| Offset | Type | Purpose |
|--------|------|---------|
| +0x00 | s16 | Base offset for callback parameter |
| +0x02 | s16 | Current callback table index (or -1 for single callback) |
| +0x04 | ptr | Callback pointer OR table base |

### Callback Dispatch Logic

```c
// From main loop (@ 0x80082ae0)
iVar2 = (int)g_GameStatePtr[1];  // Callback index at +0x02
if (iVar2 != 0) {
    if (iVar2 < 1) {
        // Negative index: Direct callback pointer at +0x04
        pcVar12 = *(code **)(g_GameStatePtr + 2);
    } else {
        // Positive index: Table-based lookup
        // Table at offset stored in +0x04, index * 8 bytes per entry
        iVar7 = iVar2 * 8 + *(int *)((int)g_GameStatePtr + (int)g_GameStatePtr[2]);
        unaff_s4 = *(undefined4 *)(iVar7 + -8);  // Parameter offset
        pcVar12 = *(code **)(iVar7 + -4);        // Callback function
    }
    iVar7 = (int)*g_GameStatePtr;  // Base offset from +0x00
    if (0 < iVar2 << 0x10) {
        iVar7 = (short)unaff_s4 + iVar7;  // Adjust for table entry
    }
    (*pcVar12)((int)g_GameStatePtr + iVar7);  // Call with adjusted offset
}
```

### Mode Callback Initialization

In `InitGameState` the mode callback is initialized to 0x8007e654:
```c
state[0] = 0xFFFF0000;           // Base offset in high word
state[1] = &LAB_8007e654;        // Initial mode handler (indirect pointer)
```

The initial mode handler at 0x8007e654 manages level loading transitions,
checkpoint handling, and respawn logic.

## Game State Tick Specification

**For building replay/validation tools, capture the following state at EntityTickLoop entry:**

This section documents the complete game state required for deterministic replay and validation.

### GameState Structure (Base @ 0x8009DC40)

The GameState structure is the central data structure for the game loop. All state modifications
happen through functions called by the main loop.

#### Core State Offsets

| Offset | Type | Size | Field Name | Description | Modified By |
|--------|------|------|------------|-------------|-------------|
| 0x00 | s16 | 2 | mode_base_offset | Base offset for callback parameter | InitGameState, mode callbacks |
| 0x02 | s16 | 2 | mode_callback_index | Callback table index (-1 = direct ptr) | Mode transition functions |
| 0x04 | ptr | 4 | mode_callback_ptr | Callback pointer or table base | Mode transition functions |
| 0x08 | ptr | 4 | (unknown) | | |
| 0x0C | ptr | 4 | level_data_context_ptr | Pointer to LevelDataContext (+0x84) | InitializeAndLoadLevel |
| 0x1C | ptr | 4 | entity_tick_list_head | Head of tick-sorted entity list | Entity add/remove functions |
| 0x20 | ptr | 4 | entity_render_list_head | Head of render-sorted list | Entity add/remove functions |
| 0x24 | ptr | 4 | entity_collision_list_head | Entity collision/update queue | Collision system |
| 0x28 | ptr | 4 | entity_pool | Raw entity definitions (Asset 501) | LoadEntitiesFromAsset501 |
| 0x2C | ptr | 4 | player_alt | Alternate player reference | SpawnPlayerAndEntities |
| 0x30 | ptr | 4 | player_entity_ptr | Main player entity pointer | SpawnPlayerAndEntities |
| 0x38 | s16 | 2 | camera_x | Camera X position (pixels) | UpdateCameraPosition |
| 0x3A | s16 | 2 | (camera_x_high) | | |
| 0x3C | s16 | 2 | camera_y | Camera Y position (pixels) | UpdateCameraPosition |
| 0x3E | s16 | 2 | (camera_y_high) | | |
| 0x50 | ptr | 4 | input_state_ptr | g_pPlayer1Input pointer | InitGameState |
| 0x7C | ptr | 4 | callback_table_ptr | Entity type callback table (0x8009D5F8) | InitGameState |
| 0x84 | struct | varies | level_data_context | LevelDataContext (0x8009DCC4) | Level loading functions |
| 0x11C | u32 | 4 | player_scale | Player scale factor (0x8000-0x10000) | SpawnPlayerAndEntities |
| 0x124 | u8 | 1 | player_tint_r | Player RGB tint (red) | Various |
| 0x125 | u8 | 1 | player_tint_g | Player RGB tint (green) | Various |
| 0x126 | u8 | 1 | player_tint_b | Player RGB tint (blue) | Various |
| 0x130 | u8 | 1 | bg_color_change_flag | Background color change request | Various, cleared by RenderEntities |
| 0x131 | u8 | 1 | bg_color_r | Background color (red) | Various |
| 0x132 | u8 | 1 | bg_color_g | Background color (green) | Various |
| 0x133 | u8 | 1 | bg_color_b | Background color (blue) | Various |
| 0x134 | struct | varies | checkpoint_entity_list | Saved entity list for respawn | SaveCheckpointState |
| 0x161 | u8 | 1 | respawn_flag | Respawn requested flag | Respawn system |
| 0x171 | u8[10] | 10 | password_level_list | Password-selectable levels | InitGameState |
| 0x17B | u8 | 1 | password_level_count | Count of password levels (max 10) | InitGameState |
| 0x17C | u16[8] | 16 | checkpoint_save_data | Checkpoint/save data | InitGameState, checkpoint system |

### Input State Structure (g_pPlayer1Input)

Input is captured by `PadRead(1)` in the main loop and processed by `UpdateInputState`.

**Structure Layout (verified @ 0x800259d4):**

| Offset | Type | Field Name | Description |
|--------|------|------------|-------------|
| 0x00 | u16 | buttons_held | Currently held buttons (bitfield) |
| 0x02 | u16 | buttons_pressed | Newly pressed this frame (edge detect) |
| 0x04 | ptr | playback_data_ptr | Pointer to playback buffer (demo mode) |
| 0x05 | u8 | playback_active | Non-zero if replaying recorded input |
| 0x08 | ptr | recording_buffer | Pointer to recording buffer |
| 0x10 | u16 | playback_index | Current playback position |
| 0x12 | u16 | playback_timer | Frames until next input event |

**PSX Controller Button Mapping:**

| Bit | Hex | Button |
|-----|-----|--------|
| 0 | 0x0001 | Select |
| 1 | 0x0002 | L3 |
| 2 | 0x0004 | R3 |
| 3 | 0x0008 | Start |
| 4 | 0x0010 | Triangle |
| 5 | 0x0020 | Circle |
| 6 | 0x0040 | Cross (X) |
| 7 | 0x0080 | Square |
| 8 | 0x0100 | L2 |
| 9 | 0x0200 | R2 |
| 10 | 0x0400 | L1 |
| 11 | 0x0800 | R1 |
| 12 | 0x1000 | D-Pad Up |
| 13 | 0x2000 | D-Pad Right |
| 14 | 0x4000 | D-Pad Down |
| 15 | 0x8000 | D-Pad Left |

**Input Processing Flow:**
1. `PadRead(1)` returns 32-bit value (P1 in low 16 bits, P2 in high 16 bits)
2. `UpdateInputState(g_pPlayer1Input, padData & 0xFFFF)` processes P1
3. `UpdateInputState(g_pPlayer2Input, padData >> 16)` processes P2
4. Function updates `buttons_held` and calculates `buttons_pressed` (edge detect)
5. If playback mode active, reads from recording buffer instead

### Camera State

Camera position is stored in GameState and updated by `UpdateCameraPosition` @ 0x80023dbc.

**Memory Locations:**
- GameState+0x38: `camera_x` (s16) - X position in pixels
- GameState+0x3C: `camera_y` (s16) - Y position in pixels

**Camera Update Logic:**
```c
void UpdateCameraPosition(GameState* state) {
    Entity* player = state->player_entity_ptr;  // +0x30
    
    // Calculate camera target based on player position
    s16 target_x = player->x_pos - SCREEN_WIDTH/2;
    s16 target_y = player->y_pos - SCREEN_HEIGHT/2;
    
    // Apply level bounds clamping
    // Apply smoothing/interpolation
    
    state->camera_x = clamped_x;
    state->camera_y = clamped_y;
}
```

### Entity State

Entities are stored in linked lists in GameState. The `EntityTickLoop` @ 0x80020e1c
iterates the tick list at GameState+0x1C and calls each entity's update callback.

**Entity Lists:**
- **Tick List** (+0x1C): Entities that need per-frame updates
- **Render List** (+0x20): Entities sorted by z-order for rendering
- **Collision List** (+0x24): Entities involved in collision detection

**Entity Structure (see [entities.md](entities.md) for full details):**

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x00 | ptr | next | Next entity in linked list |
| 0x04 | ptr | callback_main | Main tick callback function |
| 0x08 | ptr | callback_render | Render callback function |
| 0x0C | u16 | entity_type | Entity type ID (0-120) |
| 0x48 | s16 | x_whole | X position (whole part) |
| 0x4A | s16 | y_whole | Y position (whole part) |
| 0x4C | u16 | x_frac | X position (fractional) |
| 0x4E | u16 | y_frac | Y position (fractional) |
| 0x68 | s16 | x_pos | X pixel position (for rendering) |
| 0x6A | s16 | y_pos | Y pixel position (for rendering) |
| 0xB4 | s32 | vx | X velocity (16.16 fixed-point) |
| 0xB8 | s32 | vy | Y velocity (16.16 fixed-point) |
| 0xBC | u32 | sprite_id | Current sprite ID |
| 0xD8 | u16 | anim_timer | Animation timer |
| 0xDA | u16 | anim_frame | Current animation frame |

**Entity Iteration:**
```c
void EntityTickLoop(GameState* state) {
    Entity* entity = state->entity_tick_list_head;  // +0x1C
    
    while (entity != NULL) {
        if (entity->callback_main != NULL) {
            entity->callback_main(entity);
        }
        entity = entity->next;
    }
}
```

### Main Loop State Modifications

**Functions that WRITE to GameState during main loop:**

1. **UpdateInputState** (0x800259d4)
   - Writes to: g_pPlayer1Input, g_pPlayer2Input (external to GameState)
   - Updates: buttons_held, buttons_pressed, playback state

2. **Mode Callback** (varies by mode)
   - Writes to: Various GameState fields depending on current mode
   - Examples: Level transitions, checkpoint handling, respawn logic

3. **EntityTickLoop** (0x80020e1c)
   - Writes to: Entity structures (position, velocity, state, animation)
   - Does NOT directly write to GameState base structure

4. **RenderEntities** (0x80020e80)
   - Writes to: GameState+0x130 (clears bg_color_change_flag)
   - Copies RGB values from GameState+0x131/132/133 to frame buffers

5. **UpdateCameraPosition** (0x80023dbc)
   - Writes to: GameState+0x38 (camera_x), GameState+0x3C (camera_y)

### Deterministic Replay Requirements

To replay a level deterministically, capture the following per frame:

**Required State (Frame N):**
1. Input state (buttons_held, buttons_pressed)
2. GameState mode callback state (mode_index, mode_callback_ptr)
3. Camera position (camera_x, camera_y)
4. Player entity (position, velocity, state, sprite, animation frame)
5. All entities (position, velocity, type, callback, animation)
6. Background color change flag and RGB values

**Optional State (for validation):**
1. RNG seed (if deterministic RNG used)
2. Frame counter
3. BLB metadata (level ID, stage index)

**Capture Points:**
- **Before EntityTickLoop**: Input state is finalized
- **After EntityTickLoop**: All entity updates complete
- **After RenderEntities**: Background color flag cleared

**Implementation Notes:**
- PSX uses little-endian byte order
- Fixed-point velocities: divide by 65536.0 for float conversion
- Entity pointers must be validated (0x80000000-0x80200000 range)
- Camera position can be negative (level bounds)

### Key Functions for State Capture

| Address | Name | Purpose | Reads From | Writes To |
|---------|------|---------|------------|-----------|
| 0x800259d4 | UpdateInputState | Process controller input | PadRead result | g_pPlayer1Input |
| 0x80020e1c | EntityTickLoop | Update all entities | GS+0x1C (tick list) | Entity structures |
| 0x80020e80 | RenderEntities | Render frame | GS+0x20 (render list) | GS+0x130 (bg flag) |
| 0x80023dbc | UpdateCameraPosition | Update camera scroll | Player entity, level bounds | GS+0x38, GS+0x3C |
| 0x8007e654 | InitialModeHandler | Level loading/respawn logic | Various | GS mode fields |

## InitGameState (`InitGameState` @ 0x8007cd34)

Called once at startup to initialize the game:

```c
void InitGameState(GameState* state, void* inputState) {
    LoadBLBHeader(g_GameStatePtr);
    InitializeAndLoadLevel(state, 99);  // 99 = MENU level
    
    // Set player state from level data
    char levelIdx = GetCurrentLevelAssetIndex(state + 0x84);
    if (levelIdx == 0) {
        g_pPlayerState[0] = 1;
        g_pPlayerState[1] = 1;
    } else {
        g_pPlayerState[0] = levelIdx;
        g_pPlayerState[1] = GetCurrentStageIndex(state + 0x21);
    }
    
    // Clear checkpoint/save data (8 entries at +0x17C)
    for (i = 0; i < 8; i++) {
        *(u16*)(state + 0x17C + i*2) = 0;
    }
    
    // Clear various state flags
    state[0x161] = 0;  // respawn flag
    state[0x199] = 0x40;  // RGB defaults
    state[0x19a] = 0x40;
    state[0x19b] = 0x40;
    state[0x50] = inputState;  // g_pPlayer1Input
    
    // Set initial mode callback
    state[0] = 0xFFFF0000;
    state[1] = &LAB_8007e654;  // Initial mode handler
    
    RemapEntityTypesForLevel(state);
    state[0x59] = GetVehicleDataPtr(state + 0x21);
    *(u16*)(state + 0x5a) = GetTileHeaderField16(state + 0x21);
    ClearSaveSlotFlags(state);
    
    // Start audio
    StartCDAudioForLevel(GetCurrentLevelAssetIndex(), GetCurrentStageIndex());
    
    // Build password-selectable level list at state+0x171 (max 10)
    state[0x17b] = 0;  // count
    for (i = 0; i < GetLevelCount(); i++) {
        if (state[0x17b] < 10 && GetLevelFlagByIndex(i) != 0 
            && GetLevelAssetIndex(i) != 0) {
            state[0x171 + state[0x17b]] = GetLevelAssetIndex(i);
            state[0x17b]++;
        }
    }
    
    // Copy world index from tile header to player state
    // NOTE: GetTileHeaderField08 was MISNAMED - actually reads offset 0x20
    // Renamed to GetTileHeaderWorldIndex (2026-01-13)
    g_pPlayerState[4] = GetTileHeaderWorldIndex(state + 0x21);
    
    SpawnPlayerAndEntities(state);
}
```

## Player Entity Creation

### Dispatch Function (`SpawnPlayerAndEntities` @ 0x8007df38)

Creates the player entity based on level type flags from the tile header:

```c
void SpawnPlayerAndEntities(GameState* state, LevelDataContext* ctx, 
                            void* param3, void* param4) {
    u16 levelFlags = GetLevelFlags(ctx);
    
    // Priority-based dispatch (checked in order)
    if (levelFlags & 0x0400) {
        // FINN (swimming) level
        CreateFinnPlayerEntity(state, ctx, param3, param4);
    }
    else if (levelFlags & 0x0200) {
        // Menu/password screen
        CreateMenuPlayerEntity(state, ctx, param3, param4);
    }
    else if (levelFlags & 0x2000) {
        // Boss fight
        CreateBossPlayerEntity(state, ctx, param3, param4);
    }
    else if (levelFlags & 0x0100) {
        // RUNN (auto-run) level
        CreateRunnPlayerEntity(state, ctx, param3, param4);
    }
    else if (levelFlags & 0x0010) {
        // SOAR (flying) level
        CreateSoarPlayerEntity(state, ctx, param3, param4);
    }
    else if (levelFlags & 0x0004) {
        // GLID (gliding) level
        CreateGlidePlayerEntity(state, ctx, param3, param4);
    }
    else {
        // Default platforming player
        CreatePlayerEntity(state, ctx, param3, param4);
    }
    
    // Also creates camera entity
    CreateCameraEntity(state, ctx, ...);
}
```

### Level Type Flags

Stored in the tile header (Asset 100), accessed via `GetLevelFlags` @ 0x8007b47c.

| Flag | Hex | Level Type | Player Creator |
|------|-----|------------|----------------|
| GLID | 0x0004 | Gliding levels | `CreateGlidePlayerEntity` @ 0x8006edb8 |
| SOAR | 0x0010 | Flying levels | `CreateSoarPlayerEntity` @ 0x80070d68 |
| RUNN | 0x0100 | Auto-run levels | `CreateRunnPlayerEntity` @ 0x80073934 |
| MENU | 0x0200 | Menu/password | (menu handler) |
| FINN | 0x0400 | Swimming levels | `CreateFinnPlayerEntity` @ 0x80074100 |
| BOSS | 0x2000 | Boss fights | `CreateBossPlayerEntity` @ 0x80078200 |
| (none) | 0x0000 | Normal platforming | `CreatePlayerEntity` @ 0x800596a4 |

### Flag Priority Order

When multiple flags are set, they're checked in this order:
1. FINN (0x0400) - highest priority
2. MENU (0x0200)
3. BOSS (0x2000)
4. RUNN (0x0100)
5. SOAR (0x0010)
6. GLID (0x0004)
7. Default platforming

### CreatePlayerEntity @ 0x800596a4 (Detailed)

Creates the standard platforming player entity (Klaymen):

```c
int CreatePlayerEntity(void* buffer, void* inputController, 
                       short spawn_x, short spawn_y, int facingLeft) {
    Entity* entity = (Entity*)buffer;
    
    // Initialize sprite from lookup table at DAT_8009c174
    InitEntityWithSprite(entity, &DAT_8009c174, 1000, spawn_x, spawn_y);
    
    // Store controller input pointer
    entity[0x40] = inputController;  // g_pPlayer1Input
    
    // Set scale based on GameState+0x11c (or 0x8000 if PlayerState[0x18] set)
    u32 scale = (g_pPlayerState[0x18] != 0) ? 0x8000 : g_GameStatePtr[0x11c];
    entity[0x130] = scale;
    entity[0x134] = scale;
    
    // Copy RGB color from GameState+0x124/125/126 to entity+0x15d/15e/15f
    entity[0x15d] = *(byte*)(g_GameStatePtr + 0x124);  // R
    entity[0x15e] = *(byte*)(g_GameStatePtr + 0x125);  // G
    entity[0x15f] = *(byte*)(g_GameStatePtr + 0x126);  // B
    
    // Set main update callback (player tick)
    entity[1] = FUN_8005b414;  // Player tick callback
    
    // Set state machine from data tables based on facingLeft param
    int initialState = facingLeft ? DAT_800a5cc4 : DAT_800a5cc0;
    EntitySetState(entity, initialState);
    
    // Create halo powerup effect if PlayerState[0x17] & 1
    if (g_pPlayerState[0x17] & 1) {
        void* haloBuffer = AllocateFromHeap(blbHeaderBufferBase, 0x68, 1, 0);
        int haloEntity = FUN_800589e8(haloBuffer);  // Halo init
        entity[0x168] = haloEntity;  // Store halo reference
        AddToXPositionList(g_GameStatePtr, haloEntity);
    }
    
    return (int)entity;
}
```

**Key entity offsets (player 0x1B4 bytes):**
| Offset | Purpose |
|--------|---------|
| 0x40 | Input controller pointer (g_pPlayer1Input) |
| 0x100 | Checkpoint reference (copied from GameState+0x140) |
| 0x130-0x134 | X/Y scale factors |
| 0x15d/e/f | Player RGB tint |
| 0x168 | Halo powerup entity ptr |
| 0x16c | Trail powerup entity ptr |
| 0x1af | Particle spawn trigger |

### InitMenuEntity @ 0x80076928 (Detailed)

Creates the menu "player" entity for menu-type levels:

```c
Entity* InitMenuEntity(void* buffer, void* inputController, 
                       void* levelList, byte levelCount) {
    Entity* entity = (Entity*)buffer;
    
    // Initialize with menu sprite at position (0,0)
    InitEntitySprite(entity, 0xb8700ca1, 1000, 0, 0, 1);
    
    // Set method table pointer
    entity[6] = &DAT_80011e94;  // Menu entity method table
    
    // Clear global/state variables
    DAT_800a6045 = 0;           // Menu global flag
    entity[0x13a] = 0;          // Timer/counter
    entity[0x4b] = 0;           // Child entity count
    entity[0x12d] = 0;          // Current menu item index
    entity[4] = 1000;           // Z-order
    
    // Store parameters
    entity[0x40] = inputController;  // Input state pointer
    entity[0x4d] = levelList;        // Password level list
    entity[0x4e] = levelCount;       // Password level count
    
    // Set state machine and tick callback
    entity[0] = 0xFFFF0000;
    entity[1] = &MenuTickCallback;  // @ 0x80077940
    
    // Set background color from color table
    FUN_800778ec(entity);  // Reads DAT_800a6042 * 3 for RGB
    
    // Dispatch to stage-specific init based on current stage
    byte stage = GetCurrentStageIndex(g_GameStatePtr + 0x84);
    if (stage > 4) stage = 1;  // Clamp to valid range
    
    switch (stage) {
        case 2:  InitMenuStage2(entity); break;  // Password entry
        case 3:  InitMenuStage3(entity); break;  // Options/settings
        case 4:  InitMenuStage4(entity); break;  // Load game
        case 1:
        default: InitMenuStage1(entity); break;  // Main menu
    }
    
    // Trigger initial menu item callback if items exist
    if (entity[0x4b] != 0) {
        int itemIdx = entity[0x12d];
        Entity* menuItem = entity[0x41 + itemIdx];
        (*menuItem->callback)(menuItem);  // Highlight first item
    }
    
    return entity;
}
```

**Menu entity size**: 0x140 bytes (320 bytes)

**Menu sprite ID**: 0xb8700ca1 (menu UI frame)

### Menu Entity Offsets

| Offset | Type | Purpose |
|--------|------|---------|
| 0x04 | u16 | Z-order (1000) |
| 0x06 | ptr | Method table (DAT_80011e94) |
| 0x40 | ptr | Input controller (g_pPlayer1Input) |
| 0x4b | u8 | Child entity count |
| 0x4d | ptr | Password level list (GameState+0x171) |
| 0x4e | u8 | Password level count (max 10) |
| 0x78 | struct | Sprite context (InitSpriteContext) |
| 0x100 | ptr | First input state reference |
| 0x104-0x11C | ptr[7] | Child menu item entity pointers |
| 0x12d | u8 | Current selected menu item index |
| 0x12e-0x130 | u8[3] | Stage 4 selection indices |
| 0x131 | u8 | Input repeat counter (auto-repeat) |
| 0x13a | u16 | Timer/animation counter (probably for playing demo on idle) |

### Menu Stages

| Stage | Init Function | Purpose | Key Features |
|-------|--------------|---------|--------------|
| 1 | FUN_80076ba0 | Main Menu | Title, 4 menu buttons, Klaymen animation |
| 2 | FUN_80077068 | Password Entry | 12-digit password input, cursor |
| 3 | FUN_800771c4 | Options | Color picker, back button |
| 4 | FUN_800773fc | Load Game | 3 save slots, back button |

### Stage 1 - Main Menu (FUN_80076ba0)

Creates the title screen with animated elements:

```c
void InitMenuStage1(Entity* menuEntity) {
    // Background decorations (5 sprites)
    InitEntitySprite(alloc, 0x68c01218, 2000, 0xa0, 0xa8, 0);  // BG1
    InitEntitySprite(alloc, 0x3080840d, 2000, 0xa0, 0xa8, 0);  // BG2
    InitEntitySprite(alloc, 0x3080820d, 2000, 0xa0, 0xa8, 0);  // BG3
    InitEntitySprite(alloc, 0x30808e0d, 2000, 0xa0, 0xa8, 0);  // BG4
    InitEntitySprite(alloc, 0x38a0c119, 2000, 0xa0, 0xa8, 0);  // BG5
    
    // Klaymen animation entity (from DAT_8009cbdc sprite table)
    InitEntityWithSprite(alloc, &DAT_8009cbdc, 2000, 0xa0, 0xa8);
    EntitySetState(entity, DAT_800a6050);
    
    // 4 menu buttons at positions from DAT_8009cb0c table
    // Sprite ID: 0x10094096 (menu button)
    for (i = 0; i < 4; i++) {
        short x = DAT_8009cb0c[i * 3];
        short y = DAT_8009cb0e[i * 3];
        byte type = DAT_8009cb10[i * 6];
        
        InitEntitySprite(alloc, 0x10094096, 1000, x, y, 0);
        FUN_800754cc(entity);  // Attach cursor sprite
        menuEntity[0x104 + menuEntity[0x4b]++] = entity;
    }
    
    // Optional bonus entity (if sprite 0x40b18011 exists)
    // Klaymen head bonus animation
}
```

**Main Menu Sprite IDs:**
| ID | Purpose |
|----|---------|
| 0x68c01218 | Background layer 1 |
| 0x3080840d | Background layer 2 |
| 0x3080820d | Background layer 3 |
| 0x30808e0d | Background layer 4 |
| 0x38a0c119 | Background layer 5 |
| 0x10094096 | Menu button (reused for all buttons) |
| 0x40b18011 | Bonus head animation (optional) |

### Stage 2 - Password Entry (FUN_80077068)

```c
void InitMenuStage2(Entity* menuEntity) {
    // Password display entity (0x144 bytes)
    FUN_80075ff4(alloc, 0x24, 0x69, &DAT_8009cb00, &DAT_800a6041);
    // Creates 12 character slots using sprite 0xec95689b
    // Position highlight using sprite 0x3099991b
    
    // Back button
    InitEntitySprite(alloc, 0x10094096, 1000, 0x20, 0x85, 0);
    menuEntity[0x12d] = 1;  // Default to back button
}
```

### Stage 3 - Options (FUN_800771c4)

```c
void InitMenuStage3(Entity* menuEntity) {
    // Color picker entity (0x110 bytes)
    InitEntitySprite(alloc, 0x10094096, 1000, 0x5f, 0x9b, 0);
    // Uses sprite 0x81100030 for color preview
    // Links to DAT_800a6042 for color index
    
    // Back button
    InitEntitySprite(alloc, 0x10094096, 1000, 0x59, 0xb7, 0);
    menuEntity[0x12d] = 1;
}
```

### Stage 4 - Load Game (FUN_800773fc)

```c
void InitMenuStage4(Entity* menuEntity) {
    // Restore save indices from globals
    menuEntity[0x12e] = DAT_800a607f;  // Slot 1 selection
    menuEntity[0x12f] = DAT_800a6080;  // Slot 2 selection
    menuEntity[0x130] = DAT_800a607e - 1;  // Slot 3 selection
    
    // 3 save slot selectors
    for (i = 0; i < 3; i++) {
        InitEntitySprite(alloc, 0x10094096, 1000, x[i], y[i], 0);
        // Uses sprite 0xe289c059 for slot preview
        // Each references menuEntity+0x12e/f/130 for state
    }
    
    // Back button
    InitEntitySprite(alloc, 0x10094096, 1000, 0x29, 0xa2, 0);
    menuEntity[0x12d] = 3;  // Default to back
}
```

### Menu Tick Callback (@ 0x80077940)

The menu tick callback handles input and updates:

```c
void MenuTickCallback(Entity* menuEntity) {
    byte stageIdx = GetCurrentStageIndex(g_GameStatePtr + 0x84);
    if (stageIdx > 4) stageIdx = 1;
    
    // Get current input state
    InputState* input = menuEntity[0x100];
    
    // Check if any child items exist
    if (menuEntity[0x4b] == 0) return;
    
    // Update current menu item (calls FUN_80077af0)
    FUN_80077af0(menuEntity);
}
```

### Menu Input Handler (FUN_80077af0)

Processes controller input for menu navigation:

```c
void MenuInputHandler(Entity* menuEntity) {
    if (menuEntity[0x4b] == 0) return;  // No items
    
    InputState* input = menuEntity[0x100];
    u16 buttons = input[1];  // Current frame buttons
    u16 held = input[0];     // Held buttons
    
    // D-pad Down (0x4000) - next item
    if (buttons & 0x4000) {
        byte idx = menuEntity[0x12d];
        if (idx == menuEntity[0x4b] - 1) {
            idx = 0;  // Wrap to first
        } else {
            idx++;
        }
        menuEntity[0x12d] = idx;
        PlaySound(0x646c2cc0, 0xa0, 0);  // Menu move SFX
    }
    
    // D-pad Up (0x1000) - previous item
    if (buttons & 0x1000) {
        byte idx = menuEntity[0x12d];
        if (idx == 0) {
            idx = menuEntity[0x4b] - 1;  // Wrap to last
        } else {
            idx--;
        }
        menuEntity[0x12d] = idx;
        PlaySound(0x646c2cc0, 0xa0, 0);
    }
    
    // Cross/Circle (0x8000) - select item
    if (buttons & 0x8000) {
        Entity* item = menuEntity[0x104 + menuEntity[0x12d]];
        (*item->selectCallback)(item);
    }
    
    // D-pad Left/Right (0x2000/0x8000) - adjust value
    // With auto-repeat after 10 frames
    if ((held & 0x2000) && ++menuEntity[0x131] > 10) {
        (*item->adjustCallback)(item, direction);
        menuEntity[0x131] = 0;
    }
    
    // Button mapping for specific actions (0-7)
    // 0x80=1, 0x20=3, 0x40=0, 0x04=4, 0x01=5, 0x08=6, 0x02=7
}
```

### Menu Background Color (FUN_800778ec)

Sets background color based on global color index:

```c
void SetMenuBackgroundColor(void) {
    int idx = DAT_800a6042 * 3;  // Color index * 3 (RGB triplet)
    g_DefaultBGColorB = DAT_8009cbac[idx + 0];
    g_DefaultBGColorR = DAT_8009cbac[idx + 1];
    g_DefaultBGColorG = DAT_8009cbac[idx + 2];
}
```

The color table at DAT_8009cbac contains multiple RGB presets that can be cycled in the options menu.

### Menu Cursor Entity (FUN_800754cc)

Creates and attaches a cursor sprite to a menu button:

```c
void AttachMenuCursor(Entity* button) {
    Entity* cursor = AllocateFromHeap(0x100);
    
    // Position cursor relative to button (+0x6a, +0x0e)
    short x = button[0x68] + 0x6a;
    short y = button[0x6a] + 0x0e;
    
    InitEntityWithSprite(cursor, &DAT_8009cbe8, 2000, x, y);
    cursor[0x18] = &DAT_8001208c;  // Cursor method table
    
    button[0x100] = cursor;  // Store cursor reference
    button[0x104] = 0;       // Cursor state
}
```

### Player Entity Sizes by Type

| Player Type | Entity Size | Creator Function |
|-------------|-------------|------------------|
| Normal | 0x1B4 (436 bytes) | CreatePlayerEntity |
| Menu | 0x140 (320 bytes) | InitMenuEntity |
| Finn | 0x114 (276 bytes) | CreateFinnPlayerEntity |
| Runn | 0x110 (272 bytes) | CreateRunnPlayerEntity |
| Glide | 0x11C (284 bytes) | CreateGlidePlayerEntity |
| Soar | 0x128 (296 bytes) | CreateSoarPlayerEntity |
| Boss | 0x158 (344 bytes) | CreateBossPlayerEntity |
| Camera | 0x10C (268 bytes) | CreateCameraEntity |

### Player Tick Callback @ 0x8005b414

Main per-frame player update function. Called via EntityTickLoop:

```c
void PlayerTickCallback(Entity* player) {
    // Check debug/pause state
    if (g_GameFlags & 1) {
        // Debug mode - skip normal processing
        return;
    }
    
    // Call base entity animation update
    EntityUpdateCallback(player);
    
    // RGB modulation (damage flash, invincibility)
    if (player[0x1ab] != 0) {
        // Apply damage flash RGB modulation
        ApplyRGBModulation(player);
        player[0x1ab]--;
    }
    
    // Powerup display management
    Entity* halo = player[0x168];  // Halo powerup entity
    if (halo != NULL) {
        UpdateHaloPosition(halo, player);
    }
    
    Entity* trail = player[0x16c];  // Trail powerup entity
    if (trail != NULL) {
        UpdateTrailPosition(trail, player);
    }
    
    // Scale transition handling (shrink/grow effects)
    u32 currentScale = player[0x130];
    u32 targetScale = g_GameStatePtr[0x11c];
    if (currentScale != targetScale) {
        // Interpolate scale toward target
        player[0x130] = LerpTowards(currentScale, targetScale, rate);
        player[0x134] = player[0x130];
    }
    
    // Particle spawning
    if (player[0x1af] != 0) {
        SpawnPlayerParticles(player);
        player[0x1af] = 0;
    }
}
```

### Scale Factors (GameState+0x11c)

Scale is determined by level flags in SpawnPlayerAndEntities:

| Flag | Scale Value | Effect |
|------|-------------|--------|
| 0x80 | 0x8000 | Half size (50%) |
| 0x08 | 0xC000 | 3/4 size (75%) |
| (none) | 0x10000 | Full size (100%) |

Additionally, if `PlayerState[0x18]` is set, scale defaults to 0x8000 regardless of level flags.

## Entity Tick Loop (`EntityTickLoop` @ 0x80020e1c)

Called every frame to update all active entities:

```c
void EntityTickLoop(GameState* state) {
    LevelDataContext* ctx = state + 0x84;
    Entity* entity = *(Entity**)(ctx + 0x1C);  // Head of linked list
    
    while (entity != NULL) {
        EntityCallback callback = entity->callback_main;  // entity[1]
        
        if (callback != NULL) {
            callback(entity);
        }
        
        entity = entity->next;
    }
}
```

## RenderEntities (`RenderEntities` @ 0x80020e80)

Called every frame to render all entities in z-order:

```c
void RenderEntities(GameState* state) {
    // Handle background color update request
    if (*(char *)(state + 0x130) != 0) {
        // Copy RGB from state+0x131/132/133 to BLB header buffer
        // for both frame buffers (double-buffered)
        blbHeaderBufferBase[0x1d] = state[0x131];  // R
        blbHeaderBufferBase[0x1e] = state[0x132];  // G
        blbHeaderBufferBase[0x1f] = state[0x133];  // B
        blbHeaderBufferBase[0x505d] = state[0x131]; // Second buffer
        blbHeaderBufferBase[0x505e] = state[0x132];
        blbHeaderBufferBase[0x505f] = state[0x133];
        *(char *)(state + 0x130) = 0;  // Clear request
    }
    
    // First pass: iterate tick list (+0x1C) - empty loop, possibly for sorting
    for (entity = state+0x1c; entity != NULL; entity = entity->next) {
        // No operations (optimization artifact?)
    }
    
    // Second pass: iterate render list (+0x20) and call render callbacks
    for (node = state+0x20; node != NULL; node = node->next) {
        int methodTable = *(node[1] + 0xC);
        code* renderFunc = *(methodTable + 0xC);
        short offset = *(methodTable + 0x8);
        renderFunc(node[1] + offset);  // Call entity render method
    }
}
```

**Note:** The render list at +0x20 stores entities in z-order (sorted during insertion).
Each entity's method table at +0xC contains function pointers for update and render.

## Memory Allocation

### Heap Allocator (`AllocateFromHeap` @ 0x800143f0)

Block-based allocator used for entity and buffer allocation:

```c
void* AllocateFromHeap(void* baseAddr, u32 elementSize, u32 count, int flags) {
    // Calculate blocks needed (16-byte aligned)
    u32 totalSize = elementSize * count;
    u32 blocksNeeded = (totalSize + 0x13) >> 4;
    
    // Get free list head
    void** freeList = baseAddr + 0xA648;
    
    // Find and allocate contiguous blocks
    // ... allocation logic ...
    
    return allocatedPtr;
}
```

## Render List Management

### Sorted Insertion (`AddEntityToSortedRenderList` @ 0x800213a8)

Inserts entities into the render/update list sorted by z_order:

```c
void AddEntityToSortedRenderList(GameState* state, Entity* entity) {
    Entity** listHead = &state->entityListHead;
    Entity* current = *listHead;
    
    // Find insertion point based on z_order
    while (current != NULL && current->z_order < entity->z_order) {
        current = current->next;
    }
    
    // Insert entity at this position
    // ... linked list insertion ...
}
```

## Supporting Functions

### RemapEntityTypesForLevel @ 0x8008150c

Large switch table that translates entity type IDs for special level handling.
Used during level initialization to remap entity behaviors.

### GetVehicleDataPtr @ 0x8007b924

Returns pointer to vehicle control data (Asset 504).
Only used in FINN and RUNN levels that have vehicle mechanics.

### StartCDAudioForLevel @ 0x8007ca9c

Initializes CD audio playback based on level configuration.
Handles background music and ambient audio setup.

### SetSequenceIndexByMode @ 0x8007a33c

Initializes the game's sequence/playback index based on current mode.
Used during level transitions.

## Key Functions Reference

| Address | Name | Purpose |
|---------|------|---------|
| 0x800828b0 | `main` | Game entry point and main loop |
| 0x8007cd34 | `InitGameState` | One-time game initialization |
| 0x8007df38 | `SpawnPlayerAndEntities` | Player creation dispatcher |
| 0x8007b47c | `GetLevelFlags` | Read level type flags |
| 0x80020e1c | `EntityTickLoop` | Per-frame entity updates |
| 0x800143f0 | `AllocateFromHeap` | Memory allocation |
| 0x800145a4 | `FreeFromHeap` | Memory deallocation |
| 0x800213a8 | `AddEntityToSortedRenderList` | Render list management |
| 0x80020f68 | `AddToZOrderList` | Z-sorted list insertion |
| 0x8002107c | `AddToXPositionList` | X-sorted list insertion |
| 0x800596a4 | `CreatePlayerEntity` | Default player creation |
| 0x80044f7c | `CreateCameraEntity` | Camera entity creation |
| 0x8007c36c | `SetGameMode` | Set game mode (0-6) |

### Mode-Specific Player Creators

| Address | Name | Level Type |
|---------|------|------------|
| 0x8006edb8 | `CreateGlidePlayerEntity` | GLID levels |
| 0x80070d68 | `CreateSoarPlayerEntity` | SOAR levels |
| 0x80073934 | `CreateRunnPlayerEntity` | RUNN levels |
| 0x80074100 | `CreateFinnPlayerEntity` | FINN levels |
| 0x80078200 | `CreateBossPlayerEntity` | Boss fights |

### Animation System

| Address | Name | Purpose |
|---------|------|---------|
| 0x8001cb88 | `EntityUpdateCallback` | Default entity tick handler |
| 0x8001d290 | `TickEntityAnimation` | Animation frame countdown |
| 0x8001d554 | `ApplyPendingSpriteState` | Apply pending changes (flags +0xE0) |
| 0x8001d748 | `UpdateSpriteFrameData` | Update frame dimensions/bbox |
| 0x8001eaac | `EntitySetState` | State machine transitions |

## Related Documentation

- [Level Loading](level-loading.md) - Asset loading flow
- [Entities](entities.md) - Entity system details  
- [Player Animation](player-animation.md) - Player sprite rendering, direction, powerups
- [Game Functions](../reference/game-functions.md) - Complete function list

## Pause System

The game includes a complete pause system with audio muting, entity backup, and a HUD display.

### Pause Flow

**Entering Pause (START button pressed):**

1. `PauseGameAndShowMenu @ 0x8007EC08` called from GameModeCallback
2. Calls `SaveAndMuteAllVoicePitches @ 0x8007CB44`:
   - Saves 24 SPU voice pitches to buffer @ 0x8009CC30
   - Sets all voice pitches to 0 (mutes audio)
   - Calls audio pause function @ 0x80038E50
   - Sets mute flag @ 0x800A6087
3. Plays menu open sound (ID 0x65281E40, volume 0xA0)
4. Backs up game state:
   - Saves tick list pointer (state+0x1C → state+0x15C)
   - Saves frame counter (state+0x10C → state+0x154)
   - Saves pause byte (state+0x63 → state+0x158)
5. Sets pause flags:
   - state+0x151 = 0 (no fade-out)
   - state+0x150 = 1 (pause active)
   - state+0x160 = 22 (22 frame countdown)
   - state+0x63 = 1 (halt gameplay logic)
6. Clears active tick list (state+0x1C = 0)
7. Shows pause menu HUD via `ShowPauseMenuHUD @ 0x8002B22C`:
   - Adds 3 fade overlays to render list
   - Spawns shadow entity for menu
   - Displays HUD elements (lives, orbs, powerups):
     * Lives count (from g_pPlayerState[0x11])
     * Orb count (from g_pPlayerState[0x12])
     * Checkpoint count (from g_pPlayerState[0x13])
     * "1970" icons × 3 (from g_pPlayerState[0x19])
     * Green orbs × 3 (from g_pPlayerState[0x1A])
     * 7 powerup icons (from g_pPlayerState[0x14-0x16, 0x1C])

**Exiting Pause (menu selection confirmed):**

1. `PauseGameWithFadeOut @ 0x8007ED34` called
2. Plays pause sound (ID 0x4C60F249, volume 0xA0)
3. Sets fade-out flag (state+0x151 = 1)
4. Calls fade function @ 0x8002BB94 (hides HUD)
5. Sets countdown to 22 frames (state+0x160 = 0x16)
6. GameModeCallback decrements countdown each frame
7. When countdown hits 1 and fade flag set:
   - Calls `UnpauseGameAndRestoreEntities @ 0x8007ED9C`
   - Calls `ResumeAllVoicePitches @ 0x8007CBC0`:
     * Restores 24 voice pitches from buffer @ 0x8009CC30
     * Calls audio resume @ 0x80038EA0
     * Clears mute flag @ 0x800A6087
   - Clears pause flags (state+0x151 = 0, state+0x150 = 0)
   - Restores frame counter (state+0x154 → state+0x10C)
   - Restores pause byte (state+0x158 → state+0x63)
   - Restores tick list (state+0x15C → state+0x1C)
   - Re-adds player to render list if needed

### Pause State Offsets

| Offset | Type | Purpose |
|--------|------|---------|
| +0x63 | u8 | Pause state (1 = paused) |
| +0x10C | u32 | Frame counter (saved during pause) |
| +0x14C | ptr | Pause menu entity pointer |
| +0x150 | u8 | Pause active flag |
| +0x151 | u8 | Fade-out active flag |
| +0x154 | u32 | Saved frame counter |
| +0x158 | u8 | Saved pause byte |
| +0x15C | ptr | Saved tick list pointer |
| +0x160 | u8 | Pause countdown (22 frames) |
| +0x18D | u8 | Re-add player flag |

### Pause Menu HUD Structure

The pause menu entity (state+0x14C) contains 30+ HUD element pointers:

| Offset | Count | Element |
|--------|-------|---------|
| +0x20 | 3 | Lives icons |
| +0x2C | 3 | Lives count digits |
| +0x38 | 3 | Orb icons |
| +0x44 | 3 | "1970" icon indicators |
| +0x50 | 3 | Green orb icons |
| +0x5C | 3 | Powerup icon 1 (type from +0x14) |
| +0x68 | 4 | Powerup icon 2 (type from +0x15) |
| +0x78 | 3 | Powerup icon 3 (type from +0x16) |
| +0x84 | 3 | Powerup icon 4 (type from +0x1C) |
| +0x90 | 1 | Shadow entity |
| +0x94 | 1 | Fade overlay 1 |
| +0x98 | 1 | Fade overlay 2 (RGB 0x80/0x80/0x80) |
| +0x9C | 1 | Fade overlay 3 (RGB 0x40/0x40/0x40) |

## Cheat Code System

The game includes a button sequence detection system with 22 cheat codes.

### Input Ring Buffer

- Located at GameState+0x17C (8 × u16 = 16 bytes)
- Current position at GameState+0x18C (u8)
- Advances on any button press (non-zero button state)
- Wraps around after 8 entries

### Pattern Matching

Called by `CheckCheatCodeInput @ 0x800820B4` from GameModeCallback during pause handling.

**Algorithm:**
1. Read button state from input+0x02
2. Store in ring buffer at current position
3. Advance position (wrap at 8)
4. Compare all 8 positions against 22 cheat patterns
5. Cheat patterns stored at global table @ 0x8009DAE0
6. Each pattern is 8 buttons wide × 16 bytes per cheat = 128 bytes total

**Pattern Format:**
- 8 consecutive button states must match exactly
- Pattern table entry = [button0, button1, ... button7] (8 × u16)
- 22 patterns indexed 0-0x15

### Cheat Codes

All cheats activated via 8-button sequences during pause menu.

| Index | Name | Effect | Implementation |
|-------|------|--------|----------------|
| 0x00 | Remove Pause Text & Inventory Screen | Hides pause menu HUD elements | g_GameFlags ^= 0x80 |
| 0x01 | (Unknown) | - | - |
| 0x02 | Max Items (most types) | Full powerups + 7 lives + 48 orbs | Sets multiple player state fields |
| 0x03 | Get all Swirly Q's immediately | Sets orb/checkpoint count to 20 | g_pPlayerState[0x13] = 20 |
| 0x04 | (Extra Halo?) | Enables invincibility flag | Sets player invincibility |
| 0x05 | Max Lives | Sets lives to 99 | g_pPlayerState[0x11] = 99 |
| 0x06 | Max Universe Enemas | Unlocks powerup slot 3 (7 uses) | g_pPlayerState[0x16] = 7 |
| 0x07 | Max Phoenix Hands | Unlocks powerup slot 1 (7 uses) | g_pPlayerState[0x14] = 7 |
| 0x08 | Max Super Willies | Unlocks powerup slot 4 (7 uses) | g_pPlayerState[0x1C] = 7 |
| 0x09 | Max Phart Heads | Unlocks powerup slot 2 (7 uses) + reset flags | g_pPlayerState[0x15] = 7 |
| 0x0A | Max Green Bullets | Sets green orbs to 3 | g_pPlayerState[0x1A] = 3 |
| 0x0B | Max 1970s Items | Sets "1970" icons to 3 | g_pPlayerState[0x19] = 3 |
| 0x0C | Level Skip | Warp to next stage/level | Advances level sequence |
| 0x0D | (Menu Warp?) | Warp to stage 99 (debug menu) | Sets stage index to 99 |
| 0x0E | (Invincibility Toggle) | Toggle invincibility | g_GameFlags ^= 1 |
| 0x0F | (Pause Menu Toggle) | Toggle pause menu visibility | Hides/shows HUD |
| 0x10 | Tinted Klaymen | Random RGB color effect (rainbow) | Sets entity RGB @ +0x15D-15F |
| 0x11 | Hazy Klaymen | Special visual effect | Level-restricted ability |
| 0x12 | Slow/Fast Gameplay | Toggle frame skip/turbo mode | g_GameFlags ^= 2 |
| 0x13 | (Player Respawn) | Set player re-add flag | Forces player respawn |
| 0x14 | Mini Klaymen | Player size reduction | Level-restricted ability |
| 0x15 | Fire Klaymen's Heads | Special attack ability | Level-restricted ability |

**Level Restrictions (0x10, 0x11, 0x14, 0x15):**
- Only activate if level flags (0x400, 0x200, 0x2000, 0x100, 0x10, 0x04) are ALL clear
- Prevents use in vehicle levels, bosses, and special stages
- Cheat 0x10 (Tinted Klaymen) calls ApplyRandomRGBEffect @ 0x8005B3D0

### Cheat Activation

When valid pattern detected:
1. Plays activation sound (ID 0x90810000, volume 0xA0)
2. Executes cheat effect (modifies g_GameFlags or g_pPlayerState)
3. Returns 1 (processed) or 0 (no effect)
4. Return value determines if GameModeCallback should continue or pause

### Global Flags Affected

| Flag Bit | Cheat | Effect |
|----------|-------|--------|
| 0x01 | 0x0E | Invincibility |
| 0x02 | 0x12 | Frame skip/turbo mode |
| 0x40 | 0x00 | Debug graphics |
| 0x80 | 0x00 | Debug overlay |

---

## Alternate Entity Spawning System

In addition to the Asset 501 spawning system, there's a parallel entity spawning mechanism for animated tiles, particles, and decorative elements.

**Implementation:**
- SpawnEntitiesAlternateSystem @ 0x80081d0c (main loop)
- Source data: 128-byte stride array @ GameState+0x164
- Entity count: GameState+0x168
- Allocated size: 0x10C bytes per entity

**Spawning Algorithm:**
```c
void SpawnEntitiesAlternateSystem(GameState* state) {
    for (int i = 0; i < state->alt_entity_count; i += 2) {  // Process in pairs
        u8* source = state->alt_entity_data + (i * 128);
        
        // Check if entity should be spawned (screen visibility, etc.)
        if (!ShouldSpawnEntity(source)) continue;
        
        // Check if already spawned
        if (IsEntityAlreadySpawned(source)) continue;
        
        // Allocate and initialize
        Entity* entity = AllocateFromHeap(blbHeaderBufferBase, 0x10C, 1, 0);
        InitAlternateEntity(entity, source);
        
        // Add to render list
        AddToRenderList(state, entity);
    }
}
```

### Source Data Structure (128-byte)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x00 | u32 | entity_type_index | For sprite ID lookup @ 0x8009B144 |
| 0x04 | u16 | anim_seq_1_id | Animation sequence 1 ID |
| 0x10 | u16 | width | Entity width in pixels |
| 0x12 | u16 | height | Entity height in pixels |
| 0x14 | u32 | custom_field_1 | Copied to entity+0x60 |
| 0x18 | u32 | custom_field_2 | Copied to entity+0x64 |
| 0x1C | u16 | frame_count | Animation frame count |
| 0x20 | u16 | anim_seq_2_id | Animation sequence 2 ID |
| 0x38 | u32 | comparison_value | For sprite ctx+0x5B flag |

### Entity Initialization Functions

**InitAlternateEntity @ 0x80033d3c:**
Complete initialization for alternate entity system.
1. Calls InitFullEntityWithAnimation(entity, 0x44C)
2. Sets vtable @ entity+0x18 = 0x80010AB8
3. Stores source pointer @ entity+0x100
4. Sets flags: +0x00=0xFFFF0000, +0x04=&LAB_8003433c
5. Allocates sprite render context (via SetupAlternateEntitySpriteContext)
6. Looks up sprite ID from table @ 0x8009B144 indexed by source[0]
7. Copies GameState RGB @ +0x124-126 to sprite context
8. Copies dimensions and custom fields from source

**SetupAlternateEntitySpriteContext @ 0x80033ef4:**
Allocates and initializes sprite rendering context. Two types based on sprite metadata:

**Type A (vtable 0x80010B00, size 0x60):**
- Used when sprite metadata == 0x180104 or 0x1498810C
- Allocates 3 frame buffer arrays (0x34 × frames, 0x34 × frames, 2 × frames)
- Creates secondary object @ entity+0x104 (0x10 bytes, vtable 0x80010AD8)
- Calls AddToXPositionList(GameState)
- Sets entity+0x108 = 1

**Type B (vtable 0x80010AE8, size 0x5C):**
- Used for other sprite types
- Allocates 2 frame buffer arrays (no frame index array)
- No secondary object (entity+0x104 = 0)
- Sets entity+0x108 = 0

**Both types allocate:**
- Pixel buffer @ entity+0xB0: width × height bytes (8bpp indexed)
- Buffer size @ entity+0xD4: width × height

**InitFullEntityWithAnimation @ 0x8001c6c8:**
Complete entity setup with sprite and animation support.
- Calls InitEntityStruct(entity, size)
- Sets vtable @ entity+0x18 = 0x8001044C
- Clears sprite context @ entity+0x78 (via ClearSpriteContextWrapper)
- Zeros field @ entity+0x8C (via ZeroEntityField)
- Initializes animation state

**InitBasicEntityWithVtable @ 0x8001543c:**
Minimal entity for UI/simple objects (0x10 bytes).
- Sets vtable @ entity+0x0C = 0x8001039C
- Stores size parameter @ entity+0x08
- Zeros position fields
- Sets enable flag @ entity+0x0A = 1

**InitMenuEntityWithVtable @ 0x80019748:**
Menu/UI entity initialization (0x16 bytes).
- Sets vtable @ entity+0x0C = 0x800104AC
- Zeros 6 u16 fields (positions, velocities, state)
- Stores parameter @ entity+0x10
- Sets enable flag @ entity+0x14 = 1

**InitColoredOverlayEntity @ 0x80034894:**
Colored overlay for menu shadows/fades.
- Calls InitBasicEntityWithVtable
- Sets vtable @ entity+0x0C = 0x80010AA8
- Stores RGBA @ entity+0x40-43
- Used by ShowPauseMenuHUD with (0x20, 0x20, 0x20, 2, 9000) for dark gray shadow

### Vtable Hierarchy

| Address | Purpose | Used By |
|---------|---------|---------|
| 0x80010AB8 | Alternate entity main | entity+0x18 |
| 0x80010B00 | Type A sprite context | sprite+0x0C |
| 0x80010AE8 | Type B sprite context | sprite+0x0C |
| 0x80010AD8 | Secondary object (Type A) | entity+0x104 |
| 0x8001044C | Full entity with animation | entity+0x18 |
| 0x8001039C | Basic entity | entity+0x0C |
| 0x800104AC | Menu entity | entity+0x0C |
| 0x80010AA8 | Colored overlay | entity+0x0C |

---

## CD Audio Control Integration

The pause system integrates CD-ROM audio control alongside SPU voice muting.

**Functions:**
- PauseCDAudio @ 0x80038e50 (called during pause)
- ResumeCDAudio @ 0x80038ea0 (called during unpause)

**Global State:**
- 0x800A59E8: CD audio enable flag (byte)
- 0x800A59EA: CD paused state flag (byte)

**PSY-Q Commands:**
- CdControl(0x09, NULL, NULL) = CdlPause (pause CD audio)
- CdControl(0x1B, NULL, NULL) = CdlResume (resume CD audio)

**Pause Flow Integration:**
```c
void SaveAndMuteAllVoicePitches(GameState* state) {
    // Save SPU voice pitches (24 voices @ state+0x17C)
    for (int i = 0; i < 24; i++) {
        state->saved_voice_pitch[i] = SpuGetVoicePitch(i);
        SpuSetVoicePitch(i, 0);  // Mute
    }
    
    // Pause CD audio
    PauseCDAudio();  // @ 0x80038E50
}

void ResumeAllVoicePitches(GameState* state) {
    // Restore SPU voice pitches
    for (int i = 0; i < 24; i++) {
        SpuSetVoicePitch(i, state->saved_voice_pitch[i]);
    }
    
    // Resume CD audio
    ResumeCDAudio();  // @ 0x80038EA0
}
```

**Notes:**
- CD commands use PSY-Q libcd.h CdControl function
- State flags prevent redundant pause/resume commands
- Paired with SPU voice backup/restore for complete audio pause
- PauseCDAudio called from SaveAndMuteAllVoicePitches @ 0x8007CB44
- ResumeCDAudio called from ResumeAllVoicePitches @ 0x8007CBC0

---

## Level Progression System

**AdvanceLevelSequence @ 0x8007a578:**
Called from GameModeCallback when level is complete (state+0x146 set, g_pPlayerState[0x11] == 0).

**Algorithm:**
```c
void AdvanceLevelSequence(LevelDataContext* ctx) {
    u8 current_index = ctx->blb_header[0xF30];  // Read from BLB header
    ctx->next_level_index = current_index - 2;   // Decrement by 2
}
```

**Memory Layout:**
- Context+0x5C: Pointer to BLB header
- BLB header+0xF30: Current level sequence index (byte)
- Context+0x60: Next level index to load (byte)

**Notes:**
- Decrement by 2 suggests level pairs or reverse indexing
- BLB header+0xF30 is the master level progression counter
- Next level loads based on context+0x60 value
- Called from GameModeCallback when player completes level

---

## Cheat Code Effects

### Tinted Klaymen (Rainbow Effect)

**ApplyRandomRGBEffect @ 0x8005b3d0:**
Applies random RGB color to entity sprite (cheat code 0x10 "Tinted Klaymen").

**Algorithm:**
```c
void ApplyRandomRGBEffect(Entity* entity) {
    entity->color_r = rand() & 0xFF;  // +0x15D
    entity->color_g = rand() & 0xFF;  // +0x15E
    entity->color_b = rand() & 0xFF;  // +0x15F
    entity->effect_enabled = 1;       // +0x1AE
}
```

**Entity Fields:**
- +0x15D: Red color component (byte)
- +0x15E: Green color component (byte)
- +0x15F: Blue color component (byte)
- +0x1AE: Effect enabled flag (1 = rainbow mode active)

**Notes:**
- Called from CheckCheatCodeInput case 0x10
- Creates rainbow/tinted visual effect on player sprite
- Rendering code checks +0x1AE to enable color modulation
- RGB values randomly regenerated each frame for animation effect



