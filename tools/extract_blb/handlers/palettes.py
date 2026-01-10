"""
Palette Handlers - Asset Types 400 (palette container) and 602 (level palette)

Extracts color palette data and converts to viewable formats.

Asset 400 (PALETTE_CONTAINER):
  - Contains sub-TOC with multiple 256-color palettes (CLUTs)
  - Each palette is 512 bytes (256 × 16-bit PSX colors)
  - Format: u32 count, then count × SubTOCEntry (id, size, offset)

Asset 602 (LEVEL_PALETTE / AUDIO_SETTINGS):
  - In primary segment: Level palette (256 × 16-bit PSX colors = 512 bytes)
  - In secondary segment: Per-sample audio volume/pan table

PSX 15-bit color format:
  - Bits 0-4: Red (0-31)
  - Bits 5-9: Green (0-31)
  - Bits 10-14: Blue (0-31)
  - Bit 15: STP (semi-transparency flag)
"""

from pathlib import Path
import json
import struct
from . import register_handler

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


def psx_color_to_rgb(color16: int) -> tuple[int, int, int, int]:
    """
    Convert PSX 15-bit color to RGBA.
    Returns (r, g, b, a) with 8-bit values.
    """
    r5 = (color16 >> 0) & 0x1F
    g5 = (color16 >> 5) & 0x1F
    b5 = (color16 >> 10) & 0x1F
    stp = (color16 >> 15) & 0x1
    
    # Expand 5-bit to 8-bit
    r = (r5 << 3) | (r5 >> 2)
    g = (g5 << 3) | (g5 >> 2)
    b = (b5 << 3) | (b5 >> 2)
    
    # Color 0 with STP=0 is transparent
    if color16 == 0:
        a = 0
    else:
        a = 255
    
    return (r, g, b, a)


def parse_palette_256(data: bytes, offset: int = 0) -> list[dict]:
    """Parse a 256-color palette (512 bytes) at the given offset."""
    colors = []
    for i in range(256):
        if offset + i * 2 + 2 > len(data):
            break
        color16 = struct.unpack_from('<H', data, offset + i * 2)[0]
        r, g, b, a = psx_color_to_rgb(color16)
        colors.append({
            "index": i,
            "raw": color16,
            "r": r, "g": g, "b": b, "a": a,
            "hex": f"#{r:02x}{g:02x}{b:02x}",
        })
    return colors


def create_palette_image(colors: list[dict], cols: int = 16) -> 'Image.Image':
    """Create a visual palette swatch image (16x16 grid of color squares)."""
    if not HAS_PIL:
        return None
    
    square_size = 16
    rows = (len(colors) + cols - 1) // cols
    img = Image.new('RGBA', (cols * square_size, rows * square_size), (0, 0, 0, 0))
    
    for i, c in enumerate(colors):
        x = (i % cols) * square_size
        y = (i // cols) * square_size
        # Draw filled square
        for dy in range(square_size):
            for dx in range(square_size):
                img.putpixel((x + dx, y + dy), (c['r'], c['g'], c['b'], c['a']))
    
    return img


@register_handler(400)
def palette_container_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for palette container (asset type 400).
    
    Creates:
    - Raw .bin file (via default handler)
    - palettes/palette_XX.json: Each palette's color data
    - palettes/palette_XX.png: Visual swatch of each palette
    - palettes/summary.json: Container overview
    """
    from . import default_handler
    
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    if len(data) < 4:
        return output_paths
    
    # Parse container header
    palette_count = struct.unpack_from('<I', data, 0)[0]
    
    # Sanity check
    if palette_count > 100 or palette_count * 12 + 4 > len(data):
        return output_paths
    
    # Parse sub-TOC
    toc_entries = []
    for i in range(palette_count):
        toc_offset = 4 + i * 12
        entry_id, size, offset = struct.unpack_from('<III', data, toc_offset)
        toc_entries.append({
            "index": i,
            "id": entry_id,
            "size": size,
            "offset": offset,
        })
    
    # Create palettes subdirectory
    palettes_dir = output_dir / "palettes"
    palettes_dir.mkdir(parents=True, exist_ok=True)
    
    # Extract each palette
    for entry in toc_entries:
        if entry["offset"] + 512 > len(data):
            continue
        
        colors = parse_palette_256(data, entry["offset"])
        
        # Save JSON
        palette_json_path = palettes_dir / f"palette_{entry['index']:02d}.json"
        palette_json_path.write_text(json.dumps({
            "index": entry["index"],
            "id": entry["id"],
            "size": entry["size"],
            "offset": entry["offset"],
            "color_count": len(colors),
            "colors": colors,
        }, indent=2))
        output_paths.append(palette_json_path)
        
        # Save PNG swatch
        if HAS_PIL:
            img = create_palette_image(colors)
            if img:
                png_path = palettes_dir / f"palette_{entry['index']:02d}.png"
                img.save(png_path)
                output_paths.append(png_path)
    
    # Summary
    summary = {
        "level": asset_info.level,
        "segment": asset_info.segment,
        "palette_count": palette_count,
        "palettes": toc_entries,
    }
    summary_path = palettes_dir / "summary.json"
    summary_path.write_text(json.dumps(summary, indent=2))
    output_paths.append(summary_path)
    
    return output_paths


@register_handler(602)
def level_palette_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for level palette / audio settings (asset type 602).
    
    In primary segment: 256-color level palette (512 bytes)
    In secondary segment: Audio settings table
    
    Creates:
    - Raw .bin file (via default handler)
    - palette.json: Parsed color data (if palette)
    - palette.png: Visual swatch (if palette)
    """
    from . import default_handler
    
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    # Check if this looks like a palette (512 bytes = 256 colors)
    if len(data) == 512:
        colors = parse_palette_256(data, 0)
        
        # Save JSON
        palette_path = output_dir / "level_palette.json"
        palette_path.write_text(json.dumps({
            "level": asset_info.level,
            "segment": asset_info.segment,
            "color_count": len(colors),
            "colors": colors,
        }, indent=2))
        output_paths.append(palette_path)
        
        # Save PNG swatch
        if HAS_PIL:
            img = create_palette_image(colors)
            if img:
                png_path = output_dir / "level_palette.png"
                img.save(png_path)
                output_paths.append(png_path)
    else:
        # Likely audio settings - just document size
        info_path = output_dir / "602_info.json"
        info_path.write_text(json.dumps({
            "level": asset_info.level,
            "segment": asset_info.segment,
            "size": len(data),
            "note": "Likely audio settings table (not a palette)",
        }, indent=2))
        output_paths.append(info_path)
    
    return output_paths
