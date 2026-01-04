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
                             max_tiles: int = 100) -> int:
    """
    Extract tiles from a level's secondary container.
    
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
        palettes = [[(i*17, i*17, i*17) for i in range(16)]]
    
    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Extract tiles
    extracted = 0
    tile_offset = 0
    
    # Extract 16×16 tiles (256 bytes each = 16 rows × 16 bytes for 8bpp)
    # But the copy loop does 16 iterations copying 16 bytes = 256 bytes
    # If it's 4bpp: 16×16 = 256 pixels, at 4bpp = 128 bytes
    # The decompiled code shows 0x100 (256) bytes per 16×16 tile, so it's 8bpp
    
    num_16x16 = min(header.count_16x16, max_tiles)
    print(f"  Extracting {num_16x16} tiles (16×16)...")
    
    for i in range(num_16x16):
        # 16×16 @ 8bpp = 256 bytes
        tile_bytes = tile_data[tile_offset:tile_offset + 256]
        tile_offset += 256
        
        if len(tile_bytes) < 256:
            break
        
        pixels = extract_tile_8bpp(tile_bytes, 16, 16)
        
        # Use first palette for now
        palette = palettes[0] if palettes else [(i*17, i*17, i*17) for i in range(256)]
        
        if HAS_PIL:
            img = tile_to_image(pixels, palette)
            img.save(output_dir / f"tile_{i:04d}_16x16.png")
        else:
            # Save raw data
            with open(output_dir / f"tile_{i:04d}_16x16.raw", 'wb') as f:
                f.write(tile_bytes)
        
        extracted += 1
    
    print(f"  Extracted {extracted} tiles to {output_dir}")
    return extracted


def create_tile_sheet(blb: BLBFile, level_index: int, output_path: Path,
                      tiles_per_row: int = 16, max_tiles: int = 256) -> bool:
    """
    Create a single spritesheet image with all tiles.
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
    
    # Get first palette
    palette = [(i*17, i*17, i*17) for i in range(256)]  # Grayscale default
    if 400 in assets:
        _, _, pal_container = assets[400]
        if len(pal_container) >= 4:
            pal_count = struct.unpack('<I', pal_container[0:4])[0]
            if pal_count > 0:
                _, pal_size, pal_offset = struct.unpack('<III', pal_container[4:16])
                pal_data = pal_container[pal_offset:pal_offset+pal_size]
                palette = parse_palette(pal_data)
    
    # Calculate sheet dimensions
    num_tiles = min(header.count_16x16, max_tiles)
    rows = (num_tiles + tiles_per_row - 1) // tiles_per_row
    
    sheet_width = tiles_per_row * 16
    sheet_height = rows * 16
    
    print(f"  Creating {sheet_width}×{sheet_height} sheet ({num_tiles} tiles)")
    
    sheet = Image.new('RGBA', (sheet_width, sheet_height), (0, 0, 0, 0))
    
    tile_offset = 0
    for i in range(num_tiles):
        tile_bytes = tile_data[tile_offset:tile_offset + 256]
        tile_offset += 256
        
        if len(tile_bytes) < 256:
            break
        
        pixels = extract_tile_8bpp(tile_bytes, 16, 16)
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
    level_index = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    output_dir = Path(sys.argv[3]) if len(sys.argv) > 3 else Path(f"output/tiles_{level_index}")
    
    if not blb_path.exists():
        print(f"Error: File not found: {blb_path}")
        sys.exit(1)
    
    with BLBFile(blb_path) as blb:
        print(f"BLB file: {blb_path}")
        print(f"Level count: {blb.header.level_count}")
        print()
        
        # Create individual tiles
        count = extract_tiles_from_level(blb, level_index, output_dir, max_tiles=50)
        
        # Also create a tile sheet
        if HAS_PIL and count > 0:
            print()
            sheet_path = output_dir / f"tilesheet_{level_index}.png"
            create_tile_sheet(blb, level_index, sheet_path)


if __name__ == "__main__":
    main()
