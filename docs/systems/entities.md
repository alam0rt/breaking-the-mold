# Entity System

Entities are game objects combining sprite graphics, behavior callbacks, and position/state data.

## Overview

The entity system has three key aspects:
1. **Entity data** - BLB Asset 501 stores placement (24-byte structures)
2. **Entity behavior** - Hardcoded init functions with sprite IDs
3. **Entity struct** - Runtime 0x44C byte structure

**Critical**: Entity type → sprite ID mapping is **HARDCODED** in game code, not in BLB data.

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

| Type | Name | Count | Description |
|------|------|-------|-------------|
| 2 | Clayball | 5,727 | Collectible coins |
| 3 | Ammo | 308 | Bullet pickup |
| 8 | Item | 144 | Generic item pickup |
| 9 | Unknown | - | Uses extended layer flags |
| 24 | SpecialAmmo | 227 | Special ammunition |
| 25, 27 | Enemy | - | Enemy entities |
| 28, 48 | Platform | - | Moving platforms |
| 45 | MessageBox | - | In-game messages |
| 64, 103 | Unknown | - | Various types |
| 81 | Unknown | - | Uses extended layer flags |

## Runtime Entity Structure (0x44C bytes)

Based on Ghidra decompilation:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | state_high | State machine upper word |
| 0x04 | 4 | callback_main | Main update callback |
| 0x08 | 4 | state2_high | Secondary state |
| 0x0C | 4 | callback2 | Secondary callback |
| 0x18 | 4 | vtable | Method pointer table |
| 0x24 | 4 | callback3 | Tertiary callback |
| 0x28 | 4 | callback4 | |
| 0x2C | 4 | callback5 | |
| 0x30 | 4 | callback6 | |
| 0x34 | 4 | sprite_ptr | Pointer to sprite/POLY |
| 0x68 | 2 | x_pos | X position |
| 0x6A | 2 | y_pos | Y position |
| 0xF6 | 1 | visibility | Rendering flag |
| 0xF7 | 1 | load_flag | Sprite load method |
| 0x100+ | ... | Extended | Entity-specific data |

## Entity Lifecycle

```
1. ALLOCATION
   FUN_800143f0(blbHeaderBuffer, size, 1, 0)
   └── Allocates from BLB buffer

2. INITIALIZATION
   InitEntitySprite(entity, sprite_id, z_order, x, y, flags)
   ├── FUN_8001a0c8() - Zero/init struct
   ├── FUN_8007bbc0() - GPU/render state
   ├── FUN_8001954c() - Animation state
   ├── FUN_8001c980() - Entity setup
   ├── FUN_8001cdac/d024() - Load sprite
   └── FUN_8001d080() - Finalize

3. CALLBACK ASSIGNMENT
   entity[1] = update_callback;
   entity[3] = secondary_callback;
   entity[10] = collision_callback?
   entity[12] = destroy_callback?

4. REGISTRATION
   FUN_800213a8(GameState, entity) - Add to list
   FUN_80021190(GameState, entity) - Add to queue

5. TICK LOOP
   EntityTickLoop (0x80020e1c)
   └── Iterates entities, calls entity[1] callback

6. DESTRUCTION
   (Not yet fully analyzed)
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

| Function | Address | Purpose |
|----------|---------|---------|
| `InitEntitySprite` | 0x8001c720 | Core entity sprite init |
| `EntityTickLoop` | 0x80020e1c | Main update loop |
| `LoadEntitiesFromAsset501` | 0x80024dc4 | Load entity defs |
| `GetEntityCount` | 0x8007b7a8 | Entity count accessor |
| `FUN_800213a8` | 0x800213a8 | Add to entity list |
| `FUN_80021190` | 0x80021190 | Add to update queue |
| `InitPlayerEntity` | 0x8001fcf0 | Player setup |
| `InitBossEntity` | 0x80047fb8 | Boss setup |

## LevelDataContext Entity Offsets

| Offset | Description |
|--------|-------------|
| +0x38 (ctx[14]) | Asset 501 pointer (entity data) |
| +0x1C (ctx[7]) | Active entity linked list head |
| +0x28 (ctx[10]) | Entity definition list |

## Sprite ID Lookup

Entity type → sprite ID is hardcoded in init functions. The BLB does NOT contain this mapping.

To add a new entity type → sprite mapping, you must:
1. Find the init function in Ghidra
2. Extract the sprite ID constant
3. Verify the sprite exists in the level's tertiary container

## Related Documentation

- [Sprites](sprites.md) - Sprite data format
- [Rendering Order](rendering-order.md) - Entity z_order
- [Asset Types](../blb/asset-types.md) - Asset 501 details
- [Runtime Behavior](level-loading.md) - Entity loading flow
