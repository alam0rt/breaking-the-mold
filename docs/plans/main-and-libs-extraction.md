# Extraction Plan: main() and LIBS Functions

**Created:** January 24, 2026  
**Updated:** January 24, 2026  
**Status:** Planning

## Overview

This document outlines the plan to extract and decompile:
1. **`main()`** - The game's entry point (0x800828B0)
2. **Related GAMELOOP functions** - Functions called by main and in the same segment
3. **LIBS section** - PSY-Q SDK library functions (0x80083000 - 0x80090FEB)

## Call Graph Analysis (from Ghidra)

`main()` directly calls 22 functions at depth 1, which themselves call 52 more functions at depth 2.

### Direct Calls from main() (Depth 1)

| Address | Name | Section | Notes |
|---------|------|---------|-------|
| 0x8008E6E0 | `__main` | LIBS | C runtime init |
| 0x80013248 | `SsUtReverbOn` | RENDER | Audio reverb enable |
| 0x80087E18 | `ResetCallback` | LIBS | PSX interrupt setup |
| 0x800389DC | `LoadGameAssetLocations` | OBJECT | Find GAME.BLB on CD |
| 0x80013268 | `InitGraphicsSystem` | RENDER | Double-buffer GPU setup |
| 0x8008762C | `PadInit` | LIBS | Controller init |
| 0x8008D504 | `InitGeom` | LIBS | GTE init |
| 0x8008A1FC | `SetDispMask` | LIBS | Enable display |
| 0x80088C70 | `FntLoad` | LIBS | Load debug font |
| 0x80088D14 | `FntOpen` | LIBS | Open debug font stream |
| 0x80088C30 | `SetDumpFnt` | LIBS | Set font dump target |
| 0x800260D0 | `initPlayerState` | ENTITY | Init player struct |
| 0x8007CD34 | `InitGameState` | GAMELOOP | Load BLB header, init level |
| 0x8007A9B0 | `GetLevelCount` | GAMELOOP | BLB accessor |
| 0x8007AA08 | `getLevelName` | GAMELOOP | BLB accessor |
| 0x8007ACDC | `GetAssetCount` | GAMELOOP | BLB accessor |
| 0x8007ACF0 | `GetMovieEntryByIndex` | GAMELOOP | BLB accessor |
| 0x8007CCB8 | `TickCDStreamBuffer` | GAMELOOP | CD streaming |
| 0x8008767C | `PadRead` | LIBS | Read controllers |
| 0x800259D4 | `UpdateInputState` | ENTITY | Process button input |
| 0x80020E1C | `EntityTickLoop` | ENTITY | Update all entities |
| 0x8001352C | `WaitForVBlankIfNeeded` | RENDER | Conditional VSync |
| 0x80020E80 | `RenderEntities` | ENTITY | Draw entity render pass |
| 0x8008A298 | `DrawSync` | LIBS | Wait for GPU |
| 0x80087C24 | `VSync` | LIBS | Wait for vblank |
| 0x80082C10 | `ProcessDebugMenuInput` | GAMELOOP | Debug level select |
| 0x80013500 | `FlushDebugFontAndEndFrame` | RENDER | Draw debug text |

### Key GAMELOOP Functions Called by main()

These functions are in the GAMELOOP segment and should be extracted together:

| Address | ROM Offset | Size | Name | Priority |
|---------|------------|------|------|----------|
| 0x8007A9B0 | 0x6B1B0 | 0x14 | `GetLevelCount` | High |
| 0x8007A9C4 | 0x6B1C4 | 0x24 | `GetLevelAssetIndex` | High |
| 0x8007AA08 | 0x6B208 | 0x20 | `getLevelName` | High |
| 0x8007AA28 | 0x6B228 | 0x24 | `GetLevelFlagByIndex` | High |
| 0x8007AA4C | 0x6B24C | 0x44 | `GetCurrentLevelAssetIndex` | High |
| 0x8007ACDC | 0x6B4DC | 0x14 | `GetAssetCount` | High |
| 0x8007ACF0 | 0x6B4F0 | 0x20 | `GetMovieEntryByIndex` | High |
| 0x8007CCB8 | 0x6D4B8 | 0x70 | `TickCDStreamBuffer` | High |
| 0x8007CD34 | 0x6D534 | 0x270 | `InitGameState` | High |
| 0x800828B0 | 0x730B0 | 0x360 | `main` | **Critical** |
| 0x80082C10 | 0x73410 | 0x280 | `ProcessDebugMenuInput` | High |
| 0x80082E90 | 0x73690 | 0x28 | `DestroySpecificEntity` | Medium |
| 0x80082F54 | 0x73754 | 0xC | `strcmp` | Low (stdlib) |
| 0x80082F64 | 0x73764 | 0xC | `memcpy` | Low (stdlib) |
| 0x80082F74 | 0x73774 | 0xC | `rand` | Low (stdlib) |
| 0x80082F84 | 0x73784 | 0xC | `srand` | Low (stdlib) |
| 0x80082F94 | 0x73794 | 0x64 | `memmove` | Low (stdlib) |

## 1. main() Function

### Current State
- **Address:** 0x800828B0 (ROM offset: 0x730B0)
- **Size:** 0x360 (864 bytes)
- **Location:** `asm/Game/GAMELOOP.s` (lines 20915-21147)
- **Symbol defined:** Yes, in `symbol_addrs.txt`

### Ghidra Decompilation (Verified)

```c
void main(void) {
    // === INITIALIZATION PHASE ===
    __main();                           // C runtime init
    SsUtReverbOn();                     // Enable audio reverb
    ResetCallback();                    // PSX interrupt setup
    LoadGameAssetLocations();           // Find GAME.BLB on CD
    InitGraphicsSystem(blbHeaderBufferBase);  // Double-buffer GPU (320x256)
    g_GameStatePtr = &g_GameState;
    PadInit(0);                         // Controller init
    InitGeom();                         // GTE init
    SetDispMask(1);                     // Enable display
    
    // Debug font at VRAM (0x3c0, 0x100)
    FntLoad(0x3c0, 0x100);
    SetDumpFnt(FntOpen(0x10, 0x20, 0x120, 200, 0, 0x200));
    
    initPlayerState(g_pPlayerState);
    InitGameState(&g_GameState, g_pPlayer1Input);
    
    // Populate level/movie name tables
    for (i = 0; i < GetLevelCount(); i++) {
        g_LevelNameTable[i] = getLevelName(i);
        g_TotalMenuItems++;
        g_LevelCount++;
    }
    for (i = 0; i < GetAssetCount(); i++) {
        g_MenuItemNames[g_TotalMenuItems++] = GetMovieReservedByIndex();
        g_MovieCount++;
    }
    
    // Default background color
    g_DefaultBGColorR = 0x40;
    g_DefaultBGColorG = 0x20;
    g_DefaultBGColorB = 0x80;
    g_pCurrentInputState = g_pPlayer1Input;
    
    // === MAIN GAME LOOP (infinite) ===
    while (true) {
        TickCDStreamBuffer();           // Stream CD data every 4 frames
        u_long padData = PadRead(1);    // Read controller ports
        UpdateInputState(g_pPlayer1Input, padData & 0xFFFF);       // P1
        UpdateInputState(g_pPlayer2Input, padData >> 16);          // P2
        
        // Mode callback dispatch
        if (g_GameStatePtr->mode_callback_index != 0) {
            // Execute current game mode handler via callback table
            (*callback)(g_GameStatePtr + offset);
        }
        
        EntityTickLoop(g_GameStatePtr); // Update all entities
        WaitForVBlankIfNeeded();        // Conditional VSync
        RenderEntities(g_GameStatePtr); // Draw entities
        DrawSync(0);                    // Wait for GPU
        
        // Layer render callback (tile layers)
        (*g_GameStatePtr->layer_render_callback)(...)
        DrawSync(0);
        
        // Frame timing (wait 2 frames if flag set)
        if ((g_GameFlags & 6) != 0) {
            VSync(2);
        }
        
        ProcessDebugMenuInput();        // Handle debug level select
        FlushDebugFontAndEndFrame();
    }
}
```

### Extraction Steps

#### Phase 1: Extract main() Only (Minimal Change)

1. **Modify `SLES_010.90.yaml`** - Add C segment for main:
   ```yaml
   # In GAMELOOP subsegments, change:
   - [0x61848, asm, Game/GAMELOOP]
   
   # To:
   - [0x61848, asm, Game/GAMELOOP]        # 0x80071048 - Functions before main
   - [0x730B0, c, Game/GAMELOOP/main]     # 0x800828B0 - main()
   - [0x73410, asm, Game/GAMELOOP]        # 0x80082C10 - Functions after main
   ```

2. **Create `src/Game/GAMELOOP/main.c`**:
   ```c
   #include "common.h"
   
   // External function declarations (from other segments)
   extern void __main(void);
   extern void SsUtReverbOn(void);
   extern void ResetCallback(void);
   extern void LoadGameAssetLocations(void);
   extern void InitGraphicsSystem(void *buffer);
   extern void PadInit(int mode);
   extern void InitGeom(void);
   extern void SetDispMask(int mask);
   extern void FntLoad(int x, int y);
   extern int FntOpen(int x, int y, int w, int h, int clear, int count);
   extern void SetDumpFnt(int id);
   extern void initPlayerState(void *state);
   extern void InitGameState(void *state, void *input);
   extern u8 GetLevelCount(void *ctx);
   extern char *getLevelName(void *ctx, int idx);
   extern u8 GetAssetCount(void *ctx);
   extern void *GetMovieEntryByIndex(void *ctx, int idx);
   extern void TickCDStreamBuffer(void);
   extern u_long PadRead(int port);
   extern void UpdateInputState(void *input, u16 pad);
   extern void EntityTickLoop(void *state);
   extern void WaitForVBlankIfNeeded(void);
   extern void RenderEntities(void *state);
   extern void DrawSync(int mode);
   extern int VSync(int mode);
   extern void ProcessDebugMenuInput(void);
   extern void FlushDebugFontAndEndFrame(void);
   
   INCLUDE_ASM("asm/nonmatchings/Game/GAMELOOP/main", main);
   ```

3. **Run build**: `make clean && make check`

#### Phase 2: Extract BLB Accessor Functions

These small utility functions should be easy to decompile:

```yaml
# Split GAMELOOP further for BLB accessors:
- [0x6B1B0, c, Game/GAMELOOP/BLBAccessors]  # 0x8007A9B0 - GetLevelCount, etc.
- [0x6B530, asm, Game/GAMELOOP]              # Continue with rest
```

**Target functions in `BLBAccessors.c`:**
- `GetLevelCount` (0x8007A9B0, 20 bytes) - trivial
- `GetLevelAssetIndex` (0x8007A9C4, 36 bytes) - trivial
- `GetLevelDisplayName` (0x8007A9E8, 32 bytes) - trivial
- `getLevelName` (0x8007AA08, 32 bytes) - trivial
- `GetLevelFlagByIndex` (0x8007AA28, 36 bytes) - trivial
- `GetCurrentLevelAssetIndex` (0x8007AA4C, 68 bytes)
- etc.

#### Phase 3: Extract InitGameState and Level Loading

**Target functions:**
- `InitGameState` (0x8007CD34) - Large, complex, loads levels
- `InitializeAndLoadLevel` (0x8007D1D0)
- `SpawnPlayerAndEntities` (0x8007DF38)

#### Phase 4: Extract Debug Menu and Utilities

**Target functions:**
- `ProcessDebugMenuInput` (0x80082C10) - Debug level selector
- `strcmp`, `memcpy`, `rand`, `srand`, `memmove` - Standard library functions

### YAML Segment Layout (Final Target)

```yaml
# --- GAMELOOP: Main game flow, menus, level loading (0x80071048 - 0x80082FFF) ---
# Tile collision, FINN vehicle, menu system, BLB loading, main()

# Tile collision system (0x80071048 - 0x800733FF)
- [0x61848, asm, Game/GAMELOOP/TileCollision]

# FINN vehicle system (0x80073400 - 0x80074FFF)  
- [0x64600, asm, Game/GAMELOOP/FinnVehicle]

# Menu system (0x80075000 - 0x8007A9AF)
- [0x65800, asm, Game/GAMELOOP/Menu]

# BLB header accessors (0x8007A9B0 - 0x8007B3EF)
- [0x6B1B0, c, Game/GAMELOOP/BLBAccessors]

# Level data accessors (0x8007B3F0 - 0x8007CA9B)
- [0x6BBF0, c, Game/GAMELOOP/LevelAccessors]

# CD streaming (0x8007CA9C - 0x8007CF9F)
- [0x6D29C, c, Game/GAMELOOP/CDStream]

# Level loading (0x8007CFA0 - 0x8008149B)
- [0x6D7A0, c, Game/GAMELOOP/LevelLoad]

# Entity type mapping (0x8008149C - 0x80082847)
- [0x71C9C, asm, Game/GAMELOOP/EntityTypes]

# Main function (0x80082848 - 0x80082C0F)
- [0x73048, c, Game/GAMELOOP/main]

# Debug menu and utilities (0x80082C10 - 0x80082FFF)
- [0x73410, c, Game/GAMELOOP/DebugMenu]
```

### Related Functions (Same Extraction Batch)

| Address | Size | Name | Description |
|---------|------|------|-------------|
| 0x800828B0 | 0x360 | `main` | Game entry point |
| 0x80082C10 | 0x280 | `ProcessDebugMenuInput` | Debug menu handler |
| 0x80082E90 | 0x28 | `DestroySpecificEntity` | Entity cleanup |

---

## 2. LIBS Section (PSY-Q SDK)

### Current State
- **Address Range:** 0x80083000 - 0x80090FEB
- **ROM Offset:** 0x73800 - 0x80FEB
- **Function Count:** 559 functions
- **Current Config:** `[0x73800, asm, LIBS]`

### Library Breakdown

| Library | Address Range | Functions | Description |
|---------|---------------|-----------|-------------|
| libcd | 0x80083000 - 0x800867FF | ~70 | CD-ROM access |
| libgpu | 0x80086800 - 0x8008C7FF | ~150 | GPU/graphics primitives |
| libgte | 0x8008C800 - 0x8008D7FF | ~40 | Geometry Transform Engine |
| libetc | 0x8008D800 - 0x8008E7FF | ~30 | Misc utilities |
| libspu | 0x8008E800 - 0x80090FFF | ~100 | Sound Processing Unit |
| crt0 | 0x8008E6E0 - 0x8008E7B7 | 3 | C runtime (__main, etc.) |

### Key Functions (Priority for Naming/Documentation)

#### libcd (CD-ROM)
```c
CdInit()           @ 0x80083030  // Initialize CD subsystem
CdSearchFile()     @ 0x800850E4  // Find file on disc
CdRead()           @ 0x80085XXX  // Read sectors
CdSync()           @ 0x800832AC  // Wait for CD operation
CdPosToInt()       @ 0x8008386C  // Convert position to sector
```

#### libgpu (Graphics)
```c
DrawSync()         @ 0x8008A298  // Wait for GPU
SetDispMask()      @ 0x8008A1FC  // Enable/disable display
ClearOTag()        @ 0x8008A6E8  // Clear ordering table
LoadImage()        @ 0x8008A55C  // Upload to VRAM
StoreImage()       @ 0x8008A5C0  // Download from VRAM
```

#### libetc (Utilities)
```c
ResetCallback()    @ 0x80087E18  // Reset interrupt handlers
PadInit()          @ 0x8008762C  // Initialize controllers
PadRead()          @ 0x8008767C  // Read controller state
VSync()            @ 0x80087C24  // Wait for vertical blank
```

#### libgte (GTE)
```c
InitGeom()         @ 0x8008D504  // Initialize GTE
```

#### libspu (Audio)
```c
SsUtReverbOn()     @ 0x80013248  // Enable reverb (note: in game code area!)
SpuSetVoiceVolume() @ 0x80090724 // Set voice volume
```

#### crt0 (C Runtime)
```c
__main()           @ 0x8008E6E0  // C runtime initialization (calls constructors)
```

### Ghidra Decompilation Examples

#### `__main()` - C Runtime Init
```c
void __main(void) {
    if (g_CrtInitialized == 0) {
        g_CrtInitialized = 1;
        // Call 4 static constructors from table at 0x800907D0
        for (int i = 0; i < 4; i++) {
            g_CrtConstructorTable[i]();
        }
    }
}
```

#### `CdInit()` - CD Subsystem Init
```c
int CdInit(void) {
    for (int i = 4; i >= 0; i--) {
        if (CdReset(1) == 1) {
            CdSyncCallback(def_cbsync);
            CdReadyCallback(def_cbready);
            CdReadCallback(def_cbread);
            CdReadMode(0);
            return EVENT_OBJ_84();
        }
    }
    printf("ings.\n");  // Truncated error message
    return 0;
}
```

#### `DrawSync()` - GPU Wait
```c
int DrawSync(int mode) {
    if (debug_level > 1) {
        printf("DrawSync(%d)...\n", mode);
    }
    return (*gpu_vtable->DrawSync)(mode);
}
```

### Extraction Strategy for LIBS

#### Option A: Keep as Assembly (Recommended for Now)
- PSY-Q SDK code is well-documented externally
- Focus decompilation effort on game-specific code
- Use assembly with proper symbol names

#### Option B: Split into Subsegments by Library
```yaml
# In SLES_010.90.yaml, replace:
- [0x73800, asm, LIBS]

# With:
- [0x73800, asm, LIBS/libcd]    # 0x80083000 - libcd
- [0x77800, asm, LIBS/libgpu]   # 0x80087000 - libgpu  
- [0x7B800, asm, LIBS/libgte]   # 0x8008B000 - libgte
- [0x7D800, asm, LIBS/libetc]   # 0x8008D000 - libetc
- [0x7E800, asm, LIBS/libspu]   # 0x8008E000 - libspu
```

#### Option C: Full Decompilation
- Create C stubs with INCLUDE_ASM for each library
- Gradually decompile using Ghidra output
- Compare against known PSY-Q source code patterns

### Priority Functions to Decompile

1. **`__main`** (0x8008E6E0) - Simple, shows CRT initialization
2. **`ResetCallback`** (0x80087E18) - Very simple wrapper
3. **`DrawSync`** (0x8008A298) - Simple with debug output
4. **`VSync`** (0x80087C24) - Important for understanding timing
5. **`PadRead`** (0x8008767C) - Controller input parsing

### Recommended Next Steps

1. **Phase 1: Symbol Naming** (No YAML changes)
   - Add all 559 LIBS function names to `symbol_addrs.txt`
   - Use Ghidra export script to get current names
   - Cross-reference with PSY-Q documentation

2. **Phase 2: Split Segments** (YAML change)
   - Split LIBS into per-library ASM segments
   - Better organization for future work

3. **Phase 3: Selective Decompilation** (C files)
   - Start with simple utility functions
   - Create matching C code for verification
   - Build library of PSY-Q patterns

---

## Implementation Checklist

### Phase 1: main() Extraction (Minimal)
- [ ] Update `SLES_010.90.yaml` - Add C segment for main at 0x730B0
- [ ] Create `src/Game/GAMELOOP/main.c` with INCLUDE_ASM stub
- [ ] Run `make clean && make check` to verify build
- [ ] Write decompiled C code for `main()`
- [ ] Achieve matching build

### Phase 2: BLB Accessors
- [ ] Create `src/Game/GAMELOOP/BLBAccessors.c`
- [ ] Decompile `GetLevelCount`, `getLevelName`, etc. (trivial functions)
- [ ] Update YAML with BLBAccessors segment

### Phase 3: Game State / Level Loading
- [ ] Create `src/Game/GAMELOOP/GameState.c`
- [ ] Decompile `InitGameState` (complex)
- [ ] Decompile `TickCDStreamBuffer`

### Phase 4: Debug Menu
- [ ] Create `src/Game/GAMELOOP/DebugMenu.c`
- [ ] Decompile `ProcessDebugMenuInput`
- [ ] Decompile stdlib functions (`strcmp`, `memcpy`, etc.)

### LIBS Extraction
- [ ] Export all 559 function names from Ghidra
- [ ] Add to `symbol_addrs.txt`
- [ ] Split LIBS segment in YAML by library (optional)
- [ ] Create C stubs for priority functions
- [ ] Decompile `__main` (simple CRT init)
- [ ] Decompile `ResetCallback` (simple wrapper)
- [ ] Decompile `DrawSync` (simple with debug output)

---

## Quick Start Commands

```bash
# After updating YAML and creating main.c:
cd /home/sam/projects/btm
make clean && make check

# If splat needs regeneration:
rm -rf asm/nonmatchings/Game/GAMELOOP/main
python3 -m splat split SLES_010.90.yaml

# To verify matching:
make check
sha1sum bin/SLES_010.90 build/SLES_010.90
```

---

## References

- [PSY-Q SDK Documentation](http://problemkaputt.de/psx-spx.htm)
- [soul-re decompilation patterns](https://github.com/Xeeynamo/sotn-decomp)
- Ghidra project: `skullmonkeys/SLES_010.90`
- Call graph generated via `mcp_ghidra_analysis_get_callgraph(address=0x800828B0, max_depth=2)`
