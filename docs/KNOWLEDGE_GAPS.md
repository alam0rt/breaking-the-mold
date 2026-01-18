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
| 0x3C-0x43 | Unknown | Between deferred removal and camera |
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
**Location**: `docs/systems/boss-ai/`  
**Known**: Boss entity types (49-51), basic attack patterns  
**Unknown**:
- Complete state machine transitions for each boss
- Boss damage thresholds and phase triggers
- Boss hitbox dimensions during each attack phase
- How boss music/audio is coordinated with phases

**Bosses Needing Analysis**:
| Boss | File | Completeness |
|------|------|--------------|
| Shriney Guard | boss-shriney-guard.md | ~40% (estimated) |
| Joe-Head-Joe | boss-behaviors.md | ~60% (some tracing) |
| Glenn Yntis | boss-glenn-yntis.md | ~30% (estimated) |
| Monkey Mage | boss-monkey-mage.md | ~30% (estimated) |
| Klogg | boss-klogg.md | ~50% (recent analysis) |

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
| Tile collision attributes | 2026-01-15 | `systems/collision-complete.md` |
| Animation framework | 2026-01-15 | `systems/animation-framework.md` |
| Camera smooth scrolling | 2026-01-15 | `systems/camera.md` |
| Entity type identification | 2026-01-13 | `systems/entity-identification.md` |
| Cheat code buffer | 2026-01-19 | `systems/gamestate-field-analysis.md` |
| Checkpoint system | 2026-01-19 | `systems/checkpoint-system.md` |
| Input system | 2026-01-17 | `systems/input-system-complete.md` |
| HUD system | 2026-01-17 | `systems/hud-system-complete.md` |
| Menu system | 2026-01-17 | `systems/menu-system-complete.md` |
| Password encoding algorithm | 2026-01-19 | `reference/data-section-map.md` |
| Animation motion curves | 2026-01-19 | `reference/data-section-map.md` |
| Demo/attract mode replay format | 2026-01-19 | `systems/demo-attract-mode.md` |
| Asset 700 purpose | 2026-01-19 | `blb/vestigial-fields-complete.md` |

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
| GameState fields | 60+ | ~80 | ~75% |
| Entity types | 121 | 121 | 100% |
| Tile attributes | 100+ | ~128 | ~80% |
| Sound effects | 35 | ~100 | ~35% |
| Boss AI states | 5 bosses | 5 bosses | ~40% depth |
| BLB assets | 16/16 | 16 | 100% |

**Overall Project Coverage**: ~85% (estimated based on function naming in Ghidra)
