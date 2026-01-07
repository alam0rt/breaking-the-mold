/**
 * Skullmonkeys BLB Viewer UI
 * 
 * Handles DOM interactions, event handlers, tree view, console logging, and state management.
 * Depends on BLB for file parsing, GameEngine for stage loading, and BLBRenderer for canvas rendering.
 */

// ============================================================================
// UI State
// ============================================================================

const UIState = {
  file: null,
  fileData: null,
  levels: [],
  selectedLevel: null,
  selectedStage: null,
  // View mode: 'stage' or 'sprite'
  viewMode: 'stage',
  // Currently selected sprite (for sprite preview mode)
  selectedSprite: null,
  selectedSpriteStageData: null,
};

// ============================================================================
// DOM References
// ============================================================================

let dom = {};

function initDOM() {
  dom = {
    fileInput: document.getElementById('file-input'),
    btnOpen: document.getElementById('btn-open'),
    btnConsole: document.getElementById('btn-console'),
    cameraX: document.getElementById('camera-x'),
    cameraXVal: document.getElementById('camera-x-val'),
    zoomLevel: document.getElementById('zoom-level'),
    // Display options
    showGrid: document.getElementById('show-grid'),
    showCollision: document.getElementById('show-collision'),
    showMinimap: document.getElementById('show-minimap'),
    optDebug: document.getElementById('opt-debug'),
    // 90s mode options
    mode90s: document.getElementById('mode-90s'),
    optStaticBg: document.getElementById('opt-static-bg'),
    optMouseFollow: document.getElementById('opt-mouse-follow'),
    // Panels
    treeContainer: document.getElementById('tree-container'),
    renderTitle: document.getElementById('render-title'),
    renderSize: document.getElementById('render-size'),
    renderContainer: document.getElementById('render-container'),
    canvas: document.getElementById('render-canvas'),
    infoPanel: document.getElementById('info-panel'),
    // Minimap
    minimapContainer: document.getElementById('minimap-container'),
    minimapCanvas: document.getElementById('minimap-canvas'),
    minimapViewport: document.getElementById('minimap-viewport'),
    // Layers panel
    layersContainer: document.getElementById('layers-container'),
    // Sprites panel
    optShowSprites: document.getElementById('opt-show-sprites'),
    optShowPlayer: document.getElementById('opt-show-player'),
    optAnimateSprites: document.getElementById('opt-animate-sprites'),
    optEntityDebug: document.getElementById('opt-entity-debug'),
    spriteCount: document.getElementById('sprite-count'),
    // Status
    statusMsg: document.getElementById('status-msg'),
    statusFile: document.getElementById('status-file'),
    statusSize: document.getElementById('status-size'),
    loading: document.getElementById('loading'),
    loadingProgress: document.getElementById('loading-progress'),
    consolePanel: document.getElementById('console-panel'),
    consoleContent: document.getElementById('console-content'),
    btnConsoleClear: document.getElementById('btn-console-clear'),
    btnConsoleClose: document.getElementById('btn-console-close'),
    // Entities panel
    entitiesPanel: document.getElementById('entities-panel'),
    entitiesList: document.getElementById('entities-list'),
    entitiesCount: document.getElementById('entities-count'),
    entitiesFilter: document.getElementById('entities-filter'),
    entitiesGroup: document.getElementById('entities-group'),
  };
}

// ============================================================================
// Console Logger
// ============================================================================

const consoleLogger = {
  entries: [],
  maxEntries: 500,
  
  formatTime() {
    const now = new Date();
    return `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}.${now.getMilliseconds().toString().padStart(3, '0')}`;
  },
  
  addEntry(type, args) {
    const msg = args.map(arg => {
      if (typeof arg === 'object') {
        try {
          return JSON.stringify(arg, null, 2);
        } catch {
          return String(arg);
        }
      }
      return String(arg);
    }).join(' ');
    
    this.entries.push({ type, time: this.formatTime(), msg });
    
    if (this.entries.length > this.maxEntries) {
      this.entries.shift();
    }
    
    this.render();
  },
  
  render() {
    if (!dom.consoleContent) return;
    dom.consoleContent.innerHTML = this.entries.map(e => 
      `<div class="console-entry ${e.type}"><span class="console-time">[${e.time}]</span><span class="console-msg">${this.escapeHtml(e.msg)}</span></div>`
    ).join('');
    dom.consoleContent.scrollTop = dom.consoleContent.scrollHeight;
  },
  
  escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  },
  
  clear() {
    this.entries = [];
    this.render();
  },
  
  toggle() {
    dom.consolePanel.classList.toggle('visible');
  },
  
  show() {
    dom.consolePanel.classList.add('visible');
  },
  
  hide() {
    dom.consolePanel.classList.remove('visible');
  }
};

// ============================================================================
// Console Interception
// ============================================================================

function setupConsoleInterception() {
  const originalConsole = {
    log: console.log.bind(console),
    info: console.info.bind(console),
    warn: console.warn.bind(console),
    error: console.error.bind(console),
  };

  console.log = (...args) => {
    originalConsole.log(...args);
    consoleLogger.addEntry('log', args);
  };

  console.info = (...args) => {
    originalConsole.info(...args);
    consoleLogger.addEntry('info', args);
  };

  console.warn = (...args) => {
    originalConsole.warn(...args);
    consoleLogger.addEntry('warn', args);
  };

  console.error = (...args) => {
    originalConsole.error(...args);
    consoleLogger.addEntry('error', args);
  };

  // Catch unhandled errors
  window.addEventListener('error', (e) => {
    consoleLogger.addEntry('error', [`Uncaught Error: ${e.message} at ${e.filename}:${e.lineno}:${e.colno}`]);
    consoleLogger.show();
  });

  window.addEventListener('unhandledrejection', (e) => {
    consoleLogger.addEntry('error', [`Unhandled Promise Rejection: ${e.reason}`]);
    consoleLogger.show();
  });
}

// ============================================================================
// Entities Panel
// ============================================================================

const entitiesPanel = {
  entities: [],        // Current stage entities
  filteredEntities: [], // After filter applied
  groupMode: 'type',   // 'type' or 'instance'
  filterText: '',
  selectedIndex: -1,   // Selected entity/type index in current view
  
  update(entities) {
    this.entities = entities || [];
    this.selectedIndex = -1;
    this.applyFilter();
  },
  
  applyFilter() {
    const filter = this.filterText.toLowerCase();
    
    if (!filter) {
      this.filteredEntities = this.entities;
    } else {
      this.filteredEntities = this.entities.filter(e => 
        e.typeName.toLowerCase().includes(filter) ||
        e.typeDesc.toLowerCase().includes(filter) ||
        e.type.toString().includes(filter)
      );
    }
    
    this.render();
  },
  
  render() {
    if (!dom.entitiesList) return;
    
    if (this.filteredEntities.length === 0) {
      const msg = this.entities.length === 0 ? 'No entities in stage' : 'No matches';
      dom.entitiesList.innerHTML = `<div class="entities-empty">${msg}</div>`;
      dom.entitiesCount.textContent = '0 entities';
      return;
    }
    
    if (this.groupMode === 'type') {
      this.renderByType();
    } else {
      this.renderAllInstances();
    }
  },
  
  renderByType() {
    // Group entities by type and count them
    const typeMap = new Map();
    for (const entity of this.filteredEntities) {
      const key = entity.type;
      if (!typeMap.has(key)) {
        typeMap.set(key, {
          type: entity.type,
          typeName: entity.typeName,
          typeDesc: entity.typeDesc,
          count: 0,
          entities: [],
        });
      }
      const group = typeMap.get(key);
      group.count++;
      group.entities.push(entity);
    }
    
    // Sort by type number
    const uniqueTypes = Array.from(typeMap.values()).sort((a, b) => a.type - b.type);
    
    // Build HTML - clicking snaps to first entity of that type
    const html = uniqueTypes.map((t, idx) => {
      const icon = this.getEntityIcon(t.type, t.typeName);
      const selected = idx === this.selectedIndex ? ' selected' : '';
      // Store first entity position for snap
      const first = t.entities[0];
      return `
        <div class="entity-row${selected}" data-index="${idx}" data-type="${t.type}" data-x="${first.xCenter}" data-y="${first.yCenter}" title="${t.typeDesc}\nType ID: ${t.type}\nClick to snap to first instance">
          <div class="entity-icon">${icon}</div>
          <div class="entity-info">
            <div class="entity-name">${t.typeName}</div>
            <div class="entity-desc">${t.typeDesc}</div>
          </div>
          <div class="entity-count">×${t.count}</div>
        </div>
      `;
    }).join('');
    
    dom.entitiesList.innerHTML = html;
    dom.entitiesCount.textContent = `${this.filteredEntities.length} (${uniqueTypes.length} types)`;
    
    // Add click handlers
    this.attachClickHandlers();
  },
  
  renderAllInstances() {
    // Sort by position (left to right, top to bottom)
    const sorted = [...this.filteredEntities].sort((a, b) => {
      if (a.yCenter !== b.yCenter) return a.yCenter - b.yCenter;
      return a.xCenter - b.xCenter;
    });
    
    const html = sorted.map((e, idx) => {
      const icon = this.getEntityIcon(e.type, e.typeName);
      const selected = idx === this.selectedIndex ? ' selected' : '';
      return `
        <div class="entity-row${selected}" data-index="${idx}" data-entity-idx="${e.index}" data-x="${e.xCenter}" data-y="${e.yCenter}" title="${e.typeDesc}\nType ID: ${e.type}\nPos: ${e.xCenter}, ${e.yCenter}\nClick to snap">
          <div class="entity-icon">${icon}</div>
          <div class="entity-info">
            <div class="entity-name">${e.typeName}</div>
            <div class="entity-desc">${e.xCenter}, ${e.yCenter}</div>
          </div>
          <div class="entity-count">#${e.index}</div>
        </div>
      `;
    }).join('');
    
    dom.entitiesList.innerHTML = html;
    dom.entitiesCount.textContent = `${sorted.length} instances`;
    
    // Add click handlers
    this.attachClickHandlers();
  },
  
  attachClickHandlers() {
    const rows = dom.entitiesList.querySelectorAll('.entity-row');
    rows.forEach(row => {
      row.addEventListener('click', () => {
        const x = parseInt(row.dataset.x, 10);
        const y = parseInt(row.dataset.y, 10);
        const idx = parseInt(row.dataset.index, 10);
        
        // Update selection
        this.selectedIndex = idx;
        rows.forEach(r => r.classList.remove('selected'));
        row.classList.add('selected');
        
        // Snap camera to entity
        this.snapToPosition(x, y);
      });
    });
  },
  
  snapToPosition(x, y) {
    // Calculate camera position to center the entity in view
    const viewWidth = BLBRenderer.state.mode90s ? BLBRenderer.state.psxWidth : BLBRenderer.state.levelWidth;
    const viewHeight = BLBRenderer.state.mode90s ? BLBRenderer.state.psxHeight : BLBRenderer.state.levelHeight;
    
    // In 90s mode, we control the camera. Otherwise scroll to position
    if (BLBRenderer.state.mode90s) {
      // Center entity in viewport
      const halfWidth = Math.floor(viewWidth / 2);
      const halfHeight = Math.floor(viewHeight / 2);
      
      let newX = x - halfWidth;
      let newY = y - halfHeight;
      
      // Clamp to valid range
      const limits = BLBRenderer.getCameraLimits();
      newX = Math.max(0, Math.min(newX, limits.maxX));
      newY = Math.max(0, Math.min(newY, limits.maxY));
      
      BLBRenderer.setCamera(newX, newY);
      dom.cameraX.value = newX;
      dom.cameraXVal.textContent = newX;
      rerender();
      
      console.log(`📍 Snapped to (${x}, ${y}) - Camera: (${newX}, ${newY})`);
    } else {
      // In full view mode, scroll the container to the entity position
      const zoom = parseInt(dom.zoomLevel.value, 10);
      const scrollX = (x * zoom) - (dom.renderContainer.clientWidth / 2);
      const scrollY = (y * zoom) - (dom.renderContainer.clientHeight / 2);
      
      dom.renderContainer.scrollTo({
        left: Math.max(0, scrollX),
        top: Math.max(0, scrollY),
        behavior: 'smooth'
      });
      
      console.log(`📍 Scrolled to (${x}, ${y})`);
    }
  },
  
  getEntityIcon(type, name) {
    // Return emoji icons based on entity type name
    const lowerName = name.toLowerCase();
    if (lowerName.includes('klaymen') || lowerName.includes('player')) return '🧍';
    if (lowerName.includes('swirl') || lowerName.includes('collect')) return '🌀';
    if (lowerName.includes('bird') || lowerName.includes('yellow')) return '🐦';
    if (lowerName.includes('checkpoint')) return '🚩';
    if (lowerName.includes('spike') || lowerName.includes('hazard')) return '⚠️';
    if (lowerName.includes('enemy') || lowerName.includes('monster')) return '👾';
    if (lowerName.includes('platform') || lowerName.includes('moving')) return '⬛';
    if (lowerName.includes('portal') || lowerName.includes('exit')) return '🚪';
    if (lowerName.includes('ring') || lowerName.includes('hoop')) return '⭕';
    if (lowerName.includes('button') || lowerName.includes('switch')) return '🔘';
    if (lowerName.includes('door')) return '🚪';
    if (lowerName.includes('key')) return '🔑';
    if (lowerName.includes('spring') || lowerName.includes('bounce')) return '🔄';
    if (lowerName.includes('water')) return '💧';
    if (lowerName.includes('fire') || lowerName.includes('flame')) return '🔥';
    // Default to type number as fallback
    return type.toString(16).toUpperCase().padStart(2, '0');
  },
  
  clear() {
    this.entities = [];
    this.filteredEntities = [];
    this.selectedIndex = -1;
    if (!dom.entitiesList) return;
    dom.entitiesList.innerHTML = '<div class="entities-empty">No stage loaded</div>';
    dom.entitiesCount.textContent = '0 entities';
  }
};

// Log current options state (when debug is enabled)
function logOptions(trigger) {
  if (!dom.optDebug || !dom.optDebug.checked) return;
  
  const opts = {
    showGrid: dom.showGrid.checked,
    showCollision: dom.showCollision.checked,
    showMinimap: dom.showMinimap.checked,
    mode90s: dom.mode90s.checked,
    staticBg: dom.optStaticBg.checked,
    mouseFollow: dom.optMouseFollow.checked,
    debug: dom.optDebug.checked,
  };
  
  console.log(`[OPT] ${trigger}:`, opts);
}

// ============================================================================
// Status Helpers
// ============================================================================

function setStatus(msg) {
  dom.statusMsg.textContent = msg;
}

function showLoading(show, msg = 'Loading...') {
  dom.loading.classList.toggle('visible', show);
  dom.loadingProgress.textContent = msg;
}

// ============================================================================
// Layer Panel
// ============================================================================

/**
 * Update the layers panel with checkboxes for each layer
 */
function updateLayersPanel(stageData) {
  if (!dom.layersContainer) return;
  
  // Clear existing
  dom.layersContainer.innerHTML = '';
  
  if (!stageData || !stageData.layers || stageData.layers.length === 0) {
    dom.layersContainer.innerHTML = '<span style="color: #808080;">No layers</span>';
    return;
  }
  
  const { layers } = stageData;
  
  // Initialize visibility array (all visible by default)
  BLBRenderer.state.layerVisibility = layers.map(() => true);
  
  // Create checkbox for each layer
  for (let i = 0; i < layers.length; i++) {
    const layer = layers[i];
    const isVisible = GameEngine.isLayerVisible(layer);
    
    const label = document.createElement('label');
    label.style.display = 'flex';
    label.style.alignItems = 'center';
    label.style.gap = '4px';
    label.style.marginRight = '8px';
    
    const checkbox = document.createElement('input');
    checkbox.type = 'checkbox';
    checkbox.checked = isVisible;
    checkbox.disabled = !isVisible; // Can't toggle layers that are hidden by game logic
    checkbox.dataset.layerIndex = i;
    
    // Build label text with layer info
    const scrollX = layer.scrollX.toFixed(2);
    const scrollInfo = layer.scrollX < 0.5 ? 'bg' : 'fg';
    const labelText = `L${i} (${layer.width}×${layer.height}, ${scrollInfo})`;
    
    checkbox.addEventListener('change', (e) => {
      const idx = parseInt(e.target.dataset.layerIndex, 10);
      BLBRenderer.state.layerVisibility[idx] = e.target.checked;
      
      // Log layer info to console
      const l = layers[idx];
      const scrollDesc = l.scrollX < 0.5 ? 'background' : 'foreground';
      console.log(`[LAYER] Layer ${idx}: ${e.target.checked ? 'VISIBLE' : 'HIDDEN'}`);
      console.log(`  Size: ${l.width}×${l.height} tiles (${l.width * 16}×${l.height * 16} px)`);
      console.log(`  Offset: (${l.xOffset}, ${l.yOffset})`);
      console.log(`  Scroll: X=${l.scrollX.toFixed(3)}, Y=${l.scrollY.toFixed(3)} (${scrollDesc})`);
      console.log(`  Type: ${l.layerType}, RenderParam: 0x${l.renderParam.toString(16).toUpperCase()}`);
      
      rerender();
    });
    
    label.appendChild(checkbox);
    label.appendChild(document.createTextNode(labelText));
    dom.layersContainer.appendChild(label);
  }
}

// ============================================================================
// Render Helpers
// ============================================================================

// Animation state (timing constant from BLBRenderer.ANIMATION_TICK_RATE)
let animationFrameId = null;
let animationTickCount = 0;

function rerender() {
  // Check view mode - render sprite preview or stage
  if (UIState.viewMode === 'sprite' && UIState.selectedSprite && UIState.selectedSpriteStageData) {
    renderSingleSprite(UIState.selectedSpriteStageData, UIState.selectedSprite);
    return;
  }
  
  // Stage view mode
  if (!BLBRenderer.state.cachedStage) return;
  
  const info = BLBRenderer.renderStage(BLBRenderer.state.cachedStage, {
    cameraX: BLBRenderer.state.cameraX,
    cameraY: BLBRenderer.state.cameraY,
    zoom: parseInt(dom.zoomLevel.value, 10),
    showGrid: dom.showGrid.checked,
    showCollision: dom.showCollision.checked,
    showMinimap: dom.showMinimap.checked,
    mode90s: dom.mode90s.checked,
    staticBg: dom.optStaticBg.checked,
    mouseFollow: dom.optMouseFollow.checked,
  });
  
  if (info) {
    updateRenderInfo(info);
    updateMinimapViewportIndicator();
  }
}

/**
 * Start sprite animation loop
 * Runs at ~60fps, advances sprite frame every N ticks (TWOS timing)
 */
function startSpriteAnimation() {
  if (animationFrameId !== null) return; // Already running
  
  console.log('[SPRITES] Starting animation loop');
  animationTickCount = 0;
  const tickRate = BLBRenderer.ANIMATION_TICK_RATE;
  
  function animationLoop() {
    animationTickCount++;
    
    // Advance sprite frame index every tickRate ticks
    if (animationTickCount >= tickRate) {
      animationTickCount = 0;
      BLBRenderer.state.spriteFrameIndex = (BLBRenderer.state.spriteFrameIndex || 0) + 1;
      rerender();
    }
    
    if (BLBRenderer.state.animateSprites) {
      animationFrameId = requestAnimationFrame(animationLoop);
    }
  }
  
  animationFrameId = requestAnimationFrame(animationLoop);
}

/**
 * Stop sprite animation loop
 */
function stopSpriteAnimation() {
  if (animationFrameId !== null) {
    cancelAnimationFrame(animationFrameId);
    animationFrameId = null;
    console.log('[SPRITES] Stopped animation loop');
  }
  // Reset to frame 0
  BLBRenderer.state.spriteFrameIndex = 0;
  rerender();
}

function updateRenderInfo(info) {
  const sizeInfo = BLBRenderer.state.mode90s 
    ? `${info.viewWidth}×${info.viewHeight} (PSX) / Level: ${info.levelWidth}×${info.levelHeight}`
    : `${info.viewWidth}×${info.viewHeight}`;
  dom.renderSize.textContent = sizeInfo;
  
  dom.infoPanel.innerHTML = `
    Layers: ${info.layerCount}<br>
    Tiles: ${info.totalTiles}<br>
    Palettes: ${info.paletteCount}<br>
    Entities: ${info.entityCount}${info.spriteCount !== undefined ? '<br>Sprites: ' + info.spritesRendered + '/' + info.spriteCount : ''}${BLBRenderer.state.mode90s ? '<br>Camera: ' + info.cameraX + ',' + info.cameraY : ''}
  `;
  dom.infoPanel.classList.add('visible');
  
  // Update sprite count display
  if (dom.spriteCount && info.spriteCount !== undefined) {
    dom.spriteCount.textContent = `(${info.spritesRendered}/${info.spriteCount} rendered)`;
  }
  
  // Update camera slider range
  const limits = BLBRenderer.getCameraLimits();
  dom.cameraX.max = limits.maxX;
}

function updateMinimapViewportIndicator() {
  const vp = BLBRenderer.getMinimapViewport();
  if (vp) {
    dom.minimapViewport.style.left = `${vp.x}px`;
    dom.minimapViewport.style.top = `${vp.y}px`;
    dom.minimapViewport.style.width = `${vp.width}px`;
    dom.minimapViewport.style.height = `${vp.height}px`;
  }
}

/**
 * Render a single sprite centered in the canvas for preview
 * Uses current animation frame index when animating
 */
function renderSingleSprite(stageData, sprite) {
  // Get the correct container data based on whether sprite is shared or stage-specific
  const containerData = sprite.isShared 
    ? stageData.sharedSpriteContainerData 
    : stageData.spriteContainerData;
  
  if (!containerData) {
    console.warn('[SPRITE] No container data for sprite');
    return;
  }
  
  // Get sprite info to determine frame count
  const header = GameEngine.parseSpriteHeader(containerData, sprite.offset);
  if (!header) return;
  
  const animations = GameEngine.parseSpriteAnimations(containerData, sprite.offset, header.animationCount);
  const anim = animations[0];
  const frameCount = anim ? anim.frameCount : 1;
  
  // Use animated frame index (loops within sprite's frame count)
  const frameIdx = (BLBRenderer.state.spriteFrameIndex || 0) % frameCount;
  
  const frameData = GameEngine.getSpriteFrame(containerData, sprite, 0, frameIdx);
  if (!frameData) return;
  
  const { pixels, palette, width, height } = frameData;
  
  // Get canvas and context
  const canvas = dom.canvas;
  const ctx = canvas.getContext('2d');
  
  // Set canvas size to fit sprite with padding
  const padding = 20;
  const displayWidth = Math.max(width + padding * 2, 100);
  const displayHeight = Math.max(height + padding * 2, 100);
  
  canvas.width = displayWidth;
  canvas.height = displayHeight;
  canvas.style.width = `${displayWidth * 2}px`;
  canvas.style.height = `${displayHeight * 2}px`;
  
  // Clear with checkerboard pattern for transparency
  ctx.fillStyle = '#404040';
  ctx.fillRect(0, 0, displayWidth, displayHeight);
  ctx.fillStyle = '#505050';
  for (let y = 0; y < displayHeight; y += 8) {
    for (let x = 0; x < displayWidth; x += 8) {
      if ((x + y) % 16 === 0) {
        ctx.fillRect(x, y, 8, 8);
      }
    }
  }
  
  // Create ImageData for the sprite
  const imageData = ctx.createImageData(width, height);
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
  
  // Draw sprite centered
  const offsetX = Math.floor((displayWidth - width) / 2);
  const offsetY = Math.floor((displayHeight - height) / 2);
  
  // Use a temporary canvas to draw with transparency
  const tempCanvas = document.createElement('canvas');
  tempCanvas.width = width;
  tempCanvas.height = height;
  const tempCtx = tempCanvas.getContext('2d');
  tempCtx.putImageData(imageData, 0, 0);
  
  ctx.drawImage(tempCanvas, offsetX, offsetY);
  
  const sourceLabel = sprite.isShared ? 'shared' : 'stage';
  console.log(`[SPRITE] Rendered ${sourceLabel} sprite: ${width}×${height} at (${offsetX}, ${offsetY})`);
}

// ============================================================================
// Tree View
// ============================================================================

// Cache for loaded stage data (to avoid reloading when expanding tree)
const stageDataCache = new Map();

function getStageKey(level, stageIndex) {
  return `${level.levelId}:${stageIndex}`;
}

function getCachedStageData(level, stageIndex) {
  const key = getStageKey(level, stageIndex);
  if (stageDataCache.has(key)) {
    return stageDataCache.get(key);
  }
  const stageData = GameEngine.loadStage(UIState.fileData, level, stageIndex);
  if (stageData) {
    stageDataCache.set(key, stageData);
  }
  return stageData;
}

function buildTreeView() {
  dom.treeContainer.innerHTML = '';
  stageDataCache.clear(); // Clear cache when building new tree
  
  if (UIState.levels.length === 0) {
    dom.treeContainer.innerHTML = '<div style="color: #808080; padding: 20px; text-align: center;">No levels found</div>';
    return;
  }
  
  const rootNode = createTreeNode({
    label: UIState.file.name,
    icon: '📁',
    expanded: true,
    children: UIState.levels.map(level => ({
      label: `${level.levelId} - ${level.levelName}`,
      icon: '📂',
      data: { type: 'level', level },
      expanded: false,
      children: Array.from({ length: level.stageCount }, (_, i) => ({
        label: `Stage ${i}`,
        icon: '🎮',
        data: { type: 'stage', level, stageIndex: i },
        expanded: false,
        // Lazy load children - will be populated on expand
        lazyChildren: () => getLayerChildren(level, i),
      })),
    })),
  });
  
  dom.treeContainer.appendChild(rootNode);
}

/**
 * Get layer and sprite children for a stage (called lazily when stage is expanded)
 */
function getLayerChildren(level, stageIndex) {
  const stageData = getCachedStageData(level, stageIndex);
  if (!stageData || !stageData.layers) {
    return [{ label: '(no data)', icon: '⚠️' }];
  }
  
  const children = [];
  
  // Add layers
  for (let i = 0; i < stageData.layers.length; i++) {
    const layer = stageData.layers[i];
    const scrollDesc = layer.scrollX < 0.5 ? 'bg' : 'fg';
    const visible = GameEngine.isLayerVisible(layer);
    const visIcon = visible ? '' : ' 👁️‍🗨️';
    
    children.push({
      label: `Layer ${i} (${layer.width}×${layer.height}, ${scrollDesc})${visIcon}`,
      icon: '📄',
      data: { type: 'layer', level, stageIndex, layerIndex: i, layer, stageData },
    });
  }
  
  // Add sprites if available
  if (stageData.sprites && stageData.sprites.count > 0) {
    const spriteList = stageData.sprites.sprites || [];
    const stageCount = stageData.sprites.stageCount || 0;
    const sharedCount = stageData.sprites.sharedCount || 0;
    
    // Create sprite entries with shared indicator
    const spriteChildren = spriteList.slice(0, 100).map((sprite, i) => {
      const isShared = sprite.isShared === true;
      const sharedTag = isShared ? ' [geometry]' : '';
      const icon = isShared ? '🏔️' : '👾';  // Mountain for background geometry, alien for entity sprites
      return {
        label: `Sprite ${sprite.index} (ID: 0x${sprite.id.toString(16).toUpperCase()}, ${sprite.size} B)${sharedTag}`,
        icon: icon,
        data: { type: 'sprite', level, stageIndex, spriteIndex: i, sprite, stageData },
      };
    });
    
    // Show count breakdown in parent label
    const countLabel = sharedCount > 0 
      ? `Sprites (${stageCount} entity + ${sharedCount} geometry)`
      : `Sprites (${stageData.sprites.count})`;
    
    children.push({
      label: countLabel,
      icon: '🎭',
      expanded: false,
      children: spriteChildren,
    });
  }
  
  return children;
}

function createTreeNode({ label, icon, data, expanded = false, children = [], lazyChildren = null }) {
  const node = document.createElement('div');
  node.className = 'tree-node';
  
  const content = document.createElement('div');
  content.className = 'tree-node-content';
  
  // Determine if this node has or can have children
  const hasChildren = children.length > 0 || lazyChildren !== null;
  
  // Toggle button
  const toggle = document.createElement('span');
  toggle.className = 'tree-toggle';
  toggle.textContent = hasChildren ? (expanded ? '−' : '+') : ' ';
  content.appendChild(toggle);
  
  // Icon
  const iconSpan = document.createElement('span');
  iconSpan.className = 'tree-icon';
  iconSpan.textContent = icon;
  content.appendChild(iconSpan);
  
  // Label
  const labelSpan = document.createElement('span');
  labelSpan.className = 'tree-label';
  labelSpan.textContent = label;
  content.appendChild(labelSpan);
  
  node.appendChild(content);
  
  // Children container (create even for lazy nodes)
  let childContainer = null;
  let lazyLoaded = false;
  
  if (hasChildren) {
    childContainer = document.createElement('div');
    childContainer.className = `tree-children${expanded ? '' : ' collapsed'}`;
    
    // Add existing children
    for (const child of children) {
      childContainer.appendChild(createTreeNode(child));
    }
    
    node.appendChild(childContainer);
    
    // Toggle click - handles both regular and lazy children
    toggle.addEventListener('click', (e) => {
      e.stopPropagation();
      const isCollapsed = childContainer.classList.contains('collapsed');
      
      // Lazy load children on first expand
      if (isCollapsed && lazyChildren && !lazyLoaded) {
        lazyLoaded = true;
        const lazyNodes = lazyChildren();
        for (const child of lazyNodes) {
          childContainer.appendChild(createTreeNode(child));
        }
      }
      
      childContainer.classList.toggle('collapsed');
      toggle.textContent = isCollapsed ? '−' : '+';
    });
  }
  
  // Content click
  content.addEventListener('click', () => {
    // Deselect all
    document.querySelectorAll('.tree-node-content.selected').forEach(el => {
      el.classList.remove('selected');
    });
    content.classList.add('selected');
    
    if (data) {
      handleTreeSelection(data);
    }
  });
  
  return node;
}

function handleTreeSelection(data) {
  if (data.type === 'level') {
    setStatus(`Selected level: ${data.level.levelId}`);
    dom.renderTitle.textContent = `${data.level.levelId} - ${data.level.levelName}`;
    UIState.selectedLevel = data.level;
    UIState.selectedStage = null;
    
    // Clear layers panel when level is selected
    updateLayersPanel(null);
    
    // Log level info
    const level = data.level;
    const primaryFileOff = level.sectorOffset * BLB.SECTOR_SIZE;
    console.info(`📂 Level: ${level.levelId} - ${level.levelName}`);
    console.log(`Primary segment: sector ${level.sectorOffset} (file offset: 0x${primaryFileOff.toString(16).toUpperCase()})`);
    console.log(`Stages: ${level.stageCount}`);
    console.log('Secondary sectors: ' + level.stages.slice(0, level.stageCount).map((s, i) => `[${i}]=${s.secSectorOff}`).join(', '));
    console.log('Tertiary sectors: ' + level.stages.slice(0, level.stageCount).map((s, i) => `[${i}]=${s.tertSectorOff}`).join(', '));
    
  } else if (data.type === 'stage') {
    // Reset view mode to stage
    UIState.viewMode = 'stage';
    UIState.selectedSprite = null;
    UIState.selectedSpriteStageData = null;
    
    setStatus(`Loading stage ${data.stageIndex} of ${data.level.levelId}...`);
    dom.renderTitle.textContent = `${data.level.levelId} - Stage ${data.stageIndex}`;
    UIState.selectedLevel = data.level;
    UIState.selectedStage = data.stageIndex;
    
    // Reset camera position for new stage
    BLBRenderer.setCamera(0, 0);
    dom.cameraX.value = 0;
    dom.cameraXVal.textContent = '0';
    
    // Load and render stage
    const stageData = GameEngine.loadStage(UIState.fileData, data.level, data.stageIndex);
    if (stageData) {
      // Update layer panel with new stage layers
      updateLayersPanel(stageData);
      
      // Update entities panel with stage entities
      entitiesPanel.update(stageData.entities);
      
      const info = BLBRenderer.renderStage(stageData, {
        zoom: parseInt(dom.zoomLevel.value, 10),
        showGrid: dom.showGrid.checked,
        showCollision: dom.showCollision.checked,
        showMinimap: dom.showMinimap.checked,
        mode90s: dom.mode90s.checked,
        staticBg: dom.optStaticBg.checked,
        mouseFollow: dom.optMouseFollow.checked,
      });
      
      if (info) {
        updateRenderInfo(info);
        updateMinimapViewportIndicator();
      }
      
      // Log layer summary
      console.info(`🎮 Stage ${data.stageIndex}: ${stageData.layers.length} layers`);
      for (let i = 0; i < stageData.layers.length; i++) {
        const l = stageData.layers[i];
        const scrollDesc = l.scrollX < 0.5 ? 'background' : 'foreground';
        const visible = GameEngine.isLayerVisible(l);
        console.log(`  [${i}] ${l.width}×${l.height} scroll=${l.scrollX.toFixed(2)} (${scrollDesc}) ${visible ? '' : '[hidden]'}`);
      }
      
      setStatus(`Rendered stage ${data.stageIndex}`);
    } else {
      updateLayersPanel(null);
      entitiesPanel.clear();
      setStatus(`Failed to load stage ${data.stageIndex}`);
    }
    
  } else if (data.type === 'layer') {
    // Layer selected - show layer details in info panel
    const { level, stageIndex, layerIndex, layer, stageData } = data;
    
    // Reset view mode to stage
    UIState.viewMode = 'stage';
    UIState.selectedSprite = null;
    UIState.selectedSpriteStageData = null;
    
    setStatus(`Layer ${layerIndex} of ${level.levelId} Stage ${stageIndex}`);
    dom.renderTitle.textContent = `${level.levelId} - Stage ${stageIndex} - Layer ${layerIndex}`;
    
    // If this stage isn't rendered yet, render it
    if (UIState.selectedLevel !== level || UIState.selectedStage !== stageIndex) {
      UIState.selectedLevel = level;
      UIState.selectedStage = stageIndex;
      
      BLBRenderer.setCamera(0, 0);
      dom.cameraX.value = 0;
      dom.cameraXVal.textContent = '0';
      
      updateLayersPanel(stageData);
      
      BLBRenderer.renderStage(stageData, {
        zoom: parseInt(dom.zoomLevel.value, 10),
        showGrid: dom.showGrid.checked,
        showCollision: dom.showCollision.checked,
        showMinimap: dom.showMinimap.checked,
        mode90s: dom.mode90s.checked,
        staticBg: dom.optStaticBg.checked,
        mouseFollow: dom.optMouseFollow.checked,
      });
    }
    
    // Display layer properties in info panel
    const scrollDesc = layer.scrollX < 0.5 ? 'background' : 'foreground';
    const visible = GameEngine.isLayerVisible(layer);
    
    dom.infoPanel.innerHTML = `
      <strong>Layer ${layerIndex}</strong><br>
      <hr style="margin: 4px 0; border: none; border-top: 1px solid #808080;">
      <strong>Dimensions:</strong><br>
      &nbsp; Size: ${layer.width}×${layer.height} tiles<br>
      &nbsp; Pixels: ${layer.width * 16}×${layer.height * 16} px<br>
      &nbsp; Level: ${layer.levelWidth}×${layer.levelHeight}<br>
      <strong>Position:</strong><br>
      &nbsp; Offset: (${layer.xOffset}, ${layer.yOffset})<br>
      <strong>Scrolling:</strong><br>
      &nbsp; X: ${layer.scrollX.toFixed(4)} (${scrollDesc})<br>
      &nbsp; Y: ${layer.scrollY.toFixed(4)}<br>
      <strong>Rendering:</strong><br>
      &nbsp; Type: ${layer.layerType}<br>
      &nbsp; Param: 0x${layer.renderParam.toString(16).toUpperCase()}<br>
      &nbsp; Visible: ${visible ? 'Yes' : 'No (skipRender)'}<br>
      <strong>Background:</strong><br>
      &nbsp; RGB(${layer.bgColor[0]}, ${layer.bgColor[1]}, ${layer.bgColor[2]})
    `;
    dom.infoPanel.classList.add('visible');
    
    // Log to console
    console.info(`📄 Layer ${layerIndex} of ${level.levelId} Stage ${stageIndex}`);
    console.log(`  Size: ${layer.width}×${layer.height} tiles (${layer.width * 16}×${layer.height * 16} px)`);
    console.log(`  Offset: (${layer.xOffset}, ${layer.yOffset})`);
    console.log(`  Scroll: X=${layer.scrollX.toFixed(4)}, Y=${layer.scrollY.toFixed(4)} (${scrollDesc})`);
    console.log(`  Type: ${layer.layerType}, RenderParam: 0x${layer.renderParam.toString(16).toUpperCase()}`);
    console.log(`  Visible: ${visible}, BgColor: RGB(${layer.bgColor.join(', ')})`);
    
  } else if (data.type === 'sprite') {
    // Sprite selected - show sprite preview and details
    const { level, stageIndex, spriteIndex, sprite, stageData } = data;
    
    // Set view mode to sprite preview
    UIState.viewMode = 'sprite';
    UIState.selectedSprite = sprite;
    UIState.selectedSpriteStageData = stageData;
    
    const sharedTag = sprite.isShared ? ' [geometry]' : '';
    setStatus(`Sprite ${spriteIndex}${sharedTag} of ${level.levelId} Stage ${stageIndex}`);
    dom.renderTitle.textContent = `${level.levelId} - Stage ${stageIndex} - Sprite ${spriteIndex}${sharedTag}`;
    
    // Get the correct container data (stage or shared)
    const containerData = sprite.isShared 
      ? stageData.sharedSpriteContainerData 
      : stageData.spriteContainerData;
    
    // Get sprite header for animation info
    const header = containerData 
      ? GameEngine.parseSpriteHeader(containerData, sprite.offset) 
      : null;
    const animations = header 
      ? GameEngine.parseSpriteAnimations(containerData, sprite.offset, header.animationCount) 
      : [];
    
    // Try to decode the sprite
    let frameInfo = '';
    if (containerData) {
      const frameData = GameEngine.getSpriteFrame(containerData, sprite, 0, 0);
      if (frameData) {
        const anim = animations[0];
        const frameCount = anim ? anim.frameCount : 1;
        frameInfo = `
      <strong>Frame 0:</strong><br>
      &nbsp; Size: ${frameData.width}×${frameData.height} px<br>
      &nbsp; Render: (${frameData.meta.renderX}, ${frameData.meta.renderY})<br>
      &nbsp; Anchor: (${frameData.meta.anchorX}, ${frameData.meta.anchorY})<br>
      <strong>Animations:</strong> ${header ? header.animationCount : 0}<br>
      <strong>Frames:</strong> ${frameCount}<br>
        `;
        
        // Render just this sprite in the canvas
        renderSingleSprite(stageData, sprite);
      } else {
        frameInfo = '<span style="color: #ff8080;">Failed to decode sprite</span><br>';
      }
    }
    
    // Show source indicator
    const sourceInfo = sprite.isShared 
      ? '<span style="color: #8080ff;">🏔️ Level Geometry (Primary segment)</span>'
      : '<span style="color: #80ff80;">👾 Entity Sprite (Tertiary segment)</span>';
    
    dom.infoPanel.innerHTML = `
      <strong>👾 Sprite ${sprite.index}</strong><br>
      ${sourceInfo}<br>
      <hr style="margin: 4px 0; border: none; border-top: 1px solid #808080;">
      <strong>Container:</strong><br>
      &nbsp; ID: 0x${sprite.id.toString(16).toUpperCase()}<br>
      &nbsp; Size: ${sprite.size} bytes<br>
      &nbsp; Offset: 0x${sprite.offset.toString(16).toUpperCase()}<br>
      ${frameInfo}
    `;
    dom.infoPanel.classList.add('visible');
    
    // Log to console
    console.info(`👾 Sprite ${sprite.index}${sharedTag} of ${level.levelId} Stage ${stageIndex}`);
    console.log(`  ID: 0x${sprite.id.toString(16)}, Size: ${sprite.size} bytes, Offset: 0x${sprite.offset.toString(16)}`);
    console.log(`  Source: ${sprite.isShared ? 'Primary (level geometry)' : 'Tertiary (entity sprite)'}`);
  }
}

// ============================================================================
// File Loading
// ============================================================================

async function loadBLBFile(file) {
  showLoading(true, `Loading ${file.name}...`);
  
  try {
    console.info(`📂 Reading file: ${file.name}`);
    const startTime = performance.now();
    
    const buffer = await file.arrayBuffer();
    UIState.file = file;
    UIState.fileData = buffer;
    
    const readTime = (performance.now() - startTime).toFixed(1);
    console.log(`File loaded: ${BLB.formatSize(buffer.byteLength)} in ${readTime}ms`);
    
    // Parse header
    console.info('Parsing BLB header...');
    const { levels } = BLB.parseHeader(buffer);
    UIState.levels = levels;
    
    // Log level summary
    console.log(`Found ${levels.length} levels:`);
    for (const level of levels) {
      console.log(`  [${level.index}] ${level.levelId} - "${level.levelName}" (${level.stageCount} stages)`);
    }
    
    // Update UI
    dom.statusFile.textContent = file.name;
    dom.statusSize.textContent = BLB.formatSize(buffer.byteLength);
    
    // Enable controls
    dom.cameraX.disabled = false;
    dom.zoomLevel.disabled = false;
    dom.showGrid.disabled = false;
    dom.showCollision.disabled = false;
    dom.showMinimap.disabled = false;
    dom.mode90s.disabled = false;
    
    // Build tree
    buildTreeView();
    
    setStatus(`Loaded ${levels.length} levels from ${file.name}`);
    
    // Auto-select first playable stage if available
    const scieLevel = levels.find(l => l.levelId === 'SCIE');
    if (scieLevel && scieLevel.stageCount > 1) {
      handleTreeSelection({ type: 'stage', level: scieLevel, stageIndex: 1 });
    }
  } catch (error) {
    console.error('Failed to load BLB:', error);
    setStatus(`Error: ${error.message}`);
  } finally {
    showLoading(false);
  }
}

// ============================================================================
// Event Handlers Setup
// ============================================================================

function setupEventHandlers() {
  // File open button
  dom.btnOpen.addEventListener('click', () => {
    dom.fileInput.click();
  });

  dom.fileInput.addEventListener('change', (e) => {
    const file = e.target.files[0];
    if (file) {
      loadBLBFile(file);
    }
  });

  // Camera slider
  dom.cameraX.addEventListener('input', (e) => {
    const camera = BLBRenderer.setCamera(parseInt(e.target.value, 10), BLBRenderer.state.cameraY);
    dom.cameraXVal.textContent = camera.x;
    rerender();
  });

  // Zoom
  dom.zoomLevel.addEventListener('change', () => {
    rerender();
  });

  // Display options
  dom.showGrid.addEventListener('change', () => {
    logOptions('showGrid');
    rerender();
  });
  dom.showCollision.addEventListener('change', () => {
    logOptions('showCollision');
    rerender();
  });
  
  dom.showMinimap.addEventListener('change', (e) => {
    logOptions('showMinimap');
    dom.minimapContainer.classList.toggle('visible', e.target.checked);
    rerender();
  });

  // 90s Mode group toggle
  dom.mode90s.addEventListener('change', (e) => {
    const enabled = e.target.checked;
    
    // Toggle render container class for centering
    dom.renderContainer.classList.toggle('mode-90s', enabled);
    
    // Enable/disable sub-options
    dom.optStaticBg.disabled = !enabled;
    dom.optMouseFollow.disabled = !enabled;
    
    // When 90s mode is toggled on, auto-enable sub-options
    if (enabled) {
      dom.optStaticBg.checked = true;
      dom.optMouseFollow.checked = true;
      dom.cameraX.disabled = true;
    } else {
      dom.cameraX.disabled = false;
    }
    
    logOptions('mode90s');
    rerender();
  });

  dom.optStaticBg.addEventListener('change', () => {
    logOptions('staticBg');
    rerender();
  });
  
  dom.optMouseFollow.addEventListener('change', (e) => {
    logOptions('mouseFollow');
    dom.cameraX.disabled = e.target.checked;
  });

  // Sprites toggle
  if (dom.optShowSprites) {
    dom.optShowSprites.addEventListener('change', (e) => {
      BLBRenderer.state.showSprites = e.target.checked;
      console.log(`[SPRITES] Show sprites: ${e.target.checked}`);
      rerender();
    });
  }

  // Sprite animation toggle
  if (dom.optAnimateSprites) {
    dom.optAnimateSprites.addEventListener('change', (e) => {
      BLBRenderer.state.animateSprites = e.target.checked;
      console.log(`[SPRITES] Animate sprites: ${e.target.checked}`);
      if (e.target.checked) {
        startSpriteAnimation();
      } else {
        stopSpriteAnimation();
      }
    });
  }

  // Entity debug toggle
  if (dom.optEntityDebug) {
    dom.optEntityDebug.addEventListener('change', (e) => {
      BLBRenderer.state.showEntityDebug = e.target.checked;
      console.log(`[ENTITIES] Show debug: ${e.target.checked}`);
      rerender();
    });
  }

  // Player toggle
  if (dom.optShowPlayer) {
    dom.optShowPlayer.addEventListener('change', (e) => {
      BLBRenderer.state.showPlayer = e.target.checked;
      console.log(`[PLAYER] Show player: ${e.target.checked}`);
      rerender();
    });
  }

  // Mouse tracking for camera - RTS style (works in 90s mode)
  dom.renderContainer.addEventListener('mousemove', (e) => {
    if (!dom.optMouseFollow.checked || !BLBRenderer.state.cachedStage) return;
    if (!dom.mode90s.checked) return; // Only in 90s mode
    
    // Get canvas bounds (canvas is centered in container in 90s mode)
    const canvasRect = dom.canvas.getBoundingClientRect();
    const mouseX = e.clientX - canvasRect.left;
    const mouseY = e.clientY - canvasRect.top;
    const canvasWidth = canvasRect.width;
    const canvasHeight = canvasRect.height;
    
    // Only track if mouse is over the canvas
    if (mouseX < 0 || mouseX > canvasWidth || mouseY < 0 || mouseY > canvasHeight) return;
    
    const limits = BLBRenderer.getCameraLimits();
    
    // Direct linear mapping: mouse position on canvas maps to camera position
    const ratioX = Math.max(0, Math.min(1, mouseX / canvasWidth));
    const ratioY = Math.max(0, Math.min(1, mouseY / canvasHeight));
    
    const newCameraX = Math.floor(ratioX * limits.maxX);
    const newCameraY = Math.floor(ratioY * limits.maxY);
    
    if (newCameraX !== BLBRenderer.state.cameraX || newCameraY !== BLBRenderer.state.cameraY) {
      BLBRenderer.setCamera(newCameraX, newCameraY);
      dom.cameraX.value = newCameraX;
      dom.cameraXVal.textContent = newCameraX;
      rerender();
    }
  });

  // Arrow key camera control (works in both modes)
  document.addEventListener('keydown', (e) => {
    const debug = dom.optDebug && dom.optDebug.checked;
    
    if (debug) {
      console.log(`[KEY] key=${e.key} target=${e.target.tagName} cachedStage=${!!BLBRenderer.state.cachedStage}`);
    }
    
    if (!BLBRenderer.state.cachedStage) {
      if (debug) console.log('[KEY] No cachedStage, ignoring');
      return;
    }
    
    // Don't capture if user is in an input field
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') {
      if (debug) console.log('[KEY] In input field, ignoring');
      return;
    }
    
    const SCROLL_SPEED = 16;
    let dx = 0, dy = 0;
    
    switch (e.key) {
      case 'ArrowLeft': dx = -SCROLL_SPEED; break;
      case 'ArrowRight': dx = SCROLL_SPEED; break;
      case 'ArrowUp': dy = -SCROLL_SPEED; break;
      case 'ArrowDown': dy = SCROLL_SPEED; break;
      default: return;
    }
    
    e.preventDefault();
    
    if (debug) {
      console.log(`[KEY] Moving camera dx=${dx} dy=${dy} from (${BLBRenderer.state.cameraX}, ${BLBRenderer.state.cameraY})`);
    }
    
    const camera = BLBRenderer.moveCamera(dx, dy);
    
    if (debug) {
      console.log(`[KEY] Camera now at (${camera.x}, ${camera.y})`);
    }
    
    dom.cameraX.value = camera.x;
    dom.cameraXVal.textContent = camera.x;
    rerender();
  });

  // Drag and drop
  document.body.addEventListener('dragover', (e) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  });

  document.body.addEventListener('drop', (e) => {
    e.preventDefault();
    const file = e.dataTransfer.files[0];
    if (file && file.name.toLowerCase().endsWith('.blb')) {
      loadBLBFile(file);
    }
  });

  // Console panel
  dom.btnConsole.addEventListener('click', () => consoleLogger.toggle());
  dom.btnConsoleClear.addEventListener('click', () => consoleLogger.clear());
  dom.btnConsoleClose.addEventListener('click', () => consoleLogger.hide());
  
  // Entities filter and group controls
  dom.entitiesFilter.addEventListener('input', (e) => {
    entitiesPanel.filterText = e.target.value;
    entitiesPanel.selectedIndex = -1;
    entitiesPanel.applyFilter();
  });
  
  dom.entitiesGroup.addEventListener('change', (e) => {
    entitiesPanel.groupMode = e.target.value;
    entitiesPanel.selectedIndex = -1;
    entitiesPanel.render();
  });
}

// ============================================================================
// Initialization
// ============================================================================

function initUI() {
  initDOM();
  setupConsoleInterception();
  
  // Initialize renderer with canvas elements
  BLBRenderer.initRenderer(dom.canvas, dom.minimapCanvas);
  
  setupEventHandlers();
  
  setStatus('Ready - Open a BLB file or drag and drop');
  console.info('BLB Viewer initialized');
}

// ============================================================================
// Exports
// ============================================================================

const BLBUI = {
  state: UIState,
  dom,
  consoleLogger,
  
  // Initialization
  initUI,
  
  // Helpers
  setStatus,
  showLoading,
  rerender,
  updateLayersPanel,
  
  // File handling
  loadBLBFile,
  
  // Tree view
  buildTreeView,
  handleTreeSelection,
};

// Make available globally for non-module usage
if (typeof window !== 'undefined') {
  window.BLBUI = BLBUI;
}

// Auto-initialize when DOM is ready
if (typeof document !== 'undefined') {
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initUI);
  } else {
    initUI();
  }
}
