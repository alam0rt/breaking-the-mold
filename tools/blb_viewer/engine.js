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
  SPRITE_METADATA: 500,   // Sprite metadata
  ENTITIES: 501,          // Entity placement data (24-byte structures)
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
// Entity System (Asset 501)
// ============================================================================

/**
 * Parse entity placement data (Asset 501)
 * 
 * Structure (24 bytes per entity):
 *   0x00: u16 x1          - Bounding box min X (pixels)
 *   0x02: u16 y1          - Bounding box min Y (pixels)
 *   0x04: u16 x2          - Bounding box max X (pixels)
 *   0x06: u16 y2          - Bounding box max Y (pixels)
 *   0x08: u16 x_center    - Entity center X (pixels)
 *   0x0A: u16 y_center    - Entity center Y (pixels)
 *   0x0C: u32 padding     - Always 0
 *   0x10: u16 padding2    - Always 0
 *   0x12: u16 entity_type - Type ID
 *   0x14: u32 flags       - Flags (1, 2, 3 observed)
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
    const entityType = view.getUint16(offset + 0x12, true);
    const flags = view.getUint32(offset + 0x14, true);
    
    // Skip entries that look like padding/invalid
    if (x1 === 0 && y1 === 0 && x2 === 0 && y2 === 0) {
      continue;
    }
    
    entities.push({
      index: i,
      x1, y1, x2, y2,
      xCenter, yCenter,
      width: x2 - x1,
      height: y2 - y1,
      type: entityType,
      flags,
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
  
  // Parse sprites from tertiary Asset 600
  const sprites = tertiary[ASSET_TYPE.SPRITES]
    ? parseSpriteContainer(tertiary[ASSET_TYPE.SPRITES].data)
    : { count: 0, sprites: [] };
  
  // Parse entities from tertiary Asset 501
  const entities = tertiary[ASSET_TYPE.ENTITIES]
    ? parseEntities(tertiary[ASSET_TYPE.ENTITIES].data)
    : [];
  
  // Debug: log what assets are in tertiary
  console.log('[ENGINE] Tertiary asset IDs:', Object.keys(tertiary).map(Number).sort((a,b) => a-b));
  console.log('[ENGINE] Entities parsed:', entities.length);
  
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
    // Keep raw sprite container data for on-demand frame decoding
    spriteContainerData: tertiary[ASSET_TYPE.SPRITES]?.data || null,
  };
}

// ============================================================================
// Sprite System (Asset 600 from Tertiary)
// ============================================================================

/**
 * Parse sprite container TOC
 * Returns { count, sprites: [{ id, size, offset }] }
 */
function parseSpriteContainer(containerData) {
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
      sprites.push({ index: i, id, size, offset });
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
