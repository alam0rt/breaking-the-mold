#!/usr/bin/env python3
"""
RAM Snapshot Tool for Skullmonkeys (SLES-01090) PAL
Captures and parses game state from PCSX-Redux emulator.

Usage:
    python3 scripts/ram_snapshot.py [--output snapshot.json] [--verbose]
"""

import argparse
import json
import struct
import sys
import urllib.request
import urllib.error
from dataclasses import dataclass, asdict
from typing import Optional, List, Dict, Any
from datetime import datetime

# =============================================================================
# Memory Addresses (PAL / SLES-01090)
# =============================================================================

ADDRESSES = {
    # Core structures
    "GameState": 0x8009DC40,
    "LevelDataContext": 0x8009DCC4,
    "BLBHeader": 0x800AE3E0,
    "EntityTypeCallbackTable": 0x8009D5F8,
    
    # BLB header offsets
    "LevelCount": 0x800AF311,
    "MovieCount": 0x800AF312,
    "GameMode": 0x800AF316,
    "CurrentLevelIndex": 0x800AF372,
    
    # File info
    "GameBLBFile": 0x8009B4B4,
    "GameBLBSector": 0x800A59F0,
    
    # Player sprite table
    "PlayerSpriteTable": 0x8009C174,
}

# GameState offsets
GAMESTATE_OFFSETS = {
    "entity_tick_list": 0x1C,
    "entity_render_list": 0x20,
    "entity_collision_queue": 0x24,
    "entity_def_pool": 0x28,
    "player_alt": 0x2C,
    "player": 0x30,
    "camera_x": 0x38,
    "camera_y": 0x3C,
    "entity_callback_table": 0x7C,
    "level_context": 0x84,
}

# Player entity offsets (size 0x1B4 = 436 bytes)
PLAYER_OFFSETS = {
    "callback_ptr": 0x04,
    "x_pos": 0x48,
    "y_pos": 0x4A,
    "x_velocity": 0x4C,
    "y_velocity": 0x4E,
    "width": 0x50,
    "height": 0x52,
    "facing": 0x74,  # 0=right, 1=left
    "anim_frames_table": 0x78,
    "state_offset": 0xA0,
    "state_index": 0xA2,
    "state_callback": 0xA4,
    "sprite_id": 0xBC,
    "sprite_id_current": 0xCC,
    "anim_frame_index": 0xDA,
    "anim_frame_end": 0xDE,
    "anim_flags": 0xE0,
    "frame_delay": 0xEC,
}

# LevelDataContext offsets (array of pointers)
LEVEL_CONTEXT_OFFSETS = {
    "tile_header": 0x04,        # ctx[1]
    "vram_slot_config": 0x08,  # ctx[2]
    "tilemap_container": 0x0C, # ctx[3]
    "layer_entries": 0x10,     # ctx[4]
    "tile_pixels": 0x14,       # ctx[5]
    "palette_indices": 0x18,   # ctx[6]
    "tile_flags": 0x1C,        # ctx[7]
    "palette_container": 0x20, # ctx[8]
    "palette_anim": 0x24,      # ctx[9]
    "animated_tiles": 0x28,    # ctx[10]
    "tile_attributes": 0x2C,   # ctx[11]
    "anim_offsets": 0x30,      # ctx[12]
    "vehicle_data": 0x34,      # ctx[13]
    "entities": 0x38,          # ctx[14]
    "vram_rects": 0x3C,        # ctx[15]
    "sprites": 0x40,           # ctx[16]
    "sprites_size": 0x44,      # ctx[17]
    "audio": 0x48,             # ctx[18]
    "audio_size": 0x4C,        # ctx[19]
    "palette": 0x50,           # ctx[20]
}

# =============================================================================
# PCSX-Redux API
# =============================================================================

class PCSXRedux:
    def __init__(self, host: str = "localhost", port: int = 8080):
        self.base_url = f"http://{host}:{port}"
    
    def get_status(self) -> Dict[str, Any]:
        """Get emulator status."""
        try:
            with urllib.request.urlopen(f"{self.base_url}/api/v1/execution-flow") as resp:
                return json.loads(resp.read())
        except urllib.error.URLError as e:
            raise ConnectionError(f"Cannot connect to PCSX-Redux: {e}")
    
    def read_ram(self, address: int, size: int) -> bytes:
        """Read bytes from PSX RAM."""
        url = f"{self.base_url}/api/v1/cpu/ram/raw?address={address:#x}&size={size}"
        try:
            with urllib.request.urlopen(url) as resp:
                return resp.read()
        except urllib.error.URLError as e:
            raise ConnectionError(f"Failed to read RAM: {e}")
    
    def read_u8(self, address: int) -> int:
        return struct.unpack('<B', self.read_ram(address, 1))[0]
    
    def read_u16(self, address: int) -> int:
        return struct.unpack('<H', self.read_ram(address, 2))[0]
    
    def read_s16(self, address: int) -> int:
        return struct.unpack('<h', self.read_ram(address, 2))[0]
    
    def read_u32(self, address: int) -> int:
        return struct.unpack('<I', self.read_ram(address, 4))[0]
    
    def read_s32(self, address: int) -> int:
        return struct.unpack('<i', self.read_ram(address, 4))[0]

# =============================================================================
# Data Structures
# =============================================================================

@dataclass
class TileHeader:
    bg_color: int
    spawn_x: int
    spawn_y: int
    tile_count_16x16: int
    tile_count_8x8: int
    tile_count_extra: int
    layer_count: int
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'TileHeader':
        if len(data) < 36:
            return None
        values = struct.unpack('<HHHHHHH', data[:14])
        return cls(
            bg_color=values[0],
            spawn_x=values[1],
            spawn_y=values[2],
            tile_count_16x16=values[3],
            tile_count_8x8=values[4],
            tile_count_extra=values[5],
            layer_count=values[6],
        )

@dataclass
class PlayerState:
    address: int
    x: int
    y: int
    velocity_x: int
    velocity_y: int
    width: int
    height: int
    facing: str
    sprite_id: int
    current_sprite_id: int
    anim_frame: int
    anim_frame_end: int
    frame_delay: int
    anim_flags: int
    state_offset: int
    state_index: int
    callback_ptr: int
    anim_table_ptr: int

@dataclass
class EntityDef:
    """Raw 24-byte entity definition from Asset 501."""
    x1: int
    y1: int
    x2: int
    y2: int
    x_center: int
    y_center: int
    variant: int
    entity_type: int
    layer: int
    
    @classmethod
    def from_bytes(cls, data: bytes) -> 'EntityDef':
        if len(data) < 24:
            return None
        values = struct.unpack('<HHHHHHHHHHHH', data[:24])
        return cls(
            x1=values[0],
            y1=values[1],
            x2=values[2],
            y2=values[3],
            x_center=values[4],
            y_center=values[5],
            variant=values[6],
            # values[7-8] are padding
            entity_type=values[9],
            layer=values[10],
            # values[11] is padding
        )

@dataclass
class GameSnapshot:
    timestamp: str
    emulator_running: bool
    
    # BLB header info
    level_count: int
    movie_count: int
    game_mode: int
    current_level_index: int
    blb_sector: int
    
    # GameState
    gamestate_address: int
    camera_x: int
    camera_y: int
    entity_tick_list: int
    entity_render_list: int
    player_ptr: int
    
    # Level context
    level_context_address: int
    tile_header: Optional[TileHeader]
    level_context_pointers: Dict[str, int]
    
    # Player
    player: Optional[PlayerState]
    
    # Entity definitions (first N)
    entity_defs: List[EntityDef]
    entity_count: int

# =============================================================================
# Snapshot Functions
# =============================================================================

def capture_snapshot(pcsx: PCSXRedux, max_entities: int = 50, verbose: bool = False) -> GameSnapshot:
    """Capture a complete game state snapshot."""
    
    def log(msg):
        if verbose:
            print(f"  {msg}")
    
    print("Capturing RAM snapshot...")
    
    # Check emulator status
    try:
        status = pcsx.get_status()
        running = status.get("running", False)
    except:
        running = True  # Assume running if can't check
    
    log(f"Emulator running: {running}")
    
    # Read BLB header info
    log("Reading BLB header...")
    try:
        level_count = pcsx.read_u8(ADDRESSES["LevelCount"])
        movie_count = pcsx.read_u8(ADDRESSES["MovieCount"])
        game_mode = pcsx.read_u8(ADDRESSES["GameMode"])
        current_level = pcsx.read_u8(ADDRESSES["CurrentLevelIndex"])
        blb_sector = pcsx.read_u32(ADDRESSES["GameBLBSector"])
    except Exception as e:
        log(f"BLB header not loaded yet: {e}")
        level_count = 0
        movie_count = 0
        game_mode = 0
        current_level = 0
        blb_sector = 0
    
    log(f"Level {current_level}/{level_count}, mode={game_mode}")
    
    # Read GameState
    log("Reading GameState...")
    gs_addr = ADDRESSES["GameState"]
    gs_data = pcsx.read_ram(gs_addr, 0x140)
    
    def gs_u32(offset):
        return struct.unpack('<I', gs_data[offset:offset+4])[0]
    def gs_s16(offset):
        return struct.unpack('<h', gs_data[offset:offset+2])[0]
    
    camera_x = gs_s16(GAMESTATE_OFFSETS["camera_x"])
    camera_y = gs_s16(GAMESTATE_OFFSETS["camera_y"])
    entity_tick = gs_u32(GAMESTATE_OFFSETS["entity_tick_list"])
    entity_render = gs_u32(GAMESTATE_OFFSETS["entity_render_list"])
    player_ptr = gs_u32(GAMESTATE_OFFSETS["player"])
    
    log(f"Camera: ({camera_x}, {camera_y})")
    log(f"Player ptr: {player_ptr:#010x}")
    
    # Read LevelDataContext
    log("Reading LevelDataContext...")
    ldc_addr = ADDRESSES["LevelDataContext"]
    ldc_data = pcsx.read_ram(ldc_addr, 0x60)
    
    level_context_pointers = {}
    for name, offset in LEVEL_CONTEXT_OFFSETS.items():
        if offset + 4 <= len(ldc_data):
            ptr = struct.unpack('<I', ldc_data[offset:offset+4])[0]
            level_context_pointers[name] = ptr
    
    # Read tile header
    tile_header = None
    tile_header_ptr = level_context_pointers.get("tile_header", 0)
    if tile_header_ptr and tile_header_ptr > 0x80000000:
        log(f"Reading tile header at {tile_header_ptr:#010x}")
        try:
            th_data = pcsx.read_ram(tile_header_ptr, 36)
            tile_header = TileHeader.from_bytes(th_data)
        except:
            pass
    
    # Read player entity
    player = None
    if player_ptr and player_ptr > 0x80000000:
        log(f"Reading player entity at {player_ptr:#010x}")
        try:
            player_data = pcsx.read_ram(player_ptr, 0x1B4)
            
            def p_u16(off):
                return struct.unpack('<H', player_data[off:off+2])[0]
            def p_s16(off):
                return struct.unpack('<h', player_data[off:off+2])[0]
            def p_u32(off):
                return struct.unpack('<I', player_data[off:off+4])[0]
            
            facing_val = player_data[PLAYER_OFFSETS["facing"]] if PLAYER_OFFSETS["facing"] < len(player_data) else 0
            
            player = PlayerState(
                address=player_ptr,
                x=p_s16(PLAYER_OFFSETS["x_pos"]),
                y=p_s16(PLAYER_OFFSETS["y_pos"]),
                velocity_x=p_s16(PLAYER_OFFSETS["x_velocity"]),
                velocity_y=p_s16(PLAYER_OFFSETS["y_velocity"]),
                width=p_u16(PLAYER_OFFSETS["width"]),
                height=p_u16(PLAYER_OFFSETS["height"]),
                facing="left" if facing_val else "right",
                sprite_id=p_u32(PLAYER_OFFSETS["sprite_id"]),
                current_sprite_id=p_u32(PLAYER_OFFSETS["sprite_id_current"]),
                anim_frame=p_u16(PLAYER_OFFSETS["anim_frame_index"]),
                anim_frame_end=p_u16(PLAYER_OFFSETS["anim_frame_end"]),
                frame_delay=p_u16(PLAYER_OFFSETS["frame_delay"]),
                anim_flags=p_u16(PLAYER_OFFSETS["anim_flags"]),
                state_offset=p_u16(PLAYER_OFFSETS["state_offset"]),
                state_index=p_u16(PLAYER_OFFSETS["state_index"]),
                callback_ptr=p_u32(PLAYER_OFFSETS["callback_ptr"]),
                anim_table_ptr=p_u32(PLAYER_OFFSETS["anim_frames_table"]),
            )
            
            log(f"Player pos: ({player.x}, {player.y})")
            log(f"Player sprite: {player.sprite_id:#010x}, frame {player.anim_frame}/{player.anim_frame_end}")
        except Exception as e:
            log(f"Failed to read player: {e}")
    
    # Read entity definitions
    entity_defs = []
    entity_count = 0
    entities_ptr = level_context_pointers.get("entities", 0)
    if entities_ptr and entities_ptr > 0x80000000:
        log(f"Reading entity definitions at {entities_ptr:#010x}")
        try:
            # Read up to max_entities * 24 bytes
            ent_data = pcsx.read_ram(entities_ptr, max_entities * 24)
            for i in range(0, len(ent_data), 24):
                chunk = ent_data[i:i+24]
                if len(chunk) < 24:
                    break
                
                ent = EntityDef.from_bytes(chunk)
                if ent and ent.x1 == 0xFFFF:
                    # End marker
                    break
                if ent and ent.x1 < ent.x2 and ent.y1 < ent.y2:
                    entity_defs.append(ent)
                    entity_count += 1
            
            log(f"Found {entity_count} entity definitions")
        except Exception as e:
            log(f"Failed to read entities: {e}")
    
    return GameSnapshot(
        timestamp=datetime.now().isoformat(),
        emulator_running=running,
        level_count=level_count,
        movie_count=movie_count,
        game_mode=game_mode,
        current_level_index=current_level,
        blb_sector=blb_sector,
        gamestate_address=gs_addr,
        camera_x=camera_x,
        camera_y=camera_y,
        entity_tick_list=entity_tick,
        entity_render_list=entity_render,
        player_ptr=player_ptr,
        level_context_address=ldc_addr,
        tile_header=tile_header,
        level_context_pointers=level_context_pointers,
        player=player,
        entity_defs=entity_defs,
        entity_count=entity_count,
    )

def format_snapshot(snap: GameSnapshot) -> str:
    """Format snapshot as human-readable text."""
    lines = []
    lines.append("=" * 70)
    lines.append(f"SKULLMONKEYS RAM SNAPSHOT - {snap.timestamp}")
    lines.append("=" * 70)
    lines.append("")
    
    lines.append("EMULATOR STATUS")
    lines.append(f"  Running: {snap.emulator_running}")
    lines.append("")
    
    lines.append("BLB HEADER")
    lines.append(f"  Levels: {snap.level_count}")
    lines.append(f"  Movies: {snap.movie_count}")
    lines.append(f"  Game Mode: {snap.game_mode} ({'level' if snap.game_mode == 3 else 'special' if snap.game_mode == 6 else 'unknown'})")
    lines.append(f"  Current Level: {snap.current_level_index}")
    lines.append(f"  BLB Sector: {snap.blb_sector} ({snap.blb_sector:#x})")
    lines.append("")
    
    lines.append("GAMESTATE")
    lines.append(f"  Address: {snap.gamestate_address:#010x}")
    lines.append(f"  Camera: ({snap.camera_x}, {snap.camera_y})")
    lines.append(f"  Entity Tick List: {snap.entity_tick_list:#010x}")
    lines.append(f"  Entity Render List: {snap.entity_render_list:#010x}")
    lines.append(f"  Player Ptr: {snap.player_ptr:#010x}")
    lines.append("")
    
    if snap.tile_header:
        th = snap.tile_header
        lines.append("TILE HEADER")
        lines.append(f"  BG Color: {th.bg_color:#06x}")
        lines.append(f"  Spawn: ({th.spawn_x}, {th.spawn_y})")
        lines.append(f"  Tiles 16x16: {th.tile_count_16x16}")
        lines.append(f"  Tiles 8x8: {th.tile_count_8x8}")
        lines.append(f"  Tiles Extra: {th.tile_count_extra}")
        lines.append(f"  Layers: {th.layer_count}")
        lines.append("")
    
    if snap.player:
        p = snap.player
        lines.append("PLAYER (Klayman)")
        lines.append(f"  Address: {p.address:#010x}")
        lines.append(f"  Position: ({p.x}, {p.y})")
        lines.append(f"  Velocity: ({p.velocity_x}, {p.velocity_y})")
        lines.append(f"  Size: {p.width}x{p.height}")
        lines.append(f"  Facing: {p.facing}")
        lines.append(f"  Sprite ID: {p.sprite_id:#010x}")
        lines.append(f"  Current Sprite: {p.current_sprite_id:#010x}")
        lines.append(f"  Animation: frame {p.anim_frame}/{p.anim_frame_end}, delay={p.frame_delay}")
        lines.append(f"  Anim Flags: {p.anim_flags:#06x}")
        lines.append(f"  State: offset={p.state_offset:#x}, index={p.state_index}")
        lines.append(f"  Callback: {p.callback_ptr:#010x}")
        lines.append(f"  Anim Table: {p.anim_table_ptr:#010x}")
        lines.append("")
    
    lines.append("LEVEL CONTEXT POINTERS")
    lines.append(f"  Address: {snap.level_context_address:#010x}")
    for name, ptr in snap.level_context_pointers.items():
        if ptr and ptr > 0x80000000:
            lines.append(f"  {name}: {ptr:#010x}")
    lines.append("")
    
    if snap.entity_defs:
        lines.append(f"ENTITY DEFINITIONS ({snap.entity_count} total)")
        for i, ent in enumerate(snap.entity_defs[:20]):  # Show first 20
            lines.append(f"  [{i:2d}] type={ent.entity_type:3d} layer={ent.layer} "
                        f"pos=({ent.x_center:5d},{ent.y_center:5d}) "
                        f"bounds=({ent.x1},{ent.y1})-({ent.x2},{ent.y2}) "
                        f"var={ent.variant}")
        if len(snap.entity_defs) > 20:
            lines.append(f"  ... and {len(snap.entity_defs) - 20} more")
        lines.append("")
    
    lines.append("=" * 70)
    return "\n".join(lines)

def snapshot_to_dict(snap: GameSnapshot) -> Dict[str, Any]:
    """Convert snapshot to JSON-serializable dict."""
    result = {
        "timestamp": snap.timestamp,
        "emulator_running": snap.emulator_running,
        "blb_header": {
            "level_count": snap.level_count,
            "movie_count": snap.movie_count,
            "game_mode": snap.game_mode,
            "current_level_index": snap.current_level_index,
            "blb_sector": snap.blb_sector,
        },
        "gamestate": {
            "address": f"{snap.gamestate_address:#010x}",
            "camera_x": snap.camera_x,
            "camera_y": snap.camera_y,
            "entity_tick_list": f"{snap.entity_tick_list:#010x}",
            "entity_render_list": f"{snap.entity_render_list:#010x}",
            "player_ptr": f"{snap.player_ptr:#010x}",
        },
        "level_context": {
            "address": f"{snap.level_context_address:#010x}",
            "pointers": {k: f"{v:#010x}" for k, v in snap.level_context_pointers.items() if v},
        },
    }
    
    if snap.tile_header:
        result["tile_header"] = asdict(snap.tile_header)
    
    if snap.player:
        p = snap.player
        result["player"] = {
            "address": f"{p.address:#010x}",
            "position": {"x": p.x, "y": p.y},
            "velocity": {"x": p.velocity_x, "y": p.velocity_y},
            "size": {"width": p.width, "height": p.height},
            "facing": p.facing,
            "sprite_id": f"{p.sprite_id:#010x}",
            "current_sprite_id": f"{p.current_sprite_id:#010x}",
            "animation": {
                "frame": p.anim_frame,
                "frame_end": p.anim_frame_end,
                "delay": p.frame_delay,
                "flags": f"{p.anim_flags:#06x}",
            },
            "state": {
                "offset": f"{p.state_offset:#x}",
                "index": p.state_index,
            },
            "callback_ptr": f"{p.callback_ptr:#010x}",
            "anim_table_ptr": f"{p.anim_table_ptr:#010x}",
        }
    
    result["entity_definitions"] = {
        "count": snap.entity_count,
        "entities": [
            {
                "type": e.entity_type,
                "layer": e.layer,
                "center": {"x": e.x_center, "y": e.y_center},
                "bounds": {"x1": e.x1, "y1": e.y1, "x2": e.x2, "y2": e.y2},
                "variant": e.variant,
            }
            for e in snap.entity_defs
        ],
    }
    
    return result

# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description="Capture RAM snapshot from PCSX-Redux")
    parser.add_argument("--output", "-o", help="Output JSON file")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--host", default="localhost", help="PCSX-Redux host")
    parser.add_argument("--port", type=int, default=8080, help="PCSX-Redux port")
    parser.add_argument("--max-entities", type=int, default=100, help="Max entities to read")
    args = parser.parse_args()
    
    try:
        pcsx = PCSXRedux(args.host, args.port)
        snapshot = capture_snapshot(pcsx, max_entities=args.max_entities, verbose=args.verbose)
        
        # Print human-readable output
        print(format_snapshot(snapshot))
        
        # Save JSON if requested
        if args.output:
            with open(args.output, 'w') as f:
                json.dump(snapshot_to_dict(snapshot), f, indent=2)
            print(f"Saved JSON to: {args.output}")
        
    except ConnectionError as e:
        print(f"Error: {e}", file=sys.stderr)
        print("Make sure PCSX-Redux is running with web server enabled.", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
