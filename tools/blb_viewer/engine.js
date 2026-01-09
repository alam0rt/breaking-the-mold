/**
 * Skullmonkeys Game Engine
 * 
 * JavaScript implementation of the PSX game engine's data interpretation.
 * This module understands how the game uses assets from the BLB file:
 * - Tile system (16x16 and 8x8 tiles with palettes)
 * - Layer system (parallax scrolling backgrounds)
 * - Entity markers in tilemaps
 * 
 * Depends on: BLB module for raw asset data
 */

// ============================================================================
// Asset Type IDs (as used by the game engine)
// ============================================================================

const ASSET_TYPE = {
  TILE_HEADER: 100,       // Stage header with tile counts, BG color, spawn point
  TILEMAP: 200,           // Container of tilemap data per layer
  LAYER_ENTRIES: 201,     // Layer metadata (scroll, size, position)
  TILE_PIXELS: 300,       // Raw 8-bit indexed pixel data for all tiles
  PALETTE_IDX: 301,       // Per-tile palette index
  TILE_FLAGS: 302,        // Per-tile flags (size, transparency, skip)
  PALETTES: 400,          // Container of 256-color palettes
  TILE_ATTRS: 500,        // Tile attribute map (collision/triggers)
  ENTITIES: 501,          // Entity placement data (24-byte structures)
  SPRITES: 600,           // Sprite/entity graphics
  COLLISION: 601,         // Collision data
};

// Tile attribute values (Asset 500)
const TILE_ATTR = {
  PASSABLE: 0,      // Empty/air
  SOLID: 2,         // Collision
  TRIGGER_18: 18,   // Unknown trigger
  CHECKPOINT: 83,   // Checkpoint/save point (0x53)
  ENTITY_ZONE: 101, // Entity spawn zone (0x65)
};

// Tilemap entry bitmask constants (verified via docs/blb-data-format.md)
// Tilemap entries are 16-bit values: bits 0-11 = tile index, bits 12-15 = color tint
const TILE_INDEX_MASK = 0xFFF;  // 12 bits for tile index (0 = transparent, 1+ = tile)
const COLOR_TINT_SHIFT = 12;   // Upper 4 bits select color tint from layer's palette

// ============================================================================
// PSX Color Conversion
// ============================================================================

/**
 * Convert PSX 15-bit BGR5551 color to RGBA array
 * Color 0x0000 is treated as fully transparent
 */
function psxColorToRGBA(color16) {
  if (color16 === 0) return [0, 0, 0, 0]; // Transparent
  const r = (color16 & 0x1F) * 8;
  const g = ((color16 >> 5) & 0x1F) * 8;
  const b = ((color16 >> 10) & 0x1F) * 8;
  return [r, g, b, 255];
}

// ============================================================================
// Tile Header Parsing
// ============================================================================

/**
 * Parse the stage tile header (Asset 100)
 * 
 * Structure:
 *   0x00: u8 bgR, bgG, bgB    - Background color
 *   0x04: u8 altR, altG, altB - Alternate color
 *   0x08: u16 levelWidth      - Level width in tiles
 *   0x0A: u16 levelHeight     - Level height in tiles
 *   0x0C: u16 spawnX          - Player spawn X
 *   0x0E: u16 spawnY          - Player spawn Y
 *   0x10: u16 count16x16      - Number of 16x16 tiles
 *   0x12: u16 count8x8        - Number of 8x8 tiles
 *   0x14: u16 countExtra      - Extra tile count
 */
function parseTileHeader(assetData) {
  if (!assetData || assetData.length < 0x16) return null;
  
  const view = new DataView(assetData.buffer, assetData.byteOffset);
  
  return {
    bgColor: [assetData[0], assetData[1], assetData[2]],
    altColor: [assetData[4], assetData[5], assetData[6]],
    levelWidth: view.getUint16(0x08, true),
    levelHeight: view.getUint16(0x0A, true),
    spawnX: view.getUint16(0x0C, true),
    spawnY: view.getUint16(0x0E, true),
    count16x16: view.getUint16(0x10, true),
    count8x8: view.getUint16(0x12, true),
    countExtra: view.getUint16(0x14, true),
  };
}

/**
 * Get total tile count from header
 */
function getTotalTileCount(tileHeader) {
  return tileHeader.count16x16 + tileHeader.count8x8 + tileHeader.countExtra;
}

// ============================================================================
// Entity System (Asset 501) - VERIFIED via Ghidra 2026-01-07
// ============================================================================

/**
 * Entity type → sprite ID mapping is HARDCODED in game code.
 * 
 * The game's sprite lookup chain:
 *   InitSpriteContext(context+0x78, sprite_id) @ 0x8007bc3c
 *     └─► LookupSpriteById(sprite_id) @ 0x8007bb10
 *           └─► FindSpriteInTOC(DAT_800a6064, sprite_id) @ 0x8007b968
 * 
 * Known hardcoded sprite IDs from game code:
 *   0x21842018 - Player sprite (FUN_8001fcf0)
 *   0xb8700ca1 - Common UI element
 *   0xe2f188   - Menu/UI (heavily reused)
 *   0xa9240484, 0xe8628689, 0x88a28194, 0x80e85ea0, 0x9158a0f6, 0x902c0002
 * 
 * To properly render entities, we'd need a static lookup table built by
 * tracing each entity type's spawn dispatcher in the game code.
 */

/**
 * Known entity types and their sprite IDs (from Ghidra code tracing)
 * 
 * Sprite IDs are 32-bit values used to look up sprite data in the TOC.
 * Many entity types share the same base sprite with different animations/variants.
 * 
 * Note: Most entity types don't have fixed sprite IDs because the game uses
 * the variant field and runtime state to select sprites dynamically.
 * The sprite IDs here are the DEFAULT/initial sprites used.
 * 
 * VERIFIED via PCSX-Redux MCP (2026-01-07):
 * - TOC entry format: {sprite_id: u32, data_size: u32, data_offset: u32}
 * - LevelDataContext+0x40 = secondary TOC, +0x70 = tertiary TOC
 * 
 * Entity type → sprite ID mappings are now loaded from config.js (ENTITY_SPRITE_MAP)
 * which is auto-generated by scripts/extract_sprite_ids.py from the game binary.
 */

/**
 * Get sprite ID for an entity type from config
 * Falls back to hardcoded values if not in config
 */
function getEntitySpriteId(entityType) {
  // First check the auto-extracted mapping from config.js
  if (window.SpriteConfig && window.SpriteConfig.ENTITY_SPRITE_MAP[entityType]) {
    const spriteId = window.SpriteConfig.ENTITY_SPRITE_MAP[entityType];
    console.log(`[CONFIG] Entity type ${entityType} → sprite ID 0x${spriteId.toString(16)} from config`);
    return spriteId;
  }
  // Fall back to hardcoded values in ENTITY_TYPES
  const typeInfo = ENTITY_TYPES[entityType];
  return typeInfo ? typeInfo.spriteId : null;
}

const ENTITY_TYPES = {
  // Collectibles (use variant to select animation frame)
  // Sprite ID loaded from config.js ENTITY_SPRITE_MAP
  2: { name: 'Clayball', shortName: 'Clay', desc: 'Collectible coin', useVariant: true },
  8: { name: 'Item', shortName: 'Item', desc: 'Collectible item', spriteId: 0xc34aa22, useVariant: true },
  
  // Ammo pickups (consumable bullets for player weapon)
  3: { name: 'Ammo', shortName: 'Ammo', desc: 'Bullet pickup (standard)', color: '#ff0' },
  24: { name: 'AmmoSpecial', shortName: 'Ammo+', desc: 'Bullet pickup (special)', color: '#fa0' },
  
  // Enemies (from FUN_8006dd98, FUN_80073338)
  25: { name: 'Enemy1', shortName: 'Enm1', desc: 'Enemy type 1', spriteId: 0x1e1000b3, color: '#f00' },
  27: { name: 'Enemy2', shortName: 'Enm2', desc: 'Enemy type 2', spriteId: 0x182d840c, color: '#f00' },
  
  // Objects
  10: { name: 'Object', shortName: 'Obj', desc: 'Large object' },
  28: { name: 'Platform', shortName: 'Plat', desc: 'Moving platform', color: '#88f' },
  48: { name: 'Platform2', shortName: 'Plat2', desc: 'Moving platform 2', color: '#88f' },
  
  // Interactive
  42: { name: 'Portal', shortName: 'Port', desc: 'Portal/warp point', spriteId: 0xb01c25f0, color: '#f0f' },
  45: { name: 'Message', shortName: 'Msg', desc: 'Message/save box', spriteId: 0xa89d0ad0, color: '#0ff' },
  
  // Boss/NPC entities (from FUN_80047fb8, FUN_80058310)
  50: { name: 'Boss', shortName: 'Boss', desc: 'Boss entity', spriteId: 0x181c3854, color: '#f80' },
  51: { name: 'BossPart', shortName: 'BPrt', desc: 'Boss sub-entity', spriteId: 0x8818a018, color: '#f80' },
  
  // Effects (from FUN_80034bb8, FUN_80037ae0)
  60: { name: 'Particle', shortName: 'Prtc', desc: 'Particle effect', spriteId: 0x168254b5 },
  61: { name: 'Sparkle', shortName: 'Sprk', desc: 'Sparkle effect', spriteId: 0x6a351094 },
};

// Player sprite ID (hardcoded in FUN_8001fcf0 / InitPlayerEntity)
const PLAYER_SPRITE_ID = 0x21842018;

/**
 * Known sprite IDs from game code (extracted via Ghidra analysis 2025-01-07)
 * These are hardcoded in various init functions throughout the game code.
 * 
 * NOTE: This lookup table is exported for DOCUMENTATION/REFERENCE only.
 * For actual entity→sprite mapping, use config.js ENTITY_SPRITE_MAP.
 * For detailed sprite info with call sites, see docs/sprite_ids_detailed.json.
 * 
 * Sprite lookup chain:
 *   InitSpriteContext (0x8007bc3c) → LookupSpriteById (0x8007bb10) → FindSpriteInTOC (0x8007b968)
 * 
 * Two sprite sources:
 *   - DAT_800a6064 (tertiary): Level-specific sprites
 *   - DAT_800a6060 (secondary): Global/shared sprites
 * 
 * TOC entry format: 12 bytes [u32 count, u32 sprite_id, u32 offset]
 */
const KNOWN_SPRITE_IDS = {
  // Core Game Sprites
  PLAYER: 0x21842018,             // Player character (FUN_8001fcf0)
  
  // UI/HUD Sprites (from FUN_800281a4, FUN_80078200)
  UI_COMMON: 0xb8700ca1,          // Common UI element, menu system (FUN_80076928)
  UI_DIGIT: 0xe4ac9451,           // HUD digit/counter display (used 18+ times)
  HUD_MAIN: 0xec95689b,           // HUD status element (FUN_80078200, also in FUN_80075ff4)
  HUD_SECONDARY: 0xaa0da270,      // HUD secondary element
  HUD_AUDIO: 0x121941c4,          // HUD audio/sound sprite
  
  // Entity/Object Sprites
  TITLE_ELEMENT: 0x8c510186,      // Title screen element (FUN_80027a00)
  PARTICLE: 0x168254b5,           // Particle effect (FUN_80034bb8)
  SPARKLE: 0x6a351094,            // Sparkle/shine effect (FUN_80037ae0)
  ENEMY_SPRITE: 0x1e1000b3,       // Enemy sprite type (FUN_8006dd98)
  ENEMY_VARIANT: 0x182d840c,      // Enemy variant sprite
  
  // Boss/NPC Sprites
  BOSS_ELEMENT_1: 0x181c3854,     // Boss element (FUN_80047fb8, FUN_80049828)
  BOSS_ELEMENT_2: 0x8818a018,     // Boss element 2 (FUN_80058310)
  BOSS_ELEMENT_3: 0x244655d,      // Boss/NPC element (FUN_80047fb8)
  BOSS_GLOW: 0xca1b20cb,          // Boss glow effect (FUN_80070d68)
  BOSS_PARTICLE: 0x4835000,       // Boss particle (FUN_80070d68)
  
  // Collectible/Item Sprites
  GEM_INDICATOR: 0x3099991b,      // Gem indicator (FUN_80075ff4)
  SAVE_INDICATOR: 0xa89d0ad0,     // Save/checkpoint indicator (FUN_80052678)
  PORTAL: 0xb01c25f0,             // Portal/warp point (FUN_80053268)
  FLYING_OBJECT: 0xc34aa22,       // Flying collectible (FUN_800549f0)
  PROJECTILE: 0x1b301085,         // Projectile sprite (FUN_80073338)
  
  // Interactive Object Sprites
  SWITCH: 0x10094096,             // Switch/button (FUN_80075ff4, FUN_80077068)
  SWITCH_OVERLAY: 0x3da80d13,     // Switch overlay (FUN_80074100)
  TRIGGER_EFFECT: 0xcc6c8070,     // Trigger effect (FUN_80074100)
};

/**
 * Find a sprite entry by its 32-bit ID in the sprite container
 * @param {Object} spriteContainer - Parsed sprite container { count, sprites: [...] }
 * @param {number} spriteId - 32-bit sprite ID to find
 * @returns {Object|null} Sprite entry { index, id, size, offset } or null
 */
function findSpriteById(spriteContainer, spriteId) {
  if (!spriteContainer || !spriteContainer.sprites) return null;
  return spriteContainer.sprites.find(s => s.id === spriteId) || null;
}

/**
 * Get a sprite for an entity, with fallback strategies
 * 
 * Strategy (in order):
 * 1. Check config.js ENTITY_SPRITE_MAP for this entity type
 * 2. If entity type has a known spriteId in ENTITY_TYPES, look it up in TOC
 * 3. Use entity type to select a consistent sprite from available ones
 * 
 * Sprite IDs are extracted from the game binary by scripts/extract_sprite_ids.py
 * and stored in config.js. As more entity types are identified, add them to
 * the ENTITY_SPRITE_MAP in config.js.
 * 
 * @param {Object} spriteContainer - Parsed sprite container { count, sprites: [...] }
 * @param {Object} entity - Entity with type, variant fields
 * @returns {Object|null} Sprite entry { index, id, size, offset, data } or null
 */
function getSpriteForEntity(spriteContainer, entity) {
  if (!spriteContainer || !spriteContainer.sprites || spriteContainer.sprites.length === 0) {
    return null;
  }
  
  const sprites = spriteContainer.sprites;
  const typeInfo = ENTITY_TYPES[entity.type];
  
  // Strategy 1: Check config.js ENTITY_SPRITE_MAP (auto-extracted from binary)
  const configSpriteId = getEntitySpriteId(entity.type);
  if (configSpriteId) {
    const sprite = findSpriteById(spriteContainer, configSpriteId);
    if (sprite) {
      console.log(`[LOOKUP] Entity type ${entity.type} → found sprite ID 0x${configSpriteId.toString(16)} at index ${sprite.index}`);
      return sprite;
    } else {
      // Debug: show what IDs are available
      console.log(`[LOOKUP] Entity type ${entity.type} → sprite ID 0x${configSpriteId.toString(16)} NOT FOUND in container`);
      console.log(`[LOOKUP] Available IDs (first 5):`, sprites.slice(0, 5).map(s => `0x${s.id.toString(16)}`));
    }
  }
  
  // Strategy 2: If we have a known sprite ID in ENTITY_TYPES fallback
  if (typeInfo && typeInfo.spriteId) {
    const sprite = findSpriteById(spriteContainer, typeInfo.spriteId);
    if (sprite) return sprite;
  }
  
  // Strategy 3: Items (type 8) - fallback to index 1
  if (entity.type === 8) {
    return sprites.length > 1 ? sprites[1] : sprites[0];
  }
  
  // Strategy 4: Use entity type as a seed to pick a consistent sprite
  // This ensures all entities of the same type use the same sprite
  // but different types get different sprites (variety)
  const typeHash = entity.type * 7; // Simple hash
  const idx = typeHash % sprites.length;
  return sprites[idx];
}

/**
 * Parse entity placement data (Asset 501)
 * 
 * Structure (24 bytes per entity) - VERIFIED via Ghidra:
 *   0x00: u16 x1          - Bounding box min X (pixels)
 *   0x02: u16 y1          - Bounding box min Y (pixels)
 *   0x04: u16 x2          - Bounding box max X (pixels)
 *   0x06: u16 y2          - Bounding box max Y (pixels)
 *   0x08: u16 x_center    - Entity center X (pixels)
 *   0x0A: u16 y_center    - Entity center Y (pixels)
 *   0x0C: u16 variant     - Animation frame / subtype selector
 *   0x0E: u16 padding1    - Always 0
 *   0x10: u16 padding2    - Always 0
 *   0x12: u16 entity_type - Type ID (2=clayball, 45=message, etc)
 *   0x14: u16 layer       - Render layer (1, 2, or 3)
 *   0x16: u16 padding3    - Always 0
 * 
 * @param {Uint8Array} entityData - Raw entity data from Asset 501
 * @returns {Array} Array of entity objects
 */
function parseEntities(entityData) {
  if (!entityData || entityData.length < 24) {
    return [];
  }
  
  const view = new DataView(entityData.buffer, entityData.byteOffset);
  const entitySize = 24;
  const count = Math.floor(entityData.length / entitySize);
  const entities = [];
  
  for (let i = 0; i < count; i++) {
    const offset = i * entitySize;
    
    const x1 = view.getUint16(offset + 0x00, true);
    const y1 = view.getUint16(offset + 0x02, true);
    const x2 = view.getUint16(offset + 0x04, true);
    const y2 = view.getUint16(offset + 0x06, true);
    const xCenter = view.getUint16(offset + 0x08, true);
    const yCenter = view.getUint16(offset + 0x0A, true);
    const variant = view.getUint16(offset + 0x0C, true);  // Animation frame / subtype
    const entityType = view.getUint16(offset + 0x12, true);
    const layer = view.getUint16(offset + 0x14, true);  // Render layer (1-3)
    
    // Skip entries that look like padding/invalid
    if (x1 === 0 && y1 === 0 && x2 === 0 && y2 === 0) {
      continue;
    }
    
    // Get entity type info from ENTITY_TYPES lookup
    const typeInfo = ENTITY_TYPES[entityType] || { name: `Type${entityType}`, shortName: `T${entityType}`, desc: 'Unknown' };
    
    entities.push({
      index: i,
      x1, y1, x2, y2,
      xCenter, yCenter,
      width: x2 - x1,
      height: y2 - y1,
      type: entityType,
      typeName: typeInfo.name,
      typeShortName: typeInfo.shortName || typeInfo.name,
      typeDesc: typeInfo.desc,
      variant,
      layer,
    });
  }
  
  return entities;
}

// ============================================================================
// Palette System
// ============================================================================

/**
 * Parse palette container (Asset 400) into array of RGBA palettes
 * Each palette has 256 colors, stored as PSX 15-bit BGR5551
 */
function parsePalettes(paletteContainerData) {
  const entries = BLB.parseSubTOC(paletteContainerData);
  const palettes = [];
  
  for (const entry of entries) {
    const palette = [];
    const view = new DataView(entry.data.buffer, entry.data.byteOffset);
    
    // Read up to 256 colors (512 bytes)
    for (let i = 0; i < 256 && i * 2 + 2 <= entry.data.length; i++) {
      const color16 = view.getUint16(i * 2, true);
      palette.push(psxColorToRGBA(color16));
    }
    
    palettes.push(palette);
  }
  
  return palettes;
}

// ============================================================================
// Tile System
// ============================================================================

/**
 * Tile flags interpretation:
 *   Bit 0: Semi-transparent
 *   Bit 1: 8x8 tile (0 = 16x16, 1 = 8x8)
 *   Bit 2: Skip/hidden tile
 */
const TILE_FLAG = {
  SEMI_TRANSPARENT: 0x01,
  SIZE_8X8: 0x02,
  SKIP: 0x04,
};

/**
 * Get tile information and pixel data
 * 
 * @param {number} tileIndex - 1-based tile index from tilemap
 * @param {object} tileHeader - Parsed tile header
 * @param {Uint8Array} pixelData - Raw pixel data (Asset 300)
 * @param {Uint8Array} flagsData - Tile flags (Asset 302)
 * @param {Uint8Array} paletteIndices - Per-tile palette index (Asset 301)
 * @param {Array} palettes - Parsed palette array
 * @returns {object|null} Tile data or null if transparent/skipped
 */
function getTile(tileIndex, tileHeader, pixelData, flagsData, paletteIndices, palettes) {
  // Index 0 = transparent/empty
  if (tileIndex === 0) return null;
  
  const idx0 = tileIndex - 1; // Convert to 0-based
  const totalTiles = getTotalTileCount(tileHeader);
  
  // Indices beyond tile count are entity markers
  if (idx0 >= totalTiles) {
    return {
      type: 'entity',
      entityId: tileIndex,
    };
  }
  
  // Read tile flags
  const flags = idx0 < flagsData.length ? flagsData[idx0] : 0;
  
  // Skip hidden tiles
  if (flags & TILE_FLAG.SKIP) return null;
  
  const is8x8 = (flags & TILE_FLAG.SIZE_8X8) !== 0;
  const isSemiTransparent = (flags & TILE_FLAG.SEMI_TRANSPARENT) !== 0;
  const tileSize = is8x8 ? 8 : 16;
  
  // Calculate pixel data offset
  // 16x16 tiles: 256 bytes each (16 rows × 16 bytes)
  // 8x8 tiles: 128 bytes each (8 rows × 16 bytes stride)
  const stride = 16; // Always 16-byte row stride
  const bytesPerTile = is8x8 ? 128 : 256;
  
  let pixelOffset;
  if (is8x8 && idx0 >= tileHeader.count16x16) {
    // 8x8 tiles come after all 16x16 tiles
    pixelOffset = tileHeader.count16x16 * 256 + (idx0 - tileHeader.count16x16) * 128;
  } else {
    pixelOffset = idx0 * 256;
  }
  
  if (pixelOffset + bytesPerTile > pixelData.length) return null;
  
  // Get palette for this tile
  const paletteIdx = idx0 < paletteIndices.length ? paletteIndices[idx0] : 0;
  const palette = palettes[paletteIdx] || palettes[0] || [];
  
  // Extract pixel indices
  const pixels = [];
  for (let y = 0; y < tileSize; y++) {
    const row = [];
    for (let x = 0; x < tileSize; x++) {
      const byteOffset = pixelOffset + y * stride + x;
      row.push(byteOffset < pixelData.length ? pixelData[byteOffset] : 0);
    }
    pixels.push(row);
  }
  
  return {
    type: 'tile',
    size: tileSize,
    pixels,
    palette,
    isSemiTransparent,
  };
}

// ============================================================================
// Layer System
// ============================================================================

/**
 * Layer entry size in bytes
 */
const LAYER_ENTRY_SIZE = 92;

/**
 * Parse a single layer entry
 * 
 * Structure (92 bytes):
 *   0x00: u16 xOffset       - Layer X offset in tiles
 *   0x02: u16 yOffset       - Layer Y offset in tiles
 *   0x04: u16 width         - Layer width in tiles
 *   0x06: u16 height        - Layer height in tiles
 *   0x08: u16 levelWidth    - Full level width
 *   0x0A: u16 levelHeight   - Full level height
 *   0x0C: u32 renderParam   - Render parameters
 *   0x10: u32 scrollX       - X scroll factor (16.16 fixed point)
 *   0x14: u32 scrollY       - Y scroll factor (16.16 fixed point)
 *   0x26: u8  layerType     - Layer type (3 = special)
 *   0x28: u16 skipRender    - Skip rendering flag
 *   0x2C: u8  bgR, bgG, bgB - Layer background color
 */
function parseLayerEntry(data, index) {
  const offset = index * LAYER_ENTRY_SIZE;
  if (offset + LAYER_ENTRY_SIZE > data.length) return null;
  
  const view = new DataView(data.buffer, data.byteOffset + offset, LAYER_ENTRY_SIZE);
  
  // Convert 16.16 fixed point to float
  const scrollX = view.getUint32(0x10, true) / 65536.0;
  const scrollY = view.getUint32(0x14, true) / 65536.0;
  
  return {
    index,
    xOffset: view.getUint16(0x00, true),
    yOffset: view.getUint16(0x02, true),
    width: view.getUint16(0x04, true),
    height: view.getUint16(0x06, true),
    levelWidth: view.getUint16(0x08, true),
    levelHeight: view.getUint16(0x0A, true),
    renderParam: view.getUint32(0x0C, true),
    scrollX,  // 0.0 = static background, 1.0 = moves with camera
    scrollY,
    layerType: view.getUint8(0x26),
    skipRender: view.getUint16(0x28, true) !== 0,
    bgColor: [view.getUint8(0x2C), view.getUint8(0x2D), view.getUint8(0x2E)],
  };
}

/**
 * Parse all layer entries from Asset 201
 */
function parseLayers(layerEntriesData) {
  const count = Math.floor(layerEntriesData.length / LAYER_ENTRY_SIZE);
  const layers = [];
  
  for (let i = 0; i < count; i++) {
    const layer = parseLayerEntry(layerEntriesData, i);
    if (layer && layer.width > 0 && layer.height > 0) {
      layers.push(layer);
    }
  }
  
  return layers;
}

/**
 * Check if a layer should be rendered
 */
function isLayerVisible(layer) {
  // Layer type 3 is special (collision?), skip it
  if (layer.layerType === 3) return false;
  // Explicitly marked as skip
  if (layer.skipRender) return false;
  return true;
}

// ============================================================================
// Tile Attribute Map (Asset 500) - Collision/Triggers
// ============================================================================

/**
 * Parse tile attribute map (Asset 500)
 * 
 * Structure:
 *   0x00: u32 flags      - Unknown (often 0)
 *   0x04: u16 width      - Map width in tiles
 *   0x06: u16 height     - Map height in tiles
 *   0x08: u8[] data      - One byte per tile (row-major)
 * 
 * @param {Uint8Array} assetData - Raw Asset 500 data
 * @returns {object|null} Parsed tile attribute map
 */
function parseTileAttributes(assetData) {
  if (!assetData || assetData.length < 8) return null;
  
  const view = new DataView(assetData.buffer, assetData.byteOffset);
  const flags = view.getUint32(0, true);
  const width = view.getUint16(4, true);
  const height = view.getUint16(6, true);
  
  // Validate size: header (8) + width * height should match
  const expectedSize = 8 + (width * height);
  if (assetData.length < expectedSize) {
    console.warn(`[ENGINE] Tile attrs size mismatch: expected ${expectedSize}, got ${assetData.length}`);
  }
  
  // Extract tile data
  const data = new Uint8Array(assetData.buffer, assetData.byteOffset + 8, width * height);
  
  return {
    flags,
    width,
    height,
    data,
  };
}

/**
 * Get tile attribute at (x, y) position
 */
function getTileAttribute(tileAttrs, x, y) {
  if (!tileAttrs || x < 0 || y < 0 || x >= tileAttrs.width || y >= tileAttrs.height) {
    return TILE_ATTR.PASSABLE;
  }
  return tileAttrs.data[y * tileAttrs.width + x];
}

// ============================================================================
// Tilemap Access
// ============================================================================

/**
 * Parse tilemaps from container (Asset 200)
 * Returns array of tilemap data, one per layer
 */
function parseTilemaps(tilemapContainerData) {
  return BLB.parseSubTOC(tilemapContainerData);
}

/**
 * Read a tile index from a tilemap at given tile coordinates
 * Returns the 12-bit tile index (bits 0-11 of u16 tilemap entry)
 * See TILE_INDEX_MASK constant for bit layout documentation.
 */
function getTilemapEntry(tilemapData, x, y, layerWidth) {
  const idx = (y * layerWidth + x) * 2;
  if (idx + 2 > tilemapData.length) return 0;
  
  const view = new DataView(tilemapData.buffer, tilemapData.byteOffset);
  const raw = view.getUint16(idx, true);
  
  // Lower 12 bits are tile index, upper 4 bits are color tint selector
  return raw & TILE_INDEX_MASK;
}

// ============================================================================
// Stage Loading (High-level)
// ============================================================================

/**
 * Load and parse all stage data needed for rendering
 * 
 * @param {ArrayBuffer} fileData - Raw BLB file data
 * @param {object} level - Parsed level entry from BLB.parseHeader()
 * @param {number} stageIndex - Stage index within level
 * @returns {object} Complete stage data ready for rendering
 */
function loadStage(fileData, level, stageIndex) {
  // Load raw assets from BLB
  const assets = BLB.loadStageAssets(fileData, level, stageIndex);
  if (!assets) return null;
  
  const { primary, secondary, tertiary } = assets;
  
  // Parse tile header from secondary segment
  const tileHeader = secondary[ASSET_TYPE.TILE_HEADER]
    ? parseTileHeader(secondary[ASSET_TYPE.TILE_HEADER].data)
    : null;
  
  if (!tileHeader) return null;
  
  // Get raw tile data
  const pixelData = secondary[ASSET_TYPE.TILE_PIXELS]?.data || new Uint8Array(0);
  const paletteIndices = secondary[ASSET_TYPE.PALETTE_IDX]?.data || new Uint8Array(0);
  const flagsData = secondary[ASSET_TYPE.TILE_FLAGS]?.data || new Uint8Array(0);
  
  // Parse palettes
  const palettes = secondary[ASSET_TYPE.PALETTES]
    ? parsePalettes(secondary[ASSET_TYPE.PALETTES].data)
    : [];
  
  // Parse layers from tertiary segment
  const layers = tertiary[ASSET_TYPE.LAYER_ENTRIES]
    ? parseLayers(tertiary[ASSET_TYPE.LAYER_ENTRIES].data)
    : [];
  
  // Parse tilemaps
  const tilemaps = tertiary[ASSET_TYPE.TILEMAP]
    ? parseTilemaps(tertiary[ASSET_TYPE.TILEMAP].data)
    : [];
  
  // Parse sprites from tertiary Asset 600 (stage-specific entity sprites)
  const stageSprites = tertiary[ASSET_TYPE.SPRITES]
    ? parseSpriteContainer(tertiary[ASSET_TYPE.SPRITES].data, false)
    : { count: 0, sprites: [] };
  
  // Parse sprites from primary Asset 600 (level geometry/background graphics)
  // NOTE: Primary Asset 600 contains "level geometry/RLE sprites" per blb.hexpat.
  // These are background art elements, NOT entity sprites, but use the same TOC format.
  // We include them for completeness and mark them as "shared" (level-wide).
  const sharedSprites = primary && primary[ASSET_TYPE.SPRITES]
    ? parseSpriteContainer(primary[ASSET_TYPE.SPRITES].data, true)
    : { count: 0, sprites: [] };
  
  // Merge sprites: stage sprites first, then shared sprites
  // Shared sprites are marked with isShared=true for UI display
  const sprites = {
    count: stageSprites.count + sharedSprites.count,
    sprites: [
      ...stageSprites.sprites,
      ...sharedSprites.sprites,
    ],
    stageCount: stageSprites.count,
    sharedCount: sharedSprites.count,
  };
  
  // Parse entities from tertiary Asset 501
  const entities = tertiary[ASSET_TYPE.ENTITIES]
    ? parseEntities(tertiary[ASSET_TYPE.ENTITIES].data)
    : [];
  
  // Parse tile attributes from tertiary Asset 500 (collision/triggers)
  const tileAttrs = tertiary[ASSET_TYPE.TILE_ATTRS]
    ? parseTileAttributes(tertiary[ASSET_TYPE.TILE_ATTRS].data)
    : null;
  
  // Debug: log what assets are in each segment
  console.log('[ENGINE] Primary asset IDs:', Object.keys(primary).map(Number).sort((a,b) => a-b));
  console.log('[ENGINE] Tertiary asset IDs:', Object.keys(tertiary).map(Number).sort((a,b) => a-b));
  console.log('[ENGINE] Stage sprites:', stageSprites.count, 'IDs:', stageSprites.sprites.slice(0, 5).map(s => `0x${s.id.toString(16)}`));
  console.log('[ENGINE] Shared sprites:', sharedSprites.count, 'IDs:', sharedSprites.sprites.slice(0, 5).map(s => `0x${s.id.toString(16)}`));
  console.log('[ENGINE] Entities parsed:', entities.length);
  if (tileAttrs) {
    console.log('[ENGINE] Tile attrs:', tileAttrs.width, 'x', tileAttrs.height, '=', tileAttrs.width * tileAttrs.height, 'tiles');
  }
  
  // Check if clayball sprite ID is in the container
  const clayballId = 0x09406d8a;
  const foundClayball = sprites.sprites.find(s => s.id === clayballId);
  console.log('[ENGINE] Clayball sprite ID 0x09406d8a found:', foundClayball ? `index ${foundClayball.index}` : 'NOT FOUND');
  
  return {
    level,
    stageIndex,
    tileHeader,
    pixelData,
    paletteIndices,
    flagsData,
    palettes,
    layers,
    tilemaps,
    sprites,
    entities,
    tileAttrs,
    // Keep raw sprite container data for on-demand frame decoding
    spriteContainerData: tertiary[ASSET_TYPE.SPRITES]?.data || null,
    sharedSpriteContainerData: primary?.[ASSET_TYPE.SPRITES]?.data || null,
  };
}

// ============================================================================
// Sprite System (Asset 600 from Primary and Tertiary)
// ============================================================================

/**
 * Parse sprite container TOC
 * 
 * @param {Uint8Array} containerData - Raw sprite container data
 * @param {boolean} isShared - True if sprites are from Primary segment (level-wide/shared)
 * @returns {{ count: number, sprites: Array<{ index, id, size, offset, isShared }> }}
 */
function parseSpriteContainer(containerData, isShared = false) {
  if (!containerData || containerData.length < 4) {
    return { count: 0, sprites: [] };
  }
  
  const view = new DataView(containerData.buffer, containerData.byteOffset);
  const count = view.getUint32(0, true);
  
  // Sanity check
  if (count > 1000 || count * 12 + 4 > containerData.length) {
    return { count: 0, sprites: [] };
  }
  
  const sprites = [];
  for (let i = 0; i < count; i++) {
    const entryOffset = 4 + i * 12;
    const id = view.getUint32(entryOffset, true);
    const size = view.getUint32(entryOffset + 4, true);
    const offset = view.getUint32(entryOffset + 8, true);
    
    if (offset + 12 <= containerData.length) {
      sprites.push({ index: i, id, size, offset, isShared });
    }
  }
  
  return { count, sprites };
}

/**
 * Parse a sprite's header (12 bytes at sprite offset)
 */
function parseSpriteHeader(containerData, spriteOffset) {
  if (!containerData || spriteOffset + 12 > containerData.length) {
    return null;
  }
  
  const view = new DataView(containerData.buffer, containerData.byteOffset + spriteOffset);
  
  return {
    animationCount: view.getUint16(0, true),
    frameMetaOffset: view.getUint16(2, true),
    rleDataOffset: view.getUint32(4, true),
    paletteOffset: view.getUint32(8, true),
  };
}

/**
 * Parse animation entries for a sprite
 */
function parseSpriteAnimations(containerData, spriteOffset, animationCount) {
  const animations = [];
  const animBase = spriteOffset + 12; // Animations start after 12-byte header
  
  for (let i = 0; i < animationCount; i++) {
    const off = animBase + i * 12;
    if (off + 12 > containerData.length) break;
    
    const view = new DataView(containerData.buffer, containerData.byteOffset + off);
    animations.push({
      animationId: view.getUint32(0, true),
      frameCount: view.getUint16(4, true),
      frameOffset: view.getUint16(6, true), // Index into frame metadata array
      flags: view.getUint16(8, true),
      extra: view.getUint16(10, true),
    });
  }
  
  return animations;
}

/**
 * Parse frame metadata (36 bytes per frame)
 */
function parseSpriteFrameMetadata(containerData, spriteOffset, frameMetaOffset, frameIndex) {
  const frameOff = spriteOffset + frameMetaOffset + frameIndex * 36;
  if (frameOff + 36 > containerData.length) return null;
  
  const view = new DataView(containerData.buffer, containerData.byteOffset + frameOff);
  
  return {
    flags: view.getUint16(4, true),
    renderX: view.getInt16(6, true),  // Signed
    renderY: view.getInt16(8, true),  // Signed
    renderWidth: view.getUint16(10, true),
    renderHeight: view.getUint16(12, true),
    anchorX: view.getInt16(18, true), // Signed
    anchorY: view.getInt16(20, true), // Signed
    clipWidth: view.getUint16(22, true),
    clipHeight: view.getUint16(24, true),
    rleOffset: view.getUint32(32, true), // Offset from sprite's rleDataOffset
  };
}

/**
 * Parse embedded sprite palette (256 colors, 512 bytes)
 */
function parseSpritePalette(containerData, spriteOffset, paletteOffset) {
  const palOff = spriteOffset + paletteOffset;
  if (palOff + 512 > containerData.length) return null;
  
  const view = new DataView(containerData.buffer, containerData.byteOffset + palOff);
  const palette = [];
  
  for (let i = 0; i < 256; i++) {
    const color = view.getUint16(i * 2, true);
    if (i === 0) {
      palette.push([0, 0, 0, 0]); // Index 0 is transparent
    } else {
      const r = (color & 0x1F) * 8;
      const g = ((color >> 5) & 0x1F) * 8;
      const b = ((color >> 10) & 0x1F) * 8;
      palette.push([r, g, b, 255]);
    }
  }
  
  return palette;
}

/**
 * Decode RLE-encoded sprite frame to pixel array
 * 
 * RLE Command format (u16):
 *   Bit 15:    New line (advance Y, reset X)
 *   Bits 14-8: Skip count (transparent pixels)
 *   Bits 7-0:  Copy count (literal pixels)
 */
function decodeRLEFrame(containerData, spriteOffset, rleDataOffset, frameRLEOffset, width, height) {
  const rleStart = spriteOffset + rleDataOffset + frameRLEOffset;
  if (rleStart + 2 > containerData.length) {
    return new Uint8Array(width * height);
  }
  
  const view = new DataView(containerData.buffer, containerData.byteOffset + rleStart);
  const cmdCount = view.getUint16(0, true);
  
  if (cmdCount === 0 || cmdCount > 10000) {
    return new Uint8Array(width * height);
  }
  
  const pixels = new Uint8Array(width * height);
  let x = 0, y = 0, pixelIdx = 0;
  const pixelDataStart = rleStart + 2 + cmdCount * 2;
  
  for (let i = 0; i < cmdCount; i++) {
    const cmdOffset = 2 + i * 2;
    if (rleStart + cmdOffset + 2 > containerData.length) break;
    
    const cmd = view.getUint16(cmdOffset, true);
    const newLine = (cmd & 0x8000) !== 0;
    const skip = (cmd >> 8) & 0x7F;
    const copyCount = cmd & 0xFF;
    
    if (newLine) {
      y++;
      x = 0;
    }
    
    x += skip;
    
    for (let j = 0; j < copyCount; j++) {
      if (y < height && x < width) {
        const srcIdx = pixelDataStart + pixelIdx - containerData.byteOffset;
        if (srcIdx >= 0 && srcIdx < containerData.length) {
          pixels[y * width + x] = containerData[srcIdx];
        }
      }
      pixelIdx++;
      x++;
    }
  }
  
  return pixels;
}

/**
 * Get a decoded sprite frame with pixels and palette
 * Returns { pixels: Uint8Array, palette: RGBA[], width, height, meta }
 */
function getSpriteFrame(containerData, spriteEntry, animIndex = 0, frameIndex = 0) {
  const header = parseSpriteHeader(containerData, spriteEntry.offset);
  if (!header) return null;
  
  const animations = parseSpriteAnimations(containerData, spriteEntry.offset, header.animationCount);
  if (animIndex >= animations.length) animIndex = 0;
  
  const anim = animations[animIndex];
  if (!anim || frameIndex >= anim.frameCount) frameIndex = 0;
  
  // Frame metadata index is anim.frameOffset + frameIndex
  const frameMetaIndex = anim.frameOffset + frameIndex;
  const meta = parseSpriteFrameMetadata(containerData, spriteEntry.offset, header.frameMetaOffset, frameMetaIndex);
  if (!meta || meta.renderWidth === 0 || meta.renderHeight === 0) return null;
  
  const palette = parseSpritePalette(containerData, spriteEntry.offset, header.paletteOffset);
  if (!palette) return null;
  
  const pixels = decodeRLEFrame(
    containerData,
    spriteEntry.offset,
    header.rleDataOffset,
    meta.rleOffset,
    meta.renderWidth,
    meta.renderHeight
  );
  
  return {
    pixels,
    palette,
    width: meta.renderWidth,
    height: meta.renderHeight,
    meta,
    header,
    animation: anim,
  };
}

// ============================================================================
// Exports
// ============================================================================

const GameEngine = {
  // Asset type constants
  ASSET_TYPE,
  TILE_FLAG,
  LAYER_ENTRY_SIZE,
  
  // Entity system
  ENTITY_TYPES,
  KNOWN_SPRITE_IDS,
  PLAYER_SPRITE_ID,
  parseEntities,
  findSpriteById,
  getSpriteForEntity,
  
  // Color conversion
  psxColorToRGBA,
  
  // Tile system
  parseTileHeader,
  getTotalTileCount,
  parsePalettes,
  getTile,
  
  // Layer system
  parseLayerEntry,
  parseLayers,
  isLayerVisible,
  
  // Tilemap access
  parseTilemaps,
  getTilemapEntry,
  
  // Sprite system
  parseSpriteContainer,
  parseSpriteHeader,
  parseSpriteAnimations,
  parseSpriteFrameMetadata,
  parseSpritePalette,
  decodeRLEFrame,
  getSpriteFrame,
  
  // High-level loading
  loadStage,
};

// Make available globally
if (typeof window !== 'undefined') {
  window.GameEngine = GameEngine;
}
