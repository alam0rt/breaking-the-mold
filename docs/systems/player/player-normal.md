# Normal Player Entity (Platforming)

The standard platforming player used in most levels (Klaymen).

## Level Flag

Normal platforming is used when NO special flags are set (0x0000).
Check order: FINN → MENU → BOSS → RUNN → SOAR → GLID → **Default (Normal)**

## Creation

**Function**: `CreatePlayerEntity` @ 0x800596a4

### Parameters
- `param_1`: Entity buffer (pre-allocated, 0x1B4 bytes)
- `param_2`: Input controller pointer (g_pPlayer1Input)
- `param_3`: Spawn X (pixels)
- `param_4`: Spawn Y (pixels)
- `param_5`: Facing direction (0=right, 1=left)

### Initialization Flow
```c
Entity* CreatePlayerEntity(void* buffer, void* inputController, 
                           short spawn_x, short spawn_y, char facingLeft) {
    // Initialize with player sprite table at 0x8009c174
    InitEntityWithSprite(buffer, &DAT_8009c174, 1000, spawn_x, spawn_y);
    
    // Set up callbacks
    entity[0x18] = &DAT_80011804;          // VTable
    entity[0x100] = inputController;        // Input
    entity[0x04] = PlayerTickCallback;      // Main tick (0x8005b414)
    entity[0x20] = LAB_80061180;           // Secondary callback
    entity[0x28] = FUN_8001a26c;           // Movement X callback
    entity[0x30] = FUN_8001a29c;           // Movement Y callback
    
    // Scale based on powerup state
    u32 scale = (g_pPlayerState[0x18] != 0) ? 0x8000 : g_GameStatePtr[0x11c];
    entity[0x58] = scale;  // X scale
    entity[0x5C] = scale;  // Y scale
    
    // Copy RGB from GameState
    entity[0x15d] = g_GameStatePtr[0x124];  // R
    entity[0x15e] = g_GameStatePtr[0x125];  // G  
    entity[0x15f] = g_GameStatePtr[0x126];  // B
    
    // Set initial state based on respawn flag
    if (g_GameStatePtr[0x161] == 0) {
        // Fresh start
        if (facingLeft) {
            EntitySetState(entity, DAT_800a5d28, PTR_LAB_800a5d2c);  // Facing left
        } else {
            EntitySetState(entity, DAT_800a5d30, PTR_LAB_800a5d34);  // Facing right
        }
    } else {
        // Respawning
        SetGameMode(g_GameStatePtr[0x198]);
        EntitySetState(entity, DAT_800a5d20, PTR_LAB_800a5d24);  // Respawn state
    }
    
    // Create halo powerup entity if has halo
    if (g_pPlayerState[0x17] & 1) {
        entity[0x168] = CreateHaloEntity();
    }
    
    return entity;
}
```

## Entity Structure (0x1B4 = 436 bytes)

### Base Entity Offsets (0x00-0xFF)
| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | state_high | State machine parameter |
| 0x04 | 4 | tickCallback | Main per-frame callback |
| 0x08-0x0A | 4 | position | X,Y (for sorting) |
| 0x10 | 2 | z_order | Render depth (1000) |
| 0x18 | 4 | vtable | Method table pointer |
| 0x20 | 4 | secondaryCallback | Secondary update |
| 0x24-0x30 | 16 | movementCallbacks | X/Y movement dispatch |
| 0x34 | 4 | spriteDataPtr | Pointer to sprite frame |
| 0x40-0x47 | 8 | bbox | Bounding box |
| 0x50 | 4 | scaleTarget | Scale transition target |
| 0x54 | 4 | scaleY | Y scale (16.16 fixed) |
| 0x58 | 4 | scaleX | X scale (16.16 fixed) |
| 0x5C | 4 | scaleDiv | Scale divisor |
| 0x68 | 2 | x_pos | X position (pixels) |
| 0x6A | 2 | y_pos | Y position (pixels) |
| 0xA0-0xAC | 16 | stateData | State machine state |
| 0xCC | 4 | currentSpriteId | Current sprite lookup ID |
| 0xF0-0xF5 | 6 | rgbData | RGB modulation values |

### Player-Specific Offsets (0x100+)
| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x100 | 4 | inputController | Pointer to button state |
| 0x104 | 2 | stateOffset | State callback offset |
| 0x106 | 2 | stateIndex | State callback index |
| 0x108 | 4 | stateCallback | State function pointer |
| 0x128 | 1 | invincibilityTimer | Damage invincibility countdown |
| 0x134 | 2 | unknown134 | |
| 0x144 | 2 | powerupTimer | Powerup effect countdown |
| 0x14C | 4 | hudEntity | HUD entity pointer |
| 0x156 | 2 | unknown156 | |
| 0x159 | 1 | pendingStateChange | Flag for deferred state |
| 0x15A-0x15C | 3 | currentRGB | Current R,G,B values |
| 0x15D-0x15F | 3 | baseRGB | Base R,G,B from spawn |
| 0x160 | 2 | xPush | Horizontal push force |
| 0x162 | 2 | yPush | Vertical push force |
| 0x168 | 4 | haloEntity | Halo powerup entity |
| 0x16C | 4 | glideEntity | Glide/scroll entity |
| 0x170 | 1 | hasPlayer | Level has player flag |
| 0x174 | 4 | soundHandle | Current sound effect |
| 0x178 | 1 | disableScale | Disable scale transition |
| 0x17D | 1 | rgbCooldown | RGB update cooldown |
| 0x1A6 | 2 | scrollFlagX | X scroll trigger |
| 0x1A8 | 2 | scrollFlagY | Y scroll trigger |
| 0x1AE | 1 | damageFlag | Taking damage flag |
| 0x1AF | 1 | particleFlag | Spawn particles flag |
| 0x1B0 | 1 | shrinkFlag | Shrink mode active |
| 0x1B1 | 1 | unknown1b1 | |
| 0x1B2 | 1 | unknown1b2 | |
| 0x1B3 | 1 | gameMode | Current game mode (0-6) |

## Main Tick Callback

**Function**: `PlayerTickCallback` @ 0x8005b414

Called every frame by EntityTickLoop:

```c
void PlayerTickCallback(Entity* player) {
    // Debug mode check
    if ((g_GameFlags & 1) && player->currentSpriteId != 0x1e28e0d4) {
        EntitySetState(player, DAT_800a5d90, PTR_LAB_800a5d94);
    }
    
    // Execute state callback (indexed via +0x104/0x106/0x108)
    if (player->stateIndex != 0) {
        ExecuteStateCallback(player);
    }
    
    // Update animation
    EntityUpdateCallback(player);
    
    // Shrink mode overrides bbox
    if (player->shrinkFlag != 0) {
        player->bbox = {-5, -10, 10, 10};
    }
    
    // Process tile collision (if not paused)
    if (g_GameStatePtr[0x14a] == 0) {
        PlayerProcessTileCollision(player);
        
        // Invincibility countdown
        if (player->invincibilityTimer > 0) player->invincibilityTimer--;
        
        // Powerup timer countdown
        if (player->powerupTimer > 0) {
            player->powerupTimer--;
            if (player->powerupTimer == 0) {
                // Restore normal color
                PlaySound(player, 0x40e28045);
                player->currentRGB = player->baseRGB;
            }
        }
        
        // Halo powerup management
        if (g_pPlayerState[0x17] & 1) {
            if (player->haloEntity == NULL) {
                player->haloEntity = CreateHaloEntity();
            }
        } else if (player->haloEntity != NULL) {
            DestroyEntity(player->haloEntity);
            player->haloEntity = NULL;
        }
        
        // Glide entity management (bit 2 of powerup state)
        if (g_pPlayerState[0x17] & 2) {
            if (player->glideEntity == NULL) {
                player->glideEntity = CreateGlideEntity();
            }
        } else if (player->glideEntity != NULL) {
            DestroyGlideEntity(player->glideEntity);
            player->glideEntity = NULL;
        }
    }
    
    // Scale transition (shrink/grow effects)
    if (!player->disableScale) {
        if (!player->shrinkFlag) {
            // Growing back to normal
            if (g_GameStatePtr[0x11c] == 0x10000 && player->scaleTarget < 0x10000) {
                player->scaleTarget += 0x1000;
            }
        } else {
            // Shrinking
            if (player->scaleTarget > 0x4000) {
                player->scaleTarget -= 0x1000;
            }
        }
    }
    
    // RGB modulation (damage flash, invincibility)
    UpdatePlayerRGB(player);
    
    // Particle spawning (every 8 frames)
    if (player->particleFlag && (g_GameStatePtr[0x10c] & 7) == 0) {
        SpawnParticle(player);
    }
}
```

## Tile Collision Processing

**Function**: `PlayerProcessTileCollision` @ 0x8005a914

Checks trigger zones at GameState+0x74 and handles tile attribute effects:

### Tile Attribute Values (SCIE Stage 0)
| Value | Hex | Name | Count | Effect |
|-------|-----|------|-------|--------|
| 0 | 0x00 | Empty | 21996 | No collision |
| 2 | 0x02 | Solid | 706 | Block movement |
| 18 | 0x12 | Trigger | 5 | Level trigger |
| 23 | 0x17 | Unknown | 1 | |
| 33 | 0x21 | Unknown | 1 | |
| 34 | 0x22 | Unknown | 1 | |
| 83 | 0x53 | Checkpoint | 14 | Save position |
| 101 | 0x65 | SpawnZone | 281 | Entity spawn area |

### Collision Handler Cases
| Case | Hex | Effect |
|------|-----|--------|
| 0 | 0x00 | Set GameState+0x148 |
| 2-7 | 0x02-0x07 | Change game mode |
| 42 | 0x2A | Enter water state |
| 50-59 | 0x32-0x3B | Collect clayball (index 0-9) |
| 61 | 0x3D | Push left |
| 62 | 0x3E | Push right |
| 63 | 0x3F | Push left + up |
| 64 | 0x40 | Push right + up |
| 65 | 0x41 | Push up |
| 81 | 0x51 | Enable X scroll (mode 1) |
| 82 | 0x52 | Enable Y scroll (mode 1) |
| 101 | 0x65 | Disable X scroll |
| 102 | 0x66 | Disable Y scroll |
| 121 | 0x79 | Enable X scroll (mode 2) |
| 122 | 0x7A | Enable Y scroll (mode 2) |

## Wall Collision

**Function**: `CheckWallCollision` @ 0x80059bc8

Checks 4 points vertically for solid tiles:
- Y-0x0F (15 pixels above feet)
- Y-0x10 (16 pixels)
- Y-0x20 (32 pixels)
- Y-0x30 (48 pixels - head height)

Returns `false` if ANY point hits tile value 0x65 ('e' = solid).

## Input System

Input controller at entity+0x100 points to:
```c
struct InputState {
    u16 buttons_held;     // Currently pressed
    u16 buttons_pressed;  // Just pressed this frame
    u16 unused;
    u8  demo_playback;    // 1 = playing demo
    u8  recording;        // 1 = recording input
    u16* demo_data;       // Demo input buffer
    // ...
};
```

### PSX Button Masks
| Mask | Button |
|------|--------|
| 0x0001 | Select |
| 0x0008 | Start |
| 0x0010 | Up |
| 0x0020 | Right |
| 0x0040 | Down |
| 0x0080 | Left |
| 0x0100 | L2 |
| 0x0200 | R2 |
| 0x0400 | L1 |
| 0x0800 | R1 |
| 0x1000 | Triangle |
| 0x2000 | Circle |
| 0x4000 | X |
| 0x8000 | Square |

## State Machine

State table at 0x800a5d20:

| State | Entry Handler | Description |
|-------|---------------|-------------|
| 0 | 0x80066ce0 | Respawn/checkpoint |
| 1 | 0x8006a310 | Facing left |
| 2 | 0x8006864c | Normal/idle |
| 3 | 0x8006ae0c | Unknown |

### Verified Player States (from gameplay trace)

**Standing Idle** @ 0x8006888C (`PlayerState_StandingIdle`)
- Entered when: No input or velocity stops
- Sprite: 0x1c395196 (if +0xCC == 0x388110), else 0x3838801a
- Movement callback: PlayerCallback_8006120c (or 80061934 if shrunk)
- Next state: PlayerStateCallback_0
- Observed: Frame 504, 572 (after pickup)

**Walking Right** @ 0x8006736C (`PlayerState_WalkingRight`)
- Entered when: Right input detected from idle
- Sprite: 0x292e8480 (same as running - direction handled by facing flag)
- Secondary callback: PlayerCallback_8005cc84
- Movement callback: PlayerCallback_800638d0 (or 80062ad4 if shrunk)
- Next state: Callback_800678d4 (falling state)
- Clears +0x156 field
- Observed: Frame 601, 618, 641 (repeated short walk bursts)

**Walking Left** @ 0x800674CC (`PlayerState_WalkingLeft`)
- Entered when: Left input detected from idle
- Sprite: 0x18298210 (mirrored version)
- Secondary callback: PlayerCallback_8005f540
- Movement callback: PlayerCallback_800638d0 (or 80062ad4 if shrunk)
- Next state: PlayerStateCallback_0
- Clears +0x156 field

**Running** @ 0x8006762C (`PlayerState_Running`)
- Entered when: Sustained horizontal input from walking
- Sprite: 0x292e8480 (running animation)
- Movement callback: PlayerCallback_800638d0 (same as walk)
- Next state: Callback_800678d4 (DIFFERENT from walk - leads to falling)
- Clears +0x156 field
- Calls FUN_8001d0c0(entity, 1)

**Jump** @ 0x80067E28 (`PlayerState_Jump`)
- Entered when: X button pressed on ground
- Sprite: 0x092b8480 (jump animation)
- Secondary callback: FUN_8005c1c4
- Movement callback: PlayerCallback_800638d0 (or 80062ad4 if shrunk)
- Sets +0x156 = 0x0C (jump parameter)
- Plays jump sound: FUN_8001c4a4(entity, 0x248e52)
- Calls FUN_8001ec18 with state table parameters
- Observed: Frame 731 (first jump), 1064 (second jump)

**Falling** @ 0x800678D4 (`PlayerState_Falling`)
- Entered when: Leaving ground (after run) or apex of jump
- Sprite: 0x0b2084d0 (falling/descending animation)
- Tick callback: PlayerCallback_8005bb80 (DIFFERENT from normal tick!)
- Secondary callback: FUN_8005c1c4 (same as jump)
- Movement callback: PlayerCallback_800638d0
- Sets +0x156 = 0x0C (if coming from jump 0x092b8480), else 0
- Clears +0x110 field
- Observed: Frame 653 (transition), 1059 (pre-jump)

**Death** @ 0x8006A0B8 (`PlayerState_Death`)
- Entered when: Hit by enemy with no health remaining
- Sprite: 0x1b301085 (death/explosion animation)
- Tick callback: PlayerTickCallback (normal)
- Secondary callback: PlayerCallback_8005d404
- Movement callback: PlayerCallback_80061180 (position update only)
- Clears +0x104/+0x108 callbacks (no movement processing)
- Sets g_GameStatePtr[0x170] = 0
- Sets +0x178 = 1 (entity flag)
- Sets +0x158 = 0 (clear field)
- Sets +0x168 = 1 (render flag)
- Observed: Frame 814 (death after monkey collision)

**Respawn** @ 0x80066CE0 (`PlayerStateCallback_0`)
- Entered when: Transitioning between movement states
- Multiple sprites: 0x48204012 (turn animation), 0x00388110 (other)
- Used as "next state" target by many other states
- Observed: Frame 604, 621 (between walk bursts)

**Normal/Idle** @ 0x8006864C (`PlayerStateCallback_2`)
- Entered when: Respawning or returning from death
- Sprite: 0x00388110 (idle standing)
- Observed: Frame 984 (after respawn)

**Pickup Item** @ 0x80068B48 (`PlayerState_PickupItem`)
- Entered when: Collision with clayball or collectible
- Sprite: 0x1c3aa013 (pickup animation)
- Tick callback: PlayerCallback_8005bbac (SPECIAL - not normal tick!)
- Movement callback: PlayerCallback_80064b40 (or 80064008 if shrunk)
- Next state: EntityInitCallback_80069600
- Sets g_GameStatePtr[0x60] = 1 (pickup flag)
- Sets velocity: 0x30000 (if bit match), else 0x20000, OR'd with 0x8000
- **WARNING**: This state may cause segfaults on rapid pickups due to SetEntitySpriteId spam
- Observed: Frame 521 (Klayman head?), 1183 (clayball), 1243 (crash)
- **Frequency**: 37 transitions in PHRO Stage 1 trace (frequent item collection)

**Checkpoint Activation (Ma-Bird)** @ 0x8006A214 (`PlayerState_CheckpointActivated`)
- **Purpose**: Activates checkpoint save and teleports player to exit
- **Sequence** (verified from trace frame 3810-4124):
  1. Player collides with Ma-Bird checkpoint entity at position (6739, 667)
  2. Transitions to this state (frame 3810)
  3. Calls `StopCDStreaming()` to pause audio
  4. Clears all entity callbacks except `EntityUpdateCallback`
  5. Freezes player in cutscene state (0x8001CB88) for ~160 frames
  6. Triggers `LevelLoad` event (frame 3969) - reloads same level
  7. Calls `SaveCheckpointState` @ 0x8007EAAC to save entity list
  8. Teleports player to checkpoint exit point (632, 927)
  9. Returns to normal gameplay in IdleLook state (frame 4216)
- **NOT a death/respawn** - this is the checkpoint save + exit teleport sequence
- **Fields modified**:
  - +0x1B2: Set to 1 (checkpoint active flag)
  - +0x5A: Checkpoint entity reference (gets +0x2C set to 1)
  - +0x4A, +0x43, +0x44: Cleared
- Related: `RestoreCheckpointEntities` @ 0x8007EAEC (called on death to respawn)
- Observed: Frame 3810 (single checkpoint activation in PHRO Stage 1)

### State Transition Flow (from trace)

```
[Spawn] 
  → Idle (0x8006888C, frame 504)
  → Pickup (0x80068B48, frame 521) 
  → Idle (0x8006888C, frame 572)
  → Walk Right (0x8006736C, frame 601)
  → Respawn (0x80066CE0, frame 604) 
  → Walk Right (0x8006736C, frame 618)
  → Respawn (0x80066CE0, frame 621)
  → Walk Right (0x8006736C, frame 641)
  → Jump (0x80067E28, frame 731)
  → Death (0x8006A0B8, frame 814)   ← Hit monkey
  → Normal (0x8006864C, frame 984)  ← Respawn
  → Falling (0x800678D4, frame 1059)
  → Jump (0x80067E28, frame 1064)
  → Pickup (0x80068B48, frame 1183)
  → Falling (0x800678D4, frame 1238)
  → Jump (0x80067E28, frame 1239)
  → Pickup (0x80068B48, frame 1243)  ← CRASH
```

## Spawn Position

Read from tile header (Asset 100) at offset 0x14-0x17:
- **SCIE Stage 0**: Tile (13, 20) = Pixel (216, 335)

## Scale Values
| Value | Meaning |
|-------|---------|
| 0x8000 | Half size (shrunk) |
| 0xC000 | 3/4 size |
| 0x10000 | Full size |

## Related Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x800596a4 | CreatePlayerEntity | Entity creation |
| 0x8005b414 | PlayerTickCallback | Main per-frame update |
| 0x8005a914 | PlayerProcessTileCollision | Tile attribute handling |
| 0x800245bc | CheckTriggerZoneCollision | Trigger zone lookup |
| 0x80059bc8 | CheckWallCollision | Wall collision check |
| 0x800241f4 | GetTileAttributeAtPosition | Tile attribute lookup |
| 0x8001eaac | EntitySetState | State transition |
| 0x8001cb88 | EntityUpdateCallback | Animation update |

## Example: SCIE Stage 0

- **Level size**: 535×45 tiles (8560×720 pixels)
- **Spawn**: (216, 335) pixels
- **Entity count**: 211
- **Level flags**: 0x0000 (normal platforming)
- **Unique tile attributes**: 8 types
- **Enemy type 25 count**: 8 instances
