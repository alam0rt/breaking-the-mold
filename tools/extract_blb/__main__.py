#!/usr/bin/env python3
"""
BLB Asset Extraction Tool

Extracts all assets from a Skullmonkeys GAME.BLB file using the ImHex JSON output
to locate assets, then reads raw bytes from the BLB binary.

Usage:
    python scripts/extract_blb.py --json /tmp/blb.json --blb disks/blb/GAME.BLB -o extracted/

The script uses a handler registry pattern - handlers can be registered for specific
asset types to provide conversion (e.g., sprites to PNG). If no handler is registered,
the default handler saves raw bytes as .bin files.

Output structure (folders named by what each BLB segment actually holds;
see segment_to_relpath() for the raw-key -> path mapping):
    extracted/
    ├── header/
    │   ├── movies/
    │   └── sectors/
    ├── EGGS/                  # world (level code)
    │   ├── shared/            # primary: world-shared geometry + audio
    │   │   └── 600.bin
    │   ├── stage0/
    │   │   ├── tileset/       # secondary[0]: tile/palette graphics bank
    │   │   │   └── 100.bin
    │   │   └── map/           # tertiary[0]: tilemap + collision + entities
    │   │       └── 501.bin
    │   └── stage1/
    │       ├── tileset/       # secondary1
    │       └── map/           # stage1
    └── extraction_report.json
"""

import argparse
import json
import os
import sys
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Callable, Optional
from urllib.parse import unquote

# ============================================================================
# Constants
# ============================================================================

SECTOR_SIZE = 2048
HEADER_SIZE = 4096

# Header asset types (virtual IDs for header-level data, 1000+)
# These allow header data to use the same handler registry as level assets
HEADER_ASSET_TYPES = {
    1000: "bs_loading_screen",    # Sector table entries (loading screens)
    1001: "bs_credits",           # Credits sequence BS frames
    1002: "bs_password_screen",   # Mode 6 / password screen BS frames
    1003: "bs_unreferenced",      # Known unreferenced BS frames (cut content)
}

# Unreferenced sectors (not in any table but contain valid data)
UNREFERENCED_SECTORS = [
    {"name": "unused_legal_screen", "sector": 2, "count": 10, "asset_type": 1003},
]

# Fallback asset type names (used only if not provided in JSON)
# The ImHex template (blb.hexpat) is the source of truth - it exports
# asset_name in each TOC entry. This fallback is for compatibility.
ASSET_NAMES_FALLBACK = {
    # Level assets (100-799)
    100: "tile_header",
    101: "vram_slot_config",
    200: "tilemap_container",
    201: "layer_entries",
    300: "tile_pixels",
    301: "palette_indices",
    302: "tile_flags",
    303: "animated_tiles",
    400: "palette_container",
    401: "palette_anim",
    500: "tile_attributes",
    501: "entities",
    502: "vram_rects",
    503: "anim_offsets",
    504: "vehicle_data",
    600: "geometry_or_sprites",
    601: "audio_samples",
    602: "palette_data",
    700: "spu_audio",
    # Header assets (1000+)
    **HEADER_ASSET_TYPES,
}

# ============================================================================
# Data Classes
# ============================================================================

@dataclass
class AssetInfo:
    """Information about an extracted asset."""
    asset_type: int
    asset_name: str              # Human-readable name from hexpat
    size: int
    offset_in_segment: int  # Offset relative to segment start
    segment_offset: int     # Absolute offset of segment in BLB
    absolute_offset: int    # Absolute offset in BLB file
    level: str
    segment: str
    toc_index: int
    output_path: Optional[str] = None
    handler_used: str = "default"
    error: Optional[str] = None

@dataclass
class CoverageRegion:
    """A region of extracted bytes in the BLB file."""
    start: int
    end: int  # exclusive
    description: str
    level: str = ""
    segment: str = ""
    asset_type: int = 0

@dataclass
class GapInfo:
    """Information about a gap (unextracted region)."""
    start: int
    end: int
    size: int
    description: str
    is_padding: bool = False  # True if gap appears to be sector padding (mostly zeros)

@dataclass
class OverlapInfo:
    """Information about overlapping extractions."""
    start: int
    end: int
    size: int
    region1: str
    region2: str

@dataclass
class ExtractionReport:
    """Summary of extraction results."""
    blb_file: str
    json_file: str
    output_dir: str
    blb_size: int = 0
    total_assets: int = 0
    extracted_assets: int = 0
    failed_assets: int = 0
    total_bytes_extracted: int = 0
    unique_bytes_extracted: int = 0  # Actual unique bytes (no overlap counting)
    coverage_percent: float = 0.0
    padding_bytes: int = 0       # Bytes identified as sector padding
    unknown_data_bytes: int = 0  # Bytes that contain non-zero data but weren't extracted
    overlap_bytes: int = 0       # Bytes extracted multiple times
    overlaps: list = field(default_factory=list)  # List of OverlapInfo
    gaps: list = field(default_factory=list)
    largest_gaps: list = field(default_factory=list)
    assets: list = field(default_factory=list)
    warnings: list = field(default_factory=list)
    errors: list = field(default_factory=list)

# ============================================================================
# Coverage Tracking
# ============================================================================

def is_all_zeros(data: bytes, threshold: float = 1.0) -> bool:
    """Check if data is mostly zeros (likely sector padding)."""
    if not data:
        return True
    zero_count = data.count(b'\x00'[0])
    return (zero_count / len(data)) >= threshold


def compute_coverage_gaps(
    regions: list[CoverageRegion],
    file_size: int,
    blb_data: bytes
) -> tuple[list[GapInfo], list[OverlapInfo], float, int, int, int]:
    """
    Compute gaps between extracted regions and classify as padding or unknown data.
    Also detects overlapping extractions.
    
    Returns:
        Tuple of (gaps, overlaps, coverage_percent, padding_bytes, unknown_data_bytes, unique_bytes)
    """
    if not regions:
        return [GapInfo(0, file_size, file_size, "entire file")], [], 0.0, 0, file_size, 0
    
    # Sort regions by start offset
    sorted_regions = sorted(regions, key=lambda r: (r.start, r.end))
    
    # Detect overlaps BEFORE merging
    overlaps = []
    for i, r1 in enumerate(sorted_regions):
        for r2 in sorted_regions[i+1:]:
            if r2.start >= r1.end:
                break  # No more possible overlaps with r1
            # r2.start < r1.end means overlap
            overlap_start = r2.start
            overlap_end = min(r1.end, r2.end)
            overlap_size = overlap_end - overlap_start
            if overlap_size > 0:
                overlaps.append(OverlapInfo(
                    start=overlap_start,
                    end=overlap_end,
                    size=overlap_size,
                    region1=r1.description,
                    region2=r2.description,
                ))
    
    # Merge overlapping regions for gap calculation
    merged = []
    for region in sorted_regions:
        if merged and region.start <= merged[-1].end:
            # Extend the last region if overlapping
            merged[-1] = CoverageRegion(
                merged[-1].start,
                max(merged[-1].end, region.end),
                merged[-1].description + "; " + region.description
            )
        else:
            merged.append(region)
    
    # Find gaps
    gaps = []
    prev_end = 0
    padding_bytes = 0
    unknown_data_bytes = 0
    
    for region in merged:
        if region.start > prev_end:
            gap_size = region.start - prev_end
            gap_data = blb_data[prev_end:region.start]
            is_padding = is_all_zeros(gap_data)
            
            if is_padding:
                padding_bytes += gap_size
                desc = f"sector padding before {region.description.split(';')[0]}"
            else:
                unknown_data_bytes += gap_size
                desc = f"unknown data before {region.description.split(';')[0]}"
            
            gaps.append(GapInfo(prev_end, region.start, gap_size, desc, is_padding))
        prev_end = max(prev_end, region.end)
    
    # Check for gap at end of file
    if prev_end < file_size:
        gap_size = file_size - prev_end
        gap_data = blb_data[prev_end:file_size]
        is_padding = is_mostly_zeros(gap_data)
        
        if is_padding:
            padding_bytes += gap_size
            desc = "sector padding at end of file"
        else:
            unknown_data_bytes += gap_size
            desc = "unknown data at end of file"
        
        gaps.append(GapInfo(prev_end, file_size, gap_size, desc, is_padding))
    
    # Calculate coverage (unique bytes, not counting overlaps)
    unique_bytes = sum(r.end - r.start for r in merged)
    coverage_pct = (unique_bytes / file_size * 100) if file_size > 0 else 0.0
    
    return gaps, overlaps, coverage_pct, padding_bytes, unknown_data_bytes, unique_bytes

# ============================================================================
# Handler Registry - Import from handlers module
# ============================================================================

from .handlers import register_handler, get_handler, default_handler, register_all_handlers

# Register all available handlers
register_all_handlers()

# ============================================================================
# JSON Parsing Helpers
# ============================================================================

def decode_string(s: str) -> str:
    """Decode URL-encoded string from ImHex output."""
    if not s:
        return s
    return unquote(s).rstrip('\x00')

def parse_hex_offset(offset_str: str) -> int:
    """Parse hex offset string (e.g., '0x28') to int."""
    if isinstance(offset_str, int):
        return offset_str
    return int(offset_str, 16)

def get_level_code(header: dict, level_index: int) -> str:
    """Get level code (e.g., 'SCIE') from header."""
    levels = header.get('header', {}).get('levels', [])
    if level_index < len(levels):
        return decode_string(levels[level_index].get('level_id', f'level_{level_index:02d}'))
    return f'level_{level_index:02d}'

def get_asset_name(entry: dict) -> str:
    """Get asset name from TOC entry, with fallback to hardcoded names."""
    # Prefer name from hexpat (source of truth)
    if 'asset_name' in entry:
        return entry['asset_name']
    # Fallback to hardcoded names
    asset_type = entry.get('asset_type', 0)
    return ASSET_NAMES_FALLBACK.get(asset_type, f"unknown_{asset_type}")

# ============================================================================
# Header Table Descriptors
# ============================================================================
# Each descriptor tells the extraction script how to treat a header table
# as an extractable container. The schema is:
#   table_path: where to find the table in JSON (dot-separated)
#   asset_type: virtual asset ID (1000+) for handler registry
#   segment_name: output folder name
#   name_format: function(entry, index) -> asset_name string

@dataclass
class HeaderTableDescriptor:
    """Describes a header table that contains extractable sector data."""
    table_path: str      # JSON path like "header.header.sectors"
    asset_type: int      # Virtual asset type (1000+)
    segment_name: str    # Output folder name
    name_format: Callable[[dict, int], str]  # (entry, index) -> name
    offset_field: str = "sector_offset"      # Field containing sector offset
    count_field: str = "sector_count"        # Field containing sector count


def _navigate_json_path(data: dict, path: str) -> list:
    """Navigate dot-separated path in JSON data, return list or empty list."""
    current = data
    for key in path.split('.'):
        if isinstance(current, dict) and key in current:
            current = current[key]
        else:
            return []
    return current if isinstance(current, list) else []


# Define extractable header tables
# This is the ONLY place you need to update when adding new header tables
HEADER_TABLES = [
    HeaderTableDescriptor(
        table_path="header.header.sectors",
        asset_type=1000,
        segment_name="sectors",
        name_format=lambda e, i: f"bs_loading_{decode_string(e.get('code', f'{i:02d}'))}",
    ),
    HeaderTableDescriptor(
        table_path="header.header.credits",
        asset_type=1001,
        segment_name="credits",
        name_format=lambda e, i: f"bs_credits_{decode_string(e.get('code', '')).strip() or f'{i:02d}'}",
    ),
    HeaderTableDescriptor(
        table_path="header.header.password_screens",
        asset_type=1002,
        segment_name="password_screens",
        name_format=lambda e, i: f"bs_mode6_{i:02d}",
    ),
]

# ============================================================================
# Segment -> output-path mapping
# ============================================================================
#
# Segment keys are the ImHex pattern's member names (blb.hexpat). The current
# template emits descriptive keys that already match what each segment is
# (verified against the level metadata struct + LoadAssetContainer @0x8007b074):
#
#   shared          -> world-shared assets, loaded once (600 geometry, 601/602 audio)
#   stageN_tileset  -> stage N's tile/palette GRAPHICS bank (300 pixels, 400 CLUTs, ...)
#   stageN_map      -> stage N's MAP: tilemap, collision, entities (200/201/500/501)
#
# tileset[N] and map[N] pair 1:1 per stage (both are u16[6] per-stage arrays in
# the level entry). Output is laid out world -> stage -> {tileset, map}:
#
#   EGGS/shared/...            (shared)
#   EGGS/stage0/tileset/...    (stage0_tileset)
#   EGGS/stage0/map/...        (stage0_map)
#   EGGS/stage1/tileset/...    (stage1_tileset)
#   EGGS/stage1/map/...        (stage1_map)
#
# Legacy positional keys (primary / secondary[N] / stageN) from older JSON
# exports are still accepted so previously-generated JSON keeps extracting.
# Header pseudo-segments (sectors/credits/...) and any unrecognised key pass
# through unchanged.

def segment_to_relpath(segment_name: str) -> str:
    """
    Map a BLB segment key to its descriptive output sub-path.

    Returns a POSIX-style relative path (may contain '/') under the world
    directory. Accepts both the current descriptive keys and the legacy
    positional keys; unknown keys are returned unchanged so new/header
    segments still extract somewhere sensible.
    """
    name = segment_name.strip()

    # --- Current descriptive keys (blb.hexpat member names) ---
    if name == "shared":
        return "shared"
    if name.startswith("stage") and name.endswith("_tileset"):
        n = name[len("stage"):-len("_tileset")]
        if n.isdigit():
            return f"stage{n}/tileset"
    if name.startswith("stage") and name.endswith("_map"):
        n = name[len("stage"):-len("_map")]
        if n.isdigit():
            return f"stage{n}/map"

    # --- Legacy positional keys (older JSON exports) ---
    if name == "primary":
        return "shared"
    # "secondary" == stage 0; "secondaryN" suffix is the stage index.
    if name == "secondary":
        return "stage0/tileset"
    if name.startswith("secondary") and name[len("secondary"):].isdigit():
        return f"stage{name[len('secondary'):]}/tileset"
    # Bare "stageN" (no _map/_tileset suffix) was the old tertiary/map key.
    if name.startswith("stage") and name[len("stage"):].isdigit():
        return f"{name}/map"

    # Header pseudo-segments and anything unrecognised: leave as-is.
    return name


# ============================================================================
# Asset Discovery
# ============================================================================

def discover_segment_assets(
    segment_data: dict,
    level_code: str,
    segment_name: str,
) -> list[AssetInfo]:
    """
    Discover assets from a single segment.
    
    A segment is any dict that has 'toc' (list of TOC entries) and '_base' (segment offset).
    This is generic - works for primary, secondary, tertiary, or any future segment type.
    """
    assets = []
    
    # Get segment base offset (required) - ImHex exports as '_base'
    if '_base' not in segment_data:
        return assets  # Not a segment with extractable assets
    
    segment_offset = parse_hex_offset(segment_data['_base'])
    toc = segment_data.get('toc', [])
    
    for toc_idx, entry in enumerate(toc):
        # Each TOC entry should have: asset_type, size, offset, asset_name
        if 'asset_type' not in entry or 'size' not in entry or 'offset' not in entry:
            continue
        
        asset_offset = parse_hex_offset(entry['offset'])
        assets.append(AssetInfo(
            asset_type=entry['asset_type'],
            asset_name=get_asset_name(entry),
            size=entry['size'],
            offset_in_segment=asset_offset,
            segment_offset=segment_offset,
            absolute_offset=segment_offset + asset_offset,
            level=level_code,
            segment=segment_name,
            toc_index=toc_idx,
        ))
    
    return assets


def discover_assets(data: dict) -> list[AssetInfo]:
    """
    Discover all assets from ImHex JSON output.
    
    Iterates over all levels and their segments generically.
    Any dict with 'toc' and '_base' fields is treated as an extractable segment.
    
    Returns list of AssetInfo objects with location information.
    """
    assets = []
    header = data.get('header', {})
    levels = data.get('levels', {})
    
    for level_key in sorted(levels.keys(), key=lambda x: int(x.replace('level_', '')) if x.startswith('level_') else 999):
        level_index = int(level_key.replace('level_', '')) if level_key.startswith('level_') else -1
        level_code = get_level_code(header, level_index) if level_index >= 0 else level_key
        level_data = levels[level_key]
        
        # Iterate over all segments in this level
        for segment_name, segment_data in level_data.items():
            if not isinstance(segment_data, dict):
                continue
            
            # Check if this looks like a segment (has toc and _base)
            if 'toc' in segment_data and '_base' in segment_data:
                segment_assets = discover_segment_assets(segment_data, level_code, segment_name)
                assets.extend(segment_assets)
    
    return assets


def discover_toc_coverage(data: dict) -> list[CoverageRegion]:
    """
    Discover TOC (table of contents) regions for coverage tracking.
    
    Each segment has a TOC at its start that lists the assets. The TOC spans
    from the segment base to the first asset's offset. This function returns
    CoverageRegion entries for all TOCs so they're counted as "covered".
    
    Returns list of CoverageRegion for TOC structures.
    """
    coverage = []
    header = data.get('header', {})
    levels = data.get('levels', {})
    
    for level_key in sorted(levels.keys(), key=lambda x: int(x.replace('level_', '')) if x.startswith('level_') else 999):
        level_index = int(level_key.replace('level_', '')) if level_key.startswith('level_') else -1
        level_code = get_level_code(header, level_index) if level_index >= 0 else level_key
        level_data = levels[level_key]
        
        # Iterate over all segments in this level
        for segment_name, segment_data in level_data.items():
            if not isinstance(segment_data, dict):
                continue
            
            # Check if this looks like a segment (has toc and _base)
            if 'toc' not in segment_data or '_base' not in segment_data:
                continue
                
            segment_offset = parse_hex_offset(segment_data['_base'])
            toc = segment_data.get('toc', [])
            
            if not toc:
                continue
            
            # Find the first asset offset to determine TOC size
            first_asset_offset = None
            for entry in toc:
                if 'offset' in entry:
                    offset = parse_hex_offset(entry['offset'])
                    if first_asset_offset is None or offset < first_asset_offset:
                        first_asset_offset = offset
            
            if first_asset_offset is not None and first_asset_offset > 0:
                coverage.append(CoverageRegion(
                    start=segment_offset,
                    end=segment_offset + first_asset_offset,
                    description=f"{level_code}/{segment_name}/TOC",
                    level=level_code,
                    segment=segment_name,
                    asset_type=0,  # 0 = TOC structure
                ))
    
    return coverage


def extract_segment_metadata(data: dict, output_dir: Path) -> int:
    """
    Extract segment metadata (TOC, headers) as JSON files.
    
    For each segment (primary, secondary, stage0, etc.), creates a .json file
    containing the parsed structure: TOC entries, stage headers, and other
    metadata from the ImHex template.
    
    Args:
        data: Parsed ImHex JSON output
        output_dir: Base output directory
        
    Returns:
        Number of metadata files created
    """
    count = 0
    header = data.get('header', {})
    levels = data.get('levels', {})
    
    for level_key in sorted(levels.keys(), key=lambda x: int(x.replace('level_', '')) if x.startswith('level_') else 999):
        level_index = int(level_key.replace('level_', '')) if level_key.startswith('level_') else -1
        level_code = get_level_code(header, level_index) if level_index >= 0 else level_key
        level_data = levels[level_key]
        
        # Iterate over all segments in this level
        for segment_name, segment_data in level_data.items():
            if not isinstance(segment_data, dict):
                continue
            
            # Check if this looks like a segment (has toc and _base)
            if 'toc' not in segment_data or '_base' not in segment_data:
                continue
            
            # Build metadata structure
            segment_offset = parse_hex_offset(segment_data['_base'])
            toc = segment_data.get('toc', [])
            
            metadata = {
                "level": level_code,
                "level_index": level_index,
                "segment": segment_name,
                "segment_offset": segment_offset,
                "segment_offset_hex": f"0x{segment_offset:X}",
                "toc_count": segment_data.get('toc_count', len(toc)),
                "toc": [],
            }
            
            # Process TOC entries
            for entry in toc:
                toc_entry = {
                    "asset_type": entry.get('asset_type'),
                    "asset_name": get_asset_name(entry),
                    "size": entry.get('size', 0),
                    "offset": parse_hex_offset(entry.get('offset', '0')),
                    "offset_hex": entry.get('offset', '0x0'),
                }
                # Calculate absolute offset
                toc_entry["absolute_offset"] = segment_offset + toc_entry["offset"]
                toc_entry["absolute_offset_hex"] = f"0x{toc_entry['absolute_offset']:X}"
                metadata["toc"].append(toc_entry)
            
            # Include stage header if present (for stage segments)
            if 'stage_header' in segment_data:
                sh = segment_data['stage_header']
                metadata["stage_header"] = {
                    "bg_color": {
                        "r": sh.get('bg_r', 0),
                        "g": sh.get('bg_g', 0),
                        "b": sh.get('bg_b', 0),
                    },
                    "secondary_color": {
                        "r": sh.get('secondary_r', 0),
                        "g": sh.get('secondary_g', 0),
                        "b": sh.get('secondary_b', 0),
                    },
                    "level_width": sh.get('level_width', 0),
                    "level_height": sh.get('level_height', 0),
                    "spawn_x": sh.get('spawn_x', 0),
                    "spawn_y": sh.get('spawn_y', 0),
                    "count_16x16": sh.get('count_16x16', 0),
                    "count_8x8_a": sh.get('count_8x8_a', 0),
                    "count_8x8_b": sh.get('count_8x8_b', 0),
                    "field_1c": sh.get('field_1c', 0),
                    "field_1e": sh.get('field_1e', 0),
                }
            
            # Write metadata JSON alongside the segment's extracted assets.
            # Mapped path may be nested (e.g. "stage0/tileset"); name the file
            # after its leaf (tileset.json / map.json / shared.json).
            rel = segment_to_relpath(segment_name)
            seg_dir = output_dir / level_code / rel
            seg_dir.mkdir(parents=True, exist_ok=True)
            metadata_path = seg_dir / f"{Path(rel).name}.json"
            metadata_path.write_text(json.dumps(metadata, indent=2))
            count += 1
    
    # Also extract header-level metadata
    header_metadata = extract_header_metadata(data, output_dir)
    count += header_metadata
    
    return count


def extract_header_metadata(data: dict, output_dir: Path) -> int:
    """
    Extract BLB header metadata as JSON.
    
    Creates header.json with level table, sector table, credits, etc.
    
    Returns:
        Number of metadata files created
    """
    header = data.get('header', {}).get('header', {})
    if not header:
        return 0
    
    # Build header metadata
    metadata = {
        "magic": header.get('magic', ''),
        "version": header.get('version', 0),
        "level_count": header.get('level_count', 0),
        "movie_count": header.get('movie_count', 0),
        "levels": [],
        "sectors": [],
        "credits": [],
        "password_screens": [],
    }
    
    # Level table
    for i, level in enumerate(header.get('levels', [])):
        level_entry = {
            "index": i,
            "level_id": decode_string(level.get('level_id', '')),
            "sector_offset": level.get('sector_offset', 0),
            "unknown_02": level.get('unknown_02', 0),
        }
        # Add stage offsets if present
        for stage_idx in range(8):
            key = f'tert_sector_off_{stage_idx}'
            if key in level:
                level_entry[key] = level[key]
        metadata["levels"].append(level_entry)
    
    # Sector table (loading screens)
    for i, sector in enumerate(header.get('sectors', [])):
        sector_offset = sector.get('sector_offset', 0)
        if isinstance(sector_offset, str):
            sector_offset = int(sector_offset, 16) if sector_offset.startswith('0x') else int(sector_offset)
        sector_count = sector.get('sector_count', 0)
        if isinstance(sector_count, str):
            sector_count = int(sector_count, 16) if sector_count.startswith('0x') else int(sector_count)
        metadata["sectors"].append({
            "index": i,
            "code": decode_string(sector.get('code', '')),
            "sector_offset": sector_offset,
            "sector_count": sector_count,
            "absolute_offset": sector_offset * SECTOR_SIZE,
            "size": sector_count * SECTOR_SIZE,
        })
    
    # Credits table
    for i, credit in enumerate(header.get('credits', [])):
        sector_offset = credit.get('sector_offset', 0)
        if isinstance(sector_offset, str):
            sector_offset = int(sector_offset, 16) if sector_offset.startswith('0x') else int(sector_offset)
        sector_count = credit.get('sector_count', 0)
        if isinstance(sector_count, str):
            sector_count = int(sector_count, 16) if sector_count.startswith('0x') else int(sector_count)
        metadata["credits"].append({
            "index": i,
            "code": decode_string(credit.get('code', '')),
            "sector_offset": sector_offset,
            "sector_count": sector_count,
            "absolute_offset": sector_offset * SECTOR_SIZE,
            "size": sector_count * SECTOR_SIZE,
        })
    
    # Password screens
    for i, ps in enumerate(header.get('password_screens', [])):
        sector_offset = ps.get('sector_offset', 0)
        if isinstance(sector_offset, str):
            sector_offset = int(sector_offset, 16) if sector_offset.startswith('0x') else int(sector_offset)
        sector_count = ps.get('sector_count', 0)
        if isinstance(sector_count, str):
            sector_count = int(sector_count, 16) if sector_count.startswith('0x') else int(sector_count)
        metadata["password_screens"].append({
            "index": i,
            "sector_offset": sector_offset,
            "sector_count": sector_count,
            "absolute_offset": sector_offset * SECTOR_SIZE,
            "size": sector_count * SECTOR_SIZE,
        })
    
    # Write header metadata
    header_dir = output_dir / "header"
    header_dir.mkdir(parents=True, exist_ok=True)
    metadata_path = header_dir / "blb_header.json"
    metadata_path.write_text(json.dumps(metadata, indent=2))
    
    return 1


def discover_header_assets(data: dict) -> list[AssetInfo]:
    """
    Discover header-level assets (loading screens, credits, password screens).
    
    Uses HEADER_TABLES descriptors to generically discover any header table
    that contains sector-based data. Each descriptor defines:
      - table_path: JSON path to the table
      - asset_type: Virtual asset ID for handler registry
      - segment_name: Output folder name
      - name_format: Function to generate asset names
    
    Returns list of AssetInfo objects with location information.
    """
    assets = []
    
    # Process each header table descriptor
    for descriptor in HEADER_TABLES:
        table = _navigate_json_path(data, descriptor.table_path)
        
        for i, entry in enumerate(table):
            # Get offset and count using descriptor's field names
            sector_offset = entry.get(descriptor.offset_field, 0)
            if isinstance(sector_offset, str):
                sector_offset = int(sector_offset, 16) if sector_offset.startswith('0x') else int(sector_offset)
            
            sector_count = entry.get(descriptor.count_field, 0)
            
            # Skip invalid entries
            if sector_count <= 0 or sector_offset <= 0:
                continue
            
            assets.append(AssetInfo(
                asset_type=descriptor.asset_type,
                asset_name=descriptor.name_format(entry, i),
                size=sector_count * SECTOR_SIZE,
                offset_in_segment=0,  # Header assets don't have segments
                segment_offset=0,
                absolute_offset=sector_offset * SECTOR_SIZE,
                level="header",
                segment=descriptor.segment_name,
                toc_index=i,
            ))
    
    # Add unreferenced sectors (defined in UNREFERENCED_SECTORS constant)
    for i, entry in enumerate(UNREFERENCED_SECTORS):
        assets.append(AssetInfo(
            asset_type=entry.get("asset_type", 1003),
            asset_name=entry["name"],
            size=entry["count"] * SECTOR_SIZE,
            offset_in_segment=0,
            segment_offset=0,
            absolute_offset=entry["sector"] * SECTOR_SIZE,
            level="header",
            segment="unreferenced",
            toc_index=i,
        ))
    
    return assets

# ============================================================================
# Extraction
# ============================================================================

def extract_assets(
    blb_path: Path,
    assets: list[AssetInfo],
    output_dir: Path,
    report: ExtractionReport
) -> list[CoverageRegion]:
    """
    Extract all assets from BLB file.
    
    Returns list of CoverageRegion for tracking what bytes were extracted.
    """
    coverage = []
    
    with open(blb_path, 'rb') as f:
        blb_data = f.read()
    
    blb_size = len(blb_data)
    
    for asset in assets:
        report.total_assets += 1
        
        # Validate offset and size
        if asset.absolute_offset < 0:
            asset.error = f"Negative offset: {asset.absolute_offset}"
            report.errors.append(f"{asset.level}/{asset.segment}/asset_{asset.asset_type}: {asset.error}")
            report.failed_assets += 1
            continue
        
        if asset.absolute_offset + asset.size > blb_size:
            asset.error = f"Asset extends beyond file: offset={hex(asset.absolute_offset)}, size={asset.size}, file_size={blb_size}"
            report.errors.append(f"{asset.level}/{asset.segment}/asset_{asset.asset_type}: {asset.error}")
            report.failed_assets += 1
            continue
        
        if asset.size <= 0:
            asset.error = f"Invalid size: {asset.size}"
            report.warnings.append(f"{asset.level}/{asset.segment}/asset_{asset.asset_type}: {asset.error}")
            # Still try to continue
        
        # Read asset data
        data = blb_data[asset.absolute_offset:asset.absolute_offset + asset.size]
        
        # Determine output directory (descriptive world/stage/{tileset,map} layout)
        asset_output_dir = output_dir / asset.level / segment_to_relpath(asset.segment)
        
        # Get handler and extract
        handler, handler_name = get_handler(asset.asset_type)
        asset.handler_used = handler_name
        
        try:
            output_files = handler(data, asset, asset_output_dir, context={})
            asset.output_path = str(output_files[0]) if output_files else None
            report.extracted_assets += 1
            report.total_bytes_extracted += asset.size
            # Track coverage
            coverage.append(CoverageRegion(
                start=asset.absolute_offset,
                end=asset.absolute_offset + asset.size,
                description=f"{asset.level}/{asset.segment}/{asset.asset_type}_{asset.asset_name}",
                level=asset.level,
                segment=asset.segment,
                asset_type=asset.asset_type,
            ))
        except Exception as e:
            asset.error = str(e)
            report.errors.append(f"{asset.level}/{asset.segment}/asset_{asset.asset_type}: {e}")
            report.failed_assets += 1
        
        report.assets.append(asdict(asset))
    
    return coverage

def extract_header_data(
    blb_path: Path,
    data: dict,
    output_dir: Path,
    report: ExtractionReport
) -> list[CoverageRegion]:
    """
    Extract header-level data (loading screens from sector table).
    
    Returns list of CoverageRegion for tracking what bytes were extracted.
    """
    coverage = []
    
    with open(blb_path, 'rb') as f:
        blb_data = f.read()
    
    # Add header itself as covered region (first 4KB)
    coverage.append(CoverageRegion(
        start=0,
        end=HEADER_SIZE,
        description="BLB header",
    ))
    
    header = data.get('header', {}).get('header', {})
    sectors = header.get('sectors', [])
    
    # Extract loading screen BS frames
    sectors_dir = output_dir / 'header' / 'sectors'
    sectors_dir.mkdir(parents=True, exist_ok=True)
    
    for i, sector in enumerate(sectors):
        code = decode_string(sector.get('code', f'sector_{i:02d}'))
        sector_offset = parse_hex_offset(sector.get('sector_offset', '0'))
        sector_count = sector.get('sector_count', 0)
        
        if sector_count <= 0:
            continue
        
        byte_offset = sector_offset * SECTOR_SIZE
        byte_size = sector_count * SECTOR_SIZE
        
        if byte_offset + byte_size > len(blb_data):
            report.warnings.append(f"Sector {code} extends beyond file")
            continue
        
        # Extract raw data
        data_bytes = blb_data[byte_offset:byte_offset + byte_size]
        
        bin_path = sectors_dir / f"{i:02d}_{code}.bin"
        manifest_path = sectors_dir / f"{i:02d}_{code}.json"
        
        bin_path.write_bytes(data_bytes)
        
        manifest = {
            "index": i,
            "code": code,
            "sector_offset": hex(sector_offset),
            "sector_count": sector_count,
            "byte_offset": hex(byte_offset),
            "byte_size": byte_size,
            "entry_flags": decode_string(str(sector.get('entry_flags', ''))),
            "display_timeout": sector.get('display_timeout', 0),
        }
        manifest_path.write_text(json.dumps(manifest, indent=2))
        
        report.total_assets += 1
        report.extracted_assets += 1
        report.total_bytes_extracted += byte_size
        
        # Track coverage
        coverage.append(CoverageRegion(
            start=byte_offset,
            end=byte_offset + byte_size,
            description=f"header/sectors/{code}",
        ))
    
    # Extract credits data (BS frames for credits sequence)
    credits = header.get('credits', [])
    credits_dir = output_dir / 'header' / 'credits'
    credits_dir.mkdir(parents=True, exist_ok=True)
    
    for i, credit in enumerate(credits):
        code = decode_string(credit.get('code', f'credit_{i:02d}'))
        sector_offset = credit.get('sector_offset', 0)
        if isinstance(sector_offset, str):
            sector_offset = int(sector_offset, 16)
        sector_count = credit.get('sector_count', 0)
        
        if sector_count <= 0:
            continue
        
        byte_offset = sector_offset * SECTOR_SIZE
        byte_size = sector_count * SECTOR_SIZE
        
        if byte_offset + byte_size > len(blb_data):
            report.warnings.append(f"Credits {code} extends beyond file")
            continue
        
        data_bytes = blb_data[byte_offset:byte_offset + byte_size]
        
        bin_path = credits_dir / f"{i:02d}_{code if code.strip() else 'credits'}.bin"
        manifest_path = credits_dir / f"{i:02d}_{code if code.strip() else 'credits'}.json"
        
        bin_path.write_bytes(data_bytes)
        
        manifest = {
            "index": i,
            "code": code,
            "sector_offset": hex(sector_offset),
            "sector_count": sector_count,
            "byte_offset": hex(byte_offset),
            "byte_size": byte_size,
        }
        manifest_path.write_text(json.dumps(manifest, indent=2))
        
        report.total_assets += 1
        report.extracted_assets += 1
        report.total_bytes_extracted += byte_size
        
        coverage.append(CoverageRegion(
            start=byte_offset,
            end=byte_offset + byte_size,
            description=f"header/credits/{code if code.strip() else 'credits'}",
        ))
    
    # Extract movies data (sector counts from header, actual files are external STR)
    movies = header.get('movies', [])
    movies_dir = output_dir / 'header' / 'movies'
    movies_dir.mkdir(parents=True, exist_ok=True)
    
    for i, movie in enumerate(movies):
        movie_id = decode_string(movie.get('movie_id', f'movie_{i:02d}'))
        filename = decode_string(movie.get('filename', ''))
        sector_count = movie.get('sector_count', 0)
        
        # Movies reference external STR files, so we just save the manifest
        manifest = {
            "index": i,
            "movie_id": movie_id,
            "short_name": decode_string(movie.get('short_name', '')),
            "filename": filename,
            "sector_count": sector_count,
        }
        manifest_path = movies_dir / f"{i:02d}_{movie_id}.json"
        manifest_path.write_text(json.dumps(manifest, indent=2))
    
    # Extract password screens / Mode 6 sector data (BS frames for inter-level transitions)
    # These are at header offset 0xECC-0xF0F, 16 entries × 4 bytes each
    pw_screens = header.get('password_screens', [])
    pw_screens_dir = output_dir / 'header' / 'password_screens'
    pw_screens_dir.mkdir(parents=True, exist_ok=True)
    
    for i, ps in enumerate(pw_screens):
        sector_offset = ps.get('sector_offset', 0)
        if isinstance(sector_offset, str):
            sector_offset = int(sector_offset, 16)
        sector_count = ps.get('sector_count', 0)
        
        if sector_count <= 0 or sector_offset <= 0:
            continue
        
        byte_offset = sector_offset * SECTOR_SIZE
        byte_size = sector_count * SECTOR_SIZE
        
        if byte_offset + byte_size > len(blb_data):
            report.warnings.append(f"Password screen {i} extends beyond file")
            continue
        
        data_bytes = blb_data[byte_offset:byte_offset + byte_size]
        
        bin_path = pw_screens_dir / f"{i:02d}_mode6.bin"
        manifest_path = pw_screens_dir / f"{i:02d}_mode6.json"
        
        bin_path.write_bytes(data_bytes)
        
        manifest = {
            "index": i,
            "sector_offset": hex(sector_offset),
            "sector_count": sector_count,
            "byte_offset": hex(byte_offset),
            "byte_size": byte_size,
            "description": "Mode 6 BS frame data (inter-level transitions)",
        }
        manifest_path.write_text(json.dumps(manifest, indent=2))
        
        report.total_assets += 1
        report.extracted_assets += 1
        report.total_bytes_extracted += byte_size
        
        coverage.append(CoverageRegion(
            start=byte_offset,
            end=byte_offset + byte_size,
            description=f"header/password_screens/{i:02d}_mode6",
        ))
    
    # Extract known unreferenced sectors (unused content discovered in GAME.BLB)
    # See docs/unconfirmed_findings.md for details on these hidden assets
    # NOTE: Credits at sectors 26-196 and 36929-37099 are referenced by the credits
    # table and extracted above - they're "unused" in gameplay but not unreferenced.
    unreferenced_dir = output_dir / 'header' / 'unreferenced'
    unreferenced_dir.mkdir(parents=True, exist_ok=True)
    
    # Known unreferenced sectors (verified 2026-01-06):
    # Only sectors 2-11 are truly unreferenced (not in any header table)
    unreferenced_sectors = [
        # Sectors 2-11: Unused legal/copyright screen (BS/MDEC v2 frame)
        # This is NOT referenced by any table - LEGL uses sector 22 instead
        {"name": "unused_legal_screen", "sector": 2, "count": 10, 
         "description": "Unused legal/copyright screen - replaced by LEGL at sector 22"},
    ]
    
    for entry in unreferenced_sectors:
        byte_offset = entry["sector"] * SECTOR_SIZE
        byte_size = entry["count"] * SECTOR_SIZE
        
        if byte_offset + byte_size > len(blb_data):
            report.warnings.append(f"Unreferenced {entry['name']} extends beyond file")
            continue
        
        data_bytes = blb_data[byte_offset:byte_offset + byte_size]
        
        bin_path = unreferenced_dir / f"{entry['name']}.bin"
        manifest_path = unreferenced_dir / f"{entry['name']}.json"
        
        bin_path.write_bytes(data_bytes)
        
        manifest = {
            "name": entry["name"],
            "sector_offset": entry["sector"],
            "sector_count": entry["count"],
            "byte_offset": hex(byte_offset),
            "byte_size": byte_size,
            "description": entry["description"],
        }
        manifest_path.write_text(json.dumps(manifest, indent=2))
        
        report.total_assets += 1
        report.extracted_assets += 1
        report.total_bytes_extracted += byte_size
        
        coverage.append(CoverageRegion(
            start=byte_offset,
            end=byte_offset + byte_size,
            description=f"header/unreferenced/{entry['name']}",
        ))
    
    return coverage

# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description='Extract assets from Skullmonkeys GAME.BLB file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('--json', '-j', required=True, type=Path,
                        help='Path to ImHex JSON output')
    parser.add_argument('--blb', '-b', required=True, type=Path,
                        help='Path to GAME.BLB file')
    parser.add_argument('--output', '-o', type=Path, default=Path('extracted'),
                        help='Output directory (default: extracted/)')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Verbose output')
    
    args = parser.parse_args()
    
    # Validate inputs
    if not args.json.exists():
        print(f"Error: JSON file not found: {args.json}", file=sys.stderr)
        sys.exit(1)
    
    if not args.blb.exists():
        print(f"Error: BLB file not found: {args.blb}", file=sys.stderr)
        sys.exit(1)
    
    # Load JSON
    print(f"Loading JSON from {args.json}...")
    with open(args.json) as f:
        data = json.load(f)
    
    # Create output directory
    args.output.mkdir(parents=True, exist_ok=True)
    
    # Initialize report
    blb_size = args.blb.stat().st_size
    report = ExtractionReport(
        blb_file=str(args.blb),
        json_file=str(args.json),
        output_dir=str(args.output),
        blb_size=blb_size,
    )
    
    # Track all coverage regions
    all_coverage: list[CoverageRegion] = []
    
    # Discover assets
    print("Discovering assets...")
    assets = discover_assets(data)
    print(f"  Found {len(assets)} assets in level data")
    
    # Discover TOC coverage (segment headers)
    toc_coverage = discover_toc_coverage(data)
    all_coverage.extend(toc_coverage)
    print(f"  Found {len(toc_coverage)} TOC regions")
    
    # Extract segment metadata as JSON
    print("Extracting segment metadata...")
    metadata_count = extract_segment_metadata(data, args.output)
    print(f"  Created {metadata_count} metadata files")
    
    # Extract header data (loading screens)
    print("Extracting header data...")
    header_coverage = extract_header_data(args.blb, data, args.output, report)
    all_coverage.extend(header_coverage)
    
    # Extract assets
    print("Extracting assets...")
    asset_coverage = extract_assets(args.blb, assets, args.output, report)
    all_coverage.extend(asset_coverage)
    
    # Read BLB data for gap analysis
    print("Analyzing coverage...")
    with open(args.blb, 'rb') as f:
        blb_data = f.read()
    
    gaps, overlaps, coverage_pct, padding_bytes, unknown_bytes, unique_bytes = compute_coverage_gaps(all_coverage, blb_size, blb_data)
    report.coverage_percent = coverage_pct
    report.padding_bytes = padding_bytes
    report.unknown_data_bytes = unknown_bytes
    report.unique_bytes_extracted = unique_bytes
    report.overlap_bytes = sum(o.size for o in overlaps)
    report.overlaps = [asdict(o) for o in overlaps]
    report.gaps = [asdict(g) for g in gaps]
    report.largest_gaps = [asdict(g) for g in sorted(gaps, key=lambda g: g.size, reverse=True)[:20]]
    
    # Write report
    report_path = args.output / 'extraction_report.json'
    with open(report_path, 'w') as f:
        json.dump(asdict(report), f, indent=2)
    
    # Summary
    print()
    print("=" * 60)
    print("EXTRACTION COMPLETE")
    print("=" * 60)
    print(f"  Total assets:     {report.total_assets}")
    print(f"  Extracted:        {report.extracted_assets}")
    print(f"  Failed:           {report.failed_assets}")
    print(f"  Bytes extracted:  {report.total_bytes_extracted:,} (raw)")
    print(f"  Unique bytes:     {unique_bytes:,} (deduplicated)")
    print(f"  Overlap bytes:    {report.overlap_bytes:,} (extracted multiple times)")
    print(f"  BLB file size:    {blb_size:,}")
    print(f"  Coverage:         {coverage_pct:.1f}%")
    print()
    print("  Unaccounted bytes breakdown:")
    print(f"    Sector padding: {padding_bytes:>12,} bytes (CD-ROM alignment)")
    print(f"    Unknown data:   {unknown_bytes:>12,} bytes (non-zero, possibly missed)")
    print(f"    Total:          {padding_bytes + unknown_bytes:>12,} bytes")
    print()
    print(f"  Output directory: {args.output}")
    print(f"  Report:           {report_path}")
    
    # Show overlaps (these are potential issues)
    if overlaps:
        print()
        print(f"⚠️  OVERLAPPING EXTRACTIONS ({len(overlaps)} regions, {report.overlap_bytes:,} bytes):")
        for o in sorted(overlaps, key=lambda o: o.size, reverse=True)[:10]:
            print(f"  0x{o.start:08X} - 0x{o.end:08X}: {o.size:>8,} bytes")
            print(f"    Region 1: {o.region1}")
            print(f"    Region 2: {o.region2}")
        if len(overlaps) > 10:
            print(f"  ... and {len(overlaps) - 10} more overlaps")
    
    # Show largest non-padding gaps (unknown data)
    unknown_gaps = [g for g in gaps if not g.is_padding]
    if unknown_gaps:
        print()
        print(f"UNKNOWN DATA REGIONS ({len(unknown_gaps)} regions with non-zero data):")
        for g in sorted(unknown_gaps, key=lambda g: g.size, reverse=True)[:10]:
            print(f"  0x{g.start:08X} - 0x{g.end:08X}: {g.size:>10,} bytes  ({g.description})")
    
    if report.warnings:
        print()
        print(f"WARNINGS ({len(report.warnings)}):")
        for w in report.warnings[:10]:
            print(f"  - {w}")
        if len(report.warnings) > 10:
            print(f"  ... and {len(report.warnings) - 10} more")
    
    if report.errors:
        print()
        print(f"ERRORS ({len(report.errors)}):")
        for e in report.errors[:10]:
            print(f"  - {e}")
        if len(report.errors) > 10:
            print(f"  ... and {len(report.errors) - 10} more")
    
    sys.exit(0 if report.failed_assets == 0 else 1)

if __name__ == '__main__':
    main()
