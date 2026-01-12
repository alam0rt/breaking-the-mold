# Stage Loading Analysis

> ⚠️ **DEPRECATED**: This document has been reorganized.
> See the new documentation:
> - [Level Loading](../systems/level-loading.md)
> - [LevelDataContext](../reference/level-data-context.md)
> - [Asset Types](../blb/asset-types.md)
>
> This file is kept for reference but will not be updated.

---

**Status**: Working Document  
**Last Updated**: 2026-01-06  
**Sources**: Ghidra decompilation, ImHex template (`blb.hexpat`), existing documentation, runtime verification via PCSX-Redux MCP

---

## Executive Summary

Stage loading in Skullmonkeys follows a three-phase process:
1. **Header Loading** - BLB header (first 0x1000 bytes) loaded once at game boot
2. **Level Loading** - Primary/secondary segments loaded when level selected
3. **Stage Loading** - Tertiary segment loaded when entering specific stage

Entities are **not** stored with explicit sprite references. Instead, entity types trigger hardcoded init functions with sprite IDs compiled into the game code.

---

## Phase 1: Header Loading (One-Time at Boot)

### KNOWN (Verified via Ghidra)

**Function**: `LoadBLBHeader` @ 0x800208b0

```c
void LoadBLBHeader(int state) {
    // Read first 2 sectors (0x1000 bytes) into RAM
    CdBLB_ReadSectors(0, 2, *(void **)(state + 0x40));
    
    // Initialize LevelDataContext at GameState+0x84
    InitLevelDataContext((int *)(state + 0x84), headerBuffer, &loaderCallback);
    
    // Set sprite table globals (NULL for secondary bank initially)
    SetSpriteTables(0, (int *)(state + 0x84));
}
```

**Header Layout** (0x1000 bytes):
| Offset | Size | Content |
|--------|------|---------|
| 0x000-0xB5F | 26×0x70 | Level metadata table |
| 0xB60-0xCCB | 13×0x1C | Movie table |
| 0xCD0-0xECF | 32×0x10 | Sector table |
| 0xECC-0xF0F | 17×0x04 | Mode 6 sector table |
| 0xF10-0xF27 | 2×0x0C | Credits entries |
| 0xF31 | u8 | Level count (26) |
| 0xF32 | u8 | Movie count (13) |
| 0xF34-0xFFF | var | Playback sequence data |

**RAM Locations**:
- BLB Header: 0x800AE3E0 (PAL)
- LevelDataContext: 0x8009DCC4 (GameState + 0x84)

### THINK (Inferred but not 100% verified)

- The header is read ONCE and stays resident for the entire game session
- The playback sequence data (0xF34+) controls demo sequences and level transitions

### UNKNOWN

- Exact structure of playback sequence data at 0xF34-0xFFF
- Full meaning of mode values (we know 3=level, 6=special, but modes 0-2, 4-5?)

---

## Phase 2: Level Loading (On Level Selection)

### KNOWN (Verified via Ghidra)

**Function**: `InitializeAndLoadLevel` @ 0x8007d1d0

**Parameters**:
- `param_1`: GameState* pointer
- `param_2`: Load flags/mode (1=normal, 5=demo mode 1, 6=demo mode 2, 99=special)

**Loading Sequence**:
1. Reset various state buffers
2. Advance playback sequence based on mode
3. Display loading screen (`DisplayLoadingScreen`)
4. Parse level data (`LevelDataParser`)
5. Upload audio to SPU (`UploadAudioToSPU`)
6. Load asset container with secondary data (`LoadAssetContainer`)
7. Calculate buffer sizes from `GetPrimaryBufferSize()` and `GetCurrentTertiaryDataSize()`
8. Load tile data to VRAM (`LoadTileDataToVRAM`)
9. Load tertiary asset container
10. Initialize player spawn position (`InitPlayerSpawnPosition`)
11. Initialize layers and tile state (`InitLayersAndTileState`)

**Key Call**:
```c
// Load secondary container (param_2 = stage index, '\x01' = secondary type)
LoadAssetContainer(pLevelDataCtx, (uint)param_2, '\x01');

// Later, load tertiary container (param_2 = stage index, '\x00' = tertiary type)
LoadAssetContainer(param_1 + 0x21, (uint)param_2, '\x00');
```

**Level Entry Structure** (0x70 bytes per level):
| Offset | Field | Verified |
|--------|-------|----------|
| 0x00 | Primary sector offset (u16) | ✓ |
| 0x02 | Primary sector count (u16) | ✓ |
| 0x04 | Primary buffer size (u32) | ✓ |
| 0x08 | Entry[1] offset (u32) | ✓ |
| 0x0C | Level asset index (u8) | ✓ |
| 0x0D | Password-selectable flag (u8) | ✓ |
| 0x0E | Stage count (u16) | ✓ |
| 0x10 | Tertiary data offsets[6] (u16×6) | ✓ |
| 0x1E | Secondary sector offsets[6] (u16×6) | ✓ |
| 0x2C | Secondary sector counts[6] (u16×6) | ✓ |
| 0x3A | Tertiary sector offsets[6] (u16×6) | ✓ |
| 0x48 | Tertiary sector counts[6] (u16×6) | ✓ |
| 0x56 | Level ID (5 chars) | ✓ |
| 0x5B | Level name (21 chars) | ✓ |

### THINK (Inferred)

- The demo mode rotation counter cycles through attract modes (menu demos)
- Mode 99 appears to be a "full playback sequence" mode used for cutscenes/transitions

### UNKNOWN

- What determines if secondary bank (`g_pSecondarySpriteBank`) is used vs NULL
- Exact differences between modes 1, 5, and 6 for level loading
- How inter-level transition animations work with mode 6 sector table

---

## Phase 3: Asset Container Loading

### KNOWN (Verified via Ghidra)

**Function**: `LoadAssetContainer` @ 0x8007b074

**Parameters**:
- `pLevelDataCtx`: LevelDataContext* pointer
- `subBlockIndex`: Stage index (1-6 for secondary/tertiary)
- `containerType`: '\x01' = secondary, '\x00' = tertiary

**Asset Type Mapping** (from TOC entries):
| Type ID | Decimal | Context Offset | Asset Name |
|---------|---------|----------------|------------|
| 0x64 | 100 | ctx[1] (+0x04) | TileHeader |
| 0x65 | 101 | ctx[2] (+0x08) | Unknown |
| 0xC8 | 200 | ctx[3] (+0x0C) | TilemapContainer |
| 0xC9 | 201 | ctx[4] (+0x10) | LayerEntries |
| 0x12C | 300 | ctx[5] (+0x14) | TilePixels |
| 0x12D | 301 | ctx[6] (+0x18) | PaletteIndices |
| 0x12E | 302 | ctx[7] (+0x1C) | TileFlags |
| 0x12F | 303 | ctx[10] (+0x28) | Unknown |
| 0x190 | 400 | ctx[8] (+0x20) | PaletteContainer |
| 0x191 | 401 | ctx[9] (+0x24) | Unknown |
| 0x1F4 | 500 | ctx[11] (+0x2C) | EntityList (Asset 501) |
| 0x1F5 | 501 | ctx[14] (+0x38) | Unknown |
| 0x1F6 | 502 | ctx[15] (+0x3C) | Unknown |
| 0x1F7 | 503 | ctx[12] (+0x30) | Unknown |
| 0x1F8 | 504 | ctx[13] (+0x34) | Unknown |
| 0x258 | 600 | ctx[16-17] (+0x40/0x44) | SpriteContainer + size |
| 0x259 | 601 | ctx[18-19] (+0x48/0x4C) | Collision data + size |
| 0x25A | 602 | ctx[20] (+0x50) | Unknown |
| 0x2BC | 700 | ctx[21-22] (+0x54/0x58) | Unknown + size |

**Sector Offset Calculation** (from LevelDataParser):
```c
// For mode 3 (normal level), get sector info from level entry
if (mode == 3) {
    levelEntry = headerBase + (levelIndex * 0x70);
    sectorOffset = *(u16*)(levelEntry + 0x00);
    sectorCount = *(u16*)(levelEntry + 0x02);
}

// For mode 6 (special), use mode 6 sector table
if (mode == 6) {
    addr = headerBase + (levelIndex * 4) + 0xECC;
    sectorOffset = *(u16*)(addr);
    sectorCount = *(u16*)(addr + 2);
}
```

**Secondary vs Tertiary Selection** (containerType parameter):
```c
if (containerType == '\x00') {  // Tertiary
    levelEntry = headerBase + (levelIndex * 0x70) + (stageIndex * 2);
    sectorOffset = *(u16*)(levelEntry + 0x38);  // tert_sector_off
    sectorCount = *(u16*)(levelEntry + 0x46);   // tert_sector_cnt
} else {  // Secondary
    levelEntry = headerBase + (levelIndex * 0x70) + (stageIndex * 2);
    sectorOffset = *(u16*)(levelEntry + 0x1C);  // sec_sector_off
    sectorCount = *(u16*)(levelEntry + 0x2A);   // sec_sector_cnt
}
```

### THINK (Inferred)

- Asset types 500-504 contain entity-related data (501 confirmed as entities)
- Asset 700 might be audio-related given the `UploadAudioToSPU` call after loading

### UNKNOWN

- Purpose of asset types: 101, 303, 401, 501-504, 602, 700
- Whether all TOC entries are always present or level-dependent

---

## Phase 4: Sprite Lookup at Runtime

### KNOWN (Verified via Ghidra + MCP)

**Function Chain**:
1. `InitEntitySprite` @ 0x8001c720 - Core entity setup
2. `FUN_8001d080` - Sets sprite for entity
3. `LookupSpriteById` @ 0x8007bb10 - Finds sprite data by 32-bit ID
4. `FindSpriteInTOC` @ 0x8007b968 - Searches TOC tables

**Lookup Order** (from `LookupSpriteById`):
```c
int LookupSpriteById(int spriteId) {
    // 1. First try tertiary container (level-specific sprites)
    if (g_pLevelDataContext != NULL) {
        result = FindSpriteInTOC(g_pLevelDataContext, spriteId);
        if (result != 0) return result;
    }
    
    // 2. Fall back to secondary container (shared/global sprites)
    if (g_pSecondarySpriteBank != NULL) {
        // Linear search through secondary bank TOC
        for (i = 0; i < secondaryBank->count; i++) {
            if (secondaryBank->entries[i].spriteId == spriteId) {
                return computeSpriteDataPointer();
            }
        }
    }
    
    return 0;  // Sprite not found
}
```

**TOC Search** (from `FindSpriteInTOC`):
```c
int FindSpriteInTOC(int ctx, uint spriteId) {
    // Search tertiary TOC at ctx+0x70
    spriteContainer = *(uint**)(ctx + 0x70);
    if (spriteContainer != NULL) {
        for (i = 0; i < spriteContainer[0]; i++) {
            if (spriteContainer[i*3 + 1] == spriteId) {
                return (int)spriteContainer + spriteContainer[i*3 + 3];
            }
        }
    }
    
    // Search secondary TOC at ctx+0x40
    spriteContainer = *(uint**)(ctx + 0x40);
    if (spriteContainer != NULL) {
        for (i = 0; i < spriteContainer[0]; i++) {
            if (spriteContainer[i*3 + 1] == spriteId) {
                return (int)spriteContainer + spriteContainer[i*3 + 3];
            }
        }
    }
    
    return 0;
}
```

**Sprite TOC Entry Structure** (12 bytes):
```c
struct SpriteTOCEntry {
    uint32_t sprite_id;     // +0x00: 32-bit hash identifier
    uint32_t data_size;     // +0x04: Size in bytes
    uint32_t data_offset;   // +0x08: Offset from container start
};
```

**Global Variables**:
| Address | Name | Description |
|---------|------|-------------|
| 0x800a6060 | g_pSecondarySpriteBank | Secondary sprite bank (often NULL) |
| 0x800a6064 | g_pLevelDataContext | Points to LevelDataContext |

### THINK (Inferred)

- Secondary sprite bank is used for "shared" sprites that appear across multiple levels
- The 32-bit sprite IDs are likely CRC32 or similar hash of sprite asset names
- Each level's tertiary container has a subset of sprites needed for that level

### UNKNOWN

- **CRITICAL**: What determines which sprites go in secondary vs tertiary containers at build time
- How the 32-bit sprite IDs are generated (hash algorithm)
- Whether sprites can be "inherited" from previous level loads

---

## Phase 5: Entity Spawning and Sprite Association

### KNOWN (Verified via Ghidra)

**Entity Init Functions** (91 callers of `InitEntitySprite`):
- Each entity type has a dedicated init function
- Init functions call `InitEntitySprite(ctx, SPRITE_ID, z_order, x, y, flags)`
- **Sprite IDs are HARDCODED in the game code**, not stored in BLB data

**Example** (from FUN_800281a4 - Menu entities):
```c
InitEntitySprite(entity, 0xb8700ca1, 10000, 0x18, 0xffffffe0, 1);  // UI element
InitEntitySprite(entity, 0xe2f188,   10000, 0x25, 0xffffffe0, 1);  // Menu item
InitEntitySprite(entity, 0xa9240484, 10000, 0x118, 0xffffffe0, 1); // Button
InitEntitySprite(entity, 0x88a28194, 10000, 0x60, 0xffffffe0, 1);  // Icon
```

**Entity Data Structure** (Asset 501, 24 bytes):
| Offset | Size | Field |
|--------|------|-------|
| 0x00 | u16 | X position |
| 0x02 | u16 | Y position |
| 0x04 | u32 | Entity type (hash/ID) |
| 0x08 | u8 | Variant |
| 0x09 | u8 | Layer |
| ... | ... | Additional fields |

**Entity Type → Sprite Mapping**:
The game has a large dispatch table (likely switch statement or function pointer array) that maps entity type → init function. The init function knows the sprite ID.

### THINK (Inferred)

- Entity types are probably 32-bit hashes like sprite IDs
- There's a registration system that links entity type hashes to init function pointers
- This explains why sprites are level-specific: each level only includes sprites needed by its entities

### UNKNOWN

- **CRITICAL**: Where is the entity type → init function dispatch table?
- How entities with the same type but different sprites are handled (variant field?)
- Whether entity types can override sprite selection dynamically

---

## Key Insight: Sprite Availability is Per-Level

### KNOWN (Verified via Python script)

Each level's tertiary container has a **different set of sprite IDs**:

| Level | Index | Sprite Count | First Sprite ID | Has 0x09406d8a? |
|-------|-------|--------------|-----------------|-----------------|
| MENU | 0 | 9 | 0x0005c699 | ❌ No |
| SCIE | 1 | 22 | 0x09406d8a | ✓ Yes (index 0) |
| TMPL | 2 | 20 | 0x09406d8a | ✓ Yes (index 0) |
| BOIL | 3 | 34 | 0x002c4800 | ✓ Yes (index 2) |

**Implication**: Sprite ID `0x09406d8a` (clayball?) only exists in levels with clayballs as enemies.

### THINK (Inferred)

- The BLB build pipeline only includes sprites needed by each level
- If an entity type is spawned in a level without its sprite, `LookupSpriteById` returns 0
- When sprite lookup fails, the game likely uses a fallback (first sprite or crash?)

### UNKNOWN

- What happens when `LookupSpriteById` returns 0 (fallback behavior)
- Whether there's a "common" sprite bank loaded for all levels
- How the BLB was built (what tool packaged assets per-level)

---

## LevelDataContext Structure (Verified)

**Base Address**: 0x8009DCC4 (GameState + 0x84)

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x00 | u32 | current_stage | Currently loaded stage index |
| 0x04 | ptr | tile_header | Asset 100 pointer |
| 0x08 | ptr | unknown_101 | Asset 101 pointer |
| 0x0C | ptr | tilemap_container | Asset 200 pointer |
| 0x10 | ptr | layer_entries | Asset 201 pointer |
| 0x14 | ptr | tile_pixels | Asset 300 pointer |
| 0x18 | ptr | palette_indices | Asset 301 pointer |
| 0x1C | ptr | tile_flags | Asset 302 pointer |
| 0x20 | ptr | palette_container | Asset 400 pointer |
| 0x24 | ptr | unknown_401 | Asset 401 pointer |
| 0x28 | ptr | unknown_303 | Asset 303 pointer |
| 0x2C | ptr | entity_list | Asset 500 pointer |
| 0x30 | ptr | unknown_503 | Asset 503 pointer |
| 0x34 | ptr | unknown_504 | Asset 504 pointer |
| 0x38 | ptr | unknown_501 | Asset 501 pointer |
| 0x3C | ptr | unknown_502 | Asset 502 pointer |
| 0x40 | ptr | **secondary_sprite_toc** | Asset 600 secondary pointer |
| 0x44 | u32 | secondary_sprite_size | Asset 600 secondary size |
| 0x48 | ptr | audio_sample_bank | Asset 601 pointer (SPU ADPCM samples) |
| 0x4C | u32 | audio_sample_size | Asset 601 size |
| 0x50 | ptr | audio_volume_pan | Asset 602 pointer (volume/pan table) |
| 0x54 | ptr | unknown_700 | Asset 700 pointer |
| 0x58 | u32 | unknown_700_size | Asset 700 size |
| 0x5C | ptr | **blb_header** | BLB header pointer (0x800AE3E0) |
| 0x60 | u8 | playback_index | Current playback sequence index |
| 0x64 | ptr | loader_callback | CD loading callback function |
| 0x68 | ptr | primary_buffer | Primary segment buffer |
| 0x6C | ptr | tertiary_buffer | Tertiary segment buffer |
| 0x70 | ptr | **tertiary_sprite_toc** | Asset 600 tertiary pointer |
| 0x74 | u32 | tertiary_sprite_size | Asset 600 tertiary size |
| ... | ... | ... | ... |

---

## Summary: What We Know vs Don't Know

### ✓ CONFIRMED

1. Header loads once at boot (2 sectors → 0x1000 bytes)
2. Level entry is 0x70 bytes with sector offsets for primary/secondary/tertiary
3. Each stage has its own tertiary container loaded on-demand
4. Asset containers have TOC at start with type IDs (100-700)
5. Sprite lookup: tertiary → secondary → fail
6. Sprite IDs are 32-bit constants hardcoded in init functions
7. Entity types trigger dispatch to init functions (91 init functions found)
8. Each level has different sprite IDs in tertiary container

### ⚠️ INFERRED (High Confidence)

1. Mode values: 3=level, 6=special/transition, others unknown
2. Secondary sprite bank used for shared cross-level sprites
3. Sprite IDs are CRC32 or similar hash of asset names
4. Entity type → init function mapping via dispatch table

### ❓ UNKNOWN (Needs Investigation)

1. Entity type dispatch table location and structure
2. What determines secondary vs tertiary sprite placement
3. Fallback behavior when sprite not found
4. Mode 6 sector table exact purpose
5. Asset types 101, 303, 401, 501-504, 602, 700 purposes
6. Playback sequence data structure (0xF34+)
7. How 32-bit IDs are generated at build time

---

## Verification Methods

### Runtime Verification (PCSX-Redux MCP)
```lua
-- Read LevelDataContext pointers
ctx = 0x8009DCC4
secondary_toc = read_u32(ctx + 0x40)
tertiary_toc = read_u32(ctx + 0x70)
blb_header = read_u32(ctx + 0x5C)

-- Dump sprite TOC
if tertiary_toc ~= 0 then
    count = read_u32(tertiary_toc)
    for i = 0, count-1 do
        id = read_u32(tertiary_toc + 4 + i*12)
        size = read_u32(tertiary_toc + 8 + i*12)
        offset = read_u32(tertiary_toc + 12 + i*12)
        print(string.format("Sprite %d: ID=0x%08x Size=%d Offset=0x%x", i, id, size, offset))
    end
end
```

### Static Analysis (Python/ImHex)
```bash
# Parse BLB with template
imhex --pl format --pattern scripts/blb.hexpat --input disks/blb/GAME.BLB > /tmp/blb.json

# Query specific level
jq '.levels.level_01' /tmp/blb.json
```
