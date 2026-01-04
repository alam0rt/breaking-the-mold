#!/usr/bin/env python3
"""
extract_sprites_tertiary.py - Extract character/enemy sprites from Tertiary segments

The tertiary segment contains sprites for enemies, characters, and animated objects.
These use a sparse RLE format optimized for sprites with transparent areas.

Sprite RLE Format (from reverse engineering FUN_80010068):
  Each sprite has:
  - Header (24 bytes)
  - Frame metadata (variable size, 0x24 bytes per frame)
  - Control words (u16 array)
  - Pixel data (8bpp indexed)

  Control word format (u16):
  - Bit 15 (0x8000): New row flag
  - Bits 8-14 (0x7F00): Skip count (transparent pixels)
  - Bits 0-7 (0x00FF): Copy count (pixels to copy from source)

Usage:
    python3 extract_sprites_tertiary.py <blb_file> <level_index> [output_dir]
"""

import struct
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Tuple, Optional

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("Warning: PIL not installed. Install with: pip install Pillow")

sys.path.insert(0, str(Path(__file__).parent))
from blb import BLBFile


def psx_to_rgba(color: int) -> Tuple[int, int, int, int]:
    """Convert 15-bit PSX color to RGBA tuple."""
    if color == 0:
        return (0, 0, 0, 0)
    r = (color & 0x1F) << 3
    g = ((color >> 5) & 0x1F) << 3
    b = ((color >> 10) & 0x1F) << 3
    return (r, g, b, 255)


@dataclass
class SpriteFrame:
    """A single animation frame."""
    width: int
    height: int
    x_offset: int  # Signed offset for positioning
    y_offset: int
    pixel_offset: int  # Offset to pixel data
    control_offset: int  # Offset to control words
    control_count: int  # Number of control words


@dataclass  
class SpriteEntry:
    """A complete sprite with all frames."""
    flags: int
    frame_count: int
    frames: List[SpriteFrame]
    control_data: bytes
    pixel_data: bytes


def decode_sprite_frame(frame: SpriteFrame, control_data: bytes, pixel_data: bytes,
                         width: int, height: int, stride: int) -> List[int]:
    """
    Decode a sprite frame using the RLE format.
    
    Returns a list of pixel indices (0 = transparent).
    """
    # Output buffer (0 = transparent)
    output = [0] * (width * height)
    
    # Parse control words
    control_offset = 0
    pixel_offset = 0
    
    row = 0
    col = 0
    
    while control_offset < len(control_data) - 1:
        # Read u16 control word
        ctrl = struct.unpack('<H', control_data[control_offset:control_offset+2])[0]
        control_offset += 2
        
        new_row = (ctrl & 0x8000) != 0
        skip = (ctrl >> 8) & 0x7F
        copy = ctrl & 0xFF
        
        if new_row:
            row += 1
            col = 0
            if row >= height:
                break
        
        col += skip
        
        # Copy pixels
        for _ in range(copy):
            if pixel_offset < len(pixel_data) and row < height and col < width:
                idx = row * width + col
                if idx < len(output):
                    output[idx] = pixel_data[pixel_offset]
            pixel_offset += 1
            col += 1
    
    return output


def parse_sprite_entry(data: bytes, flags: int) -> Optional[SpriteEntry]:
    """Parse a sprite entry from Asset 600 sub-entry."""
    if len(data) < 24:
        return None
    
    # Header (24 bytes)
    magic = struct.unpack('<H', data[0:2])[0]
    header_size = struct.unpack('<H', data[2:4])[0]
    frame_meta_size = struct.unpack('<H', data[4:6])[0]
    # data[6:8] = 0
    pixel_data_size = struct.unpack('<I', data[8:12])[0]
    entry_flags = struct.unpack('<I', data[12:16])[0]
    frame_count = struct.unpack('<H', data[16:18])[0]
    
    if magic != 1 or header_size != 24:
        return None
    
    # Frame metadata starts at offset 24
    # Each frame is 0x24 (36) bytes based on code: iVar4 = (param_3 & 0xffff) * 0x24 + *param_1
    frame_size = 0x24
    frames = []
    
    for i in range(frame_count):
        frame_off = header_size + i * frame_size
        if frame_off + frame_size > len(data):
            break
        
        frame_data = data[frame_off:frame_off + frame_size]
        
        # Frame structure (from FUN_8007bebc analysis):
        # +0x0A (10): width (u16)
        # +0x0C (12): height (u16)
        # +0x20 (32): control word offset relative to data start
        
        width = struct.unpack('<H', frame_data[10:12])[0]
        height = struct.unpack('<H', frame_data[12:14])[0]
        ctrl_offset = struct.unpack('<I', frame_data[32:36])[0]
        
        frames.append(SpriteFrame(
            width=width,
            height=height,
            x_offset=0,
            y_offset=0,
            pixel_offset=0,
            control_offset=ctrl_offset,
            control_count=0
        ))
    
    # Control and pixel data follow frame metadata
    data_start = header_size + frame_meta_size
    
    return SpriteEntry(
        flags=flags,
        frame_count=frame_count,
        frames=frames,
        control_data=data[data_start:data_start + pixel_data_size],  # Approximate
        pixel_data=data[data_start:]  # Full remaining data
    )


def extract_sprites(blb_path: str, level_index: int, output_dir: str = "output/sprites"):
    """Extract sprites from a level's tertiary segment."""
    
    output_path = Path(output_dir)
    output_path.mkdir(parents=True, exist_ok=True)
    
    with BLBFile(blb_path) as blb:
        level = blb.header.get_level_by_index(level_index)
        if not level:
            print(f"Level {level_index} not found")
            return
        
        print(f"Level: {level.level_id} - {level.name}")
        
        # Get tertiary offsets from raw header
        raw = blb.header.raw_data
        base = level_index * 0x70
        
        tert_offsets = struct.unpack('<6H', raw[base+0x3A:base+0x46])
        tert_counts = struct.unpack('<6H', raw[base+0x48:base+0x54])
        
        # Load palettes from secondary segment
        palettes = load_palettes(blb, level)
        print(f"Loaded {len(palettes)} palettes")
        
        # Process each tertiary block
        for block_idx in range(6):
            if tert_offsets[block_idx] == 0 or tert_counts[block_idx] == 0:
                continue
            
            print(f"\nTertiary block {block_idx}: sectors {tert_offsets[block_idx]}+{tert_counts[block_idx]}")
            
            tert_data = blb.read_sectors(tert_offsets[block_idx], tert_counts[block_idx])
            
            # Parse TOC
            count = struct.unpack('<I', tert_data[:4])[0]
            assets = {}
            for j in range(count):
                off = 4 + j * 12
                type_id, size, offset = struct.unpack('<III', tert_data[off:off+12])
                assets[type_id] = (offset, size, tert_data[offset:offset+size])
            
            # Look for Asset 600 (0x258) - sprite graphics container
            if 0x258 not in assets:
                print(f"  No sprite container (0x258) in block {block_idx}")
                continue
            
            a600 = assets[0x258][2]
            sprite_count = struct.unpack('<I', a600[:4])[0]
            print(f"  Found {sprite_count} sprite entries")
            
            # Extract each sprite
            for sprite_idx in range(sprite_count):
                off = 4 + sprite_idx * 12
                flags, size, offset = struct.unpack('<III', a600[off:off+12])
                sprite_data = a600[offset:offset+size]
                
                entry = parse_sprite_entry(sprite_data, flags)
                if not entry:
                    continue
                
                print(f"    Sprite {sprite_idx}: {entry.frame_count} frames, flags=0x{flags:08X}")
                
                # Try to render first frame
                if entry.frames and HAS_PIL and palettes:
                    frame = entry.frames[0]
                    if frame.width > 0 and frame.height > 0:
                        # Decode sprite
                        try:
                            img = render_sprite_entry(entry, palettes[0] if palettes else None)
                            if img:
                                out_file = output_path / f"{level.level_id}_block{block_idx}_sprite{sprite_idx:03d}.png"
                                img.save(out_file)
                                print(f"      Saved: {out_file}")
                        except Exception as e:
                            print(f"      Error rendering: {e}")


def load_palettes(blb: BLBFile, level) -> List[List[Tuple[int, int, int, int]]]:
    """Load palettes from secondary segment."""
    palettes = []
    
    sec_data = blb.read_sectors(level.secondary_offset, level.secondary_count)
    sec_count = struct.unpack('<I', sec_data[:4])[0]
    
    for j in range(sec_count):
        off = 4 + j * 12
        type_id, size, offset = struct.unpack('<III', sec_data[off:off+12])
        
        # Animation container has palettes
        if type_id == 0x190:
            anim_data = sec_data[offset:offset+size]
            anim_count = struct.unpack('<I', anim_data[:4])[0]
            
            for k in range(anim_count):
                off2 = 4 + k * 12
                _, pal_size, pal_off = struct.unpack('<III', anim_data[off2:off2+12])
                pal_bytes = anim_data[pal_off:pal_off+pal_size]
                
                palette = [psx_to_rgba(struct.unpack('<H', pal_bytes[i:i+2])[0])
                          for i in range(0, min(len(pal_bytes), 512), 2)]
                palettes.append(palette)
    
    return palettes


def render_sprite_entry(entry: SpriteEntry, palette: List[Tuple[int, int, int, int]]) -> Optional[Image.Image]:
    """Render a sprite entry to an image (first frame or sheet)."""
    if not entry.frames:
        return None
    
    frame = entry.frames[0]
    if frame.width <= 0 or frame.height <= 0:
        return None
    
    # For now, try to interpret the raw pixel data
    # The actual format needs more RE work
    
    # Create a simple test output with raw data
    width = frame.width
    height = frame.height
    
    if width > 512 or height > 512:
        return None
    
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    
    # Try direct pixel mapping (this won't be correct but gives us something to look at)
    pixel_data = entry.pixel_data
    for y in range(min(height, len(pixel_data) // width)):
        for x in range(width):
            idx = y * width + x
            if idx < len(pixel_data):
                color_idx = pixel_data[idx]
                if color_idx < len(palette):
                    img.putpixel((x, y), palette[color_idx])
    
    return img


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 extract_sprites_tertiary.py <blb_file> <level_index> [output_dir]")
        print("\nExamples:")
        print("  python3 extract_sprites_tertiary.py disks/blb/GAME.BLB 1")
        print("  python3 extract_sprites_tertiary.py disks/blb/GAME.BLB 2 output/scie_sprites")
        return
    
    blb_path = sys.argv[1]
    level_index = int(sys.argv[2])
    output_dir = sys.argv[3] if len(sys.argv) > 3 else "output/sprites"
    
    if not HAS_PIL:
        print("Error: PIL/Pillow required for image output")
        return
    
    extract_sprites(blb_path, level_index, output_dir)


if __name__ == '__main__':
    main()
