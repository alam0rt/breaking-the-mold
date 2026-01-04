#!/usr/bin/env python3
"""
extract_rle_sprites.py - Extract RLE-compressed sprites from Skullmonkeys BLB

Sprite Format (Asset 600 in PRIMARY and TERTIARY segments):
===========================================================

Container Structure:
    - u32 count: Number of sprites
    - 12-byte entries × count:
        - u32 sprite_id
        - u32 size  
        - u32 offset (from container start)

Sprite Header (24 bytes):
    - u16 magic: Always 1
    - u16 header_size: Always 24 (0x18)
    - u16 frame_meta_size: Total bytes for frame metadata
    - u16 padding
    - u32 pixel_size: Total bytes of RLE pixel data
    - u32 sprite_id: Same as TOC ID
    - u16 frame_count: Number of animation frames
    - u16 unknown_12
    - u16 unknown_14
    - u16 unknown_16

Frame Metadata (36 bytes per frame):
    - Bytes 0x0A-0x0B: u16 width
    - Bytes 0x0C-0x0D: u16 height
    - Bytes 0x20-0x23: u32 pixel_offset (from pixel data start)

RLE Control Word (u16):
    - Bit 15 (0x8000): New row flag
    - Bits 8-14 (0x7F00): Skip count (transparent pixels)
    - Bits 0-7 (0x00FF): Copy count (pixels to copy)

Based on DecodeRLESprite (0x80010068) from Ghidra analysis.
The game was built using ToolX, a proprietary animation scripting tool
that output "sequence files" read by the game engine.

Usage:
    python3 scripts/extract_rle_sprites.py [--level N] [--output DIR]
"""

import argparse
import struct
import sys
from pathlib import Path
from dataclasses import dataclass

# Add scripts directory to path for blb module
sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile, parse_container_toc

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: PIL not available, will output raw data only")


@dataclass
class SpriteHeader:
    """
    Sprite header (24 or 36 bytes).
    
    VERIFIED 2026-01-04 via Ghidra analysis and data verification:
    - type: 1=standard (24-byte header), 2=extended (36-byte header)
    - header_size: 24 for type 1, 36 for type 2
    - frames_end: header_size + total_frame_count × 36
    - Frame entry size: ALWAYS 36 bytes for both types
    - Type 2: total frames = frame_count + extra_frames (offset 0x1C)
    - clut (0x815D): GARBAGE - game loads palettes separately
    """
    sprite_type: int        # 0x00: 1=standard, 2=extended
    header_size: int        # 0x02: 24 or 36
    frames_end: int         # 0x04: offset to RLE data
    padding: int            # 0x06: always 0
    rle_size: int           # 0x08: approximate RLE data size (u32)
    sprite_id: int          # 0x0C: sprite ID (u32)
    frame_count: int        # 0x10: number of primary frames
    padding2: int           # 0x12: always 0
    unknown_flag: int       # 0x14: 0 or 1
    clut: int               # 0x16: always 0x815D (GARBAGE)
    # Type 2 extended fields (only present if header_size == 36)
    extra_frames: int = 0   # 0x1C: additional frame count for type 2
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'SpriteHeader':
        if len(data) < 24:
            raise ValueError(f"Sprite header too short: {len(data)} bytes")
        
        fields = struct.unpack('<HHHH I I HHHH', data[:24])
        header = cls(
            sprite_type=fields[0],
            header_size=fields[1],
            frames_end=fields[2],
            padding=fields[3],
            rle_size=fields[4],
            sprite_id=fields[5],
            frame_count=fields[6],
            padding2=fields[7],
            unknown_flag=fields[8],
            clut=fields[9],
        )
        
        # Parse extended header for type 2 sprites
        if header.sprite_type == 2 and header.header_size == 36 and len(data) >= 36:
            header.extra_frames = struct.unpack('<H', data[0x1C:0x1E])[0]
        
        return header
    
    @property
    def total_frame_count(self) -> int:
        """Total number of 36-byte frame entries."""
        if self.sprite_type == 2:
            return self.frame_count + self.extra_frames
        return self.frame_count
    
    @property
    def frame_entry_size(self) -> int:
        """Frame entry size is ALWAYS 36 bytes."""
        return 36


@dataclass  
class FrameMetadata:
    """Frame metadata (36 bytes per frame)"""
    raw_bytes: bytes
    width: int
    height: int
    pixel_offset: int
    
    # Exposure sheet / animation fields
    origin_x: int = 0        # +0x06: X origin offset (signed)
    origin_y: int = 0        # +0x08: Y origin offset (signed)
    hitbox_x: int = 0        # +0x12: Hitbox X offset (signed)
    hitbox_y: int = 0        # +0x14: Hitbox Y offset (signed)
    hitbox_w: int = 0        # +0x16: Hitbox width
    hitbox_h: int = 0        # +0x18: Hitbox height
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'FrameMetadata':
        if len(data) < 36:
            raise ValueError(f"Frame metadata too short: {len(data)} bytes")
        
        # Known fields from Ghidra analysis (GetFrameMetadata @ 0x8007bebc)
        width = struct.unpack('<H', data[0x0A:0x0C])[0]
        height = struct.unpack('<H', data[0x0C:0x0E])[0]
        pixel_offset = struct.unpack('<I', data[0x20:0x24])[0]
        
        # Animation offset fields (signed i16)
        origin_x = struct.unpack('<h', data[0x06:0x08])[0]
        origin_y = struct.unpack('<h', data[0x08:0x0A])[0]
        hitbox_x = struct.unpack('<h', data[0x12:0x14])[0]
        hitbox_y = struct.unpack('<h', data[0x14:0x16])[0]
        hitbox_w = struct.unpack('<H', data[0x16:0x18])[0]
        hitbox_h = struct.unpack('<H', data[0x18:0x1A])[0]
        
        return cls(
            raw_bytes=data,
            width=width,
            height=height,
            pixel_offset=pixel_offset,
            origin_x=origin_x,
            origin_y=origin_y,
            hitbox_x=hitbox_x,
            hitbox_y=hitbox_y,
            hitbox_w=hitbox_w,
            hitbox_h=hitbox_h,
        )


def decode_rle_sprite(rle_data: bytes, width: int, height: int) -> list[list[int]]:
    """
    Decode RLE-compressed sprite data.
    
    Based on DecodeRLESprite (0x80010068) and GetFrameMetadata (0x8007bebc).
    
    RLE Format:
        u16 control_count      - Number of control words
        u16[control_count]     - Control words
        u8[]                   - Pixel data (indexed)
    
    Control word format:
        Bit 15 (0x8000): New row flag - advance to next row
        Bits 8-14 (0x7F00 >> 8): Skip count - transparent pixels
        Bits 0-7 (0x00FF): Copy count - pixels to copy
    
    Args:
        rle_data: Raw RLE data starting with control count
        width: Frame width in pixels
        height: Frame height in pixels
        
    Returns:
        2D array of palette indices (0 = transparent)
    """
    # Initialize output buffer with transparent pixels
    output = [[0] * width for _ in range(height)]
    
    if len(rle_data) < 2 or width == 0 or height == 0:
        return output
    
    # First u16 = control count
    control_count = struct.unpack('<H', rle_data[0:2])[0]
    
    if len(rle_data) < 2 + control_count * 2:
        return output
    
    # Control words start at offset 2
    control_start = 2
    # Pixel data starts after all control words
    pixel_start = 2 + control_count * 2
    
    row = 0
    col = 0
    pixel_pos = pixel_start
    first_ctrl = True
    
    for i in range(control_count):
        ctrl = struct.unpack('<H', rle_data[control_start + i*2:control_start + i*2 + 2])[0]
        
        newline = (ctrl & 0x8000) != 0
        skip = (ctrl >> 8) & 0x7F
        copy = ctrl & 0xFF
        
        # Handle newline flag (advance to next row)
        if newline and not first_ctrl:
            row += 1
            col = 0
        first_ctrl = False
        
        if row >= height:
            break
        
        # Skip transparent pixels
        col += skip
        
        # Copy pixels from pixel data section
        for _ in range(copy):
            if pixel_pos < len(rle_data) and col < width and row < height:
                output[row][col] = rle_data[pixel_pos]
                col += 1
            pixel_pos += 1
    
    return output


def extract_sprite(sprite_data: bytes) -> dict:
    """
    Extract a single sprite from its data blob.
    
    VERIFIED structure (2026-01-04):
    - Sprite type 1: 24-byte header, frame_count × 36-byte frame entries
    - Sprite type 2: 36-byte header, (frame_count + extra_frames) × 36-byte frame entries
    - frames_end = header_size + total_frame_count × 36
    - RLE data starts at frames_end
    
    Args:
        sprite_data: Raw sprite data (header + frame metadata + RLE pixels)
        
    Returns:
        Dict with sprite info and decoded frames
    """
    header = SpriteHeader.from_bytes(sprite_data)
    
    if header.sprite_type not in (1, 2):
        raise ValueError(f"Invalid sprite type: {header.sprite_type}, expected 1 or 2")
    
    # Frame entry size is ALWAYS 36 bytes
    frame_entry_size = 36
    
    frames = []
    frame_meta_start = header.header_size
    
    # Pixel data starts at frames_end
    pixel_data_start = header.frames_end
    pixel_data = sprite_data[pixel_data_start:]
    
    # Use total_frame_count which includes extra_frames for type 2
    for i in range(header.total_frame_count):
        frame_offset = frame_meta_start + i * frame_entry_size
        frame_bytes = sprite_data[frame_offset:frame_offset + 36]
        
        if len(frame_bytes) < 36:
            continue
            
        frame = FrameMetadata.from_bytes(frame_bytes)
        
        # Sanity check dimensions
        if frame.width <= 0 or frame.height <= 0 or frame.width > 512 or frame.height > 512:
            print(f"    Frame {i}: Invalid dimensions {frame.width}x{frame.height}")
            continue
        
        # Get RLE data for this frame
        if frame.pixel_offset < len(pixel_data):
            frame_rle = pixel_data[frame.pixel_offset:]
        else:
            frame_rle = b''
        
        try:
            pixels = decode_rle_sprite(frame_rle, frame.width, frame.height)
        except Exception as e:
            print(f"    Frame {i} decode error: {e}")
            pixels = [[0] * frame.width for _ in range(frame.height)]
        
        frames.append({
            'index': i,
            'width': frame.width,
            'height': frame.height,
            'origin_x': frame.origin_x,
            'origin_y': frame.origin_y,
            'hitbox': (frame.hitbox_x, frame.hitbox_y, frame.hitbox_w, frame.hitbox_h),
            'pixel_offset': frame.pixel_offset,
            'pixels': pixels,
            'raw_meta': frame.raw_bytes.hex(),
        })
    
    return {
        'header': {
            'sprite_type': header.sprite_type,
            'header_size': header.header_size,
            'frames_end': header.frames_end,
            'rle_size': header.rle_size,
            'sprite_id': header.sprite_id,
            'frame_count': header.frame_count,
            'extra_frames': header.extra_frames,
            'total_frame_count': header.total_frame_count,
            'frame_entry_size': frame_entry_size,
            'unknown_flag': header.unknown_flag,
            'clut': header.clut,
        },
        'frames': frames,
    }


def sprite_to_image(frame_pixels: list[list[int]], palette: list[tuple], 
                    transparent_index: int = 0) -> 'Image.Image':
    """Convert decoded sprite pixels to PIL Image."""
    if not HAS_PIL:
        return None
        
    height = len(frame_pixels)
    width = len(frame_pixels[0]) if height > 0 else 0
    
    if width == 0 or height == 0:
        return None
    
    # Create RGBA image
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    
    for y, row in enumerate(frame_pixels):
        for x, idx in enumerate(row):
            if idx == transparent_index:
                continue  # Leave transparent
            if idx < len(palette):
                r, g, b = palette[idx]
                img.putpixel((x, y), (r, g, b, 255))
            else:
                # Out of range - show as magenta
                img.putpixel((x, y), (255, 0, 255, 255))
    
    return img


def parse_psx_palette(data: bytes, count: int = 256) -> list[tuple]:
    """Parse PSX 15-bit palette to RGB tuples."""
    palette = []
    for i in range(min(count, len(data) // 2)):
        color = struct.unpack('<H', data[i*2:i*2+2])[0]
        r = (color & 0x1F) << 3
        g = ((color >> 5) & 0x1F) << 3
        b = ((color >> 10) & 0x1F) << 3
        palette.append((r, g, b))
    
    # Pad to 256 colors
    while len(palette) < 256:
        palette.append((0, 0, 0))
    
    return palette


def decode_clut_value(clut: int) -> tuple[int, int, int]:
    """
    Decode a PSX CLUT value to VRAM coordinates and palette index.
    
    The CLUT value encodes a VRAM position where the palette is loaded:
        X = (clut & 0x3F) << 4  (low 6 bits * 16)
        Y = clut >> 6           (upper 10 bits)
    
    For this game, palettes are loaded starting at Y=512 in VRAM,
    so the palette index = Y - 512 + 1 (1-indexed).
    
    Args:
        clut: CLUT value from sprite header (unknown_16)
        
    Returns:
        Tuple of (vram_x, vram_y, palette_index)
    """
    vram_x = (clut & 0x3F) << 4
    vram_y = clut >> 6
    
    # Calculate palette index (assuming palettes start at Y=512)
    palette_idx = vram_y - 512 + 1 if vram_y >= 512 else 1
    
    return (vram_x, vram_y, palette_idx)


def parse_asset_400_palettes(data: bytes) -> list[list[tuple]]:
    """
    Parse Asset 400 palette container.
    
    Structure (standard container TOC):
        u32 count           - Number of palette entries (typically 8)
        12-byte entries × count:
            - u32 id        - Palette index (0-7)
            - u32 size      - Always 512 (256 colors × 2 bytes)
            - u32 offset    - Offset from container start
        Palette data at offsets (512 bytes each = 256 × 16-bit PSX colors)
    
    Note: Only some palettes are full 256-color palettes for sprites.
    Others are 16-color tile palettes (still 512 bytes but mostly empty).
    This function identifies "full" palettes by counting non-black colors.
        
    Returns:
        Tuple of (all_palettes, full_palette_indices):
            - all_palettes: List of all palettes
            - full_palette_indices: List of indices that are full 256-color palettes
    """
    if len(data) < 8:
        return [], []
    
    count = struct.unpack('<I', data[0:4])[0]
    
    if count == 0 or count > 16:  # Sanity check
        return [], []
    
    palettes = []
    full_indices = []
    
    for i in range(count):
        entry_off = 4 + i * 12
        if entry_off + 12 > len(data):
            break
            
        pal_id, size, offset = struct.unpack('<III', data[entry_off:entry_off + 12])
        
        if size != 512 or offset + 512 > len(data):
            # Not a valid 256-color palette
            palettes.append([(0, 0, 0)] * 256)
            continue
        
        # Parse the palette at this offset
        palette = parse_psx_palette(data[offset:offset + 512], 256)
        palettes.append(palette)
        
        # Check if this is a "full" palette (many non-black colors)
        non_black = sum(1 for c in palette if c != (0, 0, 0) and c != (0, 0, 8))
        if non_black > 50:  # Threshold for "full" palette
            full_indices.append(i)
    
    return palettes, full_indices


def get_palette_for_level(blb: BLBFile, level_idx: int, palette_idx: int = 1) -> list[tuple]:
    """
    Get a palette from a level's secondary segment.
    
    Args:
        blb: BLB file handle
        level_idx: Level index
        palette_idx: Palette index (1-7) from Asset 400, or 0 for auto
        
    Returns:
        List of 256 RGB tuples
    """
    level = blb.header.get_level_by_index(level_idx)
    
    # Read secondary segment
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    sec_entries = parse_container_toc(sec_data)
    
    # Try Asset 400 (palette container)
    asset400 = next((e for e in sec_entries if e.asset_id == 400), None)
    if asset400:
        a400_data = sec_data[asset400.offset:asset400.offset + asset400.size]
        palettes, full_indices = parse_asset_400_palettes(a400_data)
        
        if palettes:
            # Use specified palette index (1-indexed, convert to 0-indexed)
            if 1 <= palette_idx <= len(palettes):
                return palettes[palette_idx - 1]
            # Default to first full palette (256-color sprite palette)
            if full_indices:
                return palettes[full_indices[0]]
            return palettes[0]
    
    # Grayscale fallback
    return [(i, i, i) for i in range(256)]


def extract_level_sprites(blb: BLBFile, level_idx: int, output_dir: Path,
                          segment: str = 'tertiary', block_idx: int = 0,
                          max_sprites: int = 100, palette_idx: int = 0):
    """
    Extract sprites from a level's segment.
    
    Args:
        blb: BLB file handle
        level_idx: Level index
        output_dir: Output directory path
        segment: 'tertiary' or 'primary'
        block_idx: Tertiary block index (0-5)
        max_sprites: Maximum sprites to extract
        palette_idx: Override palette index (0 = auto from CLUT, 1-7 = specific palette)
    """
    level = blb.header.get_level_by_index(level_idx)
    level_name = level.level_id.rstrip('\x00')
    
    print(f"\n{'='*60}")
    print(f"Level {level_idx}: {level_name}")
    print(f"{'='*60}")
    
    # Get all palettes from secondary segment
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    sec_entries = parse_container_toc(sec_data)
    
    all_palettes = []
    full_indices = []
    asset400 = next((e for e in sec_entries if e.asset_id == 400), None)
    if asset400:
        a400_data = sec_data[asset400.offset:asset400.offset + asset400.size]
        all_palettes, full_indices = parse_asset_400_palettes(a400_data)
    
    # Default palette if none found
    if not all_palettes:
        all_palettes = [[(i, i, i) for i in range(256)]]
        full_indices = [0]
    
    print(f"  Found {len(all_palettes)} palettes ({len(full_indices)} full 256-color) in Asset 400")
    
    # Read segment data
    if segment == 'tertiary':
        if block_idx >= len(level.tert_sub_offsets) or level.tert_sub_offsets[block_idx] == 0:
            print(f"  No tertiary block {block_idx}")
            return 0
        seg_data = blb.read_sectors(level.tert_sub_offsets[block_idx], 
                                     level.tert_sub_counts[block_idx])
        print(f"  Reading tertiary block {block_idx}: {len(seg_data)} bytes")
    elif segment == 'primary':
        seg_data = blb.read_sectors(level.sector_offset, level.sector_count)
        print(f"  Reading primary segment: {len(seg_data)} bytes")
    else:
        print(f"  Unknown segment: {segment}")
        return 0
    
    # Parse container TOC
    entries = parse_container_toc(seg_data)
    print(f"  Container has {len(entries)} assets")
    
    # Find Asset 600 (sprites)
    asset600 = next((e for e in entries if e.asset_id == 600), None)
    if not asset600:
        print(f"  No Asset 600 (sprites) found")
        return 0
    
    print(f"  Asset 600: {asset600.size} bytes at offset 0x{asset600.offset:X}")
    
    # Parse sprite container
    sprite_toc = parse_container_toc(asset600.data)
    print(f"  Found {len(sprite_toc)} sprites")
    
    # Create output directory
    level_dir = output_dir / f"{level_idx:02d}_{level_name}" / segment
    if segment == 'tertiary':
        level_dir = level_dir / f"block_{block_idx}"
    level_dir.mkdir(parents=True, exist_ok=True)
    
    # Extract each sprite
    extracted = 0
    for i, entry in enumerate(sprite_toc):
        if i >= max_sprites:
            print(f"  (stopping at {max_sprites} sprites)")
            break
            
        try:
            sprite = extract_sprite(entry.data)
            sprite_id = sprite['header']['sprite_id']
            total_frames = sprite['header']['total_frame_count']
            sprite_type = sprite['header']['sprite_type']
            clut_value = sprite['header']['clut']
            
            # Determine palette to use - always use first full 256-color palette
            # (CLUT field is garbage, always 0x815D)
            if palette_idx > 0:
                # Use specified palette
                pal_to_use = palette_idx
            elif full_indices:
                # Use first full 256-color palette (auto mode)
                pal_to_use = full_indices[0] + 1  # Convert to 1-indexed
            else:
                pal_to_use = 1
            
            # Clamp to available palettes
            if pal_to_use < 1 or pal_to_use > len(all_palettes):
                pal_to_use = 1
            
            palette = all_palettes[pal_to_use - 1]
            
            type_str = "ext" if sprite_type == 2 else "std"
            print(f"\n  Sprite {i}: ID=0x{sprite_id:08X}, {total_frames} frame(s), "
                  f"type={type_str}, palette {pal_to_use}")
            
            # Save each frame
            for frame in sprite['frames']:
                w, h = frame['width'], frame['height']
                
                if HAS_PIL and w > 0 and h > 0:
                    img = sprite_to_image(frame['pixels'], palette)
                    if img:
                        filename = f"sprite_{sprite_id:08x}_frame_{frame['index']:02d}.png"
                        img.save(level_dir / filename)
                        ox, oy = frame['origin_x'], frame['origin_y']
                        print(f"    Frame {frame['index']}: {w}x{h} "
                              f"origin=({ox}, {oy}) -> {filename}")
                        extracted += 1
        
        except Exception as e:
            print(f"  Error extracting sprite {i} (0x{entry.asset_id:08X}): {e}")
    
    return extracted


def analyze_sprite_format(blb: BLBFile, level_idx: int):
    """Analyze the sprite format without extracting images."""
    level = blb.header.get_level_by_index(level_idx)
    level_name = level.level_id.rstrip('\x00')
    
    print(f"\n{'='*60}")
    print(f"Sprite Format Analysis: Level {level_idx} ({level_name})")
    print(f"{'='*60}")
    
    # Read tertiary block 0
    if level.tert_sub_offsets[0] == 0:
        print("  No tertiary block 0")
        return
    
    seg_data = blb.read_sectors(level.tert_sub_offsets[0], level.tert_sub_counts[0])
    entries = parse_container_toc(seg_data)
    
    asset600 = next((e for e in entries if e.asset_id == 600), None)
    if not asset600:
        print("  No Asset 600")
        return
    
    sprite_toc = parse_container_toc(asset600.data)
    print(f"  {len(sprite_toc)} sprites in container")
    
    # Analyze first few sprites
    for i, entry in enumerate(sprite_toc[:5]):
        print(f"\n--- Sprite {i}: ID=0x{entry.asset_id:08X}, size={entry.size} bytes ---")
        
        try:
            sprite = extract_sprite(entry.data)
            hdr = sprite['header']
            
            print(f"  Header:")
            print(f"    magic={hdr['magic']} header_size={hdr['header_size']}")
            print(f"    frame_meta_size={hdr['frame_meta_size']} pixel_size={hdr['pixel_size']}")
            print(f"    frame_count={hdr['frame_count']} bytes_per_frame={hdr['bytes_per_frame']}")
            print(f"    unknown: 0x{hdr['unknown_12']:04X} 0x{hdr['unknown_14']:04X} 0x{hdr['unknown_16']:04X}")
            
            for frame in sprite['frames'][:3]:
                print(f"  Frame {frame['index']}:")
                print(f"    size: {frame['width']}x{frame['height']}")
                print(f"    origin: ({frame['origin_x']}, {frame['origin_y']})")
                print(f"    pixel_offset: 0x{frame['pixel_offset']:X}")
                print(f"    raw_meta: {frame['raw_meta']}")
                
        except Exception as e:
            print(f"  Error: {e}")


def collect_level_sprites(blb: BLBFile, level_idx: int) -> dict:
    """
    Collect all sprites from a level (all blocks).
    
    Returns a dict with:
        'level_idx': int
        'level_name': str
        'palettes': list of palettes from Asset 400
        'sprites': list of sprite dicts with 'sprite', 'block', 'index' keys
    """
    level = blb.header.get_level_by_index(level_idx)
    level_name = level.level_id.rstrip('\x00')
    
    # Get all palettes from secondary segment
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    sec_entries = parse_container_toc(sec_data)
    
    all_palettes = []
    full_indices = []
    asset400 = next((e for e in sec_entries if e.asset_id == 400), None)
    if asset400:
        a400_data = sec_data[asset400.offset:asset400.offset + asset400.size]
        all_palettes, full_indices = parse_asset_400_palettes(a400_data)
    
    if not all_palettes:
        all_palettes = [[(i, i, i) for i in range(256)]]
        full_indices = [0]
    
    sprites = []
    
    # Collect from all tertiary blocks
    for block_idx in range(6):
        if block_idx >= len(level.tert_sub_offsets) or level.tert_sub_offsets[block_idx] == 0:
            continue
        
        try:
            seg_data = blb.read_sectors(level.tert_sub_offsets[block_idx], 
                                         level.tert_sub_counts[block_idx])
            entries = parse_container_toc(seg_data)
            asset600 = next((e for e in entries if e.asset_id == 600), None)
            
            if not asset600:
                continue
            
            sprite_toc = parse_container_toc(asset600.data)
            
            for i, entry in enumerate(sprite_toc):
                try:
                    sprite = extract_sprite(entry.data)
                    sprites.append({
                        'sprite': sprite,
                        'block': block_idx,
                        'index': i,
                        'segment': 'tertiary'
                    })
                except Exception:
                    pass
        except Exception:
            pass
    
    # Also collect from primary segment
    try:
        seg_data = blb.read_sectors(level.sector_offset, level.sector_count)
        entries = parse_container_toc(seg_data)
        asset600 = next((e for e in entries if e.asset_id == 600), None)
        
        if asset600:
            sprite_toc = parse_container_toc(asset600.data)
            for i, entry in enumerate(sprite_toc):
                try:
                    sprite = extract_sprite(entry.data)
                    sprites.append({
                        'sprite': sprite,
                        'block': -1,  # primary
                        'index': i,
                        'segment': 'primary'
                    })
                except Exception:
                    pass
    except Exception:
        pass
    
    return {
        'level_idx': level_idx,
        'level_name': level_name,
        'palettes': all_palettes,
        'full_indices': full_indices,  # Indices of full 256-color palettes
        'sprites': sprites
    }


def create_overview_image(level_data: dict, palette_idx: int, output_path: Path,
                          max_sprites: int = 200, cell_height: int = 80) -> int:
    """
    Create an overview image showing all sprites from a level.
    Each sprite gets its own row with all animation frames displayed horizontally.
    
    Args:
        level_data: Dict from collect_level_sprites
        palette_idx: Which palette to use (0-based index, or -1 for auto)
        output_path: Path to save PNG
        max_sprites: Maximum sprites to include
        cell_height: Height of each row (frames scaled to fit)
        
    Returns:
        Number of sprites rendered
    """
    if not HAS_PIL:
        print("  PIL not available, skipping overview")
        return 0
    
    sprites = level_data['sprites'][:max_sprites]
    if not sprites:
        return 0
    
    all_palettes = level_data['palettes']
    full_indices = level_data.get('full_indices', [0])
    
    # Determine palette to use
    if palette_idx >= 0 and palette_idx < len(all_palettes):
        pal = all_palettes[palette_idx]
    elif full_indices:
        pal = all_palettes[full_indices[0]]
    elif all_palettes:
        pal = all_palettes[0]
    else:
        return 0
    
    # First pass: calculate dimensions for each sprite row
    padding = 4
    row_data = []  # List of (sprite_idx, frame_images, row_width, row_height)
    
    for i, sprite_data in enumerate(sprites):
        sprite = sprite_data['sprite']
        frames = sprite['frames']
        
        if not frames:
            continue
        
        # Render all frames for this sprite
        frame_images = []
        max_frame_height = 0
        total_width = 0
        
        for frame in frames:
            if frame['width'] == 0 or frame['height'] == 0:
                continue
            
            img = sprite_to_image(frame['pixels'], pal)
            if img:
                frame_images.append(img)
                max_frame_height = max(max_frame_height, img.height)
                total_width += img.width + padding
        
        if frame_images:
            # Scale factor to fit in cell_height
            scale = min(1.0, (cell_height - padding * 2) / max_frame_height)
            scaled_height = int(max_frame_height * scale)
            scaled_width = sum(int(img.width * scale) for img in frame_images) + padding * len(frame_images)
            
            row_data.append({
                'sprite_idx': i,
                'sprite_id': sprite['header']['sprite_id'],
                'sprite_type': sprite['header']['sprite_type'],
                'frame_count': len(frame_images),
                'frame_images': frame_images,
                'scale': scale,
                'row_width': scaled_width + 120,  # Extra space for label
                'row_height': cell_height
            })
    
    if not row_data:
        return 0
    
    # Calculate total image size
    img_width = max(r['row_width'] for r in row_data)
    img_width = max(img_width, 400)  # Minimum width
    img_height = len(row_data) * cell_height
    
    # Create overview image
    overview = Image.new('RGBA', (img_width, img_height), (32, 32, 32, 255))
    
    # We need ImageDraw for text
    try:
        from PIL import ImageDraw
        draw = ImageDraw.Draw(overview)
        has_draw = True
    except ImportError:
        has_draw = False
    
    # Second pass: render each sprite row
    rendered = 0
    for row_idx, row in enumerate(row_data):
        y_base = row_idx * cell_height
        x = padding
        
        # Draw label
        if has_draw:
            type_char = 'L' if row['sprite_type'] == 2 else 'S'
            label = f"{row['sprite_idx']:2d}:{type_char} {row['frame_count']:2d}f 0x{row['sprite_id']:08X}"
            draw.text((x, y_base + padding), label, fill=(180, 180, 180, 255))
        
        x = 120  # Start frames after label
        
        # Draw separator line
        if has_draw and row_idx > 0:
            draw.line([(0, y_base), (img_width, y_base)], fill=(60, 60, 60, 255))
        
        # Draw each frame
        scale = row['scale']
        for img in row['frame_images']:
            # Scale frame
            new_w = max(1, int(img.width * scale))
            new_h = max(1, int(img.height * scale))
            
            if scale < 1.0:
                scaled_img = img.resize((new_w, new_h), Image.NEAREST)
            else:
                scaled_img = img
            
            # Center vertically in row
            y = y_base + (cell_height - new_h) // 2
            
            # Paste frame
            overview.paste(scaled_img, (x, y), scaled_img)
            x += new_w + padding
        
        rendered += 1
    
    # Save
    output_path.parent.mkdir(parents=True, exist_ok=True)
    overview.save(output_path)
    
    return rendered


def create_all_overviews(blb_path: Path, output_dir: Path, max_sprites: int = 200, 
                         auto_only: bool = False):
    """
    Create overview images for all levels, with all palette variants.
    
    For each level, creates:
        - overview_auto.png (uses first full 256-color palette)
        - overview_palN.png for each full 256-color palette (unless auto_only=True)
    """
    output_dir.mkdir(parents=True, exist_ok=True)
    
    with BLBFile(blb_path) as blb:
        print(f"BLB: {blb_path}")
        print(f"Levels: {blb.header.level_count}")
        
        for level_idx in range(blb.header.level_count):
            print(f"\n{'='*60}")
            print(f"Level {level_idx}")
            print(f"{'='*60}")
            
            try:
                level_data = collect_level_sprites(blb, level_idx)
                level_name = level_data['level_name']
                sprite_count = len(level_data['sprites'])
                palette_count = len(level_data['palettes'])
                full_indices = level_data.get('full_indices', [0])
                
                print(f"  Name: {level_name}")
                print(f"  Sprites: {sprite_count}")
                print(f"  Palettes: {palette_count} total, {len(full_indices)} full 256-color")
                
                if sprite_count == 0:
                    print("  No sprites found, skipping")
                    continue
                
                level_dir = output_dir / f"{level_idx:02d}_{level_name}"
                
                # Create auto overview (uses first full palette)
                auto_path = level_dir / "overview_auto.png"
                count = create_overview_image(level_data, -1, auto_path, max_sprites)
                print(f"  -> {auto_path.name}: {count} sprites (auto = palette {full_indices[0] if full_indices else 0})")
                
                # Create overview for each full 256-color palette only
                if not auto_only:
                    for pal_idx in full_indices:
                        pal_path = level_dir / f"overview_pal{pal_idx}.png"
                        count = create_overview_image(level_data, pal_idx, pal_path, max_sprites)
                        print(f"  -> {pal_path.name}: {count} sprites")
                    
            except Exception as e:
                print(f"  Error: {e}")
                import traceback
                traceback.print_exc()
    
    print(f"\n{'='*60}")
    print(f"Overview images saved to: {output_dir}")


def main():
    parser = argparse.ArgumentParser(description='Extract RLE sprites from Skullmonkeys BLB')
    parser.add_argument('--blb', default='disks/blb/GAME.BLB', help='Path to GAME.BLB')
    parser.add_argument('--level', type=int, default=1, help='Level index (0-25)')
    parser.add_argument('--all-levels', action='store_true', help='Extract all levels')
    parser.add_argument('--segment', choices=['primary', 'tertiary'], default='tertiary',
                        help='Which segment to extract from')
    parser.add_argument('--block', type=int, default=0, help='Tertiary block index')
    parser.add_argument('--output', default='output/sprites', help='Output directory')
    parser.add_argument('--analyze', action='store_true', help='Analyze format only, no extraction')
    parser.add_argument('--max', type=int, default=100, help='Maximum sprites to extract per level')
    parser.add_argument('--palette', type=int, default=0, 
                        help='Palette index 1-7 (0 = auto from CLUT value)')
    parser.add_argument('--overview', action='store_true',
                        help='Create overview images for all levels')
    parser.add_argument('--all-palettes', action='store_true',
                        help='With --overview, also generate per-palette variants')
    
    args = parser.parse_args()
    
    blb_path = Path(args.blb)
    if not blb_path.exists():
        print(f"Error: BLB file not found: {blb_path}")
        sys.exit(1)
    
    output_dir = Path(args.output)
    
    # Handle --overview mode separately (doesn't need BLB context manager twice)
    if args.overview:
        create_all_overviews(blb_path, output_dir, args.max, auto_only=not args.all_palettes)
        return
    
    with BLBFile(blb_path) as blb:
        print(f"BLB: {blb_path}")
        print(f"Levels: {blb.header.level_count}")
        
        if args.analyze:
            analyze_sprite_format(blb, args.level)
            return
        
        total = 0
        if args.all_levels:
            for i in range(blb.header.level_count):
                try:
                    total += extract_level_sprites(blb, i, output_dir, 
                                                   args.segment, args.block, args.max,
                                                   args.palette)
                except Exception as e:
                    print(f"Error on level {i}: {e}")
        else:
            total = extract_level_sprites(blb, args.level, output_dir, 
                                          args.segment, args.block, args.max,
                                          args.palette)
    
    print(f"\n{'='*60}")
    print(f"Extracted {total} sprite frames to: {output_dir}")


if __name__ == '__main__':
    main()
