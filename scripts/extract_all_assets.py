#!/usr/bin/env python3
"""
extract_all_assets.py - Extract ALL known/parseable assets from Skullmonkeys BLB

This script extracts all assets we can currently parse from the BLB file:

PRIMARY SEGMENT:
  - Asset 602 (0x25A): World palette (15-bit RGB)

SECONDARY SEGMENT (Background tiles):
  - Asset 100 (0x064): Tile header
  - Asset 300 (0x12C): Tile pixels (8bpp)
  - Asset 301 (0x12D): Palette index per tile  
  - Asset 302 (0x12E): Tile size flags
  - Asset 400 (0x190): Palette container
  - Asset 401 (0x191): Animation config
  - Asset 602 (0x25A): Secondary palette (if present)

TERTIARY SEGMENT (Sprites/Audio):
  - Asset 100 (0x064): Tertiary header
  - Asset 200 (0x0C8): Sprite index container
  - Asset 201 (0x0C9): Animation definitions
  - Asset 302 (0x12E): Sprite size flags
  - Asset 401 (0x191): Animation config
  - Asset 600 (0x258): Sprite pixels (RLE - raw dump for now)
  - Asset 700 (0x2BC): Audio data (raw dump)

Output structure:
    output/
    └── <level_id>/
        ├── primary/
        │   └── palette_602.bin
        ├── secondary/
        │   ├── tiles/
        │   │   ├── tile_0000.png
        │   │   └── ...
        │   ├── header_100.bin
        │   ├── palettes.png (palette visualization)
        │   └── tileset.png (all tiles combined)
        └── tertiary/
            └── block_0/
                ├── sprites/
                │   ├── sprite_00_raw.bin
                │   └── ...
                ├── header_100.bin
                ├── animation_201.bin
                └── audio_700.bin (if present)

Usage:
    python3 extract_all_assets.py <blb_file> [output_dir]
    python3 extract_all_assets.py disks/blb/GAME.BLB output/assets
"""

import struct
import sys
import json
from pathlib import Path
from dataclasses import dataclass, asdict
from typing import Optional, Dict, List, Tuple

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


# Asset type constants
ASSET_HEADER = 0x064       # 100
ASSET_HEADER_101 = 0x065   # 101
ASSET_SPRITE_INDEX = 0x0C8 # 200
ASSET_ANIM_DEF = 0x0C9     # 201
ASSET_TILE_PIXELS = 0x12C  # 300
ASSET_PALETTE_IDX = 0x12D  # 301
ASSET_SIZE_FLAGS = 0x12E   # 302
ASSET_PALETTE = 0x190      # 400
ASSET_ANIM_CFG = 0x191     # 401
ASSET_UNKNOWN_500 = 0x1F4  # 500
ASSET_VRAM_COORDS = 0x1F5  # 501
ASSET_VRAM_RECTS = 0x1F6   # 502
ASSET_FRAME_OFFS = 0x1F7   # 503
ASSET_LEVEL_GFX = 0x258    # 600
ASSET_COLLISION = 0x259    # 601
ASSET_RAW_PALETTE = 0x25A  # 602
ASSET_AUDIO = 0x2BC        # 700


def parse_toc(data: bytes) -> Dict[int, Tuple[int, int, bytes]]:
    """Parse a container TOC and return dict of asset_id -> (offset, size, data)."""
    if len(data) < 4:
        return {}
    
    count = struct.unpack('<I', data[0:4])[0]
    if count > 1000:  # Sanity check
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
    """Parse a sub-container TOC. Returns list of (flags, size, offset, data)."""
    if len(data) < 4:
        return []
    
    count = struct.unpack('<I', data[0:4])[0]
    if count > 1000:  # Sanity check
        return []
    
    entries = []
    for i in range(count):
        off = 4 + i * 12
        if off + 12 > len(data):
            break
        flags, size, offset = struct.unpack('<III', data[off:off+12])
        if offset + size <= len(data):
            entries.append((flags, size, offset, data[offset:offset+size]))
    
    return entries


def psx_color_to_rgb(color: int) -> Tuple[int, int, int]:
    """Convert PSX 15-bit color to RGB tuple."""
    r = (color & 0x1F) * 8
    g = ((color >> 5) & 0x1F) * 8
    b = ((color >> 10) & 0x1F) * 8
    return (r, g, b)


def psx_color_to_rgba(color: int) -> Tuple[int, int, int, int]:
    """Convert PSX 15-bit color to RGBA tuple (color 0 = transparent)."""
    if color == 0:
        return (0, 0, 0, 0)
    r = (color & 0x1F) * 8
    g = ((color >> 5) & 0x1F) * 8
    b = ((color >> 10) & 0x1F) * 8
    return (r, g, b, 255)


def parse_palette(data: bytes) -> List[Tuple[int, int, int, int]]:
    """Parse a 256-color PSX palette (512 bytes) to RGBA tuples."""
    colors = []
    for i in range(min(256, len(data) // 2)):
        color = struct.unpack('<H', data[i*2:i*2+2])[0]
        colors.append(psx_color_to_rgba(color))
    return colors


@dataclass
class TileHeader:
    """Header structure from asset 100 (36 bytes)."""
    bg_color_r: int
    bg_color_g: int
    bg_color_b: int
    alt_color_r: int
    alt_color_g: int
    alt_color_b: int
    level_width: int
    level_height: int
    count_16x16: int
    count_8x8: int
    count_extra: int
    raw_data: bytes = None
    
    @property
    def total_tiles(self) -> int:
        return self.count_16x16 + self.count_8x8 + self.count_extra
    
    @classmethod
    def from_bytes(cls, data: bytes) -> "TileHeader":
        if len(data) < 0x16:
            raise ValueError(f"Header too short: {len(data)} bytes")
        
        return cls(
            bg_color_r=data[0],
            bg_color_g=data[1],
            bg_color_b=data[2],
            alt_color_r=data[4],
            alt_color_g=data[5],
            alt_color_b=data[6],
            level_width=struct.unpack('<H', data[0x08:0x0A])[0],
            level_height=struct.unpack('<H', data[0x0A:0x0C])[0],
            count_16x16=struct.unpack('<H', data[0x10:0x12])[0],
            count_8x8=struct.unpack('<H', data[0x12:0x14])[0],
            count_extra=struct.unpack('<H', data[0x14:0x16])[0],
            raw_data=data,
        )


def extract_tiles(
    header: TileHeader,
    pixel_data: bytes,
    palette_indices: bytes,
    size_flags: bytes,
    palettes: List[List[Tuple[int, int, int, int]]],
    output_dir: Path,
    save_individual: bool = False
) -> Optional[Image.Image]:
    """Extract tiles and create tileset image."""
    if not HAS_PIL:
        return None
    
    tiles = []
    pixel_offset = 0
    
    # Extract 16x16 tiles first
    for i in range(header.count_16x16):
        if pixel_offset + 256 > len(pixel_data):
            break
        
        tile_pixels = pixel_data[pixel_offset:pixel_offset+256]
        pixel_offset += 256
        
        # Get palette for this tile
        pal_idx = palette_indices[i] if i < len(palette_indices) else 0
        if pal_idx >= len(palettes):
            pal_idx = 0
        palette = palettes[pal_idx]
        
        # Create tile image
        img = Image.new('RGBA', (16, 16))
        for y in range(16):
            for x in range(16):
                idx = y * 16 + x
                if idx < len(tile_pixels):
                    color_idx = tile_pixels[idx]
                    if color_idx < len(palette):
                        img.putpixel((x, y), palette[color_idx])
        
        tiles.append(img)
        
        if save_individual:
            img.save(output_dir / f"tile_{i:04d}.png")
    
    # Extract 8x8 tiles (stored with 16-byte stride, only 8 bytes used)
    for i in range(header.count_8x8):
        tile_idx = header.count_16x16 + i
        if pixel_offset + 128 > len(pixel_data):
            break
        
        tile_pixels = pixel_data[pixel_offset:pixel_offset+128]
        pixel_offset += 128
        
        # Get palette for this tile
        pal_idx = palette_indices[tile_idx] if tile_idx < len(palette_indices) else 0
        if pal_idx >= len(palettes):
            pal_idx = 0
        palette = palettes[pal_idx]
        
        # Create tile image (8x8 with 16-byte stride)
        img = Image.new('RGBA', (8, 8))
        for y in range(8):
            for x in range(8):
                idx = y * 16 + x  # 16-byte stride
                if idx < len(tile_pixels):
                    color_idx = tile_pixels[idx]
                    if color_idx < len(palette):
                        img.putpixel((x, y), palette[color_idx])
        
        tiles.append(img)
        
        if save_individual:
            img.save(output_dir / f"tile_{tile_idx:04d}.png")
    
    # Create combined tileset image
    if tiles:
        tiles_per_row = 32
        rows = (len(tiles) + tiles_per_row - 1) // tiles_per_row
        tileset = Image.new('RGBA', (tiles_per_row * 16, rows * 16), (0, 0, 0, 0))
        
        for i, tile in enumerate(tiles):
            x = (i % tiles_per_row) * 16
            y = (i // tiles_per_row) * 16
            # Resize 8x8 tiles to 16x16 for uniform grid
            if tile.size == (8, 8):
                tile = tile.resize((16, 16), Image.NEAREST)
            tileset.paste(tile, (x, y))
        
        return tileset
    
    return None


def create_palette_image(palettes: List[List[Tuple[int, int, int, int]]]) -> Optional[Image.Image]:
    """Create a visualization of all palettes."""
    if not HAS_PIL or not palettes:
        return None
    
    # Each palette is 256 colors, show as 16x16 grid
    pal_width = 16 * 8  # 16 colors wide, 8 pixels each
    pal_height = 16 * 8  # 16 colors tall
    
    img = Image.new('RGBA', (pal_width, pal_height * len(palettes)))
    
    for p_idx, palette in enumerate(palettes):
        y_base = p_idx * pal_height
        for c_idx, color in enumerate(palette[:256]):
            cx = (c_idx % 16) * 8
            cy = (c_idx // 16) * 8 + y_base
            for dy in range(8):
                for dx in range(8):
                    img.putpixel((cx + dx, cy + dy), color)
    
    return img


def extract_level(blb: BLBFile, level_idx: int, output_base: Path, verbose: bool = True):
    """Extract all assets for a single level."""
    raw = blb.header.raw_data
    base = level_idx * 0x70
    
    level_id = raw[base+0x56:base+0x5A].decode('ascii', errors='ignore').strip('\x00')
    level_name = raw[base+0x5B:base+0x70].decode('ascii', errors='ignore').strip('\x00')
    
    output_dir = output_base / f"{level_idx:02d}_{level_id}"
    output_dir.mkdir(parents=True, exist_ok=True)
    
    if verbose:
        print(f"\n{'='*60}")
        print(f"Level {level_idx}: {level_id} - {level_name}")
        print(f"{'='*60}")
    
    stats = {
        "level_idx": level_idx,
        "level_id": level_id,
        "level_name": level_name,
        "primary": {},
        "secondary": {},
        "tertiary": []
    }
    
    # =========================================================================
    # PRIMARY SEGMENT
    # =========================================================================
    prim_off = struct.unpack('<H', raw[base+0x00:base+0x02])[0]
    prim_cnt = struct.unpack('<H', raw[base+0x02:base+0x04])[0]
    
    if prim_cnt > 0:
        prim_dir = output_dir / "primary"
        prim_dir.mkdir(exist_ok=True)
        
        try:
            prim_data = blb.read_sectors(prim_off, prim_cnt)
            prim_assets = parse_toc(prim_data)
            
            # Extract raw palette (602)
            if ASSET_RAW_PALETTE in prim_assets:
                _, size, data = prim_assets[ASSET_RAW_PALETTE]
                (prim_dir / "palette_602.bin").write_bytes(data)
                stats["primary"]["palette_602"] = size
                
                if HAS_PIL and size >= 2:
                    # Create palette visualization
                    colors = parse_palette(data)
                    if colors:
                        img = Image.new('RGBA', (16 * 8, (len(colors) // 16 + 1) * 8))
                        for i, c in enumerate(colors):
                            x = (i % 16) * 8
                            y = (i // 16) * 8
                            for dy in range(8):
                                for dx in range(8):
                                    img.putpixel((x + dx, y + dy), c)
                        img.save(prim_dir / "palette_602.png")
            
            if verbose:
                print(f"  PRIMARY: {len(prim_assets)} assets")
                for aid, (off, size, _) in sorted(prim_assets.items()):
                    print(f"    Asset {aid:3} (0x{aid:03X}): {size:>8} bytes")
            
            stats["primary"]["asset_count"] = len(prim_assets)
            stats["primary"]["assets"] = {f"0x{k:03X}": v[1] for k, v in prim_assets.items()}
            
        except Exception as e:
            if verbose:
                print(f"  PRIMARY: Error - {e}")
    
    # =========================================================================
    # SECONDARY SEGMENT (Background tiles)
    # =========================================================================
    sec_off = struct.unpack('<H', raw[base+0x1E:base+0x20])[0]
    sec_cnt = struct.unpack('<H', raw[base+0x2C:base+0x2E])[0]
    
    if sec_cnt > 0:
        sec_dir = output_dir / "secondary"
        sec_dir.mkdir(exist_ok=True)
        tiles_dir = sec_dir / "tiles"
        tiles_dir.mkdir(exist_ok=True)
        
        try:
            sec_data = blb.read_sectors(sec_off, sec_cnt)
            sec_assets = parse_toc(sec_data)
            
            header = None
            palettes = []
            
            # Extract header (100)
            if ASSET_HEADER in sec_assets:
                _, size, data = sec_assets[ASSET_HEADER]
                (sec_dir / "header_100.bin").write_bytes(data)
                header = TileHeader.from_bytes(data)
                stats["secondary"]["header"] = asdict(header)
                del stats["secondary"]["header"]["raw_data"]
            
            # Extract palettes (400)
            if ASSET_PALETTE in sec_assets:
                _, size, data = sec_assets[ASSET_PALETTE]
                (sec_dir / "palettes_400.bin").write_bytes(data)
                
                # Parse palette container
                pal_entries = parse_sub_toc(data)
                for flags, pal_size, offset, pal_data in pal_entries:
                    if pal_size >= 512:
                        palettes.append(parse_palette(pal_data[:512]))
                
                stats["secondary"]["palette_count"] = len(palettes)
                
                # Create palette visualization
                if HAS_PIL and palettes:
                    pal_img = create_palette_image(palettes)
                    if pal_img:
                        pal_img.save(sec_dir / "palettes.png")
            
            # Extract tiles if we have all required assets
            if (header and palettes and 
                ASSET_TILE_PIXELS in sec_assets and 
                ASSET_PALETTE_IDX in sec_assets):
                
                _, _, pixel_data = sec_assets[ASSET_TILE_PIXELS]
                _, _, palette_indices = sec_assets[ASSET_PALETTE_IDX]
                _, _, size_flags = sec_assets.get(ASSET_SIZE_FLAGS, (0, 0, b''))
                
                # Save raw data
                (sec_dir / "pixels_300.bin").write_bytes(pixel_data)
                (sec_dir / "palette_idx_301.bin").write_bytes(palette_indices)
                if size_flags:
                    (sec_dir / "size_flags_302.bin").write_bytes(size_flags)
                
                # Extract tiles as images
                tileset = extract_tiles(
                    header, pixel_data, palette_indices, size_flags,
                    palettes, tiles_dir, save_individual=False
                )
                
                if tileset:
                    tileset.save(sec_dir / "tileset.png")
                    stats["secondary"]["tileset_size"] = tileset.size
                
                stats["secondary"]["tiles_16x16"] = header.count_16x16
                stats["secondary"]["tiles_8x8"] = header.count_8x8
            
            # Save animation config (401)
            if ASSET_ANIM_CFG in sec_assets:
                _, size, data = sec_assets[ASSET_ANIM_CFG]
                (sec_dir / "anim_config_401.bin").write_bytes(data)
                stats["secondary"]["anim_config_size"] = size
            
            if verbose:
                print(f"  SECONDARY: {len(sec_assets)} assets")
                if header:
                    print(f"    Tiles: {header.count_16x16} (16x16) + {header.count_8x8} (8x8)")
                    print(f"    Palettes: {len(palettes)}")
            
            stats["secondary"]["asset_count"] = len(sec_assets)
            
        except Exception as e:
            if verbose:
                print(f"  SECONDARY: Error - {e}")
            import traceback
            traceback.print_exc()
    
    # =========================================================================
    # TERTIARY SEGMENT (Sprites/Audio)
    # =========================================================================
    tert_offs = struct.unpack('<6H', raw[base+0x3A:base+0x46])
    tert_cnts = struct.unpack('<6H', raw[base+0x48:base+0x54])
    
    for block_idx in range(6):
        if tert_cnts[block_idx] == 0:
            continue
        
        tert_dir = output_dir / "tertiary" / f"block_{block_idx}"
        tert_dir.mkdir(parents=True, exist_ok=True)
        sprites_dir = tert_dir / "sprites"
        sprites_dir.mkdir(exist_ok=True)
        
        block_stats = {"block_idx": block_idx}
        
        try:
            tert_data = blb.read_sectors(tert_offs[block_idx], tert_cnts[block_idx])
            tert_assets = parse_toc(tert_data)
            
            # Extract header (100)
            if ASSET_HEADER in tert_assets:
                _, size, data = tert_assets[ASSET_HEADER]
                (tert_dir / "header_100.bin").write_bytes(data)
                block_stats["header_size"] = size
            
            # Extract sprite index (200)
            if ASSET_SPRITE_INDEX in tert_assets:
                _, size, data = tert_assets[ASSET_SPRITE_INDEX]
                (tert_dir / "sprite_index_200.bin").write_bytes(data)
                
                # Parse sprite index
                sprite_entries = parse_sub_toc(data)
                block_stats["sprite_count"] = len(sprite_entries)
                
                # Save each sprite entry
                for i, (flags, spr_size, offset, spr_data) in enumerate(sprite_entries):
                    (sprites_dir / f"sprite_{i:02d}.bin").write_bytes(spr_data)
            
            # Extract animation definitions (201)
            if ASSET_ANIM_DEF in tert_assets:
                _, size, data = tert_assets[ASSET_ANIM_DEF]
                (tert_dir / "animation_201.bin").write_bytes(data)
                block_stats["animation_size"] = size
            
            # Extract sprite graphics (600)
            if ASSET_LEVEL_GFX in tert_assets:
                _, size, data = tert_assets[ASSET_LEVEL_GFX]
                (tert_dir / "sprite_gfx_600.bin").write_bytes(data)
                
                # Parse sprite graphics container
                gfx_entries = parse_sub_toc(data)
                block_stats["sprite_gfx_count"] = len(gfx_entries)
                
                # Save each sprite's raw RLE data
                for i, (flags, gfx_size, offset, gfx_data) in enumerate(gfx_entries):
                    (sprites_dir / f"sprite_{i:02d}_gfx.bin").write_bytes(gfx_data)
            
            # Extract VRAM coordinates (501, 502)
            if ASSET_VRAM_COORDS in tert_assets:
                _, size, data = tert_assets[ASSET_VRAM_COORDS]
                (tert_dir / "vram_coords_501.bin").write_bytes(data)
                block_stats["vram_coords_size"] = size
            
            if ASSET_VRAM_RECTS in tert_assets:
                _, size, data = tert_assets[ASSET_VRAM_RECTS]
                (tert_dir / "vram_rects_502.bin").write_bytes(data)
                block_stats["vram_rects_size"] = size
            
            # Extract animation frame offsets (503)
            if ASSET_FRAME_OFFS in tert_assets:
                _, size, data = tert_assets[ASSET_FRAME_OFFS]
                (tert_dir / "frame_offsets_503.bin").write_bytes(data)
                block_stats["frame_offsets_size"] = size
            
            # Extract animation config (401)
            if ASSET_ANIM_CFG in tert_assets:
                _, size, data = tert_assets[ASSET_ANIM_CFG]
                (tert_dir / "anim_config_401.bin").write_bytes(data)
                block_stats["anim_config_size"] = size
            
            # Extract audio (700)
            if ASSET_AUDIO in tert_assets:
                _, size, data = tert_assets[ASSET_AUDIO]
                (tert_dir / "audio_700.bin").write_bytes(data)
                block_stats["audio_size"] = size
            
            if verbose:
                print(f"  TERTIARY block {block_idx}: {len(tert_assets)} assets")
                for aid, (off, size, _) in sorted(tert_assets.items()):
                    print(f"    Asset {aid:3} (0x{aid:03X}): {size:>8} bytes")
            
            block_stats["asset_count"] = len(tert_assets)
            block_stats["assets"] = {f"0x{k:03X}": v[1] for k, v in tert_assets.items()}
            
        except Exception as e:
            if verbose:
                print(f"  TERTIARY block {block_idx}: Error - {e}")
            block_stats["error"] = str(e)
        
        stats["tertiary"].append(block_stats)
    
    # Save stats JSON
    (output_dir / "extraction_stats.json").write_text(
        json.dumps(stats, indent=2, default=str)
    )
    
    return stats


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    blb_path = Path(sys.argv[1])
    output_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("output/assets")
    
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}")
        sys.exit(1)
    
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Extracting assets from: {blb_path}")
    print(f"Output directory: {output_dir}")
    
    all_stats = []
    
    with BLBFile(str(blb_path)) as blb:
        # Get level count
        level_count = blb.header.raw_data[0xF31]
        print(f"Found {level_count} levels")
        
        for level_idx in range(level_count):
            try:
                stats = extract_level(blb, level_idx, output_dir, verbose=True)
                all_stats.append(stats)
            except Exception as e:
                print(f"Error extracting level {level_idx}: {e}")
                import traceback
                traceback.print_exc()
    
    # Save overall stats
    (output_dir / "all_stats.json").write_text(
        json.dumps(all_stats, indent=2, default=str)
    )
    
    print(f"\n{'='*60}")
    print(f"Extraction complete!")
    print(f"Output: {output_dir}")
    print(f"{'='*60}")


if __name__ == "__main__":
    main()
