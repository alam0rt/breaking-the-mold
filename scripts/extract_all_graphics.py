#!/usr/bin/env python3
"""
extract_all_graphics.py - Extract ALL graphics from Skullmonkeys BLB files

Extracts:
  - Asset 600 (0x258): RLE sprites with embedded palettes (Primary segment)
  - Asset 300 (0x12C): Tiles with palette from Asset 400 (Secondary segment)
  - Asset 200/201: Tilemap layers rendered as full images (Secondary segment)
  - Tertiary sprites: RLE sprites from tertiary blocks

Usage:
    # Extract everything from all BLB files
    python3 extract_all_graphics.py
    
    # Extract specific BLB file
    python3 extract_all_graphics.py disks/blb/GAME.BLB
    
    # Extract specific level
    python3 extract_all_graphics.py disks/blb/GAME.BLB --level 2
    
    # Custom output directory
    python3 extract_all_graphics.py --output extracted/

Output structure:
    extracted/
    └── <blb_name>/
        └── <level_id>/
            ├── sprites/           # Asset 600 RLE sprites
            │   ├── sprite_00000001.png
            │   └── ...
            ├── tiles/             # Individual tiles from Asset 300
            │   ├── tile_0000.png
            │   └── ...
            ├── layers/            # Rendered tilemap layers
            │   ├── layer_00.png
            │   └── ...
            ├── tileset.png        # All tiles combined
            └── metadata.json      # Level info and asset details
"""

import struct
import argparse
import json
import sys
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional, Dict, List, Tuple

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Error: PIL required. Install with: pip install Pillow")
    sys.exit(1)

# Add scripts directory to path
sys.path.insert(0, str(Path(__file__).parent))

# Asset type constants
ASSET_HEADER = 100         # 0x064 - Tile header
ASSET_TILEMAP = 200        # 0x0C8 - Tilemap container
ASSET_LAYER_ENTRIES = 201  # 0x0C9 - Layer definitions
ASSET_TILE_PIXELS = 300    # 0x12C - Tile pixel data
ASSET_PALETTE_IDX = 301    # 0x12D - Palette index per tile
ASSET_SIZE_FLAGS = 302     # 0x12E - Tile size flags
ASSET_PALETTE = 400        # 0x190 - Palette container
ASSET_SPRITES = 600        # 0x258 - RLE sprite container
ASSET_COLLISION = 601      # 0x259 - Collision data

SECTOR_SIZE = 2048


def psx_color_to_rgba(color: int) -> Tuple[int, int, int, int]:
    """Convert PSX 15-bit color to RGBA tuple."""
    if color == 0:
        return (0, 0, 0, 0)  # Transparent
    r = (color & 0x1F) * 8
    g = ((color >> 5) & 0x1F) * 8
    b = ((color >> 10) & 0x1F) * 8
    return (r, g, b, 255)


def parse_palette_256(data: bytes) -> List[Tuple[int, int, int, int]]:
    """Parse a 256-color PSX palette (512 bytes) to RGBA tuples."""
    colors = []
    for i in range(min(256, len(data) // 2)):
        color = struct.unpack('<H', data[i*2:i*2+2])[0]
        if i == 0:
            colors.append((0, 0, 0, 0))  # Index 0 = transparent
        else:
            colors.append(psx_color_to_rgba(color))
    return colors


def parse_toc(data: bytes) -> Dict[int, Tuple[int, int, bytes]]:
    """Parse a container TOC. Returns dict of asset_id -> (offset, size, data)."""
    if len(data) < 4:
        return {}
    
    count = struct.unpack('<I', data[0:4])[0]
    if count > 1000:
        return {}
    
    assets = {}
    for i in range(count):
        offset = 4 + i * 12
        if offset + 12 > len(data):
            break
        asset_id, size, data_offset = struct.unpack('<III', data[offset:offset+12])
        if data_offset + size <= len(data):
            assets[asset_id] = (data_offset, size, data[data_offset:data_offset+size])
    
    return assets


def parse_sub_toc(data: bytes) -> List[Tuple[int, int, int, bytes]]:
    """Parse a sub-container TOC. Returns list of (id, size, offset, data)."""
    if len(data) < 4:
        return []
    
    count = struct.unpack('<I', data[0:4])[0]
    if count > 10000:
        return []
    
    entries = []
    for i in range(count):
        off = 4 + i * 12
        if off + 12 > len(data):
            break
        entry_id, size, offset = struct.unpack('<III', data[off:off+12])
        if offset + size <= len(data):
            entries.append((entry_id, size, offset, data[offset:offset+size]))
    
    return entries


@dataclass
class LevelMeta:
    """Level metadata from BLB header."""
    index: int
    level_id: str
    level_name: str
    primary_sector: int
    primary_count: int
    secondary_sector: int
    secondary_count: int
    tertiary_sector: int
    tertiary_count: int
    tert_block_count: int  # Number of tertiary sub-blocks


def read_level_metadata(blb_path: str, level_index: int) -> LevelMeta:
    """Read level metadata from BLB header."""
    with open(blb_path, 'rb') as f:
        f.seek(level_index * 0x70)
        meta = f.read(0x70)
        
        if len(meta) < 0x70:
            raise ValueError(f"Invalid level index {level_index}")
        
        # Level metadata structure (0x70 bytes per level):
        # +0x00: u16 primary_sector
        # +0x02: u16 primary_count
        # +0x0E: u16 tert_block_count
        # +0x1E: u16 secondary_sector
        # +0x2C: u16 secondary_count
        # +0x3A: u16 tertiary_sector (first tertiary block)
        # +0x48: u16 tertiary_count (first tertiary block)
        # +0x56: 5-byte level_id
        # +0x5B: 21-byte level_name
        
        return LevelMeta(
            index=level_index,
            level_id=meta[0x56:0x5B].decode('ascii', errors='replace').strip('\x00'),
            level_name=meta[0x5B:0x70].decode('ascii', errors='replace').strip('\x00'),
            primary_sector=struct.unpack('<H', meta[0x00:0x02])[0],
            primary_count=struct.unpack('<H', meta[0x02:0x04])[0],
            secondary_sector=struct.unpack('<H', meta[0x1E:0x20])[0],
            secondary_count=struct.unpack('<H', meta[0x2C:0x2E])[0],
            tertiary_sector=struct.unpack('<H', meta[0x3A:0x3C])[0],
            tertiary_count=struct.unpack('<H', meta[0x48:0x4A])[0],
            tert_block_count=struct.unpack('<H', meta[0x0E:0x10])[0],
        )


def read_segment(blb_path: str, sector: int, count: int) -> bytes:
    """Read a segment from BLB file."""
    with open(blb_path, 'rb') as f:
        f.seek(sector * SECTOR_SIZE)
        return f.read(count * SECTOR_SIZE)


def get_level_count(blb_path: str) -> int:
    """Get number of levels in BLB file."""
    # Read until we find an empty level slot
    count = 0
    with open(blb_path, 'rb') as f:
        while True:
            f.seek(count * 0x70)
            meta = f.read(0x70)
            if len(meta) < 0x70:
                break
            # Check if this is a valid level (has sectors)
            primary_sector = struct.unpack('<H', meta[0:2])[0]
            if primary_sector == 0 and count > 0:
                break
            count += 1
            if count > 100:  # Safety limit
                break
    return min(count, 26)  # Max 26 levels in game


# ============================================================================
# SPRITE EXTRACTION (Asset 600)
# ============================================================================

def decode_rle_sprite(sprite_data: bytes, rle_base_offset: int, frame_rle_offset: int,
                      width: int, height: int) -> List[int]:
    """Decode RLE-encoded sprite frame to pixel array."""
    rle_start = rle_base_offset + frame_rle_offset
    if rle_start >= len(sprite_data):
        return [0] * (width * height)
    
    rle_data = sprite_data[rle_start:]
    if len(rle_data) < 2:
        return [0] * (width * height)
    
    cmd_count = struct.unpack('<H', rle_data[0:2])[0]
    
    # Parse commands
    commands = []
    for i in range(min(cmd_count, 10000)):
        if 2 + i*2 + 2 > len(rle_data):
            break
        cmd = struct.unpack('<H', rle_data[2 + i*2:4 + i*2])[0]
        new_line = (cmd & 0x8000) != 0
        skip = (cmd & 0x7F00) >> 8
        copy_count = cmd & 0xFF
        commands.append((new_line, skip, copy_count))
    
    # Pixel data follows commands
    pixel_start = 2 + cmd_count * 2
    pixel_data = rle_data[pixel_start:]
    
    # Decode to 2D pixel array
    pixels = [0] * (width * height)
    x = 0
    y = 0
    pixel_idx = 0
    
    for new_line, skip, copy_count in commands:
        if new_line:
            y += 1
            x = 0
        
        x += skip
        
        for _ in range(copy_count):
            if y < height and x < width:
                px_offset = y * width + x
                if px_offset < len(pixels) and pixel_idx < len(pixel_data):
                    pixels[px_offset] = pixel_data[pixel_idx]
                pixel_idx += 1
                x += 1
    
    return pixels


def parse_sprite_header(sprite_data: bytes, sprite_size: int) -> dict:
    """Parse sprite header, animation entries, and embedded palette."""
    if len(sprite_data) < 12:
        return None
    
    anim_count = struct.unpack('<H', sprite_data[0:2])[0]
    frame_meta_offset = struct.unpack('<H', sprite_data[2:4])[0]
    rle_data_offset = struct.unpack('<I', sprite_data[4:8])[0]
    palette_offset = struct.unpack('<I', sprite_data[8:12])[0]
    
    # Parse embedded palette if present
    palette = None
    if palette_offset < sprite_size and palette_offset + 512 <= sprite_size:
        pal_data = sprite_data[palette_offset:palette_offset+512]
        if len(pal_data) >= 512:
            palette = []
            for i in range(256):
                color = struct.unpack('<H', pal_data[i*2:i*2+2])[0]
                if i == 0:
                    palette.append((0, 0, 0, 0))  # Transparent
                else:
                    r = (color & 0x1f) * 8
                    g = ((color >> 5) & 0x1f) * 8
                    b = ((color >> 10) & 0x1f) * 8
                    palette.append((r, g, b, 255))
    
    # Parse animation entries
    animations = []
    for i in range(min(anim_count, 100)):
        entry_off = 12 + i * 12
        if entry_off + 12 > len(sprite_data):
            break
        anim_entry = sprite_data[entry_off:entry_off+12]
        anim_id, frame_count, frame_off, flags, extra = struct.unpack('<IHHHH', anim_entry)
        animations.append({
            'id': anim_id,
            'frame_count': frame_count,
            'frame_offset': frame_off,
            'flags': flags,
        })
    
    return {
        'anim_count': anim_count,
        'frame_meta_offset': frame_meta_offset,
        'rle_data_offset': rle_data_offset,
        'palette_offset': palette_offset,
        'palette': palette,
        'animations': animations,
    }


def parse_frame_metadata(sprite_data: bytes, frame_meta_offset: int, frame_index: int) -> dict:
    """Parse frame metadata (36 bytes per frame)."""
    offset = frame_meta_offset + frame_index * 0x24
    if offset + 0x24 > len(sprite_data):
        return None
    
    frame = sprite_data[offset:offset+0x24]
    vals = struct.unpack('<HHHhhHHHHhhHHHHHI', frame)
    
    return {
        'x_offset': vals[3],
        'y_offset': vals[4],
        'width': vals[5],
        'height': vals[6],
        'rle_offset': vals[16],
    }


def create_sprite_sheet(sprite_data: bytes, header: dict, palette: List[Tuple]) -> Optional[Image.Image]:
    """Create a sprite sheet image from all frames."""
    all_frames = []
    max_width = 0
    max_height = 0
    
    for anim in header['animations']:
        for frame_idx in range(anim['frame_count']):
            frame_meta = parse_frame_metadata(
                sprite_data, 
                header['frame_meta_offset'], 
                frame_idx
            )
            if frame_meta is None:
                continue
            
            width = frame_meta['width']
            height = frame_meta['height']
            
            if width == 0 or height == 0 or width > 512 or height > 512:
                continue
            
            max_width = max(max_width, width)
            max_height = max(max_height, height)
            
            pixels = decode_rle_sprite(
                sprite_data,
                header['rle_data_offset'],
                frame_meta['rle_offset'],
                width,
                height
            )
            
            all_frames.append({
                'pixels': pixels,
                'width': width,
                'height': height,
            })
    
    if not all_frames:
        return None
    
    # Calculate sprite sheet dimensions
    frames_per_row = min(8, len(all_frames))
    rows = (len(all_frames) + frames_per_row - 1) // frames_per_row
    
    padding = 2
    sheet_width = frames_per_row * (max_width + padding) + padding
    sheet_height = rows * (max_height + padding) + padding
    
    img = Image.new('RGBA', (sheet_width, sheet_height), (0, 0, 0, 0))
    
    for i, frame in enumerate(all_frames):
        col = i % frames_per_row
        row = i // frames_per_row
        
        x_start = padding + col * (max_width + padding)
        y_start = padding + row * (max_height + padding)
        
        for y in range(frame['height']):
            for x in range(frame['width']):
                px_idx = frame['pixels'][y * frame['width'] + x]
                if px_idx > 0 and px_idx < len(palette):
                    img.putpixel((x_start + x, y_start + y), palette[px_idx])
    
    return img


def extract_sprites_600(primary_data: bytes, output_dir: Path, level_id: str) -> int:
    """Extract all sprites from Asset 600."""
    toc = parse_toc(primary_data)
    if ASSET_SPRITES not in toc:
        return 0
    
    _, _, asset_data = toc[ASSET_SPRITES]
    sprites = parse_sub_toc(asset_data)
    
    sprites_dir = output_dir / "sprites"
    sprites_dir.mkdir(parents=True, exist_ok=True)
    
    extracted = 0
    seen_offsets = set()
    
    # Default grayscale palette
    default_palette = [(i, i, i, 255 if i > 0 else 0) for i in range(256)]
    
    for sprite_id, size, offset, data in sprites:
        if offset in seen_offsets:
            continue
        seen_offsets.add(offset)
        
        try:
            header = parse_sprite_header(data, size)
            if header is None or header['anim_count'] == 0:
                continue
            
            palette = header['palette'] if header['palette'] else default_palette
            img = create_sprite_sheet(data, header, palette)
            
            if img is not None:
                filename = f"sprite_{sprite_id:08x}.png"
                img.save(sprites_dir / filename)
                extracted += 1
        except Exception as e:
            pass  # Skip problematic sprites
    
    return extracted


# ============================================================================
# TILE EXTRACTION (Asset 300)
# ============================================================================

def extract_tiles(secondary_data: bytes, output_dir: Path, level_id: str) -> int:
    """Extract all tiles from Asset 300 with palettes from Asset 400."""
    toc = parse_toc(secondary_data)
    
    if ASSET_TILE_PIXELS not in toc or ASSET_PALETTE not in toc:
        return 0
    
    _, _, pixel_data = toc[ASSET_TILE_PIXELS]
    _, _, palette_container = toc[ASSET_PALETTE]
    
    # Parse palette container
    palettes = []
    palette_entries = parse_sub_toc(palette_container)
    for _, _, _, pal_data in palette_entries:
        palettes.append(parse_palette_256(pal_data))
    
    if not palettes:
        return 0
    
    # Get palette index per tile (Asset 301)
    palette_indices = b''
    if ASSET_PALETTE_IDX in toc:
        _, _, palette_indices = toc[ASSET_PALETTE_IDX]
    
    # Get tile flags (Asset 302)
    flags_data = b''
    if ASSET_SIZE_FLAGS in toc:
        _, _, flags_data = toc[ASSET_SIZE_FLAGS]
    
    # Get tile header for counts
    n_16x16 = 0
    n_8x8 = 0
    if ASSET_HEADER in toc:
        _, _, header_data = toc[ASSET_HEADER]
        if len(header_data) >= 0x16:
            n_16x16 = struct.unpack('<H', header_data[0x10:0x12])[0]
            n_8x8 = struct.unpack('<H', header_data[0x12:0x14])[0]
    
    total_tiles = n_16x16 + n_8x8
    if total_tiles == 0:
        # Estimate from data size
        total_tiles = len(pixel_data) // 256
    
    tiles_dir = output_dir / "tiles"
    tiles_dir.mkdir(parents=True, exist_ok=True)
    
    extracted = 0
    tile_images = []
    
    for tile_idx in range(total_tiles):
        # Get tile properties
        is_8x8 = False
        if tile_idx < len(flags_data):
            is_8x8 = (flags_data[tile_idx] & 0x02) != 0
        
        tile_size = 8 if is_8x8 else 16
        
        # Calculate offset
        if is_8x8:
            offset = n_16x16 * 256 + (tile_idx - n_16x16) * 64
            bytes_per_tile = 64
        else:
            offset = tile_idx * 256
            bytes_per_tile = 256
        
        if offset + bytes_per_tile > len(pixel_data):
            continue
        
        tile_data = pixel_data[offset:offset+bytes_per_tile]
        
        # Get palette for this tile
        pal_idx = 0
        if tile_idx < len(palette_indices):
            pal_idx = palette_indices[tile_idx]
        pal_idx = min(pal_idx, len(palettes) - 1)
        palette = palettes[pal_idx]
        
        # Create tile image
        img = Image.new('RGBA', (tile_size, tile_size), (0, 0, 0, 0))
        for y in range(tile_size):
            for x in range(tile_size):
                px_idx = tile_data[y * tile_size + x] if (y * tile_size + x) < len(tile_data) else 0
                if px_idx > 0 and px_idx < len(palette):
                    img.putpixel((x, y), palette[px_idx])
        
        filename = f"tile_{tile_idx:04d}.png"
        img.save(tiles_dir / filename)
        tile_images.append(img)
        extracted += 1
    
    # Create combined tileset image
    if tile_images:
        tiles_per_row = 16
        rows = (len(tile_images) + tiles_per_row - 1) // tiles_per_row
        
        # Use max tile size (16x16)
        tileset = Image.new('RGBA', (tiles_per_row * 16, rows * 16), (0, 0, 0, 0))
        
        for i, tile_img in enumerate(tile_images):
            col = i % tiles_per_row
            row = i // tiles_per_row
            # Center smaller tiles
            x_off = (16 - tile_img.size[0]) // 2
            y_off = (16 - tile_img.size[1]) // 2
            tileset.paste(tile_img, (col * 16 + x_off, row * 16 + y_off))
        
        tileset.save(output_dir / "tileset.png")
    
    return extracted


# ============================================================================
# LAYER EXTRACTION (Asset 200/201)
# ============================================================================

def extract_layers(secondary_data: bytes, tertiary_data: bytes, output_dir: Path, level_id: str) -> int:
    """Extract rendered tilemap layers (layer defs in tertiary, tiles in secondary)."""
    sec_toc = parse_toc(secondary_data)
    tert_toc = parse_toc(tertiary_data) if tertiary_data else {}
    
    # Tiles are in secondary, layer definitions in tertiary
    required_sec = [ASSET_TILE_PIXELS, ASSET_PALETTE]
    required_tert = [ASSET_TILEMAP, ASSET_LAYER_ENTRIES]
    
    if not all(a in sec_toc for a in required_sec):
        return 0
    if not all(a in tert_toc for a in required_tert):
        return 0
    
    _, _, tilemap_data = tert_toc[ASSET_TILEMAP]
    _, _, layer_entries = tert_toc[ASSET_LAYER_ENTRIES]
    _, _, pixel_data = sec_toc[ASSET_TILE_PIXELS]
    _, _, palette_container = sec_toc[ASSET_PALETTE]
    
    # Parse palettes
    palettes = []
    for _, _, _, pal_data in parse_sub_toc(palette_container):
        palettes.append(parse_palette_256(pal_data))
    
    if not palettes:
        return 0
    
    # Parse tilemap sub-TOC to get layer data
    tilemap_entries = parse_sub_toc(tilemap_data)
    
    # Get layer count from sub-TOC
    layer_count = len(tilemap_entries)
    if layer_count == 0:
        return 0
    
    # Get tile info from secondary segment
    n_16x16 = 0
    palette_indices = b''
    flags_data = b''
    
    if ASSET_HEADER in sec_toc:
        _, _, header_data = sec_toc[ASSET_HEADER]
        if len(header_data) >= 0x16:
            n_16x16 = struct.unpack('<H', header_data[0x10:0x12])[0]
    
    if ASSET_PALETTE_IDX in sec_toc:
        _, _, palette_indices = sec_toc[ASSET_PALETTE_IDX]
    
    if ASSET_SIZE_FLAGS in sec_toc:
        _, _, flags_data = sec_toc[ASSET_SIZE_FLAGS]
    
    layers_dir = output_dir / "layers"
    layers_dir.mkdir(parents=True, exist_ok=True)
    
    extracted = 0
    
    # Each layer entry is 92 bytes
    LAYER_ENTRY_SIZE = 92
    
    for layer_idx in range(layer_count):
        entry_offset = layer_idx * LAYER_ENTRY_SIZE
        if entry_offset + LAYER_ENTRY_SIZE > len(layer_entries):
            continue
        
        entry = layer_entries[entry_offset:entry_offset+LAYER_ENTRY_SIZE]
        
        # Parse layer entry (92 bytes)
        # Offset 0x04: u16 src_width_tiles
        # Offset 0x06: u16 src_height_tiles
        # Offset 0x08: u16 dst_width
        # Offset 0x0A: u16 dst_height
        width_tiles = struct.unpack('<H', entry[4:6])[0]
        height_tiles = struct.unpack('<H', entry[6:8])[0]
        
        if width_tiles == 0 or height_tiles == 0:
            continue
        if width_tiles > 256 or height_tiles > 256:
            continue
        
        # Get tilemap data for this layer
        if layer_idx >= len(tilemap_entries):
            continue
        
        _, _, _, layer_tilemap = tilemap_entries[layer_idx]
        
        # Create layer image (16x16 per tile)
        layer_width = width_tiles * 16
        layer_height = height_tiles * 16
        
        if layer_width > 4096 or layer_height > 4096:
            continue
        
        img = Image.new('RGBA', (layer_width, layer_height), (0, 0, 0, 0))
        
        # Render tiles
        for ty in range(height_tiles):
            for tx in range(width_tiles):
                tile_map_idx = ty * width_tiles + tx
                if tile_map_idx * 2 + 2 > len(layer_tilemap):
                    continue
                
                tile_idx = struct.unpack('<H', layer_tilemap[tile_map_idx*2:tile_map_idx*2+2])[0]
                if tile_idx == 0:
                    continue
                
                tile_idx_0 = tile_idx - 1  # Convert to 0-based
                
                # Get tile properties
                is_8x8 = False
                if tile_idx_0 < len(flags_data):
                    is_8x8 = (flags_data[tile_idx_0] & 0x02) != 0
                
                tile_size = 8 if is_8x8 else 16
                
                # Calculate pixel offset
                if is_8x8:
                    offset = n_16x16 * 256 + (tile_idx_0 - n_16x16) * 64
                    bytes_per_tile = 64
                else:
                    offset = tile_idx_0 * 256
                    bytes_per_tile = 256
                
                if offset + bytes_per_tile > len(pixel_data):
                    continue
                
                tile_data = pixel_data[offset:offset+bytes_per_tile]
                
                # Get palette
                pal_idx = 0
                if tile_idx_0 < len(palette_indices):
                    pal_idx = palette_indices[tile_idx_0]
                pal_idx = min(pal_idx, len(palettes) - 1)
                palette = palettes[pal_idx]
                
                # Draw tile
                for py in range(tile_size):
                    for px in range(tile_size):
                        px_idx = tile_data[py * tile_size + px] if (py * tile_size + px) < len(tile_data) else 0
                        if px_idx > 0 and px_idx < len(palette):
                            img.putpixel((tx * 16 + px, ty * 16 + py), palette[px_idx])
        
        filename = f"layer_{layer_idx:02d}.png"
        img.save(layers_dir / filename)
        extracted += 1
    
    return extracted


# ============================================================================
# MAIN EXTRACTION
# ============================================================================

def extract_level(blb_path: str, level_idx: int, output_base: Path, blb_name: str) -> dict:
    """Extract all graphics from a single level."""
    try:
        meta = read_level_metadata(blb_path, level_idx)
    except Exception as e:
        return {'error': str(e)}
    
    level_dir = output_base / blb_name / f"{level_idx:02d}_{meta.level_id}"
    level_dir.mkdir(parents=True, exist_ok=True)
    
    results = {
        'level_id': meta.level_id,
        'level_name': meta.level_name,
        'sprites': 0,
        'tiles': 0,
        'layers': 0,
    }
    
    # Read segment data
    # NOTE: Verified via runtime analysis:
    # - Primary segment: Asset 600 (level geometry), 601 (collision), 602 (palette)
    # - Secondary segment: Assets 100, 300-302, 400-401 (tiles + metadata)
    # - Tertiary segment[0]: Assets 200/201 (layers), 500-504 (audio), 600 (SPRITES!), 700
    
    secondary_data = None
    if meta.secondary_count > 0:
        secondary_data = read_segment(blb_path, meta.secondary_sector, meta.secondary_count)
    
    tertiary_data = None
    if meta.tertiary_count > 0 and meta.tert_block_count > 0:
        tertiary_data = read_segment(blb_path, meta.tertiary_sector, meta.tertiary_count)
    
    # Extract sprites from TERTIARY segment (Asset 600 = RLE sprites there)
    if tertiary_data:
        results['sprites'] = extract_sprites_600(tertiary_data, level_dir, meta.level_id)
    
    # Extract tiles from secondary segment
    if secondary_data:
        results['tiles'] = extract_tiles(secondary_data, level_dir, meta.level_id)
    
    # Extract layers (uses tile data from secondary, layer definitions from tertiary)
    if secondary_data and tertiary_data:
        results['layers'] = extract_layers(secondary_data, tertiary_data, level_dir, meta.level_id)
    
    # Save metadata
    with open(level_dir / "metadata.json", 'w') as f:
        json.dump({
            'level_index': level_idx,
            'level_id': meta.level_id,
            'level_name': meta.level_name,
            'extracted': results,
        }, f, indent=2)
    
    return results


def main():
    parser = argparse.ArgumentParser(
        description='Extract all graphics from Skullmonkeys BLB files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    # Extract everything from all BLB files in disks/blb/
    python3 extract_all_graphics.py
    
    # Extract from specific BLB
    python3 extract_all_graphics.py disks/blb/GAME.BLB
    
    # Extract specific level only
    python3 extract_all_graphics.py disks/blb/GAME.BLB --level 2
    
    # Custom output directory
    python3 extract_all_graphics.py --output my_assets/
"""
    )
    parser.add_argument('blb_path', nargs='?', help='Path to BLB file (or all in disks/blb/)')
    parser.add_argument('--level', '-l', type=int, help='Extract specific level only')
    parser.add_argument('--output', '-o', default='extracted_graphics', help='Output directory')
    args = parser.parse_args()
    
    output_base = Path(args.output)
    output_base.mkdir(parents=True, exist_ok=True)
    
    # Find BLB files to process
    if args.blb_path:
        blb_files = [Path(args.blb_path)]
    else:
        blb_dir = Path('disks/blb')
        if not blb_dir.exists():
            print(f"Error: {blb_dir} not found. Specify a BLB file path.")
            sys.exit(1)
        blb_files = sorted(blb_dir.glob('*.blb')) + sorted(blb_dir.glob('*.BLB'))
    
    total_stats = {
        'blb_files': 0,
        'levels': 0,
        'sprites': 0,
        'tiles': 0,
        'layers': 0,
    }
    
    for blb_path in blb_files:
        if not blb_path.exists():
            print(f"Error: {blb_path} not found")
            continue
        
        blb_name = blb_path.stem
        print(f"\n{'='*60}")
        print(f"Processing: {blb_path}")
        print(f"{'='*60}")
        
        total_stats['blb_files'] += 1
        
        level_count = get_level_count(str(blb_path))
        
        if args.level is not None:
            levels_to_extract = [args.level]
        else:
            levels_to_extract = range(level_count)
        
        for level_idx in levels_to_extract:
            print(f"\n  Level {level_idx}...", end=' ', flush=True)
            
            results = extract_level(str(blb_path), level_idx, output_base, blb_name)
            
            if 'error' in results:
                print(f"ERROR: {results['error']}")
            else:
                print(f"{results['level_id']}: "
                      f"{results['sprites']} sprites, "
                      f"{results['tiles']} tiles, "
                      f"{results['layers']} layers")
                
                total_stats['levels'] += 1
                total_stats['sprites'] += results['sprites']
                total_stats['tiles'] += results['tiles']
                total_stats['layers'] += results['layers']
    
    print(f"\n{'='*60}")
    print(f"EXTRACTION COMPLETE")
    print(f"{'='*60}")
    print(f"  BLB files processed: {total_stats['blb_files']}")
    print(f"  Levels extracted:    {total_stats['levels']}")
    print(f"  Sprites extracted:   {total_stats['sprites']}")
    print(f"  Tiles extracted:     {total_stats['tiles']}")
    print(f"  Layers extracted:    {total_stats['layers']}")
    print(f"\nOutput directory: {output_base.absolute()}")


if __name__ == '__main__':
    main()
