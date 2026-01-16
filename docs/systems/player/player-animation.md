# Player Sprite & Animation System

This document describes how the player (Klaymen) sprite rendering, animation, direction, idle detection, scale effects, and powerups work.

## Overview

The player entity uses a sophisticated state machine for animations combined with:
- **Sprite flipping** for left/right direction (no separate left-facing sprites)
- **State callbacks** at entity+0x104/106/108 for behavior-specific updates
- **RGB modulation** for damage flash, invincibility, and color powerups
- **Scale transitions** for shrink mode and screen entrance effects
- **Powerup child entities** (halo, trail) that follow the player

## Entity Structure (Player-Specific Offsets)

The player entity is 0x1B4 bytes. Key offsets beyond the standard entity fields:

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0x34 | ptr | poly_ptr | GPU POLY_FT4 structure for rendering |
| 0x40 | ptr | input_ptr | Controller input state pointer |
| 0x50 | s32 | scale_current_x | Current X scale (fixed-point) |
| 0x54 | s32 | scale_current_y | Current Y scale (fixed-point) |
| 0x58 | s32 | scale_target | Target scale for transitions |
| 0x68 | s16 | x_pos | X position (pixels) |
| 0x6A | s16 | y_pos | Y position (pixels) |
| 0x74 | u8 | facing_left | 0=right, 1=left |
| 0xF0 | u8 | flip_x | Sprite horizontal flip flag |
| 0xF1 | u8 | loop_anim | Animation loops (1=yes) |
| 0xF2 | u8 | anim_enabled | Animation ticks if non-zero |
| 0x100 | ptr | checkpoint_ptr | Reference to checkpoint entity |
| 0x104 | u32 | state_offset | Secondary state machine offset |
| 0x106 | u16 | state_index | Secondary state machine index |
| 0x108 | ptr | state_callback | Secondary state callback function |
| 0x128 | u16 | damage_flash | Damage flash countdown timer |
| 0x144 | u16 | invincibility | Invincibility countdown timer |
| 0x14C | ptr | damage_entity | Damage effect entity pointer |
| 0x15A-0x15C | u8×3 | rgb_current | Current rendered RGB (modulated) |
| 0x15D-0x15F | u8×3 | rgb_base | Base RGB from GameState (checkpoint color) |
| 0x160 | s16 | x_velocity | Horizontal velocity |
| 0x162 | s16 | y_velocity | Vertical velocity |
| 0x168 | ptr | halo_entity | Halo powerup entity pointer |
| 0x16C | ptr | trail_entity | Trail powerup entity pointer |
| 0x17D | u8 | rgb_restore_timer | Countdown for RGB restore after powerup |
| 0x180 | u32×7 | sprite_ids | Available alternate sprite IDs |
| 0x19C | u8 | sprite_count | Count of available sprites |
| 0x19D | u8 | sprite_index | Current sprite (0xFF=default) |
| 0x1AE | u8 | damage_immune | Currently immune to pickups |
| 0x1AF | u8 | particle_spawn | Trigger particle spawn |
| 0x1B0 | u8 | shrink_mode | In shrink mode (small bounding box) |
| 0x1B3 | u8 | game_mode | Current game mode (from tile collision) |

## Player Creation (`CreatePlayerEntity` @ 0x800596a4)

```c
void CreatePlayerEntity(Entity* entity, void* input, short x, short y, char facingLeft) {
    // Initialize with sprite table (DAT_8009c174) and z_order 1000
    InitEntityWithSprite(entity, &DAT_8009c174, 1000, x, y);
    
    // Set main tick callback
    entity[1] = PlayerTickCallback;  // Player tick handler
    
    // Set secondary callback slot
    entity[8] = LAB_80061180;  // Additional update logic
    
    // Calculate scale from GameState+0x11c (or 0x8000 if shrink mode)
    uint scale = g_pPlayerState[0x18] ? 0x8000 : *(g_GameStatePtr + 0x11c);
    entity[0x16] = scale;  // Current scale
    entity[0x17] = scale;  // Target scale
    
    // Check which alternate sprites are available (7 IDs at DAT_8009c3a8)
    InitPlayerSpriteAvailability(entity);
    
    // Copy RGB from GameState spawn color
    entity[0x15D] = g_GameStatePtr[0x124];  // R
    entity[0x15E] = g_GameStatePtr[0x125];  // G
    entity[0x15F] = g_GameStatePtr[0x126];  // B
    
    // Select initial state based on respawn flag
    if (g_GameStatePtr[0x161] != 0) {
        // Respawning - use respawn state
        EntitySetState(entity, DAT_800a5d20, PTR_LAB_800a5d24);  // Idle facing right
    } else {
        // Normal spawn
        if (facingLeft) {
            EntitySetState(entity, DAT_800a5d28, PTR_LAB_800a5d2c);  // Idle facing left
        } else {
            EntitySetState(entity, DAT_800a5d30, PTR_LAB_800a5d34);  // Walk facing right
        }
    }
    
    // Create halo powerup if active
    if (g_pPlayerState[0x17] & 1) {
        entity[0x5A] = CreateHaloEntity(AllocateFromHeap(..., 0x30), entity);
    }
}
```

## Player State Machine

States are stored at address tables starting at 0x800a5d20. Each entry is 8 bytes:
- 4 bytes: State parameter (usually 0xFFFF0000)
- 4 bytes: Callback function pointer

### State Callbacks (from DAT_800a5d20)

| Offset | Parameter | Callback | State |
|--------|-----------|----------|-------|
| 0x00 | 0xFFFF0000 | 0x80066ce0 | Idle (facing right) |
| 0x08 | 0xFFFF0000 | 0x8006a310 | Idle (facing left) |
| 0x10 | 0xFFFF0000 | 0x8006864c | Walk/run (facing right) |
| 0x18 | 0xFFFF0000 | 0x8006ae0c | Walk/run (facing left) |

State transitions occur via `EntitySetState(entity, param, callback)` which:
1. Clears pending state flags (entity+0xA0 through 0xA4)
2. Calls exit callback if there's a pending transition
3. Sets new state parameters and callback
4. Calls entry callback

## Direction & Sprite Flipping

The game uses **sprite flipping** rather than separate left/right sprites.

### How Direction Works

1. **Direction Flag**: `entity[0x74]` stores facing direction (0=right, 1=left)
2. **Flip Flag**: `entity[0xF0]` controls horizontal flip during render
3. **State Selection**: Different state callbacks for left vs right movement

### Flip in GetFrameMetadata (@ 0x8007bebc)

```c
void GetFrameMetadata(SpriteContext* ctx, RenderParams* params, 
                      uint frame, uint dst_addr, uint stride, 
                      byte flip_x, char flip_y) {
    // ... setup code ...
    
    if (flip_y != 0) {
        params->stride = -params->stride;  // Negate row stride
        params->dst = dst + stride * (height - 1);  // Start from bottom
    }
    
    if (flip_x != 0) {
        params->dst = dst + (width - 1);  // Start from right edge
    }
}
```

The RLE decoder (@ 0x80010068) then processes in reverse:
- Normal: dst += skip, dst += 1 per pixel
- Flipped: dst -= skip, dst -= 1 per pixel

## Animation System

### Animation Offsets (Entity)

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| 0xD8 | u16 | frame_count | Total frames in animation |
| 0xDA | u16 | current_frame | Current frame index |
| 0xDC | u16 | loop_frame | Frame to loop back to |
| 0xDE | u16 | end_frame | Last frame before loop/stop |
| 0xE0 | u16 | pending_flags | Bit flags for pending changes |
| 0xEC | u16 | frame_timer | Countdown to next frame |

### Pending Flags (entity+0xE0)

| Bit | Mask | Purpose |
|-----|------|---------|
| 0-1 | 0x03 | State: 0=none, 1=immediate, 2=deferred, 3=queued |
| 2 | 0x04 | Sprite change pending |
| 3 | 0x08 | Animation start frame pending |
| 4 | 0x10 | Loop frame pending |
| 5 | 0x20 | End frame pending |
| 6 | 0x40 | Flip X pending |
| 7 | 0x80 | Loop mode pending |
| 8 | 0x100 | Anim enabled pending |

### Animation Tick (`TickEntityAnimation` @ 0x8001d290)

Each frame:
1. Decrement frame_timer (entity+0xEC)
2. When timer hits 0:
   - Check if at end_frame (0xDE)
   - If looping (0xF1 set), jump to loop_frame (0xDC)
   - Otherwise advance current_frame (0xDA)
3. Call `UpdateSpriteFrameData` to update render params

### Frame Advancement (`AdvanceAnimationFrame`)

```c
void AdvanceAnimationFrame(Entity* entity) {
    short current = entity[0xDA];
    
    if (current == entity[0xDE]) {  // At end frame
        if (entity[0xF1]) {         // Loop enabled
            entity[0xDA] = entity[0xDC];  // Jump to loop point
        }
    } else if (entity[0xF0] == 0) { // Normal direction
        entity[0xDA] = current + 1;
        if (entity[0xDA] >= entity[0xD8]) {
            entity[0xDA] = 0;  // Wrap to start
        }
    } else {                        // Reverse direction
        entity[0xDA] = current - 1;
        if (entity[0xDA] < 0) {
            entity[0xDA] = entity[0xD8] - 1;  // Wrap to end
        }
    }
}
```

## Idle Animation Detection

The idle animation system works via state callbacks, not a timer:

1. **State-Based**: Each state callback (e.g., 0x80066ce0 for idle-right) handles its own animations
2. **Input Detection**: The state callback checks for no input to trigger idle animation
3. **Idle Variations**: Multiple idle animations may be selected based on time spent idle

The state callback at 0x80066ce0 (idle facing right):
- Monitors input for movement commands
- If no input, plays standing idle animation
- Extended idle may trigger special animations (looking around, etc.)

## RGB Modulation & Damage Effects

### Per-Frame RGB Processing (`PlayerTickCallback`)

```c
// Damage flash effect
if (entity[0x128] != 0) {  // damage_flash timer
    entity[0x128]--;
    // Cycle RGB based on frame counter for flash effect
    int phase = g_GameStatePtr[0x10C] % 3;
    // Apply different colors per phase
}

// Invincibility timer
if (entity[0x144] != 0) {
    entity[0x144]--;
    if (entity[0x144] == 1) {
        // Restore normal sprite
    }
}

// RGB restore delay (after color powerup)
if (entity[0x17D] != 0) {
    entity[0x17D]--;
    if (entity[0x17D] == 0) {
        // Restore base RGB from 0x15D-0x15F
    }
}
```

### Color Sources

| Priority | Source | Description |
|----------|--------|-------------|
| 1 | Damage flash | Rapid color cycling when hit |
| 2 | Powerup effect | Temporary color from pickup |
| 3 | Base RGB | From GameState+0x124 (checkpoint color) |

## Scale System

### Scale Values (Fixed-Point 16.16)

| Value | Percentage | Usage |
|-------|------------|-------|
| 0x8000 | 50% | Shrink mode (small Klaymen) |
| 0xC000 | 75% | Medium scale |
| 0x10000 | 100% | Normal size |

### Scale Transition (`PlayerTickCallback`)

```c
// Interpolate toward target scale
if (entity[0x50] < entity[0x58]) {
    entity[0x50] += 0x1000;  // +6.25% per frame
} else if (entity[0x50] > entity[0x58]) {
    entity[0x50] -= 0x1000;
}
// Same for Y scale (entity[0x54])
```

### Shrink Mode

When `g_pPlayerState[0x18] != 0`:
- Player uses 50% scale (0x8000)
- Bounding box reduced to (-5, -10, 10, 10)
- Can access smaller passages

## Halo Powerup (`CreateHaloEntity` @ 0x8006de98)

Created when `g_pPlayerState[0x17] & 1`:

```c
Entity* CreateHaloEntity(void* memory, Entity* player) {
    // 0x30 byte entity
    InitEntityStruct(entity, 0x30);
    entity[7] = player;  // Parent reference
    
    // Create particle child (0x1E8 bytes) via InitHUDIconEntity
    Entity* particle = AllocateFromHeap(..., 0x1E8);
    InitHUDIconEntity(particle);
    
    // Position above player head
    // z_order = 0x3E9 (1001)
    return entity;
}
```

The halo entity follows the player and creates a glowing particle effect above their head.

## Yellow Bird Powerup (`CreateYellowBirdEntity` @ 0x8006e1d8)

Created when `g_pPlayerState[0x17] & 2`:

```c
Entity* CreateYellowBirdEntity(void* memory, Entity* player) {
    // 0x110 byte entity with sprite DAT_8009c3c4
    InitEntityWithSprite(entity, &DAT_8009c3c4, 999, ...);
    
    entity[0x44] = player;  // Parent reference
    
    // Uses same scale as player
    // Offset based on facing direction
    if (player->facing_left) {
        x_offset = +16;
    } else {
        x_offset = -16;
    }
    
    return entity;
}
```

The trail entity follows behind the player, offset based on facing direction.

## Alternate Sprites (`InitPlayerSpriteAvailability` @ 0x80059a70)

The player can use alternate sprites if they exist in the level's sprite container:

```c
void InitPlayerSpriteAvailability(Entity* entity) {
    uint* sprite_table = &DAT_8009c3a8;  // 7 sprite IDs
    uint* available = entity + 0x180;
    int count = 0;
    
    for (int i = 0; i < 7; i++) {
        uint sprite_id = sprite_table[i];
        if (LookupSpriteById(sprite_id) != NULL) {
            available[count++] = sprite_id;
        }
    }
    
    entity[0x19C] = count;      // Available count
    entity[0x19D] = 0xFF;       // Current = default
}
```

### Sprite Table (DAT_8009c3a8)

7 hardcoded sprite IDs for alternate player appearances. Only sprites present in the current level's container are available.

## Particle Spawning

Triggered when `entity[0x1AF] != 0` and `(frameCount & 7) == 0`:

```c
if (entity[0x1AF] && (g_GameStatePtr[0x10C] & 7) == 0) {
    Entity* particle = AllocateFromHeap(..., size);
    CreatePlayerParticleEntity(particle);  // Initialize particle
    AddToZOrderList(g_GameStatePtr, particle);
    AddToXPositionList(g_GameStatePtr, particle);
}
```

Spawns particle every 8 frames when flag is set.

## Key Functions

| Address | Function | Purpose |
|---------|----------|---------|
| 0x800596a4 | CreatePlayerEntity | Create and initialize player |
| 0x8005b414 | PlayerTickCallback | Per-frame player update |
| 0x80059a70 | InitPlayerSpriteAvailability | Check alternate sprites |
| 0x8006de98 | CreateHaloEntity | Create halo powerup entity |
| 0x8006e1d8 | CreateYellowBirdEntity | Create yellow bird powerup entity |
| 0x8001cb88 | EntityUpdateCallback | Generic entity update |
| 0x8001d290 | TickEntityAnimation | Animation frame advancement |
| 0x8001d4bc | AdvanceAnimationFrame | Next frame calculation |
| 0x8001d748 | UpdateSpriteFrameData | Copy frame metadata to entity |
| 0x8007bebc | GetFrameMetadata | Get frame render parameters |
| 0x80010068 | DecodeRLESprite | Decode sprite with flip support |
| 0x800362a4 | CreatePlayerParticleEntity | Create player trailing particle |

## State Callback Addresses

| Address | Purpose |
|---------|---------|
| 0x80066ce0 | Idle state (facing right) |
| 0x8006a310 | Idle state (facing left) |
| 0x8006864c | Walk/run state (facing right) |
| 0x8006ae0c | Walk/run state (facing left) |

These are labeled `LAB_*` in Ghidra as they aren't recognized as function starts.

## Related Documentation

- [Sprites](sprites.md) - Sprite format and RLE decoding
- [Entities](entities.md) - General entity system
- [Game Loop](game-loop.md) - CreatePlayerEntity dispatch
