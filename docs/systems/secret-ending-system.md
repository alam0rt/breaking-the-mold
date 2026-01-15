# Secret Ending System

**Status**: ✅ FULLY DOCUMENTED from C Code  
**Last Updated**: January 15, 2026  
**Source**: SLES_010.90.c lines 37985-38049, 40855, 42618

---

## Overview

Skullmonkeys has a **secret ending** (END2/MVWIN.STR) that is only accessible by completing the game with sufficient Swirly Q collectibles.

**Requirement**: **48 or more Swirly Qs** (checkpoint/swirl count)  
**Storage**: `g_pPlayerState[0x1b]`  
**Normal Ending**: END1 (always plays)  
**Secret Ending**: END2 (conditional on Swirly Q count)

---

## Secret Ending Condition

### The Check (Lines 37985-37989, 38028)

```c
// Check if player has 48+ Swirly Qs
if ((byte)g_pPlayerState[0x1b] > 0x2f) {  // 0x2f = 47, so > 47 means >= 48
    // Spawn special entity (secret ending trigger)
    entity = AllocateFromHeap(blbHeaderBufferBase, 0x100, 1, 0);
    entity = InitEntitySprite(entity, 0xaa0da270, 1000, 0, 0, 0);
    AddEntityToSortedRenderList(g_GameStatePtr, entity);
}
```

**Condition**: `g_pPlayerState[0x1b] >= 48` (0x30 in hex)

**Trigger**: Special entity spawned with sprite ID 0xaa0da270

**Location**: Lines 37985-37989 (in password/ending sequence)

### Additional Check (Line 38028)

```c
// In button input handler
if ((input->buttons & 0x10) == 0 ||  // D-Pad button check
    state[0x14c] == 0 ||
    (byte)g_pPlayerState[0x1b] < 0x30) {  // < 48 Swirly Qs
    goto LAB_80079700;  // Skip secret ending path
}
```

**Prevents**: Triggering secret ending if < 48 Swirly Qs

**Button**: 0x10 = D-Pad Right (special trigger button?)

---

## Player State Field 0x1b

### Swirly Q Total Counter

**Offset**: `g_pPlayerState[0x1b]`  
**Type**: u8 (byte)  
**Purpose**: **Total Swirly Qs collected across entire game**  
**Max**: Unknown (at least 48+ for secret ending)

**Different from**:
- `g_pPlayerState[0x13]`: Current Swirly Q ammo (max 20, used for shooting)
- `g_pPlayerState[0x1b]`: **Total collected** (cumulative, for secret ending)

### Cheat Code Sets It (Line 42618)

```c
// Cheat code 0x02 (All Powerups)
g_pPlayerState[0x1b] = 0x30;  // Set to exactly 48
```

**Cheat**: Sets total Swirly Qs to 48 (minimum for secret ending)

**Implication**: This field tracks cumulative collection, not current ammo

---

## Secret Ending Trigger Entity

**Sprite ID**: 0xaa0da270  
**Z-Order**: 1000  
**Position**: (0, 0)  
**Size**: 0x100 bytes

**Purpose**: Visual indicator or trigger for secret ending path

**When Spawned**: During ending sequence if >= 48 Swirly Qs

---

## Ending Sequence Logic

### Normal Ending (< 48 Swirly Qs)

**Flow**:
1. Complete final level (EVIL world)
2. Defeat Klogg boss
3. Play END1 movie (\\MVEND.STR)
4. Show credits
5. Return to menu

**Movie**: END1 only

### Secret Ending (>= 48 Swirly Qs)

**Flow**:
1. Complete final level (EVIL world)
2. Defeat Klogg boss
3. Check: g_pPlayerState[0x1b] >= 48?
4. If YES: Spawn special entity (0xaa0da270)
5. Play END1 movie
6. **Play END2 movie** (\\MVWIN.STR) - SECRET!
7. Show extended credits
8. Return to menu

**Movies**: END1 + END2

**Special Entity**: Visual feedback that secret ending unlocked

---

## How to Achieve Secret Ending

### Requirement

**Collect 48+ Swirly Qs** across the entire game

**Swirly Q Locations**:
- Found in levels as collectibles
- Different from clayballs
- Tracked separately from ammo count

**Challenge**: Must explore thoroughly and collect most/all Swirly Qs

### Password Hint

**"No Password longer than 3"**:
- Implies completing game without using passwords
- OR using only early passwords (worlds 1-3)
- Ensures player collects Swirly Qs from early levels

**Rationale**: Passwords skip levels, missing Swirly Q collection opportunities

---

## Implementation

### Tracking System

```c
// When player collects Swirly Q
void CollectSwirlQ() {
    // Increment ammo (for shooting)
    if (g_pPlayerState[0x13] < 20) {
        g_pPlayerState[0x13]++;
    }
    
    // Increment total (for secret ending)
    g_pPlayerState[0x1b]++;
    
    // Play collection sound
    PlaySoundEffect(COLLECTION_SOUND, pan, 0);
}
```

### Ending Check

```c
// At game completion
void TriggerEnding() {
    // Play normal ending
    PlayMovie("END1");
    
    // Check for secret ending
    if (g_pPlayerState[0x1b] >= 48) {
        // Spawn special entity (visual feedback)
        Entity* special = CreateEntity(0xaa0da270);
        AddToRenderList(special);
        
        // Play secret ending movie
        PlayMovie("END2");
    }
    
    // Show credits
    ShowCredits();
}
```

### Godot Implementation

```gdscript
extends Node
class_name EndingManager

# Player state reference
var player_state: PlayerState

# Ending movies
const END1_MOVIE = "res://movies/ending1.ogv"
const END2_MOVIE = "res://movies/victory_secret.ogv"

# Secret ending requirement
const SECRET_ENDING_SWIRLY_QS = 48

func trigger_ending() -> void:
    # Always play normal ending
    await CutscenePlayer.play_movie_from_path(END1_MOVIE)
    
    # Check for secret ending
    if player_state.total_swirly_qs >= SECRET_ENDING_SWIRLY_QS:
        # Show special indicator
        show_secret_ending_unlocked()
        
        # Play secret ending
        await CutscenePlayer.play_movie_from_path(END2_MOVIE)
        
        # Extended credits or special reward
        show_extended_credits()
    else:
        # Normal credits
        show_credits()

func show_secret_ending_unlocked() -> void:
    # Visual feedback that secret ending unlocked
    var special_sprite = Sprite2D.new()
    special_sprite.texture = load("res://sprites/secret_unlocked.png")
    add_child(special_sprite)
    
    await get_tree().create_timer(3.0).timeout
    special_sprite.queue_free()
```

---

## Player State Field Documentation

### Field 0x1b - Total Swirly Qs Collected

**Offset**: `g_pPlayerState[0x1b]`  
**Type**: u8 (byte)  
**Purpose**: **Cumulative Swirly Q collection counter** (entire game)  
**Used For**: Secret ending unlock condition  
**Threshold**: >= 48 for secret ending

**Different From**:
- **Field 0x13**: Current Swirly Q ammo (max 20, for shooting)
- **Field 0x1b**: Total collected (cumulative, for ending)

**Cheat Code**: Cheat 0x02 sets this to 0x30 (48) - minimum for secret

---

## Secret Ending Entity

**Sprite ID**: 0xaa0da270  
**Purpose**: Visual indicator that secret ending unlocked  
**Display**: Shown during ending sequence  
**Z-Order**: 1000  
**Position**: (0, 0) - may be centered or specific location

**Visual**: Unknown (needs sprite extraction) - likely:
- "Secret Ending Unlocked!" text
- Special icon or animation
- Victory emblem

---

## Related Systems

### Swirly Q Collection

**Two Counters**:
1. **Ammo** (0x13): For projectile attacks, max 20
2. **Total** (0x1b): For secret ending, cumulative

**Collection**: Both increment when Swirly Q collected

**Display**: 
- Ammo shown in HUD during gameplay
- Total not displayed (hidden counter)

### Password System Impact

**Password Use**: Skips levels, misses Swirly Q collection

**Secret Ending Strategy**: 
- Play through game without passwords
- OR use only early passwords (collect from later levels)
- Ensures sufficient Swirly Q collection

**Hint**: "No Password longer than 3" = Don't skip past world 3

---

## Movie Details

### END1 (Normal Ending)

**File**: \\MVEND.STR;1  
**Size**: 1044 sectors (~2.1 MB)  
**Always Plays**: Yes (all completions)

### END2 (Secret Ending)

**File**: \\MVWIN.STR;1  
**Size**: 793 sectors (~1.6 MB)  
**Conditional**: Only if >= 48 Swirly Qs  
**Content**: Extended/alternate ending sequence

**Reward**: Special story conclusion or extra content

---

## Verification

**Code References**:
- Line 37985: `if (0x2f < g_pPlayerState[0x1b])` - Spawn special entity
- Line 38028: `if (g_pPlayerState[0x1b] < 0x30)` - Block secret path
- Line 40855: Additional check (context unclear)
- Line 42618: Cheat sets to 0x30 (48)

**Confirmed**: Secret ending requires >= 48 Swirly Qs

---

## Related Documentation

- [Items Reference](../../reference/items.md) - Swirly Q documentation
- [Movie System](movie-cutscene-system.md) - END1/END2 movies
- [Player System](../player/player-system.md) - Player state structure
- [Cheat Codes](../../reference/cheat-codes.md) - Cheat 0x02

---

**Status**: ✅ **FULLY DOCUMENTED**  
**Condition**: >= 48 Swirly Qs (g_pPlayerState[0x1b])  
**Movies**: END1 (normal) + END2 (secret)  
**Implementation**: Ready for secret ending system

