#!/usr/bin/env python3
"""
render_level.py - Render Skullmonkeys level to PNG images

Usage:
    python3 scripts/render_level.py [level_index] [output_dir]
    
Examples:
    python3 scripts/render_level.py 1              # PHRO (Skullmonkey Gate)
    python3 scripts/render_level.py 2              # SCIE (Science Center)
    python3 scripts/render_level.py 2 output/scie  # Custom output directory
    
Level indices:
    0 = MENU (Options)
    1 = PHRO (Skullmonkey Gate) - first gameplay level
    2 = SCIE (Science Center)
    3 = TMPL (Monkey Shrines)
    ... (see blb.py for full list)
"""

import sys
import os
import struct
from pathlib import Path

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile
from PIL import Image


def parse_toc(data: bytes) -> dict:
    """Parse a TOC (Table of Contents) from a data segment."""
    count = struct.unpack_from('<I', data, 0)[0]
    assets = {}
    for i in range(count):
        entry_off = 4 + i * 12
        asset_id, asset_size, data_offset = struct.unpack_from('<III', data, entry_off)
        if asset_size > 0 and data_offset + asset_size <= len(data):
            assets[asset_id] = data[data_offset:data_offset + asset_size]
    return assets


def parse_palettes(palette_container: bytes) -> list:
    """Parse a palette container (Asset 0x190) into a list of 256-color palettes."""
    count = struct.unpack_from('<I', palette_container, 0)[0]
    palettes = []
    
    for i in range(count):
        entry_off = 4 + i * 12
        pal_idx, pal_size, pal_offset = struct.unpack_from('<III', palette_container, entry_off)
        pal_data = palette_container[pal_offset:pal_offset + pal_size]
        
        palette = []
        for j in range(256):
            if j * 2 + 2 <= len(pal_data):
                color = struct.unpack_from('<H', pal_data, j * 2)[0]
                r = ((color & 0x1F) * 255) // 31
                g = (((color >> 5) & 0x1F) * 255) // 31
                b = (((color >> 10) & 0x1F) * 255) // 31
                # Color 0 is transparent unless STP bit is set
                a = 255 if j > 0 or (color & 0x8000) else 0
                palette.append((r, g, b, a))
            else:
                palette.append((0, 0, 0, 0))
        palettes.append(palette)
    
    return palettes


def extract_tiles(pixel_data: bytes, pal_indices: list, palettes: list,
                  count_16x16: int, count_8x8: int) -> list:
    """Extract tiles as RGBA PIL Images."""
    tiles = []
    
    # Extract 16x16 tiles (256 bytes each)
    for i in range(count_16x16):
        tile_offset = i * 256
        tile_img = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
        pal_idx = pal_indices[i] if i < len(pal_indices) else 0
        palette = palettes[pal_idx] if pal_idx < len(palettes) else palettes[0]
        
        for y in range(16):
            for x in range(16):
                pix_off = tile_offset + y * 16 + x
                if pix_off < len(pixel_data):
                    color_idx = pixel_data[pix_off]
                    tile_img.putpixel((x, y), palette[color_idx])
        tiles.append(tile_img)
    
    # Extract 8x8 tiles (128 bytes each - 8 rows x 16 bytes, only first 8 used)
    for i in range(count_8x8):
        tile_offset = count_16x16 * 256 + i * 128
        tile_img = Image.new('RGBA', (8, 8), (0, 0, 0, 0))
        pal_idx = pal_indices[count_16x16 + i] if (count_16x16 + i) < len(pal_indices) else 0
        palette = palettes[pal_idx] if pal_idx < len(palettes) else palettes[0]
        
        for y in range(8):
            for x in range(8):
                pix_off = tile_offset + y * 16 + x
                if pix_off < len(pixel_data):
                    color_idx = pixel_data[pix_off]
                    tile_img.putpixel((x, y), palette[color_idx])
        tiles.append(tile_img)
    
    return tiles


def parse_layer_entries(asset201: bytes) -> list:
    """Parse layer entries (92 bytes each) from Asset 0xC9."""
    layers = []
    num_layers = len(asset201) // 92
    
    for i in range(num_layers):
        off = i * 92
        layer = {
            'x_offset': struct.unpack_from('<H', asset201, off + 0x00)[0],
            'y_offset': struct.unpack_from('<H', asset201, off + 0x02)[0],
            'width': struct.unpack_from('<H', asset201, off + 0x04)[0],
            'height': struct.unpack_from('<H', asset201, off + 0x06)[0],
            'scroll_x': struct.unpack_from('<I', asset201, off + 0x10)[0],
            'scroll_y': struct.unpack_from('<I', asset201, off + 0x14)[0],
        }
        layers.append(layer)
    
    return layers


def parse_tilemaps(asset200: bytes) -> list:
    """Parse tilemap data from Asset 0xC8 (sub-TOC format)."""
    count = struct.unpack_from('<I', asset200, 0)[0]
    tilemaps = []
    
    for i in range(count):
        entry_off = 4 + i * 12
        layer_idx, data_size, data_offset = struct.unpack_from('<III', asset200, entry_off)
        tilemap_data = asset200[data_offset:data_offset + data_size]
        num_tiles = data_size // 2
        tilemap = [struct.unpack_from('<H', tilemap_data, j * 2)[0] for j in range(num_tiles)]
        tilemaps.append(tilemap)
    
    return tilemaps


def render_level(blb: BLBFile, level_idx: int, output_dir: str, max_width: int = 8192):
    """
    Render a level to PNG images.
    
    Creates:
        - {level_id}_composite.png: Full level render
        - {level_id}_overview.png: Scaled-down overview
        - {level_id}_viewport.png: 2x zoomed viewport of interesting area
    """
    entry = blb.header.level_entries[level_idx]
    level_id = entry.level_id
    
    print(f"Rendering Level {level_idx}: {entry.name} ({level_id})")
    
    f = blb._file
    
    # Read secondary data (tiles)
    f.seek(entry.secondary_byte_offset)
    sec_data = f.read(entry.secondary_byte_size)
    sec_assets = parse_toc(sec_data)
    
    # Read tertiary block 0 (layers, tilemaps)
    tert_offset = entry.get_tert_sub_byte_offset(0)
    tert_size = entry.get_tert_sub_byte_size(0)
    if tert_size == 0:
        print("  Error: No tertiary data!")
        return None
    
    f.seek(tert_offset)
    tert_data = f.read(tert_size)
    tert_assets = parse_toc(tert_data)
    
    # Validate required assets
    required_sec = [0x64, 0x12C, 0x12D, 0x190]
    required_tert = [0x64, 0xC8, 0xC9]
    
    for asset_id in required_sec:
        if asset_id not in sec_assets:
            print(f"  Error: Missing secondary asset 0x{asset_id:X}")
            return None
    
    for asset_id in required_tert:
        if asset_id not in tert_assets:
            print(f"  Error: Missing tertiary asset 0x{asset_id:X}")
            return None
    
    # Parse secondary assets
    sec_asset100 = sec_assets[0x64]
    count_16x16 = struct.unpack_from('<H', sec_asset100, 0x10)[0]
    count_8x8 = struct.unpack_from('<H', sec_asset100, 0x12)[0]
    total_tiles = count_16x16 + count_8x8
    
    pixel_data = sec_assets[0x12C]
    pal_indices = list(sec_assets[0x12D])
    palettes = parse_palettes(sec_assets[0x190])
    tiles = extract_tiles(pixel_data, pal_indices, palettes, count_16x16, count_8x8)
    
    # Parse tertiary assets
    tert_asset100 = tert_assets[0x64]
    level_width = struct.unpack_from('<H', tert_asset100, 0x08)[0]
    level_height = struct.unpack_from('<H', tert_asset100, 0x0A)[0]
    bg_r, bg_g, bg_b = tert_asset100[0], tert_asset100[1], tert_asset100[2]
    
    layer_entries = parse_layer_entries(tert_assets[0xC9])
    tilemaps = parse_tilemaps(tert_assets[0xC8])
    
    print(f"  Level size: {level_width}x{level_height} tiles ({level_width * 16}x{level_height * 16} pixels)")
    print(f"  Tiles: {total_tiles} ({count_16x16} 16x16 + {count_8x8} 8x8)")
    print(f"  Layers: {len(layer_entries)}")
    print(f"  Background: RGB({bg_r}, {bg_g}, {bg_b})")
    
    # Create canvas
    canvas_width = min(level_width * 16, max_width)
    canvas_height = level_height * 16
    canvas = Image.new('RGBA', (canvas_width, canvas_height), (bg_r, bg_g, bg_b, 255))
    
    # Render layers (back to front)
    for layer, tilemap in zip(layer_entries, tilemaps):
        width = layer['width']
        height = layer['height']
        x_offset = layer['x_offset']
        y_offset = layer['y_offset']
        
        for y in range(height):
            for x in range(width):
                idx = y * width + x
                if idx >= len(tilemap):
                    continue
                
                tile_val = tilemap[idx]
                if tile_val == 0:
                    continue
                
                # Extract tile index (lower 11 bits, 1-based, matching extract_all_graphics.py)
                tile_idx = (tile_val & 0x7FF) - 1
                # Note: Bits 11-15 are unused (tiles are not flipped in game)
                
                if tile_idx < 0 or tile_idx >= len(tiles):
                    continue
                
                tile_img = tiles[tile_idx]
                
                dest_x = (x_offset + x) * 16
                dest_y = (y_offset + y) * 16
                
                if 0 <= dest_x < canvas.width - 16 and 0 <= dest_y < canvas.height - 16:
                    canvas.paste(tile_img, (dest_x, dest_y), tile_img)
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Save composite
    composite_path = f'{output_dir}/{level_id}_composite.png'
    canvas.save(composite_path)
    print(f"  Saved: {composite_path} ({canvas.width}x{canvas.height})")
    
    # Save overview (scaled down)
    scale = max(1, canvas.width // 2048)
    thumb = canvas.resize((canvas.width // scale, canvas.height // scale), Image.NEAREST)
    overview_path = f'{output_dir}/{level_id}_overview.png'
    thumb.save(overview_path)
    print(f"  Saved: {overview_path} ({thumb.width}x{thumb.height})")
    
    # Save viewport (2x zoomed section)
    viewport_x = min(1000, max(0, canvas.width - 320))
    viewport_y = min(200, max(0, canvas.height - 240))
    viewport = canvas.crop((viewport_x, viewport_y, 
                            min(viewport_x + 320, canvas.width),
                            min(viewport_y + 240, canvas.height)))
    viewport_2x = viewport.resize((viewport.width * 2, viewport.height * 2), Image.NEAREST)
    viewport_path = f'{output_dir}/{level_id}_viewport.png'
    viewport_2x.save(viewport_path)
    print(f"  Saved: {viewport_path}")
    
    return canvas


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Render Skullmonkeys level to PNG')
    parser.add_argument('level_index', type=int, nargs='?', default=1,
                        help='Level index (0=MENU, 1=PHRO, 2=SCIE, ...)')
    parser.add_argument('output_dir', nargs='?', default='extracted_memory/level_renders',
                        help='Output directory')
    parser.add_argument('--blb', default='disks/blb/GAME.BLB',
                        help='Path to GAME.BLB file')
    parser.add_argument('--list', action='store_true',
                        help='List all levels and exit')
    
    args = parser.parse_args()
    
    blb_path = Path(args.blb)
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}")
        sys.exit(1)
    
    with BLBFile(blb_path) as blb:
        if args.list:
            print("Available levels:")
            for i, entry in enumerate(blb.header.level_entries):
                print(f"  {i:2}: {entry.level_id:4} - {entry.name}")
            return
        
        if args.level_index < 0 or args.level_index >= len(blb.header.level_entries):
            print(f"Error: Invalid level index {args.level_index}")
            print(f"Valid range: 0-{len(blb.header.level_entries) - 1}")
            sys.exit(1)
        
        render_level(blb, args.level_index, args.output_dir)


if __name__ == '__main__':
    main()
