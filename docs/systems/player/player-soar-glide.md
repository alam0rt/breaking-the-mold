# SOAR and GLIDE Player Modes

**Status**: ✅ DOCUMENTED from C Code  
**Last Updated**: June 12, 2026  
**Source**: Ghidra `SLES_010.90` decompilation and xrefs; older line references below are retained only as historical context.

> **Clean-room caveat:** names are inferred from behavior. SOAR shares a large platform/RUNN callback cluster, so field names below describe observed roles in that shared code rather than original source names.

---

## Overview

SOAR and GLIDE are special player movement modes for specific level types.

**SOAR Flag**: 0x10 (flying/soaring mode)  
**GLIDE Flag**: 0x04 (gliding mode)

---

## GLIDE Mode (Flag 0x04)

### Creation

**Function**: CreateGlidePlayerEntity @ 0x8006edb8  
**Entity Size**: 0x11c bytes (284 bytes)

```c
Entity* CreateGlidePlayerEntity(Entity* buffer, void* inputController,
                                 short spawn_x, short spawn_y) {
    // Initialize with sprite table
    InitEntityWithSprite(buffer, &DAT_8009ca34, 1000, spawn_x, spawn_y);
    
    // Set vtable
    buffer[6] = &DAT_80011cf4;
    buffer[0x45] = 0xffffffff;
    
    // Configure
    buffer[4] = 1000;  // Z-order
    buffer[0x40] = inputController;
    buffer[0] = 0xffff0000;
    buffer[1] = FinnMainTickHandler;  // Reuses FINN tick handler!
    buffer[2] = 0xffff0000;
    buffer[3] = EntityInitCallback_8006f190;
    
    // Initialize physics
    buffer[0x41] = 0x20000;  // Initial velocity?
    buffer[0x43] = 0x400;    // Angle or speed
    buffer[0x42] = 0;
    buffer[0x1b] = 0;
    buffer[0x6e] = 0;
    buffer[0x10e] = 0;
    buffer[0x10f] = 0;
    buffer[0x44] = 0;
    buffer[0x111] = 0;
    buffer[0x112] = 0x14;  // Timer = 20
    
    // Set state
    EntitySetState(buffer, null_FFFF0000h_800a5f84, PTR_Callback_8006fdf4_800a5f88);
    
    buffer[0x113] = 0;
    buffer[0x119] = 0;
    
    return buffer;
}
```

**Sprite Table**: DAT_8009ca34  
**Tick Handler**: FinnMainTickHandler (same as FINN!)  
**Vtable**: DAT_80011cf4

### Glide Mechanics

**Key Observation**: Reuses FINN tick handler

**Likely Behavior**:
- Gliding/floating movement
- Gravity-affected but slower fall
- Horizontal control
- May use rotation like FINN

**Constants**:
- Field +0x41: 0x20000 (2.0 in 16.16 fixed)
- Field +0x43: 0x400 (1024 - angle or speed)
- Field +0x112: 0x14 (20 - timer)

---

## SOAR Mode (Flag 0x10)

### Creation

**Function**: CreateSoarPlayerEntity @ 0x80070d68  
**Entity Size**: 0x128 bytes (296 bytes)

```c
Entity* CreateSoarPlayerEntity(Entity* buffer, void* inputController,
                                short spawn_x, short spawn_y) {
    // Initialize with sprite table
    InitEntityWithSprite(buffer, &DAT_8009cabc, 1000, spawn_x, spawn_y);
    
    // Set vtable
    buffer[6] = &DAT_80011d34;
    
    // Configure
    buffer[4] = 1000;  // Z-order
    buffer[0x40] = inputController;
    soar->gravityHoldTimer = 0;
    soar->forcedGravityTimer = 0;
    soar->jumpTransitionLock = 0;
    soar->jumpHoldTimer = 0;
    soar->pendingLevelLoadId = 0;
    soar->inputEnabled = 0;
    soar->stateReturnTimer = 0;
    soar->levelLoadTimer = 0;
    soar->rgb[0] = 0x40;
    soar->rgb[1] = 0x40;
    soar->rgb[2] = 0x40;
    
    // Set callbacks (continues with more initialization...)
    
    return buffer;
}
```

**Sprite Table**: DAT_8009cabc  
**Vtable**: DAT_80011d34

### Soar / Platform Timer-Gate Fields

The SOAR player reuses platform/RUNN tick and input helpers, so offsets `+0x118..+0x120` are now documented as shared platform/flight timer-gate fields rather than six unknown flags:

| Offset | Field | Evidence-backed role |
|--------|-------|----------------------|
| `+0x118` | `gravityHoldTimer` | Set to `5` by `PlatformStateInit_BounceWithGravity @ 0x800727EC`; decremented by `PlatformEntityProcessInput @ 0x800720C4` while applying/capping vertical gravity response. |
| `+0x119` | `forcedGravityTimer` | Auxiliary gravity countdown decremented by `PlatformEntityProcessInput` when non-zero. No non-zero producer found in the checked SOAR/platform range, so the name is conservative. |
| `+0x11A` | `jumpTransitionLock` | If clear, jump/confirm input enters `PlatformStateInit_BounceWithGravity`; if set, the same input refreshes `jumpHoldTimer` instead. |
| `+0x11B` | `jumpHoldTimer` | Set to `10` while jump is held under the lock; decremented by `PlatformTick_MainWithSoundAndTimers @ 0x800713F4`; checked by `SoarState_SelectNextByInput @ 0x80071FD8`. |
| `+0x11C` | `pendingLevelLoadId` | Trigger-zone code stores the direct-level-load id here; copied to `GameState.direct_level_load` when `levelLoadTimer` reaches zero. |
| `+0x11D` | `inputEnabled` | Set by `PlatformState_EnablePlayerInput @ 0x80072CAC`; the main platform tick only calls `PlatformEntityProcessInput` while this is non-zero. |
| `+0x11E` | `stateReturnTimer` | Countdown to `EntityEnterAnimatedIdleState`; trigger-zone reset paths set it to `0x5A` frames. |
| `+0x120` | `levelLoadTimer` | Fade/direct-level-load countdown; set to `0x20` by `PlatformStateInit_FadeAndTimer @ 0x800730F8`. |
| `+0x122..+0x124` | `rgb[3]` | Runtime tint bytes; initialized to `0x40,0x40,0x40` and updated by generic trigger-zone color handling. |

Important state transitions:

- `SoarStateInit_BeginFlightMode @ 0x80072A98` clears the timer/gate fields, sets `jumpTransitionLock = 1`, disables input, and queues `SoarState_BeginFlight`.
- `SoarState_BeginFlight @ 0x80072BA0` restores scale, sets `nFlightSpeed = 0x50000`, clears `jumpTransitionLock` and `inputEnabled`, then installs `PlatformState_EnablePlayerInput` as the finalizer hook.
- `PlatformState_EnablePlayerInput` sets `inputEnabled = 1` and sends player event `0x100F`, after which the shared platform tick starts honoring controller input.

---

## Player Mode Selection (Lines 41218-41360)

### Flag Priority Order

**From SpawnPlayerAndEntities** (line 41222):

```c
void SpawnPlayerAndEntities(GameState* state) {
    uint flags = GetLevelFlags(state + 0x84);
    
    if (flags & 0x400) {
        // FINN mode (swimming)
        CreateFinnPlayerEntity(...);
    } else if (flags & 0x200) {
        // Menu mode
        InitMenuEntity(...);
    } else if (flags & 0x2000) {
        // Boss mode
        CreateBossPlayerEntity(...);
    } else if (flags & 0x100) {
        // RUNN mode (auto-scroller)
        CreateRunnPlayerEntity(...);
    } else if (flags & 0x10) {
        // SOAR mode (flying)
        CreateSoarPlayerEntity(...);
        // Also creates camera with offset -0x80 Y
    } else if (flags & 0x04) {
        // GLIDE mode
        CreateGlidePlayerEntity(...);
    } else {
        // Normal platformer
        CreatePlayerEntity(...);
    }
}
```

**Priority** (checked in order):
1. FINN (0x400) - Swimming
2. Menu (0x200) - Menu system
3. Boss (0x2000) - Boss fight
4. RUNN (0x100) - Auto-scroller
5. SOAR (0x10) - Flying
6. GLIDE (0x04) - Gliding
7. Normal - Standard platforming

---

## Level Type Summary

| Mode | Flag | Levels | Control Style | Camera |
|------|------|--------|---------------|--------|
| **Normal** | None | Most levels | Full platformer | Smooth follow |
| **FINN** | 0x400 | FINN (Lv 4) | Tank/rotation | Follow |
| **RUNN** | 0x100 | RUNN (Lv 22) | Auto-scroll + dodge | Auto-scroll |
| **SOAR** | 0x10 | Unknown | Flying | Vertical offset |
| **GLIDE** | 0x04 | Unknown | Gliding | Follow |
| **Menu** | 0x200 | MENU (Lv 0) | Menu navigation | None |
| **Boss** | 0x2000 | 5 boss levels | Normal | Arena |

---

## Camera Differences

### SOAR Camera (Lines 41344-41349)

```c
// Create camera with special offset
camera = CreateCameraEntity(buffer, spawn_x, spawn_y - 0x80, 0xa000);
```

**Y Offset**: -0x80 (-128 pixels) - Camera positioned higher  
**Parameter**: 0xa000 (special camera mode?)

**Purpose**: Keep player lower in screen for vertical flying space

### GLIDE Camera

**No Special Camera**: Uses standard camera or none

**Spawn Offsets** (line 41321):
- state[100] = 0 (no offset)
- state[0x66] = 0 (no offset)

### RUNN Camera

**No Camera Created**: Auto-scroll likely handled differently

**Spawn Offsets** (line 41353):
- state[100] = 0x28 (40)
- state[0x66] = 0xffd0 (-48)

---

## Sprite Tables

| Mode | Sprite Table | Address |
|------|--------------|---------|
| FINN | DAT_8009caec | 0x8009caec |
| RUNN | DAT_8009cadc | 0x8009cadc |
| SOAR | DAT_8009cabc | 0x8009cabc |
| GLIDE | DAT_8009ca34 | 0x8009ca34 |

**Pattern**: Each special mode has dedicated sprite table

---

## Implementation Notes

### For Godot

```gdscript
enum PlayerMode { NORMAL, FINN, RUNN, SOAR, GLIDE, MENU, BOSS }

func create_player_for_level(level_flags: int) -> Node2D:
    if level_flags & 0x400:
        return PlayerFinn.new()
    elif level_flags & 0x200:
        return MenuSystem.new()
    elif level_flags & 0x2000:
        return PlayerBoss.new()
    elif level_flags & 0x100:
        return PlayerRunn.new()
    elif level_flags & 0x10:
        return PlayerSoar.new()
    elif level_flags & 0x04:
        return PlayerGlide.new()
    else:
        return PlayerNormal.new()
```

---

## Related Documentation

- [Player FINN](player-finn.md) - Swimming mode (complete)
- [Player RUNN](player-runn.md) - Auto-scroller mode (complete)
- [Player Normal](player-normal.md) - Standard platforming
- [Level Flags](../../blb/asset-types.md) - Asset 100 flags

---

**Status**: ✅ **DOCUMENTED**  
**RUNN**: Fully documented from C code  
**SOAR/GLIDE**: Structure documented, gameplay needs verification  
**Implementation**: Ready for all special modes

