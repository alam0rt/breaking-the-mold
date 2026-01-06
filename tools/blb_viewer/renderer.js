/**
 * Skullmonkeys BLB Renderer
 * 
 * Handles canvas rendering, parallax, tile drawing, and minimap.
 * Depends on GameEngine for tile/layer access.
 */

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
  
  // PSX dimensions
  psxWidth: 320,
  psxHeight: 256, // PAL
  
  // Level dimensions (calculated from layers)
  levelWidth: 0,
  levelHeight: 0,
  
  // Display options
  showEntities: true,
  showGrid: false,
  showMinimap: false,
  
  // 90s mode options
  mode90s: false,
  staticBg: false,
  mouseFollow: false,
  
  // Cached stage data
  cachedStage: null,
};

// Minimap constants
const MINIMAP_MAX_WIDTH = 160;
const MINIMAP_MAX_HEIGHT = 80;

// ============================================================================
// Initialization
// ============================================================================

function initRenderer(canvas, minimapCanvas) {
  RendererState.canvas = canvas;
  RendererState.ctx = canvas.getContext('2d');
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
  
  // Create ImageData for pixel manipulation
  const imageData = ctx.createImageData(viewWidth, viewHeight);
  const data = imageData.data;
  
  // Pre-fill with background
  for (let i = 0; i < data.length; i += 4) {
    data[i] = bgR;
    data[i + 1] = bgG;
    data[i + 2] = bgB;
    data[i + 3] = 255;
  }
  
  // Entity markers list
  const entityMarkers = [];
  const totalTiles = GameEngine.getTotalTileCount(tileHeader);
  
  // Render layers back-to-front
  for (let layerIdx = 0; layerIdx < layers.length; layerIdx++) {
    const layer = layers[layerIdx];
    
    // Skip non-renderable layers
    if (!GameEngine.isLayerVisible(layer)) continue;
    
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
    
    // Render tiles
    for (let ty = 0; ty < layer.height; ty++) {
      for (let tx = 0; tx < layer.width; tx++) {
        const tileDataIdx = (ty * layer.width + tx) * 2;
        if (tileDataIdx + 2 > tilemapData.length) continue;
        
        const rawIdx = tilemapView.getUint16(tileDataIdx, true);
        const tileIdx = rawIdx & 0x7FF;
        
        if (tileIdx === 0) continue;
        
        // Calculate world position (including layer offset)
        const worldX = layerOffsetX + tx * 16;
        const worldY = layerOffsetY + ty * 16;
        
        // Calculate screen position
        let screenX, screenY;
        if (RendererState.mode90s) {
          if (RendererState.staticBg && scrollFactorX < 0.5) {
            // Static background - tile to fill viewport
            screenX = worldX % viewWidth;
            if (screenX < 0) screenX += viewWidth;
            screenY = worldY;
          } else {
            // Normal parallax
            screenX = worldX - Math.floor(RendererState.cameraX * scrollFactorX);
            screenY = worldY - Math.floor(RendererState.cameraY * scrollFactorY);
          }
        } else {
          // In full view mode, just show everything at world position
          screenX = worldX;
          screenY = worldY;
        }
        
        // Skip tiles completely outside viewport
        if (screenX + 16 <= 0 || screenX >= viewWidth || screenY + 16 <= 0 || screenY >= viewHeight) {
          // Still collect entity markers even if off-screen
          if (tileIdx > totalTiles) {
            entityMarkers.push({
              x: screenX,
              y: screenY,
              entityId: tileIdx,
              layer: layerIdx,
            });
          }
          continue;
        }
        
        // Check if entity marker
        if (tileIdx > totalTiles) {
          entityMarkers.push({
            x: screenX,
            y: screenY,
            entityId: tileIdx,
            layer: layerIdx,
          });
          continue;
        }
        
        // Get tile pixels
        const tile = GameEngine.getTile(
          tileIdx, tileHeader, pixelData, flagsData, paletteIndices, palettes
        );
        
        if (!tile || tile.type === 'entity') continue;
        
        const tileSize = tile.size;
        const scale = 16 / tileSize;
        
        // Render tile pixels
        for (let py = 0; py < tileSize; py++) {
          for (let px = 0; px < tileSize; px++) {
            const colorIdx = tile.pixels[py][px];
            if (colorIdx === 0) continue;
            
            const color = tile.palette[colorIdx] || [255, 0, 255, 255];
            const alpha = tile.isSemiTransparent ? 128 : color[3];
            
            for (let dy = 0; dy < scale; dy++) {
              for (let dx = 0; dx < scale; dx++) {
                const ix = screenX + px * scale + dx;
                const iy = screenY + py * scale + dy;
                
                if (ix < 0 || ix >= viewWidth || iy < 0 || iy >= viewHeight) continue;
                
                const pixelOffset = (iy * viewWidth + ix) * 4;
                
                if (alpha === 255) {
                  data[pixelOffset] = color[0];
                  data[pixelOffset + 1] = color[1];
                  data[pixelOffset + 2] = color[2];
                  data[pixelOffset + 3] = 255;
                } else if (alpha > 0) {
                  const a = alpha / 255;
                  data[pixelOffset] = Math.round(data[pixelOffset] * (1 - a) + color[0] * a);
                  data[pixelOffset + 1] = Math.round(data[pixelOffset + 1] * (1 - a) + color[1] * a);
                  data[pixelOffset + 2] = Math.round(data[pixelOffset + 2] * (1 - a) + color[2] * a);
                }
              }
            }
          }
        }
      }
    }
  }
  
  // Put image data to canvas
  ctx.putImageData(imageData, 0, 0);
  
  // Draw entity markers on top
  if (RendererState.showEntities && entityMarkers.length > 0) {
    ctx.strokeStyle = '#ff0';
    ctx.lineWidth = 1;
    ctx.fillStyle = 'rgba(255, 255, 0, 0.3)';
    ctx.font = '8px monospace';
    
    for (const marker of entityMarkers) {
      ctx.fillRect(marker.x, marker.y, 16, 16);
      ctx.strokeRect(marker.x + 0.5, marker.y + 0.5, 15, 15);
      ctx.fillStyle = '#ff0';
      ctx.fillText(marker.entityId.toString(16), marker.x + 2, marker.y + 12);
      ctx.fillStyle = 'rgba(255, 255, 0, 0.3)';
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
    totalTiles,
    layerCount: layers.length,
    paletteCount: palettes.length,
    entityCount: entityMarkers.length,
    cameraX: RendererState.cameraX,
    cameraY: RendererState.cameraY,
  };
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
        const tileIdx = rawIdx & 0x7FF;
        
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
        const tileIdx = rawIdx & 0x7FF;
        
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
  updateMinimap,
  getMinimapViewport,
  
  // Camera
  setCamera,
  moveCamera,
  getCameraLimits,
  
  // Constants
  MINIMAP_MAX_WIDTH,
  MINIMAP_MAX_HEIGHT,
};

// Make available globally for non-module usage
if (typeof window !== 'undefined') {
  window.BLBRenderer = BLBRenderer;
}
