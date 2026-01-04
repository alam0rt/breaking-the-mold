#!/usr/bin/env python3
"""
extract_layers.py - Extract tilemap layers from all Skullmonkeys levels

This script extracts all background/tilemap layers from each level in the game,
rendering them as PNG images using the correct per-tile palettes.

Usage:
    python3 scripts/extract_layers.py                    # Extract all levels
    python3 scripts/extract_layers.py --level PHRO       # Extract specific level by ID
    python3 scripts/extract_layers.py --level 1          # Extract specific level by index
    python3 scripts/extract_layers.py --output dir/      # Custom output directory
    python3 scripts/extract_layers.py --list             # List all levels

Output Structure:
    output/layers/
    ├── 00_MENU/
    │   ├── layer_00.png
    │   ├── layer_01.png
    │   └── ...
    ├── 01_PHRO/
    │   ├── layer_00.png
    │   └── ...
    └── ...
"""

import argparse
import os
import struct
import sys
from pathlib import Path

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

from blb import BLBFile, parse_container_toc, TileHeader, parse_psx_palette

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: PIL not available. Install with: pip install Pillow")


def get_tile_pixels(tile_idx_1based: int, n_16x16: int, pixel_data: bytes, 
                    flags_data: bytes) -> tuple:
    """
    Get pixel data for a tile.
    
    Tile indices are 1-based (0 = empty/transparent).
    16x16 tiles: 256 bytes each, stored first
    8x8 tiles: 128 bytes each (8 rows x 16-byte stride), stored after 16x16 tiles
    
    Args:
        tile_idx_1based: 1-based tile index (0 = empty)
        n_16x16: Number of 16x16 tiles
        pixel_data: Asset 300 raw pixel data
        flags_data: Asset 302 size flags (0=16x16, 2=8x8)
    
    Returns:
        (pixels, tile_size) where pixels is 2D list of palette indices,
        or (None, None) if tile is empty/invalid
    """
    if tile_idx_1based == 0:
        return None, None
    
    idx0 = tile_idx_1based - 1
    
    # Determine tile size from flags
    if idx0 < len(flags_data):
        is_8x8 = (flags_data[idx0] & 2) != 0
    else:
        # Fallback: assume 8x8 if index >= n_16x16
        is_8x8 = idx0 >= n_16x16
    
    if is_8x8:
        # 8x8 tiles: 128 bytes each, stored after 16x16 tiles
        # Note: pixel_data uses the corrected formula from CopyTilePixelData
        offset = n_16x16 * 128 + (idx0 - n_16x16) * 128 if idx0 >= n_16x16 else n_16x16 * 128 + idx0 * 128
        # Actually the formula is simpler: all 8x8 tiles start at n_16x16 * 128
        # But idx0 for 8x8 tiles is relative to total, not to 8x8 section
        # Let me use the correct offset based on flags
        if idx0 >= n_16x16:
            offset = n_16x16 * 128 + (idx0 - n_16x16) * 128
        else:
            # This shouldn't happen - 8x8 tiles should have idx >= n_16x16
            offset = idx0 * 128
        tile_size = 8
        bytes_per_tile = 128
    else:
        # 16x16 tiles: 256 bytes each
        offset = idx0 * 256
        tile_size = 16
        bytes_per_tile = 256
    
    if offset + bytes_per_tile > len(pixel_data):
        return None, None
    
    # Read pixels (8bpp, row stride = 16 for both sizes)
    pixels = []
    stride = 16
    for y in range(tile_size):
        row = []
        for x in range(tile_size):
            byte_offset = offset + y * stride + x
            if byte_offset < len(pixel_data):
                row.append(pixel_data[byte_offset])
            else:
                row.append(0)
        pixels.append(row)
    
    return pixels, tile_size


def extract_level_layers(blb: BLBFile, level_idx: int, output_dir: str, 
                         verbose: bool = True) -> dict:
    """
    Extract all tilemap layers from a single level.
    
    Args:
        blb: Open BLBFile instance
        level_idx: Level index (0-25)
        output_dir: Output directory for PNG files
        verbose: Print progress messages
    
    Returns:
        Dictionary with extraction statistics
    """
    level = blb.header.get_level_by_index(level_idx)
    level_dir = os.path.join(output_dir, f"{level_idx:02d}_{level.level_id}")
    os.makedirs(level_dir, exist_ok=True)
    
    stats = {
        'level_id': level.level_id,
        'level_name': level.name,
        'level_index': level_idx,
        'layers': [],
        'errors': []
    }
    
    if verbose:
        print(f"\n{'='*60}")
        print(f"Level {level_idx:2d}: {level.level_id} - {level.name}")
        print(f"{'='*60}")
    
    # Read secondary segment (tiles, palettes)
    try:
        sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
        sec_entries = parse_container_toc(sec_data)
    except Exception as e:
        stats['errors'].append(f"Failed to read secondary: {e}")
        if verbose:
            print(f"  ERROR: Failed to read secondary segment: {e}")
        return stats
    
    # Find required assets
    asset100 = next((e for e in sec_entries if e.asset_id == 100), None)
    asset300 = next((e for e in sec_entries if e.asset_id == 300), None)
    asset301 = next((e for e in sec_entries if e.asset_id == 301), None)
    asset302 = next((e for e in sec_entries if e.asset_id == 302), None)
    asset400 = next((e for e in sec_entries if e.asset_id == 400), None)
    
    if not all([asset100, asset300, asset301, asset302, asset400]):
        missing = []
        if not asset100: missing.append('100')
        if not asset300: missing.append('300')
        if not asset301: missing.append('301')
        if not asset302: missing.append('302')
        if not asset400: missing.append('400')
        stats['errors'].append(f"Missing secondary assets: {missing}")
        if verbose:
            print(f"  WARNING: Missing secondary assets: {missing}")
        return stats
    
    # Parse tile data
    header = TileHeader.from_bytes(asset100.data)
    n_16x16 = header.count_16x16
    n_8x8 = header.count_8x8_a + header.count_8x8_b
    pixel_data = asset300.data
    palette_indices = list(asset301.data)
    flags_data = list(asset302.data)
    
    stats['n_16x16'] = n_16x16
    stats['n_8x8'] = n_8x8
    stats['total_tiles'] = header.total_tiles
    
    if verbose:
        print(f"  Tiles: {n_16x16} 16x16 + {n_8x8} 8x8 = {n_16x16 + n_8x8} total")
    
    # Parse palettes
    try:
        palette_toc = parse_container_toc(asset400.data)
        palettes = [parse_psx_palette(p.data) for p in palette_toc]
        stats['n_palettes'] = len(palettes)
        if verbose:
            print(f"  Palettes: {len(palettes)}")
    except Exception as e:
        stats['errors'].append(f"Failed to parse palettes: {e}")
        if verbose:
            print(f"  ERROR: Failed to parse palettes: {e}")
        return stats
    
    # Check for tertiary data (layer definitions)
    if level.tert_block_count == 0 or level.tert_sub_counts[0] == 0:
        stats['errors'].append("No tertiary data (no layers)")
        if verbose:
            print(f"  WARNING: No tertiary data available")
        return stats
    
    # Read tertiary segment (layer definitions)
    try:
        tert_data = blb.read_sectors(level.tert_sub_offsets[0], level.tert_sub_counts[0])
        tert_entries = parse_container_toc(tert_data)
    except Exception as e:
        stats['errors'].append(f"Failed to read tertiary: {e}")
        if verbose:
            print(f"  ERROR: Failed to read tertiary segment: {e}")
        return stats
    
    asset200 = next((e for e in tert_entries if e.asset_id == 200), None)
    asset201 = next((e for e in tert_entries if e.asset_id == 201), None)
    
    if not asset200 or not asset201:
        stats['errors'].append("Missing tertiary assets 200/201")
        if verbose:
            print(f"  WARNING: Missing tertiary assets 200/201")
        return stats
    
    # Calculate layer count from Asset 201 (92 bytes per layer)
    n_layers = len(asset201.data) // 92
    stats['n_layers'] = n_layers
    
    if verbose:
        print(f"  Layers: {n_layers}")
    
    if not HAS_PIL:
        stats['errors'].append("PIL not available - skipping image generation")
        return stats
    
    # Extract each layer
    for layer_idx in range(n_layers):
        layer_stats = extract_single_layer(
            layer_idx, asset200.data, asset201.data,
            pixel_data, palette_indices, flags_data, palettes,
            n_16x16, level_dir, verbose
        )
        stats['layers'].append(layer_stats)
    
    return stats


def extract_single_layer(layer_idx: int, asset200_data: bytes, asset201_data: bytes,
                         pixel_data: bytes, palette_indices: list, flags_data: list,
                         palettes: list, n_16x16: int, output_dir: str,
                         verbose: bool) -> dict:
    """
    Extract a single tilemap layer to PNG.
    
    Returns:
        Dictionary with layer statistics
    """
    layer_stats = {
        'index': layer_idx,
        'tiles_rendered': 0,
        'error': None
    }
    
    # Parse layer entry (92 bytes)
    layer_offset = layer_idx * 92
    layer = asset201_data[layer_offset:layer_offset + 92]
    
    if len(layer) < 92:
        layer_stats['error'] = "Truncated layer entry"
        return layer_stats
    
    # Layer dimensions
    src_w = struct.unpack('<H', layer[4:6])[0]
    src_h = struct.unpack('<H', layer[6:8])[0]
    dst_w = struct.unpack('<H', layer[8:10])[0]
    dst_h = struct.unpack('<H', layer[10:12])[0]
    priority = struct.unpack('<H', layer[14:16])[0]
    
    layer_stats['src_w'] = src_w
    layer_stats['src_h'] = src_h
    layer_stats['dst_w'] = dst_w
    layer_stats['dst_h'] = dst_h
    layer_stats['priority'] = priority
    
    if src_w == 0 or src_h == 0:
        layer_stats['error'] = "Zero dimensions"
        if verbose:
            print(f"    Layer {layer_idx:2d}: SKIP (zero dimensions)")
        return layer_stats
    
    # Get tilemap data from Asset 200 sub-TOC
    subtoc_offset = 4 + layer_idx * 12
    if subtoc_offset + 12 > len(asset200_data):
        layer_stats['error'] = "Sub-TOC out of bounds"
        return layer_stats
    
    _, tilemap_size, tilemap_data_off = struct.unpack(
        '<III', asset200_data[subtoc_offset:subtoc_offset + 12]
    )
    
    if tilemap_data_off + tilemap_size > len(asset200_data):
        layer_stats['error'] = "Tilemap data out of bounds"
        return layer_stats
    
    tilemap = asset200_data[tilemap_data_off:tilemap_data_off + tilemap_size]
    
    # Create image (16 pixels per tile)
    tile_px = 16
    img_w = src_w * tile_px
    img_h = src_h * tile_px
    
    img = Image.new('RGBA', (img_w, img_h), (0, 0, 0, 0))
    pixels_img = img.load()
    
    tiles_rendered = 0
    
    for ty in range(src_h):
        for tx in range(src_w):
            tile_data_idx = (ty * src_w + tx) * 2
            if tile_data_idx + 2 > len(tilemap):
                continue
            
            raw_idx = struct.unpack('<H', tilemap[tile_data_idx:tile_data_idx + 2])[0]
            tile_idx = raw_idx & 0x7FF  # Lower 11 bits = tile index
            
            if tile_idx == 0:
                continue
            
            # Get tile pixels
            tile_pixels, tile_size = get_tile_pixels(
                tile_idx, n_16x16, pixel_data, flags_data
            )
            if tile_pixels is None:
                continue
            
            # Get palette for this tile
            tile_idx0 = tile_idx - 1
            if tile_idx0 < len(palette_indices):
                pal_idx = palette_indices[tile_idx0]
                if pal_idx < len(palettes):
                    palette = palettes[pal_idx]
                else:
                    palette = palettes[0] if palettes else [(255, 0, 255)] * 256
            else:
                palette = palettes[0] if palettes else [(255, 0, 255)] * 256
            
            tiles_rendered += 1
            
            # Render tile (scale 8x8 to 16x16 for consistent output)
            scale = tile_px // tile_size
            
            for py in range(tile_size):
                for px in range(tile_size):
                    color_idx = tile_pixels[py][px]
                    if color_idx < len(palette):
                        r, g, b = palette[color_idx]
                        alpha = 0 if color_idx == 0 else 255
                    else:
                        r, g, b, alpha = 255, 0, 255, 255
                    
                    for dy in range(scale):
                        for dx in range(scale):
                            ix = tx * tile_px + px * scale + dx
                            iy = ty * tile_px + py * scale + dy
                            if 0 <= ix < img_w and 0 <= iy < img_h:
                                pixels_img[ix, iy] = (r, g, b, alpha)
    
    # Save image
    out_path = os.path.join(output_dir, f'layer_{layer_idx:02d}.png')
    img.save(out_path)
    
    layer_stats['tiles_rendered'] = tiles_rendered
    layer_stats['output_path'] = out_path
    
    if verbose:
        print(f"    Layer {layer_idx:2d}: {src_w:3d}x{src_h:2d} tiles, "
              f"{tiles_rendered:5d} rendered, dst={dst_w}x{dst_h}, pri={priority}")
    
    return layer_stats


def main():
    parser = argparse.ArgumentParser(
        description='Extract tilemap layers from Skullmonkeys levels',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('--blb', default='disks/blb/GAME.BLB',
                        help='Path to GAME.BLB file (default: disks/blb/GAME.BLB)')
    parser.add_argument('--output', '-o', default='output/layers',
                        help='Output directory (default: output/layers)')
    parser.add_argument('--level', '-l', 
                        help='Extract specific level (by ID like PHRO or index like 1)')
    parser.add_argument('--list', action='store_true',
                        help='List all levels and exit')
    parser.add_argument('--quiet', '-q', action='store_true',
                        help='Suppress progress output')
    
    args = parser.parse_args()
    
    if not os.path.exists(args.blb):
        print(f"Error: BLB file not found: {args.blb}")
        sys.exit(1)
    
    with BLBFile(args.blb) as blb:
        level_count = blb.header.level_count
        
        if args.list:
            print(f"Levels in {args.blb}:")
            print(f"{'Idx':>3}  {'ID':<5}  {'Name':<25}  {'Tert':>4}  {'Layers':>6}")
            print("-" * 50)
            for i in range(level_count):
                level = blb.header.get_level_by_index(i)
                # Quick layer count check
                n_layers = "?"
                if level.tert_block_count > 0 and level.tert_sub_counts[0] > 0:
                    try:
                        tert_data = blb.read_sectors(level.tert_sub_offsets[0], 
                                                     level.tert_sub_counts[0])
                        tert_entries = parse_container_toc(tert_data)
                        asset201 = next((e for e in tert_entries if e.asset_id == 201), None)
                        if asset201:
                            n_layers = len(asset201.data) // 92
                    except:
                        pass
                print(f"{i:3d}  {level.level_id:<5}  {level.name:<25}  "
                      f"{level.tert_block_count:>4}  {n_layers:>6}")
            return
        
        # Determine which levels to extract
        if args.level is not None:
            # Specific level
            try:
                level_idx = int(args.level)
                if level_idx < 0 or level_idx >= level_count:
                    print(f"Error: Level index {level_idx} out of range (0-{level_count-1})")
                    sys.exit(1)
                levels_to_extract = [level_idx]
            except ValueError:
                # Try as level ID
                level_id = args.level.upper()
                found = None
                for i in range(level_count):
                    level = blb.header.get_level_by_index(i)
                    if level.level_id.upper() == level_id:
                        found = i
                        break
                if found is None:
                    print(f"Error: Level ID '{args.level}' not found")
                    sys.exit(1)
                levels_to_extract = [found]
        else:
            # All levels
            levels_to_extract = list(range(level_count))
        
        verbose = not args.quiet
        os.makedirs(args.output, exist_ok=True)
        
        if verbose:
            print(f"Extracting {len(levels_to_extract)} level(s) to {args.output}/")
        
        all_stats = []
        total_layers = 0
        total_errors = 0
        
        for level_idx in levels_to_extract:
            stats = extract_level_layers(blb, level_idx, args.output, verbose)
            all_stats.append(stats)
            total_layers += len(stats.get('layers', []))
            if stats.get('errors'):
                total_errors += len(stats['errors'])
        
        if verbose:
            print(f"\n{'='*60}")
            print(f"SUMMARY")
            print(f"{'='*60}")
            print(f"Levels processed: {len(levels_to_extract)}")
            print(f"Total layers extracted: {total_layers}")
            if total_errors > 0:
                print(f"Total errors: {total_errors}")
            print(f"Output directory: {args.output}/")


if __name__ == '__main__':
    main()
