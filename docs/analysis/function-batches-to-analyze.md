# Function Batches Ready for Analysis

**Generated**: 2026-01-15  
**Source**: Regenerated Ghidra decompilation (SLES_010.90.c)

This document identifies batches of related functions that can be systematically analyzed and renamed.

---

## Batch 1: Bounding Box Collision Helpers ✅ HIGH PRIORITY

**Category**: Collision Detection  
**Estimated Time**: 30 minutes  
**Status**: Ready to rename

### Functions

| Address | Current Name | Proposed Name | Lines | Purpose |
|---------|--------------|---------------|-------|---------|
| 0x8001b360 | FUN_8001b360 | CheckPointInBox | 18 | Point-in-box test (2D) |
| 0x8001b3f0 | FUN_8001b3f0 | CheckBoxOverlap | 28 | Box-box overlap test |

### Analysis

**FUN_8001b360** @ line 4966:
```c
bool FUN_8001b360(int param_1, short param_2, short param_3)
{
  // Check if point (param_2, param_3) is inside entity bounding box
  // Entity bbox: [+0x48, +0x4a] to [+0x4c, +0x4e]
  
  return (entity[0x48] <= x && x <= entity[0x4c]) &&
         (entity[0x4a] <= y && y <= entity[0x4e]);
}
```

**FUN_8001b3f0** @ line 4982:
```c
bool FUN_8001b3f0(int param_1, undefined4 param_2, undefined4 param_3)
{
  // Box-box overlap test
  // param_2 = box2_x1x2 (packed s16s)
  // param_3 = box2_y1y2 (packed s16s)
  
  short box2_x1 = (short)param_2;
  short box2_x2 = (short)(param_2 >> 16);
  short box2_y1 = (short)param_3;
  short box2_y2 = (short)(param_3 >> 16);
  
  return (entity[0x48] <= box2_x1 && box2_x2 <= entity[0x4c]) &&
         (entity[0x4a] <= box2_y1 && box2_y2 <= entity[0x4e]);
}
```

**Already named**:
- 0x8001b594 = `CollisionCheckWrapper` (line 5032) - Documented wrapper

**Called by**: CheckEntityCollision @ 0x800226f8

**Action**: Rename and document with entity bbox field references (+0x48, +0x4a, +0x4c, +0x4e)

---

## Batch 2: Audio/SPU System ✅ HIGH PRIORITY

**Category**: Sound System  
**Estimated Time**: 1 hour  
**Status**: Ready to analyze

### Functions

| Address | Current Name | Proposed Name | Lines | Purpose |
|---------|--------------|---------------|-------|---------|
| 0x8007c388 | PlaySoundEffect | ✅ Already named | 140 | Play sound with panning |
| 0x8007c7b8 | FUN_8007c7b8 | StopSoundEffect | 8 | Stop SPU voice |
| 0x8007c7e0 | StopAllSPUVoices | ✅ Already named | 6 | Stop all 24 voices |
| 0x8007c818 | FUN_8007c818 | CalculateStereoVolume | 80 | Pan position → L/R volume |
| 0x8007ca28 | FUN_8007ca28 | SetVoicePanning | 12 | Update voice pan in realtime |
| 0x8007ca60 | StartCDAudioForLevel | ✅ Already named | 15 | Start CD music for level |

### Analysis

**PlaySoundEffect** @ 0x8007c388 (line 40345):
```c
uint PlaySoundEffect(uint sound_id, short pan_pos, char force_flag)
{
  // sound_id: 32-bit hash (e.g., 0x248e52 = jump sound)
  // pan_pos: -160 to +160 (left to right)
  // force_flag: 0=respect mute, 1=force play
  
  // Features:
  // - Sound remapping table at 0x8009d0fc (mode-dependent)
  // - Random playback probability (flags 0x100, 0x200, 0x400)
  // - Finds free SPU voice (0-23)
  // - Returns voice index or 0xFFFFFFFF if failed
}
```

**FUN_8007c7b8** @ line 40475:
```c
void FUN_8007c7b8(uint voice_index)
{
  // Stop specific SPU voice
  if (voice_index != 0xFFFFFFFF) {
    SpuSetKey(0, 1 << (voice_index & 0x1F));
  }
}
```

**FUN_8007c818** @ line 40496:
```c
void FUN_8007c818(ushort *out_volumes, ushort base_volume, short pan_pos)
{
  // Convert pan position to stereo L/R volumes
  // pan_pos: -160 to +160
  //   -160 = full left  (L=100%, R=0%)
  //      0 = center     (L=50%, R=50%)
  //   +160 = full right (L=0%, R=100%)
  
  // Applies master volume multiplier (null_04h_800a607f)
  // out_volumes[0] = left volume
  // out_volumes[1] = right volume
}
```

**FUN_8007ca28** @ line 40575:
```c
void FUN_8007ca28(uint voice_index, short pan_pos)
{
  // Update voice panning in realtime
  // Recalculates L/R volumes and applies to active voice
  
  if (voice_index < 24) {
    ushort volumes[2];
    FUN_8007c818(volumes, base_volume_from_table, pan_pos);
    SpuSetVoiceVolume(voice_index, volumes[0], volumes[1]);
  }
}
```

**Key Globals**:
- `g_SoundTable` @ 0x8009cc64: Array of sound entries (12 bytes each)
- `g_SoundTableCount` @ 0x800a6078: Number of loaded sounds
- `null_00h_800a6088`: Next voice to allocate (round-robin)
- `null_00h_800a6087`: Mute flag (if set, sounds don't play)
- `null_00h_800a6082`: Sound remapping mode

**Sound Entry Structure** (12 bytes):
```c
struct SoundEntry {
  u32 sound_id;          // +0x00: Hash (e.g., 0x248e52)
  u32 spu_address;       // +0x04: SPU RAM address
  u16 base_volume;       // +0x08: Default volume (0-0x3FFF)
  u16 flags;             // +0x0A: Playback flags
};
```

**Flags** (from line 40380-40440):
- `0x001`: Use 0x800 ADSR instead of 0x400
- `0x010`: Random pitch ±0x17F (subtle variation)
- `0x020`: Random pitch ±0x2FF (moderate variation)
- `0x040`: Random pitch ±0x5FF (extreme variation)
- `0x100`: 25% playback probability
- `0x200`: 50% playback probability
- `0x400`: 75% playback probability

**Action**: 
1. Rename 3 audio helper functions
2. Document sound table format
3. Extract common sound IDs from PlaySoundEffect calls
4. Create sound-system.md reference

---

## Batch 3: Level Loading System ⚠️ MEDIUM PRIORITY

**Category**: Asset Management  
**Estimated Time**: 2 hours  
**Status**: Partially named, needs deep analysis

### Functions

| Address | Current Name | Proposed Name | Lines | Purpose |
|---------|--------------|---------------|-------|---------|
| 0x80020848 | LoadBLBHeader | ✅ Already named | 30 | Load BLB header |
| 0x8007a218 | InitLevelDataContext | ✅ Already named | 40 | Initialize level context |
| 0x8007b074 | LoadAssetContainer | ✅ Already named | 120 | Load asset by type |
| 0x8007cd34 | InitializeAndLoadLevel | ✅ Already named | 200 | Main level loader |
| 0x8007f270 | LoadLevelSpriteAssets | ✅ Already named | 80 | Load sprites/sequences |

### Status

**All major functions already named!** This batch is **90% complete**.

**Remaining work**:
- Document LoadAssetContainer asset type → context index mapping (already done in tile-collision-complete.md)
- Extract helper functions called by these loaders
- Document error handling patterns

**Action**: Low priority, focus on other batches first.

---

## Batch 4: Player Physics Constants ✅ HIGH PRIORITY

**Category**: Physics Extraction  
**Estimated Time**: 1.5 hours  
**Status**: Ready to extract

### Functions to Grep

Search for velocity/acceleration assignments in player state callbacks:

| Callback | Address | State | Physics Constants |
|----------|---------|-------|-------------------|
| PlayerCallback_8005bad0 | 0x8005bad0 | ? | Walk speed |
| PlayerCallback_HandleMovementAndCollision | ? | Moving | Acceleration, friction |
| PlayerState_Jump | ? | Jump | Jump velocity |
| PlayerState_Falling | ? | Fall | Gravity, terminal velocity |

### Strategy

1. **Grep for velocity assignments**:
   ```bash
   grep -n "param_1.*0x16[0-9]" SLES_010.90.c | head -50
   ```
   
2. **Look for constants**:
   - `*(short *)(param_1 + 0x160)` = horizontal velocity
   - `*(short *)(param_1 + 0x162)` = vertical velocity
   - Look for patterns like `= 0x200`, `+= 0x20`, etc.

3. **Cross-reference with trace data**:
   - Compare extracted constants with game_watcher velocity logs
   - Verify jump height matches recorded traces

**Action**: Systematic grep → extract constants → verify with traces → document

---

## Batch 5: Damage/Health System ⚠️ MEDIUM PRIORITY

**Category**: Combat Mechanics  
**Estimated Time**: 1 hour  
**Status**: Ready to search

### Functions to Find

Search for common patterns:

```bash
# Damage dealing
grep -n "TakeDamage\|DealDamage\|ApplyDamage" SLES_010.90.c

# Health management
grep -n "health.*param_1.*0x" SLES_010.90.c | head -30

# Death sequences
grep -n "Death.*State\|EntitySetState.*death" SLES_010.90.c
```

**Expected functions**:
- DealDamageToEntity
- TakeDamageFromEntity
- ApplyKnockback
- CheckEntityHealth
- TriggerDeathSequence

**Action**: Grep → identify functions → rename → document common patterns

---

## Batch 6: Input Processing ✅ HIGH PRIORITY

**Category**: Controller Input  
**Estimated Time**: 45 minutes  
**Status**: Ready to search

### Functions to Find

```bash
# Button input reading
grep -n "PadData\|ReadController\|GetInput" SLES_010.90.c

# Input state checks
grep -n "0x0010\|0x0020\|0x0040\|0x0080" SLES_010.90.c | grep param_1
```

PSY-Q button masks:
- `0x0010`: PAD_UP
- `0x0020`: PAD_RIGHT
- `0x0040`: PAD_DOWN
- `0x0080`: PAD_LEFT
- `0x1000`: PAD_TRIANGLE
- `0x2000`: PAD_CIRCLE
- `0x4000`: PAD_CROSS
- `0x8000`: PAD_SQUARE

**Expected functions**:
- ReadControllerInput
- ProcessPlayerInput
- CheckButtonPressed
- CheckButtonHeld
- GetAnalogStick (if supported)

**Action**: Search → identify → rename → document input handling flow

---

## Batch 7: Memory Pool Management ⚠️ LOW PRIORITY

**Category**: Memory Allocation  
**Estimated Time**: 2 hours  
**Status**: Complex, requires deep analysis

### Functions to Find

```bash
grep -n "Allocate\|Malloc\|Pool\|Free" SLES_010.90.c
```

**Expected patterns**:
- Entity pool allocation (24-byte structures)
- Sprite context allocation (20-byte contexts)
- Temporary buffers for level loading
- Fixed-size pools vs dynamic allocation

**Action**: Low priority, defer until entity system fully documented

---

## Recommended Order of Analysis

### Today (2-3 hours)

1. **Batch 1: Bounding Box Helpers** (30 min)
   - Rename CheckPointInBox, CheckBoxOverlap
   - Document entity bbox fields
   - Update collision.md

2. **Batch 2: Audio System** (1 hour)
   - Rename 3 audio functions
   - Extract sound ID table from PlaySoundEffect calls
   - Create sound-system.md reference

3. **Batch 6: Input Processing** (45 min)
   - Find and rename input functions
   - Document button masks
   - Create input-system.md

### This Week (5-6 hours)

4. **Batch 4: Player Physics** (1.5 hours)
   - Extract velocity constants
   - Verify with trace data
   - Update player-physics.md

5. **Batch 5: Damage System** (1 hour)
   - Find damage functions
   - Document common patterns
   - Create combat-system.md

6. **Batch 3: Level Loading** (2 hours)
   - Document remaining helpers
   - Extract error handling
   - Complete level-loading.md

---

## Success Metrics

### Current State
- Unknown functions: 225 FUN_ (14%)
- Audio system: 30% documented
- Physics constants: 30% documented
- Input system: 10% documented

### After Batch 1-2-6 (Today)
- Unknown functions: ~215 FUN_ (13%)
- Audio system: **80% documented** ✅
- Input system: **70% documented** ✅
- Collision system: **95% documented** ✅

### After Batch 4-5 (This Week)
- Unknown functions: ~200 FUN_ (12%)
- Physics constants: **80% documented** ✅
- Combat system: **70% documented** ✅

---

## Notes

**Key insight**: The regenerated decompilation has better function signatures and more recognizable patterns. Many functions have clear purposes from:
- Parameter types (entity pointer, sound ID, etc.)
- Called PSY-Q functions (SpuSetKey, SpuSetVoiceVolume)
- Field access patterns (entity+0x48 = bbox, entity+0x160 = velocity)

**Strategy**: Focus on **high-impact, low-effort** batches first:
1. Bounding box helpers (trivial, used everywhere)
2. Audio system (clear PSY-Q calls, well-documented)
3. Input system (standard button masks)

Then move to **high-value** analysis:
4. Physics constants (needed for evil-engine accuracy)
5. Damage system (common entity pattern)
