#!/usr/bin/env python3
"""
Extract sprites from Asset 600 (Primary Segment Sprite Container).

Asset 600 contains RLE-compressed sprites with multiple animation groups.
Each sprite has frame metadata and RLE-encoded pixel data.
"""

import struct
import argparse
from pathlib import Path
from PIL import Image
import os

def read_level_primary_segment(blb_path: str, level_index: int) -> tuple[bytes, str, str]:
    """Read the primary segment data for a level."""
    with open(blb_path, 'rb') as f:
        # Read level metadata
        f.seek(level_index * 0x70)
        meta = f.read(0x70)
        
        primary_sector = struct.unpack('<H', meta[0:2])[0]
        primary_count = struct.unpack('<H', meta[2:4])[0]
        level_id = meta[0x56:0x5B].decode('ascii').strip('\x00')
        level_name = meta[0x5B:0x70].decode('ascii').strip('\x00')
        
        # Read primary segment
        file_offset = primary_sector * 2048
        f.seek(file_offset)
        data = f.read(primary_count * 2048)
        
    return data, level_id, level_name


def parse_asset600_toc(data: bytes) -> list[dict]:
    """Parse the Asset 600 sub-TOC to get sprite entries."""
    # First find Asset 600 in primary TOC
    entry_count = struct.unpack('<I', data[0:4])[0]
    
    asset600_offset = None
    asset600_size = None
    
    for i in range(entry_count):
        offset = 4 + i * 12
        type_id, size, data_offset = struct.unpack('<III', data[offset:offset+12])
        if type_id == 600:
            asset600_offset = data_offset
            asset600_size = size
            break
    
    if asset600_offset is None:
        return []
    
    # Parse Asset 600 sub-TOC
    asset600 = data[asset600_offset:]
    sub_count = struct.unpack('<I', asset600[0:4])[0]
    
    sprites = []
    for i in range(sub_count):
        entry_off = 4 + i * 12
        sprite_id, sprite_size, sprite_off = struct.unpack('<III', asset600[entry_off:entry_off+12])
        
        sprite_data = asset600[sprite_off:sprite_off+sprite_size]
        sprites.append({
            'id': sprite_id,
            'size': sprite_size,
            'offset': sprite_off,
            'data': sprite_data,
        })
    
    return sprites


def parse_sprite_header(sprite_data: bytes, sprite_size: int) -> dict:
    """Parse sprite header, animation entries, and embedded palette."""
    anim_count = struct.unpack('<H', sprite_data[0:2])[0]
    frame_meta_offset = struct.unpack('<H', sprite_data[2:4])[0]
    rle_data_offset = struct.unpack('<I', sprite_data[4:8])[0]
    palette_offset = struct.unpack('<I', sprite_data[8:12])[0]
    
    # Parse embedded palette if present (within sprite bounds)
    # Palette is 512 bytes = 256 × 2-byte PSX colors (no header)
    palette = None
    if palette_offset < sprite_size:
        pal_data = sprite_data[palette_offset:palette_offset+512]
        if len(pal_data) >= 512:
            palette = []
            for i in range(256):
                color = struct.unpack('<H', pal_data[i*2:i*2+2])[0]
                # Convert PSX 15-bit to RGB
                r = (color & 0x1f) * 8
                g = ((color >> 5) & 0x1f) * 8
                b = ((color >> 10) & 0x1f) * 8
                # Color 0 is typically transparent
                if i == 0:
                    palette.append((255, 0, 255, 0))  # Transparent (magenta for visibility)
                else:
                    palette.append((r, g, b, 255))
    
    animations = []
    for i in range(anim_count):
        entry_off = 12 + i * 12
        anim_entry = sprite_data[entry_off:entry_off+12]
        anim_id, frame_count, frame_off, flags, extra = struct.unpack('<IHHHH', anim_entry)
        animations.append({
            'id': anim_id,
            'frame_count': frame_count,
            'frame_offset': frame_off,
            'flags': flags,
            'extra': extra,
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
    frame = sprite_data[offset:offset+0x24]
    
    if len(frame) < 0x24:
        return None
    
    vals = struct.unpack('<HHHhhHHHHhhHHHHHI', frame)
    
    return {
        'unknown1': vals[0],
        'unknown2': vals[1],
        'flags': vals[2],
        'x_offset': vals[3],  # signed
        'y_offset': vals[4],  # signed
        'width': vals[5],
        'height': vals[6],
        'unknown3': vals[7],
        'unknown4': vals[8],
        'anchor_x': vals[9],  # signed
        'anchor_y': vals[10], # signed
        'clip_width': vals[11],
        'clip_height': vals[12],
        'unknown5': vals[13],
        'unknown6': vals[14],
        'unknown7': vals[15],
        'rle_offset': vals[16],
    }


def decode_rle_sprite(sprite_data: bytes, rle_base_offset: int, frame_rle_offset: int, 
                      width: int, height: int) -> list[int]:
    """Decode RLE-encoded sprite frame to pixel array."""
    rle_start = rle_base_offset + frame_rle_offset
    rle_data = sprite_data[rle_start:]
    
    # First u16 is command count
    cmd_count = struct.unpack('<H', rle_data[0:2])[0]
    
    # Parse commands
    commands = []
    for i in range(cmd_count):
        cmd = struct.unpack('<H', rle_data[2 + i*2:4 + i*2])[0]
        new_line = (cmd & 0x8000) != 0
        skip = (cmd & 0x7F00) >> 8
        copy_count = cmd & 0xFF
        commands.append((new_line, skip, copy_count))
    
    # Pixel data follows commands
    pixel_start = 2 + cmd_count * 2
    pixel_data = rle_data[pixel_start:]
    
    # Decode to 2D pixel array
    pixels = [0] * (width * height)  # 0 = transparent
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


def get_default_palette() -> list[tuple]:
    """Generate a default grayscale palette for visualization."""
    palette = []
    for i in range(256):
        palette.append((i, i, i, 255 if i > 0 else 0))  # Transparent for index 0
    return palette


def create_sprite_sheet(sprite_data: bytes, header: dict, palette: list[tuple] = None) -> Image.Image:
    """Create a sprite sheet image from all frames."""
    if palette is None:
        palette = get_default_palette()
    
    # Get all frames across all animations
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
            
            if width == 0 or height == 0:
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
                'meta': frame_meta,
            })
    
    if not all_frames:
        return None
    
    # Calculate sprite sheet dimensions
    frames_per_row = min(8, len(all_frames))
    rows = (len(all_frames) + frames_per_row - 1) // frames_per_row
    
    padding = 2
    sheet_width = frames_per_row * (max_width + padding) + padding
    sheet_height = rows * (max_height + padding) + padding
    
    # Create RGBA image
    img = Image.new('RGBA', (sheet_width, sheet_height), (0, 0, 0, 0))
    
    for i, frame in enumerate(all_frames):
        col = i % frames_per_row
        row = i // frames_per_row
        
        x_start = padding + col * (max_width + padding)
        y_start = padding + row * (max_height + padding)
        
        for y in range(frame['height']):
            for x in range(frame['width']):
                px_idx = frame['pixels'][y * frame['width'] + x]
                if px_idx > 0:  # Not transparent
                    color = palette[px_idx]
                    img.putpixel((x_start + x, y_start + y), color)
    
    return img


def main():
    parser = argparse.ArgumentParser(description='Extract sprites from Asset 600')
    parser.add_argument('blb_path', help='Path to GAME.BLB')
    parser.add_argument('--level', '-l', type=int, default=2, help='Level index (default: 2 = SCIE)')
    parser.add_argument('--output', '-o', default='extracted_sprites', help='Output directory')
    parser.add_argument('--count', '-c', type=int, default=10, help='Max sprites to extract')
    args = parser.parse_args()
    
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    print(f"Reading level {args.level} from {args.blb_path}")
    
    # Read primary segment
    data, level_id, level_name = read_level_primary_segment(args.blb_path, args.level)
    print(f"Level: {level_name} ({level_id})")
    
    # Parse sprites
    sprites = parse_asset600_toc(data)
    print(f"Found {len(sprites)} sprites in Asset 600")
    
    # Extract sprites
    extracted = 0
    unique_offsets = set()  # Track unique sprite data to avoid duplicates
    
    for i, sprite in enumerate(sprites):
        if extracted >= args.count:
            break
        
        # Skip duplicate data (multiple IDs can point to same sprite)
        if sprite['offset'] in unique_offsets:
            continue
        unique_offsets.add(sprite['offset'])
        
        try:
            header = parse_sprite_header(sprite['data'], sprite['size'])
            
            # Skip sprites with no animations
            if header['anim_count'] == 0:
                continue
            
            # Use embedded palette if available, otherwise default grayscale
            palette = header['palette'] if header['palette'] else get_default_palette()
            img = create_sprite_sheet(sprite['data'], header, palette)
            
            if img is not None:
                filename = f"sprite_{level_id}_{sprite['id']:08x}.png"
                filepath = output_dir / filename
                img.save(filepath)
                
                total_frames = sum(a['frame_count'] for a in header['animations'])
                has_palette = header['palette'] is not None
                print(f"  Saved {filename} ({header['anim_count']} anims, {total_frames} frames, {img.size[0]}x{img.size[1]}, palette={has_palette})")
                extracted += 1
                
        except Exception as e:
            print(f"  Error extracting sprite {sprite['id']:08x}: {e}")
    
    print(f"\nExtracted {extracted} sprite sheets to {output_dir}/")


if __name__ == '__main__':
    main()
