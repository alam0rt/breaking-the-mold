# FINN Player Entity (Boat/Controllable Fish Levels)

The FINN player handles boat/controllable fish levels where the player uses tank controls
to navigate. The vehicle has rotation-based steering and forward momentum.

## Level Flag

FINN levels are identified by flag `0x0400` in the tile header (Asset 100).
This flag takes priority over other player types in the dispatch order.

## Creation

**Function**: `CreateFinnPlayerEntity` @ 0x80074100

```c
Entity* CreateFinnPlayerEntity(void* buffer, void* inputController, 
                                short spawn_x, short spawn_y) {
    Entity* entity = buffer;
    
    // Initialize with sprite and z-order
    InitEntityWithSprite(entity, &DAT_8009caec, 1000, spawn_x, spawn_y);
    
    // Allocate secondary sprite (shadow/wake effect)
    Entity* shadow = AllocateFromHeap(g_BLBHeader, 0x100, 1, 0);
    InitEntitySprite(shadow, 0x3da80d13, 0x3e9, spawn_x, spawn_y, 0);
    shadow->vtable = 0x80011e14;
    
    // Store input controller and secondary sprite
    entity[0x104] = 0;         // X velocity (16.16 fixed-point)
    entity[0x108] = 0;         // Y velocity (16.16 fixed-point)
    entity[0x10c] = 0;         // Rotation angle accumulator
    entity[0x10f] = 0;         // Rotation velocity (signed char)
    entity[0x100] = inputController;
    entity[0x104] = shadow;
    
    // Set up main tick callback (inline code at 0x800742c8)
    entity[0x00] = 0xffff0000;
    entity[0x04] = 0x800742c8;  // Main tick handler
    
    // Set up secondary callback (0x80074a40)
    entity[0x1c] = 0xffff0000;
    entity[0x20] = 0x80074a40;  // Position update callback
    
    // Initialize animation system
    entity[0x110] = FUN_8001c4a4(entity, 0xcc6c8070);
    
    // Initialize state machine
    EntitySetState(entity, null_FFFF0000h_800a601c, PTR_LAB_800a6020);
    
    return entity;
}
```

## Entity Structure (0x120+ bytes)

Standard entity base (0x100 bytes) plus FINN-specific fields:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x68 | 2 | x_pos | Current X position (pixels) |
| 0x6A | 2 | y_pos | Current Y position (pixels) |
| 0x100 | 4 | inputController | Pointer to button state (g_pPlayer1Input) |
| 0x104 | 4 | xVelocity | X velocity (16.16 fixed-point) |
| 0x108 | 4 | yVelocity | Y velocity (16.16 fixed-point) |
| 0x10C | 2 | rotationAngle | Current rotation angle (0-0x400 = 0-360°) |
| 0x10F | 1 | rotationVel | Rotation velocity (signed char, ±0x40 max) |
| 0x110 | 1 | moveFlag | Set to 1 when forward button pressed |
| 0x111 | 1 | turnFlag | Set to 1 when turning |
| 0x41 | 4 | shadowEntity | Secondary sprite entity pointer (wake/shadow) |

## Tank Controls

**Function**: `FinnHandleInput` @ 0x8006fbd0

Button masks (PSX controller):
- `0x8000` - Up/D-Pad Up → Turn left (rotate counter-clockwise)
- `0x2000` - Down/D-Pad Down → Turn right (rotate clockwise)
- `g_DefaultBGColorR` (action button) → Move forward in facing direction

### Rotation System

```c
void FinnHandleInput(Entity* entity) {
    u16* input = entity->inputController;
    char rotVel = entity[0x10f];  // Current rotation velocity
    
    if (*input & 0x2000) {  // Down - turn right
        entity[0x111] = 1;  // Set turn flag
        rotVel += 0x10;
        if (rotVel > 0x40) rotVel = 0x40;  // Clamp max
    }
    else if (*input & 0x8000) {  // Up - turn left
        entity[0x111] = 1;  // Set turn flag
        rotVel -= 0x10;
        if (rotVel < -0x40) rotVel = -0x40;  // Clamp min
    }
    else {
        // Apply drag when no turn input
        if (rotVel > 0) {
            rotVel -= 8;
            if (rotVel < 0) rotVel = 0;
        }
        else if (rotVel < 0) {
            rotVel += 8;
            if (rotVel > 0) rotVel = 0;
        }
    }
    
    entity[0x10f] = rotVel;
    entity[0x10c] += rotVel;  // Update angle accumulator
    
    // Forward movement when action button pressed
    if (*input & g_DefaultBGColorR) {
        short angle = entity[0x10c];
        // Apply velocity based on facing direction
        entity[0x108] -= (csin(0x400 - angle) << 13) >> 12;  // Y velocity
        entity[0x104] += (ccos(0x400 - angle) << 13) >> 12;  // X velocity
        entity[0x110] = 1;  // Set move flag
    }
}
```

### Control Characteristics

- **Rotation velocity**: ±0x10 per frame when turning, ±8 drag when not
- **Max rotation speed**: ±0x40 (clamped)
- **Forward speed**: Calculated via sin/cos, scaled by (value << 13) >> 12
- **Angle range**: 0-0x400 (wraps around, 0x400 = 360 degrees)
- **No reverse**: Only forward movement available

## State Machine

**State Table**: Located at 0x800a601c
**Main Tick Callback**: 0x8006EFC8 (`FinnMainTickHandler`)

The main player callback at 0x8006EFC8 is the active state handler that:
1. Manages secondary entity (wake/shadow) at +0x114
2. Handles random state timing at +0x112 (counter decrements, resets to 0x14 via rand())
3. Calls movement/collision subsystems:
   - `FinnCheckTriggerZones` @ 0x80070128 - Trigger zone collision detection
   - `FinnHandleInput` @ 0x8006fbd0 - Tank control input handling
   - `FUN_8006f250` - Unknown (possibly physics/constraints)
   - `FUN_8006fd48` - Unknown (possibly animation)
   - `EntityUpdateCallback` - Standard entity update

Format: 8-byte entries with END2 terminators
```
struct StateEntry {
    u16 param1;        // Usually 0
    u16 param2;        // Usually 0xFFFF
    void* handler;     // Function pointer
};
```

### Initial State Table (0x800a601c)

| Index | Address | Name | Description |
|-------|---------|------|-------------|
| 0 | 0x80074bf4 | FinnStateInit | Initial/spawn state |
| 1 | 0x80074db0 | FinnStateActive | Main navigation state |
| 2 | 0x80074c84 | FinnStateTurn | Direction change transition |
| 3 | 0x80074d18 | FinnStateSpecial | Special action/collision response |

Note: The observed runtime callback 0x8006EFC8 is NOT in this table - it's the
main entity tick handler that dispatches to these states via EntitySetState().

### Death/Explosion State @ 0x8006FE94

When the player hits a mine or dies, transition to `Callback_8006fe94` which:
1. Spawns projectile entities in a loop (explosion particles)
2. Changes sprite to 0x88c5011 (death animation)
3. Sets new callbacks:
   - Main: EntityUpdateCallback
   - Secondary: EntityInitCallback_8006f224
   - Tertiary (+0x98): EntityInitCallback_80070064
4. Calls FUN_8001d240(entity, 0) to trigger death sequence

## Secondary Callback @ 0x80074a40

Position update callback that:
1. Gets RGB modulation (visual effects)
2. Checks specific byte patterns (0xCB/0xC9) for special states
3. Applies position clamping/bounds
4. Syncs position with secondary sprite

## Related Functions

| Address | Name | Purpose |
|---------|------|---------|  
| 0x80074100 | CreateFinnPlayerEntity | Entity creation and initialization |
| 0x800742c8 | (inline tick) | Main tick callback (inline code, not a function) |
| 0x80074a40 | (inline secondary) | Secondary position update callback |
| 0x8006EFC8 | FinnMainTickHandler | Main active state handler (GLIDE reuses this) |
| 0x8006fbd0 | FinnHandleInput | Tank control input and rotation |
| 0x8006f250 | FUN_8006f250 | Physics/constraints subsystem |
| 0x8006fd48 | FUN_8006fd48 | Animation subsystem |
| 0x8006FE94 | FinnDeathExplosion | Death/explosion state |
| 0x80070128 | FinnCheckTriggerZones | Trigger zone collision detection |
| 0x80074b54 | FinnStateIdle | Idle state handler |
| 0x8001fea8 | EntityApplyMovementCallbacks | X/Y movement dispatch |
| 0x800241f4 | GetTileAttributeAtPosition | Collision lookup |
| 0x8001eaac | EntitySetState | State machine dispatch | |

## Collision & Pickups

### Trigger Zones (`FinnCheckTriggerZones` @ 0x80070128)

The player checks for trigger zone collisions which handle:
- **0x00**: State transition (sets callback at 0x80070094)
- **0x51**: Trigger type 1 (FUN_80025664 with param 1)
- **0x52**: Trigger type with data (FUN_800256b8)
- **0x65**: Trigger type 0 (FUN_80025664 with param 0)
- **0x66**: Trigger type with data 0
- **0x79**: Trigger type 2 (FUN_80025664 with param 2)
- **0x7a**: Trigger type with data 2
- Other values passed to FUN_8007ee6c for custom handling

Pickups mentioned in trace:
- Klayman head (extra life) - pickup entity type
- Clayball (weapon/throwable) - pickup entity type  
- Sea mine - collision triggers death state (0x8006FE94)

## Sprites

- Main sprite ID: `0x3da80d13` (boat/fish vehicle)
- Secondary sprite: Separate entity at +0x41 (wake/shadow effect)
- Z-orders: 1000 (main), 0x3e9 (shadow)
- Death sprite: `0x88c5011` (explosion animation)

## Collision

Uses `GetTileAttributeAtPosition` @ 0x800241f4 for tile-based collision.
The function queries the tile attribute map (Asset 500) to determine
solid tiles, water, hazards, etc.

## Vehicle Data (Asset 504)

FINN levels use Asset 504 for vehicle path data:
- 64-byte waypoints
- Count stored in tile header at offset 0x16
- FINN level has 78 waypoints typically

## Level Identification

To check if a level is a FINN level:
```c
u16 flags = GetLevelFlags(ctx);  // @ 0x8007b47c
if (flags & 0x0400) {
    // This is a FINN swimming level
}
```

## Ghidra Notes

The tick callbacks at 0x800742c8 and 0x80074a40 are not defined as
functions in Ghidra - they are inline code after CreateFinnPlayerEntity.
Labels can be found at LAB_800742c8 and LAB_80074a40.

The main active state handler EntityInitCallback_8006efc8 (0x8006EFC8) is
called every frame and manages the boat physics, not the state table handlers.

Memory at 0x800a601c contains the initial FINN state table with "END2" string
markers separating state groups. This table is used during initialization but
the runtime behavior is controlled by 0x8006EFC8.

## Trace Analysis Summary (Level 4, Stage 1)

From runtime trace of FINN level:
- **Frames 300-600**: Player at (473,639) → (538,602), navigating with tank controls
- **Frame 1056**: Death triggered, state change to 0x8006FE94, sprite → 0x88c5011
- **Frame 1138**: Level reload (respawn after death)
- Player callback during gameplay: 0x8006EFC8 (not state table addresses)
- Animation: 8-frame cycle (0-7) at ~4 frames per anim frame
