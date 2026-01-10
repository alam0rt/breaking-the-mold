"""
Sprite Container Handler (Asset Type 600)

Extracts sprite containers to:
1. Raw bytes (.bin) via default handler
2. Individual sprite frames as PNG files

Sprite Format:
- Container TOC: count (u32), then sprite_id/size/offset entries (12 bytes each)
- Each sprite has:
  - Header (12 bytes): anim_count, frame_meta_offset, rle_offset, palette_offset
  - Animations (12 bytes each): anim_id, frame_count, frame_offset, flags
  - Frame metadata (36 bytes each): dimensions, anchors, rle_offset
  - Palette (512 bytes): 256 PSX 15-bit colors
  - RLE data: compressed pixel indices
"""

import json
import struct
from pathlib import Path

from . import register_handler, default_handler


def parse_sprite_header(data: bytes, offset: int) -> dict:
    """Parse 12-byte sprite header."""
    animation_count, frame_meta_offset, rle_offset, palette_offset = struct.unpack_from('<HHII', data, offset)
    return {
        'animation_count': animation_count,
        'frame_meta_offset': frame_meta_offset,
        'rle_offset': rle_offset,
        'palette_offset': palette_offset,
    }


def parse_animation_entry(data: bytes, offset: int) -> dict:
    """Parse 12-byte animation entry."""
    anim_id, frame_count, frame_offset, flags, extra = struct.unpack_from('<IHHHH', data, offset)
    return {
        'animation_id': anim_id,
        'frame_count': frame_count,
        'frame_offset': frame_offset,
        'flags': flags,
    }


def parse_frame_metadata(data: bytes, offset: int) -> dict:
    """Parse 36-byte frame metadata."""
    fields = struct.unpack_from('<HHHhhHHHHhhHH6xI', data, offset)
    return {
        'flags': fields[2],
        'render_x': fields[3],
        'render_y': fields[4],
        'render_width': fields[5],
        'render_height': fields[6],
        'anchor_x': fields[9],
        'anchor_y': fields[10],
        'clip_width': fields[11],
        'clip_height': fields[12],
        'rle_offset': fields[13],
    }


def parse_palette(data: bytes, offset: int) -> list[tuple[int, int, int, int]]:
    """Parse 256-color PSX palette (512 bytes) to RGBA tuples."""
    colors = []
    for i in range(256):
        raw = struct.unpack_from('<H', data, offset + i * 2)[0]
        r = (raw & 0x1F) << 3
        g = ((raw >> 5) & 0x1F) << 3
        b = ((raw >> 10) & 0x1F) << 3
        # Index 0 is typically transparent
        a = 0 if i == 0 else 255
        colors.append((r, g, b, a))
    return colors


def decode_rle_sprite(data: bytes, rle_offset: int, width: int, height: int, palette: list) -> bytes:
    """
    Decode RLE sprite data to RGBA pixels.
    
    RLE format:
    - u16 command_count
    - For each command (u16):
      - Bit 15: new_line (advance Y, reset X)
      - Bits 14-8: skip_count (transparent pixels)
      - Bits 7-0: copy_count (literal pixels to copy)
    - Followed by pixel data (1 byte per pixel, palette index)
    """
    if width <= 0 or height <= 0:
        return bytes(4)  # Return transparent pixel
    
    # Limit size to prevent memory issues
    if width > 512 or height > 512:
        return bytes(width * height * 4)
    
    # Create RGBA buffer
    pixels = bytearray(width * height * 4)
    
    # Read command count
    cmd_count = struct.unpack_from('<H', data, rle_offset)[0]
    
    # Read all commands
    commands = []
    for i in range(cmd_count):
        cmd = struct.unpack_from('<H', data, rle_offset + 2 + i * 2)[0]
        new_line = (cmd >> 15) & 1
        skip_count = (cmd >> 8) & 0x7F
        copy_count = cmd & 0xFF
        commands.append((new_line, skip_count, copy_count))
    
    # Pixel data starts after commands
    pixel_offset = rle_offset + 2 + cmd_count * 2
    
    # Decode
    x, y = 0, 0
    for new_line, skip_count, copy_count in commands:
        if new_line:
            y += 1
            x = 0
            if y >= height:
                break
        
        # Skip transparent pixels
        x += skip_count
        
        # Copy literal pixels
        for _ in range(copy_count):
            if x >= width or y >= height:
                break
            if pixel_offset < len(data):
                idx = data[pixel_offset]
                pixel_offset += 1
                r, g, b, a = palette[idx] if idx < len(palette) else (0, 0, 0, 0)
                px_offset = (y * width + x) * 4
                if px_offset + 3 < len(pixels):
                    pixels[px_offset] = r
                    pixels[px_offset + 1] = g
                    pixels[px_offset + 2] = b
                    pixels[px_offset + 3] = a
            x += 1
    
    return bytes(pixels)


def save_png(pixels: bytes, width: int, height: int, path: Path) -> bool:
    """Save RGBA pixels as PNG. Returns True on success."""
    try:
        from PIL import Image
        img = Image.frombytes('RGBA', (width, height), pixels)
        img.save(path, 'PNG')
        return True
    except ImportError:
        # PIL not available - skip PNG export
        return False
    except Exception:
        return False


@register_handler(600)
def sprite_container_handler(
    data: bytes,
    asset_info,
    output_dir: Path,
    context: dict
) -> list[Path]:
    """
    Handler for asset type 600 (sprite container).
    
    Extracts:
    1. Raw bytes (via default handler)
    2. Individual sprites as PNG files in sprites/ subdirectory
    """
    # First, run default handler to save raw bytes
    output_files = default_handler(data, asset_info, output_dir, context)
    
    # Create sprites subdirectory
    sprites_dir = output_dir / "sprites"
    sprites_dir.mkdir(parents=True, exist_ok=True)
    
    # Parse sprite container TOC
    if len(data) < 4:
        return output_files
    
    sprite_count = struct.unpack_from('<I', data, 0)[0]
    
    # Sanity check
    if sprite_count > 1000 or sprite_count * 12 + 4 > len(data):
        return output_files
    
    # Read sprite TOC (each entry: sprite_id u32, size u32, offset u32)
    sprites_extracted = 0
    for i in range(sprite_count):
        toc_offset = 4 + i * 12
        sprite_id, sprite_size, sprite_offset = struct.unpack_from('<III', data, toc_offset)
        
        if sprite_offset + 12 > len(data):
            continue
        
        # Parse sprite header
        header = parse_sprite_header(data, sprite_offset)
        
        if header['animation_count'] == 0:
            continue
        
        # Parse palette
        pal_abs = sprite_offset + header['palette_offset']
        if pal_abs + 512 > len(data):
            continue
        palette = parse_palette(data, pal_abs)
        
        # Parse animations and extract first frame of each
        for anim_idx in range(min(header['animation_count'], 32)):  # Limit animations
            anim_offset = sprite_offset + 12 + anim_idx * 12
            if anim_offset + 12 > len(data):
                break
            
            anim = parse_animation_entry(data, anim_offset)
            
            if anim['frame_count'] == 0:
                continue
            
            # Get first frame metadata
            frame_meta_base = sprite_offset + header['frame_meta_offset']
            frame_idx = anim['frame_offset']
            frame_offset = frame_meta_base + frame_idx * 36
            
            if frame_offset + 36 > len(data):
                continue
            
            frame = parse_frame_metadata(data, frame_offset)
            
            width = frame['render_width']
            height = frame['render_height']
            
            if width <= 0 or height <= 0 or width > 512 or height > 512:
                continue
            
            # Decode RLE data
            rle_abs = sprite_offset + header['rle_offset'] + frame['rle_offset']
            if rle_abs + 4 > len(data):
                continue
            
            try:
                pixels = decode_rle_sprite(data, rle_abs, width, height, palette)
                
                # Save PNG
                png_path = sprites_dir / f"sprite_{sprite_id:04d}_anim{anim_idx:02d}.png"
                if save_png(pixels, width, height, png_path):
                    output_files.append(png_path)
                    sprites_extracted += 1
            except Exception:
                continue  # Skip problematic sprites
    
    # Write sprite extraction summary
    summary_path = sprites_dir / "_summary.json"
    summary = {
        "sprite_count": sprite_count,
        "sprites_extracted": sprites_extracted,
        "level": asset_info.level,
        "segment": asset_info.segment,
    }
    summary_path.write_text(json.dumps(summary, indent=2))
    output_files.append(summary_path)
    
    return output_files
