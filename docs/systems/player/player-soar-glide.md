# SOAR and GLIDE Player Modes

**Status**: ✅ DOCUMENTED from C Code  
**Last Updated**: January 15, 2026  
**Source**: SLES_010.90.c lines 34434-35454, 41323-41348

---

## Overview

SOAR and GLIDE are special player movement modes for specific level types.

**SOAR Flag**: 0x10 (flying/soaring mode)  
**GLIDE Flag**: 0x04 (gliding mode)

---

## GLIDE Mode (Flag 0x04)

### Creation

**Function**: CreateGlidePlayerEntity @ 0x8006e434 (line 34434)  
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

**Function**: CreateSoarPlayerEntity @ 0x8006e454 (line 35454)  
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
    buffer[0x46] = 0;
    buffer[0x119] = 0;
    buffer[0x11a] = 0;
    buffer[0x11e] = 0;
    buffer[0x11b] = 0;
    buffer[0x47] = 0;
    buffer[0x48] = 0;
    buffer[0x122] = 0x40;  // Field = 64
    
    // Set callbacks (continues with more initialization...)
    
    return buffer;
}
```

**Sprite Table**: DAT_8009cabc  
**Vtable**: DAT_80011d34

### Soar Mechanics

**Key Fields**:
- +0x122: 0x40 (64 - some parameter)
- Multiple fields zeroed
- Larger entity than GLIDE

**Likely Behavior**:
- Flying/soaring through air
- Full directional control
- No gravity or reduced gravity
- Vertical and horizontal movement

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

