# Skullmonkeys (SLES-01090) Documentation

This documentation covers the reverse engineering of **Skullmonkeys** (PAL SLES-01090) for the PlayStation 1, with a focus on the BLB archive format and game data structures.

## Quick Start

- **[Decompilation Guide](decompilation-guide.md)** - How to add new functions to the decompilation
- **[BLB File Format Overview](blb/README.md)** - Start here for understanding the game's data format

## Documentation Structure

### BLB File Format (`blb/`)
The GAME.BLB archive format and its contents:

| Document | Description |
|----------|-------------|
| [README.md](blb/README.md) | BLB format overview and file hierarchy |
| [header.md](blb/header.md) | BLB header structure (0x1000 bytes) |
| [level-metadata.md](blb/level-metadata.md) | Level entry format (0x70 bytes each) |
| [asset-types.md](blb/asset-types.md) | Complete asset type reference (100-700) |
| [toc-format.md](blb/toc-format.md) | TOC and sub-TOC structures |

### Runtime Systems (`systems/`)
Game engine subsystems and runtime behavior:

| Document | Description |
|----------|-------------|
| [tiles-and-tilemaps.md](systems/tiles-and-tilemaps.md) | Tile graphics and tilemap rendering |
| [collision.md](systems/collision.md) | **Tile collision attributes and physics** |
| [sprites.md](systems/sprites.md) | RLE sprite format and lookup system |
| [entities.md](systems/entities.md) | Entity system and spawn data |
| [entity-identification.md](systems/entity-identification.md) | **Entity type identification guide** |
| [player-system.md](systems/player-system.md) | **Player mechanics, powerups, death** |
| [player-physics.md](systems/player-physics.md) | **Player physics constants and movement** |
| [audio.md](systems/audio.md) | SPU audio sample system |
| [rendering-order.md](systems/rendering-order.md) | Layer/entity z-ordering and priorities |
| [level-loading.md](systems/level-loading.md) | Stage loading state machine |
| [game-loop.md](systems/game-loop.md) | Main loop and mode callbacks |

### Reference (`reference/`)
Technical reference material:

| Document | Description |
|----------|-------------|
| [items.md](reference/items.md) | **Complete item/powerup reference with verified addresses** |
| [entity-types.md](reference/entity-types.md) | **Entity callback table (121 types) and type mappings** |
| [cheat-codes.md](reference/cheat-codes.md) | **All 22 cheat codes with button sequences** |
| [level-data-context.md](reference/level-data-context.md) | LevelDataContext structure (128 bytes) |
| [game-functions.md](reference/game-functions.md) | Key function addresses and purposes |
| [pal-jp-comparison.md](reference/pal-jp-comparison.md) | Regional version differences |

### Analysis (`analysis/`)
Ongoing research and unverified findings:

| Document | Description |
|----------|-------------|
| [gap-analysis.md](analysis/gap-analysis.md) | **Documentation gap analysis and priorities** |
| [unconfirmed-findings.md](analysis/unconfirmed-findings.md) | Observations awaiting verification |
| [password-screens.md](analysis/password-screens.md) | World completion password screens |

## Key Addresses (PAL SLES-01090)

| Address | Name | Description |
|---------|------|-------------|
| 0x800AE3E0 | blbHeaderBuffer | BLB header loaded at game boot |
| 0x8009DCC4 | LevelDataContext | Level loading state (GameState+0x84) |
| 0x8009B4B4 | g_GameBLBFile | CdlFILE structure for GAME.BLB |
| 0x800A59F0 | g_GameBLBSector | BLB starting sector (0x146) |

## Tools

- **`scripts/blb.hexpat`** - ImHex template (source of truth for format)
- **`tools/blb_viewer/`** - Web-based BLB viewer
- **`tools/extract_blb/`** - Python BLB extraction tool
- **`scripts/extract_sprites_600.py`** - Sprite extraction with palettes
- **`scripts/extract_all_graphics.py`** - Full tile/layer extraction

## Related Projects

- **[evil-engine](../../evil-engine/)** - Godot 4.5 viewer/editor using C99 core
- **PCSX-Redux** - Emulator with debugging support

## Verification Status

| Category | Status |
|----------|--------|
| BLB header format | ✅ Fully verified |
| Level metadata | ✅ Fully verified |
| Asset types 100-400 | ✅ Fully verified |
| Asset types 500-700 | ⚠️ Mostly verified |
| Sprite format | ✅ Fully verified |
| Entity system | ⚠️ Partially verified |
| Entity identification | ✅ Verified (2026-01-13) |
| Tile collision | ⚠️ Partially documented |
| Audio system | ✅ Verified |
| Rendering order | ✅ Verified |
