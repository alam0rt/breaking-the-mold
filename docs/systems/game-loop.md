# Game Loop & Player Creation

The main game loop and player entity dispatch system.

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
    
    // Copy tile header field to player state
    g_pPlayerState[4] = GetTileHeaderField08(state + 0x21);
    
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
int InitMenuEntity(void* buffer, void* inputController, 
                   void* levelList, byte levelCount) {
    Entity* entity = (Entity*)buffer;
    
    // Initialize with menu sprite at position (0,0)
    InitEntitySprite(entity, 0xb8700ca1, 1000, 0, 0, 1);
    
    // Set main callback (menu tick handler)
    entity[1] = LAB_80077940;  // Menu tick callback
    
    // Dispatch to stage-specific init based on current stage
    switch (GetCurrentStageIndex()) {
        case 2:  FUN_80077068(entity, levelList, levelCount); break;
        case 3:  FUN_800771c4(entity, levelList, levelCount); break;
        case 4:  FUN_800773fc(entity, levelList, levelCount); break;
        case 1:
        default: FUN_80076ba0(entity, levelList, levelCount); break;
    }
    
    return (int)entity;
}
```

**Menu entity size**: 0x140 bytes (smaller than player's 0x1B4)

**Menu sprite ID**: 0xb8700ca1 (menu UI frame)

**Menu stages:**
- Stage 1: Default menu (FUN_80076ba0)
- Stage 2: Secondary menu (FUN_80077068) 
- Stage 3: Tertiary menu (FUN_800771c4)
- Stage 4: Quaternary menu (FUN_800773fc)

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
- [Game Functions](../reference/game-functions.md) - Complete function list
