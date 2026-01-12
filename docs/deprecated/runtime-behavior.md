# Skullmonkeys Runtime Behavior

> ⚠️ **DEPRECATED**: This document has been reorganized.
> See the new documentation:
> - [Level Loading](../systems/level-loading.md)
> - [Sprites](../systems/sprites.md)
> - [Game Functions](../reference/game-functions.md)
>
> This file is kept for reference but will not be updated.

---

This document describes runtime behavior observed via PCSX-Redux MCP debugging.

**NOTE: All addresses are for PAL version (SLES-01090).**

## Level Loading State Machine

The game uses a sliding window state machine to track concurrent operations.
Each "slot" uses paired bytes in the BLB header:
- **Mode byte**: header[0xF36 + arrayPosition]  
- **Index byte**: header[0xF92 + arrayPosition]

### Sliding Window Formula (VERIFIED)

```
arrayPosition = headerOffset - 0x0A
levelIndex = header[0xF92 + arrayPosition]
levelEntry = header + (levelIndex * 0x70)
```

Where:
- `headerOffset` is read from LevelDataContext at ctx+0x60
- `0x0A` appears to be the base offset (MENU uses 0x0A)
- The resulting `levelIndex` is the index into the level metadata table

### Verified Examples

| headerOffset | arrayPosition | levelIndex | Level |
|--------------|---------------|------------|-------|
| 0x0A (10) | 0 | 0 | MENU |
| 0x20 (32) | 22 | 9 | FOOD (Skullmonkey Brand Hot Dogs) |

### Header Offset Progression

The `headerOffset` field in LevelDataContext (at ctx+0x60) increments as the game
performs different load operations:

| headerOffset | Operation | Notes |
|--------------|-----------|-------|
| 0x0A (10) | MENU loaded | After intro movies complete |
| 0x12 (18) | Demo level loaded | TMPL (Monkey Shrines) |
| 0x0E (14) | Gameplay level loaded | When player selects a level |

## Menu Idle Demo Behavior

When the player remains idle at the main menu, the game automatically loads
a demo level after a timeout period.

**Verified via PCSX-Redux MCP (PAL / SLES-01090):**

### Demo Level: TMPL (Monkey Shrines) - Level Index 3

| Field | Value | Description |
|-------|-------|-------------|
| Level ID | "TMPL" | Monkey Shrines |
| Level Index | 3 | In BLB header level table |
| Header Offset | 0x12 (18) | State machine position |
| Geometry (0x258) | 510,108 bytes | Level graphics/world data |
| Collision (0x259) | 188,288 bytes | Physics/collision data |
| Palette (0x25A) | 196 bytes | Color palette |
| **Total Primary** | ~699 KB | Combined asset size |

### State Transition Sequence

```
MENU (headerOffset=0x0A)
    ↓ (idle timeout)
Demo: TMPL (headerOffset=0x12)
    ↓ (demo ends or button press)
MENU (headerOffset=0x0A)
```

The demo plays a pre-recorded input sequence through the Monkey Shrines level,
showcasing gameplay to idle players (attract mode).

## Level Size Comparison

Captured via runtime memory inspection:

| Level | ID | Geometry | Collision | Palette | Total |
|-------|-----|----------|-----------|---------|-------|
| Menu | MENU | 53 KB | 11 KB | 24 B | ~65 KB |
| Demo (Monkey Shrines) | TMPL | 510 KB | 188 KB | 196 B | ~699 KB |
| Science Centre | SCIE | 524 KB | 126 KB | 148 B | ~650 KB |

The menu is intentionally lightweight (~10x smaller than gameplay levels).

## LevelDataContext Address

The active level's data is tracked in the LevelDataContext structure:

- **Base address**: `0x8009DCC4` (GameState + 0x84)
- **BLB header pointer**: ctx+0x5C → `0x800AE3E0`
- **Sliding window index**: ctx+0x60 (current state machine position, u8)
- **Loader callback**: ctx+0x64 → `0x80020848`
- **Primary data buffer**: ctx+0x68 (points to loaded TOC)
- **Secondary data buffer**: ctx+0x6C
- **Asset 0x258 pointer**: ctx+0x70 (level geometry)
- **Asset 0x259 pointer**: ctx+0x74 (collision data)
- **Asset 0x259 size**: ctx+0x78 
- **Asset 0x25A pointer**: ctx+0x7C (palette data)

See [blb-data-format.md](blb-data-format.md#leveldatacontext-structure-verified-via-ghidra--pcsx-redux-mcp) 
for the complete 128-byte structure with all asset pointer mappings.

## Debugging Tips

When reading memory during gameplay:

1. **Always pause first** - Use `pause()` before reads to get consistent snapshots
2. **Check headerOffset** - This tells you what operation is active
3. **Check asset259Size** - Quick way to identify which level is loaded:
   - MENU: 11,420 bytes (0x2C9C)
   - SCIE: 126,256 bytes (0x1ED30)
   - TMPL: 188,288 bytes (0x2DF80)

4. **Resume after** - Don't forget to `resume()` when done

## Sprite System Architecture

The game's sprite system uses a multi-level lookup chain to connect entities
to their visual representations. This is entirely code-driven, not data-driven.

### Sprite Lookup Chain (VERIFIED via Ghidra)

When an entity needs a sprite, the game calls:

```
InitSpriteContext(context+0x78, sprite_id) @ 0x8007bc3c
    └─► LookupSpriteById(sprite_id) @ 0x8007bb10
          └─► FindSpriteInTOC(DAT_800a6064, sprite_id) @ 0x8007b968
```

**Key insight**: Entity type → Sprite ID mapping is **hardcoded in game code**,
not stored in BLB data. Each entity initialization function contains the sprite
ID as a literal constant.

### Sprite Container Structure (Asset 600)

The sprite container has a Table of Contents (TOC) at the start:

| Offset | Size | Description |
|--------|------|-------------|
| 0x00 | u32 | Sprite count |
| 0x04 | 12×N | TOC entries |

Each TOC entry (12 bytes):

| Offset | Size | Description |
|--------|------|-------------|
| 0x00 | u32 | **Sprite ID** (32-bit hash/identifier) |
| 0x04 | u32 | Sprite data size |
| 0x08 | u32 | Offset to sprite data (from container start) |

### Known Sprite IDs (Comprehensive - from Ghidra Analysis)

These sprite IDs are hardcoded constants extracted from game initialization functions.
The 32-bit ID is used to look up sprite data in the TOC at runtime.

#### Core Game Sprites

| Sprite ID | Source Function | Purpose |
|-----------|-----------------|---------|
| `0x21842018` | FUN_8001fcf0 | **Player character** |
| `0xe4ac9451` | FUN_80078200 (×18) | HUD digit/counter display |
| `0xec95689b` | FUN_80078200 | HUD status element, also used in score display |
| `0xaa0da270` | FUN_80078200 | HUD secondary element (conditional) |
| `0x121941c4` | FUN_80078200 | HUD audio/sound indicator |

#### UI/Menu Sprites

| Sprite ID | Source Function | Purpose |
|-----------|-----------------|---------|
| `0xb8700ca1` | FUN_80076928 | Common UI element, menu system |
| `0x8c510186` | FUN_80027a00 | Title screen element |
| `0x3099991b` | FUN_80075ff4 | Gem/collectible indicator |
| `0x10094096` | FUN_80075ff4, FUN_80077068 | Switch/button element |

#### Entity/Object Sprites

| Sprite ID | Source Function | Purpose |
|-----------|-----------------|---------|
| `0x168254b5` | FUN_80034bb8 | Particle effect (z-order 0x3bf) |
| `0x6a351094` | FUN_80037ae0 | Sparkle/shine effect |
| `0x1e1000b3` | FUN_8006dd98 | Enemy sprite type |
| `0x182d840c` | FUN_8006dd98 | Enemy variant sprite |
| `0x1b301085` | FUN_80073338 | Projectile sprite |
| `0xc34aa22` | FUN_800549f0 | Flying collectible |

#### Boss/NPC Sprites

| Sprite ID | Source Function | Purpose |
|-----------|-----------------|---------|
| `0x181c3854` | FUN_80047fb8 | Boss element 1 |
| `0x8818a018` | FUN_80058310 | Boss element 2 (spawned 6× in loop) |
| `0x244655d` | FUN_80047fb8 | Boss/NPC element |
| `0xca1b20cb` | FUN_80070d68 | Boss glow effect |
| `0x4835000` | FUN_80070d68 | Boss particle effect |

#### Interactive Object Sprites

| Sprite ID | Source Function | Purpose |
|-----------|-----------------|---------|
| `0xa89d0ad0` | FUN_80052678 | Save/checkpoint indicator |
| `0xb01c25f0` | FUN_80053268 | Portal/warp point |
| `0x3da80d13` | FUN_80074100 | Switch overlay |
| `0xcc6c8070` | FUN_80074100 | Trigger effect |

**Note**: FUN_800281a4 contains 24 sprite init calls and appears to be the HUD initialization.
FUN_80078200 contains 19 sprite init calls for score/status display.

### Entity Initialization Flow

Entities are loaded from Asset 501 (24-byte structures) but their sprites
are determined by code, not data:

1. Game reads entity from Asset 501 (position, type, variant, layer)
2. Entity type triggers a switch/dispatch in the game engine
3. Dispatch function calls entity-specific init code
4. Init code calls `InitSpriteContext()` with hardcoded sprite ID

### Implications for BLB Viewer

Since sprite IDs are in code, the viewer uses fallback strategies:

1. **If entity type has known sprite ID** → Look up by ID in TOC
2. **For collectibles (type 2, 8)** → Use variant field as sprite index
3. **Fallback** → Use first sprite in container

To fully map all entity types would require tracing every entity dispatch
function in Ghidra to extract the literal sprite ID constants.

### Related Functions

| Address | Ghidra Name | Description |
|---------|------|-------------|
| `0x8007bc3c` | InitSpriteContext | Sets up sprite for entity context |
| `0x8007bb10` | LookupSpriteById | Finds sprite by 32-bit ID in TOC |
| `0x8007b968` | FindSpriteInTOC | Searches sprite container TOC |
| `0x8001c720` | InitEntitySprite | Core entity sprite init (91 callers) |
| `0x8001cdac` | SetupSpriteWithId | Links sprite to context |
| `0x80078200` | InitHUDSprites | Initializes 19 HUD/score sprites |
| `0x800281a4` | InitMenuSprites | Initializes 24 menu/UI sprites |
| `0x8001fcf0` | InitPlayerEntity | Player entity setup (sprite 0x21842018) |
| `0x8006dd98` | InitEnemySprite | Enemy sprite initialization |
| `0x80070d68` | InitBossSprite | Boss element initialization |
| `0x80047fb8` | InitBossEntity | Boss NPC initialization |
| `0x80020e1c` | EntityTickLoop | Main entity update loop |
| `0x80076928` | InitMenuEntity | Menu/UI entity (sprite 0xb8700ca1) |

### Key Global Variables

| Address | Ghidra Name | Description |
|---------|------|-------------|
| `0x800a6060` | g_pSecondarySpriteBank | Secondary sprite source (global/shared) - NULL when unused |
| `0x800a6064` | g_pLevelDataContext | Pointer to LevelDataContext (level-specific sprites) |
| `0x8009DCC4` | LevelDataContext | Level loading state structure |
| `0x800AE3E0` | blbHeaderBufferBase | BLB header base in RAM |

### Runtime Verification (PCSX-Redux MCP)

The following was verified with SCIE stage 0 loaded in PCSX-Redux:

**Memory Reads (PAL / SLES-01090):**
```
0x800a6060 = 0x00000000    (g_pSecondarySpriteBank - NULL, not used)
0x800a6064 = 0x8009dcc4    (g_pLevelDataContext - points to LevelDataContext)

LevelDataContext @ 0x8009dcc4:
  +0x40 = 0x8014e4c4       (Secondary sprite TOC pointer)
  +0x70 = 0x800af408       (Tertiary sprite TOC pointer)

Tertiary TOC @ 0x800af408:
  [0x00] = 0x4f (79)       (Sprite count)
  [0x04] = 0x000ac607      (Entry 0: unused/metadata)
  [0x10] = 0x524ec094      (Entry 1: Sprite ID)
  ...

Secondary TOC @ 0x8014e4c4:
  [0x00] = 0x14 (20)       (Sprite count)
  [0x04] = 0x09406d8a      (Entry 0: Sprite ID - common sprite, confirmed in blb-data-format.md)
  [0x10] = 0x400c989d      (Entry 1: Sprite ID)
  ...

Sprite Data Verification:
  0x8014e4c4 + 0xf4 → 0x8014e5b8 (Sprite 0x09406d8a data)
  Contains sprite ID at offset 0x0c: "8a 6d 40 09" = 0x09406d8a ✓
```

**TOC Entry Structure Confirmed:**
```c
struct SpriteTOCEntry {
    uint32_t sprite_id;     // 32-bit sprite identifier
    uint32_t data_size;     // Size of sprite data
    uint32_t data_offset;   // Offset from container start
};
```

### Future Work

The sprite ID mapping is now substantially complete. Remaining tasks:

1. ~~Identify the main entity dispatch/switch function~~ (Done - uses function pointers at offset 0x14)
2. ~~Trace each case to find the init function~~ (Done - 91 callers of FUN_8001c720 traced)
3. ~~Extract the sprite ID constant from each init function~~ (Done - 27+ unique IDs found)
4. ~~Build a complete lookup table for the viewer~~ (Done - engine.js updated)

---

*Last updated: 2026-01-07 (Runtime verification in PCSX-Redux, Ghidra function names updated)*
