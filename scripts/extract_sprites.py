#!/usr/bin/env python3
"""
extract_sprites.py - Extract sprite/tile graphics from Skullmonkeys BLB files

This script extracts tile graphics from the secondary container of level data
and saves them as PNG images.

Tile Format (VERIFIED via extraction and comparison):
  - Asset 100: Header (36 bytes) with tile counts at offsets 0x10, 0x12
  - Asset 300: Raw 8bpp (256-color) tile pixel data
    - First 'count_16x16' tiles: 256 bytes each (16×16, 16-byte stride)
    - Remaining 'count_8x8' tiles: 128 bytes each (8×8, 16-byte stride)
  - Asset 301: Palette index per tile (1 byte per tile)
  - Asset 400: Palette container (256 colors × 2 bytes = 512 bytes per palette)

PSX Color Format:
  - 15-bit RGB (5-5-5) in little-endian u16
  - Bit 15 is STP (semi-transparency)
  - Color 0 is typically transparent

Usage:
    python3 extract_sprites.py <blb_file> [level_index] [output_dir]
    
Examples:
    python3 extract_sprites.py disks/blb/sles-01090.blb 2
    python3 extract_sprites.py disks/blb/sles-01090.blb 0 output/menu_tiles
"""

import struct
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

# Try to import PIL for PNG export
try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: PIL not installed. Install with: pip install Pillow")
    print("         Will export raw data instead of PNG images.")

# Add parent directory to path for blb module
sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile, SECTOR_SIZE


@dataclass
class TileHeader:
    """Header structure from asset 100 (36 bytes)."""
    raw_data: bytes
    count_16x16: int  # 16×16 tile count (offset 0x10)
    count_8x8: int    # 8×8 tile count (offset 0x12)
    
    @property
    def total_tiles(self) -> int:
        return self.count_16x16 + self.count_8x8
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "TileHeader":
        if len(data) < 0x14:
            raise ValueError(f"Header too short: {len(data)} bytes")
        
        return cls(
            raw_data=data,
            count_16x16=struct.unpack('<H', data[0x10:0x12])[0],
            count_8x8=struct.unpack('<H', data[0x12:0x14])[0],
        )


def parse_toc(data: bytes) -> dict:
    """Parse a container TOC and return dict of asset_id -> (offset, size, data)."""
    if len(data) < 4:
        return {}
    
    count = struct.unpack('<I', data[0:4])[0]
    assets = {}
    
    for i in range(count):
        offset = 4 + i * 12
        if offset + 12 > len(data):
            break
        asset_id, size, data_offset = struct.unpack('<III', data[offset:offset+12])
        if data_offset + size <= len(data):
            assets[asset_id] = (data_offset, size, data[data_offset:data_offset+size])
    
    return assets


def psx_color_to_rgb(color: int) -> tuple:
    """Convert PSX 15-bit color to RGB tuple."""
    r = (color & 0x1F) * 8
    g = ((color >> 5) & 0x1F) * 8
    b = ((color >> 10) & 0x1F) * 8
    return (r, g, b)


def parse_palette(data: bytes) -> list:
    """Parse a 256-color PSX palette (512 bytes) to RGB tuples."""
    colors = []
    for i in range(min(256, len(data) // 2)):
        color = struct.unpack('<H', data[i*2:i*2+2])[0]
        colors.append(psx_color_to_rgb(color))
    return colors


def extract_tile_4bpp(tile_data: bytes, width: int = 16, height: int = 16) -> list:
    """
    Extract a 4bpp tile to a 2D array of palette indices.
    
    4bpp format: Each byte contains 2 pixels (low nibble first).
    A 16×16 tile has 16 rows of 8 bytes (16 pixels / 2 = 8 bytes per row).
    """
    pixels = []
    bytes_per_row = width // 2
    
    for y in range(height):
        row = []
        for x in range(0, width, 2):
            byte_idx = y * bytes_per_row + x // 2
            if byte_idx < len(tile_data):
                byte = tile_data[byte_idx]
                row.append(byte & 0x0F)        # Low nibble = first pixel
                row.append((byte >> 4) & 0x0F) # High nibble = second pixel
            else:
                row.extend([0, 0])
        pixels.append(row)
    
    return pixels


def extract_tile_8bpp(tile_data: bytes, width: int = 16, height: int = 16) -> list:
    """
    Extract an 8bpp tile to a 2D array of palette indices.
    
    8bpp format: Each byte is one pixel index.
    """
    pixels = []
    
    for y in range(height):
        row = []
        for x in range(width):
            byte_idx = y * width + x
            if byte_idx < len(tile_data):
                row.append(tile_data[byte_idx])
            else:
                row.append(0)
        pixels.append(row)
    
    return pixels


def tile_to_image(pixels: list, palette: list, transparent_index: int = 0) -> "Image":
    """Convert pixel array to PIL Image."""
    if not HAS_PIL:
        raise RuntimeError("PIL not available")
    
    height = len(pixels)
    width = len(pixels[0]) if pixels else 0
    
    img = Image.new('RGBA', (width, height))
    
    for y, row in enumerate(pixels):
        for x, idx in enumerate(row):
            if idx == transparent_index:
                img.putpixel((x, y), (0, 0, 0, 0))
            elif idx < len(palette):
                r, g, b = palette[idx]
                img.putpixel((x, y), (r, g, b, 255))
            else:
                img.putpixel((x, y), (255, 0, 255, 255))  # Magenta for missing
    
    return img


def extract_tiles_from_level(blb: BLBFile, level_index: int, output_dir: Path, 
                             max_tiles: int = 100, use_per_tile_palette: bool = True) -> int:
    """
    Extract tiles from a level's secondary container.
    
    Args:
        blb: BLB file object
        level_index: Level index (0-25)
        output_dir: Output directory for PNG files
        max_tiles: Maximum tiles to extract
        use_per_tile_palette: If True, use asset 301 palette index per tile
    
    Returns number of tiles extracted.
    """
    level = blb.header.get_level_by_index(level_index)
    if not level:
        print(f"Error: Level {level_index} not found")
        return 0
    
    print(f"Extracting from: {level.level_id} - {level.name}")
    
    # Read secondary container
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    print(f"  Secondary container: {len(sec_data)} bytes")
    
    # Parse TOC
    assets = parse_toc(sec_data)
    print(f"  Assets found: {list(assets.keys())}")
    
    if 100 not in assets or 300 not in assets:
        print("  Error: Missing required assets (100/300)")
        return 0
    
    # Parse tile header
    _, _, header_data = assets[100]
    header = TileHeader.from_bytes(header_data)
    print(f"  Tile counts: 16x16={header.count_16x16}, 8x8={header.count_8x8}")
    print(f"  Total tiles: {header.total_tiles}")
    
    # Get tile data
    _, tile_size, tile_data = assets[300]
    print(f"  Tile data: {tile_size} bytes")
    
    # Get palette index per tile (asset 301)
    palette_indices = None
    if 301 in assets and use_per_tile_palette:
        _, _, pal_idx_data = assets[301]
        palette_indices = list(pal_idx_data)
        unique_pals = sorted(set(palette_indices))
        print(f"  Palette indices: {len(palette_indices)} entries, uses palettes {unique_pals}")
    
    # Get palettes from asset 400
    palettes = []
    if 400 in assets:
        _, _, pal_container = assets[400]
        pal_count = struct.unpack('<I', pal_container[0:4])[0]
        print(f"  Palettes found: {pal_count}")
        
        for i in range(pal_count):
            toc_offset = 4 + i * 12
            _, pal_size, pal_offset = struct.unpack('<III', pal_container[toc_offset:toc_offset+12])
            pal_data = pal_container[pal_offset:pal_offset+pal_size]
            palettes.append(parse_palette(pal_data))
    
    if not palettes:
        # Create grayscale fallback palette
        print("  No palettes found, using grayscale")
        palettes = [[(i*17, i*17, i*17) for i in range(256)]]
    
    # Get tile size flags (asset 302) - determines 16x16 vs 8x8
    tile_flags = None
    if 302 in assets:
        _, _, flag_data = assets[302]
        tile_flags = list(flag_data)
        flag_counts = {}
        for f in tile_flags:
            flag_counts[f] = flag_counts.get(f, 0) + 1
        print(f"  Tile flags: {dict(sorted(flag_counts.items()))}")
    
    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Extract tiles using flags to determine size
    # Flag 0: 16×16 (256 bytes, 16-byte stride)
    # Flag 1: skip/special (TBD)
    # Flag 2: 8×8 (128 bytes, 16-byte stride with padding)
    
    extracted = 0
    tile_offset = 0
    total_tiles = header.total_tiles
    num_to_extract = min(total_tiles, max_tiles)
    
    print(f"  Extracting {num_to_extract} of {total_tiles} tiles...")
    
    for i in range(num_to_extract):
        # Determine tile size from flag
        flag = tile_flags[i] if tile_flags and i < len(tile_flags) else 0
        
        if flag == 2:
            # 8×8 tile with 16-byte stride = 128 bytes
            tile_bytes = tile_data[tile_offset:tile_offset + 128]
            tile_offset += 128
            tile_size_name = "8x8"
            
            if len(tile_bytes) < 128:
                break
            
            # Extract 8x8 from 16-byte stride data (only first 8 bytes of each row)
            pixels = []
            for row in range(8):
                row_start = row * 16  # 16-byte stride
                row_data = tile_bytes[row_start:row_start + 8]
                pixels.append(list(row_data))
                
        elif flag == 1:
            # Flag 1 - possibly skip or animated, treat as 16x16 for now
            tile_bytes = tile_data[tile_offset:tile_offset + 256]
            tile_offset += 256
            tile_size_name = "16x16_f1"
            
            if len(tile_bytes) < 256:
                break
            
            pixels = extract_tile_8bpp(tile_bytes, 16, 16)
            
        else:  # flag == 0 or default
            # 16×16 @ 8bpp = 256 bytes
            tile_bytes = tile_data[tile_offset:tile_offset + 256]
            tile_offset += 256
            tile_size_name = "16x16"
            
            if len(tile_bytes) < 256:
                break
            
            pixels = extract_tile_8bpp(tile_bytes, 16, 16)
        
        # Use per-tile palette if available, otherwise use first palette
        if palette_indices and i < len(palette_indices):
            pal_idx = palette_indices[i]
            if pal_idx < len(palettes):
                palette = palettes[pal_idx]
            else:
                palette = palettes[0] if palettes else [(j*17, j*17, j*17) for j in range(256)]
        else:
            palette = palettes[0] if palettes else [(j*17, j*17, j*17) for j in range(256)]
        
        if HAS_PIL:
            img = tile_to_image(pixels, palette)
            img.save(output_dir / f"tile_{i:04d}_{tile_size_name}.png")
        else:
            # Save raw data
            with open(output_dir / f"tile_{i:04d}_{tile_size_name}.raw", 'wb') as f:
                f.write(tile_bytes)
        
        extracted += 1
    
    print(f"  Extracted {extracted} tiles to {output_dir}")
    return extracted


def create_tile_sheet(blb: BLBFile, level_index: int, output_path: Path,
                      tiles_per_row: int = 16, max_tiles: int = 256) -> bool:
    """
    Create a single spritesheet image with all tiles.
    Uses per-tile palette from asset 301.
    """
    if not HAS_PIL:
        print("Error: PIL required for tile sheet")
        return False
    
    level = blb.header.get_level_by_index(level_index)
    if not level:
        print(f"Error: Level {level_index} not found")
        return False
    
    print(f"Creating tile sheet from: {level.level_id} - {level.name}")
    
    # Read secondary container
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    assets = parse_toc(sec_data)
    
    if 100 not in assets or 300 not in assets:
        print("Error: Missing required assets")
        return False
    
    # Parse header
    _, _, header_data = assets[100]
    header = TileHeader.from_bytes(header_data)
    
    # Get tile data
    _, _, tile_data = assets[300]
    
    # Get palette indices (asset 301)
    palette_indices = None
    if 301 in assets:
        _, _, pal_idx_data = assets[301]
        palette_indices = list(pal_idx_data)
    
    # Get all palettes
    palettes = []
    if 400 in assets:
        _, _, pal_container = assets[400]
        if len(pal_container) >= 4:
            pal_count = struct.unpack('<I', pal_container[0:4])[0]
            for i in range(pal_count):
                toc_off = 4 + i * 12
                _, pal_size, pal_offset = struct.unpack('<III', pal_container[toc_off:toc_off+12])
                pal_data = pal_container[pal_offset:pal_offset+pal_size]
                palettes.append(parse_palette(pal_data))
    
    # Fallback grayscale palette
    if not palettes:
        palettes = [[(i*17, i*17, i*17) for i in range(256)]]
    
    # Get tile flags (asset 302)
    tile_flags = None
    if 302 in assets:
        _, _, flag_data = assets[302]
        tile_flags = list(flag_data)
    
    # Calculate sheet dimensions - use 16x16 grid cells even for 8x8 tiles
    total_tiles = header.total_tiles
    num_tiles = min(total_tiles, max_tiles)
    rows = (num_tiles + tiles_per_row - 1) // tiles_per_row
    
    sheet_width = tiles_per_row * 16
    sheet_height = rows * 16
    
    print(f"  Creating {sheet_width}×{sheet_height} sheet ({num_tiles} tiles)")
    
    sheet = Image.new('RGBA', (sheet_width, sheet_height), (0, 0, 0, 0))
    
    tile_offset = 0
    for i in range(num_tiles):
        # Determine tile size from flag
        flag = tile_flags[i] if tile_flags and i < len(tile_flags) else 0
        
        if flag == 2:
            # 8×8 tile with 16-byte stride = 128 bytes
            tile_bytes = tile_data[tile_offset:tile_offset + 128]
            tile_offset += 128
            
            if len(tile_bytes) < 128:
                break
            
            # Extract 8x8 from 16-byte stride data
            pixels = []
            for row in range(8):
                row_start = row * 16
                row_data = tile_bytes[row_start:row_start + 8]
                pixels.append(list(row_data))
        else:
            # 16×16 @ 8bpp = 256 bytes (flag 0 or 1)
            tile_bytes = tile_data[tile_offset:tile_offset + 256]
            tile_offset += 256
            
            if len(tile_bytes) < 256:
                break
            
            pixels = extract_tile_8bpp(tile_bytes, 16, 16)
        
        # Use per-tile palette from asset 301
        if palette_indices and i < len(palette_indices):
            pal_idx = palette_indices[i]
            if pal_idx < len(palettes):
                palette = palettes[pal_idx]
            else:
                palette = palettes[0]
        else:
            palette = palettes[0]
        
        tile_img = tile_to_image(pixels, palette)
        
        x = (i % tiles_per_row) * 16
        y = (i // tiles_per_row) * 16
        sheet.paste(tile_img, (x, y))
    
    sheet.save(output_path)
    print(f"  Saved to: {output_path}")
    return True


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    blb_path = Path(sys.argv[1])
    level_arg = sys.argv[2] if len(sys.argv) > 2 else "0"
    output_dir = Path(sys.argv[3]) if len(sys.argv) > 3 else None
    
    if not blb_path.exists():
        print(f"Error: File not found: {blb_path}")
        sys.exit(1)
    
    with BLBFile(blb_path) as blb:
        print(f"BLB file: {blb_path}")
        print(f"Level count: {blb.header.level_count}")
        print()
        
        # Check for "all" mode
        if level_arg.lower() == "all":
            # Extract tilesheets for all levels
            base_output = output_dir or Path("output/tilesets")
            base_output.mkdir(parents=True, exist_ok=True)
            
            for idx in range(blb.header.level_count):
                level = blb.header.get_level_by_index(idx)
                if level.secondary_count == 0:
                    print(f"Skipping {idx}: {level.level_id} (no secondary data)")
                    continue
                
                print(f"\n{'='*60}")
                sheet_path = base_output / f"{idx:02d}_{level.level_id}_tileset.png"
                create_tile_sheet(blb, idx, sheet_path, max_tiles=10000)
            
            print(f"\n{'='*60}")
            print(f"All tilesets saved to: {base_output}")
        else:
            level_index = int(level_arg)
            out_dir = output_dir or Path(f"output/tiles_{level_index}")
            
            # Create individual tiles
            count = extract_tiles_from_level(blb, level_index, out_dir, max_tiles=50)
            
            # Also create a tile sheet
            if HAS_PIL and count > 0:
                print()
                sheet_path = out_dir / f"tilesheet_{level_index}.png"
                create_tile_sheet(blb, level_index, sheet_path, max_tiles=10000)


if __name__ == "__main__":
    main()
