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
| **Primary** | Per-level (shared) | 600, 601, 602 | Level geometry, collision, palettes |
| **Secondary** | Per-level base + per-stage variants | 100-401 | Tiles, tile metadata, palettes |
| **Tertiary** | Per-stage | 500-700 | Sprites, entities, layers, audio |

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

## Related Documentation

- [Header Format](header.md) - Detailed header structure
- [Level Metadata](level-metadata.md) - Per-level entry format
- [Asset Types](asset-types.md) - Complete asset reference
- [TOC Format](toc-format.md) - TOC parsing details

## Tools

### Parse BLB header as JSON:
```bash
imhex --pl format --pattern scripts/blb.hexpat --input disks/blb/GAME.BLB > /tmp/blb.json
```

### Query level data:
```bash
jq '.levels.level_02' /tmp/blb.json  # SCIE level
```

### Extract raw assets:
```bash
python3 tools/extract_blb/extract.py disks/blb/GAME.BLB extracted/
```
