# FINN Player Entity (Swimming Levels)

The FINN player handles swimming/underwater levels where the player controls a flying fin vehicle.

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
    InitEntityWithSprite(entity, &DAT_8009cae8, 1000, spawn_x, spawn_y);
    
    // Allocate secondary sprite (shadow/effect)
    Entity* shadow = AllocateFromHeap(g_BLBHeader, 0x100, 1, 0);
    shadow->vtable = 0x80011e14;  // Shadow callback
    InitEntitySprite(shadow, 0x3da80d13, 0x3e9, spawn_x, spawn_y, 0);
    
    // Store input controller and secondary sprite
    entity[0x104] = shadow;
    entity[0x100] = inputController;
    entity[0x108] = 0;  // Y velocity
    entity[0x10C] = 0;  // State flags
    
    // Set up main tick callback (inline code at 0x800742c8)
    entity[0x00] = 0;
    entity[0x04] = LAB_800742c8;
    
    // Set up secondary callback (0x80074a40)
    entity[0x1C] = 0;
    entity[0x20] = LAB_80074a40;
    
    // Initialize state machine
    EntitySetState(entity, DAT_800a601c, PTR_LAB_800a6020);
    
    // Add to render list
    AddEntityToSortedRenderList(g_GameStatePtr, entity);
    
    return entity;
}
```

## Entity Structure (0x114 bytes)

Standard entity base (0x100 bytes) plus FINN-specific fields:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x68 | 2 | x_pos | Current X position (pixels) |
| 0x6A | 2 | y_pos | Current Y position (pixels) |
| 0x100 | 4 | inputController | Pointer to button state (g_pPlayer1Input) |
| 0x104 | 4 | shadowEntity | Secondary sprite entity pointer |
| 0x108 | 4 | yVelocity | Y velocity (16.16 fixed-point) |
| 0x10C | 1 | stateFlags | Current state flags |

## Input Handling

Button masks (PSX controller):
- `0x8000` - Up → Decrease Y velocity (swim up)
- `0x2000` - Down → Increase Y velocity (swim down)
- `0xF0` - D-pad → Trigger state transition

Velocity change per frame: ±0x20000 (2.0 in 16.16 fixed-point)

## State Machine

**State Table**: Located at 0x800a601c

Format: 8-byte entries with END2 terminators
```
struct StateEntry {
    u16 param1;        // Usually 0
    u16 param2;        // Usually 0xFFFF
    void* handler;     // Function pointer
};
```

### State Handlers

| Index | Address | Name | Description |
|-------|---------|------|-------------|
| 0 | 0x80074bf4 | FinnStateEnter | Initial/entry state |
| 1 | 0x80074db0 | FinnStateSwim | Active swimming |
| 2 | 0x80074c84 | FinnStateTurn | Direction change |
| 3 | 0x80074d18 | FinnStateSpecial | Special action |

### FinnStateIdle @ 0x80074b54

The main swimming state handler:

```c
void FinnStateIdle(Entity* entity) {
    // Apply X/Y movement via state callbacks
    EntityApplyMovementCallbacks(entity, entity->x_pos, entity->y_pos);
    
    // Check Up button (0x8000)
    u16* inputPtr = entity->inputController;
    if (*inputPtr & 0x8000) {
        entity->yVelocity -= 0x20000;  // Swim up
    }
    else if (*inputPtr & 0x2000) {
        entity->yVelocity += 0x20000;  // Swim down
    }
    
    // D-pad triggers state change
    if ((*(inputPtr + 1) & 0xF0) != 0) {
        EntitySetState(entity, DAT_800a6034, PTR_LAB_800a6038);
    }
}
```

## Secondary Callback @ 0x80074a40

Position update callback that:
1. Gets RGB modulation (visual effects)
2. Checks specific byte patterns (0xCB/0xC9) for special states
3. Applies position clamping/bounds
4. Syncs position with secondary sprite

## Related Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x80074100 | CreateFinnPlayerEntity | Entity creation |
| 0x80074b54 | FinnStateIdle | Main swimming handler |
| 0x8001fea8 | EntityApplyMovementCallbacks | X/Y movement dispatch |
| 0x800241f4 | GetTileAttributeAtPosition | Collision lookup |
| 0x8001eaac | EntitySetState | State machine dispatch |

## Sprites

- Main sprite ID: `0x3da80d13`
- Shadow sprite: Separate entity at +0x104
- Z-orders: 1000 (main), 0x3e9 (shadow)

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
The function FUN_80074b54 (renamed to FinnStateIdle) is the only defined
state handler function.

Memory at 0x800a601c contains the FINN state table with "END2" string
markers separating state groups.
