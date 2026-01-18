#!/usr/bin/env python3
"""
Ghidra Fix Script: TileHeader Struct
Updates TileHeader (Asset 100, 36 bytes) with verified field names.

Run in Ghidra's Python console or via script manager.
Source of truth: scripts/blb.hexpat
"""

from ghidra.program.model.data import (
    StructureDataType, CategoryPath, ByteDataType, 
    UnsignedShortDataType, ArrayDataType
)

def fix_tile_header():
    dtm = currentProgram.getDataTypeManager()
    
    # Find or create category
    category = CategoryPath("/Game")
    
    # Create new struct (36 bytes)
    struct = StructureDataType(category, "TileHeader", 0)
    
    byte_type = ByteDataType.dataType
    u16_type = UnsignedShortDataType.dataType
    
    # Background color (0x00-0x03)
    struct.add(byte_type, 1, "bg_r", "Background red (0-255)")
    struct.add(byte_type, 1, "bg_g", "Background green")
    struct.add(byte_type, 1, "bg_b", "Background blue")
    struct.add(byte_type, 1, "pad_03", None)
    
    # Secondary/fog color (0x04-0x07)
    struct.add(byte_type, 1, "secondary_r", "Fog/secondary red")
    struct.add(byte_type, 1, "secondary_g", "Fog/secondary green")
    struct.add(byte_type, 1, "secondary_b", "Fog/secondary blue")
    struct.add(byte_type, 1, "pad_07", None)
    
    # Level dimensions (0x08-0x0B)
    struct.add(u16_type, 2, "level_width", "Level width in tiles")
    struct.add(u16_type, 2, "level_height", "Level height in tiles")
    
    # Spawn position (0x0C-0x0F)
    struct.add(u16_type, 2, "spawn_x", "Player spawn X (tiles)")
    struct.add(u16_type, 2, "spawn_y", "Player spawn Y (tiles)")
    
    # Tile counts (0x10-0x15)
    struct.add(u16_type, 2, "count_16x16", "Number of 16x16 pixel tiles")
    struct.add(u16_type, 2, "count_8x8_a", "Primary 8x8 tile count")
    struct.add(u16_type, 2, "count_8x8_b", "Additional tile count (often 0)")
    
    # Vehicle waypoint count (0x16)
    struct.add(u16_type, 2, "vehicle_waypoint_count", "Asset 504 waypoint count (FINN/RUNN)")
    
    # Level flags (0x18)
    struct.add(u16_type, 2, "level_flags", "Level behavior bitfield")
    
    # Special level ID (0x1A)
    struct.add(u16_type, 2, "special_level_id", "99=special mode (FINN/SEVN), 0=normal")
    
    # VRAM rect count (0x1C)
    struct.add(u16_type, 2, "vram_rect_count", "Number of VRAM rectangles (Asset 502)")
    
    # Entity count (0x1E)
    struct.add(u16_type, 2, "entity_count", "Number of entities in Asset 501")
    
    # World index (0x20)
    struct.add(u16_type, 2, "world_index", "World/area index (0-6)")
    
    # Padding (0x22-0x23)
    struct.add(ArrayDataType(byte_type, 2, 1), 2, "pad_22", None)
    
    # Replace existing or add new
    existing = dtm.getDataType(category, "TileHeader")
    if existing:
        dtm.replaceDataType(existing, struct, True)
        print("Replaced existing TileHeader struct")
    else:
        dtm.addDataType(struct, None)
        print("Added new TileHeader struct")
    
    print(f"TileHeader: {struct.getLength()} bytes")
    return struct

if __name__ == "__main__":
    fix_tile_header()
