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
  SPRITES: 600,           // Sprite/entity graphics
  COLLISION: 601,         // Collision data
};

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
 * Returns the 11-bit tile index (lower bits of u16)
 */
function getTilemapEntry(tilemapData, x, y, layerWidth) {
  const idx = (y * layerWidth + x) * 2;
  if (idx + 2 > tilemapData.length) return 0;
  
  const view = new DataView(tilemapData.buffer, tilemapData.byteOffset);
  const raw = view.getUint16(idx, true);
  
  // Lower 11 bits are tile index, upper bits are flags
  return raw & 0x7FF;
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
  
  const { secondary, tertiary } = assets;
  
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
  
  // High-level loading
  loadStage,
};

// Make available globally
if (typeof window !== 'undefined') {
  window.GameEngine = GameEngine;
}
