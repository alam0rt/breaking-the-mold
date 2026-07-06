---
title: "Unconfirmed Findings"
category: analysis
tags: [analysis, unconfirmed, findings]
---

# Unconfirmed Findings

This document contains observations and guesses about the Skullmonkeys data structures
that have not been fully verified through decompilation or runtime tracing.

## Risk Classification

Each unconfirmed section is tagged with a decomp risk level. Use these when
planning work — see [`../decompilation-guide.md`](../decompilation-guide.md) for how
these gate each Tier.

| Tag | Meaning | Policy |
|-----|---------|--------|
| 🔴 **BLOCKER** | Wrong assumption will break byte-match or produce garbage output | Must be resolved before the dependent Tier starts |
| 🟡 **CORRECTNESS** | Wrong assumption produces wrong runtime behavior but still compiles | Must be resolved before shipping the feature; may land with a TODO |
| 🟢 **POLISH** | Cosmetic / non-functional (visual effects, unused fields, rarely-hit paths) | Can ship with a TODO comment; revisit later |
| ✅ **VERIFIED / CONFIRMED** | Proven via Ghidra decomp or runtime trace | No action; treat as fact |

**Quick index of currently-unverified items:**

| Section | Tag | Blocks |
|---------|-----|--------|
| Entity Layer Field upper-byte flags (line 180) | 🟡 CORRECTNESS | Entity rendering for types 9, 81 |
| TileHeader field_18 bits 3, 6, 12 (line 167) | 🟡 CORRECTNESS | Level-mode selection |
| TileHeader field_20 (line 172) | 🟢 POLISH | Possibly visual/music variation |
| Asset 504 entry type semantics (line 404) | 🟢 POLISH | FINN/RUNN vehicle paths only |
| Stage completion container field meanings (line 308) | 🟢 POLISH | Victory screens only |
| Tentative Asset 600 entry structure details (line 622) | 🟡 CORRECTNESS | Sprite lookup |

---

## Asset Pattern Discoveries (2026-01-10) - VERIFIED ✓

Analysis of extracted BLB assets revealed several new patterns:

### Asset 602: Per-Sample Audio Settings - CONFIRMED ✓

**Pattern**: Size = Asset 601 sample_count × 4 bytes
**Verification**: 91/91 secondary segments match exactly

Structure (4 bytes per sample):
```
Offset  Size  Field
0x00    u16   volume_left   (0-16383, where 16383 = max)
0x02    u16   volume_right  (usually 0, sometimes contains flags)
```

Common values:
- 16383 (0x3FFF) = Maximum volume
- 8192 (0x2000) = 50% volume
- 4915 = ~30% volume

### Asset 401: Per-Palette Animation Config - CONFIRMED ✓

**Pattern**: Size = Asset 400 palette_count × 4 bytes
**Verification**: 104/104 secondary segments match exactly

Structure (4 bytes per palette):
```
Offset  Size  Field
0x00    u8    enabled (0=static, 1=animate)
0x01    u8    start_index (first color to cycle)
0x02    u8    end_index (last color to cycle)
0x03    u8    speed (animation rate, higher=faster)
```

This implements classic palette color cycling animation. When enabled,
colors from start_index to end_index rotate each frame. Used for:
- Flowing water/lava effects
- Glowing/pulsing objects
- Animated backgrounds

Common patterns:
- (1, 2, 253, 1) - Cycle almost full palette slowly
- (1, 216, 253, 2) - Cycle last 40 colors at medium speed
- (1, 17, 253, 1) - Skip first 17 colors, cycle rest

### Asset 101: VRAM Slot Configuration (12 bytes) - CONFIRMED ✓

**VERIFIED 2026-01-20** via Ghidra (GetAsset101Entry @ 0x8007b3fc, InitVRAMSlotTable @ 0x80013b1c)

Controls texture page slot allocation for the level's sprites/tiles.
Only present in 56 segments across ~14 levels that need custom VRAM layouts.

**Structure (12 bytes):**
```
Offset  Size  Field
0x00    u32   bank_a_count  (texture slots in upper VRAM at y=0xF0, values 1-4)
0x04    u32   bank_b_count  (texture slots in lower VRAM at y=0x1F0, values 0-1)
0x08    u32   reserved      (always 0)
```

**Runtime Behavior:**
- `GetAsset101Entry(ctx, 0)` returns bank_a_count
- `GetAsset101Entry(ctx, 1)` returns bank_b_count
- `InitVRAMSlotTable` creates `(bank_a_count + bank_b_count) * 3` texture page slots
- When Asset 101 is absent, defaults to bank_a=0, bank_b=2 (6 slots)

**Observed Values (from extracted data):**
| bank_a | bank_b | Total Slots | Count | Levels |
|--------|--------|-------------|-------|--------|
| 1 | 1 | 6 | 2 | HEAD (boss level) |
| 2 | 0 | 6 | 38 | Most levels with custom VRAM |
| 2 | 1 | 9 | 4 | FOOD (special rendering) |
| 3 | 0 | 9 | 2 | EVIL stage1/secondary1 |
| 4 | 0 | 12 | 10 | BOIL, EGGS, MEGA (complex levels) |

**Levels WITHOUT Asset 101 (use default 6 slots):**
CAVE, CRYS, CSTL, GLEN, KLOG, MENU, PHRO, RUNN, SEVN, SOAR, TMPL, WIZZ

### Asset 502: VRAM Rectangles (16-byte entries) - CONFIRMED ✓

**Relationship**: Count matches TileHeader.field_1c (offset 0x1C) exactly for all levels

Structure:
```
Offset  Size  Field
0x00    u16   x1 (left edge in pixels)
0x02    u16   y1 (top edge in pixels)
0x04    u16   x2 (right edge in pixels)
0x06    u16   y2 (bottom edge in pixels)
0x08    u16   rect_type (usually 2, sometimes 4 or 21)
0x0A    6     padding (zeros)
```

Uses: Screen regions for triggers, texture page boundaries, collision zones

### Asset 503: Animation Offset Table (ToolX sequence data) - CONFIRMED ✓

Structure:
- u32 count (number of animation entries)
- TOC entries (12 bytes each): index(u32), data_size(u32), data_offset(u32)
- Variable-length frame data sections follow TOC

Each animation contains frame offset pairs for sprite animation sequences.

### Asset 504: Vehicle Path Data (FINN/RUNN only) - CONFIRMED ✓

**Pattern**: 64-byte entries containing path waypoints
- FINN: 78 entries (4992 bytes) - complex rail path
- RUNN: 1 entry (64 bytes) - simple path

Contains bounding boxes and navigation waypoint data.

---

## Cross-Asset Relationships (VERIFIED 2026-01-10)

Analysis of assets with matching occurrence counts revealed structural relationships:

### Group 1: 208 assets (100, 302, 401)
All three appear in every stage location:
- **302.size = 100.total_tiles** (1 byte of flags per tile) ✓
- **401.size = 400.palette_count × 4** (4 bytes per palette for animation config) ✓

### Group 2: 117 assets (601, 602)
Audio-related, always appear together:
- **602.size = 601.sample_count × 4** (4 bytes per audio sample for volume settings) ✓

### Group 3: 104 assets (200, 201, 300, 301, 400, 501)
Core level data, split between segment types:
- **stage segments**: 200 (tilemaps), 201 (layers), 501 (entities)
- **secondary segments**: 300 (tile pixels), 301 (palette indices), 400 (palettes)

### Group 4: 94 assets (502, 503)
Both in stage segments, 93 of 94 locations shared:
- **502.count = 100.field_1c** (TileHeader stores VRAM rect count) ✓
- **501.size/24 = 100.field_1e** (TileHeader stores entity count) ✓

### TileHeader Unknown Fields Now Identified (Updated 2026-01-11):

**Verified Fields (Ghidra decompilation confirmed):**
- **field_16 (0x16)** = Vehicle waypoint count (matches Asset 504 entry count)
  - FINN = 78 waypoints, RUNN = 1 waypoint, all others = 0
- **field_1a (0x1A)** = Special level ID (VERIFIED 2026-01-11)
  - Value 99 only in FINN and SEVN (special gameplay modes)
  - Correlates with level_flags bit 1 (0x02)
- **field_1c (0x1C)** = VRAM rectangle count
  - Read by GetAsset100Field1C @ 0x8007b7c8
  - Stored at GameState+0x78 by InitLayersAndTileState @ 0x80024778
  - Matches Asset 502 entry count exactly for all levels
- **field_1e (0x1E)** = Entity count
  - Read by GetEntityCount @ 0x8007b7a8
  - Used by LoadEntitiesFromAsset501 @ 0x80024dc4 as loop bound
  - Matches Asset 501 size / 24 exactly for all levels

**Tentative Fields (Patterns observed, no Ghidra accessor found):** 🟡 CORRECTNESS for field_18, 🟢 POLISH for field_20
- **field_18 (0x18)** = Level flags bitfield (updated analysis 2026-01-11)
  - Bit 1 (0x02): SEVN, FINN - special gameplay mode
  - Bit 2 (0x04): FINN only - FlynnBoy-specific
  - Bit 3 (0x08): EGGS, GLID, WEED, HEAD, MEGA, TMPL, FOOD
  - Bit 4 (0x10): RUNN only - runner vehicle mode
  - Bit 6 (0x40): WIZZ, HEAD, RUNN, GLEN, MEGA, TMPL, FOOD - boss/special?
  - Bit 12 (0x1000): PHRO, MEGA - possibly world 1
  - Full value list: BOIL=0, BRG1=0, CAVE=0, PHRO=0x1000, FOOD=0x8048, etc.
- **field_20 (0x20)** = Unknown (values 0-6 observed)
  - No Ghidra accessor function found
  - Boss/special levels always = 0
  - Regular levels vary per-stage within same level
  - Possibly visual effect or music variation index

---

## Entity Layer Field Discovery (2026-01-11) - PARTIALLY VERIFIED 🟡 CORRECTNESS

**Risk:** Low byte (layer 1/2/3) is verified and used by `RemapEntityTypesForLevel`.
Upper-byte flag semantics remain unverified — affects entity types 9 and 81 in CSTL.
Safe to decomp entity spawn with `layer = raw & 0xFF`, but render/grouping behavior
for the high byte will need a follow-up pass.

The Entity structure's layer field (offset 0x14) is NOT just a simple layer index.

**Discovery:** CSTL level entities have layer values like 259, 515, 771, 7169, 11778, 62209.

**Analysis:**
- Lower byte (bits 0-7) = actual render layer (1, 2, or 3)
- Upper byte (bits 8-15) = render flags or z-order modifiers

**Examples from CSTL stage0:**
| Raw Value | Hex | Layer | Flags | Entity Types |
|-----------|-----|-------|-------|--------------|
| 259 | 0x0103 | 3 | 0x01 | various |
| 515 | 0x0203 | 3 | 0x02 | various |
| 7169 | 0x1C01 | 1 | 0x1C | type 81 |
| 11778 | 0x2E02 | 2 | 0x2E | type 9 |
| 62209 | 0xF301 | 1 | 0xF3 | type 81 |

**Affected entity types:** Type 9 and Type 81 primarily use extended layer values.

**VERIFIED 2026-01-12:** Entity z_order is HARDCODED per entity type in InitEntitySprite calls.
The layer field from Asset 501 is used for collision/logic grouping, NOT for render z-ordering.
Entity z_order values observed:
- Player: 10000
- UI/HUD: 10000
- Particles: 959
- General entities: ~1000-1001

The upper byte flags purpose remains UNVERIFIED. May affect transparency, visibility, or
special rendering modes but does NOT directly set z_order.

See `/docs/systems/rendering-order.md` for full analysis.

---

## Tilemap Tile Index 0: Empty Tile

Tile index 0 is used as a transparent/empty placeholder tile.
It is NOT counted in TileHeader tile totals, which explains why:
  unique_tiles_used = total_tiles_defined + 1

---

## Hidden/Unused Content Discovery (2026-01-06) - VERIFIED ✓

Discovered unreferenced sectors in GAME.BLB containing hidden content that is
never loaded by the game. Images were successfully decoded using jpsxdec.

### Summary

| Range | Sectors | Offset | Size | Content |
|-------|---------|--------|------|---------|
| 1 | 2-11 | 0x1000 | 20 KB | **Unused Legal/Copyright Screen** |
| 2 | 26-196 | 0xD000 | 342 KB | **Unused Credits Slideshow (10 frames)** |
| 3 | 36929-37099 | 0x4820800 | 342 KB | **Duplicate of range 2** (build artifact) |

**Total unreferenced: 704 KB (0.9% of file)**
**File coverage: 99.1%**

### Range 1: Unused Legal/Copyright Screen (Sectors 2-11) ✓ DECODED

- **Format**: BS/MDEC v2 compressed image
- **Frame size**: 13,216 words (26,432 bytes data)
- **Quant scale**: 7 (moderate quality)
- **Dimensions**: 320×256 pixels
- **Content**: Legal/copyright information screen
- **Status**: NOT referenced by any game code - completely unused

This appears to be an alternate or earlier version of the legal screen. The game
uses a different LEGL asset at sector 22 instead. This version may have been
replaced during development but never removed from the disc.

Header bytes: `A0 33 00 38 07 00 02 00`

### Range 2: Unused Credits Slideshow (Sectors 26-196) ✓ DECODED

Contains 10 individual BS/MDEC compressed frames forming a **complete credits sequence**
that is never shown in the game!

| Frame | Offset | Words | Bytes | Quant | Content |
|-------|--------|-------|-------|-------|---------|
| 0 | 0xD0F4 | 12,576 | 25,152 | 4 | Credits title/intro |
| 1 | 0x15DB8 | 11,648 | 23,296 | 4 | Credits slide |
| 2 | 0x1EEF0 | 11,744 | 23,488 | 2 | Credits slide |
| 3 | 0x281FC | 10,752 | 21,504 | 3 | Credits slide |
| 4 | 0x30764 | 13,056 | 26,112 | 3 | Credits slide |
| 5 | 0x39964 | 12,416 | 24,832 | 4 | Credits slide |
| 6 | 0x429A0 | 10,624 | 21,248 | 3 | Credits slide |
| 7 | 0x4A9CC | 12,288 | 24,576 | 2 | Credits slide |
| 8 | 0x52F78 | 10,464 | 20,928 | 3 | Credits slide |
| 9 | 0x5AF44 | 11,040 | 22,080 | 2 | Credits slide |

**This is cut content!** The game has a different credits implementation that was
used instead. These slides represent an earlier or alternate credits sequence that
was never integrated into the final game.

### Range 3: Duplicate Data (Sectors 36929-37099)

- **Content**: BYTE-FOR-BYTE identical to Range 2 (the credits slideshow)
- **Theory**: Build artifact from disc mastering process

### Decoding Instructions

```bash
# Extract and decode BS frames using jpsxdec
nix-shell -p jpsxdec --run "jpsxdec -f <file.bs> -static bs -dim 320x256 -fmt png"

# Example: Extract sector 2-11 (legal screen)
python3 -c "
with open('disks/blb/GAME.BLB', 'rb') as f:
    f.seek(0x1000)
    open('/tmp/legal.bs', 'wb').write(f.read(20480))
"
nix-shell -p jpsxdec --run "jpsxdec -f /tmp/legal.bs -static bs -dim 320x256 -fmt png"
```

### Significance

This discovery reveals that Skullmonkeys contains **unused developer content**:
1. An alternate legal screen that was replaced
2. A complete 10-frame credits slideshow that was cut from the final game

These assets occupy ~700 KB and were likely left in during the mastering process.
Comparing with the NTSC version could reveal if these were region-specific cuts.

---

## Stage Completion Screens Discovery (2026-01-06) - VERIFIED ✓

The 16 "gap containers" between levels are **world completion password screens**.
Skullmonkeys has no memory card support, so these screens display the password
needed to return to a specific world.

### All 16 Password/Completion Screens

| # | Offset | BG Color | Content |
|---|--------|----------|---------|
| 1 | 0x00EB7000 | Gray (122,122,142) | Password screen - World 1 complete |
| 2 | 0x01355000 | Gray (130,130,142) | Password screen - World 2 complete |
| 3 | 0x0173E800 | Magenta (150,0,106) | Password screen - World 3 complete |
| 4 | 0x01AFB800 | Magenta (150,0,106) | Password screen - World 4 complete |
| 5 | 0x01ED8800 | Magenta (150,0,106) | Password screen - World 5 complete |
| 6 | 0x02297800 | Magenta (150,0,106) | Password screen - World 6 complete |
| 7 | 0x025AB000 | Magenta (150,0,106) | Password screen - World 7 complete |
| 8 | 0x02880800 | Magenta (150,0,106) | Password screen - World 8 complete |
| 9 | 0x02D1B000 | Magenta (150,0,106) | Password screen - World 9 complete |
| 10 | 0x032D7000 | Magenta (150,0,106) | Password screen - World 10 complete |
| 11 | 0x0357A800 | Magenta (150,0,106) | Password screen - World 11 complete |
| 12 | 0x0397F800 | Magenta (150,0,106) | Password screen - World 12 complete |
| 13 | 0x03E93000 | Magenta (150,0,106) | Password screen - World 13 complete |
| 14 | 0x03FFC800 | Magenta (150,0,106) | Password screen - World 14 complete |
| 15 | 0x044AA800 | Magenta (150,0,106) | Password screen - World 15 complete |
| 16 | 0x047DC800 | Magenta (150,0,106) | **"YOU WIN" final completion screen** |

### Container Format

Each container has the standard 11-asset level segment structure:
- Asset 100: Tile header (36 bytes) - 30×16 level dimensions
- Asset 200: Tilemap container (5 tilemaps)
- Asset 201: Layer entries (5 layers)
- Asset 300: Tile pixels (8bpp, ~540 tiles each)
- Asset 301: Palette indices per tile
- Asset 302: Tile size flags
- Asset 400: Palette container (4 × 256-color palettes)
- Asset 401: Animation config
- Asset 600: Tile graphics
- Asset 601: Audio sample bank (SPU ADPCM)
- Asset 602: Raw palette (1 × 16-color CLUT) / Audio volume-pan

### Rendering

The main background uses **tilemap 3** (320 tiles = 20×16 grid), rendered at 320×256 pixels.

```bash
# Rendered images saved to:
ls /tmp/gap_containers/
# gap_01_FINN_to_BOIL.png through gap_16_EVIL_to_END.png
```

### Notes

- Gaps 1-2 use gray backgrounds (early game theme)
- Gaps 3-16 all share magenta `#96006A` (main game theme)
- Gap 16 is largest (675 tiles) - contains the elaborate "YOU WIN" victory screen
- These are NOT in the level metadata table - loaded via separate code path
- Password screen loading likely triggered by world completion event

---

### Asset Type Coverage (within referenced data)

| Status | Bytes | % | Description |
|--------|-------|---|-------------|
| ✓ Parsed | 3.8 MB | 5% | Can extract and view (sprites, palettes, metadata) |
| ◐ Partial | 9.0 MB | 12% | Structure known but not fully verified (tiles, audio) |
| ◐ Primary | 20.8 MB | 27% | Raw level data (partial understanding) |
| ✗ Unknown | 0.2 MB | 0.3% | Format not understood |

### Known Asset Types

| ID | Name | Status | Notes |
|----|------|--------|-------|
| 100 | Level Metadata | Parsed | 36-byte config per level |
| 400 | Sprite Palettes | Parsed | 256-color CLUT container |
| 600 | RLE Sprites | Parsed | RLE-encoded sprite container |
| 601 | Audio Samples | Verified | ADPCM audio uploaded to SPU RAM |
| 602 | Audio Metadata | Verified | Per-sample parameters (volume, channel, etc.) |
| 200 | Background Tiles | Partial | Large graphics data |
| 300 | Tile/Collision Data | Partial | Similar to 200 |
| 500 | Audio Data | Partial | Sound effects or music |

### Unknown Asset Types (priority investigation targets)

| ID | Size | Occurrences | Notes |
|----|------|-------------|-------|
| 501 | varies | all levels | **VERIFIED: Entity placement data (24-byte structs)** |
| 502/503 | varies | varies | Audio/animation metadata |
| 201/301/302 | 94 KB | 104 | Possibly tile/layer metadata |
| 504 | varies | 2 levels | **Vehicle level data (FINN, RUNN only)** - see below |
| 700 | 3 KB | 9 | Small entries, purpose unclear |

---

## Asset 504 Analysis (2026-01-07) - UNCONFIRMED 🟢 POLISH

**Risk:** Only appears in FINN (swimming) and RUNN (runner) vehicle modes.
Non-critical for decomp of the standard platforming path. Verify before
shipping FINN/RUNN support, but safe to defer to Tier 6 (Polish).

Asset 504 appears **only in vehicle/driving levels**:
- **FINN** (Level 4) - Submarine level: 4992 bytes (155 paired entries)
- **RUNN** (Level 22) - Auto-runner level: 64 bytes (2 paired entries)

### ImHex Template Support

Asset 504 is now parseable via the ImHex template (`docs/blb.hexpat`).

```bash
# Regenerate cached JSON (takes ~2 minutes)
imhex --pl format --pattern docs/blb.hexpat --input disks/blb/GAME.BLB > /tmp/blb_parsed.json

# Count collectibles in FINN (type_flags1 == 0x8000)
jq '[.. | objects | select(.vehicle9?) | .vehicle9.entries[] | select(.zone.type_flags1 == 32768)] | length' /tmp/blb_parsed.json
# Returns: 76

# Show first entry
jq '.. | objects | select(.vehicle9?) | .vehicle9.entries[0]' /tmp/blb_parsed.json
```

### Distribution

Only 2 of 26 levels contain Asset 504. Both are "vehicle" levels where Klaymen
pilots a vehicle rather than walking/jumping.

### Structure Hypothesis

**Header (8 bytes):**
```c
struct Asset504Header {
    u32 entry_count;   // FINN=149, RUNN=106 (but RUNN only has 2 entries?)
    u32 flags;         // FINN=16, RUNN=1 (possibly entry size or type indicator)
};
```

**Entries appear to be 32 bytes each (paired 16-byte sub-entries):**

```c
// Sub-entry A: Bounding box / zone definition
struct Asset504ZoneEntry {
    s16 x1, y1;        // Top-left corner
    s16 x2, y2;        // Bottom-right corner
    s16 center_x;      // Center point X (or 0)
    s16 center_y;      // Center point Y (or flag)
    u16 type_flags1;   // 0x0000 = standard zone, 0x8000 = collectible/marker
    u16 type_flags2;   // 1 = zone type A, 2 = zone type B
};

// Sub-entry B: Associated data
struct Asset504DataEntry {
    u16 flags;         // 0, 1351 (0x547), or 1601 (0x641)
    u16 index;         // Sometimes 0 or 1
    u16 ref_id;        // 251 (0xFB), 1351 (0x547), or 0
    u16 reserved1;     // Usually 0
    u16 delay;         // Frame count? Range 5-149
    u16 reserved2;     // Usually 0
    u16 next_x;        // Next waypoint X or entry index
    u16 next_y;        // Next waypoint Y
};
```

### Entry Type Analysis (FINN)

| Type | Count | Description |
|------|-------|-------------|
| (0, 1) | 77 | Standard zones with bounding boxes |
| (0, 2) | 2 | Special zones (endpoints?) |
| (0x8000, 0) | 76 | Collectibles/markers with ref=251 |

### Observations

1. **Type 0x8000 entries** (76 total in FINN):
   - All have `ref_id = 251` (0xFB)
   - Positions form a path through the level (sorted by Y coordinate)
   - Delay values range 5-15 (likely spawn/appear timing)
   - These are probably **collectible clay pods** placed along the path

2. **Type (0, 1) entries** (77 total in FINN):
   - Most have `ref_id = 0`, two have `ref_id = 1351`
   - Entries with ref=1351 have large `next_x/next_y` values (actual coordinates)
   - Entries with ref=0 have small `next_x` values (possibly entry indices?)
   - These define the **track/path boundaries** the vehicle follows

3. **Type (0, 2) entries** (2 in FINN):
   - Have `flags = 1601` (0x641)
   - Likely **start/end points** or checkpoints

4. **Position ranges** (FINN collectibles):
   - X: 208 to 1616 pixels
   - Y: 112 to 448 pixels
   - Matches expected submarine level dimensions

### RUNN Discrepancy

RUNN has `entry_count = 106` in the header but only 64 bytes of data (2 entries).
This suggests:
- The count field may mean something different
- RUNN may use a different format variant
- Or there's a size/offset issue in parsing

### Verification Needed

1. Trace code that reads Asset 504 in Ghidra
2. Run FINN level and breakpoint on memory reads from 504 data
3. Confirm if Type 0x8000 entries are indeed collectibles
4. Verify the linking between entries (next_x/next_y interpretation)

---

## Asset 700 Analysis (2026-01-07) - ✅ CONFIRMED: DEMO REPLAY DATA

**RESOLVED 2026-01-19**: Asset 700 is **demo/attract mode input replay data**, not SPU audio!

Asset 700 appears in **9 of 26 levels**, all in the tertiary segment (stage 5):
- MENU, SCIE, TMPL, BOIL, FOOD, BRG1, GLID, CAVE, WEED

### Structure Analysis

**Header format** (16 bytes):
```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    Entry count (number of replay entries)
0x02    2     u16    Reserved (always 0)
0x04    4     u32    Entry ID (varies per level)
0x08    4     u32    Data size (total bytes after header)
0x0C    4     u32    Data offset (always 0x10)
0x10    var   data   RLE replay entries (4 bytes each)
```

**Replay Entry Format** (4 bytes each):
```
Offset  Size  Type   Description
------  ----  ----   -----------
0x00    2     u16    Button state (PSX controller bitmask)
0x02    2     u16    Duration (frames to hold these buttons)
```

**Button Bitmask Values:**
- `0x0000` = No buttons (idle)
- `0x0080` = Right (D-pad)
- `0x00C0` = Right + L1 (run right)
- `0x2000` = X (jump)
- `0x2080` = X + Right (jump right)

### Demo Replay Data Per Level

| Level | Entries | Total Frames | Duration (~60fps) |
|-------|---------|--------------|-------------------|
| MENU  | 120     | 3072         | ~51 seconds       |
| SCIE  | 70      | ~1800        | ~30 seconds       |
| TMPL  | 75      | ~1950        | ~32 seconds       |
| BOIL  | 119     | ~3000        | ~50 seconds       |
| FOOD  | 78      | ~2000        | ~33 seconds       |
| BRG1  | 84      | ~2200        | ~37 seconds       |
| GLID  | 47      | ~1200        | ~20 seconds       |
| CAVE  | 51      | ~1300        | ~22 seconds       |
| WEED  | 85      | ~2200        | ~37 seconds       |

### Key Functions (Verified in Ghidra)

- `GetDemoDataPtr` @ 0x8007BAC8: Returns `ctx[0x54] + 0x10` (skips header)
- `EnableDemoPlaybackMode` @ 0x80025BC0: Sets replay mode flags
- `UpdateInputState` @ 0x800259D4: Reads replay data during demo
- `MenuTickCallback` @ 0x80077940: Triggers demo after 1801 frames idle

### Resolution Summary

The values we thought were "SPU commands" (0x80, 0xC0) are actually PSX controller
button bitmasks! The data is RLE-encoded input replay for attract mode demos.

See `docs/systems/demo-attract-mode.md` for complete documentation.

---

## Ghidra Function Naming Summary (2026-01-05)

The following functions have been renamed in Ghidra based on decompilation analysis:

### Tileset/Layer Functions

| Address | Old Name | New Name | Purpose |
|---------|----------|----------|---------|
| 0x8007b6c8 | FUN_8007b6c8 | GetLayerCount | Layer count from Asset 200 |
| 0x8007b700 | FUN_8007b700 | GetLayerEntry | Layer entry from Asset 201 (92-byte stride) |
| 0x8007b6dc | FUN_8007b6dc | GetTilemapDataPtr | Tilemap data pointer from Asset 200 sub-TOC |
| 0x8007b658 | FUN_8007b658 | GetAnimatedTileData | Animated tile lookup from ctx[11] |
| 0x8007b4f8 | FUN_8007b4f8 | GetPaletteDataPtr | Palette color data from Asset 400 |
| 0x8007b530 | FUN_8007b530 | GetPaletteAnimData | Palette animation flags from ctx[9] |

### Sprite/Animation Functions (2026-01-04)

| Address | Old Name | New Name | Purpose |
|---------|----------|----------|---------|
| 0x80010068 | FUN_80010068 | DecodeRLESpriteCore | RLE decompression with mirror support |
| 0x8007bf7c | FUN_8007bf7c | DecodeRLESpriteChecked | Checks valid flag, then calls DecodeRLESpriteCore |
| 0x8007bde8 | FUN_8007bde8 | RenderSprite | Sprite render orchestration |
| 0x8007bebc | FUN_8007bebc | GetFrameMetadata | Frame accessor: `ctx[0] + frame_idx * 0x24` |
| 0x8007bc3c | FUN_8007bc3c | InitSpriteContext | Initializes 20-byte SpriteContext |
| 0x8007bc18 | FUN_8007bc18 | ClearSpriteContext | Zeros 20-byte SpriteContext |
| 0x8007bb10 | FUN_8007bb10 | LookupSpriteById | Searches DAT_800a6064, then DAT_800a6060 |
| 0x8007bb00 | FUN_8007bb00 | SetSpriteTables | Sets global sprite table pointers |
| 0x8007b968 | FUN_8007b968 | FindSpriteInTOC | Searches ctx+0x70, then ctx+0x40 (12B stride) |

### Functions with comments added (already had names):
- `GetTotalTileCount` (0x8007b53c) - Sum of Asset 100 offsets 0x10+0x12+0x14
- `CopyTilePixelData` (0x8007b588) - Copies tile pixels with 16x16/8x8 handling
- `GetTileSizeFlags` (0x8007b6bc) - Returns Asset 302 pointer (ctx[7])
- `GetPaletteIndices` (0x8007b6b0) - Returns Asset 301 pointer (ctx[6])
- `GetPaletteGroupCount` (0x8007b4d0) - Returns palette count from Asset 400
- `LoadAssetContainer` (0x8007b074) - Full asset loading with ID mapping
- `LoadTileDataToVRAM` (0x80025240) - VRAM upload with size flag processing
- `GetAsset100Field1C` (0x8007b7c8) - Unknown field at Asset 100 offset 0x1C

---

## Primary Asset 600 Entry Structure (2026-01-04)

**Status:** CORRECTED - Asset 600 is RLE sprite container, not 92-byte entries

**CORRECTION:** Earlier analysis confused the accessor functions. The 92-byte entries
are actually in **Asset 201 (0xC9)** from TERTIARY segment, not Primary Asset 600.

Primary Asset 600 (0x258) contains **RLE-compressed background sprites**:
- Sub-TOC with sprite entries
- Each entry has 24-byte header + frame metadata + RLE pixel data
- Same format as Tertiary Asset 600

See "Tertiary Asset 201 - Sprite Display Entries" below for the 92-byte entries.

---

## Tertiary Asset 201 - Sprite Display Entries (2026-01-04)

**Status:** CODE-VERIFIED via Ghidra + data analysis

Asset 201 (0xC9) in the TERTIARY segment contains sprite display configuration.
These are the 92-byte entries accessed by `FUN_80024778`.

### Discovery Path

The accessor functions reveal the structure:
- `GetLayerCount(ctx)` (0x8007b6c8) → returns `**(u16**)(ctx + 0xc)` = count from Asset 200
- `GetLayerEntry(ctx, i)` (0x8007b700) → returns `*(ptr*)(ctx + 0x10) + i * 0x5c` = entry from Asset 201

`LoadAssetContainer` (0x8007b074) sets:
- `ctx[3]` (offset 0x0C) = Asset 200 pointer (has count at offset 0)
- `ctx[4]` (offset 0x10) = Asset 201 pointer (92-byte entry table)

### Entry Structure (0x5C = 92 bytes)

```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    2     x_offset        X position/offset (tiles or pixels)
0x02    2     y_offset        Y position/offset
0x04    2     src_width       Source/clip width
0x06    2     src_height      Source/clip height
0x08    2     dst_width       Destination width (level units)
0x0A    2     dst_height      Destination height
0x0C    2     sprite_index    Index into Asset 200 sub-TOC
0x0E    2     priority        Render priority/depth
0x10    2     flags_10        Display flags (parallax bits?)
0x12    2     flags_12        Additional flags
0x14-1B 8     unknown         
0x1C    1     byte_1C         
0x1D    1     byte_1D         
0x1E    1     enable_flag_1   If !=0: game_state+0x59 = 1
0x1F    1     enable_flag_3   If !=0: game_state+0x5b = 1
0x20    1     flag_20         Low byte: if !=0: game_state+0x58 = 1
0x21    1     enable_flag_2   If !=0: game_state+0x5a = 1
0x22-23 2     unknown
0x24    2     scroll_factor   Parallax scroll factor (256 = 1:1)
0x26    1     object_type     Type ID (if ==3, entry is skipped)
0x27    1     padding
0x28    4     skip_flag       If !=0, entry is skipped entirely
0x2C    4     rgb_or_data     Entry[0] = background RGB color
0x30    48    padding         Filled with 0x40 ('@')
```

### Example Data (PHRO Level, Tertiary Block 0)

```
 # X_off Y_off SrcW SrcH DstW DstH  Sprite Priority  Scroll Type  Description
-- ----- ----- ---- ---- ---- ---- ------- -------- ------- ---- -----------
 0     0     2   25   14   25   25     150      265     256    0  UI element
 1     0     7   25    9   25   25     250      150     256    0  UI element
 2     0    10   25   15   25   25     350      318     256    0  UI element
 3    34    16    7    8  600   90     350       47       0    0  Parallax far
 4    18    16    5    6  600   90     350       29       0    0  Parallax far
 5     4    15    4    6  600   90     350       20       0    0  Parallax far
 6    52    17    9    9  600   90     550       70       0    0  Parallax mid
 7    71    17   11   12  600   90     650      114       0    0  Parallax mid
 8     4    22  109   41  600   90     750     3243       0    0  Parallax mid
 9     2    18  454   42  456   70     850     4619       0    0  Main BG layer
10     1    19  439   37  456   70     950     1149       0    0  Main BG layer  
11    29    19  237   37  456   70    1050      532       0    0  Main BG layer
12    94    19   15   46  600   90    1150      519       0    0  Foreground
```

### Key Observations

1. **Level-sized layers**: Entries with dst_width/height matching level dimensions
   (456x70 for PHRO) are the main background tilemap layers

2. **Parallax backgrounds**: Entries with width=600 (wider than 456-tile level)
   are parallax scrolling backgrounds

3. **Scroll factor**: Value of 256 = 1:1 scroll with camera. Lower values = slower
   parallax. Value of 0 = fixed/static.

4. **Entry[0] special**: First entry's offset 0x2C contains background clear color

5. **Priority ordering**: Higher priority values render in front

6. **Asset 200 contains tilemaps**: Each Asset 201 entry has a 1:1 correspondence with
   an Asset 200 sub-TOC entry. Asset 200 entry size = src_width × src_height × 2 bytes,
   containing u16 tile indices.

7. **Tilemap rendering**: The src dimensions define the tilemap stored in Asset 200.
   The dst dimensions define how it's rendered (stretched/tiled to fill layer).

### Processing Logic (FUN_80024778 / ProcessLayerEntries)

```c
count = GetLayerCount(ctx);      // From Asset 200
entries = *(ptr*)(ctx + 0x10);   // Asset 201 base

for (int i = 0; i < count; i++) {
    entry = GetLayerEntry(ctx, i);  // entries + i * 0x5c
    
    if (entry->skip_flag != 0) continue;
    if (entry->object_type == 3) continue;
    
    // Entry 0 sets background color
    if (i == 0) {
        game->bg_r = entry->rgb[0];
        game->bg_g = entry->rgb[1]; 
        game->bg_b = entry->rgb[2];
    }
    
    // Size categorization for render list
    if (dst_width > 63 || dst_height > 63) {
        // Large - parallax layer
    } else if (dst_width < 129 && dst_height < 129) {
        // Medium - standard sprites
    } else {
        // Other
    }
}
```

---

## Tertiary Asset 200 - Tilemap Container (2026-01-04)

**Status:** DATA-VERIFIED via BLB analysis

Asset 200 (0xC8) in the TERTIARY segment contains tilemap data for background layers.
Each sub-TOC entry corresponds 1:1 with an Asset 201 layer definition entry.

### Structure

```
Offset  Size  Description
------  ----  -----------
0x00    4     Entry count (matches Asset 201 entry count)
0x04    12*N  Sub-TOC entries:
              - u32 flags
              - u32 size (bytes)
              - u32 offset
```

### Tilemap Data Format

Each sub-TOC entry points to a flat array of **u16 tile indices**:
- Size = src_width × src_height × 2 bytes (from corresponding Asset 201 entry)
- Tile index 0 = transparent/empty
- Non-zero indices reference tiles in Secondary segment tilesets

### Size Correlation (PHRO Level Example)

| Entry | Asset 200 Size | Asset 201 src_w×src_h | Match |
|-------|----------------|----------------------|-------|
| 0 | 700 | 25×14 = 350 tiles | ✅ 350×2=700 |
| 1 | 450 | 25×9 = 225 tiles | ✅ 225×2=450 |
| 2 | 750 | 25×15 = 375 tiles | ✅ 375×2=750 |
| 3 | 112 | 7×8 = 56 tiles | ✅ 56×2=112 |
| 4 | 60 | 5×6 = 30 tiles | ✅ 30×2=60 |
| 5 | 48 | 4×6 = 24 tiles | ✅ 24×2=48 |
| 6 | 162 | 9×9 = 81 tiles | ✅ 81×2=162 |
| 7 | 264 | 11×12 = 132 tiles | ✅ 132×2=264 |
| 8 | 8938 | 109×41 = 4469 tiles | ✅ 4469×2=8938 |
| 9 | 38136 | 454×42 = 19068 tiles | ✅ 19068×2=38136 |
| 10 | 32486 | 439×37 = 16243 tiles | ✅ 16243×2=32486 |
| 11 | 17538 | 237×37 = 8769 tiles | ✅ 8769×2=17538 |
| 12 | 1380 | 15×46 = 690 tiles | ✅ 690×2=1380 |

**All 13 entries match exactly: size = src_w × src_h × 2**

### Layer Rendering Model

Asset 201 defines how each tilemap layer is rendered:
- **src_w × src_h**: Actual tilemap dimensions stored in Asset 200
- **dst_w × dst_h**: Destination layer size for rendering
- Small src → large dst: Tilemap is tiled/repeated to fill the layer
- Similar src/dst: Layer rendered at native size

### Layer Categories (from PHRO analysis)

**UPDATED 2026-01-05: Layers 8-11 contain ENTITY SPAWN DATA, not just collision zones.**

| Layer | src Size | dst Size | Purpose |
|-------|----------|----------|---------|
| 0-5 | 4-25×6-15 | 25×25 or 600×90 | Parallax background tiles |
| 6 | 9×9 | 600×90 | Foreground decorative element |
| 7 | 11×12 | 600×90 | Same as 6, different parallax |
| 8 | 109×41 | 600×90 | Entity spawn layer (parallax objects) |
| 9 | 454×42 | 456×70 | Entity spawn layer (main level objects) |
| 10 | 439×37 | 456×70 | Entity spawn layer (platforms) |
| 11 | 237×37 | 456×70 | Entity spawn layer (additional objects) |
| 12 | 15×46 | 600×90 | Foreground decoration |

**See "Entity Spawn System" in blb-data-format.md for full details.**

### Tile Index Format

Sample from Entry 9 (main level layer):
```
Offset 0x2E2: 97 70 98 70 99 70 9A 70 9B 70 9C 70
              ^^^^^ ^^^^^ ^^^^^ ^^^^^ ^^^^^ ^^^^^
              0x7097 0x7098 0x7099 0x709A 0x709B 0x709C
```

Values in range ~28000-30000 have high bits set. See "Tile Index Encoding" below.

---

## Tile Data Format (2026-01-04, UPDATED 2026-01-04)

**Status:** CODE-VERIFIED via Ghidra decompilation of `CopyTilePixelData` (0x8007b588)

### Asset 100 Header (Secondary Segment)

```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    2     unknown_00      Unknown (6160 for PHRO - not tile count!)
0x08    2     level_width     Level width in tiles (456 for PHRO)
0x0A    2     level_height    Level height in tiles (70 for PHRO)
0x10    2     n_16x16         Count of 16×16 tiles (1109 for PHRO)
0x12    2     n_8x8           Count of 8×8 tiles (540 for PHRO)
0x14    2     n_unknown       Third tile count (0 for PHRO)
```

**Total tile count = n_16x16 + n_8x8 + n_unknown**

### Tile Pixel Storage (Asset 300)

From `CopyTilePixelData` (0x8007b588):

**16×16 tiles** are stored first:
- 256 bytes each (16×16 8bpp)
- Offset = `(tile_index - 1) * 0x100`

**8×8 tiles** are stored after 16×16 tiles:
- **128 bytes each** (8×16 rows - padded to 16 rows!)
- Offset = `n_16x16 * 0x80 + (tile_index - 1) * 0x80`

**IMPORTANT:** Tile indices are **1-based**. Index 0 means empty/transparent.

### Pixel Data Size Calculation

```
total_bytes = n_16x16 * 256 + n_8x8 * 128
```

For PHRO: `1109 * 256 + 540 * 128 = 283,904 + 69,120 = 353,024 bytes` ✅ Matches!

### Tile Index Encoding (Tilemap u16 values)

**VERIFIED 2026-01-05 via extract_all_graphics.py working implementation.**

For all layers, tile indices use this format:

```
Bits 0-11 (0xFFF): Tile index (12 bits, 1-based, 0 = transparent)
Bits 12-15:        Color tint selector (indexes into layer's color_tints[16])
```

**VERIFIED via Ghidra** `RenderTilemapSprites16x16` @ 0x8001713c:
```c
tile_idx = tilemap_entry & 0xFFF;           // 12-bit tile index
color_idx = (tilemap_entry >> 12) & 0xF;    // 4-bit color tint
```

**Entity Detection:** If `(tile_val & 0xFFF) > total_tile_count`, this may be an entity tile.

**Reference implementation:** (CORRECTED)
```python
tile_idx = raw_idx & 0xFFF  # Lower 12 bits = tile index
color_tint = (raw_idx >> 12) & 0xF  # Upper 4 bits = color tint
```

---

## Layer Purpose Analysis (2026-01-04, UPDATED 2026-01-05)

**Status:** ENTITY-VERIFIED via tilemap extraction and bit analysis (PHRO level)

Based on visual inspection of extracted layer images:

| Layer | Dimensions | Visual Content | Verified Purpose |
|-------|------------|----------------|---------------------|
| 0-5 | 25×14, etc | Small sprites/icons | Parallax background patterns |
| 6 | 9×9 | Foreground object | Decorative foreground element |
| 7 | 11×12 | Same as layer 6 | Same object on different parallax plane |
| 8 | 109×41 | Rectangular zones | **Entity spawn layer** - large objects/decorations |
| 9 | 454×42 | Main level tiles | **Entity spawn layer** - main level objects |
| 10 | 439×37 | Additional zones | **Entity spawn layer** - platforms/hazards |
| 11 | 237×37 | Additional zones | **Entity spawn layer** - additional objects |
| 12 | 15×46 | Tall structure | Foreground decoration |

### Entity Spawn Discovery (2026-01-05)

**STATUS: VERIFIED** - Layers 8-11 contain entity spawn data encoded in tilemap indices.

Key findings:
- Tile indices with bit 12 set (value >= 4096) are entity tiles
- Connected regions of entity tiles form single entity instances  
- 28 entity regions found in PHRO across layers 8-11
- 23 unique entity types identified by their tile ID sets
- Entity tile indices range 9-1102 (after masking bit 12)

See **Entity Spawn System** section in `blb-data-format.md` for complete documentation.

### Layer Rendering Order (Hypothesis)

Layers are likely rendered back-to-front based on priority field in Asset 201:
1. Background color (from Entry 0)
2. Far parallax layers (0-5) - tiny repeating patterns
3. Mid parallax layers (6-8) - entity objects + decorations
4. Main level layers (9-11) - entity spawn markers (platforms, objects, hazards)
5. Foreground layer (12)

### Entity vs Collision Layers (UPDATED 2026-01-10)

Previous hypothesis about layers 9-11 being "collision layers" was incorrect.

**New understanding:**
- Layers 8-11 contain **entity spawn data** encoded as tile indices with bit 12 set
- Entity tiles reference graphics from tertiary Asset 600 sprite data
- Each connected region of entity tiles = one entity instance

**Collision Data Location (CORRECTED 2026-01-10):**
- **Asset 601 (0x259) is AUDIO**, not collision (code-verified via `UploadAudioToSPU`)
- **Asset 500 (0x1F4)** is the **Tile Attribute Map** containing per-tile collision flags
  - Format: 8-byte header (u32 unknown, u16 width, u16 height) + 1 byte per tile
  - Known values: 0x00=passable, 0x02=solid, 0x12/0x53/0x65=triggers

The distinction is:
- **Tilemaps (layers)**: Visual rendering + entity spawn positions
- **Asset 500**: Per-tile collision/attribute flags (tile-based collision)
- **Asset 601**: Audio sample bank for SPU (NOT collision)

---

## RLE Sprite Decoder (2026-01-04)

**Status:** CODE-VERIFIED via Ghidra decompilation of `FUN_80010068`

The game uses a sparse RLE format for sprite compression, optimized for sprites
with large transparent regions.

**Decoder Function:** `FUN_80010068` at 0x80010068

**Context Structure (passed as param_2):**
```c
struct RLEContext {
    int control_count;      // [0] Number of control words remaining
    int stride;             // [1] Row stride (bytes per row in output)
    void* dest_row;         // [2] Current destination row pointer
    u16* control_ptr;       // [3] Pointer to control word array
    u8* pixel_ptr;          // [4] Pointer to pixel data
    short mirror_flag;      // [5] Non-zero = draw mirrored (right-to-left)
};
```

**Control Word Format (u16):**
```
Bit 15 (0x8000): New row flag - advance to next row
Bits 8-14 (0x7F00 >> 8): Skip count - transparent pixels to skip
Bits 0-7 (0xFF): Copy count - pixels to copy from source
```

**Algorithm:**
1. Read control word
2. If bit 15 set: advance destination to next row
3. Skip `(ctrl >> 8) & 0x7F` pixels (transparent)
4. Copy `ctrl & 0xFF` pixels from pixel data
5. Repeat until control_count == 0

**Mirror Mode:**
When mirror_flag != 0, pixels are written right-to-left instead of left-to-right.
Skip direction is also reversed.

**Used by:**
- Tertiary Asset 600 (0x258): Character sprites
- Primary Asset 600 (0x258): Background decoration sprites

**Verification needed:** Implement decoder and verify output matches VRAM captures.

---

## Sprite Animation System - ToolX (2026-01-04)

**Status:** RESEARCHING - Ghidra analysis in progress

### Developer Context

From a 1998 developer interview:

> "We took [the captured frames] and built our sequences using our proprietary
> animation scripting software, ToolX, which was written by Kenton Leach.
> Using this tool was similar to entering the images into an exposure sheet,
> which then wrote out a sequence file which could be read by our game engine."

This confirms:
1. **Exposure sheet model** - Frame metadata follows animation industry conventions
2. **Sequence files** - Animation data is separate from sprite pixel data
3. **ToolX output** - The 36-byte frame metadata structure is ToolX's output format

### Sprite Container Structure (Asset 600)

```
Container:
├── Count (u32) - Number of sprites
├── Sub-TOC (12 bytes × count):
│   ├── ID (u32)
│   ├── Size (u32)
│   └── Offset (u32)
└── Sprite data (for each entry):
    ├── Sprite Header (24 bytes)
    ├── Frame Metadata (36 bytes × frame_count)
    └── RLE Pixel Data
```

### Sprite Header (24 bytes)

**Status:** DATA-VERIFIED via analysis of 97 sprites across all levels (2026-01-05)

```
Offset  Size  Field              Description
------  ----  -----              -----------
0x00    2     magic              Always 1
0x02    2     header_size        Always 24 (0x18)
0x04    2     frame_meta_size    Total bytes for all frame metadata
0x06    2     padding            Always 0
0x08    4     pixel_size         Total bytes of RLE pixel data
0x0C    4     sprite_id          Same as Sub-TOC ID
0x10    2     frame_count        Number of animation frames
0x12    2     reserved           Always 0 (reserved field)
0x14    2     animation_mode     Boolean: 0 or 1 (see hypothesis below)
0x16    2     clut_value         CLUT VRAM position encoding (see below)
```

**Bytes per frame:** `frame_meta_size / frame_count` = 36 (0x24) bytes

**animation_mode Hypothesis (offset 0x14):**
- Value is always 0 or 1 (boolean flag)
- 67 sprites have animation_mode=1, 30 sprites have animation_mode=0
- Sprites with mode=1: avg 6.8 frames
- Sprites with mode=0: avg 8.5 frames
- Possible meanings: "has_default_animation", "is_looping", or "playback_mode"

### CLUT Value Encoding (2026-01-04)

**Status:** CODE-VERIFIED via `GetClut` (0x80089720) and `DumpClut` (0x80089798)

The `clut_value` (offset 0x16 in sprite header) encodes a VRAM position where
the 256-color palette is loaded at runtime:

```c
// GetClut formula (0x80089720):
u_short GetClut(int x, int y) {
    return (y << 6) | ((x >> 4) & 0x3f);
}

// DumpClut decode (0x80089798):
void DumpClut(u_short clut) {
    printf("clut: (%d,%d)\n", (clut & 0x3f) << 4, clut >> 6);
}
```

**Encoding:**
- `CLUT = (vram_y << 6) | ((vram_x >> 4) & 0x3F)`
- X is stored in low 6 bits (in units of 16 pixels)
- Y is stored in upper 10 bits

**Decoding:**
- `vram_x = (clut & 0x3F) << 4`
- `vram_y = clut >> 6`

**Example:** CLUT value 0x815D:
- X = (0x815D & 0x3F) << 4 = 29 << 4 = 464
- Y = 0x815D >> 6 = 517
- VRAM position: (464, 517)

**Palette Index Mapping:**
For this game, palettes are loaded into VRAM starting at Y=512 in the CLUT area.
Each row holds one 256-color palette. The palette index is:
- `palette_idx = vram_y - 512 + 1` (1-indexed)

For 0x815D: Y=517 → palette index = 517 - 512 + 1 = 6

### Asset 400 - Palette Container (2026-01-04)

**Status:** DATA-VERIFIED via BLB analysis

Asset 400 in the SECONDARY segment contains multiple 256-color palettes.

**Structure:**
```
Offset  Size        Field              Description
------  ----        -----              -----------
0x00    4           count              Number of palette entries
0x04    524         entry_0            Lookup table entry
0x?     524 × (N-1) entries_1..N       Raw 256-color palettes
```

**Entry 0 (Lookup Table):**
```
Offset  Size  Field         Description
------  ----  -----         -----------
0x00    12    header        VRAM RECT info (x, y, w, h, ...)
0x0C    12×N  entries       Palette sub-TOC entries
```

**Sub-TOC Entry (12 bytes each):**
```
u16 palette_idx    Index (1-7)
u16 padding
u16 size           Always 512 (256 colors × 2 bytes)
u16 padding
u16 offset         Offset within Asset 400 to palette data
u16 padding
```

**Palette Data:**
Each palette is 512 bytes = 256 colors × 2 bytes (PSX 15-bit RGB).

**PSX 15-bit Color Format:**
```
Bit 15:    STP (semi-transparency bit, usually 0)
Bits 10-14: Blue (0-31)
Bits 5-9:   Green (0-31)
Bits 0-4:   Red (0-31)
```

Conversion to 8-bit RGB:
```c
r = (color & 0x1F) << 3;
g = ((color >> 5) & 0x1F) << 3;
b = ((color >> 10) & 0x1F) << 3;
```

**PHRO Level Example:**
- Asset 400 size: 4196 bytes
- Palette count: 8
- Palette offsets: 612, 1124, 1636, 2148, 2660, 3172, 3684
- Tertiary sprites use palette 6 (CLUT 0x815D → Y=517 → index 6)

### Frame Metadata (36 bytes per frame)

**Status:** DATA-VERIFIED via analysis of 708 frames across all levels (2026-01-05)

From `FUN_8007bebc` (frame accessor) and `FUN_8007bc3c` (sprite context init):

```
Offset  Size  Field              Description
------  ----  -----              -----------
0x00    4     runtime_ptr        Usually 0 in file, pointer at runtime
0x04    2     flags              Render flags: 1, 2, or 3
0x06    2     render_x           Signed X render offset
0x08    2     render_y           Signed Y render offset
0x0A    2     render_width       Frame width in pixels
0x0C    2     render_height      Frame height in pixels
0x0E    2     offset_adjust_x    Signed X offset adjustment (animation tweak)
0x10    2     offset_adjust_y    Signed Y offset adjustment (animation tweak)
0x12    2     anchor_x           Signed anchor X (hotspot)
0x14    2     anchor_y           Signed anchor Y (hotspot)
0x16    2     clip_width         Clip rectangle width
0x18    2     clip_height        Clip rectangle height
0x1A    2     reserved_1A        Always 0
0x1C    2     collision_flag     Boolean: 0 or 1 (hitbox active?)
0x1E    2     reserved_1E        Always 0
0x20    4     rle_offset         Offset to RLE data for this frame
```

**offset_adjust_x/y Evidence (offsets 0x0E-0x11):**
These are signed 16-bit values representing per-frame position adjustments:
```
PHRO Sp11 Fr3: sz=45x71 offset_adjust=(0,-23) anchor=(-18,-62)  <- Y adjustment
SCIE Sp2  Fr0: sz=47x62 offset_adjust=(8, 0)  anchor=(-7,-53)   <- X adjustment
TMPL Sp11 Fr2: sz=45x55 offset_adjust=(-8,0)  anchor=(-19,-39)  <- negative X
```

**collision_flag Evidence (offset 0x1C):**
- 671 frames have value 0
- 37 frames have value 1
- Likely indicates whether hitbox/collision is active for this frame

### Sprite Context Structure (20 bytes, runtime)

From `InitSpriteContext` (0x8007bc3c):

```c
struct SpriteContext {
    void* frame_metadata;   // [0] Base pointer to frame metadata
    void* secondary_ptr;    // [1] From sprite_data+4 (pixel offset table?)
    void* pixel_data;       // [2] RLE pixel data base
    u16   max_width;        // [3] +0x0C: Maximum width across all frames
    u16   max_height;       //     +0x0E: Maximum height across all frames
    u16   frame_count;      // [4] +0x10: Number of frames
   u8    spriteLookupByte; //     +0x12: Low byte from sprite lookup entry+0x08; copied to cache metadata, no confirmed reader
    u8    valid_flag;       //     +0x13: 1 = initialized, 0 = invalid
};
```

### Sprite System Functions (renamed in Ghidra)

| Address | Name | Purpose |
|---------|------|---------|
| 0x80010068 | `DecodeRLESpriteCore` | RLE decompression with mirror support |
| 0x8007bf7c | `DecodeRLESpriteChecked` | Checks +0x13 valid flag before decode |
| 0x8007bde8 | `RenderSprite` | Sprite render orchestration |
| 0x8007bebc | `GetFrameMetadata` | Gets frame metadata: `ctx[0] + frame_idx * 0x24` |
| 0x8007bc3c | `InitSpriteContext` | Initializes SpriteContext from sprite data |
| 0x8007bc18 | `ClearSpriteContext` | Zeros 20-byte context |
| 0x8007bb10 | `LookupSpriteById` | Finds sprite by ID in DAT_800a6064/60 |
| 0x8007bb00 | `SetSpriteTables` | Sets global sprite table pointers |
| 0x8007b968 | `FindSpriteInTOC` | Searches ctx+0x70, then ctx+0x40 (12B stride) |

### Global Sprite Tables

From `FUN_8007bb10`:
- `DAT_800a6064` - Primary sprite table (searched first)
- `DAT_800a6060` - Secondary sprite table (fallback)

These are set by `FUN_8007bb00` during level loading.

### Connection to Assets 500-503

The animation sequence data (exposure sheet) may be split across:
- **Asset 401** (SEC/TERT): Palette animation config
- **Asset 500** (TERT): Tile attribute/collision map (see Asset 500 section below)
- **Asset 501** (TERT): Entity placement data (24-byte structures)
- **Asset 502** (TERT): VRAM rectangles
- **Asset 503** (TERT): Animation frame offsets - **likely ToolX sequence data**

**Hypothesis:** Asset 503's "animation frame offsets" correspond to the sequence
files output by ToolX, providing timing and ordering for sprite playback.

### Sprite Type 2 - Extended Animation Sequences (2026-01-05)

**Status:** DATA-VERIFIED - Frame count formula confirmed across all Type 2 sprites

Type 2 sprites (`type=2` at offset 0x00) have a 36-byte header (vs 24-byte for type 1)
and contain additional frame entries beyond the base `frame_count`.

**VERIFIED FORMULA:**
```
Total 36-byte frame entries = frame_count (offset 0x10) + extra_frames (offset 0x1C)
```

**Extended Header Structure (Type 2 only, 36 bytes):**
```
Offset  Size  Field           Description
------  ----  -----           -----------
0x00    24    base_header     Same as Type 1 sprite header
0x18    4     unknown_18      Varies (sprite ID related?)
0x1C    2     extra_frames    Additional frame entries beyond frame_count
0x1E    2     unknown_1E      Varies (360 for most, 2736 for PIRA)
0x20    2     unknown_20      0 or 1 (animation flags?)
0x22    2     clut_copy       Always 0x815D (same garbage as offset 0x16)
```

**Verification (100% match across all Type 2 sprites):**
```
PIRA spr2:  76 + 23 = 99 frames ✓
LEGL spr12: 10 + 10 = 20 frames ✓
SNOW spr8:  25 + 25 = 50 frames ✓
GLID spr2:   3 + 10 = 13 frames ✓
```

**Animation structure hypothesis:**
- `frame_count`: Number of frames in primary animation segment
- `extra_frames`: Number of frames in secondary animation segment
- Both use standard 36-byte frame entry format
- RLE offsets in extra segment often reference same data as primary (frame reuse)

**Example: LEGL sprite 12 (20 total frames = 10 + 10)**
```
Frame  0:  51x55  rle=0x0000  (start unique)
Frame  1:  60x38  rle=0x0484  (start unique)
Frame  2:  40x81  rle=0x08C8  (main sequence)
Frame  3:  51x104 rle=0x0F08  ...
Frame  4:  50x116 rle=0x15B4  ...
Frame  5:  49x116 rle=0x1C58  ...
Frame  6:  49x117 rle=0x2398  ...
Frame  7:  49x108 rle=0x2AD8  ...
Frame  8:  50x96  rle=0x31B0  ...
Frame  9:  48x69  rle=0x37B8  (main sequence end)
--- Extra frames below ---
Frame 10:  28x24  rle=0x3D50  (end unique)
Frame 11:  29x20  rle=0x3F68  (end unique)
Frame 12:  40x81  rle=0x08C8  (REUSES frame 2)
Frame 13:  51x104 rle=0x0F08  (REUSES frame 3)
Frame 14:  50x116 rle=0x15B4  (REUSES frame 4)
...
Frame 19:  48x69  rle=0x37B8  (REUSES frame 9)
```

**Hypothesis:** The extra frames provide a "reverse" or alternate animation path:
- Primary: Full forward animation (intro → main → peak)
- Extra: Reverse back (outro → loop back through main sequence)
- This creates smooth looping or state transitions without duplicating RLE data

### Sprite Unknowns Summary (2026-01-05)

**Status:** UNCONFIRMED - Collected from extraction analysis

#### Sprite Header Unknowns

| Offset | Size | Values Observed | Hypothesis |
|--------|------|-----------------|------------|
| 0x14 | 2 | 0, 1 | Boolean flag - animation loop? single-shot? |
| 0x16 | 2 | Always 0x815D | Garbage/padding - NOT actually CLUT (palette loaded at runtime) |

**Note:** The CLUT value at 0x16 appears to be leftover data from ToolX export.
At runtime, the game loads palettes from Asset 400 to fixed VRAM positions.

#### Frame Entry Unknowns (36-byte structure)

| Offset | Size | Values Observed | Hypothesis |
|--------|------|-----------------|------------|
| 0x00-0x01 | 2 | 0, 32, 79, 576, 648... | Purpose unknown - possibly tool artifact |
| 0x02-0x03 | 2 | 0, 64, 144, 257... | Purpose unknown |
| 0x04-0x05 | 2 | 1, 2, 3 (multi), 900 (single) | Frame index? RLE size for single-frame? |
| 0x0E-0x0F | 2 | 0, 3, 5, 8, 10, 65519 | Animation timing? (0-10 range with outliers) |
| 0x10-0x11 | 2 | Various | Unknown |

#### Confirmed Frame Entry Fields

| Offset | Size | Field | Verified By |
|--------|------|-------|-------------|
| 0x06 | 2 | render_x_offset (signed) | Ghidra + extraction |
| 0x08 | 2 | render_y_offset (signed) | Ghidra + extraction |
| 0x0A | 2 | width | `GetFrameMetadata` (0x8007bebc) |
| 0x0C | 2 | height | `GetFrameMetadata` (0x8007bebc) |
| 0x12 | 2 | hitbox_x (signed) | Data pattern analysis |
| 0x14 | 2 | hitbox_y (signed) | Data pattern analysis |
| 0x16 | 2 | hitbox_width | Data pattern analysis |
| 0x18 | 2 | hitbox_height | Data pattern analysis |
| 0x1A | 6 | padding (zeros) | Consistent across all sprites |
| 0x20 | 4 | rle_offset | `GetFrameMetadata` (0x8007bebc) |

### Next Steps

1. ✅ Rename sprite functions in Ghidra (9 functions renamed)
2. ✅ Decode CLUT value encoding (GetClut/DumpClut analysis)
3. ✅ Parse Asset 400 palette container structure
4. ✅ Create sprite extraction script (`scripts/extract_rle_sprites.py`)
5. ✅ Extract and verify sprites with correct palette colors
6. ☐ Map complete 36-byte frame metadata (bytes 0-5, 0x0E-0x11)
7. ☐ Decode Type 2 second 36-byte block
8. ☐ Decode Asset 503 to find sequence/timing data

---

## Asset 302 Value 0 vs 1 Meaning (2026-01-04, UPDATED 2026-01-04)

**Status:** Partially verified via Ghidra decompilation

Asset 302 (0x12E) contains one byte per tile with values 0, 1, or 2.

**CODE-VERIFIED from `LoadTileDataToVRAM` (0x80025240):**
```c
byte flags = asset302[tileIndex];
if ((flags & 4) != 0) continue;  // Bit 2: skip tile
uint tp = ((flags & 2) == 0);    // Bit 1: 0=16x16, 1=8x8
output[tileIndex + 6] = flags & 1;  // Bit 0: stored as property
```

**Confirmed:**
- Bit 1 (mask 0x02): Tile size - 0=16×16, 1=8×8 (verified by data matching)
- Bit 2 (mask 0x04): Skip flag - if set, tile is not loaded (no such tiles observed in data)

**Still unconfirmed - Bit 0 (mask 0x01):**
The value 0 vs 1 difference is stored in the tile output structure at offset +6.
This flag is preserved but its rendering effect is unknown.

**Hypothesis: Layer/Rendering property**
- Value 0 = Default rendering
- Value 1 = Special rendering (foreground layer, transparency, or priority)

**Verification needed:** Find code that reads the stored flag at offset +6 during rendering.

---

## Complete Asset Type Reference (2026-01-04)

**Status:** Documented from extraction analysis

### Segment Overview

| Segment | Purpose | Key Assets |
|---------|---------|------------|
| PRIMARY | Level geometry, collision | 600, 601, 602 |
| SECONDARY | Background tiles | 100, 300, 301, 302, 400, 401 |
| TERTIARY | Character sprites, audio | 100, 200, 201, 302, 401, 500-503, 600, 700 |

### Asset Extraction Status

| Asset | Hex | Segment(s) | Structure | Extraction |
|-------|-----|------------|-----------|------------|
| 100 | 0x064 | SEC, TERT | RAW 36B | ✅ Header with tile/sprite counts |
| 101 | 0x065 | SEC, TERT | RAW | ❓ Optional header (8 levels) |
| 200 | 0x0C8 | TERT | CONTAINER | ✅ Tilemap data (u16 tile indices) |
| 201 | 0x0C9 | TERT | RAW 92B×N | ✅ Layer definitions (92-byte entries) |
| 300 | 0x12C | SEC | RAW | ✅ Tile pixels 8bpp |
| 301 | 0x12D | SEC | RAW | ✅ Palette index per tile |
| 302 | 0x12E | SEC, TERT | RAW | ✅ Tile/sprite size flags |
| 400 | 0x190 | SEC | CONTAINER | ✅ Palette container |
| 401 | 0x191 | SEC, TERT | RAW | ⚠️ Animation config |
| 500 | 0x1F4 | TERT | CONTAINER | ❓ Unknown sub-TOC |
| 501 | 0x1F5 | TERT | RAW | ⚠️ VRAM texture coords |
| 502 | 0x1F6 | TERT | RAW | ⚠️ VRAM rectangles |
| 503 | 0x1F7 | TERT | CONTAINER | ⚠️ Animation frame offsets |
| 600 | 0x258 | PRIM, TERT | CONTAINER | ✅ RLE sprite container |
| 601 | 0x259 | PRIM, SEC | CONTAINER | ✅ Audio sample bank (CODE-VERIFIED via UploadAudioToSPU) |
| 602 | 0x25A | PRIM, SEC | RAW | ✅ Palette (15-bit RGB) / Audio volume-pan table |
| 700 | 0x2BC | TERT | RAW | ❓ Audio (9 levels only) |

Legend: ✅ Fully documented, ⚠️ Partially understood, ❓ Unknown

### Extraction Script

`scripts/extract_all_assets.py` extracts all known assets:
```
output/assets/
├── 00_MENU/
│   ├── primary/palette_602.bin, .png
│   ├── secondary/tileset.png, palettes.png, *.bin
│   └── tertiary/block_N/sprites/, *.bin
├── 01_PHRO/
...
└── all_stats.json
```

Total extracted (26 levels):
- 21,525 tiles (16×16) + 4,683 tiles (8×8)
- 196 palettes
- 1,498 sprites (raw RLE data)
- 104 tertiary blocks

---

## LevelDataContext Accessor Functions (2026-01-04, UPDATED 2026-01-05)

**Status:** CODE-VERIFIED via Ghidra (functions renamed)

These accessor functions operate on the LevelDataContext structure (GameState + 0x84).
They provide the link between loaded asset data and game rendering.

### Asset 600/200/201 (Level Layer/Sprite) Accessors

| Function | Address | Returns | Description |
|----------|---------|---------|-------------|
| `GetLayerCount` | 0x8007b6c8 | u16 | Entry count: `**(u16**)(ctx + 0xc)` (from Asset 200) |
| `GetLayerEntry` | 0x8007b700 | ptr | Entry by index: `*(ptr*)(ctx+0x10) + i * 0x5c` (from Asset 201) |
| `GetTilemapDataPtr` | 0x8007b6dc | ptr | Tilemap data from Asset 200 sub-TOC |
| `GetAnimatedTileData` | 0x8007b658 | ptr | Animated tile lookup from ctx[11] |

### Asset 100 (Tile Header) Accessors

| Function | Address | Returns | Field |
|----------|---------|---------|-------|
| `GetLevelDimensions` | 0x8007b434 | void | Writes ctx+4 offsets 0x8, 0xB (width, height) |
| `FUN_8007b458` | 0x8007b458 | void | Writes ctx+4 offsets 0xC, 0xF (alt dims?) |
| `FUN_8007b4b8` | 0x8007b4b8 | ptr | Background RGB (offset 0x00 in Asset 100) |
| `FUN_8007b4c4` | 0x8007b4c4 | ptr | Secondary RGB (offset 0x04 in Asset 100) |
| `GetPaletteGroupCount` | 0x8007b4d0 | u8 | Palette count from Asset 400 |
| `GetTotalTileCount` | 0x8007b53c | u16 | Sum of offsets 0x10+0x12+0x14 in Asset 100 |
| `CopyTilePixelData` | 0x8007b588 | void | Copy tile pixels to buffer (8bpp) |
| `GetPaletteIndices` | 0x8007b6b0 | ptr | Asset 301 pointer (ctx[6]) |
| `GetTileSizeFlags` | 0x8007b6bc | ptr | Asset 302 pointer (ctx[7]) |
| `GetAsset100Field1C` | 0x8007b7c8 | u16 | Unknown field at +0x1C |
| `FUN_8007b7dc` | 0x8007b7dc | ptr | ctx+0x3C (Asset 501?) |

### Asset 400 (Palette) Accessors

| Function | Address | Returns | Description |
|----------|---------|---------|-------------|
| `GetPaletteGroupCount` | 0x8007b4d0 | u8 | Palette count from Asset 400 sub-TOC |
| `GetPaletteDataPtr` | 0x8007b4f8 | ptr | Palette color data pointer (navigates sub-TOC) |
| `GetPaletteAnimData` | 0x8007b530 | ptr | Palette animation flags from ctx[9] (Asset 401) |

### Core Loading Functions

| Function | Address | Description |
|----------|---------|-------------|
| `LoadAssetContainer` | 0x8007b074 | Parses sub-TOC, populates ctx[1-22] based on asset IDs |
| `LoadTileDataToVRAM` | 0x80025240 | Uploads tiles to VRAM (called after LoadAssetContainer) |
| `InitLevelDataContext` | 0x8007a1bc | Initializes context with header pointer and callback |
| `LevelDataParser` | 0x8007a62c | Parses primary TOC, calls LoadAssetContainer |

### Context Field Layout (partial)

From accessor analysis, the LevelDataContext fields at low offsets:

```
Word   Offset  Description                           Source / Accessor
----   ------  -----------                           -----------------
[1]    0x04    Asset 100 pointer (tile header)       GetLevelDimensions, GetTotalTileCount
[3]    0x0C    Asset 200 pointer (tilemap TOC)       GetLayerCount (reads count at *[3])
[4]    0x10    Asset 201 pointer (layer entries)     GetLayerEntry (entries are 92 bytes each)
[5]    0x14    Asset 300 pointer (tile pixels)       CopyTilePixelData
[6]    0x18    Asset 301 pointer (palette indices)   GetPaletteIndices
[7]    0x1C    Asset 302 pointer (size flags)        GetTileSizeFlags
[8]    0x20    Asset 400 pointer (palette container) GetPaletteDataPtr
[9]    0x24    Asset 401 pointer (palette anim)      GetPaletteAnimData
[11]   0x2C    Asset 500 pointer (tile attribute map)   GetAnimatedTileData ← misnamed
[15]   0x3C    Asset 501 pointer                     FUN_8007b7dc
```

**Note:** These are separate from the main LevelDataContext offsets documented in
blb-data-format.md. The ctx+0x04 here refers to content WITHIN the loaded asset
buffers, accessed via the base pointer stored in the context.

---

## Secondary Asset 601 - AUDIO SAMPLES (2026-01-04) ✓ VERIFIED & DOCUMENTED

**Status:** ✓ VERIFIED AND MOVED TO blb-data-format.md (2026-01-07)

This section has been verified and fully documented. See:
- `docs/blb-data-format.md` → "Audio System (VERIFIED 2026-01-07)" section
- `docs/blb.hexpat` → AudioSampleEntry and AudioVolumeEntry structs

### Summary of Verified Findings

1. **Secondary Asset 601**: Audio sample bank with TOC
   - Format: `u16 count, u16 reserved, Entry[count] × 12 bytes, ADPCM data...`
   - Entry: `u32 sample_id, u32 spu_size, u32 data_offset`
   - Example: SCIE Stage 0 has 13 samples

2. **Secondary Asset 602**: Volume/pan table (4 bytes per sample)
   - Format: `u16 volume (0-0x3FFF), u16 pan (0 = center)`
   - Default if NULL: volume=0x3FFF, pan=0

3. **Tertiary Asset 700**: ✅ RESOLVED - Demo replay data
   - RLE-encoded input replay for attract mode demos
   - Accessed via `GetDemoDataPtr` @ 0x8007BAC8 (returns ctx[0x54] + 0x10)
   - See "Asset 700 Analysis" section above for complete format

4. **Key functions verified in Ghidra:**
   - `UploadAudioToSPU` @ 0x8007c088
   - `GetAsset601Ptr` @ 0x8007ba78
   - `GetAsset602Ptr` @ 0x8007baa0

5. **Correction**: Tertiary Assets 500-503 are NOT primarily audio:
   - Asset 500: Unknown (tile/sprite metadata?)
   - Asset 501: Entity placement data
   - Asset 502: VRAM rectangles
   - Asset 503: Animation frame offsets

---

## BLB Asset File Headers (OUTDATED)

> **NOTE:** This section is outdated. See blb/README.md for current documentation.
> The actual format uses standard TOC structures, not the format described below.

### Primary.bin (Level Data)

**Header format:** `[type=0x03] [0x258] [data_size] [0x28]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    4     0x00000003  Type identifier (always 3 for primary)
0x04    4     0x00000258  Header end offset (600 bytes)
0x08    4     varies      Data size or related offset
0x0C    4     0x00000028  Entry size (40 bytes)
```

- Contains level geometry, tile maps, collision data (guessed)
- Has embedded TIM textures (magic `0x10000000` found at various offsets)
- Size varies: 65KB (menu) to 1.3MB (complex levels)

### Secondary.bin / Secondary_sub_*.bin (Sprite/Object Data)

**Header format:** `[count] [entry_size=0x64] [header_size=0x24] [table_size]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    4     N           Entry count (typically 6-12)
0x04    4     0x00000064  Entry size (100 bytes per entry)
0x08    4     0x00000024  Header size (36 bytes before entry table)
0x0C    4     N*12+4      Computed table size
```

The `table_size` field follows a formula: `count * 12 + 4`

| Count | Expected [3] | Hex    |
|-------|--------------|--------|
| 6     | 76           | 0x4C   |
| 7     | 88           | 0x58   |
| 8     | 100          | 0x64   |
| 9     | 112          | 0x70   |
| 10    | 124          | 0x7C   |
| 11    | 136          | 0x88   |
| 12    | 148          | 0x94   |

- Contains sprite sheets, animations, object definitions (guessed)
- Contains embedded TIM images
- Entry table starts at offset 0x24 (36 bytes)

### Tertiary_sub_*.bin (Additional Assets)

- Same header format as secondary files
- Count ranges from 7-12
- Contains additional sprite data, backgrounds (guessed)
- Size varies widely: 4KB to 1MB

### Loading Screens

**Header format:** `[compressed_size:u16] [decompressed_size:u16=0x3800] [data...]`

```
Offset  Size  Value       Description
------  ----  -----       -----------
0x00    2     varies      Compressed data size
0x02    2     0x3800      Decompressed size (always 14,336 bytes)
0x04    4     varies      Additional header data
0x08+   ...   ...         Compressed image data
```

- Uses PSX compression (possibly LZSS or BizStream variant)
- Decompressed size always 14,336 bytes
- Likely 112×128 pixels at 8bpp, or similar dimensions
- **NOT** standard TIM format - custom compressed format

### Embedded Assets

- **TIM images** (magic `0x10000000`) found embedded in primary and secondary files
  - Multiple TIM images per file (20-40 typical)
  - Various bit depths detected (4-bit, 8-bit, 16-bit)
- **No VAG audio** found in level files
  - Audio likely handled separately (XA streaming or separate files)

## Credits Sequence Table

**Location:** BLB header offset 0xF10  
**Entry size:** 12 bytes (0x0C)  
**Max entries:** 2 complete entries before overlapping count fields

> **NOTE:** "Credits" is a GUESSED name based on the "CRD1"/"CRD2" codes found
> in entries and their relationship to the "CRED" sector entry. The actual
> purpose is not fully confirmed. JP versions don't have valid data here.

```
Offset  Size  Type    Field       Description
------  ----  ----    -----       -----------
0x00    4     str     code        4-char ID (empty for entry 0, "CRD1"/"CRD2" for others)
0x04    4     bytes   padding     Unused (zeros) - used to detect valid entries
0x08    2     u16     param_a     Entry 0: level_count; CRD1/2: unknown counter
0x0A    2     u16     param_b     Base sector offset
```

**Validation:** Valid credits entries have zero padding at bytes +4 to +8.
JP versions have non-zero values here, indicating garbage/unused data.

**Sector calculation (verified):**
```
CRED.sector_offset = param_b + level_count
Example: 171 + 26 = 197 = CRED sector entry offset
```

## JP Version Differences

### Sector Table Entry Layout

JP versions (papx-90053, slps-01501) have a different byte order for the 16-byte
sector table entries compared to PAL/NTSC versions:

**PAL/NTSC Layout:**
```
Offset  Size  Field
------  ----  -----
0x00    1     level_index
0x01    1     entry_flags
0x02    1     unknown_byte
0x03    5     code (5 chars, null-terminated)
0x08    4     short_name (4 chars)
0x0C    2     sector_offset (u16 LE)
0x0E    2     sector_count (u16 LE)
```

**JP Layout:**
```
Offset  Size  Field
------  ----  -----
0x00    2     sector_offset (u16 LE)
0x02    2     sector_count (u16 LE)
0x04    1     level_index
0x05    1     entry_flags
0x06    1     unknown_byte
0x07    5     code (5 chars, null-terminated)
0x0C    4     short_name (4 chars)
```

**Detection method:**
- PAL: byte[3] is uppercase A-Z (0x41-0x5A) - start of code field
- JP: byte[3] is 0x00 (low byte of sector_count), byte[7] is uppercase A-Z

### JP Content Differences

**CORRECTED:** Previous analysis incorrectly showed JP Full as having only 6 levels.
The correct values (verified via Ghidra and hex analysis):

| Version | Levels | Movies | Sector Entries | Credits Table | Sector Table Offset |
|---------|--------|--------|----------------|---------------|---------------------|
| PAL/NTSC | 26 | 13 | 32 | Valid (2 entries) | 0xCD0 |
| JP Demo (papx-90053) | 6 | 1 | 4 | Invalid (garbage) | 0xCB0 |
| JP Full (slps-01501) | 26 | 12 | 32 | Valid | 0xCB0 |

**Key JP Differences:**
1. **Sector table at 0xCB0** (32 bytes earlier than PAL's 0xCD0)
2. **12 movies** vs PAL's 13 (intro spliced into 1 movie)
3. **CRED moved from index 2 to index 28** - fixes credits bug

**Credits Bug Fix (from TCRF):**
In PAL, the intro is split between two movie files (INT1, INT2). A bug causes
credits to inadvertently trigger between the movies because CRED is at sector
index 2. JP fixed this by:
- Splicing the intro into a single movie file
- Moving CRED from index 2 to index 28 (after EVIL)
- All sector table entries after index 1 are shifted

**Offset Shift Analysis (JP vs PAL):**
| Access Point | PAL | JP | Difference |
|--------------|-----|-----|------------|
| Sector table base | 0xCD0 | 0xCB0 | -0x20 (-32 bytes) |
| sector_offset access | 0xCCC | 0xCB0 | -0x1C (-28 bytes) |
| level_count | 0xF31 | 0xF15 | -0x1C (-28 bytes) |
| movie_count | 0xF32 | 0xF16 | -0x1C (-28 bytes) |

---

## Level Metadata Unknown Fields Analysis

Analysis performed on all 26 levels across PAL, NTSC-US, Beta, and JP versions.

### Field 0x08: `entry1_offset_lo` - NOW CONFIRMED

**Previous understanding:** "Low 16 bits of Entry[1].offset in primary TOC"

**Actual meaning:** This is `TOC[6].offset` from the primary data blob (100% match on all 26 levels).

**Verification method:**
```python
# For each level:
toc6_offset = struct.unpack_from('<H', primary_data, 6*4)[0]
assert toc6_offset == level.entry1_offset_lo  # Always true
```

**Recommendation:** Rename field to `toc6_offset` and update documentation.

**Runtime verification:**
```bash
# Run the verification Lua script in PCSX-Redux:
make lua SCRIPT=scripts/verify_toc_fields.lua

# After level loads, check console output for TOC comparison
```

### Field 0x0A: `unknown_0A` - NOW CONFIRMED

**Actual meaning:** This is `TOC[2].count` from the primary data blob (100% match on all 26 levels).

**Verification method:**
```python
# For each level:
toc2_count = struct.unpack_from('<H', primary_data, 2*4+2)[0]
assert toc2_count == level.unknown_0A  # Always true
```

**Cross-version consistency:** Same values between PAL and JP for identical levels (PHRO, SCIE, TMPL, FINN, MEGA).

**Recommendation:** Rename field to `toc2_count` and investigate what TOC[2] points to.

**Runtime verification:**
```bash
# Run the verification Lua script in PCSX-Redux:
make lua SCRIPT=scripts/verify_toc_fields.lua

# The script will:
# 1. Dump all level metadata fields
# 2. After a level loads, compare field 0x0A with TOC[2].count
# 3. Report PASS/FAIL for each verification
```

### Field 0x06: `unknown_06` - PARTIALLY UNDERSTOOD

**Range:** 7-20

**Observations:**
- Does NOT directly match any single TOC entry count
- Consistent across regions (PAL, NTSC-US, Beta have identical values)
- Same values between PAL and JP for identical levels
- Pattern: Bosses tend to have higher values (18-20), simple/special levels have lower (7-12)

**Grouped by value:**
| Value | Levels |
|-------|--------|
| 7 | FINN |
| 9 | RUNN |
| 10 | KLOG |
| 12 | GLEN |
| 13 | FOOD |
| 14 | PHRO, SCIE, TMPL, GLID, WEED, SEVN, CRYS |
| 15 | SNOW, CAVE, BRG1, EGGS, CLOU, SOAR, CSTL, EVIL |
| 16 | MENU, BOIL, MOSS |
| 18 | MEGA, WIZZ |
| 20 | HEAD |

**Hypothesis:** May represent total asset complexity, memory requirements, or a computed value from multiple TOC entries.

### Field 0x04: `unknown_04` - UNKNOWN

**Range:** 776-65,240 (large u16 values)

**Observations:**
- Values differ slightly between PAL and JP for same levels (build-dependent)
- Points to valid data within primary blob (not garbage)
- Does NOT match any TOC entry offset directly
- Data at this offset varies (no consistent structure detected)

**Hypothesis:** Byte offset to specific asset data within primary blob (palette? collision map? geometry start?). Needs runtime tracing to confirm.

### Field 0x0D: `level_flag` - CONFIRMED: Password-Selectable Flag

**Values:** 0 or 1 only

**Purpose (CONFIRMED via Ghidra 2026-01-06):**
This flag indicates whether a level is "password-selectable" - i.e., can be directly
jumped to via the password system on the level select screen.

**Code Evidence:**
- `GetLevelFlagByIndex` (0x8007aa28) reads offset 0x0D from LevelEntry
- `InitGameState` (0x8007cd34) builds a list of selectable levels:
  ```c
  for each level where (GetLevelFlagByIndex(ctx, i) != 0 && GetLevelAssetIndex(ctx, i) != 0) {
      selectableLevels[count++] = levelAssetIndex;  // Max 10 entries
  }
  ```
- The list is stored at GameState+0x171 (10 entries max), count at GameState+0x17b

**Flag=1 (Selectable) Levels:**
| Index | ID | Name |
|-------|-----|------|
| 0 | MENU | Options (excluded by index!=0 check) |
| 2 | SCIE | Science Center |
| 3 | TMPL | Monkey Shrines |
| 6 | BOIL | Hard Boiler |
| 8 | FOOD | Skullmonkey Brand Hot Dogs |
| 10 | BRG1 | Elevated Structure of Ynt |
| 11 | GLID | Ynt Death Garden |
| 12 | CAVE | Ynt Mines |
| 13 | WEED | Ynt Weeds |

**Flag=0 (Non-Selectable) Levels:**
- PHRO (1) - First gameplay level (forced entry point)
- Boss levels: MEGA, HEAD, GLEN, WIZZ, KLOG
- Special modes: FINN, RUNN
- Later worlds: SNOW, EGGS, CLOU, SEVN, SOAR, CRYS, CSTL, MOSS, EVIL

**Interpretation:**
The 8 flag=1 levels (excluding MENU) appear to be password destinations.
PHRO is excluded because it's the natural starting level (no password needed).
Bosses and special modes are reached through normal gameplay progression.

### Groupings by (0x06, 0x0A) Pairs

Levels with identical `(unknown_06, unknown_0A)` pairs often share characteristics:

| Pair | Levels | Notes |
|------|--------|-------|
| (14, 7) | PHRO, SCIE, TMPL | First 3 gameplay worlds |
| (14, 9) | GLID, WEED, SEVN, CRYS | Ynt worlds + secret level |
| (15, 9) | SNOW, CAVE, CLOU, CSTL | Ice/castle themed |
| (15, 8) | BRG1, EGGS | Bridge + Eggs |
| (16, 9) | BOIL, MOSS | Industrial themed |
| (18, 15) | MEGA | Boss: Shriney Guard |
| (20, 16) | HEAD | Boss: Joe-Head-Joe |

This suggests these fields may relate to shared asset characteristics or tileset groupings.

---

## Level Loading System Analysis

> **NOTE:** Most of this information has been verified and moved to 
> [blb/README.md](../blb/README.md). See that document for:
> - Complete LevelDataContext structure (128 bytes, all fields documented)
> - Asset ID mapping table (LoadAssetContainer)
> - Playback sequence modes and string markers
> - Known functions with addresses

### Unverified: Menu Demo Rotation

When at the menu (level index 0), the game cycles through demo modes:

```c
// g_MenuDemoRotationCounter at 0x800A60A4
if (counter == 4) counter = 0;
else if (counter == 0) mode = 1;  // Normal
else if (counter == 2) mode = 6;  // End sequence?
else mode = 5;                     // Attract mode
counter++;
```

**Verification needed:** Confirm counter behavior and mode meanings.

---

## Layer/Entity Data Accessors (2026-01-04, UPDATED 2026-01-05)

**Status:** Decompiled via Ghidra, functions renamed

Functions that access layer/entity data from the tertiary segment:

### GetLayerCount (0x8007B6C8)

```c
u16 GetLayerCount(int ctx) {
    return **(u16**)(ctx + 0x0C);  // ctx[3] → Asset 200 (tilemapContainer)
}
```

Returns the layer count from the first u16 of Asset 200 (0xC8).
- `ctx + 0x0C` = LevelDataContext[3] = tilemapContainer pointer
- Reads count from the beginning of the asset data

### GetLayerEntry (0x8007B700)

```c
int GetLayerEntry(int ctx, uint index) {
    return *(int*)(ctx + 0x10) + (index & 0xFFFF) * 0x5C;
}
```

Returns pointer to layer entry at given index.
- `ctx + 0x10` = LevelDataContext[4] = layerEntries pointer
- Each entry is **0x5C (92) bytes**
- Asset 201 (0xC9) contains the layer definition table

### Layer Entry Structure (0x5C = 92 bytes, PARTIAL)

From FUN_80024778 analysis, each entity entry contains:

```
Offset  Size  Type    Description
------  ----  ----    -----------
0x00    4     u32     Position X (fixed point?)
0x04    4     u32     Position Y (fixed point?)
0x08    4     var     Width/size parameter
0x0A    2     u16     Height/size parameter (short access)
0x0C    4     var     Unknown (short at +0x0C checked)
0x0E    2     u16     Priority/layer value (compared against blb header)
0x10    4     var     Unknown
0x14    4     var     Unknown
0x18    4     var     Unknown
0x1A    2     u16     Unknown (copied to output +0x32)
0x1C    4     var     Unknown
0x1E    1     u8      Flag A - sets GameState+0x59 if non-zero
0x1F    1     u8      Flag B - sets GameState+0x5B if non-zero
0x20    1     u8      Flag C - sets GameState+0x58 via puVar14[8] low byte
0x21    1     u8      Flag D - sets GameState+0x5A if non-zero
0x24    4     var     Unknown (short at +0x24 checked against 0)
0x26    1     u8      Entity type (3 = special, skipped in some processing)
0x28    4     var     Unknown
0x2C    4     u32     RGB color (bytes at +0x2C, +0x2D, +0x2E copied to GameState+0x124-126)
```

**Layer Type 3:** When `*(short*)(entry + 0x26) == 3`, the layer is skipped in FUN_80024778.

**First Layer Special Handling:** When index == 0, copies RGB from entry+0x2C to GameState+0x124 (background color).

### GetTilemapDataPtr (0x8007B6DC)

Called for each layer in FUN_80024778. Returns tilemap data pointer for the layer by navigating the Asset 200 sub-TOC.

---

## Level Background Investigation (2026-01-04)

**Status:** In progress

### MDEC Frame Loading

From NOTES.md observations:
- `0x80116450` = Compressed MDEC frame buffer in RAM
- Various BLB offsets get loaded here during level transitions

Loading screens use `FUN_800399a8` (single MDEC frame) and `FUN_8003958c` (slideshow).
These call DecDCTvlc2 and DecDCTin for decompression.

### Slideshow Frame Structure (FUN_8003a13c)

```c
bool FUN_8003a13c(uint frameIndex) {
    if (frameIndex >= (byte)buffer[0x33800]) return false;
    DecDCTvlc2(
        buffer + *(int*)(buffer + frameIndex * 6 + 0x33806) + 0x67000,
        outputBuffer,
        vlcTable
    );
    return true;
}
```

- Frame count at `buffer + 0x33800` (1 byte)
- Frame offset table at `buffer + 0x33806` (6-byte stride entries)
- Frame data starts at `buffer + 0x67000` + entry offset

### Level Background Source

**Question:** Where does the static level background come from?

**Observations:**
1. Not in Asset 600 (0x258) - that's level geometry/tilemap
2. Not in Asset 200/201 - that's sprites/entities
3. Loading screens use sector table entries with MDEC frames
4. Level backgrounds might be:
   - Embedded in tertiary data
   - Part of a sector table entry loaded separately
   - Composed from tiles only (no static background image)

**Verification needed:** 
- Check if levels have a static MDEC background or just tile-based graphics
- Trace what happens during actual level rendering to VRAM

### Level Background Structure (FINDINGS)

**Conclusion: Skullmonkeys uses tile-based backgrounds, NOT static MDEC images during gameplay.**

Evidence:
1. **Loading screens** (sector table entries) use MDEC frames at 0x80116450
2. **Level backgrounds** use a solid RGB color from Asset 100 or first entity
   - Stored at GameState+0x124, +0x125, +0x126 (R, G, B)
   - Set by FUN_80024778 from first entity entry bytes at +0x2C, +0x2D, +0x2E
3. **Tile-based graphics** fill the playable area on top of the solid color

The MDEC decoder functions (`FUN_800399a8`, `FUN_8003958c`) are only used for:
- Loading screens (before level starts)
- Special screens (credits, game over)
- FMV movies (external .STR files)

No MDEC decompression occurs during normal level gameplay - all graphics are tile/sprite based.

---

## Primary Section Structure Investigation (2026-01-05)

**Status:** PARTIAL - Structure identified but data format unclear

### Primary Section TOC

Each level's primary section starts with a table of contents:

```
Offset  Size  Description
------  ----  -----------
0x00    u32   Asset count (typically 3)
0x04    12×N  Asset entries (12 bytes each)

Each asset entry:
  +00   u32   Asset type ID (0x258, 0x259, 0x25A)
  +04   u32   Offset within primary section
  +08   u32   Size in bytes
```

**Example (PHRO level):**
| Type | Offset | Size | Description |
|------|--------|------|-------------|
| 0x258 | 0x7FF50 | 40 | Level metadata |
| 0x259 | 0x1F120 | 524,152 | Main data blob |
| 0x25A | 0x94 | 651,416 | Encompasses entire section |

Note: 0x25A encompasses 0x258 and 0x259 - offsets are all relative to primary section start.

### Asset 0x258 - Level Metadata (40 bytes)

```
Offset  Size  Field        Value (PHRO)  Description
------  ----  -----        -----------   -----------
0x00    u16   width        2171          Level width in pixels
0x02    u16   height       1114          Level height in pixels
0x04    u16[16] counts     58,26,25...   Unknown (sum=373, possibly layer/entity counts)
0x24    u16   color1       0x7C1F        PSX 15-bit color (gray)
0x26    u16   color2       0x7FFF        PSX 15-bit color (white)
```

The 16 count values (58, 26, 25, 25, 24, 23, 22, 22, 21, 20, 20, 19, 18, 17, 17, 16)
sum to 373. Purpose unclear - possibly parallax layer depths or entity counts per layer.

### Asset 0x259 - Main Data (524 KB for PHRO)

Contains two distinct regions:

**Region 1 (bytes 0 to ~359,000 = 68% of data):**
- Byte values mostly in 0xA0-0xD0 range
- Contains 0x80+ bytes that could be RLE markers
- Does NOT decode correctly with documented RLE format
- Purpose unclear - possibly tile indices or compressed graphics

**Region 2 (bytes ~359,000+):**
Found structured 36-byte records starting around offset 0x57BB8:

```
Offset  Size  Field       Description
------  ----  -----       -----------
0x00    i16   type        Record type (usually 2)
0x02    i16   x_offset    X position offset (small signed value)
0x04    i16   y_offset    Y position offset (small signed value)
0x06    u16   width       Width in pixels (10-60 range)
0x08    u16   height      Height in pixels (10-70 range)
0x0A    18B   unknown     Additional fields (flags, more coords?)
0x1C    u32   data_off    Offset into Region 1
0x20    u32   padding     Usually zeros
```

**Observed records (PHRO):**
| # | Type | Position | Size | Data Offset |
|---|------|----------|------|-------------|
| 0 | 2 | (-4, -1) | 22×21 | 0 |
| 1 | 2 | (-3, -2) | 20×22 | 356 |
| 2 | 2 | (-3, -2) | 20×20 | 668 |
| 3 | 2 | (-4, 1) | 21×18 | 976 |
| 4 | 2 | (-2, 2) | 21×19 | 1288 |

Data offsets increase by ~300-400 bytes, suggesting variable-size sprite data.

### Failed Decoding Attempts

| Attempt | Result |
|---------|--------|
| RLE decode (0x80+ = run length) | Produced 21K pixels instead of ~2800 |
| Render as 8-bit indexed | Noise/static |
| Render as 4-bit indexed | Still noise |
| Render as 1-bit bitmap | Noise |
| Various widths (41, 58, 136, 146) | No recognizable image |

### Hypothesis

The 36-byte records are entity/sprite metadata pointing to compressed sprite data
in Region 1, but the compression format is NOT the simple RLE documented elsewhere.
Possible alternatives:
- Different RLE variant (control word based?)
- LZ-style compression
- Raw tile indices requiring a separate tileset

### Investigation Needed

1. Trace how the game reads this data at runtime (PCSX-Redux)
2. Compare with Secondary section which has known working sprite data
3. Check if the "77 count" at offset 0x28 (after main TOC) relates to these records

---

## Entity Placement System Discovery (2026-01-07) - VERIFIED ✓

**Verified 2026-01-06**: BLB viewer now parses Asset 501 and correctly displays 270 entities for PHRO stage.

Reverse-engineered how entities (enemies, collectibles, triggers) are placed in levels.

### Key Memory Locations (PAL / SLES-01090)

| Address | Description |
|---------|-------------|
| `TileHeader + 0x1E` | Entity count (u16) |
| `LevelDataContext + 0x38` | Pointer to entity data array |

### Asset 501 (0x1F5) is Entity Data, NOT Sprite RLE

The existing documentation incorrectly labels Asset 501 as "SPRITE_RLE".
Asset 501 actually contains **entity placement data** - an array of 24-byte structures.

In `LoadAssetContainer` (Ghidra), Asset 501 sets:
```c
pLevelDataCtx[0xe] = asset_data_ptr;  // offset 0x38 = entity array pointer
```

### Entity Structure (24 bytes / 0x18)

```
Offset  Size  Field         Description
0x00    u16   x1            Bounding box min X (pixels)
0x02    u16   y1            Bounding box min Y (pixels)
0x04    u16   x2            Bounding box max X (pixels)
0x06    u16   y2            Bounding box max Y (pixels)
0x08    u16   x_center      Entity center X (pixels)
0x0A    u16   y_center      Entity center Y (pixels)
0x0C    u32   padding       Always 0
0x10    u16   padding2      Always 0
0x12    u16   entity_type   Type ID (see table below)
0x14    u32   flags         Flags (observed values: 1, 2, 3)
```

### Entity Types Found (Full Game - 10,000+ Entities)

| Type | Count | Size | Notes |
|------|-------|------|-------|
| 2 | 5727 | 16×16 | **Clayballs** (collectible coins) |
| 3 | 308 | 16×16 | **Ammo pickup** (bullets for player weapon) |
| 8 | 144 | 16×16 | **Item pickup** |
| 24 | 227 | 16×16 | **Special ammo pickup** |
| 25 | 152 | 160×80 | **ENEMIES** (user verified) |
| 27 | 60 | 256×80 | **ENEMIES** (user verified) |
| 28 | 99 | 352×128 | Moving platforms |
| 48 | 297 | 352×128 | Moving platforms |
| 45 | - | 160×96 | Message box |
| 42 | 113 | 16×16 | Portal/warp point |

*Entity counts extracted from full GAME.BLB via ImHex (2026-01-10)*

### Related Functions (Ghidra)

| Address | Purpose |
|---------|---------|
| `FUN_8007b7a8` | Returns entity count (`TileHeader + 0x1E`) |
| `FUN_8007b7bc` | Returns entity data pointer (`LevelDataContext + 0x38`) |
| `FUN_80024dc4` | Entity loader - copies 24-byte entities to linked list at param+0x28 |
| `InitializeAndLoadLevel` @ 0x8007d1d0 | Calls entity loader during level init |

### Entity Type → Sprite ID Mapping

**IMPORTANT**: The mapping from entity type (e.g., 2, 45) to sprite ID is NOT in BLB data.
Sprite IDs are **hardcoded in game code** per entity type.

**Evidence:**
- Examined `InitializeAndLoadLevel` @ 0x8007d1d0 - calls entity loader but no sprite assignments
- Examined entity loader `FUN_80024dc4` - only copies 24-byte entity structures to linked list
- Examined `FUN_8001fcf0` (player init) - hardcoded sprite ID `0x21842018`
- Found 6 functions calling `InitSpriteContext` @ 0x8007bc3c - these are object initialization functions
- Discovered `FUN_800281a4` @ 0x800281a4 - initializes multiple objects with sprite IDs

**Sprite Initialization Function Discovered:**

The key sprite init function is **FUN_8001c720** (takes sprite ID as 2nd parameter).

**Known Sprite ID Mappings from FUN_800281a4:**
```
Sprite ID    Sprite Index  Usage Pattern
-----------  ------------  -------------
0xb8700ca1   0x18         Single object initialization
0xe2f188     0x25, 0x31, 0x92, 0x9e, 0xff, 0x10b  (Heavy reuse - UI/menu?)
0xa9240484   0x118, 0xe   Used twice
0xe8628689   0x98         Single use
0x88a28194   0x60         Loop, 3 instances
0x80e85ea0   0xd0         Loop, 3 instances
0x9158a0f6   0x18         Player related?
0x902c0002   0x118        Reused index
0x21842018   (player)     Hardcoded in FUN_8001fcf0
```

**Entity Spawning Architecture (PARTIAL):**

The entity system operates in two stages:

1. **Load Time** (FOUND):
   - `FUN_80024dc4` @ 0x80024dc4 copies Asset 501 entity data
   - Builds linked list at `GameState+0x28` (static entity definitions)
   - Called once by `InitializeAndLoadLevel`
   - Only 1 xref confirms this runs at level load, not runtime

2. **Runtime Spawning** (NOT YET FOUND):
   - Dispatcher runs continuously in game loop
   - Reads entity definitions from list at `GameState+0x28`
   - Checks spawn conditions (distance from player, visibility, already-spawned flag)
   - Switches on entity type field (u16 @ offset 0x12 in 24-byte struct)
   - Calls appropriate init function (e.g., FUN_8001c720) with sprite ID
   - Adds spawned object to active list at `GameState+0x1c`

**Active Object vs Entity Lists:**
- `GameState+0x1c` = Active objects (spawned, updated every frame by FUN_80020e1c)
- `GameState+0x28` = Entity definitions (static, loaded once at level start)

**Missing Link - Spawn Dispatcher Characteristics:**
- Likely uses function pointers (state machine handlers)
- Probably has switch statement on entity type with 20+ cases
- Integrated with visibility/culling system
- May be distributed across multiple functions

**Next Steps to Find Spawn Dispatcher:**
1. Use dataflow analysis on reads from `GameState+0x28` (entity definition list)
2. Search for switch statements with many cases in 0x80020000-0x80030000 range
3. Examine state handler functions pointed to by main loop @ 0x800828b0
4. Set runtime breakpoints on entity list reads using PCSX-Redux MCP
5. Look for functions that read offset 0x12 (entity type field)

Known mappings from user observation:
- **Type 2** = Clayballs (coins/collectibles)
- **Type 45** = Message box

### Visualization Script

Created `scripts/dump_entity_map.py` to render ASCII map of entity placement.
Shows entity positions overlaid on level dimensions.

### Runtime Verification (SCIE Level)

- Entity count: 211 (from `0x8009DCC8 + 0x1E` = TileHeader+0x1E)
- Entity data: `0x8014C704` (from LevelDataContext+0x38)
- Level size: 535×45 tiles (8560×720 pixels)

### Verification Needed

1. ~~Confirm entity types across multiple levels~~ (partially done - types verified in multiple stages)
2. Decode flags field (1, 2, 3 observed - what do they mean?)
3. ~~Map entity types to actual game objects (enemies, items, triggers)~~ (see mapping table above)
4. Verify Type 2 entities - confirmed as clayballs/coins
5. ~~Test with other levels to confirm structure is consistent~~ (verified)

---

## Asset 500 - TILE ATTRIBUTE MAP (2026-01-09) ✓ VERIFIED

**Status:** STRUCTURE-VERIFIED - Asset 500 is a 2D tile attribute/collision map, NOT animation data.

### Structure

Asset 500 contains a per-tile attribute map where each byte represents properties for one tile:

```
Offset  Size               Field         Description
------  ----               -----         -----------
0x00    4                  unknown       Always 0 (padding?)
0x04    2                  level_width   Level width in tiles
0x06    2                  level_height  Level height in tiles
0x08    width * height     tile_data     One byte per tile (row-major order)
```

**Size verification (SCIE level):**
- Header: 8 bytes
- Level dimensions: 535 × 43 = 23,005 tiles
- Total size: 23,013 bytes ✓ (matches actual asset size)

### Tile Attribute Values

| Value | Hex  | Occurrence | Hypothesized Meaning |
|-------|------|------------|---------------------|
| 0     | 0x00 | Most tiles | Empty/passable |
| 2     | 0x02 | Runs       | Solid/collision |
| 18    | 0x12 | Rare       | Unknown trigger |
| 23    | 0x17 | Single     | Special tile type |
| 33    | 0x21 | Single     | Special tile type |
| 34    | 0x22 | Single     | Special tile type |
| 83    | 0x53 | Scattered  | Checkpoint/save point? |
| 101   | 0x65 | Clusters   | Entity spawn zone/trigger |

### Pattern Observations

1. **0x65 (101) values** appear in repeating patterns (groups of 7), often at row boundaries
2. **0x02 values** appear in long runs, likely marking collision geometry
3. **0x53 values** appear periodically, possibly marking save points
4. **Entity positions correlate** - entities often land on tiles with value 0x02 or 0x65

### Connection to Entity System

Entity positions from Asset 501 map to this grid:
- `tile_x = entity.x_center // 16`
- `tile_y = entity.y_center // 16`
- `tile_index = tile_y * level_width + tile_x`

Most entities land on tiles with non-zero values, suggesting this map
controls where entities can spawn or defines collision boundaries.

---

## Entity Variant Field Discovery (2026-01-09)

**Status:** DISCOVERED - Entity structure offset 0x0C contains a variant/subtype field

### Updated Entity Structure (24 bytes)

```
Offset  Size  Field         Description
------  ----  -----         -----------
0x00    2     x1            Bounding box min X (pixels)
0x02    2     y1            Bounding box min Y (pixels)
0x04    2     x2            Bounding box max X (pixels)
0x06    2     y2            Bounding box max Y (pixels)
0x08    2     x_center      Entity center X (pixels)
0x0A    2     y_center      Entity center Y (pixels)
0x0C    2     variant       Animation frame / subtype selector ← NEW
0x0E    2     padding1      Always 0
0x10    2     padding2      Always 0
0x12    2     entity_type   Type ID (2, 8, 25, 45, etc.)
0x14    2     layer         Render layer (1, 2, or 3)
0x16    2     padding3      Always 0
```

### Variant Values by Entity Type

| Entity Type | Variant Range | Notes |
|-------------|---------------|-------|
| 2 (clayballs) | 0, 3, 4, 5, 6, 7, 8 | Animation frame cycle? |
| 3 | 0 | Single variant |
| 8 | 0 | Single variant |
| 10 | 0 | Single variant |
| 24 | 0 | Single variant |
| 25 (enemy) | 0 | Single variant |
| 27 (enemy) | 0 | Single variant |
| 28 | 1, 2 | Direction/state? |
| 42 | 2, 3 | Subtype selection |
| 45 (message) | 0, 1, 2, 12, 13, 14 | Message ID |
| 48 | 1, 2 | Direction/state? |

**Significance:** The variant field may determine:
- Which sprite frame to use
- Animation starting point
- Subtype/behavior within entity type
- Message content for type 45

---

## Sprite ID Lookup Chain (2026-01-09)

**Status:** CODE-TRACED - Complete sprite lookup chain documented from Ghidra

### Function Call Chain

```
FUN_8001cdac(context, priority, sprite_id)
    │
    └─► InitSpriteContext(context+0x78, sprite_id) @ 0x8007bc3c
            │
            └─► LookupSpriteById(sprite_id) @ 0x8007bb10
                    │
                    ├─► FindSpriteInTOC(DAT_800a6064, sprite_id) @ 0x8007b968
                    │   (Searches primary sprite table - tertiary container)
                    │
                    └─► Fallback: Search DAT_800a6060
                        (Secondary sprite table)
```

### FindSpriteInTOC Algorithm

```c
// Searches TOC at context+0x70 and context+0x40
// Each TOC entry is 12 bytes (3 uint32):
//   [0] = count (at base)
//   [1] = sprite_id (32-bit hash)
//   [2] = offset to sprite data
// Returns: base + offset when sprite_id matches
```

### Sprite IDs are 32-bit Identifiers


Sprite IDs in Asset 600 TOC and game code are 32-bit hash-like values:
- `0x09406D8A` - Common sprite (appears in all levels)
- `0x400C989D`, `0x400C991D` - Related sprites
- `0x5A89815F`, `0x5AB9815F`, `0x5AD9815F` - Family of sprites
- `0x21842018` - Player sprite (hardcoded in FUN_8001fcf0)
- `0xe2f188` - UI/menu element (heavily reused)

### Conclusion: Entity→Sprite Mapping is in Code - ✓ VERIFIED

**Status:** VERIFIED via Ghidra decompilation (2026-01-07)

The mapping from entity type (2, 8, 25, etc.) to sprite ID (0x09406D8A, etc.)
is **hardcoded in game code**, not stored in BLB data.

**Verification evidence:**
1. `FUN_8001fcf0` (player init) uses hardcoded sprite ID `0x21842018`
2. `FUN_800281a4` initializes multiple objects with hardcoded sprite IDs:
   - `0xb8700ca1`, `0xe2f188`, `0xa9240484`, `0xe8628689`
   - `0x88a28194`, `0x80e85ea0`, `0x9158a0f6`, `0x902c0002`
3. Sprite lookup chain confirmed:
   - `InitSpriteContext` @ 0x8007bc3c
   - `LookupSpriteById` @ 0x8007bb10
   - `FindSpriteInTOC` @ 0x8007b968

**Template updated:** `docs/blb.hexpat` now includes:
- Entity structure (24 bytes) for Asset 501
- Sprite ID lookup chain documentation
- Known sprite ID mappings

To build a mapping table for the BLB viewer:
1. Trace each entity type's spawn dispatcher in Ghidra
2. Extract the sprite ID constant passed to InitSpriteContext
3. Create a static lookup table: `entity_type → sprite_id`

**Known mappings:**
- Type 2 = Clayballs → sprite ID TBD
- Type 45 = Message box → sprite ID TBD
- Player → `0x21842018`

---

## Entity Dispatch Investigation (2026-01-10) - NO CENTRAL TABLE FOUND

**Status:** INVESTIGATED - No single dispatch table exists; dispatch is distributed

### Investigation Summary

Exhaustive search for entity dispatch table yielded these findings:

1. **Entity Definition Structure (24 bytes at GameState+0x28 list):**
   ```
   Offset 0x00-0x0B: Bounding box/position (12 bytes) 
   Offset 0x0C-0x0F: Unknown value (4 bytes)
   Offset 0x10-0x11: Unknown (2 bytes) - always 00 00
   Offset 0x12-0x13: Entity TYPE (u16) - values 5, 7, 8 observed at runtime
   Offset 0x14-0x15: Flags (2 bytes) - values 02, 03 observed
   Offset 0x16-0x17: Spawn status flag (can be cleared by FUN_8007eed8)
   ```

2. **Runtime Memory Verification (PAL):**
   - GameState @ 0x8009dc40
   - GameState+0x1c = 0x801a4c04 (active entity list head)
   - GameState+0x28 = 0x8017d2c4 (entity definitions list head)
   - Entity types 5, 7, 8 confirmed in live memory

3. **No Central Dispatch Table Found:**
   - Searched 0x80020000-0x80030000 for switch statements
   - Traced 178 callers of `FUN_800213a8` (AddEntityToActiveList)
   - No function reads entity type and dispatches to different init functions
   
4. **Dispatch is Distributed by Category:**
   | Category | Spawner Function | How Init is Selected |
   |----------|-----------------|---------------------|
   | Player | FUN_80024f34 | Direct call to InitPlayerEntity |
   | Menu/UI | FUN_800281a4, FUN_80078200 | Hardcoded sprite IDs |
   | HUD items | FUN_8002b22c | Hardcoded sprite IDs |
   | Boss | (via 0x80081020) | Direct call to InitMonkeyMageBoss |
   | Child entities | Various update funcs | Parent spawns with hardcoded ID |

5. **FUN_8007eed8 Checks Entity Types (but doesn't dispatch):**
   - Checks types 8, 32 (0x20) in active list (GameState+0x1c)
   - Checks types 28-29, 57-59 in definition list (GameState+0x28)
   - Only sets/clears flags, doesn't call init functions

### Hypothesis: Stage Entities Use Different Mechanism

**Observation:** The entity types (5, 7, 8) seen in the definition list don't match
the pattern of menu/player spawning (which use sprite IDs directly).

**Possible mechanisms for stage entity dispatch:**
1. Entity type → sprite ID mapping stored in BLB Asset 501 itself
2. Entity type embedded in tilemap layer data (layers 8-11)
3. A runtime lookup function that maps type to sprite ID
4. Each level has level-specific spawn code

### Key Functions to Investigate Further

| Address | Function | Role |
|---------|----------|------|
| 0x80024dc4 | FUN_80024dc4 | Loads entity defs from Asset 501 → GameState+0x28 |
| 0x8007b7a8 | FUN_8007b7a8 | Gets entity count from level data |
| 0x8007b7bc | FUN_8007b7bc | Gets entity data pointer (GameState+0x84+0x38) |
| 0x800250c8 | FUN_800250c8 | Adds pre-init entities to active list |
| 0x80024468 | FUN_80024468 | Iterates def list, removes entities with flag 0x16 bit 2 |

### Next Steps

1. **Examine Asset 501 raw structure** - does it contain sprite IDs?
2. **Check if entity type IS a sprite index** into some table
3. **Trace FUN_8007b7a8/FUN_8007b7bc** - how is entity data interpreted?
4. **Runtime breakpoint** on entity def reads during gameplay

---

*Last updated: January 10, 2026*


## InitEntityStruct (@0x8001A0C8) — near-match shelved (register coloring)

The base entity constructor is fully decoded but **not yet byte-matched**; left
as `INCLUDE_ASM` in `src/entity.c` (build stays green). Closest C draft +
permuter harness: `nonmatchings/InitEntityStruct/` (`base.c`).

What is confirmed (instruction-for-instruction, same count/order/offsets/imms):
- Each FSM `[marker, callback]` pair is written as **two `s16` halves**
  (`sh` lo/hi) + the `sw` callback — i.e. a `{ s16 markerLo; s16 markerHi;
  void(*fn)(); }` slot, NOT the `s32 marker` the global `Entity` struct shows.
  (The global struct must stay `s32` for the dispatch readers, so the
  constructor uses a local `FsmSlot` overlay cast.)
- The event slot is installed via the **stack-staged 8-byte struct copy**
  idiom (`{0,-1,StubReturnZero}` → `eventMarker`), slot at `sp+4`.
- A **padded local wrapper pins the 0x38 stack frame** (bare 8-byte slot only
  yields 0x8): `struct { s32 pad; FsmSlot s; u8 tail[0x2C]; }`.
- Six 16.16 scale fields (0x50..0x64) primed to 0x10000; field write order in
  the C must follow the non-monotonic asm order exactly.

Sole remaining diff: **register coloring**. The original holds the entity
pointer in `$v0` (`move $v0,$a0` at entry) and loads `StubReturnZero` into
`$a0`, so the event-slot struct copy's `lw $a0,8($sp)` clobbers `$a0` while the
entity survives in `$v0`. Our codegen keeps the entity in `$a0` and uses
`$v0/$v1` as the copy temps — a clean `{v0,v1,a0}` rotation, identical
otherwise. `__asm__ ("":"=r"(p):"0"(p))` barriers are a no-op here; `+r` is
rejected by gcc 2.7.2. A 150s permuter run (default weights) plateaued ~1270
without reaching 0 — needs a targeted decl-order / pointer-staging idiom (cf.
the solved `InitScrollingLayerEntity` case study in compiler-quirks.md 6i/6j),
or a longer permuter run with `perm_reorder_decls`/`perm_ins_block` boosted.

NOTE (env): during this session an external process repeatedly reverted
uncommitted edits to `src/entity.c` back to HEAD. Commit promptly when the
match lands.

## Doc↔symbol_addrs address/name drift (re-review 2026-06-29, Ghidra-needed)

The docs-vs-`symbol_addrs.txt` re-review found 12 mismatches where a doc cites
`Name @ 0xADDR` but `0xADDR` now holds a *different* function and `Name` lives
at a *different* address. Each needs per-doc context + the Ghidra/disassembly
behavior to decide whether the **name** drifted (→ rename doc to the symbol at
that address) or the **address** drifted (→ fix the doc address to where the
named function actually lives). NOT auto-swept — the camera case
(`UpdateCameraPosition`) proved blind sweeps mislabel when two similar
functions exist.

| Doc cites (name @ addr) | `0xADDR` actually holds | That name actually lives at | Likely |
|---|---|---|---|
| `CollisionCheckWrapper @0x8001b594` | `CheckEntityPointCollisionWithOffsets` | `0x8001b47c` | name drift (same domain) |
| `EntityFollowPathWithWrapping @0x80055c70` | `InterpolateTimedPathPosition` | `0x8003cc60` | name drift |
| `EntityType080_TimerMenu_Init @0x800809b8` | `EntityType083_InteractiveDecor_Init` | `0x80080ebc` | name drift (type# is addr-anchored) |
| `InitCollectibleEntity @0x800418a4` | `InitCollectibleEntity_Alt` | `0x8003a724` | name drift (the _Alt variant) |
| `InitLevelDataContext @0x8007a218` | `ClearContextOffsets68to7C` | `0x8007a1bc` | addr drift? (Δ0x5c) |
| `InitMenuEntity @0x800313cc` | `InitMenuSpriteEntity` | `0x80076928` | name drift (doc notes "second/different") |
| `InitializeAndLoadLevel @0x8007cd34` | `InitGameState` | `0x8007d1d0` | ambiguous |
| `IsEntityOffScreen @0x8002453c` | `IsEntityOffScreen_EntityLoop` | `0x8001b92c` | name drift (loop variant) |
| `LoadBLBHeader @0x80020848` | `BLB_ReadSectorsWrapper` | `0x800208b0` | addr drift? (Δ0x68) |
| `MultiPartEntityMessageHandler @0x800389ac` | `FreeEntityNoTeardown_800389ac` | `0x80032800` | semantic mismatch |
| `PlayMovieFromCD @0x80039128` | `CdPausePlayback` | `0x80039150` | addr drift? (Δ0x28) |
| `UploadAudioToSPU @0x8007bfb8` | `InitSPUDefaults` | `0x8007c088` | addr drift? (Δ0xd0) |

Resolution belongs in the per-doc `review-docs` loop (Ghidra now available on
:8089): decompile `0xADDR`, compare its behavior to the doc's surrounding
description, then fix the wrong half. (Already resolved in this pass:
`UpdateCameraPosition`→`UpdateCameraPositionSmooth @0x800233c0` — Ghidra-verified.)


## Doc↔code name/address drift — bulk catalog (audit 2026-06-29)

An automated audit of all docs against `symbol_addrs.txt` / `src` / `include`
surfaced **94** further `Name @ 0xADDR` or behavior claims that need
Ghidra/disassembly to resolve: the doc and the symbol table disagree, and names
alone can't say which side drifted (same caveat as the 12-item table above). Each
row is a per-doc review item — decompile the address, compare to the doc's prose,
then fix the wrong half. Mechanically-safe fixes from the same audit were already
applied; these are the judgment calls that remain. Grouped by file.

### `docs/ANCIENT_NOTES.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| GetMovieFilename \| 0x8007ADC8 \| 0xF38+n (mode) \| 0xF94+n (index) \| Movie table \| \| GetMovieUnknown00 \| 0x80… | symbol_addrs.txt: 0x8007ADC8 = GetCurrentMovieFilename (doc calls it GetMovieFilename); 0x8007AE14 = GetMovieFrameField00 (doc calls it GetMovieUnknown00). The address… |

### `docs/C_MIGRATION_GUIDE.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | \| `GetAssetCount` \| 0x8007b4d4 \| 20 \| Very simple \| | symbol_addrs.txt has `GetAssetCount = 0x8007ACDC; // size:0x14` (size 0x14 = 20, matching the doc's size column). The doc's address 0x8007b4d4 is not owned by any symb… |

### `docs/PASSWORD_SYSTEM_ANALYSIS.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | \| `DecrementPlayerLives` \| 0x80081e84 \| Lives management \| | Address 0x80081E84 is owned by ClearAlternateEntitySpawnFlag in symbols.txt, and no symbol named DecrementPlayerLives exists. Cannot determine from names alone whether… |
| stale-name | \| `ResetPlayerUnlocksByLevel` \| 0x80026162 \| Reset powerups by level \| | Neither the name ResetPlayerUnlocksByLevel nor the address 0x80026162 appears in symbols.txt. Unverifiable against the curated symbol set; flag for Ghidra confirmation. |

### `docs/PHYSICS_QUICK_REFERENCE.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | // Verified from EntityFallingGravityWithCollision @ 0x8003dad8 | No symbol named EntityFallingGravityWithCollision exists in symbols.txt, and address 0x8003DAD8 is not owned by any symbol there. The 'Verified from <name> @ <addr>' c… |

### `docs/README.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | \| 0x800AE3E0 \| blbHeaderBuffer \| BLB header loaded at game boot \| \| 0x8009DCC4 \| LevelDataContext \| Level load… | None of these four data-symbol names (blbHeaderBuffer, LevelDataContext, g_GameBLBFile, g_GameBLBSector) appear in symbol_addrs.txt, and none of the four addresses (0x… |

### `docs/SOURCE_STRUCTURE_PLAN.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | │   └── camera.c                  # UpdateCameraPosition @ 0x80023DBC, scrolling | 0x80023DBC is owned by SetCameraPositionDirect in symbols.txt; no symbol named UpdateCameraPosition exists (nearest UpdateCameraPositionSmooth = 0x800233C0). Same name… |

### `docs/analysis/asset-identification/family-identifications.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | **ALIAS cluster** (all 3468 B, byte-identical, +`0x326e0410`); `ProjectileState_HomingActiveVariant2` | family-identifications.md attributes the head-shooter-monkey projectile id 0x410808f9 to ProjectileState_HomingActiveVariant2 (also at line 152), but family-structure-… |

### `docs/analysis/asset-identification/frame-event-breakpoint-plan.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| `0x182D840C` \| `0x8005DEFC` \| PlayerCallback_CheckpointSaveAndSpawnHUD (also `0x8006DE60` ScaleBoundsAndCollisio… | No symbol named 'ScaleBoundsAndCollision' exists in symbol_addrs.txt; the closest is PlayerCallback_ScaleBoundsAndCollision (= 0x800622D0). The bp address 0x8006DE60 c… |

### `docs/analysis/asset-identification/sprite-id-table-8009b144.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | The access pattern is real. `InitAlternateEntity @0x80033dd4` (asm `0x80033dc0`): | InitAlternateEntity is at 0x80033D3C in live symbol_addrs.txt (= 0x80033D3C; // size:0x120), not 0x80033dd4 and not 0x80033dc0. Both cited addresses are unassigned (no… |

### `docs/analysis/binary-layout-findings.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x80078200 \| 5,180 \| CreateBossPlayerEntity \| | Address 0x80078200 is owned by 'CreateResultsScreenEntity = 0x80078200; // size:0x143C' (0x143C = 5180 bytes, matching the size column). No symbol named CreateBossPlay… |

### `docs/analysis/function-batches-to-analyze.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | \| 0x80020848 \| LoadBLBHeader \| ✅ Already named \| 30 \| Load BLB header \| | Ground truth (symbol_addrs.txt): LoadBLBHeader = 0x800208B0 (size 0xC4), while 0x80020848 is owned by a different named symbol BLB_ReadSectorsWrapper = 0x80020848 (siz… |
| stale-address | \| 0x8007cd34 \| InitializeAndLoadLevel \| ✅ Already named \| 200 \| Main level loader \| | Ground truth: InitializeAndLoadLevel = 0x8007D1D0 (size 0x6D0), while 0x8007CD34 is owned by a different named symbol InitGameState = 0x8007CD34 (size 0x28C). The sibl… |
| stale-address | \| 0x8007f270 \| LoadLevelSpriteAssets \| ✅ Already named \| 80 \| Load sprites/sequences \| | Ground truth: LoadLevelSpriteAssets = 0x80081BAC (size 0x60 = 96 bytes). 0x8007F270 is NOT owned by any symbol in symbol_addrs.txt (unassigned). Name is clearly descri… |
| stale-address | \| 0x8007ca60 \| StartCDAudioForLevel \| ✅ Already named \| 15 \| Start CD music for level \| | Ground truth: StartCDAudioForLevel = 0x8007CA9C (size 0x84). 0x8007CA60 is unassigned and falls inside SetVoicePanning's range (SetVoicePanning = 0x8007CA28, size 0x74… |
| stale-address | \| 0x8007c7e0 \| StopAllSPUVoices \| ✅ Already named \| 6 \| Stop all 24 voices \| | Ground truth: StopAllSPUVoices = 0x8007C7EC (size 0x2C), not 0x8007C7E0. 0x8007C7E0 is unassigned (between StopSPUVoice=0x8007C7B8 and StopAllSPUVoices=0x8007C7EC). Sm… |

### `docs/analysis/unconfirmed-findings.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| decomp-status | ## Doc↔symbol_addrs address/name drift (re-review 2026-06-29, Ghidra-needed) | This bottom table (lines 2671-2700) is NOT stale: I verified all 12 rows against symbol_addrs.txt and each is internally accurate (e.g. CollisionCheckWrapper lives at … |

### `docs/ghidra/struct-inventory.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| `0x800A595B` \| `g_SkipVSync` \| `u8` \| | symbol_addrs.txt assigns 0x800A595B to g_SwapInFlight ('byte flag, nonzero while a buffer swap is queued'), not g_SkipVSync. The doc records a Ghidra-session rename to… |

### `docs/ghidra/struct-workflow.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | Player state structure initialized by initPlayerState @ 0x8001F3B4 | symbol_addrs.txt has initPlayerState = 0x800260D0 (size 0x8C). Address 0x8001F3B4 is unassigned in symbol_addrs.txt. The doc cites 0x8001F3B4 in two places (line 29 'd… |

### `docs/gist/asset-system/SESSION14_audio_banks.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | ## g_SoundTable layout (UploadAudioToSPU @ 0x8007bfb8) | symbol_addrs.txt has `UploadAudioToSPU = 0x8007C088`, not 0x8007bfb8. Address 0x8007BFB8 is owned by a DIFFERENT symbol: `InitSPUDefaults = 0x8007BFB8`. The doc body a… |

### `docs/gist/asset-system/SESSION24_enemy_ai.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | InitGroundPatrolEnemy 0x8002ea3c (Clay Keeper, Loud Mouth) · InitEnemyEntityWithAI 0x8004f8dc · | symbol_addrs.txt maps 0x8002EA3C to `InitPhartHeadCollectible` (a pickup-collectible init, surrounded by other Collectible* inits), NOT a ground-patrol enemy. The name… |

### `docs/newly-discovered-functions-2026-01-19.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x8004c464 \| EntityDestructor_WithVoiceReset \| Resets voice ID to -1 before destroying \| | symbols.txt has JoeHeadJoeBossDestructor = 0x8004C464 (spelled JoeHeadJoeBoDetructor), not EntityDestructor_WithVoiceReset. Same address, different name. Many other ad… |

### `docs/plans/main-and-libs-extraction.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x80082E90 \| 0x73690 \| 0x28 \| `DestroySpecificEntity` \| Medium \| | 0x80082E90 is owned by `DestroyStaticGameState = 0x80082E90; // size:0x28`. The name 'DestroySpecificEntity' does not exist anywhere in symbol_addrs.txt or src/. Per c… |
| stale-name | \| 0x8008E6E0 \| `__main` \| LIBS \| C runtime init \| | 0x8008E6E0 is owned by `main_8008E6E0 = 0x8008E6E0; // size:0x70` (an auto-placeholder name); '__main' is not present in symbol_addrs.txt. The doc may be semantically … |
| stale-name | - `GetLevelDisplayName` (0x8007A9E8, 32 bytes) - trivial | 0x8007A9E8 is UNASSIGNED in symbol_addrs.txt (no symbol owns that address). 'GetLevelDisplayName' is not present either; the closest real symbol is `GetCurrentLevelDis… |

### `docs/reference/entity-event-ids.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | \| 3 \| Game-progression notification (sent to **GameState**'s event slot @ +0x08/+0x0C) \| `TriggerCollectible100CTi… | `InvokeGameStateCallback` appears nowhere in /tmp/docaudit/symbols.txt or live symbol_addrs.txt. Address 0x80034B2C is not owned by any symbol (unassigned; nearest sym… |

### `docs/reference/entity-types.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | \| 2 \| Green Bullets \| 5,727 \| Collectible ammunition/bullets \| 0xe8628689 \| | Cross-doc identity conflict on BLB type 2. entity-types.md calls type 2 'Green Bullets' (sprite 0xe8628689); items.md (Clay section) calls type 2 'Clay (Clayball)' wit… |

### `docs/reference/functions-complete.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | \| 0x80020974 \| **AddToZOrderList** \| Add entity to z-order list \| 100% \| | symbol_addrs.txt: AddToZOrderList is actually at 0x80020F68; address 0x80020974 is owned by DestroyEntity. game-functions.md correctly lists AddToZOrderList @ 0x80020F… |
| stale-address | \| 0x80020a1c \| **AddToUpdateQueue** \| Add entity to update queue \| 100% \| | symbol_addrs.txt: AddToUpdateQueue is actually at 0x80021190; address 0x80020A1C is owned by RemoveFromUpdateQueue. game-functions.md correctly lists AddToUpdateQueue … |
| stale-address | \| 0x80039128 \| PlayMovieFromCD \| Play external STR movie \| 100% \| | symbol_addrs.txt: PlayMovieFromCD is at 0x80039150; address 0x80039128 is owned by CdPausePlayback. Clean-room: 0x80039128 is owned by another named function, so canno… |
| stale-name | \| 0x80019650 \| GetLayerProperty \| Get layer field \| 95% \| | symbol_addrs.txt: address 0x80019650 is owned by GetSpriteFrameDataByIndex; no symbol GetLayerProperty exists. The doc's name has been superseded (and the 'layer' desc… |
| stale-address | \| 0x80019700 \| **ClearAllLayerRenderSlots** \| Initialize 20 layer slots \| 100% \| | symbol_addrs.txt: ClearAllLayerRenderSlots is at 0x80018D54; address 0x80019700 is owned by ClearAllLayerRenderSlots_CrtInit. Same base name, different address/variant… |

### `docs/reference/game-functions.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x80052678 \| InitEntity_a89d0ad0 \| Unknown entity \| | symbol_addrs.txt: address 0x80052678 is owned by InitAuraEffectAtPlayer (size 0x2D4). There is no symbol InitEntity_a89d0ad0. The doc's generic InitEntity_<hash> place… |

### `docs/reference/items.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | - Init Callback: `EntityType008_InitCallback` @ 0x80081504 (empty stub) | Address 0x80081504 is owned by EntityType008_KloggCatchableBall_Init (size 0x8, an empty stub) in symbol_addrs.txt; there is no symbol named EntityType008_InitCallback… |
| stale-name | - Init Callback: `EntityType095_InitCallback` @ 0x800814A4 | Address 0x800814A4 is owned by EntityType095_SoundEmitterPanning_Init (size 0x60) in symbol_addrs.txt; there is no symbol named EntityType095_InitCallback. The doc's g… |

### `docs/reference/physics-constants.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | \| **Jump Sound** \| `0x248E52` \| Played on jump \| Line 31589 \| | include/Game/asset_ids.h line 106 defines 0x00248e52 as FX_KLAY_RUN_FAST (a run/footstep SFX), not a jump sound. asset-hash-ids.md round 3 calls 0x00248e52 'jump-sound… |
| false-claim | if (g_DefaultBGColorB & entity_input_flags) { | This pseudocode uses g_DefaultBGColorB (defined in game-functions.md as the 'Default BG blue component', 1 byte at 0x800A5772) as a bitmask ANDed with entity input fla… |

### `docs/reference/unknown-functions.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x80081c0c \| `GetLevelSoundTableEntry` \| Sound lookup table accessor \| | Address 0x80081C0C is now owned by a differently-named, semantically-different function: live symbol_addrs.txt line 1825 `GetSlopeHeightAtSubpixel = 0x80081C0C` (size … |

### `docs/sessions/2026-01-19-function-naming-complete.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | ### Sine/Cosine Lookup Table - **Address**: 0x8009c09c | No symbol exists at 0x8009C09C in symbol_addrs.txt (grep '= 0x8009C09C;' returns nothing), nor a CalculateSineValue symbol (grep '^CalculateSineValue =' returns nothin… |

### `docs/systems/camera.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | \| `+0x12A` \| 1 \| `screen_offset` \| Screen offset parameter \| | include/Game/game_state.h labels offset 0x12A as 'glide_boss_flag' (/* 0x12A */ u8 glide_boss_flag; /* Glide/boss/FINN flag (zeroed in SpawnPlayerAndEntities) */), not… |

### `docs/systems/checkpoint-system.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | ### ClearSaveSlotFlags @ 0x80081E84  **Address**: `0x80081E84` (line 42491 in decompiled source)   **Purpose**: Clear… | Address 0x80081E84 is owned by `ClearAlternateEntitySpawnFlags` (symbol_addrs.txt), implemented in src/entinit.c:942 as a loop that zeroes a `spawned` byte across an a… |
| stale-address | \| `RemoveFromTickList` \| 0x80021190 \| Remove entity from tick list \| | symbol_addrs.txt has RemoveFromTickList = 0x80021D30. Address 0x80021190 is owned by a different symbol, `AddToUpdateQueue`. The doc's own prose also calls RemoveFromT… |

### `docs/systems/collectibles/COLLECTIBLES_COMPLETE.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| `AdjustPlayerStats` \| 0x80026954 \| +0x1a, +0x1b \| 3, 48 \| | 0x80026954 is owned by 'AwardSwirlyQsAndHamsters' in symbol_addrs.txt; the name 'AdjustPlayerStats' does not exist anywhere in symbol_addrs.txt. The address is correct… |
| stale-name | - **Callback**: `TriggerCollectible100DTickCallback` @ 0x8002f274 | 0x8002f274 is owned by 'CollectibleScaleResetTickCallback' in symbol_addrs.txt; the name 'TriggerCollectible100DTickCallback' does not exist in symbol_addrs.txt (its s… |
| stale-name | \| **11** \| Extra Life (Klaymen's Head) \| `0xa9240484` \| +0x11 \| 99 \| `InitTransparentDecorEntity` \| | 'InitTransparentDecorEntity' does not exist as a symbol in symbol_addrs.txt (grep -i 'InitTransparentDecorEntity' returns nothing). Every other Init* function in this … |

### `docs/systems/combat-system.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | From `ProjectileCollisionCallback` @ 0x80052adc: | symbols.txt names 0x80052ADC as ProjectileKeyframeEventHandler, not ProjectileCollisionCallback. The whole 'Projectile vs Enemy Collision' subsection (lines 603-620) a… |
| stale-name | 5. Set recovery callback (`PlayerCallback_800650c4`) | symbols.txt names 0x800650C4 as PlayerCallback_FallingPhysicsMain ('PlayerCallback_FallingPhyicMain'). The doc repeatedly (lines 124, 422, 528) labels 0x800650c4 the d… |

### `docs/systems/demo-attract-mode.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| other | **The demo replay data is stored in Asset 700 (0x2BC) in the tertiary segment (stage0).**  ### Asset 700 = Demo Repla… | The committed header include/Game/sound.h comment still states 'Asset 700 = SPU DMA audio data (LevelDataContext+0x54 / ctx[21])'. The demo doc claims (2026-01-19, wit… |

### `docs/systems/enemies/RIGOROUS_ENTITY_ANALYSIS.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 5 \| 0x8007f7b0 \| EntityType005_FlyingEnemy_Init \| InitGenericSpriteEntity \| Flying enemy variant \| \| 6 \| 0x… | Addresses match symbols.txt (0x8007F7B0, 0x8007F830) but the names/purposes drifted: symbols.txt has EntityType005_MovingPlatformA_Init and EntityType006_MovingPlatfor… |
| stale-name | **Pattern**: All are enemy clusters (Clay Keeper, Loud Mouth, Mental Monkey variants). All vulnerable to butt-bounce. | Doc claims Types 17-23 are 'all enemy clusters'. symbols.txt shows a mix at those exact addresses: EntityType017_EnemyCluter_Init (0x8007F930), EntityType018_EnemyClut… |
| stale-name | \| 7 \| 0x80080408 \| EntityType007_ItemCollectible_Init \| InitRandomColorDecorEntity \| Colored decoration/collecti… | Init-function names InitRandomColorDecorEntity and InitTransparentDecorEntity do not exist anywhere in symbols.txt (only InitPlatformDecorEntity @ 0x8002DEC0 exists; n… |

### `docs/systems/enemies/type-002-clayball.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 2: Clayball ... **Callback**: 0x80080328 ... **Sprite ID**: 0x09406d8a ... **Count**: 5,727 instances (… | Two newer ground-truth sources both identify internal Type 2 as Green Bullet, not Clayball: (1) symbols.txt has 'EntityType002_GreenBullet_Init = 0x80080328' (matches … |

### `docs/systems/enemies/type-005-012-collectibles.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | ## Type 5 - Collectible A  **Callback**: 0x8007f7b0   **Pattern**: Stationary collectible   **Behavior**: Similar to … | Callback 0x8007F7B0 matches but symbols.txt names it EntityType005_MovingPlatformA_Init (not a collectible). Likewise Type 6 (0x8007F830 = EntityType006_MovingPlatform… |

### `docs/systems/enemies/type-008-item-pickup.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 8: Item Pickup ... **Callback**: 0x80081504 ... **Sprite ID**: 0x0c34aa22 | symbols.txt has 'EntityType008_KloggCatchableBall_Init = 0x80081504' (matches doc callback). The inferred symbol name 'KloggCatchableBall' (a boss-fight object) disagr… |

### `docs/systems/enemies/type-010-interactive-object.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Callback**: 0x8007f244   **Sprite ID**: Unknown (needs extraction)   **Category**: Interactive Object | Callback address 0x8007F244 matches but symbols.txt names it EntityType010_EggBeater_Init, which reads like an enemy/object named 'EggBeater', whereas the doc frames T… |

### `docs/systems/enemies/type-025-enemy-a.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | # Entity Type 25: EnemyA (Ground Patrol) ... EnemyA is a standard ground-based enemy that patrols horizontally ... **… | Three ground-truth sources identify internal Type 25 as a COLLECTIBLE (Phart Head), not a ground-patrol enemy: (1) symbols.txt 'EntityType025_PhartHeadCollectible_Init… |

### `docs/systems/enemies/type-026-029-030-enemies.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | ## Type 30 - Enemy E  **Callback**: 0x80080a98   **Sprite ID**: Unknown   **Behavior**: Sequential with Type 29 | Callback 0x80080A98 matches but symbols.txt names it EntityType030_CollectibleAlt_Init (a collectible), not an enemy. Similarly the doc's 'Type 29 - Enemy D' (0x800806… |

### `docs/systems/enemies/type-028-platform-a.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 28: Platform A (Moving Platform) ... **Callback**: 0x80080638 ... Moving platforms that carry the playe… | symbols.txt has 'EntityType028_GreenHeart_Init = 0x80080638' (matches doc callback 0x80080638). docs/reference/official-enemy-names.md line 167 lists 'Green Hearts \| … |

### `docs/systems/enemies/type-031-036-object-variants.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Callbacks**: 0x80080af8 (31-33), 0x80080b60 (34-36)   **Category**: Interactive Objects | Both callback addresses match symbols.txt: 0x80080AF8 = EntityType031_032_033_ScaledMovingPlatform_Init, 0x80080B60 = EntityType034_035_036_TriggerPlatform_Init. Curre… |

### `docs/systems/enemies/type-037-041-mechanisms.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Callbacks**: 0x80080bc8 (37-38), 0x80080c8c (39), 0x80080cfc (40), 0x80080d6c (41) | Callback addresses all match symbols.txt, but the inferred identities conflict with current symbol names: 0x80080bc8 = EntityType037_038_DirectionalScaled_Init (doc: '… |

### `docs/systems/enemies/type-042-portal.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 42: Portal (Level Exit) ... **Callback**: 0x80080ddc (shared with particle types) | symbols.txt has 'EntityType042_thru_060_SnoBlo_Init = 0x80080DDC' (matches doc callback). The current symbol names this family 'SnoBlo', not 'Portal'. The doc's 'share… |

### `docs/systems/enemies/type-045-message.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 45: Message Box ... **Callback**: 0x80080f1c | symbols.txt has 'EntityType045_BounceClay_Init = 0x80080F1C' (matches doc callback 0x80080f1c). The inferred symbol name 'BounceClay' disagrees with this doc's 'Messag… |

### `docs/systems/enemies/type-060-particle.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 60: Generic Particle ... **Callback**: 0x80080ddc | symbols.txt has 'EntityType042_thru_060_SnoBlo_Init = 0x80080DDC' (matches doc callback; the '042_thru_060' span includes type 60, consistent with the doc's shared-cal… |

### `docs/systems/enemies/type-061-sparkle.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | # Entity Type 61: Sparkle Effect ... **Callback**: 0x80080718 ... **Sprite ID**: 0x6a351094 | symbols.txt has 'EntityType061_UnivereEnema_Init = 0x80080718' (matches doc callback). The inferred symbol name 'UnivereEnema' (Universe Enema, a player powerup) disag… |

### `docs/systems/enemies/types-043-044-053-055-portal-particle-variants.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Shared Callback**: 0x80080ddc   **Category**: Visual Effects / Interactive Objects   **Status**: ⚠️ Variant analysi… | Callback 0x80080DDC matches and is shared, but symbols.txt names it EntityType042_thru_060_SnoBlo_Init ('SnoBlo'), spanning types 42-60. The doc frames the callback as… |

### `docs/systems/enemies/types-086-088-variant-group-a.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | **Category**: Unknown (likely decorations or collectibles) | Callback 0x8007F050 matches but symbols.txt now names it EntityType086_087_088_InviibleHazard_Init ('InvisibleHazard'), i.e. a hazard, not a decoration/collectible. Th… |

### `docs/systems/enemy-ai-overview.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | \| `InitGroundPatrolEnemy` \| 0x8002ea3c \| Initialize ground-based patrol enemies (Clay Keeper, Loud Mouth) \| | symbol_addrs.txt has no symbol named InitGroundPatrolEnemy at all. Address 0x8002EA3C is owned by `InitPhartHeadCollectible` (a collectible item init, size 0x200), not… |
| false-claim | \| `EntityType025_GroundPatrolEnemy_Init` \| 0x800805c8 \| Type 25 factory callback \| | Address 0x800805C8 is owned by `EntityType025_PhartHeadCollectible_Init` (symbol_addrs.txt) and listed as `EntityType025_PhartHead_Init` / Collectible in docs/referenc… |
| false-claim | \| `EntityType025_GroundPatrolEnemy_Init` \| 0x800805c8 - EnemyA (ground patrol) | In the 'Entity Callback Table Reference / Confirmed Enemy Types' section the doc lists 'Type 25 @ 0x800805c8 - EnemyA (ground patrol)'. Ground truth (symbol_addrs.txt:… |
| false-claim | **Source**: Ghidra decompilation + Entity callback table analysis ... ✅ **InitGroundPatrolEnemy decompiled** | Analysis Status section claims 'InitGroundPatrolEnemy decompiled', but no such symbol exists. The address it cites (0x8002EA3C) is InitPhartHeadCollectible. The claime… |

### `docs/systems/entities.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | MarkEntityForDeferredRemoval @ 0x80020D74 | symbols.txt has no symbol named MarkEntityForDeferredRemoval, and 0x80020D74 is unassigned (nearest DeferredEntityRemoval = 0x80020C74). Both the name and the address … |
| stale-name | Event: EntityFollowPathWithWrapping @ 0x80055c70 | symbols.txt: EntityFollowPathWithWrapping = 0x8003CC60 (NOT 0x80055c70). 0x80055c70 is owned by InterpolateTimedPathPosition (symbol_addrs.txt: 'InterpolateTimedPathPo… |
| stale-name | \| `UpdateEntityPathWithWrapping` \| 0x80055c70 \| Path with wraparound at endpoints \| | 0x80055c70 is owned by InterpolateTimedPathPosition (symbol_addrs.txt). The name UpdateEntityPathWithWrapping does not exist in symbols.txt. Same address as the Entity… |
| stale-name | **Init Function**: `InitGroundPatrolEnemy` @ 0x8002ea3c | symbols.txt: 0x8002EA3C is owned by InitPhartHeadCollectible (symbol_addrs.txt: 'InitPhartHeadCollectible = 0x8002EA3C; // size:0x200'). The name InitGroundPatrolEnemy… |
| stale-name | \| `EntityType027_PathEnemy_Init` \| 0x8007f354 \| Type 27 - Path enemy \| | symbols.txt: 0x8007F354 is owned by EntityType027_FlyingEnemyVariant_Init (symbol_addrs.txt: 'EntityType027_FlyingEnemyVariant_Init = 0x8007F354; // size:0x88'). Doc n… |
| stale-name | \| `EntityType070_PathDecor_Init` \| 0x800807f8 \| Type 70 - Path decoration \| | symbols.txt: 0x800807F8 is owned by EntityType070_HamsterShield_Init (symbol_addrs.txt: 'EntityType070_HamsterShield_Init = 0x800807F8; // size:0x18'). Doc name Entity… |
| stale-address | \| `InitPathFollowEntityAlt` \| 0x800335d8 \| Alternate path init \| | symbols.txt: InitPathFollowEntityAlt = 0x800335A4 (symbol_addrs.txt size:0x13C). The cited 0x800335d8 is UNASSIGNED (no owner). Name owned by 0x800335A4, cited address… |
| stale-name | ### Entity Processing Loop @ 0x80020b34 | symbols.txt: 0x80020B34 is owned by EntityTickLoopWithCamera (NOT EntityTickLoop; EntityTickLoop = 0x80020E1C). src/blb.c defines both as distinct functions (line 130 … |

### `docs/systems/entity-init-architecture.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | \| InitSuperWillieCollectible \| 0x8003E0FC \| Super Willie heads \| | symbol_addrs.txt has 'InitSuperWillieCollectible = 0x8002F73C' while address 0x8003E0FC is owned by a DIFFERENT named function ('InitSnoBloEnemy = 0x8003E0FC' in /tmp/… |

### `docs/systems/game-loop.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| BOSS \| 0x2000 \| Boss fights \| `CreateBossPlayerEntity` @ 0x80078200 \| | symbols.txt: 0x80078200 is owned by CreateResultsScreenEntity (symbol_addrs.txt: 'CreateResultsScreenEntity = 0x80078200; // size:0x143C'). The name 'CreateBossPlayerE… |

### `docs/systems/gamestate-field-analysis.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Source:** Boss AI callbacks (e.g., `BossKloggCallback_BeamAttack` @ 0x80067F98) | No symbol named BossKloggCallback_BeamAttack exists anywhere (grep of symbol_addrs.txt, src, include returned only this doc). Address 0x80067F98 lies between PlayerSta… |

### `docs/systems/hud-system-complete.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-address | **Function**: InitTimerDisplayEntity @ 0x80026e3c (line 10351) | symbol_addrs.txt has InitTimerDisplayEntity = 0x80027CF4. Address 0x80026e3c is not a symbol start; it falls inside InitCountdownTimerEntity (0x80026DF8, size 0x13C, e… |

### `docs/systems/menu-system-complete.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x80080948 \| `EntityType079_RandomizedMenu_Init` \| Type 79 init \| \| 0x800809b8 \| `EntityType080_TimerMenu_Ini… | symbols.txt assigns these addresses to entirely different, non-menu names: 0x80080948 = EntityType081_PathDecor2_Init, 0x800809b8 = EntityType083_InteractiveDecor_Init… |

### `docs/systems/movie-cutscene-system.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| `GetMovieUnknown00` \| 0x8007ae14 \| Get unknown field at offset 0x00 \| | Address 0x8007AE14 is owned by `GetMovieFrameField00` (symbol_addrs.txt). The doc's name 'GetMovieUnknown00' does not exist as a symbol. Likely just a stale name for t… |

### `docs/systems/password-system.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| `InitPasswordEntry` (FUN_80075ff4) \| 0x80075ff4 \| Create 12 digit sprites + cursor \| | Address 0x80075FF4 is owned by 'InitPasswordDisplayEntity = 0x80075FF4; // size:0x228' in symbol_addrs.txt and src/passwd.c (INCLUDE_ASM 'asm/nonmatchings/passwd', Ini… |
| false-claim | From `g_pPlayerState` (0x8009DC20): | No symbol at 0x8009DC20 in symbol_addrs.txt (grep '= 0x8009DC20' returns nothing). The nearby tracked global is 'g_GameStateBase = 0x8009DC40; // type:GameState'. The … |

### `docs/systems/player/player-finn.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| false-claim | \| 0x110 \| 1 \| moveFlag \| Set to 1 when forward button pressed \| \| 0x111 \| 1 \| turnFlag \| Set to 1 when turni… | include/Game/entity.h FinnPlayerEntity (size 0x114) defines offset 0x110 as 's32 soundHandle_or_inputFlags' (the last field), with NO field at 0x111. trace-findings.md… |

### `docs/systems/player/player-normal.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Checkpoint Activation (Ma-Bird)** @ 0x8006A214 (`PlayerState_CheckpointActivated`) | Address 0x8006A214 is owned by 'PlayerState_LevelExitTeleporter = 0x8006A214' in symbol_addrs.txt, not 'PlayerState_CheckpointActivated'. The address matches; only the… |

### `docs/systems/player/player-runn.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | **Tick Callback**: LAB_80073a88 (line 36788) | Address 0x80073A88 is owned by 'EntityTick_ScaledSpriteWithSound = 0x80073A88' in symbol_addrs.txt (and it is decompiled - not in asm_stubs.txt). The doc invents the n… |

### `docs/systems/player/player-soar-glide.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | } else if (flags & 0x2000) {         // Boss mode         CreateBossPlayerEntity(...);     } else if (flags & 0x100) { | Describes flag 0x2000 as 'Boss mode' calling CreateBossPlayerEntity. No CreateBossPlayerEntity symbol exists; the only function for flag 0x2000 / address 0x80078200 is… |

### `docs/systems/projectiles.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x8004f2a4 \| `CalculateSineValue` \| Fixed-point sine from 256-entry table at 0x8009c09c \| | symbols.txt names 0x8004F2A4 as ScaleByCosine (stored 'ScaleByCoine' — char-stripping artifact of the distilled list, real symbol is ScaleByCosine). Doc calls it Calcu… |
| stale-name | \| 0x80052adc \| `ProjectileCollisionCallback` \| Collision detection \| | symbols.txt names 0x80052ADC as ProjectileKeyframeEventHandler. The doc (and combat-system.md line 607) call it ProjectileCollisionCallback. Same address, different in… |

### `docs/systems/sprites.md`

| Kind | Doc claim (quote) | Ground-truth / why flagged |
|------|-------------------|----------------------------|
| stale-name | \| 0x8009B508 \| 7 \| g_TempestPulsingMonkeySprites \| InitTempestPulsingMonkey @ 0x8003e0fc \| | symbol_addrs.txt maps 0x8003E0FC to InitSnoBloEnemy ('InitSnoBloEnemy = 0x8003E0FC'). The name 'InitTempestPulsingMonkey' does not appear anywhere in symbol_addrs.txt.… |
| false-claim | ### Sprite Header (12 bytes)  ``` Offset  Size  Type    Description ------  ----  ----    ----------- 0x00    2     u… | Ground truth include/Game/sprite.h SpriteHeader is 24 bytes (Size: 0x18), with fields type/header_size/frames_end/padding/rle_size/sprite_id/frame_count/padding/format… |

---

## Port-decomp harvest (2026-07-06) — 🟡 CORRECTNESS

The `port/decomp/` functional-C reconstructions (functions still `INCLUDE_ASM`
in `src/`) surfaced a large batch of struct maps, data tables, and name
corrections. The net-new material and the corroborated conflict fixes are
consolidated in [port-decomp-findings.md](port-decomp-findings.md). The items
below are the ones that remained **genuinely unsettled** after that pass and need
a runtime/disassembly check before any authoritative doc (especially
`docs/blb.hexpat`, the BLB source of truth) is changed.

### Asset 302 tile-flags bit 2 (0x04): "skip" vs "animated" — CONFLICT 🔴

Two port files disagree with each other about the meaning of bit `0x04` of the
Asset-302 tile-flags byte, and the disagreement straddles the source-of-truth
hexpat:

- `port/decomp/blb/anim_tile_entities.c:8` (`InitAnimatedTileEntities`) selects the
  level's **animated** tiles with `GetTileSizeFlags()[tile-1] & 4` — i.e. bit 2 set
  = "this tile is animated / samples the shared animated-tile sheet."
- `port/decomp/layer/tile_vram.c` (`LoadTileDataToVRAM`) treats bit 2 as **skip**
  for the static VRAM upload, agreeing with the current docs.

These are plausibly *reconcilable*: an animated tile would be **skipped from the
static upload** precisely because it renders via the animated path — so bit 2
could mean "animated (⇒ not statically uploaded)". But that is a hypothesis.
Current docs assert only "skip" in three places:
`docs/blb.hexpat:986` (TileFlags `skip:1`, source of truth),
`docs/blb/asset-types.md` §302, `docs/systems/tiles-and-tilemaps.md`.

**Action before editing the hexpat:** trace an animated-tile level in PCSX-Redux
to confirm whether a bit-2 tile is (a) rendered via the animated sheet and (b)
absent from the static upload. If both hold, relabel bit 2 "animated (implies not
statically uploaded)" across all three docs; otherwise keep "skip" and document
the animated-tile selector separately.

### Asset 302 bit 1 (0x02): tile size vs pixel bit-depth — 🟡

`port/decomp/layer/tile_vram.c` reads bit `0x02` as **CLUT pixel bit-depth**
(`is4bit = ((flag&2)==0)`, driving upload width 8-vs-4 and `GetTPage` mode), and
decides 16×16-vs-8×8 storage independently by tile *index* vs `tileHeader+0x10`.
`docs/systems/tiles-and-tilemaps.md` §302 currently labels bit 1 as "tile size
(0=16×16, 1=8×8)". Needs a runtime check on an 8×8-tile level. (upload height is
always 16 rows per the port.)

### Asset 501 byte +0x16 = runtime "spawned" flag, not padding — 🟢

`port/decomp/blb/SpawnOnScreenEntities.c:41` uses `def+0x16` bit 0 as an
"already spawned" latch (set at runtime). `docs/blb/asset-types.md` §501 labels
+0x16 "padding". The field may be zero on disk but is *used*, not inert — annotate
rather than assert.

### Other name-review items (see port-decomp-findings.md for sources)

- `FindOrderingTableSlotById` is a **sprite-sheet cache lookup**, not a GPU
  ordering-table op — likely misnomer.
- `AddPreInitEntitiesToList` @ 0x800250C8 actually builds the **animated-palette
  update list** — name misleading.
- `g_PlayerCallbackTable+0x40` is Finn-shared `{arg,fn}` pairs with a zero tail,
  not the "dense bounce-handler array" `fsm-callback-patterns.md` describes —
  verify whether a separate adjacent symbol holds the bounce array.
- Entity internal types 49/50/51 (ClayballOnPath vs Boss/BossPart), 79
  (EnemySpawner vs Skullmonkey), 42-60 (SnoBlo vs SuperWillie) — clean-room name
  divergences; port corroborates one internal doc against another but neither is
  proven.

