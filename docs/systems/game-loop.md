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
