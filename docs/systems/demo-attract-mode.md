---
title: "Demo/Attract Mode System"
category: systems
tags: [systems, demo, attract, mode]
---

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
    byte stage = GetCurrentStageIndex(g_pGameState + 0x84);
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
                    g_pGameState + 0x84,
                    *(undefined1 *)(param_1[0x4d] + (uint)g_DemoIndex),
                    1, 0
                );
                *(undefined1 *)(g_pGameState + 0x148) = 99;  // Menu mode
                *(undefined1 *)(g_pGameState + 0x152) = 1;   // Load trigger
                
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
        *(undefined1 *)(g_pGameState + 0x148) = g_DemoFlag;
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

~~1. **Replay Data Location**: Where is the demo input replay data stored in the BLB?~~ **RESOLVED**

2. **DEMO Sprite Spawning**: What entity type or function is responsible for spawning and positioning the DEMO sprite during playback?

3. **Demo Level Selection**: Which specific levels are used for demos? The playback sequence data should reveal this.

## Demo Replay Data Source - RESOLVED (2026-01-19)

**The demo replay data is stored in Asset 700 (0x2BC) in the tertiary segment (stage0).**

### Asset 700 = Demo Replay Data

Previous documentation incorrectly identified Asset 700 as "unused SPU audio data". It is actually the **demo input replay buffer**.

**Key Discovery**: `GetDemoDataPtr` @ 0x8007BAC8 returns `ctx[0x54] + 0x10`, which is the Asset 700 pointer + 16 bytes (skipping the header).

### Asset 700 Format

```c
struct Asset700_Header {
    u32 entry_count;      // Always 1 (refers to sub-entries, not replay entries)
    u32 entry_id;         // Varies per level (e.g., 0x50412804)
    u32 data_size;        // Size of replay data in bytes
    u32 data_offset;      // Always 16 (offset to replay entries)
};

struct DemoReplayEntry {
    u16 buttons;          // PSX controller button bitmask
    u16 duration;         // Frames to hold this button state (RLE encoding)
};
```

### PSX Button Bitmask

| Bit | Value | Button |
|-----|-------|--------|
| 0 | 0x0001 | Select |
| 3 | 0x0008 | Start |
| 4 | 0x0010 | Up |
| 5 | 0x0020 | Right |
| 6 | 0x0040 | Down |
| 7 | 0x0080 | Left |
| 8 | 0x0100 | L2 |
| 9 | 0x0200 | R2 |
| 10 | 0x0400 | L1 |
| 11 | 0x0800 | R1 |
| 12 | 0x1000 | Triangle |
| 13 | 0x2000 | Circle (Jump in Skullmonkeys) |
| 14 | 0x4000 | Cross |
| 15 | 0x8000 | Square |

### Levels with Demo Data (Asset 700)

| Level | Stage | Size | Entries | Duration |
|-------|-------|------|---------|----------|
| MENU | stage0 | 480 | 120 | ~51s |
| SCIE | stage0 | 284 | 71 | ~26s |
| TMPL | stage0 | 304 | 76 | ~30s |
| BOIL | stage0 | 480 | 120 | ~51s |
| FOOD | stage0 | 316 | 75 | ~varies |
| BRG1 | stage0 | 340 | 81 | ~varies |
| GLID | stage0 | 192 | 44 | ~varies |
| CAVE | stage0 | 208 | 48 | ~varies |
| WEED | stage0 | 344 | 82 | ~varies |

**17 levels do NOT have Asset 700** - they cannot be used as demo levels.

### Example Replay Data (MENU/stage0)

```
Entry   Buttons         Duration  Description
-----   -------         --------  -----------
[0]     0x0077          0         Metadata (not played)
[1]     0x0000 (none)   35        Wait 35 frames
[2]     0x0080 (Left)   77        Walk left for 77 frames
[3]     0x2080 (L+○)    2         Jump while walking left
[4]     0x20C0 (L+D+○)  13        Crouch-jump left
...
Total: 120 entries, 3072 frames (~51 seconds)
```

### Code Flow for Demo Playback

1. **Idle Detection** (`MenuTickCallback` @ 0x80077940):
   - Counts idle frames at MenuEntity+0x13A
   - At 1801 frames (~30s), triggers demo mode

2. **Demo Level Selection**:
   - Reads from demo level array at MenuEntity+0x134
   - Uses `g_DemoIndex` (0x800A6043) to rotate through demos
   - Calls `SeekToLevelInSequence` to find demo level

3. **Level Loading** (`SetupAndStartLevel` @ 0x8007D8A0):
   - Checks `gameState->field_0x152` for demo mode
   - Calls `GetDemoDataPtr(ctx)` → returns Asset 700 + 0x10
   - Calls `InitEntityDataPointers(inputState, demoDataPtr)`
   - Calls `EnableDemoPlaybackMode(inputState, 1)`

4. **Input Replay** (`UpdateInputState` @ 0x800259D4):
   - When playback_active flag set, reads from replay buffer
   - Decrements duration counter each frame
   - Advances to next entry when counter hits 0
   - Exits demo if player presses any real button

5. **Demo Sprite Display**:
   - Spawns sprite ID 0x28C080DF at position (0xA0, 0x20)
   - Z-order 30000 (always on top)

## See Also

- [BLB Header Format](../blb/header.md)
- [Playback Sequence Data](../blb/header.md#playback-sequence-data-0xf34-0xfff)
- [Input System](./input-system.md) (if documented)
- [Menu Entity](./menu-entity.md) (if documented)
