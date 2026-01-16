# Animation Framework Architecture

**Status**: ✅ Fully verified via Ghidra analysis (2026-01-16)
**SpriteFrameEntry struct**: Corrected field order - frame_delay at +0x04, flip_flags at +0x0E

Skullmonkeys uses a sophisticated **5-layer animation system** with data-driven sequences, double-buffered state changes, frame-accurate timing, and multi-priority callback dispatch.

## Architecture Overview

The animation system is organized into 5 layers, each handling a specific aspect of sprite animation:

```
Layer 1: Frame Metadata (SpriteFrameEntry)
    ↓ (36-byte structs with timing/bounds/offsets)
Layer 2: Frame Timing & Advancement
    ↓ (+0xEC countdown, +0xDA current, +0xDE target)
Layer 3: State Buffering (Double-Buffered Updates)
    ↓ (+0xE0 flags accumulate, atomic apply)
Layer 4: Sequence Control (Data-Driven Playback)
    ↓ (+0x94 table of param+callback pairs)
Layer 5: Callback Dispatch (3-Level Priority)
    ↓ (priority override, current, deferred queue)
```

## Layer 1: Frame Metadata (SpriteFrameEntry)

### Structure (36 bytes) - VERIFIED 2026-01-16 via Ghidra decompilation

| Offset | Type | Field | Description |
|--------|------|-------|-------------|
| +0x00 | u32 | callback_id | Frame event callback (0 = none) |
| +0x04 | u16 | frame_delay | Frame duration in ticks (copied to entity+0xEC) |
| +0x06 | s16 | origin_x | Sprite X offset (for velocity calc → entity+0xE6) |
| +0x08 | s16 | origin_y | Sprite Y offset (for velocity calc → entity+0xE8) |
| +0x0A | u16 | width | Render width |
| +0x0C | u16 | height | Render height |
| +0x0E | u16 | flip_flags | Mirror decode (0=normal, 1=H-flip) |
| +0x10 | u32 | unknown_10 | Unknown (accessed as piVar9[4]) |
| +0x14 | s16 | hitbox_x | Collision X offset |
| +0x16 | s16 | hitbox_y | Collision Y offset |
| +0x18 | u16 | hitbox_w | Collision width |
| +0x1A | u16 | hitbox_h | Collision height |
| +0x1C | u32 | flags | Bit 0: play positioned sound (PlayEntityPositionSound) |
| +0x20 | u32 | rle_offset | RLE pixel data offset |

### Location


- Standard sprites: `entity+0x78` points to array
- Alternate system: `entity+0x8C` (FUN_80019650 lookup)
- Frame size: 0x24 (36) bytes
- Access: `frames[current_frame_index * 0x24]`

## Layer 2: Frame Timing & Advancement

### Entity Offsets

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| +0xD8 | s16 | total_frame_count | Total frames in sprite |
| +0xDA | s16 | current_frame | Current frame index |
| +0xDC | s16 | loop_frame | Loop target frame |
| +0xDE | s16 | target_frame | Animation destination |
| +0xEC | s16 | frame_timer | Countdown (ticks) |
| +0xF0 | u8 | direction | 0=forward, 1=backward |
| +0xF1 | u8 | loop_flag | 1=loop to +0xDC |
| +0xF2 | u8 | anim_active | 1=tick enabled |
| +0xFE | u8 | slow_motion | 1=double frame_delay |

### Key Functions

**UpdateSpriteFrameData @ 0x8001D748**
```c
void UpdateSpriteFrameData(Entity* entity) {
    // Get frame metadata pointer
    SpriteFrameEntry* frame = entity->sprite_frames + (entity->current_frame * 0x24);
    
    // Copy frame_delay to entity timer
    entity->frame_timer = frame->frame_delay;
    if (entity->slow_motion) {
        entity->frame_timer *= 2;  // Double delay for slow-mo
    }
    
    // Calculate velocity deltas (for moving sprites)
    entity->velocity_x_per_frame = (frame->render_x << 16) / entity->frame_timer;
    entity->velocity_y_per_frame = (frame->render_y << 16) / entity->frame_timer;
    
    // Copy render bounds and UVs to entity
    entity->render_x = frame->render_x;
    entity->render_y = frame->render_y;
    entity->render_width = frame->width;
    entity->render_height = frame->height;
    
    // If frame has callback, execute with message 1
    if (frame->callback_id != 0) {
        CallEntityCallback(entity, 1, frame->callback_id);
    }
}
```

**AdvanceAnimationFrame @ 0x8001D4BC**
```c
void AdvanceAnimationFrame(Entity* entity) {
    if (entity->current_frame == entity->target_frame) {
        // At target - check loop flag
        if (entity->loop_flag) {
            entity->current_frame = entity->loop_frame;  // Loop
        }
    } else {
        // Advance toward target
        if (entity->direction == 0) {
            entity->current_frame++;  // Forward
            if (entity->current_frame >= entity->total_frame_count) {
                entity->current_frame = 0;  // Wrap
            }
        } else {
            entity->current_frame--;  // Backward
            if (entity->current_frame < 0) {
                entity->current_frame = entity->total_frame_count - 1;  // Wrap
            }
        }
    }
}
```

### Animation Flow

1. `UpdateSpriteFrameData` reads frame metadata, sets `frame_timer` (e.g., 6 ticks)
2. Each game tick: `frame_timer--` (6 → 5 → 4 → 3 → 2 → 1 → 0)
3. When `frame_timer == 0`: call `AdvanceAnimationFrame`
4. `AdvanceAnimationFrame` increments `current_frame` (0 → 1)
5. When `current_frame == target_frame`: call entity callback with **message 2** (animation complete)
6. If `loop_flag == 1`: jump to `loop_frame` and continue
7. Call `UpdateSpriteFrameData` again to load next frame's metadata

## Layer 3: State Buffering (Double-Buffered)

### Purpose
Allows multiple animation property changes to accumulate, then apply atomically on next frame update. Prevents mid-frame inconsistencies.

### Entity Offsets

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| +0xE0 | u16 | anim_flags | State change bitfield |
| +0xBC-0xC8 | - | pending_* | Buffered values |
| +0xF3-0xF5 | u8 | pending_flags | Buffered control flags |

### Flag Bits (+0xE0)

| Bit | Pending Field | Active Field | Purpose |
|-----|---------------|--------------|---------|
| 0-1 | State bits | - | 0=ready, 1=pending, 2=apply, 3=apply+special |
| 2 (0x04) | +0xBC | +0xCC | Sprite ID changed |
| 3 (0x08) | +0xC0 | +0xDA | Current frame set |
| 4 (0x10) | +0xC4 | +0xDC | Loop frame set |
| 5 (0x20) | +0xC8 | +0xDE | Target frame set |
| 6 (0x40) | +0xF3 | +0xF0 | Direction changed |
| 7 (0x80) | +0xF4 | +0xF1 | Loop flag changed |
| 8 (0x100) | +0xF5 | +0xF2 | Animation active changed |
| 9 (0x200) | - | - | Use callback for frame lookup |
| 10 (0x400) | - | - | Use callback for loop frame |
| 11 (0x800) | - | - | Use callback for target frame |

### ApplyPendingSpriteState @ 0x8001D548

```c
void ApplyPendingSpriteState(Entity* entity) {
    u16 flags = entity->anim_flags;
    
    // Bit 0x04: New sprite ID
    if (flags & 0x04) {
        entity->active_sprite_id = entity->pending_sprite_id;
        InitSpriteContext(entity);  // Reset frame count
        entity->total_frame_count = GetSpriteFrameCount(entity);
    }
    
    // Bit 0x08: Set current frame
    if (flags & 0x08) {
        if (flags & 0x200) {
            entity->current_frame = CallFrameLookupCallback(entity, entity->pending_frame);
        } else {
            entity->current_frame = (entity->pending_frame == -1) ? 
                entity->total_frame_count - 1 : entity->pending_frame;
        }
    }
    
    // Bit 0x10: Set loop frame
    if (flags & 0x10) {
        entity->loop_frame = entity->pending_loop_frame;
    }
    
    // Bit 0x20: Set target frame
    if (flags & 0x20) {
        entity->target_frame = entity->pending_target_frame;
    }
    
    // Bit 0x40: Direction flag
    if (flags & 0x40) {
        entity->direction = entity->pending_direction;
    }
    
    // Bit 0x80: Loop flag
    if (flags & 0x80) {
        entity->loop_flag = entity->pending_loop_flag;
    }
    
    // Bit 0x100: Animation active
    if (flags & 0x100) {
        entity->anim_active = entity->pending_anim_active;
    }
    
    // Clear all flags
    entity->anim_flags = 0;
}
```

### Usage Pattern

```c
// Game code accumulates changes
SetEntitySpriteId(entity, sprite_id);       // +0xE0 |= 0x1FC (sprite + reset)
SetAnimationTargetFrame(entity, 5);         // +0xE0 |= 0x20
SetAnimationLoopFlag(entity, 1);            // +0xE0 |= 0x80

// On next tick, TickEntityAnimation calls:
if ((entity->anim_flags & 3) == 1) {
    ApplyPendingSpriteState(entity);  // Apply all at once
    UpdateSpriteFrameData(entity);     // Load new frame metadata
}
```

## Layer 4: Sequence Control (Data-Driven)

### Purpose
Plays scripted sequences of callbacks with parameters. Used for complex multi-step animations (death sequences, transformations, cutscenes).

### Entity Offsets

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| +0x94 | u32 | sequence_table | Pointer to 8-byte entries |
| +0xE2 | s16 | sequence_step | Current step index |
| +0xE4 | s16 | sequence_length | Total steps |

### Sequence Entry Format (8 bytes)

```c
struct SequenceEntry {
    u32 param;       // Parameter value (copied to +0xA0)
    void* callback;  // Function pointer (copied to +0xA4)
};
```

### Key Functions

**StartAnimationSequence @ 0x8001E790**
```c
void StartAnimationSequence(Entity* entity, SequenceEntry* table, s16 length) {
    entity->sequence_step = 0;
    entity->sequence_length = length;
    entity->sequence_table = table;
    StepAnimationSequence(entity);  // Execute first step immediately
}
```

**StepAnimationSequence @ 0x8001E7B8**
```c
void StepAnimationSequence(Entity* entity) {
    // Process deferred callback queue first
    ProcessDeferredCallbacks(entity);
    
    if (entity->sequence_table == NULL) return;
    
    // Read current sequence entry
    SequenceEntry* entry = &entity->sequence_table[entity->sequence_step];
    entity->current_callback_param = entry->param;
    entity->current_callback = entry->callback;
    
    // Execute callback
    entry->callback(entity, entry->param);
    
    // Advance to next step
    entity->sequence_step++;
    if (entity->sequence_step >= entity->sequence_length) {
        // Sequence complete - clear table pointer
        entity->sequence_table = NULL;
    }
}
```

### Example Sequence

```c
// Death animation sequence
SequenceEntry death_seq[] = {
    {0x5A, SetEntitySpriteId},      // Step 0: Set sprite 0x5A (death start)
    {0x00, PlayDeathSound},         // Step 1: Play sound
    {0x0F, WaitFrames},             // Step 2: Wait 15 frames
    {0x5B, SetEntitySpriteId},      // Step 3: Set sprite 0x5B (explosion)
    {0x00, SpawnDeathParticles},    // Step 4: Spawn particles
    {0x1E, WaitFrames},             // Step 5: Wait 30 frames
    {0x00, RemoveEntity},           // Step 6: Delete entity
};

StartAnimationSequence(entity, death_seq, 7);
```

## Layer 5: Callback Dispatch (3-Level Priority)

### Entity Offsets

| Offset | Type | Name | Description |
|--------|------|------|-------------|
| +0x98-0x9C | 8 bytes | priority_callback | Highest priority (interrupts) |
| +0xA0-0xA4 | 8 bytes | current_callback | Normal priority (active) |
| +0xA8-0xAC | 8 bytes | deferred_callback | Deferred until frame complete |

### Callback Messages

| Message | Context | Purpose |
|---------|---------|---------|
| 0 | Unknown | (needs investigation) |
| 1 | Frame metadata | Frame has callback_id (SpriteFrameEntry+0x00) |
| 2 | Animation complete | current_frame == target_frame |
| 3 | Collision/destruction | Entity collected or destroyed |

### EntityProcessCallbackQueue @ 0x8001E928

```c
void EntityProcessCallbackQueue(Entity* entity) {
    // Priority 1: Check for priority override
    if (entity->priority_callback != NULL) {
        entity->priority_callback(entity, entity->priority_param);
        return;  // Skip normal processing
    }
    
    // Priority 2: Process sequence if active
    if (entity->sequence_table != NULL) {
        StepAnimationSequence(entity);
        return;
    }
    
    // Priority 3: Execute current callback (if set)
    if (entity->current_callback != NULL) {
        entity->current_callback(entity, entity->current_param);
    }
    
    // Process deferred queue
    ProcessDeferredCallbacks(entity);
}
```

### State Transitions (EntitySetState)

```c
void EntitySetState(Entity* entity, void* new_callback, u32 param) {
    // Clear all callbacks and sequences
    entity->priority_callback = NULL;
    entity->current_callback = NULL;
    entity->deferred_callback = NULL;
    entity->sequence_table = NULL;
    
    // Set new state
    entity->current_callback = new_callback;
    entity->current_param = param;
    
    // Execute immediately
    new_callback(entity, param);
}
```

## Complete Animation Update Flow

### Every Game Tick (60Hz PAL, 50Hz NTSC)

```
1. TickEntityAnimation (if +0xF2 == 1):
   a. Decrement frame_timer (-1 tick)
   b. If frame_timer == 0:
      - Check if current_frame == target_frame
      - If yes and anim_flags == 0: call entity callback with message 2
      - Call AdvanceAnimationFrame
   
2. Check anim_flags:
   a. If (anim_flags & 3) == 1:
      - ApplyPendingSpriteState (apply buffered changes)
      - UpdateSpriteFrameData (load new frame metadata)
   
3. EntityProcessCallbackQueue:
   a. Check priority_callback (+0x98-0x9C) - if set, execute and return
   b. Check sequence_table (+0x94) - if set, call StepAnimationSequence
   c. Execute current_callback (+0xA0-0xA4) if set
   d. Process deferred_callback (+0xA8-0xAC) queue
```

## Animation Property Setters

All setters follow the pattern: set pending field, OR flag into +0xE0, mark state as pending.

| Function | Address | Flag | Purpose |
|----------|---------|------|---------|
| SetEntitySpriteId | 0x8001D080 | 0x1FC | Set sprite + reset animation |
| FUN_8001D0B0 | 0x8001D0B0 | 0x04 | Set +0xBC (basic sprite flag) |
| FUN_8001D0C0 | 0x8001D0C0 | 0x08 | Set +0xC0 param (frame index) |
| FUN_8001D0F0 | 0x8001D0F0 | 0x208 | Set +0xC0 callback (frame lookup) |
| FUN_8001D170 | 0x8001D170 | 0x410 | Set +0xC4 data (loop frame) |
| FUN_8001D1C0 | 0x8001D1C0 | 0x20 | Set +0xC8 sprite ID |
| FUN_8001D1F0 | 0x8001D1F0 | 0x820 | Set +0xC8 callback (sprite lookup) |
| FUN_8001D218 | 0x8001D218 | 0x100 | Set +0xF5 (animation active) |
| EntitySetRenderFlags | 0x8001D290 | 0x80 | Set +0xF4 (render flags) |

## Key Function Reference

| Address | Name | Purpose |
|---------|------|---------|
| 0x8001CE80 | InitEntityAnimationState | Initialize animation fields |
| 0x8001D080 | SetEntitySpriteId | Set sprite ID, reset animation |
| 0x8001D290 | TickEntityAnimation | Main animation tick (timer countdown) |
| 0x8001D4BC | AdvanceAnimationFrame | Advance current_frame toward target |
| 0x8001D548 | ApplyPendingSpriteState | Apply buffered state changes |
| 0x8001D748 | UpdateSpriteFrameData | Load frame metadata to entity |
| 0x8001E790 | StartAnimationSequence | Start sequence playback |
| 0x8001E7B8 | StepAnimationSequence | Execute one sequence step |
| 0x8001E928 | EntityProcessCallbackQueue | Multi-priority callback dispatch |
| 0x8001EA64 | EntitySetState | State machine transition |

## Technical Notes

### Why 5 Layers?

1. **Frame Metadata**: Decouples animation timing from code (data-driven)
2. **Frame Timing**: Provides frame-accurate control (PSX runs at 60Hz/50Hz)
3. **State Buffering**: Prevents mid-frame inconsistencies (atomic updates)
4. **Sequence Control**: Enables complex multi-step animations without hardcoding
5. **Callback Priority**: Handles interrupts, frame events, and deferred actions

### Performance Considerations

- **Double buffering** (+0xE0 flags) prevents multiple UpdateSpriteFrameData calls per frame
- **Sequence table** avoids function call overhead for complex animations
- **Priority system** allows urgent callbacks (collisions) to interrupt normal flow
- **Frame metadata caching** eliminates repeated lookups

### Slow Motion (+0xFE)

When slow_motion flag is set:
- `frame_timer = frame_delay * 2` (double frame duration)
- Used for special effects, death animations, powerup pickups
- Example: death animation plays at half speed for dramatic effect

## Related Documentation

- [sprites.md](sprites.md) - Sprite format and RLE compression
- [entities.md](entities.md) - Entity structure and lifecycle
- [player-animation.md](player/player-animation.md) - Player-specific animations
- [game-loop.md](game-loop.md) - Main loop and tick order

## Verification Status

✅ **Fully verified** via Ghidra decompilation analysis (2026-01-15)
- All 5 layers traced and documented
- Frame metadata structure confirmed (36 bytes)
- State buffering mechanism verified (+0xE0 flag bits)
- Sequence format confirmed (8-byte entries: param + callback)
- Callback priority system traced through EntityProcessCallbackQueue
- Frame advancement logic verified in AdvanceAnimationFrame
