"""Handler for asset type 504 - Vehicle/path data (FINN/RUNN levels only).

Asset 504 contains vehicle path waypoint data for the FINN and RUNN vehicle
levels. Each entry is 64 bytes and appears to contain bounding box coordinates
and navigation/trigger data.

Observed structure (64 bytes per entry):
  - u16[0-1]: flags/type + padding
  - u16[2-3]: some count + padding  
  - u16[4-7]: bounding box (x1, y1, x2, y2)
  - u16[8-11]: center point (x, y) + trigger flags
  - u16[12-31]: additional waypoint/trigger data
"""
import json
import struct
from pathlib import Path
from . import register_handler


@register_handler(504)
def handle_vehicle_data(asset_type: int, data: bytes, output_dir: Path, level_name: str, stage_num: int, ctx: dict) -> dict:
    """Extract asset 504 - vehicle path/waypoint data."""
    ENTRY_SIZE = 64
    
    if len(data) % ENTRY_SIZE != 0:
        return {
            "status": "error",
            "error": f"Size {len(data)} not divisible by {ENTRY_SIZE}"
        }
    
    count = len(data) // ENTRY_SIZE
    waypoints = []
    
    for i in range(count):
        offset = i * ENTRY_SIZE
        
        # Parse the 64-byte entry as 32 u16 values
        vals = struct.unpack_from('<' + 'H' * 32, data, offset)
        
        waypoint = {
            "index": i,
            "type": vals[0],
            "count": vals[2],
            "bbox": {
                "x1": vals[4],
                "y1": vals[5],
                "x2": vals[6],
                "y2": vals[7]
            },
            "center": {
                "x": vals[8],
                "y": vals[9]
            },
            "flags": [vals[10], vals[11]],
            "trigger_id": vals[12],
            "extra": list(vals[13:]),
            "raw_hex": data[offset:offset + ENTRY_SIZE].hex()
        }
        waypoints.append(waypoint)
    
    result = {
        "count": count,
        "entry_size": ENTRY_SIZE,
        "level": level_name,
        "waypoints": waypoints
    }
    
    # Save as JSON
    json_path = output_dir / f"{asset_type:03d}_vehicle_data.json"
    with open(json_path, 'w') as f:
        json.dump(result, f, indent=2)
    
    return {
        "status": "handled",
        "files": [str(json_path)],
        "waypoint_count": count,
        "description": f"{count} vehicle waypoints"
    }
