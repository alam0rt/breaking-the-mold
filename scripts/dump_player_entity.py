#!/usr/bin/env python3
"""
Dump player entity data from PCSX-Redux via REST API.

This script reads the player entity structure from RAM to understand:
- Current animation state
- Sprite ID being used
- Position and movement state
- Callback pointers

Usage:
    1. Start PCSX-Redux with web server: make emu
    2. Load a level in the game (e.g., SCIE stage 0)
    3. Run: python3 scripts/dump_player_entity.py

Based on Ghidra analysis:
- GameState @ 0x8009DC40
- Player entity pointer at GameState + 0x30
- Player entity size: 0x1B4 (436 bytes)
"""

import requests
import struct
import sys
from dataclasses import dataclass
from typing import Optional

# PCSX-Redux REST API base
PCSX_BASE = "http://localhost:8080"

# Key addresses (PAL / SLES-01090)
GAMESTATE_ADDR = 0x8009DC40
PLAYER_ENTITY_OFFSET = 0x30  # GameState + 0x30 = main player entity pointer


@dataclass
class PlayerEntity:
    """Player entity structure (0x1B4 bytes)"""
    address: int
    
    # Base entity offsets (0x00-0xFF)
    state_high: int           # 0x00 - State machine parameter
    tick_callback: int        # 0x04 - Main per-frame callback
    x_sort: int               # 0x08 - X position for sorting
    y_sort: int               # 0x0A - Y position for sorting
    z_order: int              # 0x10 - Render depth
    vtable: int               # 0x18 - Method table pointer
    secondary_callback: int   # 0x20 - Secondary update
    sprite_data_ptr: int      # 0x34 - Pointer to sprite frame data
    scale_x: int              # 0x58 - X scale (16.16 fixed)
    scale_y: int              # 0x5C - Y scale (16.16 fixed)
    x_pos: int                # 0x68 - X position (pixels)
    y_pos: int                # 0x6A - Y position (pixels)
    
    # Sprite/Animation offsets (0xBC-0xF5)
    current_sprite_id: int    # 0xBC - Current sprite ID (32-bit)
    anim_offset_1: int        # 0xC0 - Animation offset
    anim_offset_2: int        # 0xC4 - Animation offset 2
    pending_sprite_flags: int # 0xE0 - Pending sprite state
    anim_byte_f3: int         # 0xF3 - Animation byte
    anim_byte_f4: int         # 0xF4 - Animation byte
    anim_byte_f5: int         # 0xF5 - Animation byte
    
    # Player-specific offsets (0x100+)
    input_controller: int     # 0x100 - Pointer to button state
    state_offset: int         # 0x104 - State callback offset
    state_index: int          # 0x106 - State callback index
    state_callback: int       # 0x108 - State function pointer
    invincibility_timer: int  # 0x128 - Damage invincibility
    powerup_timer: int        # 0x144 - Powerup countdown
    rgb_r: int                # 0x15D - Base R
    rgb_g: int                # 0x15E - Base G
    rgb_b: int                # 0x15F - Base B
    game_mode: int            # 0x1B3 - Current game mode


def read_ram(address: int, size: int) -> bytes:
    """Read raw bytes from PSX RAM via PCSX-Redux REST API"""
    # Convert PSX address to offset (strip 0x80000000)
    offset = address & 0x1FFFFFFF
    url = f"{PCSX_BASE}/api/v1/cpu/ram/raw?address=0x{offset:08x}&size={size}"
    try:
        resp = requests.get(url, timeout=5)
        resp.raise_for_status()
        return resp.content
    except requests.RequestException as e:
        print(f"Error reading RAM at 0x{address:08X}: {e}")
        sys.exit(1)


def read_u8(address: int) -> int:
    """Read unsigned 8-bit value"""
    data = read_ram(address, 1)
    return data[0]


def read_u16(address: int) -> int:
    """Read unsigned 16-bit value (little-endian)"""
    data = read_ram(address, 2)
    return struct.unpack('<H', data)[0]


def read_s16(address: int) -> int:
    """Read signed 16-bit value (little-endian)"""
    data = read_ram(address, 2)
    return struct.unpack('<h', data)[0]


def read_u32(address: int) -> int:
    """Read unsigned 32-bit value (little-endian)"""
    data = read_ram(address, 4)
    return struct.unpack('<I', data)[0]


def get_player_entity_ptr() -> int:
    """Get pointer to player entity from GameState"""
    ptr = read_u32(GAMESTATE_ADDR + PLAYER_ENTITY_OFFSET)
    return ptr


def dump_player_entity(entity_addr: int) -> PlayerEntity:
    """Read and parse player entity structure"""
    # Read the full entity (0x1B4 bytes)
    data = read_ram(entity_addr, 0x1B4)
    
    def u8(off): return data[off]
    def u16(off): return struct.unpack('<H', data[off:off+2])[0]
    def s16(off): return struct.unpack('<h', data[off:off+2])[0]
    def u32(off): return struct.unpack('<I', data[off:off+4])[0]
    
    return PlayerEntity(
        address=entity_addr,
        state_high=u32(0x00),
        tick_callback=u32(0x04),
        x_sort=s16(0x08),
        y_sort=s16(0x0A),
        z_order=u16(0x10),
        vtable=u32(0x18),
        secondary_callback=u32(0x20),
        sprite_data_ptr=u32(0x34),
        scale_x=u32(0x58),
        scale_y=u32(0x5C),
        x_pos=s16(0x68),
        y_pos=s16(0x6A),
        current_sprite_id=u32(0xBC),
        anim_offset_1=u32(0xC0),
        anim_offset_2=u32(0xC4),
        pending_sprite_flags=u16(0xE0),
        anim_byte_f3=u8(0xF3),
        anim_byte_f4=u8(0xF4),
        anim_byte_f5=u8(0xF5),
        input_controller=u32(0x100),
        state_offset=u16(0x104),
        state_index=u16(0x106),
        state_callback=u32(0x108),
        invincibility_timer=u8(0x128),
        powerup_timer=u16(0x144),
        rgb_r=u8(0x15D),
        rgb_g=u8(0x15E),
        rgb_b=u8(0x15F),
        game_mode=u8(0x1B3),
    )


def dump_sprite_context(sprite_ptr: int) -> dict:
    """Dump sprite render context structure (at entity + 0x34 pointer)"""
    if sprite_ptr == 0:
        return {"error": "NULL sprite pointer"}
    
    # Read sprite context (approx 0x40 bytes based on Ghidra)
    data = read_ram(sprite_ptr, 0x50)
    
    def u8(off): return data[off]
    def u16(off): return struct.unpack('<H', data[off:off+2])[0]
    def s16(off): return struct.unpack('<h', data[off:off+2])[0]
    def u32(off): return struct.unpack('<I', data[off:off+4])[0]
    
    return {
        "address": f"0x{sprite_ptr:08X}",
        "sprite_id": f"0x{u32(0x00):08X}",
        "anim_index": u16(0x04),
        "frame_index": u16(0x06),
        "frame_timer": u16(0x08),
        "flags_0a": u8(0x0A),
        "width": u16(0x10),
        "height": u16(0x12),
        "render_x": s16(0x14),
        "render_y": s16(0x16),
        "palette_ptr": f"0x{u32(0x20):08X}",
        "rle_data_ptr": f"0x{u32(0x24):08X}",
        "rgb_r": u8(0x34),
        "rgb_g": u8(0x35),
        "rgb_b": u8(0x36),
    }


def main():
    print("=" * 60)
    print("PCSX-Redux Player Entity Dump")
    print("=" * 60)
    
    # Check connection
    try:
        resp = requests.get(f"{PCSX_BASE}/api/v1/cpu/status", timeout=2)
        status = resp.json()
        print(f"Emulator status: {'PAUSED' if status.get('paused') else 'RUNNING'}")
    except Exception as e:
        print(f"Cannot connect to PCSX-Redux: {e}")
        print("Make sure PCSX-Redux is running with web server enabled.")
        print("Start with: make emu")
        sys.exit(1)
    
    # Get player entity pointer
    player_ptr = get_player_entity_ptr()
    if player_ptr == 0 or player_ptr < 0x80000000:
        print(f"\nNo player entity found (ptr = 0x{player_ptr:08X})")
        print("Make sure a level is loaded in the game.")
        sys.exit(1)
    
    print(f"\nGameState: 0x{GAMESTATE_ADDR:08X}")
    print(f"Player entity pointer: 0x{player_ptr:08X}")
    
    # Dump player entity
    entity = dump_player_entity(player_ptr)
    
    print("\n" + "-" * 40)
    print("PLAYER ENTITY")
    print("-" * 40)
    print(f"Address:           0x{entity.address:08X}")
    print(f"Position:          ({entity.x_pos}, {entity.y_pos})")
    print(f"Sort position:     ({entity.x_sort}, {entity.y_sort})")
    print(f"Z-order:           {entity.z_order}")
    print(f"Scale:             X=0x{entity.scale_x:08X} Y=0x{entity.scale_y:08X}")
    print(f"                   ({entity.scale_x / 0x10000:.3f}x, {entity.scale_y / 0x10000:.3f}x)")
    
    print("\n" + "-" * 40)
    print("CALLBACKS")
    print("-" * 40)
    print(f"Tick callback:     0x{entity.tick_callback:08X}")
    print(f"Secondary:         0x{entity.secondary_callback:08X}")
    print(f"VTable:            0x{entity.vtable:08X}")
    print(f"State callback:    0x{entity.state_callback:08X}")
    print(f"State index:       {entity.state_index} (offset: 0x{entity.state_offset:04X})")
    
    print("\n" + "-" * 40)
    print("SPRITE / ANIMATION")
    print("-" * 40)
    print(f"Sprite ID:         0x{entity.current_sprite_id:08X}")
    print(f"Sprite data ptr:   0x{entity.sprite_data_ptr:08X}")
    print(f"Anim offset 1:     0x{entity.anim_offset_1:08X}")
    print(f"Anim offset 2:     0x{entity.anim_offset_2:08X}")
    print(f"Pending flags:     0x{entity.pending_sprite_flags:04X}")
    print(f"Anim bytes:        F3=0x{entity.anim_byte_f3:02X} F4=0x{entity.anim_byte_f4:02X} F5=0x{entity.anim_byte_f5:02X}")
    print(f"RGB:               ({entity.rgb_r}, {entity.rgb_g}, {entity.rgb_b})")
    
    print("\n" + "-" * 40)
    print("STATE")
    print("-" * 40)
    print(f"Game mode:         {entity.game_mode}")
    print(f"Invincibility:     {entity.invincibility_timer}")
    print(f"Powerup timer:     {entity.powerup_timer}")
    print(f"Input controller:  0x{entity.input_controller:08X}")
    
    # Dump sprite context if available
    if entity.sprite_data_ptr != 0:
        print("\n" + "-" * 40)
        print("SPRITE CONTEXT (at sprite_data_ptr)")
        print("-" * 40)
        sprite_ctx = dump_sprite_context(entity.sprite_data_ptr)
        for key, val in sprite_ctx.items():
            print(f"{key:18} {val}")
    
    # Read raw entity bytes for debugging
    print("\n" + "-" * 40)
    print("RAW ENTITY BYTES (first 256)")
    print("-" * 40)
    raw = read_ram(entity.address, 256)
    for i in range(0, 256, 16):
        hex_str = ' '.join(f'{b:02X}' for b in raw[i:i+16])
        print(f"0x{i:03X}: {hex_str}")


if __name__ == "__main__":
    main()
