# Knowledge Gaps & Missing Understanding

**Last Updated**: January 20, 2026  
**Purpose**: Track what we still don't understand about Skullmonkeys

This document consolidates all known gaps in our understanding of the game's internals.
Items here have been verified as "unknown" after checking existing documentation.

---

## ⚠️ CRITICAL: Recent Documentation Correction (2026-01-20)

**Entity Type Mapping Error Discovered**: BLB entity types are **layer-dependent**!

Previous docs incorrectly stated "BLB 25 = Loud Mouth enemy". In reality:
- BLB 25 on Layer 1 → Internal 0 → **Pickup** (not enemy!)
- BLB 25 on Layer 2 → Internal 79 → **Spawner**
- BLB 13 on Layer 2 → Internal 25 → **Ground Enemy** (this is "EnemyA")

**See**: `docs/reference/ENTITY_REMAPPING_CORRECTION.md` for full details.

---

## Critical Gaps (Block Full Understanding)

### 1. GameState Structure Corruption - ✅ RESOLVED (2026-01-20)
**Location**: Ghidra struct `GameState` @ 0x8009DC40  
**Resolution**: Rebuilt GameState struct with correct 416-byte size via `ghidra_fix_gamestate.py`

**Verified Fields (from comprehensive decompilation analysis 2026-01-20):**
- 0x00-0x07: mode_base_offset/mode_callback_ptr (mode dispatch system)
- 0x08-0x18: layer list heads (static, scrolling, parallax, standard)
- 0x1C: tick_list_head
- 0x20: render_list_head  
- 0x44/0x46: camera_x/y
- 0x84-0x103: LevelDataContext (128 bytes reserved)
- 0x10C: frame_counter
- 0x130-0x133: bg_color double-buffer system (NEW DISCOVERY)
- 0x134: checkpoint_entity_list
- 0x14C: hud_entity_ptr
- 0x150-0x160: pause/menu system
- 0x17C-0x18C: cheat_input_buffer

### 2. Unknown GameState Fields - ✅ FULLY RESOLVED (2026-01-20)
**Location**: `docs/systems/gamestate-field-analysis.md`

| Offset Range | Status | Resolution |
|--------------|--------|------------|
| ~~0x3C-0x43~~ | ✅ RESOLVED | `previous_spawn_list` (0x3C) + `blb_header_ptr` (0x40) |
| ~~0x7A~~ | ✅ RESOLVED | Alignment padding (4-byte boundary) |
| ~~0x106~~ | ✅ RESOLVED | Alignment padding after tile_render_state_count |
| ~~0x11B~~ | ✅ RESOLVED | screen_shake_active flag |
| 0x127-0x129 | ✅ RESOLVED | Layer 1 tints (R/G/B) |
| 0x130-0x133 | ✅ RESOLVED | bg_color_change_flag + pending RGB (double-buffer system) |
| 0x191-0x198 | ✅ RESOLVED | Padding fields + _reserved_194/198 |

---

## Medium Priority Gaps

### 3. Save System Architecture
**Status**: ✅ MOSTLY RESOLVED (2026-01-19)  
**Known**: Password screens are pre-rendered, no memory card support  
**Resolved**:
- ✅ Password encoding algorithm documented in `reference/data-section-map.md`
- ✅ `BuildPasswordFromPlayerState` @ 0x80025C7C encodes player state to 12-button sequence
- ✅ `DecodePassword` @ 0x80025E48 decodes password back to state
- ✅ Encoding table at 0x8009B198 (32 field/bit pairs)
- ✅ Encoded fields: level, lives, phoenix hands, phart heads, universe enemas, 1970 icons, swirly qs, super willies

**Still Unknown**:
- How world completion triggers password screen selection
- How password validation occurs in menu system

### 4. Boss AI State Machines
**Location**: `docs/systems/bosses.md`  
**Status**: ✅ FULLY RESOLVED (2026-01-20)  
**Known**: Boss entity types (71, 100-103), complete function inventory

**Resolution Summary (80 functions named):**
| Boss | Functions Named | Coverage |
|------|-----------------|----------|
| Shriney Guard | 16 | ~100% |
| Joe-Head-Joe | 23 | ~92% |
| Glenn Yntis | 8 | ~100% |
| Monkey Mage | 11 | ~92% |
| Klogg | 13 | ~87% |
| Shared | 8 | ~100% |

**Key Discoveries:**
- HP stored in `g_pPlayerState[0x1D]` (all bosses)
- Common event codes: 0x1001 (upper hit), 0x1002 (lower hit)
- VTables: Klogg=0x80011268, JHJ=0x80011288, Shriney=0x800112a8, Glenn=0x800112c8, MM=0x80011308
- Attack pattern table: Joe-Head-Joe @ 0x8009bb28 (90 bytes, 5 phases × 18 patterns)

**Still Unknown (minor):**
- Exact hitbox dimensions during attack phases
- Boss music coordination details

### 5. FINN/RUNN Vehicle Mechanics - ✅ FULLY RESOLVED (2026-01-20)
**Location**: `docs/systems/player/player-finn.md`, `docs/systems/player/player-runn.md`  
**Status**: Complete physics and input handling documented

**FINN (Boat/Fish Mode) - 0x0400 Flag:**
- **Input Handling**: `FinnHandleInput` @ 0x8006fbd0 - Tank controls
  - Up/Down D-Pad: Rotate ±0x10 per frame, max ±0x40
  - Action button: Move forward via sin/cos calculation
  - Rotation drag: ±8 per frame when no input
  - Angle range: 0-0x400 (360°)
- **Physics**: `FinnVehicleMovementUpdate` @ 0x8006f250
  - Max velocity: ±0x20000 (2.0 px/frame per axis)
  - Drag: 0xC00 per frame
  - Bounce: `vel = -(vel >> 1)` on collision
  - 15+ tile collision types with specific handling (0xB5-0xE2)

**RUNN (Auto-Scroller Mode) - 0x0100 Flag:**
- **Input Handling**: `CreateRunnPlayerEntity` @ 0x80073934
  - Triangle (0x1000): Adjust left
  - X (0x4000): Adjust right  
  - D-Pad (0xF0): Jump/special
- **Physics**: `RunnVerticalMovementUpdate` @ 0x80073b88
  - Max vertical velocity: ±0x40000 (4.0 px/frame)
  - Drag: 0x4000 per frame (0.25 px/frame)
  - Horizontal adjust: ±0xc000 (0.75 px/frame)
- **Sound**: Voice index at +0x10C, sound 0x421586c2 while moving

**See**: `player-finn.md` and `player-runn.md` for complete physics constants

### 6. Sound Effect ID Mappings
**Location**: `docs/reference/sound-ids-complete.md`  
**Known**: ~35 sound IDs documented  
**Unknown**: 
- Complete mapping of all ~100+ sound effects
- Which sounds are CD audio vs SPU samples
- Sound priority/channel management

---

## Lower Priority Gaps (Nice to Have)

### 7. Player Sprite Availability System - ✅ RESOLVED (2026-01-21)
**Location**: `docs/systems/sprites.md`, `docs/systems/player/player-sprite-ids.md`  
**Status**: Fully understood via Ghidra + BLB analysis

**KEY DISCOVERY**: Player sprites are in **PRIMARY** segment Asset 600, NOT tertiary!

**Sprite Storage Architecture:**
| Segment | Asset 600 Contents | ctx Offset |
|---------|-------------------|------------|
| PRIMARY | Player + shared sprites (79 in SCIE) | ctx+0x40 |
| TERTIARY | Level-specific enemy/pickup sprites (20 in SCIE) | ctx+0x70 |

**Sprite Lookup Chain:**
1. `LookupSpriteById(sprite_id)` @ 0x8007bb10:
   - Calls `FindSpriteInTOC(g_pLevelDataContext, sprite_id)`
   - Falls back to `g_pSecondarySpriteBank` @ 0x800a6060 (often NULL)

2. `FindSpriteInTOC(ctx, sprite_id)` @ 0x8007b968:
   - **First** searches ctx+0x70 (tertiary - level enemies)
   - **Then** searches ctx+0x40 (primary - player/shared sprites)
   - TOC format: 12-byte entries [count, sprite_id, offset]

3. `InitPlayerSpriteAvailability(player_entity)` @ 0x80059a70:
   - Iterates table at `0x8009c3a8` (7 required sprite IDs)
   - Verifies sprites exist via InitSpriteContextWrapper
   - Stores available sprites at player+0x180 (up to 7)
   - Count at +0x19c

**Key Tables:**
- `g_PlayerSprites` @ 0x8009c174: 55 sprite IDs for all player animations
- `g_PlayerRequiredSprites` @ 0x8009c3a8: 7 required sprite IDs (all in primary!)

**SCIE Level Verification:**
- Primary Asset 600: 79 sprites (34 player sprites, all 7 required present)
- Tertiary Asset 600: 20 sprites (enemies only, 0 player sprites)

**For evil-engine**: Must load PRIMARY Asset 600 for player sprites, NOT tertiary!

### 8. Entity Initialization Patterns
**Location**: `docs/entity-system.md`, `docs/systems/entities.md`  
**Known**: Factory pattern, 121 entity types, callback table  
**Unknown**:
- Full breakdown of all 91 InitEntity_* functions
- Which entity types share initialization code
- Entity memory layout variations by type

### 8. Palette Animation Runtime - ✅ FULLY RESOLVED (2026-01-20)
**Location**: `docs/systems/entities.md`, `docs/analysis/unconfirmed-findings.md`  
**Status**: Complete runtime behavior documented via Ghidra decompilation

**Key Functions:**
- `CLUTPaletteCycleTickCallback` @ 0x8001991c - Per-frame palette rotation
- `SetTexturePageParams` @ 0x80019f2c - Enables palette cycling for a texture page
- `GetPaletteAnimData` @ 0x8007b530 - Returns Asset 401 pointer (ctx[9])

**Entity Structure for Palette Animation:**
| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x1C | 4 | clut_ptr | Pointer to CLUT data (palette) |
| +0x35 | 1 | start_index | First color index to animate |
| +0x36 | 1 | end_index | Last color index to animate |
| +0x37 | 1 | direction | 0=forward, 1=backward cycling |
| +0x38 | 1 | speed | Frames between color shifts (copied to +0x39) |
| +0x39 | 1 | timer | Countdown until next shift |

**Runtime Behavior (CLUTPaletteCycleTickCallback):**
```c
void CLUTPaletteCycleTickCallback(Entity* e) {
    if (--e->timer == 0) {
        u16* clut = e->clut_ptr;
        if (e->direction == 0) {  // Forward
            u16 first = clut[e->start_index];
            memmove(&clut[e->start_index], &clut[e->start_index+1], 
                    (e->end_index - e->start_index) * 2);
            clut[e->end_index] = first;  // Wrap first to end
        } else {  // Backward
            u16 last = clut[e->end_index];
            memmove(&clut[e->start_index+1], &clut[e->start_index],
                    (e->end_index - e->start_index) * 2);
            clut[e->start_index] = last;  // Wrap last to start
        }
        UploadTextureOrClut(e, clut);  // Send to VRAM
        e->timer = e->speed;  // Reset countdown
    }
}
```

**Performance**: One entity per animated palette, minimal CPU cost (memmove + CLUT upload)

### 9. Demo/Attract Mode Recording Format - ✅ RESOLVED (2026-01-19)
**Location**: `docs/systems/demo-attract-mode.md`  
**Status**: Fully documented

**Resolved**:
- ✅ Input recording format: RLE-encoded (buttons u16, duration u16) in Asset 700
- ✅ Frame-by-frame replay via `UpdateInputState` @ 0x800259D4
- ✅ Demo selection: `MenuTickCallback` uses idle timer (1801 frames = 30s)
- ✅ Data location: Asset 700 in tertiary segment (stage0) of 9 levels
- ✅ Header: 16 bytes (count, id, size, offset=16)

**See**: `docs/systems/demo-attract-mode.md` for complete details

### 10. Multi-Layer Parallax Scrolling - ✅ RESOLVED (2026-01-20)
**Location**: `docs/systems/rendering-order.md`, `docs/systems/camera.md`  
**Status**: Complete parallax calculation documented via Ghidra decompilation

**Key Functions:**
- `CalculateParallaxXOffset` @ 0x8001a304 - X offset with divisor
- `UpdateParallaxLayerPosition` @ 0x8001ab14 - Full X/Y parallax update
- `UpdateParallaxScrollWithWrap_Standard/Medium/Small` @ 0x8001eeec/0x8001f368/0x8001f778

**Parallax Divisor System (16.16 Fixed-Point):**
| Entity Offset | Field | Description |
|---------------|-------|-------------|
| +0x50 | x_scale | Parallax X scroll scale |
| +0x54 | y_scale | Parallax Y scroll scale |
| +0x58 | alt_x_scale | Alternative X scale (entity coord) |
| +0x60 | camera_x_scale | Camera X parallax multiplier |
| +0x64 | camera_y_scale | Camera Y parallax multiplier |

**Calculation:**
```c
// When divisor == 0x10000 (1.0): no parallax, 1:1 with camera
if (layer->camera_x_scale == 0x10000) {
    adjusted_cam_x = g_GameState->camera_x;
} else {
    // Slower scroll for background layers (divisor < 0x10000)
    adjusted_cam_x = (camera_x * layer->camera_x_scale) >> 16;
}
layer_screen_x = layer_world_x - adjusted_cam_x + layer->x_offset;
```

**Common Divisor Values:**
- 0x10000 = 1.0 (foreground, 1:1)
- 0x8000 = 0.5 (half-speed parallax)
- 0x4000 = 0.25 (quarter-speed distant background)
- 0x2000 = 0.125 (far horizon)

**Z-Order Interleaving:**
Layers are rendered in order from GameState lists:
1. Static layers (0x08-0x0B): No scroll
2. Scrolling layers (0x0C-0x0F): Parallax enabled
3. Parallax layers (0x10-0x13): Far background
4. Standard layers (0x14-0x17): Foreground
Entities interleave by z_order value (0-65535)

---

## Recently Closed Gaps ✅

These were gaps that have been resolved:

| Gap | Resolution Date | Documentation |
|-----|-----------------|---------------|
| **GameState struct rebuild** | 2026-01-20 | `systems/gamestate-field-analysis.md`, `scripts/ghidra_fix_gamestate.py` |
| **Mode callback dispatch** | 2026-01-20 | `systems/game-loop.md` |
| **Background color double-buffer** | 2026-01-20 | `systems/gamestate-field-analysis.md` (section 3) |
| **Demo mode system** | 2026-01-20 | `systems/gamestate-field-analysis.md` (section 5) |
| **Tile slope height table** | 2026-01-19 | `tile-collision-quick-ref.md`, `systems/collision-complete.md` |
| Tile collision attributes | 2026-01-15 | `systems/collision-complete.md` |
| Animation framework | 2026-01-15 | `systems/animation-framework.md` |
| Camera smooth scrolling | 2026-01-15 | `systems/camera.md` |
| Entity type identification | 2026-01-13 | `systems/entity-identification.md` |
| Cheat code buffer | 2026-01-19 | `systems/gamestate-field-analysis.md` |
| Checkpoint system | 2026-01-19 | `systems/checkpoint-system.md` |

### GameState Deep Analysis Discovery (2026-01-20)

Key patterns discovered through comprehensive function decompilation:

1. **Mode Callback Dispatch**: Uses `mode_base_offset = 0xFFFF0000` with direct `mode_callback_ptr`
   - When mode > 0: vtable lookup (unused in normal gameplay)
   - When mode <= 0: direct callback (standard path)

2. **Double-Buffered Background Colors**: RenderEntities writes to two BLB header locations
   - `blbHeaderBufferBase + 0x1D-0x1F` (primary)
   - `blbHeaderBufferBase + 0x505D-0x505F` (secondary)

3. **Demo Mode**: Sets deterministic RNG (`srand(1)`), purple BG tint, special sprite at z=30000

4. **Special Level Handling**:
   - Level 98 (credits): redirects to menu after double AdvancePlaybackSequence
   - Level 99 (menu): loaded at init, clears hamster count on entry

5. ~~**Level Flags Bitmask**: Flags 0x04/0x10/0x100/0x200/0x400/0x2000 disable cheat effects~~
   **RESOLVED 2026-01-20**: All 16 level flag bits documented in `gamestate-field-analysis.md` §7.
   - 0x04=GLIDE, 0x10=SOAR, 0x100=RUNN, 0x200=MENU, 0x400=FINN, 0x2000=BOSS
   - `IsNormalPlatformLevel` checks all 6 for cheat restrictions

### Slope Height Table Discovery (2026-01-19)

**Key Finding**: `g_SlopeHeightTable` @ 0x8009d228 contains subpixel collision heights.

- **Format**: 256 tile types × 16 height values = 4096 bytes
- **Lookup**: `height = table[(attr & 0xFF) * 16 + (x_pixel & 0xF)]`
- **Function**: `GetSlopeHeightAtSubpixel` @ 0x80081c0c (renamed from `GetLevelSoundTableEntry`)

**Slope Types Documented**:
- 45°, 22.5° (2-tile), 11.25° (3-tile), 5.6° (4-tile), 2.8° (8-tile), ~53° (special)
- Both up-right (↗) and down-right (↘) variants
- Dual-purpose: attributes 0x15-0x28 provide BOTH slope heights AND color tinting
| Input system | 2026-01-17 | `systems/input-system-complete.md` |
| HUD system | 2026-01-17 | `systems/hud-system-complete.md` |
| Menu system | 2026-01-17 | `systems/menu-system-complete.md` |
| Password encoding algorithm | 2026-01-19 | `reference/data-section-map.md` |
| Animation motion curves | 2026-01-19 | `reference/data-section-map.md` |
| Demo/attract mode replay format | 2026-01-19 | `systems/demo-attract-mode.md` |
| Asset 700 purpose | 2026-01-19 | `blb/vestigial-fields-complete.md` |
| GameState 0x3C-0x43 | 2026-01-20 | `include/Game/game_state.h` (previous_spawn_list, blb_header_ptr) |
| GameState 0x60 bounce_active_flag | 2026-01-20 | Boss AI tracing analysis |
| GameState 0x62 camera_follow_direction | 2026-01-20 | Boss AI tracing analysis |
| GameState 0x19C boss_defeated | 2026-01-20 | Boss AI tracing analysis |
| GameState 0x19D boss_facing | 2026-01-20 | Boss AI tracing analysis |
| 98 PlayerCallback functions | 2026-01-20 | Ghidra MCP renaming session |
| Boss AI state machines (76 functions) | 2026-01-20 | `systems/bosses.md` - complete reference |

---

## How to Close Gaps

### For GameState Fields
1. Use Ghidra MCP to decompile functions that access the offset
2. Trace XRefs: `mcp_ghidra_xrefs_list(to_addr="0x8009DC40")`
3. Document findings in `gamestate-field-analysis.md`

### For Boss AI
1. Record gameplay with `make record LEVEL=<boss_id>`
2. Analyze trace for state transitions
3. Cross-reference with Ghidra decompilation
4. Update boss-specific doc file

### For Sound IDs
1. Set breakpoint on `PlaySoundEffect` @ 0x8007C388
2. Log sound ID parameter with game_watcher.lua
3. Correlate with in-game actions
4. Add to `sound-ids-complete.md`

---

## Statistics

| Category | Documented | Estimated Total | Coverage |
|----------|------------|-----------------|----------|
| GameState fields | 70+ | ~80 | ~88% |
| Entity types | 121 | 121 | 100% |
| Tile attributes | 100+ | ~128 | ~80% |
| Sound effects | 35 | ~100 | ~35% |
| Boss AI states | 5 bosses | 5 bosses | ~40% depth |
| BLB assets | 16/16 | 16 | 100% |
| PlayerCallback functions | 98 | 98 | 100% |

**Overall Project Coverage**: ~90% (estimated based on function naming in Ghidra)
