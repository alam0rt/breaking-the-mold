"""Handler for asset type 502 - VRAM rectangle definitions.

Asset 502 contains texture page/rectangle definitions that specify
where graphics are placed in VRAM. Each entry is 16 bytes:
  - x1, y1: top-left corner (u16 each)
  - x2, y2: bottom-right corner (u16 each)  
  - type: rectangle type/usage (u16)
  - padding: 6 bytes reserved
"""
import json
import struct
from pathlib import Path
from . import register_handler


@register_handler(502)
def handle_vram_rects(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """Extract asset 502 - VRAM rectangle definitions."""
    output_dir.mkdir(parents=True, exist_ok=True)
    ENTRY_SIZE = 16
    
    if len(data) % ENTRY_SIZE != 0:
        # Fall back to binary dump
        bin_path = output_dir / "502_vram_rects.bin"
        bin_path.write_bytes(data)
        return [bin_path]
    
    count = len(data) // ENTRY_SIZE
    rectangles = []
    
    for i in range(count):
        offset = i * ENTRY_SIZE
        x1, y1, x2, y2, rect_type = struct.unpack_from('<HHHHH', data, offset)
        # Remaining 6 bytes are padding/reserved
        
        rect = {
            "index": i,
            "x1": x1,
            "y1": y1,
            "x2": x2,
            "y2": y2,
            "width": x2 - x1 if x2 > x1 else 0,
            "height": y2 - y1 if y2 > y1 else 0,
            "type": rect_type
        }
        rectangles.append(rect)
    
    result = {
        "asset_type": 502,
        "count": count,
        "entry_size": ENTRY_SIZE,
        "level": asset_info.level,
        "segment": asset_info.segment,
        "rectangles": rectangles
    }
    
    # Save as JSON
    json_path = output_dir / "502_vram_rects.json"
    with open(json_path, 'w') as f:
        json.dump(result, f, indent=2)
    
    return [json_path]
