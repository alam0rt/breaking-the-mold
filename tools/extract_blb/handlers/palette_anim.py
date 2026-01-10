"""
Palette Animation Handler - Asset Type 401 (0x191)

Extracts per-palette animation configuration data.
Size = palette_count × 4 bytes (where palette_count comes from Asset 400)

Each palette has a 4-byte animation entry:
  Offset  Size  Field
  0x00    u8    enabled (0=no animation, 1=animate)
  0x01    u8    start_index (first color to cycle)
  0x02    u8    end_index (last color to cycle)
  0x03    u8    speed (animation speed, higher=faster)

When enabled=1, colors from start_index to end_index cycle/rotate
at the given speed. This creates animated effects like flowing water,
lava, or glowing objects using palette rotation.

Common patterns:
  (1, 2, 253, 1)   - Cycle colors 2-253 slowly
  (1, 2, 253, 2)   - Cycle colors 2-253 medium speed
  (1, 216, 253, 2) - Cycle colors 216-253 (last ~40 colors)
  (1, 17, 253, 1)  - Cycle colors 17-253

VERIFIED: Size = palette_count × 4 in 104/104 secondary segments
"""

from pathlib import Path
import json
import struct
from . import register_handler


def parse_palette_anim_entry(data: bytes, offset: int = 0) -> dict:
    """Parse a single 4-byte palette animation entry."""
    if len(data) < offset + 4:
        return None
    
    enabled, start_idx, end_idx, speed = struct.unpack_from('BBBB', data, offset)
    
    return {
        "enabled": enabled == 1,
        "start_index": start_idx,
        "end_index": end_idx,
        "color_count": end_idx - start_idx + 1 if end_idx >= start_idx else 0,
        "speed": speed,
    }


def parse_palette_anim(data: bytes) -> list[dict]:
    """Parse all palette animation entries."""
    entries = []
    for offset in range(0, len(data), 4):
        entry = parse_palette_anim_entry(data, offset)
        if entry:
            entry["palette_index"] = offset // 4
            entries.append(entry)
    return entries


@register_handler(401)
def palette_anim_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for palette animation data (asset type 401).
    
    Creates:
    - Raw .bin file (via default handler)
    - palette_anim.json: Parsed animation config per palette
    """
    from . import default_handler
    
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    # Parse animation entries
    entries = parse_palette_anim(data)
    
    # Count animated palettes
    animated = [e for e in entries if e["enabled"]]
    
    # Build output
    result = {
        "level": asset_info.level,
        "segment": asset_info.segment,
        "palette_count": len(entries),
        "animated_count": len(animated),
        "entries": entries,
    }
    
    # Add summary of animated palettes
    if animated:
        result["animated_palettes"] = [
            {
                "palette": e["palette_index"],
                "range": f"{e['start_index']}-{e['end_index']}",
                "colors": e["color_count"],
                "speed": e["speed"],
            }
            for e in animated
        ]
    
    # Save JSON
    json_path = output_dir / "palette_anim.json"
    json_path.write_text(json.dumps(result, indent=2))
    output_paths.append(json_path)
    
    return output_paths
