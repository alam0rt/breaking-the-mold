# Game Loop & Player Creation

The main game loop and player entity dispatch system.

## Main Loop (`main` @ 0x800828b0)

The main function initializes all subsystems and runs an infinite game loop:

```c
int main(void) {
    // Hardware initialization
    ResetCallback();
    SpuInit();
    InitGraphics();
    EnterCriticalSection();
    
    // System setup
    InitializeCDSubsystem();
    InitializeMemcardSystem();
    VSync(0);
    
    // Game state initialization
    LoadBLBHeader(&gameState);
    InitGameState(&gameState);
    
    // Main game loop (infinite)
    while (true) {
        TickCDStreamBuffer();           // CD streaming
        PadRead(0);                     // Controller input
        UpdateInputState(&gameState);   // Process input
        
        // Mode callback (level-specific logic)
        if (gameState.modeCallback != NULL) {
            gameState.modeCallback(&gameState);
        }
        
        // Entity updates
        EntityTickLoop(&gameState);
        
        // Rendering
        RenderEntities(&gameState);
        DrawSync(0);
        VSync(0);
    }
}
```

## InitGameState (`InitGameState` @ 0x8007cd34)

Called once at startup to initialize the game:

```c
void InitGameState(GameState* state) {
    // Clear state structure
    memset(state, 0, sizeof(GameState));
    
    // Entity type remapping for special levels
    RemapEntityTypesForLevel(state, state + 0x84);
    
    // Get vehicle data pointer (FINN/RUNN levels)
    GetVehicleDataPtr(state + 0x84);
    
    // Read tile header field
    GetTileHeaderField16(state + 0x84);
    
    // Clear save slot flags
    ClearSaveSlotFlags();
    
    // Initialize audio for level
    StartCDAudioForLevel(state);
    
    // Load menu level (index 99 = MENU)
    InitializeAndLoadLevel(state, 99);
    
    // Build password-selectable level list
    BuildPasswordLevelList(state);
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
| 0x800213a8 | `AddEntityToSortedRenderList` | Render list management |
| 0x800596a4 | `CreatePlayerEntity` | Default player creation |
| 0x80044f7c | `CreateCameraEntity` | Camera entity creation |

### Mode-Specific Player Creators

| Address | Name | Level Type |
|---------|------|------------|
| 0x8006edb8 | `CreateGlidePlayerEntity` | GLID levels |
| 0x80070d68 | `CreateSoarPlayerEntity` | SOAR levels |
| 0x80073934 | `CreateRunnPlayerEntity` | RUNN levels |
| 0x80074100 | `CreateFinnPlayerEntity` | FINN levels |
| 0x80078200 | `CreateBossPlayerEntity` | Boss fights |

## Related Documentation

- [Level Loading](level-loading.md) - Asset loading flow
- [Entities](entities.md) - Entity system details
- [Game Functions](../reference/game-functions.md) - Complete function list
