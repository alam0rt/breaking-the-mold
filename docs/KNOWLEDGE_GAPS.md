# Knowledge Gaps & Missing Understanding

**Last Updated**: January 19, 2026  
**Purpose**: Track what we still don't understand about Skullmonkeys

This document consolidates all known gaps in our understanding of the game's internals.
Items here have been verified as "unknown" after checking existing documentation.

---

## Critical Gaps (Block Full Understanding)

### 1. GameState Structure Corruption
**Location**: Ghidra struct `GameState` @ 0x8009DC40  
**Issue**: During LevelDataContext field addition, struct grew from 416 to 509 bytes  
**Impact**: Fields at offset 0x104+ may have incorrect offsets in Ghidra decompilation  
**Action Required**: Rebuild GameState struct in Ghidra with correct 416-byte size

**Verified Fields (from tracing session 2026-01-19):**
- 0x1C: tick_list_head
- 0x20: render_list_head  
- 0x44/0x46: camera_x/y
- 0x10C: frame_counter
- 0x11A: countdown_timer
- 0x134: checkpoint_entity_list
- 0x14C: hud_entity_ptr
- 0x150-0x160: pause/menu system
- 0x17C-0x18C: cheat_input_buffer

### 2. Unknown GameState Fields
**Location**: `docs/systems/gamestate-field-analysis.md`

| Offset Range | Status | Notes |
|--------------|--------|-------|
| ~~0x3C-0x43~~ | ✅ RESOLVED | `previous_spawn_list` (0x3C) + `blb_header_ptr` (0x40) |
| 0x7A | Unknown | In level/layer state region |
| 0x106 | Unknown | In render/frame state region |
| 0x11B | Unknown | Near screen shake |
| 0x127-0x129 | Unknown | In special mode region |
| 0x130-0x133 | Partial | bg_color_change fields need verification |
| 0x191-0x198 | Unknown | Between debug_pause and bg colors |

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
**Status**: ✅ MOSTLY RESOLVED (2026-01-20)  
**Known**: Boss entity types (71, 100-103), complete function inventory

**Resolution Summary (76 functions named):**
| Boss | Functions Named | Coverage |
|------|-----------------|----------|
| Shriney Guard | 15 | ~100% |
| Joe-Head-Joe | 23 | ~92% |
| Glenn Yntis | 8 | ~100% |
| Monkey Mage | 11 | ~92% |
| Klogg | 13 | ~87% |
| Shared | 5 | ~100% |

**Key Discoveries:**
- HP stored in `g_pPlayerState[0x1D]` (all bosses)
- Common event codes: 0x1001 (upper hit), 0x1002 (lower hit)
- VTables: Klogg=0x80011268, JHJ=0x80011288, Shriney=0x800112a8, Glenn=0x800112c8, MM=0x80011308
- Attack pattern table: Joe-Head-Joe @ 0x8009bb28 (90 bytes, 5 phases × 18 patterns)

**Still Unknown (minor):**
- Exact hitbox dimensions during attack phases
- Boss music coordination details
- Some intermediate state transitions

### 5. FINN/RUNN Vehicle Mechanics
**Location**: `docs/systems/player/player-finn.md`  
**Known**: Asset 504 path data format, vehicle entity types  
**Unknown**:
- How vehicle handles player input differently
- Rail grinding physics constants
- Auto-scroll speed calculations
- How path waypoints are followed

### 6. Sound Effect ID Mappings
**Location**: `docs/reference/sound-ids-complete.md`  
**Known**: ~35 sound IDs documented  
**Unknown**: 
- Complete mapping of all ~100+ sound effects
- Which sounds are CD audio vs SPU samples
- Sound priority/channel management

---

## Lower Priority Gaps (Nice to Have)

### 7. Entity Initialization Patterns
**Location**: `docs/entity-system.md`, `docs/systems/entities.md`  
**Known**: Factory pattern, 121 entity types, callback table  
**Unknown**:
- Full breakdown of all 91 InitEntity_* functions
- Which entity types share initialization code
- Entity memory layout variations by type

### 8. Palette Animation Runtime
**Location**: `docs/analysis/unconfirmed-findings.md`  
**Known**: Asset 401 format (4 bytes per palette: enabled, start, end, speed)  
**Unknown**:
- Which function processes palette animation each frame
- How animated palettes interact with entity rendering
- Performance impact of palette cycling

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

### 10. Multi-Layer Parallax Scrolling
**Location**: `docs/systems/rendering-order.md`, `docs/systems/camera.md`  
**Known**: 4 layer list types (static, scrolling, parallax, standard)  
**Unknown**:
- Exact parallax divisor per layer
- How layer priorities interleave with entity z-order
- Layer culling/visibility optimizations

---

## Recently Closed Gaps ✅

These were gaps that have been resolved:

| Gap | Resolution Date | Documentation |
|-----|-----------------|---------------|
| **Tile slope height table** | 2026-01-19 | `tile-collision-quick-ref.md`, `systems/collision-complete.md` |
| Tile collision attributes | 2026-01-15 | `systems/collision-complete.md` |
| Animation framework | 2026-01-15 | `systems/animation-framework.md` |
| Camera smooth scrolling | 2026-01-15 | `systems/camera.md` |
| Entity type identification | 2026-01-13 | `systems/entity-identification.md` |
| Cheat code buffer | 2026-01-19 | `systems/gamestate-field-analysis.md` |
| Checkpoint system | 2026-01-19 | `systems/checkpoint-system.md` |

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
