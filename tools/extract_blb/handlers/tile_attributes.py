"""
Tile Attributes Handler - Asset Type 500 (0x1F4)

Extracts tile attribute/collision map data.
Each byte represents collision/trigger properties for one tile.

TileAttributeMap structure:
  Offset  Size  Field
  0x00    u32   unknown (always 0, padding)
  0x04    u16   level_width (in tiles)
  0x06    u16   level_height (in tiles)
  0x08    n     tile_data (1 byte per tile, row-major)

Known attribute values:
  0x00 = Empty/passable
  0x02 = Solid/collision
  0x12 = Unknown trigger
  0x53 = Checkpoint/save point?
  0x65 = Entity spawn zone/trigger
"""

from pathlib import Path
import json
import struct
from collections import Counter
from . import register_handler

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


# Known attribute types and their colors for visualization
ATTRIBUTE_COLORS = {
    0x00: (0, 0, 0, 0),         # Empty - transparent
    0x02: (255, 0, 0, 200),     # Solid - red
    0x12: (255, 255, 0, 200),   # Trigger - yellow
    0x53: (0, 255, 0, 200),     # Checkpoint - green
    0x65: (0, 0, 255, 200),     # Spawn zone - blue
}

ATTRIBUTE_NAMES = {
    0x00: "empty",
    0x02: "solid",
    0x12: "trigger_12",
    0x53: "checkpoint",
    0x65: "spawn_zone",
}


def parse_tile_attributes(data: bytes) -> dict:
    """Parse tile attribute map data."""
    if len(data) < 8:
        return {"error": f"Data too short: {len(data)} bytes, expected at least 8"}
    
    # Parse header
    unknown, width, height = struct.unpack_from('<IHH', data, 0)
    
    # Validate dimensions
    expected_size = 8 + width * height
    if len(data) < expected_size:
        # Some levels have truncated data, just use what we have
        pass
    
    # Extract tile data
    tile_data = data[8:]
    actual_tiles = len(tile_data)
    
    # Count attribute types
    attr_counts = Counter(tile_data)
    
    # Build attribute statistics
    attr_stats = {}
    for attr_value, count in sorted(attr_counts.items()):
        name = ATTRIBUTE_NAMES.get(attr_value, f"attr_{attr_value:02x}")
        percentage = count / actual_tiles * 100 if actual_tiles > 0 else 0
        attr_stats[f"0x{attr_value:02x}_{name}"] = {
            "value": attr_value,
            "count": count,
            "percentage": round(percentage, 2),
        }
    
    return {
        "header": {
            "unknown": unknown,
            "level_width": width,
            "level_height": height,
        },
        "expected_tiles": width * height,
        "actual_tiles": actual_tiles,
        "unique_attributes": len(attr_counts),
        "attributes": attr_stats,
    }


def create_collision_image(data: bytes, width: int, height: int) -> 'Image.Image':
    """Create a visual collision map image."""
    if not HAS_PIL:
        return None
    
    tile_data = data[8:]
    if len(tile_data) < width * height:
        # Truncated - adjust height
        height = len(tile_data) // width
    
    if width == 0 or height == 0:
        return None
    
    # Create RGBA image
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    
    for y in range(height):
        for x in range(width):
            idx = y * width + x
            if idx >= len(tile_data):
                break
            attr = tile_data[idx]
            color = ATTRIBUTE_COLORS.get(attr, (128, 128, 128, 128))
            img.putpixel((x, y), color)
    
    return img


@register_handler(500)
def tile_attributes_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for tile attribute map (asset type 500).
    
    Creates:
    - Raw .bin file (via default handler)
    - tile_attributes.json: Parsed header and attribute statistics
    - collision_map.png: Visual collision map (1 pixel per tile)
    """
    from . import default_handler
    
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    # Parse attributes
    attrs = parse_tile_attributes(data)
    attrs["level"] = asset_info.level
    attrs["segment"] = asset_info.segment
    
    # Save JSON
    json_path = output_dir / "tile_attributes.json"
    json_path.write_text(json.dumps(attrs, indent=2))
    output_paths.append(json_path)
    
    # Create collision map image
    if HAS_PIL and "header" in attrs:
        width = attrs["header"]["level_width"]
        height = attrs["header"]["level_height"]
        if width > 0 and height > 0:
            img = create_collision_image(data, width, height)
            if img:
                png_path = output_dir / "collision_map.png"
                img.save(png_path)
                output_paths.append(png_path)
    
    return output_paths
