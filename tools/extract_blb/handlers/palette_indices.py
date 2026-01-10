"""
Palette Indices Handler - Asset Type 301 (0x12D)

Extracts palette index data that maps each tile to its color palette.
Each byte specifies which palette from Asset 400 to use for that tile.

Format:
  - Array of u8 values, one per tile
  - Value = index into Asset 400's palette sub-TOC
"""

from pathlib import Path
import json
import struct
from collections import Counter
from . import register_handler


def parse_palette_indices(data: bytes) -> dict:
    """Parse palette index array."""
    if len(data) == 0:
        return {"error": "Empty data"}
    
    # Count unique palette indices
    index_counts = Counter(data)
    
    # Build statistics
    indices_used = sorted(index_counts.keys())
    stats = {}
    for idx in indices_used:
        count = index_counts[idx]
        percentage = count / len(data) * 100
        stats[idx] = {
            "count": count,
            "percentage": round(percentage, 2),
        }
    
    return {
        "tile_count": len(data),
        "unique_palettes": len(indices_used),
        "palette_indices_used": indices_used,
        "distribution": stats,
        "min_index": min(indices_used) if indices_used else 0,
        "max_index": max(indices_used) if indices_used else 0,
    }


@register_handler(301)
def palette_indices_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for palette indices (asset type 301).
    
    Creates:
    - Raw .bin file (via default handler)
    - palette_indices.json: Statistics about palette usage
    """
    from . import default_handler
    
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    # Parse indices
    indices_info = parse_palette_indices(data)
    indices_info["level"] = asset_info.level
    indices_info["segment"] = asset_info.segment
    
    # Save JSON
    json_path = output_dir / "palette_indices.json"
    json_path.write_text(json.dumps(indices_info, indent=2))
    output_paths.append(json_path)
    
    return output_paths
