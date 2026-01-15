# Animation Setter Functions Reference

**Source**: Ghidra SLES_010.90.c decompilation  
**Date**: January 14, 2026  
**Status**: ✅ Complete analysis of 8 animation property setters

These functions set animation properties by storing values to entity fields and setting pending flags in the double-buffer system at entity+0xE0.

---

## Setter Function Pattern

All 8 functions follow this pattern:
```c
void SetAnimationProperty(Entity* entity, value) {
    u16 flags = entity[0xE0];              // Read pending flags
    entity[field_offset] = value;          // Store new value
    entity[0xE0] = flags | SPECIFIC_FLAG;  // Set pending flag
    
    // If no buffering active (flags & 3 == 0), set immediate flag
    if ((flags & 3) == 0) {
        entity[0xE0] |= 1;  // Apply immediately
    }
}
```

The pending flags at +0xE0 tell `ApplyPendingSpriteState` which fields need updating.

---

## Function Reference

### 1. FUN_8001d024 - AllocateSpriteGPUPrimitive

**Address**: 0x8001d024 (line 5791)

**Purpose**: Allocate GPU primitive structure for sprite rendering

```c
void AllocateSpriteGPUPrimitive(Entity* entity, s16 size) {
    void* primitive = AllocateFromHeap(blbHeaderBufferBase, 0x3C, 1, 0);
    primitive = FUN_80015614(primitive, size);  // Initialize primitive
    entity[0x34] = primitive;  // Store at +0x34
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Size parameter (s16)

**Storage**: entity+0x34 = GPU primitive pointer

**Allocation Size**: 0x3C (60 bytes) for GPU drawing structure

**No Pending Flag**: This is immediate allocation, not buffered

---

### 2. FUN_8001d0b0 - SetAnimationSpriteFlags

**Address**: 0x8001d0b0 (line 5820)

**Purpose**: Set sprite-specific flags (render mode, effects)

```c
void SetAnimationSpriteFlags(Entity* entity, u32 flags, u16 additional_flags) {
    entity[0xE0] |= (additional_flags | 0x04);  // Set flag 0x04
    entity[0xBC] = flags;  // Store sprite flags
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Sprite flags (u32)
- `param_3`: Additional pending flags (u16)

**Storage**: entity+0xBC = sprite flags

**Pending Flag**: 0x04

---

### 3. FUN_8001d0c0 - SetAnimationFrameIndex

**Address**: 0x8001d0c0 (line 5830)

**Purpose**: Set current animation frame index

```c
void SetAnimationFrameIndex(Entity* entity, u32 frame_index) {
    u16 flags = entity[0xE0];
    entity[0xC0] = frame_index & 0xFFFF;  // Store frame index
    
    flags &= 0xFDFF;  // Clear bit 9
    entity[0xE0] = flags | 0x08;  // Set flag 0x08
    
    if ((flags & 3) == 0) {
        entity[0xE0] = flags | 0x09;  // Immediate apply
    }
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Frame index (u32, stored as u16)

**Storage**: entity+0xC0 = current frame index

**Pending Flag**: 0x08

**Special Handling**: Clears bit 9 (0xFDFF mask) before setting

---

### 4. FUN_8001d0f0 - SetAnimationFrameCallback

**Address**: 0x8001d0f0 (line 5848)

**Purpose**: Set frame-specific callback function

```c
void SetAnimationFrameCallback(Entity* entity, void* callback) {
    u16 flags = entity[0xE0];
    entity[0xC0] = callback;  // Store callback pointer
    entity[0xE0] = flags | 0x208;  // Set flags 0x200 | 0x08
    
    if ((flags & 3) == 0) {
        entity[0xE0] = flags | 0x209;  // Immediate apply
    }
}
```

**Parameters**:
- `param_1`: Entity pointer  
- `param_2`: Callback function pointer

**Storage**: entity+0xC0 = callback (same offset as frame index!)

**Pending Flag**: 0x208 (0x200 | 0x08)

**Note**: Shares storage with frame index - mutually exclusive

---

### 5. FUN_8001d170 - SetAnimationLoopFrame

**Address**: 0x8001d170 (line 5864)

**Purpose**: Set animation loop target frame

```c
void SetAnimationLoopFrame(Entity* entity, u32 loop_frame) {
    u16 flags = entity[0xE0];
    entity[0xC4] = loop_frame;  // Store loop target
    entity[0xE0] = flags | 0x410;  // Set flags 0x400 | 0x10
    
    // If buffering active (flag 0x04)
    if ((flags & 4) != 0) {
        entity[0xC0] = entity[0xC4];  // Copy to current frame
        entity[0xE0] |= 0x208;  // Set frame callback flag
    }
    
    if ((flags & 3) == 0) {
        entity[0xE0] |= 1;  // Immediate apply
    }
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Loop target frame (u32)

**Storage**: entity+0xC4 = loop frame target

**Pending Flag**: 0x410 (0x400 | 0x10)

**Special Behavior**: If flag 0x04 set, also copies to +0xC0 and sets 0x208

---

### 6. FUN_8001d1c0 - SetAnimationSpriteId

**Address**: 0x8001d1c0 (line 5884)

**Purpose**: Change current sprite (switch to different sprite asset)

```c
void SetAnimationSpriteId(Entity* entity, u32 sprite_id) {
    u16 flags = entity[0xE0];
    entity[200] = sprite_id & 0xFFFF;  // Store sprite ID at offset 200 (0xC8)
    
    flags &= 0xF7FF;  // Clear bit 11
    entity[0xE0] = flags | 0x20;  // Set flag 0x20
    
    if ((flags & 3) == 0) {
        entity[0xE0] = flags | 0x21;  // Immediate apply
    }
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Sprite ID (u32, stored as u16)

**Storage**: entity+200 (0xC8) = sprite ID

**Pending Flag**: 0x20

**Special Handling**: Clears bit 11 (0xF7FF mask) before setting

---

### 7. FUN_8001d1f0 - SetAnimationSpriteCallback

**Address**: 0x8001d1f0 (line 5902)

**Purpose**: Set sprite lookup callback function

```c
void SetAnimationSpriteCallback(Entity* entity, void* callback) {
    u16 flags = entity[0xE0];
    entity[200] = callback;  // Store callback at offset 200
    entity[0xE0] = flags | 0x820;  // Set flags 0x800 | 0x20
    
    if ((flags & 3) == 0) {
        entity[0xE0] = flags | 0x821;  // Immediate apply
    }
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Callback function pointer

**Storage**: entity+200 (0xC8) = callback (same as sprite ID!)

**Pending Flag**: 0x820 (0x800 | 0x20)

**Note**: Shares storage with sprite ID - mutually exclusive

---

### 8. FUN_8001d218 - SetAnimationActive

**Address**: 0x8001d218 (line 5918)

**Purpose**: Enable/disable animation ticking

```c
void SetAnimationActive(Entity* entity, u8 active) {
    u16 flags = entity[0xE0];
    entity[0xF5] = active;  // Store active flag
    entity[0xE0] = flags | 0x100;  // Set flag 0x100
    
    if ((flags & 3) == 0) {
        entity[0xE0] = flags | 0x101;  // Immediate apply
    }
}
```

**Parameters**:
- `param_1`: Entity pointer
- `param_2`: Active flag (u8, 0=pause, non-zero=active)

**Storage**: entity+0xF5 = animation active flag

**Pending Flag**: 0x100

---

## Pending Flag Reference

| Flag | Hex | Function | Field | Purpose |
|------|-----|----------|-------|---------|
| 0x04 | 4 | SetAnimationSpriteFlags | +0xBC | Sprite flags |
| 0x08 | 8 | SetAnimationFrameIndex | +0xC0 | Current frame |
| 0x20 | 32 | SetAnimationSpriteId | +0xC8 | Sprite ID |
| 0x80 | 128 | EntitySetRenderFlags | +0xF4 | Render flags |
| 0x100 | 256 | SetAnimationActive | +0xF5 | Active flag |
| 0x200 | 512 | (with 0x08) | +0xC0 | Frame callback mode |
| 0x400 | 1024 | (with 0x10) | +0xC4 | Loop frame |
| 0x800 | 2048 | (with 0x20) | +0xC8 | Sprite callback mode |

**Combined Flags**:
- 0x208 (0x200 | 0x08): SetAnimationFrameCallback
- 0x410 (0x400 | 0x10): SetAnimationLoopFrame  
- 0x820 (0x800 | 0x20): SetAnimationSpriteCallback

**Immediate Apply Flag**: 0x01 (set when no buffering active)

---

## Storage Location Conflicts

### Offset +0xC0 (Three Uses)
1. Frame index (via SetAnimationFrameIndex, flag 0x08)
2. Frame callback (via SetAnimationFrameCallback, flag 0x208)
3. Copied from +0xC4 (via SetAnimationLoopFrame if flag 0x04 set)

**Resolution**: Flags 0x08 and 0x200 distinguish between frame index and callback

### Offset +0xC8 (offset 200) (Two Uses)
1. Sprite ID (via SetAnimationSpriteId, flag 0x20)
2. Sprite callback (via SetAnimationSpriteCallback, flag 0x820)

**Resolution**: Flags 0x20 and 0x800 distinguish between sprite ID and callback

---

## Usage Examples

### Simple Frame Change
```c
// Set frame 5
SetAnimationFrameIndex(entity, 5);
// Next ApplyPendingSpriteState will load frame 5
```

### Sprite Switch with Callback
```c
// Change to different sprite with custom loader
SetAnimationSpriteCallback(entity, MyCustomSpriteLoader);
// Loader will be called to find sprite data
```

### Loop Animation Setup
```c
// Play frames 0-7, then loop back to frame 3
SetAnimationFrameIndex(entity, 0);  // Start at 0
SetAnimationLoopFrame(entity, 3);   // Loop target
// Animation will play: 0,1,2,3,4,5,6,7,3,4,5,6,7,3...
```

### Pause Animation
```c
SetAnimationActive(entity, 0);  // Pause
// Frame timer stops counting
```

---

## Double-Buffer System Integration

These setters are part of the animation double-buffer system:

```mermaid
graph TD
    Setter[Setter Function<br/>SetAnimationFrameIndex]
    PendingField[Pending Field<br/>entity+0xC0]
    PendingFlags[Pending Flags<br/>entity+0xE0 OR flag]
    ApplyFunction[ApplyPendingSpriteState<br/>@ 0x8001d554]
    CurrentField[Current Field<br/>Applied to animation]
    
    Setter --> PendingField
    Setter --> PendingFlags
    PendingFlags --> ApplyFunction
    PendingField --> ApplyFunction
    ApplyFunction --> CurrentField
```

**Flow**:
1. Setter stores value to pending field
2. Setter ORs flag into entity+0xE0
3. Next frame: ApplyPendingSpriteState reads flags
4. If flag set: Copy pending → current, clear flag
5. Animation system uses current values

---

## Entity Field Reference

### Animation State Fields

| Offset | Size | Field | Set By | Purpose |
|--------|------|-------|--------|---------|
| 0x34 | ptr | gpu_primitive | AllocateSpriteGPUPrimitive | GPU drawing structure |
| 0xBC | u32 | sprite_flags | SetAnimationSpriteFlags | Render mode flags |
| 0xC0 | u32 | frame_or_callback | Multiple | Frame index OR callback |
| 0xC4 | u32 | loop_frame | SetAnimationLoopFrame | Animation loop target |
| 0xC8 | u32 | sprite_or_callback | Multiple | Sprite ID OR callback |
| 0xE0 | u16 | pending_flags | All setters | Double-buffer flags |
| 0xF4 | u8 | render_flags | EntitySetRenderFlags | Visibility, etc. |
| 0xF5 | u8 | anim_active | SetAnimationActive | Pause/resume |

---

## Proposed Function Names (for Ghidra)

| Address | Old Name | New Name | Flag |
|---------|----------|----------|------|
| 0x8001d024 | FUN_8001d024 | AllocateSpriteGPUPrimitive | N/A |
| 0x8001d0b0 | FUN_8001d0b0 | SetAnimationSpriteFlags | 0x04 |
| 0x8001d0c0 | FUN_8001d0c0 | SetAnimationFrameIndex | 0x08 |
| 0x8001d0f0 | FUN_8001d0f0 | SetAnimationFrameCallback | 0x208 |
| 0x8001d170 | FUN_8001d170 | SetAnimationLoopFrame | 0x410 |
| 0x8001d1c0 | FUN_8001d1c0 | SetAnimationSpriteId | 0x20 |
| 0x8001d1f0 | FUN_8001d1f0 | SetAnimationSpriteCallback | 0x820 |
| 0x8001d218 | FUN_8001d218 | SetAnimationActive | 0x100 |

---

## Related Documentation

- [Animation Framework](animation-framework.md) - 5-layer system overview
- [Player Animation](player/player-animation.md) - Player-specific animations
- [Entities](entities.md) - Entity structure reference

---

## Summary

**Total Setters**: 8 functions  
**Pattern**: Store value + OR flag into entity+0xE0  
**Integration**: Part of double-buffer animation system  
**Status**: ✅ Fully documented

These setters provide the API for game code to trigger animation changes that will be applied on the next frame by `ApplyPendingSpriteState`.

