# Gaps We Can Close From Decompiled Code

**Analysis Date**: January 15, 2026  
**Source**: Ghidra SLES_010.90.c decompilation (64,363 lines)  
**Based On**: gap-analysis.md, unconfirmed-findings.md

This document identifies specific gaps from our gap analysis that can be closed by examining the decompiled source code.

---

## Executive Summary

After completing animation framework analysis, **242 FUN_ functions remain** (15% of 1,599 total). By systematically analyzing these unknowns against our gap analysis, we can close:

**Immediate Wins** (Can document today from existing code):
1. ✅ Animation framework (COMPLETE - 5 layers documented)
2. 🔄 Remaining animation property setters (8 functions)
3. 🔄 Position/offset application (FUN_8001cc6c - now renamed)
4. 🔄 Collision attribute readers
5. 🔄 Physics constants (embedded in player state functions)

**Medium-Term** (Need runtime verification):
1. Tile collision attribute meanings (flags at Asset 500)
2. Physics constants (gravity, jump velocity, walk speed)
3. Callback message types (0, 1, 3 meanings)
4. Sound effect IDs

**Requires New Analysis**:
1. Save/load system
2. Boss AI behaviors
3. Projectile/weapon system

---

## Gap Category 1: Animation System ✅ CLOSED

### Status Before
- Animation timing unclear
- Frame metadata connection unknown
- State buffering mechanism unclear
- Sequence control format unknown

### Closed By Analysis (2026-01-15)
Created comprehensive `animation-framework.md` documenting:
- ✅ 5-layer architecture (metadata → timing → buffering → sequences → callbacks)
- ✅ 36-byte SpriteFrameEntry format
- ✅ Frame timing system (+0xEC countdown, +0xDA/+0xDE advancement)
- ✅ Double-buffered state changes (+0xE0 flags)
- ✅ 8-byte sequence entries (param + callback)
- ✅ 3-level callback priority system

### Functions Renamed
- FUN_8001d4bc → `AdvanceAnimationFrame`
- FUN_8001e790 → `StartAnimationSequence`
- FUN_8001e7b8 → `StepAnimationSequence`
- FUN_8001cc6c → `ApplyAnimationPositionOffsets`

### Remaining Animation Functions (Can Close Today)

These 8 setters follow the same pattern - set pending field, OR flag into +0xE0:

| Address | Current Name | Proposed Name | Flag | Purpose |
|---------|--------------|---------------|------|---------|
| 0x8001D024 | FUN_8001d024 | AllocateSpriteContext | - | Allocate 20-byte context |
| 0x8001D0B0 | FUN_8001d0b0 | SetAnimationSpriteFlags | 0x04 | Basic sprite flag |
| 0x8001D0C0 | FUN_8001d0c0 | SetAnimationFrameIndex | 0x08 | Set current frame |
| 0x8001D0F0 | FUN_8001d0f0 | SetAnimationFrameCallback | 0x208 | Frame lookup callback |
| 0x8001D170 | FUN_8001d170 | SetAnimationLoopFrame | 0x410 | Set loop target |
| 0x8001D1C0 | FUN_8001d1c0 | SetAnimationSpriteId | 0x20 | Change sprite |
| 0x8001D1F0 | FUN_8001d1f0 | SetAnimationSpriteCallback | 0x820 | Sprite lookup callback |
| 0x8001D218 | FUN_8001d218 | SetAnimationActive | 0x100 | Enable/disable ticking |

**Action**: Rename these 8 functions and add plate comments with flag values.

---

## Gap Category 2: Collision System ✅ 90% COMPLETE (2026-01-15)

**STATUS: BREAKTHROUGH - Comprehensive analysis complete**

### What We Extracted

**Functions decompiled and renamed:**
1. ✅ `PlayerProcessTileCollision` @ 0x8005a914 (150+ line switch statement analyzed)
2. ✅ `CheckEntityCollision` @ 0x800226f8 (entity-to-entity collision)
3. ✅ `GetTileAttributeAtPosition` @ 0x800241f4 (tile lookup)
4. ✅ `CheckTriggerZoneCollision` @ 0x800245bc (filter solid vs triggers)
5. ✅ `InitTileAttributeState` @ 0x80024cf4 (Asset 500 loader)
6. ✅ `SetSpawnZoneGroup1` @ 0x80025664 (spawn control group 1)
7. ✅ `SetSpawnZoneGroup2` @ 0x800256b8 (spawn control group 2)
8. ✅ `HandleGenericTriggerZone` @ 0x8007ee6c (generic trigger handler)

### Complete Tile Attribute Map (from switch statement)

**Solid Range (0x01-0x3B):** 59 values for floor/wall/platform collision
**Trigger Zones (0x3C+):** 30+ documented special tiles:

| Type | Values | Purpose |
|------|--------|---------|
| Checkpoints | 0x02-0x07 | World 0-5 progress markers |
| Death Zone | 0x2A | Kills if player falling/jumping |
| Item Pickups | 0x32-0x3B | 10 collectible items |
| Wind Horizontal | 0x3D-0x3E | Push left (-1) or right (+1) |
| Wind Diagonal | 0x3F-0x40 | Push X±2, Y-1 (conditional) |
| Wind Vertical | 0x41 | Strong down (-4) |
| Spawn Enable | 0x51, 0x52 | Activate enemy groups 1 & 2 |
| Spawn Disable | 0x65, 0x66 | Deactivate groups |
| Spawn Mode 2 | 0x79, 0x7A | Alternate spawn behavior |

### Documentation Created

✅ **docs/systems/tile-collision-complete.md** (2000+ lines)
- Complete tile attribute table with all 30+ trigger types
- Wind zone velocity mappings (X: ±1/±2, Y: -1/-4)
- Spawn zone system (2 groups × 3 modes)
- Entity collision masks and layer system
- Player entity field documentation (+0x160, +0x162, +0x170, +0x1A6, +0x1A8, +0x1AE, +0x1B3)
- Sound effect IDs for triggers
- Asset 500 format specification

### Remaining Gaps (10%)

⚠️ **Slope subtypes** within solid range (0x03-0x3B need per-value testing)
⚠️ **Physics response** to slopes (angle calculations, velocity projection)
⚠️ **Entity-to-tile** collision (separate from player collision)

**Time invested:** ~2 hours for complete extraction

---

## Gap Category 3: Physics Constants ⚠️ CAN PARTIALLY CLOSE

### From gap-analysis.md

> Physics constants unknown:
> - Walk speed
> - Jump velocity  
> - Gravity
> - Terminal velocity

### What We Found in Player State Functions

From analyzing `PlayerState_JumpApex` @ 0x80067d74:
```c
*(undefined2 *)((int)param_1 + 0x136) = 0xffd8;
```

This sets velocity to **0xffd8 = -40 decimal** at jump apex.

### Embedded Constants to Extract

Search decompiled code for:

1. **Literal velocity assignments**:
   ```
   grep "0x136.*0x" SLES_010.90.c  # Y velocity assignments
   grep "0xb4.*0x" SLES_010.90.c   # X velocity assignments
   ```

2. **Gravity application** (look for += patterns):
   ```c
   *(short *)(entity + 0xb8) += GRAVITY_CONSTANT;
   ```

3. **Speed limits** (look for comparisons):
   ```c
   if (*(short *)(entity + 0xb8) > MAX_FALL_SPEED) {
       *(short *)(entity + 0xb8) = MAX_FALL_SPEED;
   }
   ```

### Known Constants from Existing Analysis

From `player-physics.md` estimates:
- Walk Speed: ~2.0 px/frame (0x20000 in 16.16 fixed)
- Jump Velocity: -8.0 px/frame (0x80000)
- Gravity: ~0.5 px/frame² (0x8000)
- Max Fall: 8.0 px/frame (0x80000)

### Player State Transition Constants

Already documented in code comments:
- Jump apex velocity: **0xffd8** (-40)
- Field +0x156: **0x0C** (12) for jump, **0** for fall

### Strategy to Close This Gap

1. **Search** for all literal hex assignments to velocity fields (+0xB4, +0xB8, +0x136)
2. **Extract** physics constants from player movement callbacks
3. **Verify** against trace data (player-physics.md has measured speeds)
4. **Document** verified values with "VERIFIED via code analysis" tag

**Estimated Coverage**: 70% of constants identifiable from code, 30% need runtime measurement.

---

## Gap Category 4: Callback Messages ⚠️ CAN PARTIALLY CLOSE

### Current Knowledge

From animation-framework.md:
```
| Message | Context | Purpose |
|---------|---------|---------|
| 0 | Unknown | (needs investigation) |
| 1 | Frame metadata | Frame has callback_id |
| 2 | Animation complete | current_frame == target_frame |
| 3 | Collision/destruction | Entity collected or destroyed |
```

### How to Find Message 0 Meaning

Search decompiled code for:
```c
(*callback)(entity, 0, param, entity);
```

Look at context - what triggers message 0 calls?

### How to Verify Message 1 Usage

Already found in `UpdateSpriteFrameData`:
```c
if (*piVar9 != 0) {  // piVar9 = SpriteFrameEntry
    // ...
    (*pcVar10)(param_1 + iVar5, 1, *piVar9, param_1);
}
```
Message 1 = frame callback with callback_id from frame metadata.

### How to Find Message 3 Usage

Search for:
```c
(*callback)(entity, 3, 
```

Common in:
- Item collection (when player touches item)
- Enemy death (when enemy takes lethal damage)
- Destruction triggers

**Action**: Systematic grep for callback invocations with each message number.

---

## Gap Category 5: Asset Field Meanings ✅ MOSTLY CLOSED

### From unconfirmed-findings.md

Already verified via Ghidra analysis:
- ✅ Asset 101: VRAM slot config (field_0 = bank_a_count, field_1 = bank_b_count)
- ✅ Asset 401: Palette animation (4 bytes: enabled, start, end, speed)
- ✅ Asset 602: Audio volume (4 bytes per sample: left, right)
- ✅ Asset 502: VRAM rectangles (16 bytes each)
- ✅ TileHeader field_1c: VRAM rect count (matches Asset 502)
- ✅ TileHeader field_1e: Entity count (matches Asset 501 / 24)

### Remaining Unknown: TileHeader field_20

From blb-unknown-fields-analysis.md:
- Values 0-6 observed
- Boss levels always = 0
- Regular levels vary per-stage
- **Theory**: Visual effect or music variation index

### How to Find field_20 Purpose

Search for:
```c
GetTileHeaderWorldIndex  // Was misnamed, now renamed
```

Find all callers and see what they do with the return value.

**Known caller**: InitGameState @ 0x8007cd34 - initializes player state

**Action**: Trace this value through game initialization to see what it controls.

---

## Gap Category 6: Sound System ⚠️ CAN PARTIALLY CLOSE

### From gap-analysis.md

> Audio System gaps:
> - Sound effect playback (PlaySoundEffect @ 0x8007C388 mentioned)
> - Sound ID table (many hardcoded IDs seen)

### What We Can Extract from Code

1. **PlaySoundEffect function** exists and can be decompiled
2. **Sound IDs** are embedded as hex literals throughout code:
   ```c
   FUN_8001c4a4(entity, 0x248e52);  // Jump sound
   ```

### Strategy

1. **Grep** for all `0x8001c4a4` calls (or whatever PlaySoundEffect is named)
2. **Extract** second parameter (sound ID) and surrounding context
3. **Build table** of sound_id → event_name:
   ```
   0x248e52 → "Jump"
   0x?????? → "Land"
   0x?????? → "Die"
   ```
4. **Document** in new sound-effects.md

**Estimated Coverage**: 80% of sound IDs can be identified from usage context.

---

## Gap Category 7: Entity Behaviors ⚠️ COMPLEX

### From gap-analysis.md

> Enemy Behaviors gaps:
> - Ground walker AI (func_0x8002ea3c)
> - Flying enemy AI
> - Damage/death handling

### What's Already Known

From entity-identification.md:
- 121 entity types identified
- Entity callbacks documented
- Init functions mapped

### What's Still Complex

Individual entity AI functions (e.g., Type 25 monkey) are **large state machines**:
- 500-1000 lines of decompiled code
- Multiple internal states
- Attack patterns
- Movement routines

These require **dedicated analysis sessions** per entity type.

### Lower-Hanging Fruit

**Common patterns** across entities:
1. Damage calculation (how much damage dealt/taken)
2. Death animation sequences
3. Spawn conditions
4. Despawn conditions

Search for:
```c
DealDamageToEntity
TakeDamageFromEntity
EntityDeathSequence
```

**Action**: Document common entity lifecycle functions before diving into individual AI.

---

## Gap Category 8: Input System ⚠️ CAN CLOSE

### From gap-analysis.md

> Input System gaps:
> - Button mappings
> - Controller vibration
> - 2-player mode

### What's Already Documented

From player-physics.md:
```c
// Input Masks (from PSY-Q LIBPAD.H)
#define PAD_SELECT   0x0001
#define PAD_L3       0x0002
#define PAD_R3       0x0004
#define PAD_START    0x0008
#define PAD_UP       0x0010
#define PAD_RIGHT    0x0020
#define PAD_DOWN     0x0040
#define PAD_LEFT     0x0080
#define PAD_L2       0x0100
#define PAD_R2       0x0200
#define PAD_L1       0x0400
#define PAD_R1       0x0800
#define PAD_TRIANGLE 0x1000
#define PAD_CIRCLE   0x2000
#define PAD_CROSS    0x4000
#define PAD_SQUARE   0x8000
```

### What to Find

1. **Vibration calls**: Search for PSY-Q SPU/motor functions
2. **2-player check**: Search for "player 2" or pad index checks
3. **Button function mapping**: Already have from player state analysis

**Action**: Search for vibration/rumble functions, document if found.

---

## Systematic Function Analysis Plan

### Phase 1: Animation System (Complete ✅)
- [x] Core animation framework
- [ ] 8 remaining property setters
- [ ] Position offset application

### Phase 2: Collision System (Next Priority)
1. Decompile 5 collision functions
2. Map tile attribute ranges
3. Document solid/hazard/trigger zones
4. Cross-reference with bounce mechanics

### Phase 3: Physics Constants
1. Extract velocity assignments
2. Extract gravity/acceleration
3. Extract speed limits
4. Verify against trace data

### Phase 4: Audio System
1. Find PlaySoundEffect implementation
2. Extract all sound ID calls
3. Build sound effect reference table

### Phase 5: Entity Lifecycle
1. Document common damage functions
2. Document spawn/despawn patterns
3. Map death sequences

### Phase 6: Per-Entity AI (Long-term)
- Analyze top 10 most common enemies
- Document boss behaviors
- Map projectile types

---

## Metrics

### Functions Analyzed This Session (2026-01-15)
- Animation framework: 15+ functions documented
- Collision system: 8+ functions documented
- **Named/renamed: 20 functions total** (12 animation, 8 collision)
  * Animation: AllocateSpriteContext, SetAnimation* (7 setters), AdvanceAnimationFrame, StartAnimationSequence, StepAnimationSequence, ApplyAnimationPositionOffsets
  * Collision: PlayerProcessTileCollision, CheckEntityCollision, GetTileAttributeAtPosition, CheckTriggerZoneCollision, InitTileAttributeState, SetSpawnZoneGroup1, SetSpawnZoneGroup2, HandleGenericTriggerZone

### Remaining Unknown Functions
- **Total: 225 FUN_ functions (14%)** - Down from 242 (15%)
- Animation: 0 immediate unknowns ✅
- Collision: ~7 (slopes, physics response)
- Physics: ~10 (need verification)
- Audio: ~20 (medium priority)
- Entity AI: ~150+ (long-term)
- Other: ~38 (misc)

### Documentation Coverage (2026-01-15)
- BLB Format: 95% complete
- **Animation System: 95% complete** ✅ (was 0% at session start)
- **Collision System: 90% complete** ✅ (was 40%)
- Physics Constants: 50% complete (estimated values)
- Entity System: 70% complete
- Audio System: 30% complete
- Boss/AI: 10% complete

---

## Recommended Next Actions

### Today (Immediate) ✅ COMPLETE
1. ✅ Rename 8 animation setter functions
2. ✅ Add plate comments with flag values
3. ✅ Decompile GetTileAttributeAtPosition
4. ✅ Decompile CheckEntityCollision
5. ✅ Decompile PlayerProcessTileCollision
6. ✅ Map tile attributes from switch statement (30+ values)
7. ✅ Rename 8 collision helper functions
8. ✅ Create comprehensive collision reference doc

### This Week (2-3 hours remaining)
1. Extract physics constants from player state functions
2. Build sound ID table from grep results
3. Document entity lifecycle patterns
4. Map remaining slope collision values

### This Month (10-15 hours)
1. Per-entity AI deep dives (top 10 enemies)
2. Boss behavior documentation
3. Complete audio system reference
4. Level-specific mechanics (FINN vehicle, RUNN scrolling, etc.)
5. 🔄 Extract physics constants from player states

### This Week
1. Complete collision attribute mapping
2. Document sound effect ID table
3. Extract remaining physics constants
4. Close callback message 0/3 mystery

### This Month
1. Document common entity lifecycle functions
2. Analyze top 5 enemy types
3. Map projectile system
4. Document boss patterns (at least 1 boss)

### Long-term
1. Complete all 121 entity type behaviors
2. Reverse-engineer password encoding
3. Document save/load system
4. Map all menu states

---

## Success Criteria

A gap is **CLOSED** when:
1. Function is renamed from FUN_* to descriptive name
2. Plate comment documents purpose and key parameters
3. Related .md file is updated with verified information
4. Cross-references are added to other docs

A gap is **PARTIALLY CLOSED** when:
1. Function behavior is understood but needs verification
2. Some constants extracted, others need runtime measurement
3. Pattern identified but not all cases documented

---

## Conclusion

By systematically analyzing the 242 remaining FUN_ functions against our gap analysis, we can:

**Immediate wins** (this week):
- Close animation system completely (8 functions)
- Close collision attribute mapping (60-80%)
- Close physics constants (70%)
- Close sound effects table (80%)

**Medium-term** (this month):
- Close common entity behaviors
- Close input system documentation
- Partially close individual enemy AI (top 5)

**Long-term** (ongoing):
- Individual entity AI (150+ functions)
- Boss behaviors
- Save/load system
- Advanced systems (2-player, etc.)

The animation framework analysis proved that **systematic code reading + Ghidra renaming** is highly effective. We should continue this approach for remaining gaps.
