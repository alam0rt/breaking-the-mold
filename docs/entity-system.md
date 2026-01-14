# Entity System Analysis

## Overview

Entities in Skullmonkeys use a **factory pattern** with type-based dispatch:

1. **Entity Type Callbacks** (EntityTypeXXX_InitCallback) - ONE-TIME initialization functions that:
   - Allocate memory from heap via `AllocateFromHeap(blbHeaderBufferBase, size, 1, 0)`
   - Call type-specific init function to set up entity structure
   - **Set the per-frame tick callback** at `entity+0x18` (the REAL update function)
   - Register entity in game loops (render list, update queue, or tick list)

2. **Per-Frame Tick Callbacks** - Stored at `entity+0x18`, called every frame by `EntityTickLoop`
   - Handles entity behavior, movement, collision, animation
   - Different for each entity type/state
   - Can be changed via `EntitySetState()` for state machine transitions

3. **Entity Structure** - Combines sprite graphics, behavior callbacks, and state data

### Key Architectural Insight

The entity type callbacks in `g_EntityTypeCallbackTable` @ `0x8009D5F8` were originally misnamed as "TickCallback" but are actually **factory/init functions** that execute once when an entity spawns. They set up the entity and assign the real per-frame callback at `entity+0x18`.

**Verified via runtime tracing**: Boss level (HEAD/Joe-Head-Joe) gameplay showed player state machine callbacks changing frequently via `EntitySetState`, confirming that per-frame logic uses dynamically assigned callbacks, not the factory functions.

## Entity Lifecycle

### 1. Spawning (Factory Pattern)

Entity type callbacks are invoked from `g_EntityTypeCallbackTable` @ `0x8009D5F8` (121 entity types, 82 unique factory functions):

```c
// Example: EntityType071_InitCallback (Boss) @ 0x80080FEC
void EntityType071_InitCallback(GameState *gameState, EntityDef *entityDef) {
    // Step 1: Allocate entity memory from heap
    Entity *entity = AllocateFromHeap(blbHeaderBufferBase, 0x148, 1, 0);
    
    // Step 2: Initialize entity structure (type-specific)
    entity = InitBossEntity(entity, entityDef);
    // InitBossEntity sets entity+0x18 to per-frame tick callback
    // and spawns 6 child entities
    
    // Step 3: Register in game loops
    AddEntityToSortedRenderList(gameState, entity);
    AddToUpdateQueue(gameState, entity);
}
```

**Factory callback patterns** (verified via Ghidra decompilation):
- **Render-only**: `AddEntityToSortedRenderList` only (static decorations)
- **Render + Update**: Both render list and update queue (active entities)
- **Tick-only**: `AddToZOrderList` for special z-sorted entities (type 80)

### 2. Per-Frame Updates

`EntityTickLoop` @ `0x80020E1C` iterates the tick list (GameState+0x1C) and calls each entity's callback at `entity+0x18`:

```c
void EntityTickLoop(GameState *gameState) {
    for (Entity *ent = gameState->tickList; ent != NULL; ent = ent->next) {
        void (*tickCallback)(Entity*) = ent->callback_at_0x18;
        tickCallback(ent);  // Per-frame logic
    }
}
```

### 3. State Transitions

`EntitySetState` @ 0x8001EAAC changes the tick callback dynamically:

```c
void EntitySetState(Entity *entity, u32 stateFlags, void (*newCallback)(Entity*)) {
    entity->state = stateFlags;
    entity->callback_at_0x18 = newCallback;  // Switch behavior
}
```

**Example from player trace** (HEAD boss level gameplay):
- Frame 509: `0x8006A3F8` (PlayerState_CheckpointExit) - respawn
- Frame 536: `0x8006888C` - normal gameplay  
- Frame 552: `0x80066CE0` (PlayerStateCallback_0) - death/respawn
- Frame 2804: `0x8006A310` (PlayerStateCallback_1) - special mode

### 4. Destruction

`RemoveEntityFromAllLists` @ `0x80022074` removes entity from all game loops and calls destructor.

## Entity Type Factory Callbacks

All 82 factory callbacks renamed in Ghidra from `EntityTypeXXX_TickCallback` → `EntityTypeXXX_InitCallback`:

| Type | Address | Comment | Heap Size | Pattern |
|------|---------|---------|-----------|---------|
| 001 | 0x8007F730 | | | Render+Update |
| 002 | 0x80080328 | Clayball | | Render+Update |
| 004 | 0x8007EFD0 | Default (types 0,3) | | Render+Update |
| 008 | 0x80081504 | Item | 0x00 | Empty stub |
| 033 | 0x80080AF8 | | 0x110 | Render-only |
| 038 | 0x80080BC8 | | 0x108 | Render-only |
| 040 | 0x80080CFC | | 0x120 | Render+Update |
| 045 | 0x80080F1C | Message | 0x114 | Render+Update |
| 048 | 0x80080E4C | PlatformB | 0x114 | Render+Update |
| 050 | 0x8007FC20 | Boss | | Render+Update |
| 051 | 0x8007FC9C | BossPart | | Render+Update |
| 060 | 0x80080DDC | Portal | 0x114 | Render+Update |
| 061 | 0x80080718 | Sparkle | | Render+Update |
| 071 | 0x80080FEC | Boss (multi-sprite) | 0x148 | Render+Update |
| 080 | 0x80080EBC | Special | 0x2C | Tick-only (AddToZOrderList) |
| 118 | 0x8007F460 | SpecialAmmo | | Render+Update |

*(See `symbol_addrs.txt` lines 195-280 for complete list of all 82 callbacks)*

## Known Entity Types

### Named/Labeled Entities (Ghidra)

| Function | Address | Sprite ID(s) | Purpose | Behavior Known |
|----------|---------|--------------|---------|----------------|
| `InitPlayerEntity` | 0x8001fcf0 | 0x21842018 | Player character (Klaymen) | Partial - init only |
| `InitBossEntity` | 0x80047fb8 | 0x181c3854, 0x8818a018, 0x244655d | Boss enemy | Full - spawns 6 sub-entities, multi-sprite |
| `InitMenuEntity` | 0x80076928 | 0xb8700ca1 | Menu/UI elements | Yes - menu state machine |
| `InitEntity_8c510186` | 0x80027a00 | 0x8c510186 | Unknown (UI related?) | Minimal |
| `InitEntity_168254b5` | 0x80034bb8 | 0x168254b5 | Unknown | Minimal |
| `InitEntity_a89d0ad0` | 0x80052678 | 0xa89d0ad0 | Unknown | Minimal |

### Menu System Entities (from FUN_800281a4)

`FUN_800281a4` at `0x800281a4` is a **menu factory** that creates many UI entities:

| Sprite ID | Count | Description |
|-----------|-------|-------------|
| 0xb8700ca1 | 1 | Menu UI frame |
| 0xe2f188 | ~12 | Menu item text/buttons (reused) |
| 0xa9240484 | 2 | Button elements |
| 0x88a28194 | 3 | Icon elements (in loop) |
| 0x80e85ea0 | 3 | Additional icons (in loop) |
| 0xe8628689 | 1 | Unknown UI element |
| 0x9158a0f6 | 1 | Unknown UI element |
| 0x902c0002 | 1 | Unknown UI element |

### Other Callers of InitEntitySprite

91 total callers of `InitEntitySprite` were found. Major function groups:

| Address Range | Est. Count | Category |
|---------------|------------|----------|
| 0x80027xxx-0x8002axxx | ~24 | Menu/UI entities |
| 0x80030xxx-0x80037xxx | ~5 | Unknown game entities |
| 0x80040xxx-0x80058xxx | ~10 | Game entities (enemies?) |
| 0x8006dxxx-0x80079xxx | ~15 | UI/HUD entities |
| 0x80078xxx-0x80079xxx | ~20 | Font/text rendering |

## Entity Struct Layout (0x44C bytes)

Based on decompiled code analysis and runtime tracing:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | state_high | State machine upper word (0xffff0000) |
| 0x04 | 4 | callback_main | Main update callback (entity[1]) |
| 0x08 | 4 | state2_high | Secondary state |
| 0x0C | 4 | callback2 | Secondary callback (entity[3]) |
| **0x18** | **4** | **tick_callback** | **Per-frame update function - set by init, changed via EntitySetState** |
| 0x24 | 4 | callback3 | Tertiary callback (entity[9]) |
| 0x28 | 4 | callback4 | (entity[10]) |
| 0x2C | 4 | callback5 | (entity[11]) |
| 0x30 | 4 | callback6 | (entity[12]) |
| 0x34 | 4 | sprite_ptr | Pointer to sprite/POLY structure |
| 0x68 | 2 | x_pos | X position (short) |
| 0x6A | 2 | y_pos | Y position (short) |
| 0xF6 | 1 | visibility? | Rendering flag |
| 0xF7 | 1 | load_flag | Sprite load method selector |
| 0x100+ | ... | Extended | Entity-specific data |

**Key insight**: Offset `0x18` is the **actual per-frame tick callback**, NOT the factory callback from the entity type table. This callback is set during initialization and can be changed via `EntitySetState` for state machine transitions.

## Sprite ID Format

Sprite IDs are **32-bit hashes** that identify sprites in the BLB file's sprite lookup table.

Example IDs found in code:
```
0x21842018 - Player (Klaymen)
0x181c3854 - Boss main body
0x8818a018 - Boss sub-parts
0x244655d  - Boss detail
0xb8700ca1 - Menu frame
0xe2f188   - Menu text
0xa9240484 - Button
0x88a28194 - Icon
0x8c510186 - Unknown entity
```

## Entity Lifecycle

```
1. ALLOCATION
   FUN_800143f0(blbHeaderBufferBase, size, 1, 0)
   └── Allocates memory from BLB buffer

2. INITIALIZATION  
   InitEntitySprite(entity, sprite_id, z_order, x, y, flags)
   ├── FUN_8001a0c8() - Zero/init 0x44c struct
   ├── FUN_8007bbc0() - Init GPU/rendering state
   ├── FUN_8001954c() - Init animation state
   ├── FUN_8001c980() - Entity setup
   ├── FUN_8001cdac/d024() - Load sprite from BLB
   └── FUN_8001d080() - Finalize sprite setup

3. CALLBACK ASSIGNMENT
   entity[1] = update_callback;  // Main tick function
   entity[3] = secondary_callback;
   entity[10] = collision_callback?
   entity[12] = destroy_callback?

4. REGISTRATION
   FUN_800213a8(GameState, entity) - Add to entity list
   FUN_80021190(GameState, entity) - Add to update queue?

5. TICK LOOP
   EntityTickLoop (0x80020e1c) - Called each frame
   └── Iterates entities, calls entity[1] callback

6. DESTRUCTION
   (Not yet analyzed)
```

## Entity Spawn System (BLB Layers 8-11)

See `docs/blb-data-format.md` for details on how entity spawn positions are encoded in BLB tilemaps.

**Summary:**
- Layers 8-11 contain entity spawn markers
- Tile index > tileset size indicates entity tile
- Connected regions of entity tiles = one entity instance
- Entity type identified by tile ID set

## Unknown/TODO

### Behavior Not Yet Analyzed
- Player movement/physics
- Enemy AI patterns
- Collectible pickup logic
- Collision detection
- Damage/health system
- Level transition triggers
- Save/load state

### Entities Not Yet Identified
- Clay balls (collectibles)
- 1-UP lives
- Health pickups
- Moving platforms
- Hazards (spikes, pits)
- Checkpoints
- Level exits
- World map nodes

### Key Functions to Analyze
- `EntityTickLoop` (0x80020e1c) - Main entity update loop
- Entity callback functions (LAB_xxxxx addresses)
- Collision detection routines
- Entity-entity interaction

## Cross-Reference: PAL vs JP

| Entity Function | PAL Address | JP Address | Delta |
|-----------------|-------------|------------|-------|
| InitEntitySprite | 0x8001c720 | 0x8001cb24 | +0x404 |
| InitPlayerEntity | 0x8001fcf0 | ~0x800200f4? | +0x404 |
| InitBossEntity | 0x80047fb8 | ~0x800483bc? | +0x404 |
| EntityTickLoop | 0x80020e1c | ~0x80021220? | +0x404 |

(JP addresses are estimates based on +0x404 delta pattern)

## Verification Status

| Item | Status | Method |
|------|--------|--------|
| Entity struct size (0x44C) | ✓ Verified | Ghidra analysis |
| Sprite ID format | ✓ Verified | Binary search + decompilation |
| InitEntitySprite function | ✓ Verified | Full decompilation |
| Entity spawn layers | ⚠️ Partial | Tilemap extraction |
| Entity behavior callbacks | ⚠️ Minimal | Only init code analyzed |
| Collision system | ✗ Unknown | Not yet analyzed |
| Entity type identification | ⚠️ Partial | Sprite IDs known, not all named |
