# Demo/Attract Mode System

**Status: PARTIALLY VERIFIED via Ghidra analysis (2026-01-16)**

The demo/attract mode system plays pre-recorded gameplay demos when the player is idle at the main menu.

## Overview

When the player remains idle at the main menu for approximately 30 seconds, the game automatically loads a demo level and plays back pre-recorded inputs. The "DEMO" sprite is displayed over the HUD during playback.

## Key Memory Addresses (PAL / SLES-01090)

| Address | Name | Purpose |
|---------|------|--------|
| 0x80076928 | `InitMenuEntity` | Menu entity initialization (uses sprite 0xb8700ca1) |
| 0x800313cc | `InitMenuEntity` | **Second** menu entity init (different, uses sprite 0xb8700ca1) |
| 0x80077940 | `MenuTickCallback` | Menu idle timer logic (VERIFIED) |
| 0x80077aa0 | `DemoCountdownCallback` | 20-frame countdown before demo load (VERIFIED) |
| 0x800a6043 | `g_DemoIndex` | Current demo index (gp+0x6ef) |
| 0x800a6045 | `DAT_800a6045` | Demo-related flag (gp+0x6f1) |
| 0x800a60a4 | `g_MenuDemoRotationCounter` | Demo mode cycling counter (0-4) |

## Menu Entity Offsets

| Offset | Type | Purpose |
|--------|------|---------|
| 0x100 | u32* | Pointer to controller input state |
| 0x134 | u32 | Menu param_2 (stored from init) |
| 0x138 | u8 | Max demo count |
| 0x13a | u16 | **Idle frame timer** |
| 0x13c | u16 | Demo countdown timer (set to 20) |
| 0x148 | u8 | Menu mode (99 = trigger demo) |

## Idle Timer Logic (MenuTickCallback @ 0x80077940)

**VERIFIED via Ghidra decompilation (2026-01-16)**

```c
// Actual decompiled code from Ghidra
void MenuTickCallback(undefined4 *param_1) {
    byte stage = GetCurrentStageIndex(g_GameStatePtr + 0x84);
    if (stage > 4) stage = 1;
    
    // Check if any input is pressed (param_1[0x40] = input state ptr)
    if (*(short *)param_1[0x40] == 0) {
        // No input - check if on stage 1 (main menu)
        if (stage == 1) {
            // Increment idle timer at +0x13A
            ushort timer = *(ushort *)((int)param_1 + 0x13a) + 1;
            *(ushort *)((int)param_1 + 0x13a) = timer;
            
            // Check threshold: 0x709 = 1801 frames ≈ 30 seconds at 60fps
            if (timer > 0x708 && *(char *)(param_1 + 0x4e) != '\0') {
                // Trigger demo mode
                SeekToLevelInSequence(
                    g_GameStatePtr + 0x84,
                    *(undefined1 *)(param_1[0x4d] + (uint)g_DemoIndex),
                    1, 0
                );
                *(undefined1 *)(g_GameStatePtr + 0x148) = 99;  // Menu mode
                *(undefined1 *)(g_GameStatePtr + 0x152) = 1;   // Load trigger
                
                // Advance demo index, wrap if needed
                g_DemoIndex = g_DemoIndex + 1;
                if (*(byte *)(param_1 + 0x4e) <= g_DemoIndex) {
                    g_DemoIndex = 0;
                }
            }
        }
    } else {
        // Input pressed - reset idle timer
        *(undefined2 *)((int)param_1 + 0x13a) = 0;
    }
    
    FUN_80077af0(param_1);  // Menu state update
    EntityUpdateCallback(param_1);
    
    // Check if demo flag was set
    if (g_DemoFlag != '\0') {
        *(undefined2 *)(param_1 + 0x4f) = 0x14;  // Countdown = 20 frames
        *param_1 = 0xffff0000;
        param_1[1] = DemoCountdownCallback;  // Switch callback
    }
}
```

## Demo Countdown Callback (DemoCountdownCallback @ 0x80077aa0)

**VERIFIED via Ghidra decompilation (2026-01-16)**

After the idle threshold is reached, this callback counts down for 20 frames before the demo actually loads:

```c
// Actual decompiled code from Ghidra
void DemoCountdownCallback(int param_1) {
    short timer = *(short *)(param_1 + 0x13c) - 1;
    *(short *)(param_1 + 0x13c) = timer;
    
    if (timer == 0) {
        // Copy demo flag to GameState menu mode
        *(undefined1 *)(g_GameStatePtr + 0x148) = g_DemoFlag;
    }
    
    EntityUpdateCallback();
}
```

## Demo Mode Rotation (InitializeAndLoadLevel @ 0x8007d1d0)

**VERIFIED via Ghidra analysis (2026-01-16)**

When loading level 0 (MENU) with param_2=1, the demo rotation logic activates:

```c
// VERIFIED from Ghidra decompilation of InitializeAndLoadLevel
cVar2 = GetCurrentLevelAssetIndex(param_1 + 0x21);
if ((cVar2 == '\0') && (param_2 == 1)) {
    param_2 = 5;  // Default to demo mode 1
    
    if (g_MenuDemoRotationCounter == 4) {
        g_MenuDemoRotationCounter = 0;
    } else {
        // Note: Uses comma operator - param_2 = 6 happens before the == 2 check
        if ((g_MenuDemoRotationCounter == 0) || (param_2 = 6, g_MenuDemoRotationCounter == 2)) {
            param_2 = 1;  // Normal mode (no demo)
        }
        g_MenuDemoRotationCounter = g_MenuDemoRotationCounter + 1;
    }
}
```

### Demo Mode Rotation Sequence

| Counter | param_2 Result | Mode |
|---------|---------------|------|
| 0 | 1 | Normal (no demo), counter→1 |
| 1 | 5 | Demo Mode 1, counter→2 |
| 2 | 1 | Normal (no demo), counter→3 |
| 3 | 5 | Demo Mode 1, counter→4 |
| 4 | 5 | Demo Mode 1, counter→0 (reset) |

### Demo Mode Values

| param_2 | Mode | Description |
|---------|------|-------------|
| 1 | Normal | Standard level loading |
| 5 | Demo Mode 1 | First demo playback mode |
| 6 | Demo Mode 2 | Set by comma operator, but immediately overwritten to 1 |

## Input Replay System (UpdateInputState @ 0x800259d4)

The same function handles both live input and replay playback.

### Input State Structure (VERIFIED in Ghidra as `InputState`)

| Offset | Type | Field | Purpose |
|--------|------|-------|--------|
| 0x00 | u16 | buttons_held | Current button state (PSX controller bitfield) |
| 0x02 | u16 | buttons_pressed | Newly pressed (edge detection) |
| 0x04 | ptr | playback_data_ptr | Pointer to playback buffer (demo mode) |
| 0x08 | u8 | playback_active | Non-zero if replaying recorded input |
| 0x09-0x0B | - | padding | Unused |
| 0x0C | ptr | recording_buffer | Pointer to recording buffer |
| 0x10 | u16 | playback_index | Current playback position |
| 0x12 | u16 | playback_timer | Frames until next input event |

### Replay Data Format (4 bytes per entry)

```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    Button state bitmask
0x02    2     u16    Frame duration (RLE count)
```

The replay format uses run-length encoding: each entry specifies a button state and how many consecutive frames to hold it.

### Playback Logic

```c
void UpdateInputState(InputState* state, u16 raw_buttons) {
    if (state->playback_mode) {
        // Playback mode: ignore raw_buttons, read from buffer
        if (state->current_index < *state->frame_count_ptr && raw_buttons == 0) {
            ReplayEntry* entry = &state->replay_buffer[state->current_index];
            
            state->pressed_buttons = entry->buttons & ~state->current_buttons;
            state->current_buttons = entry->buttons;
            state->frame_counter--;
            
            if (state->frame_counter == 0) {
                // Advance to next entry
                state->current_index++;
                state->frame_counter = state->replay_buffer[state->current_index].duration;
            }
        } else {
            // End playback (player pressed button or end of data)
            state->playback_mode = 0;
            state->current_buttons = 0;
            state->pressed_buttons = 0;
        }
        return;
    }
    
    // Normal input processing...
    state->pressed_buttons = raw_buttons & ~state->current_buttons;
    state->current_buttons = raw_buttons;
    
    // Recording logic (if enabled)...
}
```

## DEMO Sprite Display

The "DEMO" text sprite is displayed during demo playback.

### Sprite Information

| Field | Value |
|-------|-------|
| Sprite ID | 683704543 (0x28C080DF) |
| Frame Count | 1 (static sprite) |
| Location | Primary segment of each level |

The DEMO sprite exists in the primary segment sprites of **every level**, confirming it's a commonly-used HUD element.

### Levels with DEMO Sprite

All 26 levels contain the DEMO sprite (ID 683704543) in their primary segment:
- MENU, PHRO, SCIE, TMPL, BOIL, SNOW, FOOD, BRG1, GLID, CAVE
- WEED, EGGS, CLOU, SOAR, CRYS, CSTL, MOSS, EVIL
- MEGA, HEAD, GLEN, WIZZ, KLOG (bosses)
- FINN, RUNN (special modes)
- SEVN (secret bonus)

### Display Mechanism

The DEMO sprite is spawned when:
1. Level loaded with param_2 = 5 or 6 (demo modes)
2. Input playback mode is enabled on the player input state
3. Sprite is rendered at a fixed HUD position (likely screen-relative)

## BLB Header Playback Sequence

The demo system uses the playback sequence data in the BLB header to determine which levels can be played as demos.

### Key Header Offsets

| Offset | Purpose |
|--------|---------|
| 0xF30 | Sequence length (count of entries) |
| 0xF36+ | Mode array (one byte per entry) |
| 0xF92+ | Level index array (one byte per entry) |

### Mode Values in Playback Sequence

| Value | Mode |
|-------|------|
| 0 | Invalid/Reset |
| 1 | Movie playback |
| 2 | Credits sequence |
| 3 | Normal level |
| 4 | Unknown |
| 5 | Demo mode 1 |
| 6 | Demo mode 2 |

## Related Functions

| Address | Name | Purpose |
|---------|------|---------|
| 0x8007a294 | `AdvancePlaybackSequence` | Advance through BLB playback sequence |
| 0x8007a33c | `SetSequenceIndexByMode` | Find sequence entry by mode value |
| 0x8007a3ac | `SeekToLevelInSequence` | Find sequence entry by level index |
| 0x800259d4 | `UpdateInputState` | Input processing with replay support |
| 0x8001cb88 | `EntityUpdateCallback` | Entity update (called after menu tick) |

## Demo Levels

Based on the playback sequence and demo mode values, the game cycles through specific levels for demo playback. The demo rotation counter (0-4) determines which demo is shown:

- Counter 0, 2: Return to normal menu (no demo)
- Counter 1, 3: Play a demo level
- Counter 4: Reset counter to 0

## Open Questions

1. **Replay Data Location**: Where is the demo input replay data stored in the BLB? It's not in the level metadata or known asset types. Possibilities:
   - Hardcoded in the executable
   - Stored in an undocumented asset type
   - Part of entity data (Asset 501)

2. **DEMO Sprite Spawning**: What entity type or function is responsible for spawning and positioning the DEMO sprite during playback?

3. **Demo Level Selection**: Which specific levels are used for demos? The playback sequence data should reveal this.

## See Also

- [BLB Header Format](../blb/header.md)
- [Playback Sequence Data](../blb/header.md#playback-sequence-data-0xf34-0xfff)
- [Input System](./input-system.md) (if documented)
- [Menu Entity](./menu-entity.md) (if documented)
