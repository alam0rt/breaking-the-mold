/**
 * Skullmonkeys BLB File Format Parser
 * 
 * Handles BLB archive structure: header, level entries, segments, and TOC parsing.
 * This module only knows about the file format, not how the game uses the data.
 * 
 * BLB Structure:
 *   - Header (4096 bytes): Contains up to 26 level entries
 *   - Segments: Primary, Secondary, and Tertiary data blocks per level/stage
 *   - Each segment contains a TOC (Table of Contents) pointing to assets
 */

// ============================================================================
// BLB Format Constants
// ============================================================================

const BLB_SECTOR_SIZE = 2048;
const BLB_HEADER_SIZE = 4096;
const BLB_LEVEL_ENTRY_SIZE = 0x70; // 112 bytes per level entry
const BLB_MAX_LEVELS = 26;
const BLB_MAX_STAGES_PER_LEVEL = 6;

// ============================================================================
// Low-level Utilities
// ============================================================================

/**
 * Read a null-terminated ASCII string from a byte array
 */
function readString(data, offset, maxLength) {
  const bytes = new Uint8Array(data.buffer, data.byteOffset + offset, maxLength);
  let str = '';
  for (let i = 0; i < maxLength; i++) {
    if (bytes[i] === 0) break;
    str += String.fromCharCode(bytes[i]);
  }
  return str.trim();
}

/**
 * Format byte size for display
 */
function formatSize(bytes) {
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
}

// ============================================================================
// Level Entry Parsing
// ============================================================================

/**
 * Parse a single level entry from the BLB header
 * 
 * Level Entry Structure (112 bytes):
 *   0x00: u16 sectorOffset     - Primary segment sector
 *   0x02: u16 sectorCount      - Primary segment size in sectors
 *   0x04: u32 bufferSize       - Decompressed buffer size
 *   0x08: u32 entry1Offset     - Unknown offset
 *   0x0C: u8  levelIndex       - Level index
 *   0x0D: u8  selectable       - Whether level appears in menu
 *   0x0E: u16 stageCount       - Number of stages in this level
 *   0x10: u16[6] tertDataOff   - Tertiary data offsets per stage
 *   0x1E: u16[6] secSectorOff  - Secondary sector offsets per stage
 *   0x2C: u16[6] secSectorCnt  - Secondary sector counts per stage
 *   0x3A: u16[6] tertSectorOff - Tertiary sector offsets per stage
 *   0x48: u16[6] tertSectorCnt - Tertiary sector counts per stage
 *   0x56: char[5] levelId      - Level ID (e.g., "SCIE")
 *   0x5B: char[21] levelName   - Level name
 */
function parseLevelEntry(headerData, index) {
  const offset = index * BLB_LEVEL_ENTRY_SIZE;
  const view = new DataView(headerData.buffer, headerData.byteOffset + offset, BLB_LEVEL_ENTRY_SIZE);
  
  const levelId = readString(headerData, offset + 0x56, 5);
  const levelName = readString(headerData, offset + 0x5B, 21);
  
  // Skip empty entries
  if (!levelId || levelId === '\x00\x00\x00\x00\x00') {
    return null;
  }
  
  const stageCount = view.getUint16(0x0E, true);
  
  // Read per-stage arrays (up to 6 stages per level)
  const stages = [];
  for (let i = 0; i < BLB_MAX_STAGES_PER_LEVEL; i++) {
    stages.push({
      tertDataOff: view.getUint16(0x10 + i * 2, true),
      secSectorOff: view.getUint16(0x1E + i * 2, true),
      secSectorCnt: view.getUint16(0x2C + i * 2, true),
      tertSectorOff: view.getUint16(0x3A + i * 2, true),
      tertSectorCnt: view.getUint16(0x48 + i * 2, true),
    });
  }
  
  return {
    index,
    levelId,
    levelName,
    sectorOffset: view.getUint16(0x00, true),
    sectorCount: view.getUint16(0x02, true),
    bufferSize: view.getUint32(0x04, true),
    entry1Offset: view.getUint32(0x08, true),
    levelIndex: view.getUint8(0x0C),
    selectable: view.getUint8(0x0D),
    stageCount,
    stages,
  };
}

/**
 * Parse the BLB header to extract all level entries
 */
function parseHeader(fileData) {
  const headerData = new Uint8Array(fileData, 0, BLB_HEADER_SIZE);
  const levels = [];
  
  for (let i = 0; i < BLB_MAX_LEVELS; i++) {
    const level = parseLevelEntry(headerData, i);
    if (level) {
      levels.push(level);
    }
  }
  
  return { levels };
}

// ============================================================================
// Segment Access
// ============================================================================

/**
 * Read raw bytes from a segment given sector offset and count
 */
function readSegment(fileData, sectorOffset, sectorCount) {
  const byteOffset = sectorOffset * BLB_SECTOR_SIZE;
  const byteSize = sectorCount * BLB_SECTOR_SIZE;
  
  if (byteOffset + byteSize > fileData.byteLength) {
    return null;
  }
  
  return new Uint8Array(fileData, byteOffset, byteSize);
}

/**
 * Get the secondary segment data for a specific stage
 * Falls back to stage 0's secondary if the stage has no secondary
 */
function getSecondarySegment(fileData, level, stageIndex) {
  const stage = level.stages[stageIndex];
  
  if (stage.secSectorCnt > 0) {
    return readSegment(fileData, stage.secSectorOff, stage.secSectorCnt);
  }
  
  // Fall back to base secondary (stage 0)
  const baseStage = level.stages[0];
  return readSegment(fileData, baseStage.secSectorOff, baseStage.secSectorCnt);
}

/**
 * Get the tertiary segment data for a specific stage
 */
function getTertiarySegment(fileData, level, stageIndex) {
  const stage = level.stages[stageIndex];
  
  if (stage.tertSectorCnt === 0) {
    return null;
  }
  
  return readSegment(fileData, stage.tertSectorOff, stage.tertSectorCnt);
}

/**
 * Get the primary segment data for a level
 */
function getPrimarySegment(fileData, level) {
  return readSegment(fileData, level.sectorOffset, level.sectorCount);
}

// ============================================================================
// TOC (Table of Contents) Parsing
// ============================================================================

/**
 * Parse a segment's Table of Contents
 * 
 * TOC Structure:
 *   0x00: u32 entryCount
 *   For each entry (12 bytes):
 *     0x00: u32 assetType  - Asset type ID
 *     0x04: u32 size       - Asset size in bytes
 *     0x08: u32 offset     - Offset within segment
 * 
 * Returns: Map of assetType -> { offset, size, data }
 */
function parseTOC(segmentData) {
  if (!segmentData || segmentData.length < 4) return {};
  
  const view = new DataView(segmentData.buffer, segmentData.byteOffset);
  const count = view.getUint32(0, true);
  
  // Sanity check
  if (count > 1000) {
    return {};
  }
  
  const assets = {};
  for (let i = 0; i < count; i++) {
    const entryOffset = 4 + i * 12;
    if (entryOffset + 12 > segmentData.length) break;
    
    const assetType = view.getUint32(entryOffset, true);
    const size = view.getUint32(entryOffset + 4, true);
    const dataOffset = view.getUint32(entryOffset + 8, true);
    
    if (dataOffset + size <= segmentData.length) {
      assets[assetType] = {
        type: assetType,
        offset: dataOffset,
        size,
        data: segmentData.slice(dataOffset, dataOffset + size),
      };
    }
  }
  
  return assets;
}

/**
 * Parse a sub-TOC (container asset with multiple entries)
 * 
 * Sub-TOC Structure (same as TOC but entries have IDs instead of types):
 *   0x00: u32 entryCount
 *   For each entry (12 bytes):
 *     0x00: u32 id
 *     0x04: u32 size
 *     0x08: u32 offset
 * 
 * Returns: Array of { id, size, offset, data }
 */
function parseSubTOC(containerData) {
  if (!containerData || containerData.length < 4) return [];
  
  const view = new DataView(containerData.buffer, containerData.byteOffset);
  const count = view.getUint32(0, true);
  
  // Sanity check
  if (count > 10000) return [];
  
  const entries = [];
  for (let i = 0; i < count; i++) {
    const entryOffset = 4 + i * 12;
    if (entryOffset + 12 > containerData.length) break;
    
    const id = view.getUint32(entryOffset, true);
    const size = view.getUint32(entryOffset + 4, true);
    const dataOffset = view.getUint32(entryOffset + 8, true);
    
    if (dataOffset + size <= containerData.length) {
      entries.push({
        id,
        size,
        offset: dataOffset,
        data: containerData.slice(dataOffset, dataOffset + size),
      });
    }
  }
  
  return entries;
}

// ============================================================================
// High-level Stage Data Access
// ============================================================================

/**
 * Load all raw assets for a specific stage
 * Returns primary, secondary and tertiary TOCs ready for game engine processing
 * 
 * Primary segment contains level-wide assets (shared across all stages):
 *   - Asset 600 (0x258): Level geometry/world data (background graphics)
 *   - Asset 601 (0x259): Collision/layout data
 *   - Asset 602 (0x25A): Palette/color data
 * 
 * Secondary segment contains tileset data
 * Tertiary segment contains stage-specific sprites (Asset 600)
 */
function loadStageAssets(fileData, level, stageIndex) {
  const primaryData = getPrimarySegment(fileData, level);
  const secondaryData = getSecondarySegment(fileData, level, stageIndex);
  const tertiaryData = getTertiarySegment(fileData, level, stageIndex);
  
  if (!secondaryData || !tertiaryData) {
    return null;
  }
  
  return {
    level,
    stageIndex,
    primary: primaryData ? parseTOC(primaryData) : {},
    secondary: parseTOC(secondaryData),
    tertiary: parseTOC(tertiaryData),
  };
}

// ============================================================================
// Exports
// ============================================================================

const BLB = {
  // Constants
  SECTOR_SIZE: BLB_SECTOR_SIZE,
  HEADER_SIZE: BLB_HEADER_SIZE,
  LEVEL_ENTRY_SIZE: BLB_LEVEL_ENTRY_SIZE,
  MAX_LEVELS: BLB_MAX_LEVELS,
  MAX_STAGES_PER_LEVEL: BLB_MAX_STAGES_PER_LEVEL,
  
  // Utilities
  formatSize,
  readString,
  
  // Header parsing
  parseHeader,
  parseLevelEntry,
  
  // Segment access
  readSegment,
  getPrimarySegment,
  getSecondarySegment,
  getTertiarySegment,
  
  // TOC parsing
  parseTOC,
  parseSubTOC,
  
  // High-level access
  loadStageAssets,
};

// Make available globally
if (typeof window !== 'undefined') {
  window.BLB = BLB;
}
