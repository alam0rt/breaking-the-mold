"""
Tile Header Handler - Asset Type 100 (0x064)

Extracts tile header metadata containing level dimensions, spawn position,
background colors, and tile counts.

TileHeader structure (36 bytes, verified via Ghidra):
  Offset  Size  Field                   Ghidra Function
  0x00    u8    bg_r                    LoadBGColorFromTileHeader (0x80024678)
  0x01    u8    bg_g                    LoadBGColorFromTileHeader
  0x02    u8    bg_b                    LoadBGColorFromTileHeader
  0x03    u8    padding
  0x04    u8    secondary_r             LoadSecondaryColorFromTileHeader (0x800246d0)
  0x05    u8    secondary_g             LoadSecondaryColorFromTileHeader
  0x06    u8    secondary_b             LoadSecondaryColorFromTileHeader
  0x07    u8    padding
  0x08    u16   level_width             GetLevelDimensions (0x8007b434)
  0x0A    u16   level_height            GetLevelDimensions
  0x0C    u16   spawn_x                 GetSpawnPosition (0x8007b458)
  0x0E    u16   spawn_y                 GetSpawnPosition
  0x10    u16   count_16x16             GetTotalTileCount (0x8007b53c)
  0x12    u16   count_8x8_a             GetTotalTileCount
  0x14    u16   count_8x8_b             GetTotalTileCount
  0x16    u16   vehicle_waypoint_count  (Matches Asset 504 count, FINN/RUNN only)
  0x18    u16   level_flags             (Bitfield, see below)
  0x1A    u16   special_level_id        (99 = FINN/SEVN special modes)
  0x1C    u16   vram_rect_count         GetAsset100Field1C (0x8007b7c8)
  0x1E    u16   entity_count            FUN_8007b7a8
  0x20    u16   field_20                (Unknown, values 1-6)
  0x22    u16   padding

Level flags bitfield (0x18):
  Bit 1 (0x0002): FINN, SEVN - special gameplay modes
  Bit 2 (0x0004): FINN only
  Bit 3 (0x0008): EGGS, FOOD, GLID, HEAD, MEGA, TMPL, WEED
  Bit 4 (0x0010): RUNN only (vehicle runner stage)
  Bit 6 (0x0040): FOOD, GLEN, HEAD, MEGA, MENU, RUNN, TMPL, WIZZ
"""

from pathlib import Path
import json
import struct
from . import register_handler


def parse_tile_header(data: bytes) -> dict:
    """Parse a 36-byte tile header structure."""
    if len(data) < 36:
        return {"error": f"Data too short: {len(data)} bytes, expected 36"}
    
    # Unpack all fields
    bg_r, bg_g, bg_b, pad1 = struct.unpack_from('<BBBB', data, 0)
    sec_r, sec_g, sec_b, pad2 = struct.unpack_from('<BBBB', data, 4)
    level_w, level_h = struct.unpack_from('<HH', data, 8)
    spawn_x, spawn_y = struct.unpack_from('<HH', data, 12)
    count_16x16, count_8x8_a, count_8x8_b = struct.unpack_from('<HHH', data, 16)
    vehicle_waypoint_count, level_flags, special_level_id = struct.unpack_from('<HHH', data, 22)
    vram_rect_count, entity_count = struct.unpack_from('<HH', data, 28)
    field_20, = struct.unpack_from('<H', data, 32)
    
    total_tiles = count_16x16 + count_8x8_a + count_8x8_b
    
    return {
        "background_color": {
            "r": bg_r, "g": bg_g, "b": bg_b,
            "hex": f"#{bg_r:02x}{bg_g:02x}{bg_b:02x}"
        },
        "secondary_color": {
            "r": sec_r, "g": sec_g, "b": sec_b,
            "hex": f"#{sec_r:02x}{sec_g:02x}{sec_b:02x}"
        },
        "level_size": {
            "width_tiles": level_w,
            "height_tiles": level_h,
            "width_pixels": level_w * 16,
            "height_pixels": level_h * 16,
        },
        "spawn_position": {
            "tile_x": spawn_x,
            "tile_y": spawn_y,
            # spawn_x_pixels = spawn_x * 16 + 8, spawn_y_pixels = spawn_y * 16 + 15
            "pixel_x": spawn_x * 16 + 8,
            "pixel_y": spawn_y * 16 + 15,
        },
        "tile_counts": {
            "count_16x16": count_16x16,
            "count_8x8_a": count_8x8_a,
            "count_8x8_b": count_8x8_b,
            "total": total_tiles,
        },
        "level_metadata": {
            "vehicle_waypoint_count": vehicle_waypoint_count,  # Matches 504.count (FINN/RUNN)
            "level_flags": level_flags,                         # Bitfield (bits 3 and 6 observed)
            "special_level_id": special_level_id,               # 99 = FINN/SEVN
        },
        "related_counts": {
            "vram_rect_count": vram_rect_count,  # Matches 502.count
            "entity_count": entity_count,         # Matches 501.count
        },
        "unknown": {
            "field_20": field_20,  # Values 1-6 observed
        },
        "raw_size": len(data),
    }


@register_handler(100)
def tile_header_handler(data: bytes, asset_info, output_dir: Path, context: dict) -> list[Path]:
    """
    Handler for tile header data (asset type 100).
    
    Creates:
    - Raw .bin file (via default handler)
    - tile_header.json: Parsed header with readable field names
    """
    from . import default_handler
    
    # Run default handler first (saves raw .bin)
    output_paths = default_handler(data, asset_info, output_dir, context)
    
    # Parse and save structured data
    header = parse_tile_header(data)
    header["level"] = asset_info.level
    header["segment"] = asset_info.segment
    
    header_path = output_dir / "tile_header.json"
    header_path.write_text(json.dumps(header, indent=2))
    output_paths.append(header_path)
    
    return output_paths
