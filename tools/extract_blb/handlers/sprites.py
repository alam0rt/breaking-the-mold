"""
Sprite Container Handler (Asset Type 600)

Extracts sprite containers to:
1. Raw bytes (.bin) via default handler
2. Animated GIFs for multi-frame animations
3. Static PNGs for single-frame animations

Sprite Format:
- Container TOC: count (u32), then sprite_id/size/offset entries (12 bytes each)
- Each sprite has:
  - Header (12 bytes): anim_count, frame_meta_offset, rle_offset, palette_offset
  - Animations (12 bytes each): anim_id, frame_count, frame_offset, flags
  - Frame metadata (36 bytes each): dimensions, anchors, rle_offset
  - Palette (512 bytes): 256 PSX 15-bit colors
  - RLE data: compressed pixel indices

Animation Flags:
- Bit 0 (0x0001): Loop animation
"""

import json
import struct
from pathlib import Path
from typing import Optional

from . import register_handler, default_handler

# Frame delay in milliseconds for GIF animations
# PSX PAL runs at 50Hz, NTSC at 60Hz. Most game animations run at ~10fps
DEFAULT_FRAME_DELAY_MS = 100  # 10 fps


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
    """
    Parse 36-byte frame metadata.
    
    Offset  Size  Type   Field
    0x00    2     u16    field_00 (unknown)
    0x02    2     u16    field_02 (unknown)
    0x04    2     u16    flags
    0x06    2     s16    render_x (signed offset from anchor)
    0x08    2     s16    render_y (signed offset from anchor)
    0x0A    2     u16    render_width
    0x0C    2     u16    render_height
    0x0E    2     u16    field_0e (unknown)
    0x10    2     u16    field_10 (unknown)
    0x12    2     s16    anchor_x (signed)
    0x14    2     s16    anchor_y (signed)
    0x16    2     u16    clip_width
    0x18    2     u16    clip_height
    0x1A    6     -      padding
    0x20    4     u32    rle_offset
    """
    fields = struct.unpack_from('<HHHhhHHHHhhHH6xI', data, offset)
    return {
        'field_00': fields[0],
        'field_02': fields[1],
        'flags': fields[2],
        'render_x': fields[3],      # signed s16
        'render_y': fields[4],      # signed s16
        'render_width': fields[5],
        'render_height': fields[6],
        'field_0e': fields[7],
        'field_10': fields[8],
        'anchor_x': fields[9],      # signed s16
        'anchor_y': fields[10],     # signed s16
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


def save_gif(
    frames: list[tuple[bytes, int, int, int, int]],
    path: Path,
    frame_delay_ms: int = DEFAULT_FRAME_DELAY_MS,
    loop: bool = True,
) -> bool:
    """
    Save multiple frames as animated GIF.
    
    Args:
        frames: List of (pixels, width, height, offset_x, offset_y) tuples
                offset_x/y are relative to a common anchor point
        path: Output path
        frame_delay_ms: Delay between frames in milliseconds
        loop: Whether animation should loop (0 = infinite loop)
    
    Returns True on success.
    """
    if not frames:
        return False
    
    try:
        from PIL import Image
        
        # Calculate bounding box that fits all frames
        # Each frame has render_x/render_y which is offset from anchor
        min_x = min(ox for _, _, _, ox, _ in frames)
        min_y = min(oy for _, _, _, _, oy in frames)
        max_x = max(ox + w for _, w, _, ox, _ in frames)
        max_y = max(oy + h for _, _, h, _, oy in frames)
        
        canvas_width = max_x - min_x
        canvas_height = max_y - min_y
        
        if canvas_width <= 0 or canvas_height <= 0:
            return False
        
        # Create PIL images for each frame
        pil_frames = []
        for pixels, width, height, offset_x, offset_y in frames:
            # Create frame image
            frame_img = Image.frombytes('RGBA', (width, height), pixels)
            
            # Create canvas and paste frame at correct position
            canvas = Image.new('RGBA', (canvas_width, canvas_height), (0, 0, 0, 0))
            paste_x = offset_x - min_x
            paste_y = offset_y - min_y
            canvas.paste(frame_img, (paste_x, paste_y))
            
            pil_frames.append(canvas)
        
        if len(pil_frames) == 1:
            # Single frame - save as PNG instead
            png_path = path.with_suffix('.png')
            pil_frames[0].save(png_path, 'PNG')
            return True
        
        # Convert RGBA frames to palette mode for GIF
        # Use a quantization approach that preserves transparency properly
        gif_frames = []
        
        for frame in pil_frames:
            # Get alpha channel
            alpha = frame.split()[3]
            
            # Convert to RGB, using a unique color for transparent pixels
            # We'll use index 0 for transparency in the final palette
            rgb_frame = Image.new('RGB', frame.size, (0, 0, 0))
            
            # Paste the RGB portion where alpha > 0
            rgb_data = frame.convert('RGB')
            
            # Create mask from alpha (pixels with alpha > 128 are opaque)
            mask = alpha.point(lambda x: 255 if x > 128 else 0)
            rgb_frame.paste(rgb_data, mask=mask)
            
            # Quantize to 255 colors, reserving index 0 for transparency
            # Use MEDIANCUT for better color preservation
            p_frame = rgb_frame.quantize(colors=255, method=Image.Quantize.MEDIANCUT)
            
            # Now we need to set transparent pixels to a specific index
            # Get the palette and add a transparent color at the end
            palette = p_frame.getpalette()
            
            # Create a new image with transparency support
            # Map transparent pixels (where alpha was 0) to index 255
            p_data = list(p_frame.getdata())
            alpha_data = list(alpha.getdata())
            
            new_data = []
            for i, (p_val, a_val) in enumerate(zip(p_data, alpha_data)):
                if a_val < 128:  # Transparent
                    new_data.append(255)
                else:
                    new_data.append(p_val)
            
            # Create new palette image
            result = Image.new('P', frame.size)
            result.putdata(new_data)
            
            # Set palette - copy existing and set index 255 to magenta (for debugging)
            new_palette = palette[:] + [0] * (768 - len(palette))
            new_palette[255*3:255*3+3] = [255, 0, 255]  # Magenta for transparent
            result.putpalette(new_palette)
            
            gif_frames.append(result)
        
        # Save animated GIF with transparency on index 255
        gif_frames[0].save(
            path,
            format='GIF',
            save_all=True,
            append_images=gif_frames[1:],
            duration=frame_delay_ms,
            loop=0 if loop else 1,
            transparency=255,
            disposal=2,  # Clear frame before drawing next
        )
        return True
        
    except ImportError:
        return False
    except Exception as e:
        # Debug: uncomment to see errors
        # print(f"GIF save error: {e}")
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
    2. Animated GIFs for multi-frame sprites in sprites/ subdirectory
    3. Static PNGs for single-frame sprites
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
    
    # Track extraction stats
    sprites_info = []
    gifs_created = 0
    pngs_created = 0
    
    # Read sprite TOC (each entry: sprite_id u32, size u32, offset u32)
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
        
        sprite_info = {
            'sprite_id': sprite_id,
            'animations': [],
        }
        
        # Parse animations and extract ALL frames
        for anim_idx in range(min(header['animation_count'], 32)):  # Limit animations
            anim_offset = sprite_offset + 12 + anim_idx * 12
            if anim_offset + 12 > len(data):
                break
            
            anim = parse_animation_entry(data, anim_offset)
            
            if anim['frame_count'] == 0:
                continue
            
            # Determine if animation loops (flag bit 0)
            loops = bool(anim['flags'] & 0x0001)
            
            # Extract ALL frames for this animation
            frames = []
            frame_meta_base = sprite_offset + header['frame_meta_offset']
            
            for frame_idx in range(anim['frame_count']):
                abs_frame_idx = anim['frame_offset'] + frame_idx
                frame_offset = frame_meta_base + abs_frame_idx * 36
                
                if frame_offset + 36 > len(data):
                    break
                
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
                    # Store frame with offset info for proper alignment
                    frames.append((
                        pixels,
                        width,
                        height,
                        frame['render_x'],  # signed offset
                        frame['render_y'],  # signed offset
                    ))
                except Exception:
                    continue
            
            if not frames:
                continue
            
            # Save animation
            anim_info = {
                'animation_id': anim['animation_id'],
                'frame_count': len(frames),
                'loops': loops,
            }
            
            if len(frames) == 1:
                # Single frame - save as PNG
                pixels, width, height, _, _ = frames[0]
                png_path = sprites_dir / f"sprite_{sprite_id:04d}_anim{anim_idx:02d}.png"
                if save_png(pixels, width, height, png_path):
                    output_files.append(png_path)
                    pngs_created += 1
                    anim_info['file'] = png_path.name
            else:
                # Multiple frames - save as animated GIF
                gif_path = sprites_dir / f"sprite_{sprite_id:04d}_anim{anim_idx:02d}.gif"
                if save_gif(frames, gif_path, loop=loops):
                    output_files.append(gif_path)
                    gifs_created += 1
                    anim_info['file'] = gif_path.name
            
            sprite_info['animations'].append(anim_info)
        
        if sprite_info['animations']:
            sprites_info.append(sprite_info)
    
    # Write sprite extraction summary
    summary_path = sprites_dir / "_summary.json"
    summary = {
        "sprite_count": sprite_count,
        "sprites_extracted": len(sprites_info),
        "gifs_created": gifs_created,
        "pngs_created": pngs_created,
        "level": asset_info.level,
        "segment": asset_info.segment,
        "sprites": sprites_info,
    }
    summary_path.write_text(json.dumps(summary, indent=2))
    output_files.append(summary_path)
    
    return output_files
