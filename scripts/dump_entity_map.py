#!/usr/bin/env python3
"""
Dump entity placement data from PCSX-Redux runtime memory as ASCII map.

Usage:
    python3 scripts/dump_entity_map.py

Requires PCSX-Redux running with web server enabled and a level loaded.
"""

import struct
import base64
import sys

try:
    import requests
except ImportError:
    print("ERROR: requests module not found. Install with: pip install requests")
    sys.exit(1)

PCSX_URL = "http://localhost:8080"

def read_u16(addr):
    """Read u16 from PSX RAM"""
    resp = requests.get(f"{PCSX_URL}/api/v1/cpu/ram/raw?addr={addr}&size=2")
    return struct.unpack('<H', resp.content)[0]

def read_u32(addr):
    """Read u32 from PSX RAM"""
    resp = requests.get(f"{PCSX_URL}/api/v1/cpu/ram/raw?addr={addr}&size=4")
    return struct.unpack('<I', resp.content)[0]

def read_ram(addr, size):
    """Read raw bytes from PSX RAM"""
    resp = requests.get(f"{PCSX_URL}/api/v1/cpu/ram/raw?addr={addr}&size={size}")
    return resp.content

def main():
    print("=" * 70)
    print("ENTITY PLACEMENT MAP - Skullmonkeys Level Viewer")
    print("=" * 70)
    
    # Read LevelDataContext pointer locations
    GAMESTATE = 0x8009DC40
    LEVEL_DATA_CTX = GAMESTATE + 0x84  # 0x8009DCC4
    
    # Read TileHeader pointer (LevelDataContext + 0x04)
    tile_header_ptr = read_u32(LEVEL_DATA_CTX + 0x04)
    print(f"\nTileHeader at: 0x{tile_header_ptr:08X}")
    
    # Read TileHeader fields
    tile_header = read_ram(tile_header_ptr, 36)
    level_w = struct.unpack_from('<H', tile_header, 0x08)[0]
    level_h = struct.unpack_from('<H', tile_header, 0x0A)[0]
    spawn_x = struct.unpack_from('<H', tile_header, 0x0C)[0]
    spawn_y = struct.unpack_from('<H', tile_header, 0x0E)[0]
    entity_count = struct.unpack_from('<H', tile_header, 0x1E)[0]
    
    print(f"Level size: {level_w}x{level_h} tiles ({level_w*16}x{level_h*16} pixels)")
    print(f"Player spawn: tile ({spawn_x}, {spawn_y})")
    print(f"Entity count: {entity_count}")
    
    # Read entity data pointer (LevelDataContext + 0x38)
    entity_ptr = read_u32(LEVEL_DATA_CTX + 0x38)
    print(f"Entity data at: 0x{entity_ptr:08X}")
    
    # Read all entities (24 bytes each)
    ENTITY_SIZE = 24
    entity_data = read_ram(entity_ptr, entity_count * ENTITY_SIZE)
    
    # Parse entities
    entities = []
    for i in range(entity_count):
        offset = i * ENTITY_SIZE
        x1, y1, x2, y2, xc, yc = struct.unpack_from('<HHHHHH', entity_data, offset)
        pad_dword = struct.unpack_from('<I', entity_data, offset + 12)[0]
        pad_word, entity_type = struct.unpack_from('<HH', entity_data, offset + 16)
        flags = struct.unpack_from('<I', entity_data, offset + 20)[0]
        entities.append({
            'x1': x1, 'y1': y1,
            'x2': x2, 'y2': y2,
            'xc': xc, 'yc': yc,
            'type': entity_type,
            'flags': flags
        })
    
    # Entity type distribution
    type_counts = {}
    for e in entities:
        t = e['type']
        type_counts[t] = type_counts.get(t, 0) + 1
    
    print("\nEntity type distribution:")
    for t, count in sorted(type_counts.items()):
        print(f"  Type {t:3d} (0x{t:02x}): {count} entities")
    
    # Create ASCII map
    SCALE = 128  # pixels per char
    map_w = (level_w * 16) // SCALE + 1
    map_h = (level_h * 16) // SCALE + 1
    
    ascii_map = [['.' for _ in range(map_w)] for _ in range(map_h)]
    
    # Plot spawn
    sx = (spawn_x * 16) // SCALE
    sy = (spawn_y * 16) // SCALE
    if 0 <= sy < map_h and 0 <= sx < map_w:
        ascii_map[sy][sx] = 'S'
    
    # Entity symbols
    TYPE_SYMBOLS = {
        2: 'o',   # item/pickup
        3: '*',   # collectible
        8: '#',   # block
        10: '@',  # checkpoint
        24: '>',  # door
        25: 'E',  # enemy
        27: 'T',  # trigger
        28: 'M',  # monster
        42: '+',  # bonus
        45: '=',  # platform
        48: '%',  # hazard
    }
    
    # Plot entities
    for e in entities:
        x = e['xc'] // SCALE
        y = e['yc'] // SCALE
        if 0 <= y < map_h and 0 <= x < map_w:
            sym = TYPE_SYMBOLS.get(e['type'], str(e['type'] % 10))
            if ascii_map[y][x] == '.':
                ascii_map[y][x] = sym
    
    print(f"\nASCII Level Map (1 char = {SCALE}px = {SCALE//16} tiles)")
    print("Legend: S=spawn, E=enemy(25), T=trigger(27), #=block(8)")
    print("+" + "-"*map_w + "+")
    for row in ascii_map:
        print("|" + "".join(row) + "|")
    print("+" + "-"*map_w + "+")
    
    # Show entities by type
    print("\n" + "=" * 70)
    print("ENTITIES BY TYPE (detailed)")
    print("=" * 70)
    for t in sorted(type_counts.keys()):
        ents = [e for e in entities if e['type'] == t]
        print(f"\nType {t} (0x{t:02x}) - {len(ents)} entities:")
        for i, e in enumerate(ents[:10]):
            tx, ty = e['xc']//16, e['yc']//16
            w = (e['x2'] - e['x1'])
            h = (e['y2'] - e['y1'])
            print(f"  [{i:2d}] Tile ({tx:3d},{ty:2d})  Size: {w:3d}x{h:3d}px  Flags: 0x{e['flags']:08X}")
        if len(ents) > 10:
            print(f"  ... and {len(ents)-10} more")

if __name__ == "__main__":
    main()
