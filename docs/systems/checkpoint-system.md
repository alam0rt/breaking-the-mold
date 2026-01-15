# Checkpoint System (Ma-Bird)

The checkpoint system allows players to save progress and respawn at specific locations after death. In Skullmonkeys, checkpoints are represented by **Ma-Bird** entities that teleport players to a safe exit point when activated.

## Overview

Checkpoints serve a dual purpose:
1. **Save game state** - Preserve entity list and score
2. **Teleport player** - Move player to checkpoint exit point

Upon activation:
1. Player collides with Ma-Bird checkpoint entity
2. Game freezes in checkpoint cutscene (~160 frames)
3. State saved to GameState backup buffers
4. Level reloads (same level/stage)
5. Player teleports to predetermined exit coordinates
6. On death, player respawns at last checkpoint exit with restored state

## Memory Layout

### GameState Checkpoint Fields

| Offset | Size | Field | Purpose |
|--------|------|-------|---------|
| +0x1C | 4 | entity_tick_list | Active entity linked list (current gameplay) |
| +0x2C | 4 | player_entity | Player entity pointer |
| +0x63 | 1 | freeze_spawning | Checkpoint freeze flag (1 = pause entity spawning) |
| +0x10C | 4 | current_score | Current game score/counter |
| +0x134 | 4 | saved_entity_list | **Checkpoint entity snapshot** (backup of +0x1C) |
| +0x138 | 4 | saved_score | **Score at checkpoint** (backup of +0x10C) |
| +0x14A | 1 | checkpoint_active | **Checkpoint saved flag** (1 = checkpoint active) |

### Entity List Structure

The entity list at `GameState+0x1C` (and its checkpoint backup at `+0x134`) uses a linked list of 8-byte nodes:

```c
struct EntityListNode {
    EntityListNode* next;  // +0x00: Pointer to next node (or NULL)
    Entity* entity;        // +0x04: Pointer to entity structure
};
```

This list tracks all entities spawned since the last checkpoint, allowing selective restoration.

---

## Key Functions

### SaveCheckpointState @ 0x8007EAAC

**Address**: `0x8007EAAC` (line 41609 in decompiled source)  
**Called by**: `GameModeCallback` when `GameState[0x149] && !GameState[0x14A]`

Saves the current game state when a checkpoint is activated (Ma-Bird collision + teleport sequence).

**Function Signature**:
```c
void SaveCheckpointState(GameState* param_1);
```

**Implementation** (verified from decompiled code):
```c
void SaveCheckpointState(int gameState) {
    // Step 1: Backup current entity list pointer
    *(uint32_t*)(gameState + 0x134) = *(uint32_t*)(gameState + 0x1C);
    
    // Step 2: Clear active entity list (entities will be re-added)
    *(uint32_t*)(gameState + 0x1C) = 0;
    
    // Step 3: Set checkpoint active flags
    *(uint8_t*)(gameState + 0x14A) = 1;  // Checkpoint saved flag
    *(uint8_t*)(gameState + 0x63) = 1;   // Freeze entity spawning
    
    // Step 4: Backup current score/counter
    *(uint32_t*)(gameState + 0x138) = *(uint32_t*)(gameState + 0x10C);
    
    // Step 5: Re-add player entity to z-order render list
    AddToZOrderList(gameState, *(uint32_t*)(gameState + 0x2C));
}
```

**Logic Flow**:
1. **Entity List Swap**: Active list (`+0x1C`) moved to checkpoint backup (`+0x134`)
2. **List Clear**: Active list set to NULL (will rebuild as entities spawn post-checkpoint)
3. **Flags Set**: Checkpoint active (`+0x14A = 1`), entity spawning frozen (`+0x63 = 1`)
4. **Score Backup**: Current score saved to `+0x138`
5. **Player Readd**: Player entity added back to render list (remains active)

**Why This Design?**

The game uses a **differential checkpoint system**:
- Entities spawned **before** checkpoint are saved in `+0x134`
- Entities spawned **after** checkpoint accumulate in `+0x1C`
- On death, post-checkpoint entities are destroyed, pre-checkpoint entities restored

This prevents respawning enemies/items that were already collected.

---

### RestoreCheckpointEntities @ 0x8007EAEC

**Address**: `0x8007EAEC` (line 41623 in decompiled source)  
**Called by**: `RespawnAfterDeath` when player dies with active checkpoint

Restores saved state when player dies and respawns at checkpoint.

**Function Signature**:
```c
void RestoreCheckpointEntities(GameState* param_1);
```

**Implementation** (verified from decompiled code):
```c
void RestoreCheckpointEntities(int gameState) {
    undefined4 saved_list;
    int* current_node;
    int next_node;
    
    // Step 1: Clear checkpoint active flags
    *(uint8_t*)(gameState + 0x14A) = 0;  // Checkpoint no longer active
    *(uint8_t*)(gameState + 0x63) = 0;   // Resume entity spawning
    
    // Step 2: Restore saved score
    *(uint32_t*)(gameState + 0x10C) = *(uint32_t*)(gameState + 0x138);
    
    // Step 3: Remove player from tick list temporarily
    RemoveFromTickList(gameState, *(uint32_t*)(gameState + 0x2C));
    
    // Step 4: Get current post-checkpoint entity list
    current_node = *(int**)(gameState + 0x1C);
    
    // Step 5: Restore pre-checkpoint entity list
    saved_list = *(undefined4*)(gameState + 0x134);
    *(undefined4*)(gameState + 0x134) = 0;  // Clear backup
    *(undefined4*)(gameState + 0x1C) = saved_list;  // Restore to active
    
    // Step 6: Destroy all post-checkpoint entities
    while (current_node != NULL) {
        // Re-add entity to z-order list (for proper cleanup)
        AddToZOrderList(gameState, current_node[1]);
        
        // Get next node before freeing current
        next_node = *current_node;
        
        // Free the list node (8 bytes)
        FreeFromHeap(blbHeaderBufferBase, current_node, 8, 0);
        
        // Move to next
        current_node = (int*)next_node;
    }
}
```

**Logic Flow**:
1. **Clear Flags**: Disable checkpoint mode, resume entity spawning
2. **Restore Score**: Revert to checkpoint score (lose progress after checkpoint)
3. **Player Removal**: Temporarily remove player from tick list
4. **List Swap**: Saved entities (`+0x134`) → active list (`+0x1C`)
5. **Cleanup Loop**: Free all post-checkpoint entities (collectibles taken, enemies spawned)
6. **Memory Management**: Each 8-byte list node freed individually

**Entity Cleanup**:
- Entities spawned **after checkpoint** are destroyed (enemies, items collected)
- Entities from **before checkpoint** are restored (map resets to checkpoint state)
- Player respawns at checkpoint exit point with restored state

---

### ClearSaveSlotFlags @ 0x80081E84

**Address**: `0x80081E84` (line 42491 in decompiled source)  
**Purpose**: Clears save slot flags (used for password/save system)

**Implementation**:
```c
void ClearSaveSlotFlags(int param_1) {
    int slot_array = *(int*)(param_1 + 0x164);
    int count = *(short*)(param_1 + 0x168);
    
    // Clear flag at offset 0x3C for each slot (0x40 byte stride)
    for (int i = 0; i < count; i++) {
        *(uint32_t*)(slot_array + 0x3C) = 0;
        slot_array += 0x40;  // Next slot (64 bytes each)
    }
}
```

**Save Slot Structure** (inferred):
- Array stored at `GameState+0x164`
- Count stored at `GameState+0x168`
- Each slot is **0x40 bytes** (64 bytes)
- Flag at offset `+0x3C` within slot

**Note**: This is separate from the checkpoint system. This appears to be for the password/save screen system, not in-level checkpoints.

---

## Player State: Checkpoint Activation

### PlayerState_CheckpointActivated @ 0x8006A214

**Purpose**: Player state entered when colliding with Ma-Bird checkpoint entity.

**Sequence** (verified from trace):

| Frame | Event | Details |
|-------|-------|---------|
| 3810 | State transition | Enter `PlayerState_CheckpointActivated` |
| 3810-3969 | Frozen cutscene | Player locked at (6739, 667) in state 0x8001CB88 |
| 3969 | Level reload | `LevelLoad` event triggered (same level/stage) |
| 3969 | Checkpoint save | `SaveCheckpointState()` called |
| 4048 | Teleport | Player position transitions (garbage frame 0xEEEE) |
| 4124 | Exit spawn | Player at checkpoint exit (632, 927) |
| 4216 | Resume gameplay | Return to IdleLook state, falling to ground |

**Function Pseudocode**:
```c
void PlayerState_CheckpointActivated(Entity* player) {
    player[0x1B2] = 1;  // Set checkpoint flag
    
    StopCDStreaming();  // Pause audio streaming
    
    // Clear linked checkpoint entity reference
    if (player[0x5A] != 0) {
        player[0x5A][0x2C] = 1;  // Mark Ma-Bird as activated
        player[0x5A] = 0;         // Clear link
    }
    
    // Clear velocity and state fields
    player[0x4A] = 0;
    player[0x43] = 0;
    player[0x44] = 0;  // Clear gravity/vertical velocity
    
    // Set only EntityUpdateCallback (clear all other callbacks)
    player[0x00] = 0xFFFF0000;
    player[0x01] = EntityUpdateCallback;
    player[0x02] = 0;  // Clear callback 2
    player[0x03] = 0;  // Clear callback 3
    player[0x41] = 0xFFFF0000;
    player[0x42] = 0;  // Clear callback at +0x108
    player[0x07] = 0xFFFF0000;
    player[0x08] = 0;  // Clear movement callback
    
    // Setup sprite/animation for checkpoint cutscene
    FUN_8001d1c0(player, player[0xDA]);  // Lock current frame
    FUN_8001d240(player, 0);              // Disable animation
    
    player[0x0D][10] = 0;  // Clear linked entity flag
}
```

**Entity Fields Used**:

**Player Entity**:
| Offset | Field | Purpose |
|--------|-------|---------|
| +0x43 | velocity_x_2 | Cleared during checkpoint |
| +0x44 | gravity/velocity_y | Cleared during checkpoint |
| +0x4A | state_field | Cleared during checkpoint |
| +0x5A | checkpoint_link | Pointer to Ma-Bird entity (cleared after use) |
| +0xDA | current_frame | Frame index for animation lock |
| +0x1B2 | in_checkpoint | Flag set to 1 during checkpoint sequence |

**Checkpoint Entity (Ma-Bird)**:
| Offset | Field | Purpose |
|--------|-------|---------|
| +0x2C | activated | Set to 1 when checkpoint used (prevents re-use) |

---

## Checkpoint Teleport Mechanism

The teleport from checkpoint activation to exit is **NOT** a death/respawn sequence.

**Evidence from runtime trace**:
- No death state (`PlayerState_Death` @ 0x8006A0B8) triggered
- Position change occurs within same gameplay session
- Level reloads but maintains same level/stage ID
- No lives decremented

**How Teleport Works**:
1. **Freeze Player**: All movement callbacks cleared, animation locked
2. **Trigger Level Reload**: `LevelLoad` event (frame 3969) - same level, same stage
3. **Save State**: `SaveCheckpointState()` called during reload
4. **Modify Position**: Player entity position directly modified (garbage frame transition)
5. **Resume Physics**: Player exits at predetermined coordinates with normal physics

**Teleport Coordinates** (level-specific):
- **Entry Point**: Ma-Bird entity location (e.g., 6739, 667)
- **Exit Point**: Predetermined spawn coordinates (e.g., 632, 927)
- These are stored in level data (likely in Asset 501 entity variant field)

---

## Comparison: Checkpoint vs Pause System

Both checkpoint and pause systems use similar entity list backup mechanisms, but for different purposes:

| Feature | Checkpoint System | Pause System |
|---------|-------------------|--------------|
| **Backup Location** | `GameState+0x134` | `GameState+0x15C` |
| **Score Backup** | `+0x138` | `+0x154` |
| **Purpose** | Save game progress | Temporary freeze for menu |
| **Freeze Flag** | `+0x63`, `+0x14A` | `+0x63`, `+0x150` |
| **Trigger** | Ma-Bird collision | START button press |
| **Restoration** | On death (permanent until next checkpoint) | On resume (immediate) |
| **Entity Cleanup** | Frees post-checkpoint entities | Filters HUD entities only |
| **Duration** | Until death or next checkpoint | Until menu dismissed |

**Pause System Implementation** (for comparison):

```c
void PauseGameAndShowMenu(int gameState) {
    SaveAndMuteAllVoicePitches();
    PlaySoundEffect(0x65281e40, 0xa0, 1);
    
    // Backup current state
    *(uint32_t*)(gameState + 0x15C) = *(uint32_t*)(gameState + 0x1C);  // Entity list
    *(uint32_t*)(gameState + 0x154) = *(uint32_t*)(gameState + 0x10C); // Score
    *(uint8_t*)(gameState + 0x158) = *(uint8_t*)(gameState + 0x63);    // Freeze flag
    
    // Clear active list and set flags
    *(uint32_t*)(gameState + 0x1C) = 0;
    *(uint8_t*)(gameState + 0x63) = 1;
    *(uint8_t*)(gameState + 0x150) = 1;  // Pause menu active
    *(uint8_t*)(gameState + 0x160) = 0x16;  // Pause countdown (22 frames)
    
    // Add pause menu entity
    AddToZOrderList(gameState, *(uint32_t*)(gameState + 0x14C));
    
    // Filter out HUD entities (type 0x10) from saved list
    // ... (removes UI elements that shouldn't appear during pause)
}
```

---

## Related Systems

### Death & Respawn

On player death (`PlayerState_Death` @ 0x8006A0B8):

1. **Check Lives**: If `g_pPlayerState[0x11] == 0`, game over
2. **Decrement Lives**: `g_pPlayerState[0x11]--`
3. **Call Restore**: `RespawnAfterDeath()` → `RestoreCheckpointEntities()`
4. **Reset Position**: Player spawns at last checkpoint exit point
5. **Restore State**: Entity list and score reverted to checkpoint

**RespawnAfterDeath** (called from `GameModeCallback`):
```c
void RespawnAfterDeath(GameState* state) {
    if (state[0x14A]) {  // If checkpoint active
        RestoreCheckpointEntities(state);
    }
    
    // Reset player position to checkpoint exit
    // Restore player health/powerups
    // Resume gameplay
}
```

### Game Mode Callback Integration

The checkpoint system is integrated into the main game loop:

```c
void GameModeCallback(GameState* state) {
    // ... pause/menu handling ...
    
    // Checkpoint save trigger (frame after Ma-Bird collision)
    if (state[0x149] && !state[0x14A]) {
        SaveCheckpointState(state);
    }
    
    // Checkpoint restore trigger (on respawn)
    if (state[0x14A]) {
        RestoreCheckpointEntities(state);
    }
    
    // Entity spawning disabled during checkpoint
    if (!state[0x150] && !state[0x14A]) {
        SpawnOnScreenEntities(state);
    }
    
    // ... entity processing ...
}
```

**Trigger Flags**:
- `+0x149`: Checkpoint save pending (set by checkpoint collision)
- `+0x14A`: Checkpoint active (blocks entity spawning until restore)
- `+0x150`: Pause menu active (separate from checkpoint)

---

## Memory Management

### Entity List Node Allocation

Entity list nodes are allocated from the BLB header buffer heap:

```c
// Allocation (8 bytes per node)
EntityListNode* node = AllocateFromHeap(blbHeaderBufferBase, 8, 1, 0);
node->next = entity_list_head;
node->entity = new_entity;
entity_list_head = node;

// Free (during checkpoint restore)
FreeFromHeap(blbHeaderBufferBase, node, 8, 0);
```

**Heap Details**:
- **Heap Base**: `blbHeaderBufferBase` (global pointer to BLB buffer)
- **Node Size**: 8 bytes (4-byte next pointer + 4-byte entity pointer)
- **Alignment**: 1-byte aligned (no special alignment requirement)
- **Flags**: 0 (standard allocation)

### Entity Structure (Referenced by List)

The entity pointer in each node points to a full entity structure (typically 0x44C bytes for most entities):

```c
struct Entity {
    // Callbacks and state
    uint32_t state_high;              // +0x00
    void (*callback_main)(Entity*);   // +0x04
    short x_position;                 // +0x08
    short y_position;                 // +0x0A
    // ... (see docs/systems/entities.md for full structure)
};
```

The checkpoint system doesn't modify entity contents, only the list linkage.

---

## Implementation Notes for Reimplementation

### Godot Implementation

To implement a similar checkpoint system in Godot:

```gdscript
class_name CheckpointSystem
extends Node

var checkpoint_active: bool = false
var saved_entities: Array = []
var saved_score: int = 0
var checkpoint_position: Vector2

func save_checkpoint(entities: Array, current_score: int, exit_pos: Vector2):
    # Store entity states (deep copy)
    saved_entities = []
    for entity in entities:
        if not entity.is_player():
            saved_entities.append({
                "type": entity.type,
                "position": entity.position,
                "state": entity.get_state_dict()
            })
    
    saved_score = current_score
    checkpoint_position = exit_pos
    checkpoint_active = true
    
    # Clear post-checkpoint entities
    for entity in entities:
        if entity.spawned_after_checkpoint:
            entity.queue_free()

func restore_checkpoint(player: Player, entities: Array) -> void:
    if not checkpoint_active:
        return
    
    # Restore score
    GameState.score = saved_score
    
    # Destroy post-checkpoint entities
    for entity in entities:
        if not entity.is_player():
            entity.queue_free()
    
    # Respawn saved entities
    for entity_data in saved_entities:
        spawn_entity(entity_data)
    
    # Teleport player
    player.position = checkpoint_position
    player.reset_physics()

func spawn_entity(data: Dictionary) -> void:
    var entity = EntityFactory.create(data.type)
    entity.position = data.position
    entity.restore_state(data.state)
    get_tree().current_scene.add_child(entity)
```

### C Implementation Considerations

For a C-based reimplementation (matching original architecture):

1. **Use Linked Lists**: Maintain entity list as linked list for O(1) save/restore
2. **Differential Tracking**: Track "spawned after checkpoint" flag per entity
3. **Heap Management**: Use a custom heap for entity list nodes (8 bytes each)
4. **Player Exclusion**: Never include player in checkpoint entity list
5. **Cleanup Order**: Add entities to z-order list before freeing (for proper render cleanup)

---

## Related Documentation

- [Entity System](entities.md) - Entity lifecycle and callbacks
- [Game Loop](game-loop.md) - Main game mode callback integration
- [Player System](player-system.md) - Player states and death handling
- [Level Loading](level-loading.md) - Level reload during checkpoint teleport

---

## Function Reference

| Function | Address | Purpose |
|----------|---------|---------|
| `SaveCheckpointState` | 0x8007EAAC | Save entity list and score to checkpoint buffers |
| `RestoreCheckpointEntities` | 0x8007EAEC | Restore checkpoint state on respawn |
| `ClearSaveSlotFlags` | 0x80081E84 | Clear password/save slot flags (separate system) |
| `PlayerState_CheckpointActivated` | 0x8006A214 | Player state during Ma-Bird collision |
| `RespawnAfterDeath` | 0x80070736 | Respawn handler (calls RestoreCheckpointEntities) |
| `PauseGameAndShowMenu` | 0x8007ECA8 | Pause system (uses similar backup mechanism) |
| `AddToZOrderList` | 0x80020F68 | Add entity to render z-order list |
| `RemoveFromTickList` | 0x80021190 | Remove entity from tick list |
| `FreeFromHeap` | 0x800145A4 | Free memory from BLB buffer heap |
| `AllocateFromHeap` | 0x800143F0 | Allocate memory from BLB buffer heap |

---

**Last Updated**: 2026-01-14  
**Decompiled Source Reference**: SLES_010.90.c lines 41609-42507  
**Verification Status**: ✅ Verified against runtime traces and decompiled code
