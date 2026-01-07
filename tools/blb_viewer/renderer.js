/**
 * Skullmonkeys BLB Renderer
 * 
 * Handles canvas rendering, parallax, tile drawing, and minimap.
 * Depends on GameEngine for tile/layer access.
 */

// ============================================================================
// Constants
// ============================================================================

// TILE_INDEX_MASK is defined in engine.js (loaded before renderer.js)
// Bits 0-11: tile index (12 bits), Bits 12-15: color tint selector

// Minimap constants
const MINIMAP_MAX_WIDTH = 160;
const MINIMAP_MAX_HEIGHT = 80;

// Animation timing: TWOS = 2 frames per animation step on PSX (30fps NTSC)
// At 60fps browser, we advance every 12 ticks ≈ 200ms per frame
const ANIMATION_TICK_RATE = 12;

// ============================================================================
// Renderer State
// ============================================================================

const RendererState = {
  canvas: null,
  ctx: null,
  minimapCanvas: null,
  minimapCtx: null,
  
  // Viewport
  cameraX: 0,
  cameraY: 0,
  zoom: 2,
  
  // PSX display dimensions (PAL: 320×256, NTSC: 320×240)
  // The game targets PAL by default (SLES-01090)
  psxWidth: 320,
  psxHeight: 256,
  
  // Level dimensions (calculated from layers)
  levelWidth: 0,
  levelHeight: 0,
  
  // Display options
  showEntities: true,
  showEntityDebug: true, // Show entity bounding boxes and type labels
  showGrid: false,
  showMinimap: false,
  showSprites: true, // Render sprites at entity positions
  showPlayer: true, // Show player sprite at spawn point
  animateSprites: false, // Animate sprite frames
  spriteFrameIndex: 0, // Current animation frame index (global for now)
  showCollision: false, // Show collision/tile attribute overlay
  
  // 90s mode options
  mode90s: false,
  staticBg: false,
  mouseFollow: false,
  
  // Layer visibility (true = visible, index by layer index)
  layerVisibility: [],
  
  // Sprite cache (spriteId_animIdx_frameIdx -> decoded frame ImageData)
  spriteCache: new Map(),
  
  // Tile cache (tileIndex -> {canvas, hasTransparency})
  // Pre-rendered 16x16 tiles for fast blitting
  tileCache: new Map(),
  tileCacheStageId: null, // Track which stage the cache is for
  
  // Cached stage data
  cachedStage: null,
};

// ============================================================================
// Tile Cache - Pre-render tiles for fast blitting
// ============================================================================

/**
 * Get or create a cached 16x16 tile canvas
 * @param {number} tileIdx - Tile index
 * @param {object} stageData - Stage data containing tile assets
 * @returns {object|null} {canvas, hasTransparency} or null if empty/entity
 */
function getCachedTile(tileIdx, stageData) {
  if (tileIdx === 0) return null;
  
  // Check cache
  const cached = RendererState.tileCache.get(tileIdx);
  if (cached !== undefined) return cached;
  
  // Get tile data from engine
  const { tileHeader, pixelData, flagsData, paletteIndices, palettes } = stageData;
  const tile = GameEngine.getTile(tileIdx, tileHeader, pixelData, flagsData, paletteIndices, palettes);
  
  if (!tile || tile.type === 'entity') {
    RendererState.tileCache.set(tileIdx, null);
    return null;
  }
  
  // Create 16x16 canvas for this tile
  const tileCanvas = document.createElement('canvas');
  tileCanvas.width = 16;
  tileCanvas.height = 16;
  const tileCtx = tileCanvas.getContext('2d');
  const imageData = tileCtx.createImageData(16, 16);
  const data = imageData.data;
  
  const tileSize = tile.size;
  const scale = 16 / tileSize;
  let hasTransparency = false;
  
  // Render tile pixels to the cached canvas
  for (let py = 0; py < tileSize; py++) {
    for (let px = 0; px < tileSize; px++) {
      const colorIdx = tile.pixels[py][px];
      
      if (colorIdx === 0) {
        hasTransparency = true;
        continue; // Transparent pixel
      }
      
      const color = tile.palette[colorIdx] || [255, 0, 255, 255];
      const alpha = tile.isSemiTransparent ? 128 : (color[3] !== undefined ? color[3] : 255);
      
      if (alpha < 255) hasTransparency = true;
      
      // Scale pixel to 16x16
      for (let dy = 0; dy < scale; dy++) {
        for (let dx = 0; dx < scale; dx++) {
          const ix = px * scale + dx;
          const iy = py * scale + dy;
          const offset = (iy * 16 + ix) * 4;
          data[offset] = color[0];
          data[offset + 1] = color[1];
          data[offset + 2] = color[2];
          data[offset + 3] = alpha;
        }
      }
    }
  }
  
  tileCtx.putImageData(imageData, 0, 0);
  
  const result = { canvas: tileCanvas, hasTransparency };
  RendererState.tileCache.set(tileIdx, result);
  return result;
}

/**
 * Clear tile cache (call when loading new stage)
 */
function clearTileCache() {
  RendererState.tileCache.clear();
  RendererState.tileCacheStageId = null;
}

// ============================================================================
// Initialization
// ============================================================================

function initRenderer(canvas, minimapCanvas) {
  RendererState.canvas = canvas;
  RendererState.ctx = canvas.getContext('2d', { alpha: false });
  RendererState.minimapCanvas = minimapCanvas;
  RendererState.minimapCtx = minimapCanvas.getContext('2d');
}

// ============================================================================
// Main Stage Rendering
// ============================================================================

function renderStage(stageData, options = {}) {
  if (!stageData || !stageData.tileHeader) {
    console.warn('No stage data to render');
    return null;
  }
  
  // Clear sprite cache if loading a different stage
  if (RendererState.cachedStage !== stageData) {
    RendererState.spriteCache.clear();
    clearTileCache();
  }
  
  // Ensure tile cache is for this stage
  if (RendererState.tileCacheStageId !== stageData) {
    clearTileCache();
    RendererState.tileCacheStageId = stageData;
  }
  
  // Update state from options
  Object.assign(RendererState, options);
  RendererState.cachedStage = stageData;
  
  const { tileHeader, pixelData, paletteIndices, flagsData, palettes, layers, tilemaps } = stageData;
  const ctx = RendererState.ctx;
  const canvas = RendererState.canvas;
  
  // Calculate full level size from layers
  let levelWidth = 0;
  let levelHeight = 0;
  
  for (const layer of layers) {
    const layerPixelWidth = layer.width * 16;
    const layerPixelHeight = layer.height * 16;
    levelWidth = Math.max(levelWidth, layerPixelWidth);
    levelHeight = Math.max(levelHeight, layerPixelHeight);
  }
  
  if (levelWidth === 0 || levelHeight === 0) {
    levelWidth = tileHeader.levelWidth * 16 || 320;
    levelHeight = tileHeader.levelHeight * 16 || 256;
  }
  
  // Store level dimensions
  RendererState.levelWidth = levelWidth;
  RendererState.levelHeight = levelHeight;
  
  // Determine render dimensions based on 90s mode
  const zoom = RendererState.zoom;
  let viewWidth, viewHeight;
  
  if (RendererState.mode90s) {
    viewWidth = RendererState.psxWidth;
    viewHeight = RendererState.psxHeight;
  } else {
    viewWidth = levelWidth;
    viewHeight = levelHeight;
  }
  
  // Set canvas size
  canvas.width = viewWidth;
  canvas.height = viewHeight;
  canvas.style.width = `${viewWidth * zoom}px`;
  canvas.style.height = `${viewHeight * zoom}px`;
  
  // Clear with background color
  const [bgR, bgG, bgB] = tileHeader.bgColor;
  ctx.fillStyle = `rgb(${bgR}, ${bgG}, ${bgB})`;
  ctx.fillRect(0, 0, viewWidth, viewHeight);
  
  // Render layers back-to-front using cached tile canvases
  // This is much faster than per-pixel manipulation
  for (let layerIdx = 0; layerIdx < layers.length; layerIdx++) {
    const layer = layers[layerIdx];
    
    // Skip non-renderable layers
    if (!GameEngine.isLayerVisible(layer)) continue;
    
    // Skip user-hidden layers
    if (RendererState.layerVisibility[layerIdx] === false) continue;
    
    // Get tilemap for this layer
    const tilemap = tilemaps[layerIdx];
    if (!tilemap) continue;
    
    const tilemapData = tilemap.data;
    const tilemapView = new DataView(tilemapData.buffer, tilemapData.byteOffset);
    
    // Layer position offset (in pixels)
    const layerOffsetX = layer.xOffset * 16;
    const layerOffsetY = layer.yOffset * 16;
    
    // Parallax scroll factors (already converted to 0.0-1.0 by engine)
    const scrollFactorX = layer.scrollX;
    const scrollFactorY = layer.scrollY;
    
    // Pre-calculate camera offsets for this layer (avoid recalculating per-tile)
    const cameraOffsetX = RendererState.mode90s ? Math.floor(RendererState.cameraX * scrollFactorX) : 0;
    const cameraOffsetY = RendererState.mode90s ? Math.floor(RendererState.cameraY * scrollFactorY) : 0;
    const useStaticBg = RendererState.mode90s && RendererState.staticBg && scrollFactorX < 0.5;
    
    // Calculate visible tile range to avoid iterating over off-screen tiles
    let startTX = 0, endTX = layer.width;
    let startTY = 0, endTY = layer.height;
    
    if (RendererState.mode90s && !useStaticBg) {
      // Only render tiles that could be visible
      startTX = Math.max(0, Math.floor((cameraOffsetX - layerOffsetX - 16) / 16));
      endTX = Math.min(layer.width, Math.ceil((cameraOffsetX + viewWidth - layerOffsetX + 16) / 16));
      startTY = Math.max(0, Math.floor((cameraOffsetY - layerOffsetY - 16) / 16));
      endTY = Math.min(layer.height, Math.ceil((cameraOffsetY + viewHeight - layerOffsetY + 16) / 16));
    }
    
    // Render visible tiles
    for (let ty = startTY; ty < endTY; ty++) {
      for (let tx = startTX; tx < endTX; tx++) {
        const tileDataIdx = (ty * layer.width + tx) * 2;
        if (tileDataIdx + 2 > tilemapData.length) continue;
        
        const rawIdx = tilemapView.getUint16(tileDataIdx, true);
        const tileIdx = rawIdx & TILE_INDEX_MASK;
        
        if (tileIdx === 0) continue;
        
        // Calculate world position (including layer offset)
        const worldX = layerOffsetX + tx * 16;
        const worldY = layerOffsetY + ty * 16;
        
        // Calculate screen position
        let screenX, screenY;
        if (useStaticBg) {
          screenX = worldX % viewWidth;
          if (screenX < 0) screenX += viewWidth;
          screenY = worldY;
        } else if (RendererState.mode90s) {
          screenX = worldX - cameraOffsetX;
          screenY = worldY - cameraOffsetY;
        } else {
          screenX = worldX;
          screenY = worldY;
        }
        
        // Skip tiles completely outside viewport (redundant with range calc but cheap check)
        if (screenX + 16 <= 0 || screenX >= viewWidth || screenY + 16 <= 0 || screenY >= viewHeight) {
          continue;
        }
        
        // Get cached tile canvas (pre-rendered 16x16)
        const cachedTile = getCachedTile(tileIdx, stageData);
        if (!cachedTile) continue;
        
        // Draw cached tile using fast canvas blit
        ctx.drawImage(cachedTile.canvas, screenX, screenY);
      }
    }
  }
  
  // Get entities from Asset 501 (proper entity placement data)
  const entities = stageData.entities || [];
  
  // Render sprites at entity positions (or as gallery if no entities)
  let spriteCount = 0;
  let playerRendered = false;
  if (RendererState.showSprites && stageData.sprites && stageData.sprites.count > 0 && 
      (stageData.spriteContainerData || stageData.sharedSpriteContainerData)) {
    if (entities.length > 0) {
      spriteCount = renderSpritesAtEntities(ctx, stageData, entities, viewWidth, viewHeight);
    } else {
      // No entities - render sprites as a gallery for debugging
      spriteCount = renderSpriteGallery(ctx, stageData, viewWidth, viewHeight);
    }
    
    // Render player sprite at spawn point
    if (RendererState.showPlayer) {
      playerRendered = renderPlayerAtSpawn(ctx, stageData, viewWidth, viewHeight);
    }
  }
  
  // Draw collision/tile attribute overlay
  if (RendererState.showCollision && stageData.tileAttrs) {
    renderCollisionOverlay(ctx, stageData.tileAttrs, viewWidth, viewHeight);
  }
  
  // Draw entity bounding boxes on top (debug overlay)
  if (RendererState.showEntities && entities.length > 0) {
    ctx.font = '8px monospace';
    
    for (const entity of entities) {
      // Calculate screen position (apply camera offset in 90s mode)
      let screenX = entity.x1;
      let screenY = entity.y1;
      const width = entity.width || 16;
      const height = entity.height || 16;
      
      if (RendererState.mode90s) {
        screenX -= RendererState.cameraX;
        screenY -= RendererState.cameraY;
      }
      
      // Get entity type info for color
      const typeInfo = GameEngine.ENTITY_TYPES[entity.type];
      let boxColor = 'rgba(255, 255, 0, 0.5)'; // Default yellow
      let strokeColor = '#ff0';
      
      if (typeInfo) {
        if (typeInfo.invisible) {
          // Triggers are transparent/dashed
          boxColor = 'rgba(128, 128, 128, 0.2)';
          strokeColor = '#888';
        } else if (typeInfo.color) {
          // Use entity type color
          strokeColor = typeInfo.color;
          boxColor = typeInfo.color.replace(')', ', 0.3)').replace('rgb', 'rgba').replace('##', '#');
          // Handle hex colors
          if (strokeColor.startsWith('#')) {
            const hex = strokeColor.slice(1);
            const r = parseInt(hex.substr(0, 2) || hex.substr(0, 1).repeat(2), 16);
            const g = parseInt(hex.substr(2, 2) || hex.substr(1, 1).repeat(2), 16);
            const b = parseInt(hex.substr(4, 2) || hex.substr(2, 1).repeat(2), 16);
            boxColor = `rgba(${r}, ${g}, ${b}, 0.3)`;
          }
        }
      }
      
      // Draw bounding box
      ctx.fillStyle = boxColor;
      ctx.fillRect(screenX, screenY, width, height);
      ctx.strokeStyle = strokeColor;
      ctx.lineWidth = 1;
      ctx.strokeRect(screenX + 0.5, screenY + 0.5, width - 1, height - 1);
      
      // Draw type ID and name
      ctx.fillStyle = strokeColor;
      const typeName = entity.typeName || `T${entity.type}`;
      ctx.fillText(`${typeName}`, screenX + 2, screenY + 10);
      ctx.fillStyle = boxColor;
    }
  }
  
  // Draw grid
  if (RendererState.showGrid) {
    ctx.strokeStyle = 'rgba(255, 255, 255, 0.2)';
    ctx.lineWidth = 1;
    
    for (let x = 0; x <= viewWidth; x += 16) {
      ctx.beginPath();
      ctx.moveTo(x + 0.5, 0);
      ctx.lineTo(x + 0.5, viewHeight);
      ctx.stroke();
    }
    
    for (let y = 0; y <= viewHeight; y += 16) {
      ctx.beginPath();
      ctx.moveTo(0, y + 0.5);
      ctx.lineTo(viewWidth, y + 0.5);
      ctx.stroke();
    }
  }
  
  // Update minimap if enabled
  if (RendererState.showMinimap) {
    updateMinimap(stageData);
  }
  
  // Return render info for UI
  return {
    viewWidth,
    viewHeight,
    levelWidth,
    levelHeight,
    // totalTiles, // not defined
    layerCount: layers.length,
    paletteCount: palettes.length,
    entityCount: entities.length,
    spriteCount: stageData.sprites ? stageData.sprites.count : 0,
    spritesRendered: spriteCount,
    cameraX: RendererState.cameraX,
    cameraY: RendererState.cameraY,
  };
}

// ============================================================================
// Collision/Tile Attribute Overlay
// ============================================================================

/**
 * Render tile attribute overlay (collision map from Asset 500)
 * 
 * Colors:
 *   Red (0x02): Solid/collision
 *   Yellow (0x65/101): Entity spawn zone
 *   Green (0x53/83): Checkpoint
 *   Blue: Other non-zero values
 */
function renderCollisionOverlay(ctx, tileAttrs, viewWidth, viewHeight) {
  if (!tileAttrs || !tileAttrs.data) return;
  
  const { width, height, data } = tileAttrs;
  
  // Iterate over visible tiles
  for (let ty = 0; ty < height; ty++) {
    for (let tx = 0; tx < width; tx++) {
      const attr = data[ty * width + tx];
      
      // Skip passable tiles (most common)
      if (attr === 0) continue;
      
      // Calculate screen position
      let screenX = tx * 16;
      let screenY = ty * 16;
      
      // Apply camera offset in 90s mode
      if (RendererState.mode90s) {
        screenX -= RendererState.cameraX;
        screenY -= RendererState.cameraY;
        
        // Skip tiles outside viewport
        if (screenX + 16 < 0 || screenY + 16 < 0 || 
            screenX >= viewWidth || screenY >= viewHeight) {
          continue;
        }
      }
      
      // Choose color based on attribute value
      let fillColor;
      switch (attr) {
        case 2:   // Solid/collision
          fillColor = 'rgba(255, 0, 0, 0.4)';
          break;
        case 101: // Entity spawn zone (0x65)
          fillColor = 'rgba(255, 255, 0, 0.4)';
          break;
        case 83:  // Checkpoint (0x53)
          fillColor = 'rgba(0, 255, 0, 0.4)';
          break;
        default:  // Other values
          fillColor = 'rgba(0, 128, 255, 0.4)';
          break;
      }
      
      ctx.fillStyle = fillColor;
      ctx.fillRect(screenX, screenY, 16, 16);
    }
  }
}

// ============================================================================
// Sprite Rendering
// ============================================================================

/**
 * Render sprites at entity marker positions
 * 
 * SPRITE ID LOOKUP (VERIFIED via Ghidra 2026-01-07):
 * Entity type → sprite ID mapping is HARDCODED in game code, not in BLB data.
 * 
 * The game's sprite lookup chain:
 *   InitSpriteContext @ 0x8007bc3c → LookupSpriteById @ 0x8007bb10
 *     → FindSpriteInTOC @ 0x8007b968 (searches Asset 600 TOC)
 * 
 * Known hardcoded sprite IDs:
 *   0x21842018 - Player sprite
 *   0xb8700ca1, 0xe2f188, 0xa9240484, 0xe8628689, 0x88a28194, etc.
 * 
 * Since we don't have the complete entity→sprite mapping, we:
 * 1. Show entity bounding boxes and type labels (with variant info)
 * 2. Color-code entities by render layer (green=1, yellow=2, red=3)
 * 3. Attempt best-effort sprite rendering by cycling through available sprites
 * 
 * To implement proper entity sprites, we'd need to trace each entity type's
 * spawn dispatcher in game code and extract the sprite ID constants.
 * 
 * @returns {number} Number of entities rendered
 */
function renderSpritesAtEntities(ctx, stageData, entities, viewWidth, viewHeight) {
  const { sprites, spriteContainerData, sharedSpriteContainerData } = stageData;
  
  // Log available sprites (once per stage load)
  if (sprites?.sprites && !RendererState.spriteCache.has('logged_sprites')) {
    console.log(`[SPRITES] Available sprites: ${sprites.sprites.length}`);
    sprites.sprites.forEach((s, i) => {
      console.log(`  [${i}] id=0x${s.id.toString(16).padStart(8, '0')}`);
    });
    RendererState.spriteCache.set('logged_sprites', true);
  }
  
  // Get animation frame index for TWOS timing
  const globalFrameIdx = RendererState.spriteFrameIndex;
  
  let renderedCount = 0;
  
  for (let i = 0; i < entities.length; i++) {
    const entity = entities[i];
    
    // Calculate render position
    let renderX = entity.xCenter;
    let renderY = entity.yCenter;
    let bboxX = entity.x1;
    let bboxY = entity.y1;
    
    // Apply camera offset in 90s mode
    if (RendererState.mode90s) {
      renderX -= RendererState.cameraX;
      renderY -= RendererState.cameraY;
      bboxX -= RendererState.cameraX;
      bboxY -= RendererState.cameraY;
    }
    
    // Skip if completely outside viewport
    const bboxW = entity.width || (entity.x2 - entity.x1);
    const bboxH = entity.height || (entity.y2 - entity.y1);
    
    if (bboxX + bboxW < 0 || bboxX > viewWidth ||
        bboxY + bboxH < 0 || bboxY > viewHeight) {
      continue;
    }
    
    // Get sprite for this entity using the engine's lookup system
    // This uses entity type mapping with fallback to variant-based lookup
    if (sprites && sprites.sprites && sprites.sprites.length > 0 && (spriteContainerData || sharedSpriteContainerData)) {
      const spriteEntry = GameEngine.getSpriteForEntity(sprites, entity);
      if (spriteEntry) {
        // Determine which container to use based on isShared flag
        const containerData = spriteEntry.isShared ? sharedSpriteContainerData : spriteContainerData;
        if (!containerData) continue;
        
        // Log sprite selection (once per type)
        const typeLogKey = `logged_type_${entity.type}`;
        if (!RendererState.spriteCache.has(typeLogKey)) {
          const source = spriteEntry.isShared ? 'shared' : 'stage';
          console.log(`[SPRITES] Entity type ${entity.type} → sprite #${spriteEntry.index} (id=0x${spriteEntry.id.toString(16)}, ${source})`);
          RendererState.spriteCache.set(typeLogKey, true);
        }
        
        // Get frame count from cache
        let frameCount = 1;
        const headerCacheKey = `header_${spriteEntry.id}`;
        let cachedInfo = RendererState.spriteCache.get(headerCacheKey);
        
        if (!cachedInfo) {
          const firstFrame = GameEngine.getSpriteFrame(containerData, spriteEntry, 0, 0);
          if (firstFrame && firstFrame.animation) {
            frameCount = firstFrame.animation.frameCount || 1;
            cachedInfo = { frameCount };
            RendererState.spriteCache.set(headerCacheKey, cachedInfo);
          }
        } else {
          frameCount = cachedInfo.frameCount || 1;
        }
        
        // Use entity variant as animation frame if it's a collectible
        let currentFrame = globalFrameIdx % frameCount;
        if (entity.type === 2) {
          // Clayballs: animate through frames, starting from variant
          currentFrame = (globalFrameIdx + (entity.variant || 0)) % frameCount;
        } else if (entity.type === 8 && entity.variant < frameCount) {
          // Items: variant selects specific frame (static)
          currentFrame = entity.variant % frameCount;
        }
        
        const cacheKey = `${spriteEntry.id}_0_${currentFrame}`;
        let frameData = RendererState.spriteCache.get(cacheKey);
        
        if (!frameData) {
          try {
            frameData = GameEngine.getSpriteFrame(containerData, spriteEntry, 0, currentFrame);
            if (frameData) {
              const imgData = createSpriteImageData(frameData);
              frameData.imageData = imgData;
              RendererState.spriteCache.set(cacheKey, frameData);
            }
          } catch (err) {
            console.warn(`[SPRITES] Failed to decode entity sprite 0x${spriteEntry.id.toString(16)}:`, err.message);
          }
        }
        
        if (frameData && frameData.imageData) {
          const sprRenderX = renderX + (frameData.meta.renderX || 0);
          const sprRenderY = renderY + (frameData.meta.renderY || 0);
          drawSpriteFrame(ctx, frameData, sprRenderX, sprRenderY, viewWidth, viewHeight);
        }
      }
    }
    
    // Draw entity bounding box and type label if debug enabled
    if (RendererState.showEntityDebug) {
      // Use typeShortName from entity (set by engine.js parseEntities)
      const typeName = entity.typeShortName || `T${entity.type}`;
      
      // Color by layer (1=green, 2=yellow, 3=red)
      const layerColors = {
        1: 'rgba(0, 255, 0, 0.7)',
        2: 'rgba(255, 255, 0, 0.7)',
        3: 'rgba(255, 100, 100, 0.7)',
      };
      const boxColor = layerColors[entity.layer] || 'rgba(255, 255, 0, 0.7)';
      
      // Draw bounding box
      ctx.strokeStyle = boxColor;
      ctx.lineWidth = 1;
      ctx.strokeRect(bboxX, bboxY, bboxW, bboxH);
      
      // Draw center point
      ctx.fillStyle = 'rgba(255, 0, 0, 0.8)';
      ctx.beginPath();
      ctx.arc(renderX, renderY, 2, 0, Math.PI * 2);
      ctx.fill();
      
      // Draw type label with variant info if non-zero
      ctx.font = '8px monospace';
      ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
      const variantStr = entity.variant ? `:${entity.variant}` : '';
      ctx.fillText(`${typeName}${variantStr}`, bboxX + 1, bboxY + 8);
    }
    
    renderedCount++;
  }
  
  return renderedCount;
}

/**
 * Render sprites in a gallery layout for debugging
 * Used when no entity markers are found (e.g., MENU levels)
 */
function renderSpriteGallery(ctx, stageData, viewWidth, viewHeight) {
  const { sprites, spriteContainerData, sharedSpriteContainerData } = stageData;
  if (!sprites || sprites.count === 0 || (!spriteContainerData && !sharedSpriteContainerData)) return 0;
  
  const globalFrameIdx = RendererState.spriteFrameIndex || 0;
  
  const spriteList = sprites.sprites;
  let renderedCount = 0;
  let x = 10;
  let y = 10;
  let rowHeight = 0;
  const padding = 5;
  
  for (let i = 0; i < Math.min(spriteList.length, 50); i++) { // Limit to 50 sprites
    const spriteEntry = spriteList[i];
    if (!spriteEntry) continue;
    
    // Determine which container to use based on isShared flag
    const containerData = spriteEntry.isShared ? sharedSpriteContainerData : spriteContainerData;
    if (!containerData) continue;
    
    // Get sprite header to find frame count
    const header = GameEngine.parseSpriteHeader(containerData, spriteEntry.offset);
    if (!header) continue;
    
    // Get animations to find total frames in first animation
    const animations = GameEngine.parseSpriteAnimations(containerData, spriteEntry.offset, header.animationCount);
    const anim = animations[0];
    const frameCount = anim ? anim.frameCount : 1;
    
    // Calculate which frame to show (loop within this sprite's frame count)
    const frameIdx = globalFrameIdx % frameCount;
    
    // Get cached or decode sprite frame
    const cacheKey = `${spriteEntry.id}_0_${frameIdx}`;
    let frameData = RendererState.spriteCache.get(cacheKey);
    
    if (!frameData) {
      try {
        frameData = GameEngine.getSpriteFrame(containerData, spriteEntry, 0, frameIdx);
        if (frameData) {
          const imgData = createSpriteImageData(frameData);
          frameData.imageData = imgData;
          RendererState.spriteCache.set(cacheKey, frameData);
        }
      } catch (err) {
        console.warn(`[SPRITES] Failed to decode sprite 0x${spriteEntry.id.toString(16)} frame ${frameIdx}:`, err.message);
      }
    }
    
    if (frameData && frameData.imageData) {
      // Check if sprite fits on current row
      if (x + frameData.width > viewWidth - 10) {
        x = 10;
        y += rowHeight + padding;
        rowHeight = 0;
      }
      
      // Skip if sprite would be off-screen
      if (y + frameData.height > viewHeight - 10) break;
      
      drawSpriteFrame(ctx, frameData, x, y, viewWidth, viewHeight);
      renderedCount++;
      
      x += frameData.width + padding;
      rowHeight = Math.max(rowHeight, frameData.height);
    }
  }
  
  return renderedCount;
}

/**
 * Render the player sprite at the spawn point
 * 
 * The player sprite ID (0x21842018) is hardcoded in InitPlayerEntity (FUN_8001fcf0).
 * The spawn point comes from the tile header (Asset 100).
 * 
 * @param {CanvasRenderingContext2D} ctx - Canvas context
 * @param {Object} stageData - Stage data with sprites, tileHeader
 * @param {number} viewWidth - View width in pixels
 * @param {number} viewHeight - View height in pixels
 * @returns {boolean} True if player was rendered
 */
function renderPlayerAtSpawn(ctx, stageData, viewWidth, viewHeight) {
  const { sprites, spriteContainerData, sharedSpriteContainerData, tileHeader } = stageData;
  if (!sprites || !tileHeader) return false;
  // Need at least one container
  if (!spriteContainerData && !sharedSpriteContainerData) return false;
  
  // Get spawn position from tile header
  const spawnX = tileHeader.spawnX;
  const spawnY = tileHeader.spawnY;
  
  // Skip if spawn is at 0,0 (likely invalid or menu)
  if (spawnX === 0 && spawnY === 0) return false;
  
  // Look up player sprite by ID
  const playerSpriteId = GameEngine.PLAYER_SPRITE_ID;
  const playerSprite = GameEngine.findSpriteById(sprites, playerSpriteId);
  
  // Log what sprites are available for debugging
  if (!playerSprite) {
    console.log(`[PLAYER] Player sprite 0x${playerSpriteId.toString(16)} not found in ${sprites.sprites.length} sprites`);
    console.log(`[PLAYER] Available sprite IDs:`, sprites.sprites.slice(0, 10).map(s => `0x${s.id.toString(16)}`).join(', '));
    
    // Draw placeholder at spawn point
    let renderX = spawnX;
    let renderY = spawnY;
    
    if (RendererState.mode90s) {
      renderX -= RendererState.cameraX;
      renderY -= RendererState.cameraY;
    }
    
    // Draw spawn point marker with "P" for player
    ctx.strokeStyle = '#0f0';
    ctx.lineWidth = 2;
    ctx.fillStyle = 'rgba(0, 255, 0, 0.3)';
    ctx.beginPath();
    ctx.arc(renderX, renderY, 12, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
    
    ctx.fillStyle = '#0f0';
    ctx.font = 'bold 10px monospace';
    ctx.textAlign = 'center';
    ctx.fillText('P', renderX, renderY + 4);
    ctx.textAlign = 'left';
    
    return false;
  }
  
  console.log(`[PLAYER] Found player sprite at offset ${playerSprite.offset} (shared: ${playerSprite.isShared})`);
  
  // Select correct container based on whether sprite is shared (primary) or stage (tertiary)
  const containerToUse = playerSprite.isShared ? sharedSpriteContainerData : spriteContainerData;
  if (!containerToUse) {
    console.log(`[PLAYER] No container available for ${playerSprite.isShared ? 'shared' : 'stage'} sprite`);
    return false;
  }
  
  // Get animation frame
  const globalFrameIdx = RendererState.spriteFrameIndex || 0;
  
  // Get frame count from header
  const header = GameEngine.parseSpriteHeader(containerToUse, playerSprite.offset);
  if (!header) return false;
  
  const animations = GameEngine.parseSpriteAnimations(containerToUse, playerSprite.offset, header.animationCount);
  const anim = animations[0];
  const frameCount = anim ? anim.frameCount : 1;
  const frameIdx = globalFrameIdx % frameCount;
  
  // Get or cache frame data
  const cacheKey = `player_${playerSprite.isShared ? 's' : 'e'}_${frameIdx}`;
  let frameData = RendererState.spriteCache.get(cacheKey);
  
  if (!frameData) {
    frameData = GameEngine.getSpriteFrame(containerToUse, playerSprite, 0, frameIdx);
    if (frameData) {
      const imgData = createSpriteImageData(frameData);
      frameData.imageData = imgData;
      RendererState.spriteCache.set(cacheKey, frameData);
    }
  }
  
  if (!frameData || !frameData.imageData) return false;
  
  // Calculate render position (spawn point with offset from sprite metadata)
  let renderX = spawnX + (frameData.meta.renderX || 0);
  let renderY = spawnY + (frameData.meta.renderY || 0);
  
  if (RendererState.mode90s) {
    renderX -= RendererState.cameraX;
    renderY -= RendererState.cameraY;
  }
  
  // Draw the player sprite
  drawSpriteFrame(ctx, frameData, renderX, renderY, viewWidth, viewHeight);
  
  return true;
}

/**
 * Create ImageData from decoded sprite frame pixels and palette
 */
function createSpriteImageData(frameData) {
  const { pixels, palette, width, height } = frameData;
  
  // Create a temporary canvas to hold the sprite
  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = width;
  tempCanvas.height = height;
  const tempCtx = tempCanvas.getContext('2d');
  const imageData = tempCtx.createImageData(width, height);
  const data = imageData.data;
  
  for (let i = 0; i < pixels.length; i++) {
    const colorIdx = pixels[i];
    const color = palette[colorIdx] || [0, 0, 0, 0];
    
    const offset = i * 4;
    data[offset] = color[0];
    data[offset + 1] = color[1];
    data[offset + 2] = color[2];
    data[offset + 3] = color[3];
  }
  
  // Put to temp canvas to get ImageBitmap for faster drawing
  tempCtx.putImageData(imageData, 0, 0);
  
  return {
    canvas: tempCanvas,
    imageData,
    width,
    height,
  };
}

/**
 * Draw a sprite frame to the main canvas
 */
function drawSpriteFrame(ctx, frameData, x, y, viewWidth, viewHeight) {
  if (!frameData.imageData) return;
  
  // Use drawImage from the cached canvas (supports transparency properly)
  if (frameData.imageData.canvas) {
    ctx.drawImage(frameData.imageData.canvas, Math.floor(x), Math.floor(y));
  }
}

/**
 * Clear sprite cache (call when loading new stage)
 */
function clearSpriteCache() {
  RendererState.spriteCache.clear();
}

// ============================================================================
// Minimap
// ============================================================================

function updateMinimap(stageData) {
  if (!stageData || !stageData.layers || stageData.layers.length === 0) return;
  
  const { layers, tilemaps, tileHeader } = stageData;
  const minimapCtx = RendererState.minimapCtx;
  const minimapCanvas = RendererState.minimapCanvas;
  const totalTiles = GameEngine.getTotalTileCount(tileHeader);
  
  // Calculate level bounds from all layers
  let levelWidth = 0;
  let levelHeight = 0;
  for (const layer of layers) {
    levelWidth = Math.max(levelWidth, layer.width);
    levelHeight = Math.max(levelHeight, layer.height);
  }
  
  if (levelWidth === 0 || levelHeight === 0) return;
  
  // Calculate scale to fit minimap
  const scaleX = MINIMAP_MAX_WIDTH / levelWidth;
  const scaleY = MINIMAP_MAX_HEIGHT / levelHeight;
  const scale = Math.min(scaleX, scaleY, 1);
  
  const mapWidth = Math.ceil(levelWidth * scale);
  const mapHeight = Math.ceil(levelHeight * scale);
  
  // Resize canvas
  minimapCanvas.width = mapWidth;
  minimapCanvas.height = mapHeight;
  
  // Clear with dark background
  minimapCtx.fillStyle = '#1a1a2e';
  minimapCtx.fillRect(0, 0, mapWidth, mapHeight);
  
  // Draw each layer's tilemap as colored blocks
  const layerColors = ['#16213e', '#0f3460', '#1a1a40', '#4a4a6a'];
  
  for (let layerIdx = 0; layerIdx < layers.length; layerIdx++) {
    const layer = layers[layerIdx];
    const tilemap = tilemaps[layerIdx];
    if (!tilemap) continue;
    
    const tilemapData = tilemap.data;
    const tilemapView = new DataView(tilemapData.buffer, tilemapData.byteOffset);
    
    // Only draw foreground layers (high scroll factor)
    const scrollFactor = layer.scrollX;
    if (scrollFactor < 0.5) continue;
    
    const color = layerColors[layerIdx % layerColors.length];
    
    for (let ty = 0; ty < layer.height; ty++) {
      for (let tx = 0; tx < layer.width; tx++) {
        const tileDataIdx = (ty * layer.width + tx) * 2;
        if (tileDataIdx + 2 > tilemapData.length) continue;
        
        const rawIdx = tilemapView.getUint16(tileDataIdx, true);
        const tileIdx = rawIdx & TILE_INDEX_MASK;
        
        if (tileIdx === 0) continue;
        
        const x = Math.floor((layer.xOffset + tx) * scale);
        const y = Math.floor((layer.yOffset + ty) * scale);
        const w = Math.max(1, Math.ceil(scale));
        const h = Math.max(1, Math.ceil(scale));
        
        if (tileIdx > totalTiles) {
          minimapCtx.fillStyle = '#e94560';
        } else {
          minimapCtx.fillStyle = color;
        }
        
        minimapCtx.fillRect(x, y, w, h);
      }
    }
  }
  
  // Draw entity markers on top
  minimapCtx.fillStyle = '#ffd700';
  for (let layerIdx = 0; layerIdx < layers.length; layerIdx++) {
    const layer = layers[layerIdx];
    const tilemap = tilemaps[layerIdx];
    if (!tilemap) continue;
    
    const tilemapData = tilemap.data;
    const tilemapView = new DataView(tilemapData.buffer, tilemapData.byteOffset);
    
    for (let ty = 0; ty < layer.height; ty++) {
      for (let tx = 0; tx < layer.width; tx++) {
        const tileDataIdx = (ty * layer.width + tx) * 2;
        if (tileDataIdx + 2 > tilemapData.length) continue;
        
        const rawIdx = tilemapView.getUint16(tileDataIdx, true);
        const tileIdx = rawIdx & TILE_INDEX_MASK;
        
        if (tileIdx > totalTiles) {
          const x = Math.floor((layer.xOffset + tx) * scale);
          const y = Math.floor((layer.yOffset + ty) * scale);
          minimapCtx.fillRect(x, y, 2, 2);
        }
      }
    }
  }
}

function getMinimapViewport() {
  if (!RendererState.levelWidth || !RendererState.levelHeight) {
    return null;
  }
  
  const mapWidth = RendererState.minimapCanvas.width;
  const mapHeight = RendererState.minimapCanvas.height;
  
  const scaleX = mapWidth / (RendererState.levelWidth / 16);
  const scaleY = mapHeight / (RendererState.levelHeight / 16);
  
  const vpWidth = Math.max(4, Math.floor((RendererState.psxWidth / 16) * scaleX));
  const vpHeight = Math.max(4, Math.floor((RendererState.psxHeight / 16) * scaleY));
  const vpX = Math.floor((RendererState.cameraX / 16) * scaleX);
  const vpY = Math.floor((RendererState.cameraY / 16) * scaleY);
  
  return { x: vpX, y: vpY, width: vpWidth, height: vpHeight };
}

// ============================================================================
// Camera Controls
// ============================================================================

function setCamera(x, y) {
  const maxCameraX = Math.max(0, RendererState.levelWidth - RendererState.psxWidth);
  const maxCameraY = Math.max(0, RendererState.levelHeight - RendererState.psxHeight);
  
  RendererState.cameraX = Math.max(0, Math.min(maxCameraX, x));
  RendererState.cameraY = Math.max(0, Math.min(maxCameraY, y));
  
  return {
    x: RendererState.cameraX,
    y: RendererState.cameraY,
    maxX: maxCameraX,
    maxY: maxCameraY,
  };
}

function moveCamera(dx, dy) {
  return setCamera(RendererState.cameraX + dx, RendererState.cameraY + dy);
}

function getCameraLimits() {
  return {
    maxX: Math.max(0, RendererState.levelWidth - RendererState.psxWidth),
    maxY: Math.max(0, RendererState.levelHeight - RendererState.psxHeight),
  };
}

// ============================================================================
// Exports
// ============================================================================

const BLBRenderer = {
  // State (read-only access recommended)
  state: RendererState,
  
  // Initialization
  initRenderer,
  
  // Rendering
  renderStage,
  clearSpriteCache,
  clearTileCache,
  updateMinimap,
  getMinimapViewport,
  
  // Camera
  setCamera,
  moveCamera,
  getCameraLimits,
  
  // Constants
  MINIMAP_MAX_WIDTH,
  MINIMAP_MAX_HEIGHT,
  ANIMATION_TICK_RATE,
};

// Make available globally for non-module usage
if (typeof window !== 'undefined') {
  window.BLBRenderer = BLBRenderer;
}
