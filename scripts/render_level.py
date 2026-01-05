#!/usr/bin/env python3
"""
render_level.py - Render Skullmonkeys level to PNG images

Usage:
    python3 scripts/render_level.py [level_index] [output_dir]
    python3 scripts/render_level.py --all                      # Render all levels
    
Examples:
    python3 scripts/render_level.py 1              # PHRO (Skullmonkey Gate)
    python3 scripts/render_level.py 2              # SCIE (Science Center)
    python3 scripts/render_level.py 2 output/scie  # Custom output directory
    python3 scripts/render_level.py --all          # Render all levels, all stages
    
Level indices:
    0 = MENU (Options)
    1 = PHRO (Skullmonkey Gate) - first gameplay level
    2 = SCIE (Science Center)
    3 = TMPL (Monkey Shrines)
    ... (see blb.py for full list)

Stage Pairing:
    Each stage's tertiary block uses tiles from the secondary that PRECEDES it:
    - Stage 0 uses base secondary
    - Stage 1 uses stage 0's secondary sub-block
    - Stage N uses stage (N-1)'s secondary sub-block
"""

import sys
import os
import struct
from pathlib import Path

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from blb import (
    BLBFile, 
    parse_container_toc,
    parse_palette_container,
    parse_layer_entries,
    parse_tilemap_container,
    parse_tilemap,
    get_tile_index,
    extract_tiles_rgba,
    TileHeader,
)
from PIL import Image


def render_stage(tiles: list, tert_assets: dict, level_id: str, stage_idx: int, 
                 output_dir: str, max_width: int = 8192) -> Image:
    """
    Render a single stage to PNG images.
    
    Args:
        tiles: List of tile images (from extract_tiles_rgba)
        tert_assets: Dict of tertiary asset data {asset_id: bytes}
        level_id: Level ID string (e.g., "PHRO")
        stage_idx: Stage index within the level
        output_dir: Output directory path
        max_width: Maximum canvas width
        
    Returns:
        PIL Image of the rendered stage
    """
    # Parse tertiary assets
    tert_header_data = tert_assets.get(100)
    if not tert_header_data or len(tert_header_data) < 12:
        return None
    
    level_width = struct.unpack_from('<H', tert_header_data, 0x08)[0]
    level_height = struct.unpack_from('<H', tert_header_data, 0x0A)[0]
    bg_r, bg_g, bg_b = tert_header_data[0], tert_header_data[1], tert_header_data[2]
    
    # Parse layers and tilemaps
    layer_data = tert_assets.get(201)
    tilemap_data = tert_assets.get(200)
    
    if not layer_data or not tilemap_data:
        return None
    
    layer_entries = parse_layer_entries(layer_data)
    tilemap_data_list = parse_tilemap_container(tilemap_data)
    tilemaps = [parse_tilemap(tm) for tm in tilemap_data_list]
    
    # Create canvas
    canvas_width = min(level_width * 16, max_width)
    canvas_height = level_height * 16
    
    if canvas_width <= 0 or canvas_height <= 0:
        return None
    
    canvas = Image.new('RGBA', (canvas_width, canvas_height), (bg_r, bg_g, bg_b, 255))
    
    # Render layers (back to front)
    for layer, tilemap in zip(layer_entries, tilemaps):
        width = layer.width
        height = layer.height
        x_offset = layer.x_offset
        y_offset = layer.y_offset
        
        for y in range(height):
            for x in range(width):
                idx = y * width + x
                if idx >= len(tilemap):
                    continue
                
                tile_val = tilemap[idx]
                if tile_val == 0:
                    continue
                
                tile_idx = get_tile_index(tile_val)
                
                if tile_idx < 0 or tile_idx >= len(tiles):
                    continue
                
                tile_img = tiles[tile_idx]
                
                dest_x = (x_offset + x) * 16
                dest_y = (y_offset + y) * 16
                
                if 0 <= dest_x < canvas.width - 16 and 0 <= dest_y < canvas.height - 16:
                    canvas.paste(tile_img, (dest_x, dest_y), tile_img)
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate filename with stage suffix if not stage 0
    suffix = f"_s{stage_idx}" if stage_idx > 0 else ""
    
    # Save composite
    composite_path = f'{output_dir}/{level_id}{suffix}_composite.png'
    canvas.save(composite_path)
    print(f"    Saved: {composite_path} ({canvas.width}x{canvas.height})")
    
    # Save overview (scaled down)
    scale = max(1, canvas.width // 2048)
    thumb = canvas.resize((canvas.width // scale, canvas.height // scale), Image.NEAREST)
    overview_path = f'{output_dir}/{level_id}{suffix}_overview.png'
    thumb.save(overview_path)
    
    return canvas


def extract_tiles_from_secondary(sec_data: bytes) -> tuple:
    """
    Extract tiles from a secondary data block.
    
    Returns:
        (tiles, count_16x16, count_8x8) or (None, 0, 0) on error
    """
    sec_entries = parse_container_toc(sec_data)
    sec_assets = {e.asset_id: e.data for e in sec_entries if e.data}
    
    required = [100, 300, 301, 400]
    if not all(a in sec_assets for a in required):
        return None, 0, 0
    
    tile_header = TileHeader.from_bytes(sec_assets[100])
    count_16x16 = tile_header.count_16x16
    count_8x8 = tile_header.count_8x8_a
    
    pixel_data = sec_assets[300]
    pal_indices = sec_assets[301]
    flags_data = sec_assets.get(302, b'')
    palettes = parse_palette_container(sec_assets[400])
    
    tiles = extract_tiles_rgba(pixel_data, pal_indices, palettes, flags_data,
                               count_16x16, count_8x8, scale_8x8=True)
    
    return tiles, count_16x16, count_8x8


def render_level(blb: BLBFile, level_idx: int, output_dir: str, max_width: int = 8192):
    """
    Render all stages of a level to PNG images.
    
    Creates for each stage:
        - {level_id}[_sN]_composite.png: Full stage render
        - {level_id}[_sN]_overview.png: Scaled-down overview
    """
    entry = blb.header.level_entries[level_idx]
    level_id = entry.level_id
    
    print(f"Rendering Level {level_idx}: {entry.name} ({level_id})")
    
    f = blb._file
    
    # Read base secondary data (tiles shared across stages)
    f.seek(entry.secondary_byte_offset)
    base_sec_data = f.read(entry.secondary_byte_size)
    
    # Build per-stage secondary data mapping
    # Pattern: Stage 0 uses base, Stage N uses stage (N-1)'s secondary sub-block
    stage_secondaries = [base_sec_data]
    for i in range(5):
        sub_count = entry.sec_sub_counts[i] if entry.sec_sub_counts else 0
        if sub_count > 0:
            sub_offset = entry.sec_sub_offsets[i] * 2048  # Sector to byte
            sub_size = sub_count * 2048
            f.seek(sub_offset)
            stage_secondaries.append(f.read(sub_size))
        else:
            # Inherit from previous
            stage_secondaries.append(stage_secondaries[-1] if stage_secondaries else base_sec_data)
    
    stages_rendered = 0
    
    # Render EACH STAGE (tertiary sub-blocks)
    for stage_idx in range(6):
        tert_size = entry.get_tert_sub_byte_size(stage_idx)
        if tert_size == 0:
            continue
        
        print(f"  Stage {stage_idx}:")
        
        # Read tertiary data for this stage
        f.seek(entry.get_tert_sub_byte_offset(stage_idx))
        tert_data = f.read(tert_size)
        tert_entries = parse_container_toc(tert_data)
        tert_assets = {e.asset_id: e.data for e in tert_entries if e.data}
        
        # Validate required tertiary assets
        if 100 not in tert_assets or 200 not in tert_assets or 201 not in tert_assets:
            print(f"    Skipping: Missing required tertiary assets")
            continue
        
        # Get secondary for this stage (uses the one before it in sector layout)
        sec_data = stage_secondaries[stage_idx]
        
        # Extract tiles from the appropriate secondary
        tiles, count_16x16, count_8x8 = extract_tiles_from_secondary(sec_data)
        if tiles is None:
            print(f"    Skipping: Failed to extract tiles from secondary")
            continue
        
        total_tiles = count_16x16 + count_8x8
        print(f"    Tiles: {total_tiles} ({count_16x16} 16x16 + {count_8x8} 8x8)")
        
        # Render this stage
        canvas = render_stage(tiles, tert_assets, level_id, stage_idx, output_dir, max_width)
        if canvas:
            stages_rendered += 1
    
    print(f"  Rendered {stages_rendered} stage(s)")
    return stages_rendered


def main():
    import argparse
    
    parser = argparse.ArgumentParser(
        description='Render Skullmonkeys level(s) to PNG',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    python3 scripts/render_level.py 1              # PHRO (Skullmonkey Gate)
    python3 scripts/render_level.py 3              # TMPL (Monkey Shrines), all stages
    python3 scripts/render_level.py --all          # Render ALL levels
    python3 scripts/render_level.py --list         # List available levels
""")
    parser.add_argument('level_index', type=int, nargs='?', default=None,
                        help='Level index (0=MENU, 1=PHRO, 2=SCIE, ...)')
    parser.add_argument('output_dir', nargs='?', default='extracted_memory/level_renders',
                        help='Output directory')
    parser.add_argument('--blb', default='disks/blb/GAME.BLB',
                        help='Path to GAME.BLB file')
    parser.add_argument('--list', action='store_true',
                        help='List all levels and exit')
    parser.add_argument('--all', action='store_true',
                        help='Render all levels')
    
    args = parser.parse_args()
    
    blb_path = Path(args.blb)
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}")
        sys.exit(1)
    
    with BLBFile(blb_path) as blb:
        if args.list:
            print("Available levels:")
            for i, entry in enumerate(blb.header.level_entries):
                tert_count = sum(1 for j in range(6) if entry.get_tert_sub_byte_size(j) > 0)
                print(f"  {i:2}: {entry.level_id:4} - {entry.name:20} ({tert_count} stages)")
            return
        
        if args.all:
            # Render all levels
            total_stages = 0
            for level_idx in range(len(blb.header.level_entries)):
                stages = render_level(blb, level_idx, args.output_dir)
                total_stages += stages
            print(f"\nTotal: Rendered {total_stages} stages across {len(blb.header.level_entries)} levels")
            return
        
        # Default to level 1 if no index provided
        level_index = args.level_index if args.level_index is not None else 1
        
        if level_index < 0 or level_index >= len(blb.header.level_entries):
            print(f"Error: Invalid level index {level_index}")
            print(f"Valid range: 0-{len(blb.header.level_entries) - 1}")
            sys.exit(1)
        
        render_level(blb, level_index, args.output_dir)


if __name__ == '__main__':
    main()