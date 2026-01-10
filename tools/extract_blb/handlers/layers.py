"""
Stage Layer Handler (Asset Types 200, 201)

Extracts stage layers to:
1. Raw bytes (.bin) via default handler  
2. Layer metadata as JSON
3. Rendered layer images as PNG (when tile/palette data available in context)

Layer System Overview:
- Asset 200: Tilemap Container - u16 tile indices for each layer
- Asset 201: Layer Entries - 92-byte metadata per layer (dimensions, scroll, colors)
- Asset 300: Tile Pixels - 8bpp indexed tile graphics
- Asset 301: Palette Indices - which palette per tile
- Asset 302: Tile Flags - size (8x8/16x16), transparency, skip flags
- Asset 400: Palette Container - 256-color palettes

Tilemap Entry Format (u16):
- Bits 0-10: Tile index (0 = transparent, 1-2047 = tile)
- Bits 11: Usually 0
- Bits 12-15: Color tint selector (indexes into layer's color_tints[16])
"""

import json
import struct
from pathlib import Path

from . import register_handler, default_handler


def parse_layer_entry(data: bytes, offset: int) -> dict:
    """
    Parse 92-byte layer entry.
    
    Offsets verified from blb.hexpat LayerEntry struct.
    """
    if offset + 92 > len(data):
        return {}
    
    # Basic fields (0x00 - 0x0B)
    x_offset, y_offset = struct.unpack_from('<HH', data, offset + 0x00)
    width, height = struct.unpack_from('<HH', data, offset + 0x04)
    level_width, level_height = struct.unpack_from('<HH', data, offset + 0x08)
    
    # Render param (0x0C - 0x0F)
    render_param = struct.unpack_from('<I', data, offset + 0x0C)[0]
    
    # Scroll factors (0x10 - 0x17) - fixed point 16.16
    scroll_x, scroll_y = struct.unpack_from('<II', data, offset + 0x10)
    
    # Render fields (0x18 - 0x1B)
    render_30, render_32 = struct.unpack_from('<HH', data, offset + 0x18)
    
    # More render fields (0x1C - 0x1D)
    render_3a, render_3b = struct.unpack_from('<BB', data, offset + 0x1C)
    
    # Scroll enable flags (0x1E - 0x21)
    scroll_left, scroll_right, scroll_up, scroll_down = struct.unpack_from('<BBBB', data, offset + 0x1E)
    
    # Render modes (0x22 - 0x25)
    render_mode_h, render_mode_v = struct.unpack_from('<HH', data, offset + 0x22)
    
    # Layer type and skip (0x26 - 0x29)
    layer_type = data[offset + 0x26]
    skip_render = struct.unpack_from('<H', data, offset + 0x28)[0]
    
    # Color tints (0x2C - 0x5B): 16 RGB entries = 48 bytes
    color_tints = []
    for i in range(16):
        tint_offset = offset + 0x2C + i * 3
        if tint_offset + 3 <= len(data):
            r, g, b = data[tint_offset], data[tint_offset + 1], data[tint_offset + 2]
            color_tints.append({'r': r, 'g': g, 'b': b})
    
    return {
        'x_offset': x_offset,
        'y_offset': y_offset,
        'width': width,
        'height': height,
        'level_width': level_width,
        'level_height': level_height,
        'render_param': render_param,
        'scroll_x': scroll_x / 65536.0,  # Convert from 16.16 fixed point
        'scroll_y': scroll_y / 65536.0,
        'scroll_enable': {
            'left': scroll_left != 0,
            'right': scroll_right != 0,
            'up': scroll_up != 0,
            'down': scroll_down != 0,
        },
        'render_mode_h': render_mode_h,
        'render_mode_v': render_mode_v,
        'layer_type': layer_type,
        'skip_render': skip_render != 0,
        'color_tints': color_tints,
    }


def parse_tilemap_container(data: bytes) -> list[dict]:
    """
    Parse tilemap container (Asset 200).
    Returns list of tilemap entries with layer_id, size, offset.
    """
    if len(data) < 4:
        return []
    
    layer_count = struct.unpack_from('<I', data, 0)[0]
    
    if layer_count > 20 or layer_count * 12 + 4 > len(data):
        return []
    
    tilemaps = []
    for i in range(layer_count):
        entry_offset = 4 + i * 12
        layer_id, size, offset = struct.unpack_from('<III', data, entry_offset)
        tilemaps.append({
            'layer_id': layer_id,
            'size': size,
            'offset': offset,
            'tile_count': size // 2,  # Each tile is 2 bytes
        })
    
    return tilemaps


def extract_tilemap_data(data: bytes, tilemap_info: dict) -> list[int]:
    """
    Extract tilemap tile indices from container.
    Returns list of u16 values (tile indices with color tint bits).
    """
    offset = tilemap_info['offset']
    tile_count = tilemap_info['tile_count']
    
    if offset + tile_count * 2 > len(data):
        return []
    
    tiles = []
    for i in range(tile_count):
        tile = struct.unpack_from('<H', data, offset + i * 2)[0]
        tiles.append(tile)
    
    return tiles


def decode_tilemap_entry(tile_value: int) -> dict:
    """
    Decode a single tilemap u16 entry.
    
    Format:
    - Bits 0-10: Tile index (0 = transparent, 1-2047 = tile)
    - Bits 12-15: Color tint selector (0-15)
    """
    return {
        'tile_index': tile_value & 0x7FF,  # Bits 0-10
        'color_tint': (tile_value >> 12) & 0x0F,  # Bits 12-15
    }


@register_handler(200)
def tilemap_container_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 200 (tilemap container).
    
    Extracts:
    1. Raw bytes (via default handler)
    2. Tilemap metadata as JSON
    3. Individual tilemaps as binary and decoded JSON
    """
    # First, run default handler
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create tilemaps subdirectory
    tilemaps_dir = output_dir / "tilemaps"
    tilemaps_dir.mkdir(parents=True, exist_ok=True)
    
    # Parse tilemap container
    tilemaps = parse_tilemap_container(data)
    
    if not tilemaps:
        return output_files
    
    # Extract each tilemap
    for i, tm_info in enumerate(tilemaps):
        # Extract raw tilemap data
        tiles = extract_tilemap_data(data, tm_info)
        
        if not tiles:
            continue
        
        # Save tilemap as raw u16 array
        raw_path = tilemaps_dir / f"layer_{i:02d}.bin"
        raw_data = struct.pack(f'<{len(tiles)}H', *tiles)
        raw_path.write_bytes(raw_data)
        output_files.append(raw_path)
        
        # Decode tile entries for analysis
        decoded_tiles = [decode_tilemap_entry(t) for t in tiles]
        
        # Count unique tiles and color tints used
        unique_tiles = set(t['tile_index'] for t in decoded_tiles if t['tile_index'] > 0)
        color_tints_used = set(t['color_tint'] for t in decoded_tiles)
        
        # Save metadata
        meta_path = tilemaps_dir / f"layer_{i:02d}.json"
        meta = {
            'layer_id': tm_info['layer_id'],
            'tile_count': tm_info['tile_count'],
            'unique_tiles': len(unique_tiles),
            'color_tints_used': sorted(list(color_tints_used)),
            'size_bytes': tm_info['size'],
        }
        meta_path.write_text(json.dumps(meta, indent=2))
        output_files.append(meta_path)
    
    # Write summary
    summary_path = tilemaps_dir / "_summary.json"
    summary = {
        'layer_count': len(tilemaps),
        'tilemaps': tilemaps,
        'level': asset_info.level,
        'segment': asset_info.segment,
    }
    summary_path.write_text(json.dumps(summary, indent=2))
    output_files.append(summary_path)
    
    return output_files


@register_handler(201)
def layer_entries_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 201 (layer entries).
    
    Extracts:
    1. Raw bytes (via default handler)
    2. Layer metadata as JSON with all parsed fields
    """
    # First, run default handler
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create layers subdirectory
    layers_dir = output_dir / "layers"
    layers_dir.mkdir(parents=True, exist_ok=True)
    
    # Calculate layer count (92 bytes per layer)
    layer_count = len(data) // 92
    
    if layer_count == 0:
        return output_files
    
    layers = []
    
    for i in range(layer_count):
        layer = parse_layer_entry(data, i * 92)
        layer['index'] = i
        layers.append(layer)
        
        # Save individual layer metadata
        layer_path = layers_dir / f"layer_{i:02d}.json"
        layer_path.write_text(json.dumps(layer, indent=2))
        output_files.append(layer_path)
    
    # Write summary
    summary_path = layers_dir / "_summary.json"
    summary = {
        'layer_count': layer_count,
        'level': asset_info.level,
        'segment': asset_info.segment,
        'layers': [
            {
                'index': l['index'],
                'size': f"{l['width']}x{l['height']}",
                'scroll': f"({l['scroll_x']:.2f}, {l['scroll_y']:.2f})",
                'type': l['layer_type'],
                'skip': l['skip_render'],
            }
            for l in layers
        ],
    }
    summary_path.write_text(json.dumps(summary, indent=2))
    output_files.append(summary_path)
    
    return output_files


@register_handler(300)
def tile_pixels_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 300 (tile pixels).
    
    Extracts raw tile pixel data. For full tile rendering,
    also need Asset 301 (palette indices), 302 (flags), and 400 (palettes).
    """
    # Run default handler - tile pixels are just raw 8bpp indexed data
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create tiles subdirectory with metadata
    tiles_dir = output_dir / "tiles"
    tiles_dir.mkdir(parents=True, exist_ok=True)
    
    # Calculate tile count based on size
    # Most tiles are 16x16 = 256 bytes, but some are 8x8 = 64 bytes
    # Without Asset 302 (tile flags), assume all 16x16
    tile_count_16x16 = len(data) // 256
    tile_count_8x8 = len(data) // 64
    
    # Write metadata
    meta_path = tiles_dir / "_tile_info.json"
    meta = {
        'total_bytes': len(data),
        'estimated_16x16_tiles': tile_count_16x16,
        'estimated_8x8_tiles': tile_count_8x8,
        'level': asset_info.level,
        'segment': asset_info.segment,
        'note': 'Use Asset 302 (tile_flags) to determine actual tile sizes',
    }
    meta_path.write_text(json.dumps(meta, indent=2))
    output_files.append(meta_path)
    
    return output_files


@register_handler(302)
def tile_flags_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 302 (tile flags).
    
    Each byte contains:
    - Bit 0: Semi-transparent (alpha blending)
    - Bit 1: 8x8 tile (instead of 16x16)
    - Bit 2: Skip (don't load/render)
    """
    # Run default handler
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create tile_flags subdirectory
    flags_dir = output_dir / "tile_flags"
    flags_dir.mkdir(parents=True, exist_ok=True)
    
    # Analyze flags
    tile_count = len(data)
    semi_transparent = 0
    tiles_8x8 = 0
    tiles_16x16 = 0
    skipped = 0
    
    for flag_byte in data:
        if flag_byte & 0x01:
            semi_transparent += 1
        if flag_byte & 0x02:
            tiles_8x8 += 1
        else:
            tiles_16x16 += 1
        if flag_byte & 0x04:
            skipped += 1
    
    # Write analysis
    analysis_path = flags_dir / "_analysis.json"
    analysis = {
        'tile_count': tile_count,
        'tiles_16x16': tiles_16x16,
        'tiles_8x8': tiles_8x8,
        'semi_transparent': semi_transparent,
        'skipped': skipped,
        'level': asset_info.level,
        'segment': asset_info.segment,
    }
    analysis_path.write_text(json.dumps(analysis, indent=2))
    output_files.append(analysis_path)
    
    return output_files
