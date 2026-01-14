# Checkpoint System (Ma-Bird)

The checkpoint system allows players to save progress and respawn at specific locations after death.

## Overview

Checkpoints are triggered by collision with Ma-Bird entities. Upon activation:
1. Game saves current state
2. Player is teleported to checkpoint exit
3. On death, player respawns at last activated checkpoint

## Key Functions

### SaveCheckpointState @ 0x8007EAAC

Saves the current game state when a checkpoint is activated.

**Parameters:**
- `param_1`: GameState pointer (0x8009DC40)

**Actions:**
```c
void SaveCheckpointState(int gameState) {
    // Move entity tick list from active (+0x1C) to saved (+0x134)
    gameState[0x134] = gameState[0x1C];
    gameState[0x1C] = 0;
    
    // Set checkpoint active flags
    gameState[0x14A] = 1;  // Checkpoint saved flag
    gameState[0x63] = 1;   // Additional checkpoint flag
    
    // Save current score
    gameState[0x138] = gameState[0x10C];
    
    // Re-add player to render list
    AddToZOrderList(gameState, gameState[0x2C]);
}
```

**Memory Layout:**
| Offset | Field | Purpose |
|--------|-------|---------|
| +0x1C | entity_tick_list | Active entities (cleared on save) |
| +0x2C | player_entity | Player entity pointer |
| +0x10C | current_score | Current game score |
| +0x134 | saved_entity_list | Checkpoint entity snapshot |
| +0x138 | saved_score | Score at checkpoint |
| +0x14A | checkpoint_active | Flag indicating checkpoint saved |
| +0x63 | checkpoint_flag_2 | Secondary checkpoint flag |

### RestoreCheckpointEntities @ 0x8007EAEC

Restores saved state when player dies and respawns.

**Parameters:**
- `param_1`: GameState pointer

**Actions:**
```c
void RestoreCheckpointEntities(int gameState) {
    // Clear checkpoint flags
    gameState[0x14A] = 0;
    gameState[0x63] = 0;
    
    // Restore saved score
    gameState[0x10C] = gameState[0x138];
    
    // Remove player from tick list
    RemoveFromTickList(gameState, gameState[0x2C]);
    
    // Restore saved entity list
    int* current = gameState[0x1C];
    gameState[0x1C] = gameState[0x134];
    gameState[0x134] = 0;
    
    // Free entities that spawned after checkpoint
    while (current != NULL) {
        AddToZOrderList(gameState, current[1]);
        int* next = (int*)current[0];
        FreeFromHeap(blbHeaderBufferBase, current, 8, 0);
        current = next;
    }
}
```

### PlayerState_CheckpointActivated @ 0x8006A214

Player state entered when colliding with Ma-Bird checkpoint.

**Sequence (verified from trace):**

| Frame | Event | Details |
|-------|-------|---------|
| 3810 | State transition | Enter `PlayerState_CheckpointActivated` |
| 3810-3969 | Frozen cutscene | Player locked at (6739, 667) in state 0x8001CB88 |
| 3969 | Level reload | `LevelLoad` event triggered (same level) |
| 3969 | Checkpoint save | `SaveCheckpointState` called |
| 4048 | Teleport | Player position transitions (garbage frame 0xEEEE) |
| 4124 | Exit spawn | Player at checkpoint exit (632, 927) |
| 4216 | Resume gameplay | Return to IdleLook state, falling to ground |

**Function Details:**
```c
void PlayerState_CheckpointActivated(Entity* player) {
    player[0x1B2] = 1;  // Set checkpoint flag
    
    StopCDStreaming();  // Pause audio
    
    // Clear linked checkpoint entity
    if (player[0x5A] != 0) {
        player[0x5A][0x2C] = 1;  // Mark checkpoint entity as used
        player[0x5A] = 0;
    }
    
    // Clear state fields
    player[0x4A] = 0;
    player[0x43] = 0;
    player[0x44] = 0;
    
    // Set only EntityUpdateCallback (clear all other callbacks)
    player[0x00] = 0xFFFF0000;
    player[0x01] = EntityUpdateCallback;
    player[0x02] = 0;
    player[0x03] = 0;
    player[0x41] = 0;
    player[0x42] = 0;
    player[0x07] = 0;
    player[0x08] = 0;
    
    // Setup sprite/animation for checkpoint sequence
    FUN_8001d1c0(player, player[0xDA]);
    FUN_8001d240(player, 0);
    
    player[0x0D][10] = 0;  // Clear linked entity flag
}
```

## Checkpoint Exit Teleport

The teleport from checkpoint activation to exit is **NOT** a death/respawn.

**Evidence from trace:**
- No death state (0x8006A0B8) triggered
- Position change occurs within same gameplay session
- Teleport coordinates are checkpoint-specific:
  - Entry: (6739, 667) - Ma-Bird location
  - Exit: (632, 927) - Predetermined spawn point

**How teleport works:**
1. Player frozen at checkpoint location
2. Level assets reloaded (same level, same stage)
3. Player entity position directly modified
4. Resumes at exit with normal physics

## Related Systems

### Death & Respawn

On player death (`PlayerState_Death` @ 0x8006A0B8):
1. `RestoreCheckpointEntities` called
2. Entity list restored from +0x134
3. Player respawns at last checkpoint exit point
4. Score restored from +0x138

### Entity Fields

**Player Entity:**
| Offset | Field | Purpose |
|--------|-------|---------|
| +0x5A | checkpoint_link | Pointer to Ma-Bird entity |
| +0x1B2 | in_checkpoint | Flag set during checkpoint sequence |

**Checkpoint Entity (Ma-Bird):**
| Offset | Field | Purpose |
|--------|-------|---------|
| +0x2C | activated | Set to 1 when checkpoint used |

## Verification

**Confirmed via:**
- Runtime trace analysis (trace_20260114_214044_unknown_stage0_f0.jsonl)
- Ghidra decompilation of SaveCheckpointState/RestoreCheckpointEntities
- Player state function analysis

**Trace evidence:**
- Frame 3810: Checkpoint activation
- 160 frames of cutscene freeze
- Level reload at frame 3969
- Teleport to (632, 927) at frame 4124
- Normal gameplay resumed at frame 4216
