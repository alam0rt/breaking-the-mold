---
title: "BLB File Format"
category: blb
tags: [blb]
---

# BLB File Format

The GAME.BLB file is the main asset archive for Skullmonkeys, containing all level data, graphics, sprites, and audio.

## File Structure Overview

```
GAME.BLB (PAL: ~48 MB)
├── Header (0x1000 bytes = 2 sectors)
│   ├── Level Metadata Table (26 entries × 0x70 bytes)
│   ├── Movie Table (13 entries × 0x1C bytes)
│   ├── Sector Table (32 entries × 0x10 bytes)
│   ├── Mode 6 Sector Table (17 entries × 4 bytes)
│   └── Playback Sequence Data
│
└── Level Data (at sector offsets from header)
    └── Per-Level Segments
        ├── Primary (shared per-level geometry)
        ├── Secondary (tile graphics, palettes)
        └── Tertiary (per-stage sprites, entities, audio)
```

## Game Structure

Skullmonkeys contains **90 stages** across **26 level themes**:

| Category | Count | Level IDs |
|----------|-------|-----------|
| Menu | 1 | MENU |
| Regular Worlds | 17 | PHRO, SCIE, TMPL, BOIL, SNOW, FOOD, BRG1, GLID, CAVE, WEED, EGGS, CLOU, SOAR, CRYS, CSTL, MOSS, EVIL |
| Bosses | 5 | MEGA, HEAD, GLEN, WIZZ, KLOG |
| Special Modes | 2 | FINN (swimming), RUNN (runner) |
| Secret Bonus | 1 | SEVN (1970's) |

## Data Segments

Each level consists of three data segments:

| Segment | Scope | Asset Types | Contents |
|---------|-------|-------------|----------|
| **Primary** | Per-level (shared) | 600, 601, 602 | Level geometry, audio samples |
| **Secondary** | Per-stage (slots `secondary`, `secondary1..5`) | 100-401 | Tiles, tile metadata, palettes |
| **Tertiary** | Per-stage (slots `stage0..5`) | 100-700 | Stage data, entities, sprites, audio |

> **Note on naming:** "primary/secondary/tertiary" are *positional* labels (1st/2nd/3rd
> block in the level entry); the engine itself only distinguishes them by load path and a
> `containerType` byte (`0x01`=secondary, `0x00`=tertiary in `LoadAssetContainer @0x8007b074`).
> The level-metadata struct stores secondary **and** tertiary as parallel per-stage `u16[6]`
> arrays, so `secondary[N]` and `stage[N]` pair 1:1: secondary[N] is stage N's tile/palette
> **graphics bank** (the artwork), tertiary[N] is stage N's **map** (tilemap + collision +
> entity placement). `tools/extract_blb` reflects this in its output folders:
>
> | BLB segment | Extracted folder | Meaning |
> |-------------|------------------|---------|
> | `primary` | `<WORLD>/shared/` | world-shared geometry + audio |
> | `secondary`, `secondaryN` | `<WORLD>/stageN/tileset/` | per-stage tile/palette graphics |
> | `stageN` (tertiary) | `<WORLD>/stageN/map/` | per-stage tilemap, collision, entities |

### Segment Asset Patterns (Verified 2026-01-20)

**PRIMARY** - Always exactly `[600, 601, 602]`:
| Asset | Name | Size Range | Description |
|-------|------|------------|-------------|
| 600 | Sprites/Geometry | 500KB-1MB | Sprites shared across all stages |
| 601 | Audio Samples | 80KB-300KB | SPU ADPCM sound bank |
| 602 | Audio Settings | Variable | 4 bytes per sample (volume/pan) |

**SECONDARY** - Core `[100, 300, 400, 401, 301, 302]` + optional:
| Asset | Name | Description |
|-------|------|-------------|
| 100 | Tile Header | BG color, spawn position, tile counts (36 bytes) |
| 101 | VRAM Config | Optional: 8 levels only (bank_a_count, bank_b_count) |
| 300 | Tile Pixels | 8bpp indexed tile graphics |
| 301 | Palette Indices | Which palette each tile uses (1 byte/tile) |
| 302 | Tile Flags | Size/transparency flags (1 byte/tile) |
| 400 | Palettes | 256-color CLUTs (sub-TOC container) |
| 401 | Palette Anim | Color cycling config |
| 601/602 | Audio | Most levels; boss levels omit |

**TERTIARY** - Core `[100, 200, 201, 401, 500, 501, 302]` + extras:
| Asset | Name | Description |
|-------|------|-------------|
| 100 | Tile Header | Per-stage variant |
| 200 | Tilemaps | Layer tile indices (sub-TOC container) |
| 201 | Layer Entries | Layer dimensions, parallax (92 bytes each) |
| 302 | Tile Flags | Per-stage variant |
| 401 | Palette Anim | Per-stage variant |
| 500 | Tile Collision | Collision map (1 byte/tile) |
| 501 | Entities | Entity placements (24-byte structs) |
| 502 | VRAM Rects | Standard: Texture page definitions |
| 503 | Anim Offsets | Standard: Sprite animation timing |
| 504 | Vehicle Data | **FINN/RUNN only**: Path waypoints |
| 600 | Stage Sprites | Standard: Stage-specific sprites |
| 700 | Extra Audio | **9 levels**: Additional sound bank |

**Special Cases:**
- Boss/special levels (GLEN, HEAD, KLOG, WIZZ) lack 502/503
- MENU lacks 500 (no collision needed)
- Asset 101 in only 8 levels: BOIL, CLOU, FINN, FOOD, GLID, HEAD, MEGA, WEED
- Asset 700 in only 9 levels: BOIL, BRG1, CAVE, FOOD, GLID, MENU, SCIE, TMPL, WEED

### Asset Size Relationships (Derivable)

Several asset sizes are determined by other assets, so a BLB parser can use these
as sanity checks:

| Relationship | Meaning |
|--------------|---------|
| `Asset 501 size / 24` | Entity count — matches Asset 100 field_0x1E |
| `Asset 302 size` | = Asset 100 total_tiles (16×16 + 8×8 + extra), 1 byte/tile |
| `Asset 401 size` | = Asset 400 palette_count × 4 bytes/palette |
| `Asset 602 size` | = Asset 601 sample_count × 4 bytes/sample |
| `Asset 502 count` | = Asset 100 field_0x1C |

**Entity struct size: 24 bytes** — every entity record in Asset 501 is exactly 24
bytes, so `Asset 501 size` is always a multiple of 24. See
[asset-types.md § Asset 501](asset-types.md) for the field layout (including the
layer-dependent entity-type remapping warning).

### Co-occurrence Groups

Certain assets always appear together in a BLB segment, which reflects their
logical coupling. Use these as a parser invariant:

| Group | Assets | Context |
|-------|--------|---------|
| **Tile-graphics** | 100, 302, 401 | 208 segments (per-stage secondary) |
| **Audio** | 601, 602 | 117 segments (always paired; 602 = per-sample metadata for 601) |
| **Core-level** | 200, 201, 300, 301, 400, 501 | 104 segments (tiles + palettes + entities) |
| **VRAM/animation** | 502, 503 | 94 segments (texture rects + sprite animation offsets) |

If you see one of these assets without its group-mates, treat it as a parse error
unless the level is a known exception (boss/special levels omit 502/503; MENU
lacks 500).

### Sector Interleaving

Level data sectors are interleaved for streaming:
```
PRIMARY → SECONDARY_BASE → TERT[0] → SEC[0] → TERT[1] → SEC[1] → ...
```

### Secondary/Tertiary Pairing

**Important:** Each stage's tertiary block uses the secondary that *precedes* it:

| Tertiary Block | Uses Secondary Block |
|----------------|---------------------|
| Stage 0 | Base secondary |
| Stage 1 | Stage 0 secondary |
| Stage N | Stage (N-1) secondary |

## TOC Format

All segments use the same Table of Contents format:

```
Offset   Size   Description
------   ----   -----------
0x00     u32    Entry count
0x04+    12×N   TOC entries

Each TOC Entry (12 bytes):
  0x00   u32    Asset type ID (e.g., 0x258=600)
  0x04   u32    Asset size in bytes
  0x08   u32    Offset from segment start
```

## Container vs Raw Assets

| Type | Format | Asset IDs |
|------|--------|-----------|
| **Container** | Has sub-TOC | 0x258, 0x259, 0x190 |
| **Raw** | Data starts immediately | 0x25A, 0x064, 0x12C, etc. |

Container sub-TOC format:
```
0x00    u32     Sub-entry count
0x04+   12×N    Sub-entries: {flags/id, size, offset}
```

## File Locations

- **Disc path**: `\GAME.BLB;1`
- **RAM location**: Header loaded at 0x800AE3E0
- **Starting sector**: 0x146 (326) stored at 0x800A59F0

## Format Completeness

**Status**: ✅ **100% UNDERSTOOD**

All fields documented and categorized:
- ✅ Functional fields: Complete with runtime usage
- ✅ Vestigial fields: Confirmed unused (safe to ignore)
- ✅ Rarely-used fields: Documented with usage patterns
- ✅ Asset 700: Demo replay data (previously misidentified as unused SPU audio)

**See**: [vestigial-fields-complete.md](vestigial-fields-complete.md) for analysis of unused fields

## Related Documentation

- [Header Format](header.md) - Detailed header structure
- [Level Metadata](level-metadata.md) - Per-level entry format
- [Asset Types](asset-types.md) - Complete asset reference (100%)
- [TOC Format](toc-format.md) - TOC parsing details
- [Vestigial Fields](vestigial-fields-complete.md) - Analysis of unused fields

## Tools

### Parse BLB header as JSON:
```bash
imhex --pl format --pattern docs/blb.hexpat --input disks/blb/GAME.BLB > /tmp/blb.json
```

### Query level data:
```bash
jq '.levels.level_02' /tmp/blb.json  # SCIE level
```

### Extract raw assets:
```bash
python3 tools/extract_blb/extract.py disks/blb/GAME.BLB extracted/
```
